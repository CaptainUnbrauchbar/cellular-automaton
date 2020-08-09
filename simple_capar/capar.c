/* 
 * Parallel Computing Excercise 4
 *
 * One possible solution for the parallel cellular automaton.
 *
 * Please note, that the approach of distributing and collecting 
 * the data is not very sophisticated and one could use 
 * appropriate MPI calls instead. Moreover, the exchange of
 * neighbour lines just works, but could be improved...
 * All in all, it's a KISS solution.
 *
 * (Note, that this version does not use bit shuffling stuff)
 * 
 * command line arguments
 * #1: Number of lines 
 * #2: Number of iterations to be simulated
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h> 
#include <assert.h>
#include <openssl/md5.h>

#include "random.h"

/* horizontal size of the configuration */
#define XSIZE 1024

/* Random seed */
#define RAND_SEED (424243)

/* tags for communication */
#define TAG_SEND_UPPER_BOUND (1)
#define TAG_SEND_LOWER_BOUND (2)
#define TAG_RESULT (0xCAFE)

#define TAG_RECV_UPPER_BOUND TAG_SEND_LOWER_BOUND
#define TAG_RECV_LOWER_BOUND TAG_SEND_UPPER_BOUND

/* next/prev process in communicator */
#define PREV_PROC(n, numProcs) ((n - 1 + numProcs) % numProcs)
#define SUCC_PROC(n, numProcs) ((n + 1) % numProcs)

/* "ADT" State and line of states (plus border) */
typedef unsigned char State;
typedef State Line[XSIZE + 2];

/* size shortcut */
#define LINE_SIZE (sizeof(Line) / sizeof(State)) 

/* determine random integer between 0 and n-1 */
#define randInt(n) ((int)(nextRandomLEcuyer() * n))

/* --------------------- checksum output -------------------------------- */


/* calc and print MD5 checksum of a memory chunk */
static char* getMD5DigestStr(void* buf, size_t buflen)
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
static void initConfig(Line *buf, int num_lines)
{  
  int x, y;
  initRandomLEcuyer(RAND_SEED);
  for (y = 0;  y < num_lines;  y++) {
    for (x = 1;  x <= XSIZE;  x++) {
      buf[y][x] = randInt(100) >= 50;
    }
  }
}

/* annealing rule from ChoDro96 page 34 
 * the table is used to map the number of nonzero
 * states in the neighborhood to the new state
 */
static unsigned char anneal[10] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 1};

/* a: pointer to array; x,y: coordinates; result: n-th element of anneal,
      where n is the number of neighbors */
#define transition(a, x, y) \
   (anneal[(a)[(y)-1][(x)-1] + (a)[(y)][(x)-1] + (a)[(y)+1][(x)-1] +\
           (a)[(y)-1][(x)  ] + (a)[(y)][(x)  ] + (a)[(y)+1][(x)  ] +\
           (a)[(y)-1][(x)+1] + (a)[(y)][(x)+1] + (a)[(y)+1][(x)+1]])

/* treat torus like boundary conditions */
static void boundary(Line *buf, int line_from, int line_to)
{ 
  int y;

  for (y = line_from;  y <= line_to;  y++) {
     /* copy rightmost column to the buffer column 0 */
     buf[y][0      ] = buf[y][XSIZE];

     /* copy leftmost column to the buffer column XSIZE + 1 */
     buf[y][XSIZE+1] = buf[y][1    ];
  }
}


/* make one simulation iteration with lines lines.
 * old configuration is in from, new one is written to to.
 */
static void simulate(Line *buf_from, Line *buf_to, int lines)
{  
   int x, y;

   boundary(buf_from, 0, lines + 1); 
   for (y = 1;  y <= lines;  y++) {
      for (x = 1;  x <= XSIZE;  x++) {
         buf_to[y][x  ] = transition(buf_from, x, y);
      }
   }
}

/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{
  int i, lines, its, numProcs, numRemainderTasks, rank, tmp;
  int* recvOffsets;
  int* numCalcLines;
  double startTime = 0, time; 
  char* hash;
  Line *cells_from, *cells_to, *temp, *result = NULL;
  MPI_Status status;

  /* init MPI and application */
  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  assert(argc == 3);
  lines = atoi(argv[1]);
  its = atoi(argv[2]);

  tmp = lines / numProcs; /* equals (int) floor(lines / numProcs) */
  
  /* if work can not be distributed equally, distribute the remaining lines equally */
  numCalcLines = (int*) calloc(numProcs, sizeof(*numCalcLines));
  recvOffsets = (int*) calloc(numProcs, sizeof(*recvOffsets));
  numRemainderTasks = lines % numProcs;
  
  for (i = 0; i < numProcs; i++) {
    if (i < numRemainderTasks) {
      recvOffsets[i] = i * (tmp + 1);
      numCalcLines[i] = tmp + 1;
    } else {
      recvOffsets[i] = (tmp + 1) * (numRemainderTasks) + tmp * (i - numRemainderTasks);
      numCalcLines[i] = tmp;
    }
  }

  /* each process calcs only on a piece of the whole problem */
  cells_from = (Line*) calloc((numCalcLines[rank] + 2), sizeof(*cells_from));
  cells_to = (Line*) calloc((numCalcLines[rank] + 2), sizeof(*cells_to));
  result = (Line*) calloc((lines), sizeof(Line));

  initConfig(result, lines);  
  
  /* "scatter" initialized field */ 
  memmove(cells_from[1], result[recvOffsets[rank]], (numCalcLines[rank]) * sizeof(Line));
  
  /* start timing */
  if (rank == 0) {
    startTime = MPI_Wtime();   
  }

  /* actual simulation */
  for (i = 0;  i < its;  i++) {
    MPI_Sendrecv(cells_from[1], LINE_SIZE, MPI_CHAR, PREV_PROC(rank, numProcs), 
      TAG_SEND_UPPER_BOUND, cells_from[numCalcLines[rank] + 1], LINE_SIZE, MPI_CHAR,
      SUCC_PROC(rank, numProcs), TAG_RECV_LOWER_BOUND, MPI_COMM_WORLD,
      MPI_STATUS_IGNORE);

    MPI_Sendrecv(cells_from[numCalcLines[rank]], LINE_SIZE, MPI_CHAR, SUCC_PROC(rank, 
      numProcs), TAG_SEND_LOWER_BOUND, cells_from[0], LINE_SIZE, MPI_CHAR,
      PREV_PROC(rank, numProcs), TAG_RECV_UPPER_BOUND, MPI_COMM_WORLD,
      MPI_STATUS_IGNORE);
  
    simulate(cells_from, cells_to, numCalcLines[rank]);
    temp = cells_from;  cells_from = cells_to;  cells_to = temp;
  }


  /* collect result ("gather") */
  if (rank != 0) {
    MPI_Send(cells_from[1], numCalcLines[rank] * LINE_SIZE, 
     MPI_CHAR, 0, TAG_RESULT, MPI_COMM_WORLD);
  } else {
    for (i = 1; i < numProcs; i++) {
      MPI_Recv(result[recvOffsets[i]], numCalcLines[rank] * LINE_SIZE, 
        MPI_CHAR, i, TAG_RESULT, MPI_COMM_WORLD, &status);
    }
  }

  /* root just copies memory */
  if (rank == 0 && its != 0) {
    for (i = 0; i < numCalcLines[0]; i++) {
      memmove(result[i], cells_from[i + 1], sizeof(Line));
    }
  }

  if (rank == 0) {
    time = (MPI_Wtime() - startTime) * 1000.0;
    hash = getMD5DigestStr(result, lines * sizeof(Line));
    printf("hash: %s\ttime: %.1f ms\n", hash, time);
    free(hash);
  }

  free(result);
  free(cells_from);
  free(cells_to);
  free(recvOffsets);
  free(numCalcLines);

  MPI_Finalize();

  return EXIT_SUCCESS;
}


