/* (c) 1996,1997 Peter Sanders, Ingo Boesnach */
/* simulate a cellular automaton (serial version)
 * periodic boundaries
 * 
 * #1: Number of lines 
 * #2: Number of iterations to be simulated
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h> 
#include <assert.h>

#include "openssl/md5.h"

#include "random.h"


/* horizontal size of the configuration */
#define XSIZE 1024 

/* "ADT" State and line of states (plus border) */
typedef char State;
typedef State Line[XSIZE + 2];

/* determine random integer between 0 and n-1 */
#define randInt(n) ((int)(nextRandomLEcuyer() * n))

/* calc and print MD5 checksum of a memory chunk */
char* getMD5DigestStr(void* buf, size_t buflen)
{
  MD5_CTX ctx;
  unsigned char sum[MD5_DIGEST_LENGTH];
  int i;
  char* retval;
  char* ptr;

  MD5_Init(&ctx);
  MD5_Update(&ctx, buf, buflen);
  MD5_Final(sum, &ctx);

  retval = calloc(MD5_DIGEST_LENGTH * 2 + 1, sizeof(*retval));
  ptr = retval;

  for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
    snprintf(ptr, 3, "%02X", sum[i]);
    ptr += 2;
  }

  return retval;
}

/* --------------------- CA simulation -------------------------------- */

/* random starting configuration */
static void initConfig(Line *buf, int lines)
{  int x, y;

   initRandomLEcuyer(424243);
   for (y = 1;  y <= lines;  y++) {
      for (x = 1;  x <= XSIZE;  x++) {
         buf[y][x] = randInt(100) >= 50;
      }
   }
}

/* annealing rule from ChoDro96 page 34 
 * the table is used to map the number of nonzero
 * states in the neighborhood to the new state
 */
static State anneal[10] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 1};

/* a: pointer to array; x,y: coordinates; result: n-th element of anneal,
      where n is the number of neighbors */
#define transition(a, x, y) \
   (anneal[(a)[(y)-1][(x)-1] + (a)[(y)][(x)-1] + (a)[(y)+1][(x)-1] +\
           (a)[(y)-1][(x)  ] + (a)[(y)][(x)  ] + (a)[(y)+1][(x)  ] +\
           (a)[(y)-1][(x)+1] + (a)[(y)][(x)+1] + (a)[(y)+1][(x)+1]])

/* treat torus like boundary conditions */
static void boundary(Line *buf, int lines)
{  int x,y;

   for (y = 0;  y <= lines+1;  y++) {
      /* copy rightmost column to the buffer column 0 */
      buf[y][0      ] = buf[y][XSIZE];

      /* copy leftmost column to the buffer column XSIZE + 1 */
      buf[y][XSIZE+1] = buf[y][1    ];
   }

   for (x = 0;  x <= XSIZE+1;  x++) {
      /* copy bottommost row to buffer row 0 */
      buf[0][x      ] = buf[lines][x];

      /* copy topmost row to buffer row lines + 1 */
      buf[lines+1][x] = buf[1][x    ];
   }
}

/* make one simulation iteration with lines lines.
 * old configuration is in from, new one is written to to.
 */
static void simulate(Line *from, Line *to, int lines)
{
   int x,y;
  
   boundary(from, lines);
   for (y = 1;  y <= lines;  y++) {
      for (x = 1;  x <= XSIZE;  x++) {
         to[y][x  ] = transition(from, x  , y);
      }
   }
}


/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{  int lines, its;
   int i;
   double startTime; 
   char* hash;
   Line *from, *to, *temp;

   /* init */
   MPI_Init(&argc, &argv);

   assert(argc == 3);

   lines = atoi(argv[1]);
   its   = atoi(argv[2]);

   from = (Line*) calloc((lines + 2), sizeof(Line));
   to   = (Line*) calloc((lines + 2), sizeof(Line));

   initConfig(from, lines);

   /* measurement loop */
   startTime = MPI_Wtime();

   for (i = 0;  i < its;  i++) {
      simulate(from, to, lines);
      temp = from;  from = to;  to = temp;
   } 

   hash = getMD5DigestStr(from[1], sizeof(Line) * (lines));

   printf("hash: %s\ttime: %.1f ms\n", hash, (MPI_Wtime() - startTime) * 1000.0);

   free(from);
   free(to);
   free(hash);

   MPI_Finalize();

   return EXIT_SUCCESS;
}
