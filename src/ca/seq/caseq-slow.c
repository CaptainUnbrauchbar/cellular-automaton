/* (c) 1996,1997 Peter Sanders, Ingo Boesnach */
/* simulate a cellular automaton (serial version)
 * periodic boundaries
 *
 * use graphical display
 *
 * #1: Number of lines
 * #2: Number of iterations to be simulated
 * #3: Length of period (in iterations) between displayed states.
 *     0 means that no state is displayed.
 * #4: Name of X Display. E.g. "i90xxx.ira.uka.de:0.0"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "bitmap.h"
#include "random.h"

#include <assert.h>

/* horizontal size of the configuration */
#define XSIZE 256

/* "ADT" State and line of states (plus border) */
typedef char State;
typedef State Line[XSIZE + 2];
#define STATE MPI_CHAR

/* every how many simulation cycles shall the display be updated ? */
int displayPeriod;

/* determine random integer between 0 and n-1 */
#define randInt(n) ((int)(nextRandomLEcuyer() * n))

/* --------------------- debugging and I/O ---------------------------- */

/* display configuration stored in buf which consists
 * of lines lines on a window using bitmap.h
 */
static void displayConfig(Line *buf, int lines)
{  int i, x, y;

   /* pack data (-> 8 pixels/byte; necessary for bitmapDisplay) */
   i = 0;
   for (y = 1;  y <= lines;  y++) {
      for (x = 1;  x <= XSIZE;  x+= 8, i++) {
         /*>^ Dirty trick: this reads up to 7 elements beyond
          *   a line boundary. This is OK because there is always
          *   legally accessible memory follwing if XSIZE >= 4
          */
         bitmapBuffer[i] = ((unsigned char)(buf[y][x + 0]) << 7) |
                           ((unsigned char)(buf[y][x + 1]) << 6) |
                           ((unsigned char)(buf[y][x + 2]) << 5) |
                           ((unsigned char)(buf[y][x + 3]) << 4) |
                           ((unsigned char)(buf[y][x + 4]) << 3) |
                           ((unsigned char)(buf[y][x + 5]) << 2) |
                           ((unsigned char)(buf[y][x + 6]) << 1) |
                           ((unsigned char)(buf[y][x + 7]) << 0);
      }
   }

   bitmapDisplay();
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

   for (y = 1;  y <= lines;  y++) {
      /* copy rightmost column to the buffer column 0 */
      buf[y][0      ] = buf[y][XSIZE];

      /* copy leftmost column to the buffer column XSIZE + 1 */
      buf[y][XSIZE+1] = buf[y][1    ];
   }

   for (x = 1;  x <= XSIZE;  x++) {
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
   Line *from, *to, *temp;

   /* init */
   MPI_Init(&argc, &argv);
   assert(argc == 5);
   lines = atoi(argv[1]);
   its   = atoi(argv[2]);
   displayPeriod = atoi(argv[3]);
   from = malloc((lines + 2) * sizeof(Line));
   to   = malloc((lines + 2) * sizeof(Line));
   initConfig(from, lines);
   if (displayPeriod) bitmapInit(XSIZE, lines, argv[4]);

   /* measurement loop */
   startTime = MPI_Wtime();
   for (i = 0;  i < its;  i++) {
      simulate(from, to, lines);
      temp = from;  from = to;  to = temp;
      if (displayPeriod)
         if (i % displayPeriod == 0)
            displayConfig(from, lines);
   }

   printf("%f cells per second %s\n",
         lines*XSIZE*its / (MPI_Wtime() - startTime),
         displayPeriod?"(but the states have been displayed)":"");
   printf("%d lines, %d iterations, display period=%d.\n",
            lines, its, displayPeriod);
   if (displayPeriod) {
      puts("Press Enter to exit from program.");
      scanf("%c", &i);
   }
   free(from);
   free(to);
   MPI_Finalize();
   return 1;
}
