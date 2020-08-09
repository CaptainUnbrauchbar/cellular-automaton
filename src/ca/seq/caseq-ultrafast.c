/* (c) 1996, 1997 Ingo Boesnach, Peter Sanders */
/* simulate a cellular automaton (simplified and improved serial version)
 * periodic boundaries
 *
 * use graphical display
 *
 * #1: Number of lines
 * #2: Number of iterations to be simulated
 * #3: Length of period (in iterations) between displayed states.
 *     0 means that no state is displayed.
 * #4: Name of X Display. E.g. "i90s16.ira.uka.de:0.0"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <D:\Arbeitsverzeichnis\Julia\quellen\Microsoft-MPI-master\src\include\mpi.h>
#include "bitmap.h"
#include "random.h"

#include <assert.h>

/* 1/16 times the XSIZE in caseq-slow (16 states per int) */
#define XSIZE 32

/* "ADT" State and line of states (plus border) */
typedef unsigned int State;
typedef State Line[XSIZE+2];

int displayPeriod;

/* transition array to speed up transition computation */
/* needs to be initialized with void init_transition(void) before using */
static unsigned char transition[4096];

/* determine random integer between 0 and n-1 */
#define randInt(n) ((int)(nextRandomLEcuyer() * n))

/* --------------------- debugging and I/O ---------------------------- */

/* display configuration stored in buf which consists
 * of lines lines on a window using bitmap.h
 */
static void displayConfig(Line *buf, int lines)
{
  int i, x, y;
  /* pack data (-> 8 pixels/byte; necessary for bitmapDisplay) */
  i = 0;
  for (y = 1;  y <= lines;  y++) {
    /* changed: 16 pixel/int -> 2 byte/int */
    for (x = 1;  x <= XSIZE;  x++, i+=2) {
      bitmapBuffer[i] = ((unsigned char)(buf[y][x] >> 23) & 128) |
                        ((unsigned char)(buf[y][x] >> 22) &  64) |
                        ((unsigned char)(buf[y][x] >> 21) &  32) |
                        ((unsigned char)(buf[y][x] >> 20) &  16) |
                        ((unsigned char)(buf[y][x] >> 19) &   8) |
                        ((unsigned char)(buf[y][x] >> 18) &   4) |
                        ((unsigned char)(buf[y][x] >> 17) &   2) |
                        ((unsigned char)(buf[y][x] >> 16) &   1);
      bitmapBuffer[i+1] = ((unsigned char)(buf[y][x] >> 7) & 128) |
                          ((unsigned char)(buf[y][x] >> 6) &  64) |
                          ((unsigned char)(buf[y][x] >> 5) &  32) |
                          ((unsigned char)(buf[y][x] >> 4) &  16) |
                          ((unsigned char)(buf[y][x] >> 3) &   8) |
                          ((unsigned char)(buf[y][x] >> 2) &   4) |
                          ((unsigned char)(buf[y][x] >> 1) &   2) |
                          ((unsigned char)(buf[y][x])      &   1);
    }
  }
  bitmapDisplay();
}

/* --------------------- CA simulation -------------------------------- */

/* random starting configuration */
static void initConfig(Line *buf, int lines)
{
  int i, x, y;
  initRandomLEcuyer(424243);
  for (y = 1;  y <= lines;  y++) {
    for (x = 1;  x <= XSIZE;  x++) {
      buf[y][x] = 0;
      for(i = 30; i >= 0; i-=2)
	buf[y][x] |= (unsigned int)((randInt(100) >= 50) << i);
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
/* changed: transition is now a global array initialized by */
/* void init_transition(void) from main() */
void init_transition(void)
{
  unsigned int i, j, bits, shift;
  unsigned int temp[6];

  /* precompute transition for every possible neighborhood of 4 cells
   * coded as 6 x 2 bit, where each 2-bit field codes the number of
   * nonzero cells in 3 vertically adjacent cells
   */
  for(i = 0; i < 4096; i++) {
    bits  = 3; /* 2-bit mask */
    shift = 0;
    /* extract the 6 2-bit fields from the 12-bit value i */
    for(j = 0;  j < 6; j++,  bits = bits<<2, shift+=2) {
       temp[j] = (i & bits) >> shift;
    }

    /* compute the four simulatenous transitions to be encoded */
    transition[i] = (unsigned char) ((anneal[temp[0]+temp[1]+temp[2]])        |
       		                     (anneal[temp[1]+temp[2]+temp[3]] << 2)   |
       		                     (anneal[temp[2]+temp[3]+temp[4]] << 4)   |
       		                     (anneal[temp[3]+temp[4]+temp[5]] << 6));
  }
}


/* treat torus like boundary conditions */
static void boundary(Line *buf, int lines)
{
  int x,y;
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
{  int x, y;
   unsigned int l, m, r; /* store the paritial sums for
                          * for the 3 currently important lines
                          * l = to the left of the current x
                          * m = middle, r = right
                          */
   boundary(from, lines);
   for (y = 1;  y <= lines;  y++) { /* for each line */
      /* init partial sums for l and m at the beginning of a line */
      l    = from[y-1][  0] + from[y][  0] + from[y+1][  0];
      m    = from[y-1][  1] + from[y][  1] + from[y+1][  1];

      for (x = 1;  x <= XSIZE; x++) { /* for each word in a line */
         /* new partial sum to the right */
         r = from[y-1][x+1] + from[y][x+1] + from[y+1][x+1];

         /* now the 16 states in to[x][y] only depend on
          * the two least significant bits of l,
          * on m and on the two most significant bits of r
          * the 16 new states are computed 4 at time using
          * the precomputed transition table
          */
         to[y][x] = (transition[((l &  3u          ) << 10) |
                                ((m & (1023u << 22)) >> 22)  ] << 24) |
                    (transition[((m & (4095u << 14)) >> 14)  ] << 16) |
                    (transition[((m & (4095u <<  6)) >>  6)  ] <<  8) |
                     transition[((m &  1023u       ) <<  2) |
                                ((r & (   3u << 30)) >> 30)  ];

         l = m; m = r; /* shift for next iteration */
      }
   }
}


/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{
  int i, lines, its;
  double startTime, time;
  Line *from, *to, *temp;

  /* init */
  MPI_Init(&argc, &argv);
  assert(argc == 5);
  lines = atoi(argv[1]);
  its   = atoi(argv[2]);
  displayPeriod = atoi(argv[3]);
  from = malloc((lines + 2) * sizeof(Line));
  to   = malloc((lines + 2) * sizeof(Line));
  init_transition();
  initConfig(from, lines);
  if (displayPeriod) {
     bitmapInit(16*XSIZE, lines, argv[4]);
     displayConfig(from, lines);
  }

  /* measurement loop */
  startTime = MPI_Wtime();
  for (i = 0;  i < its;  i++) {
    simulate(from, to, lines);
    temp = from;  from = to;  to = temp;
    if (displayPeriod)
      if (i % displayPeriod == 0)
       	displayConfig(from, lines);
  }

  time = (double) lines*(16*XSIZE) / (MPI_Wtime() - startTime) * its;
  printf("%d lines, %d iterations, display period=%d.\n",
	 lines, its, displayPeriod);
  printf("%f cells per second %s\n", time,
         displayPeriod?"(but the states have been displayed)":"");
  if (displayPeriod) {
    puts("Press Enter to exit from program.");
    scanf("%c", &i);
  }
  free(from);
  free(to);
  MPI_Finalize();
  return 1;
}


