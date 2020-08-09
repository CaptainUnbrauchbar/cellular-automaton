/* (c) 1997 Peter Sanders */
/* parallel mandelbrot approximation of the set 
 * using random polling dynamic load balancing
 * this is a very basic version,
 * in particular, a server PE is responsible for
 * displaying results and termination detection
 *
 * argv[1]: smallest real part considered    Real(z_0) in the book
 * argv[2]: smallest imaginary part considered Im(z_0) in the book
 * argv[3]: extent of sqare area considered       a_0  in the book
 * argv[4]: resolution                            n    in the book
 * argv[5]: display 
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include "mpi.h"
#include "random.h"
#include "bitmap.h"

/* A small debugging aid. Test condition "c" and print an error if it fails */
#define Assert(c) if(!(c)){printf(\
   "\nAssertion violation %s:%u:" #c "\n", __FILE__, __LINE__);}

#define Debug(A) A
#define Debug1(A) 

/* Tags */
#define DISPLAY   1
#define WORKREQ   2
#define WORK      3
#define DONE      4
#define GOTIT     5
#define ALLGOTIT  6

#define SENDBUFFERSIZE 300000
char sendBuffer[SENDBUFFERSIZE];

int iProc, nProc;

/* encoding of a subproblem as
 * an interval of indices whose interpretation as
 * a set of complex numbers is implied by z_0, extent and resolution */
typedef struct {int start;  int end; } Work;

static Work emptyWork = {1,0};


/*********** Mandelbrot specific code ******************************/

/* mmax in the book */
#define MAXITER 1000
#define LARGE   2.0


int totalSteps = 0;
int maxSteps = 0;

complex z0;
double extent;  /* a_0 in the book */
int resolution; /* n in the book   */
double step;    /* extent / resolution */


/* perform a single Mandelbrot iteration starting at c and return
 * number of iterations
 */
int iterate(int pos)
{  int iter;
   complex c = z0 + complex((double)(pos % resolution) * step, 
                                (double)(pos / resolution) * step);
   complex z = c;

   Debug1(printf("%d->(%f,%f)\n", pos, real(z), imag(z)));
   for (iter = 1;  iter < MAXITER && abs(z) <= LARGE;  iter++) {
      z = z*z + c;
      Debug1(fprintf(stderr, "*"));
   }
   totalSteps += iter;
   if (iter < MAXITER && iter > maxSteps) maxSteps = iter; 
   Debug1(printf("%d\n", iter));
   return iter;
}


/*********** Server Code (Displaying and Termination) **********************/

void server(char *display)
{  int i, x, y, done, length, dummy;
   MPI_Status status;
   int *recvBuffer = (int*)malloc(resolution*resolution*sizeof(int));

   Assert(recvBuffer != 0);
   Debug1(printf("Server started\n"));
   bitmapInit(resolution, resolution, display);
   Debug1(printf("bitmapInit\n"));

   done = 0;
   while (done < resolution*resolution) {/* still some work somewhere */

      /* Get an interval size and a sequence of integers 
       * encoding one element of the Mandelbrot set each.
       */
      MPI_Recv(recvBuffer, resolution*resolution, MPI_INT, 
               MPI_ANY_SOURCE, DISPLAY, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_INT, &length);
      printf("plotting %d pixels out of %d\n", length-1, recvBuffer[0]);

      /* keep track of the total number of pixels finished */
      done += recvBuffer[0]; 

      /* switch on each communicated pixel in the display buffer */
      for (i = 1;  i < length;  i++) {
         x = recvBuffer[i] % resolution;
         y = recvBuffer[i] / resolution;
         bitmapSetPixel(x, resolution - y - 1);
         /* remember: (0,0) of bitmap corresponds to complex (z0 + i*n) */
      }

      /* update display */
      bitmapDisplay();
   }

   /* send termination signal to all workers */
   Debug(printf("Start sending termination signals...\n"));
   for (i = 1;  i < nProc;  i++) {
      MPI_Send(&dummy, 0, MPI_INT, i, DONE, MPI_COMM_WORLD);
   }
   Debug(printf("done\n"));

   /* Wait until all PEs got it */
   for (i = 1;  i < nProc;  i++) {
      MPI_Recv(&dummy, 0, MPI_INT, i, GOTIT, MPI_COMM_WORLD, &status);
   }
   Debug(printf("all got it\n"));
   
   /* tell them */
   for (i = 1;  i < nProc;  i++) {
      MPI_Send(&dummy, 0, MPI_INT, i, ALLGOTIT, MPI_COMM_WORLD);
   }
   Debug(printf("all know they all got it\n"));

   free(recvBuffer);
}


/*********** Worker Code **********************/

/* every how many pixels do you poll the network? 
 * increase this if you have a faster iteration code
 */
#define POLL 1

#define randPE() ((int)(nextRandomLEcuyer() * (nProc - 1)) + 1)

/* split work in old 
 * into old and give
 * such that a nonempty old never gets empty 
 */
void splitWork(Work *old, Work *give)
{  give->end   = old->end;
   give->start = (old->start + old->end)/2 + 1;
   old->end    = give->start - 1;
}


/* work on an interval of integers representing candidate elements
 * of the Mandelbrot set.
 * get new work by random polling when out of work
 */
void worker(Work work)
{  int elem,     /* index where the next Mandelbrot set element found 
                  * is written into result buffer
                  */
       flag,     /* IProbe successful ? */
       dummy, 
       pending,  /* is there an unanswered request of mine */
       done = 0; /* has the server already signalled termination? */
   Work give; 
   int *result;  /* result buffer for the current interval */
   MPI_Status status;

   /* alloc result buffer, keep result[0] free for the number of 
    * pixels looked at which is needed by the server for
    * termination detection
    */
   result = (int*)malloc(resolution * resolution * sizeof(int) + 1);
   Assert(result != 0);

   /* init  random partner selection */
   initParallelRandomLEcuyer(31415, iProc, nProc);
   
   do {
      if (work.start <= work.end) {/* do productive work */
         elem = 1;
         result[0] = work.start; /* remember where this interval started */
         printf("starting work on %d to %d\n", work.start, work.end);
         for (;  work.start <= work.end;  work.start++) {
            if (iterate(work.start) == MAXITER) {
               /* store probable Mandelbrot set element in result buffer */
               result[elem] = work.start; 
               elem++; 
            }
            if (work.start % POLL == 0) { /* serve requests every POLL pixels */
               MPI_Iprobe(MPI_ANY_SOURCE, WORKREQ, MPI_COMM_WORLD, &flag, &status);
               if (flag) { 
                  /* get incoming request */
                  MPI_Recv(&dummy, 0, MPI_INT, 
                           MPI_ANY_SOURCE, WORKREQ, MPI_COMM_WORLD, &status);

                  /* split current work */
                  splitWork(&work, &give); 

                  /* give second part away */
                  MPI_Bsend(&give, 2, MPI_INT, 
                            status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
               }
            }
         }
         /* send results to server */
         result[0] = work.end - result[0] + 1;
         MPI_Send(result, elem, MPI_INT, 0, DISPLAY, MPI_COMM_WORLD);
      } else {/* try to get work */
         /* send a request */
         MPI_Bsend(&dummy, 0, MPI_INT, randPE(), WORKREQ, MPI_COMM_WORLD);
         pending = 1; /* remember that you have to collect the reply */
         /* wait for the reply while properly processing other
          * incoming messages
          */
         do {
            MPI_Recv(&work, 2, MPI_INT, 
                     MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            switch(status.MPI_TAG) {
            case DONE: done    = 1; break;
            case WORK: pending = 0; break;
            case WORKREQ: /* reply with empty piece of work */
               MPI_Bsend(&emptyWork, 2, MPI_INT, 
                         status.MPI_SOURCE, WORK, MPI_COMM_WORLD); break;
               
            }
         } while (pending);
      }
   } while (!done);
   /* OK, now >>>I<<< know that we are done.
    * but I have to serve requests until all PEs have found out 
    */
   MPI_Bsend(&dummy, 0, MPI_INT, 0, GOTIT, MPI_COMM_WORLD);
   do {
      MPI_Recv(&dummy, 0, MPI_INT, 
               MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == WORKREQ) {
         MPI_Bsend(&emptyWork, 2, MPI_INT, 
                   status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
      }
   } while(status.MPI_TAG != ALLGOTIT);

   free(result);
}


/*********************************************************************/

void main(int argc, char **argv)
{  Work work;
   int dummy;

   MPI_Init(&argc, &argv);
   Assert(argc == 6);
   z0         = complex(atof(argv[1]), atof(argv[2]));
   extent     = atof(argv[3]);
   resolution = atof(argv[4]);
   step = extent / resolution;

   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   Assert(nProc > 1);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
   MPI_Buffer_attach(sendBuffer, SENDBUFFERSIZE);
 

   if (iProc == 0) { server(argv[5]); }
   else {
      if (iProc > 1) { worker(emptyWork); }
      else {/* iProc == 1 */
         work.start = 0; 
         work.end = resolution*resolution - 1;
         worker(work);
      }
   }

   getchar();
   MPI_Buffer_detach(sendBuffer, &dummy);
   MPI_Finalize();
}
