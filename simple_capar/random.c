#include "random.h"
/* (c) 1996,1997 Thomas Worsch, Peter Sanders */
/* =====================================================================
 * The pseudo random number generator functions in this file are
 * similar to those in 
 *    Numerical Recipes in C, Second Edition, pages 279-282.
 *
 * The RNGs have names nextRandom<something> and they return a Float64.
 *    They are initialized by a call to initRandom<something> which 
 *    take an Int32 as seed.
 */

#define EPS 1.2e-7
#define RNMX (1.0-EPS)
#define IM 2147483647
#define AM ((Float64)1.0/IM)
#define IA 16807
#define IQ 127773
#define IR 2836

static Int32 state = 123456789;

void initRandomParkMiller(Int32 seed)
{
  state = seed;
  /* but we have to make sure that state never ever is set to zero */
  if (state==0) { state = 42; }
}

Float64 nextRandomParkMiller(void)
{
  Int32 k;
  Float64 result;

  k = state/IQ;
  state = IA*(state-k*IQ)-k*IR;
  if (state < 0) { state += IM; }
  result = AM*state;
  if (result >= 1.0) { result = RNMX; }
  return result;
}

/* =====================================================================
 * Now a pseudo RNG with a longer period (roughly 10^18) by lEcuyer
 *    (Numerical Recipes, page 282).
 */

#define IM1 2147483563
#define IM2 2147483399
#define AM1 ((Float64)1.0/IM1)
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791

#define NTAB 32
#define NDIV (1+IMM1/NTAB)

static Int32 state1 = 987654321;
static Int32 state2;
static Int32 y;
static Int32 v[NTAB];

/* ------------------------------------------------------------------ */
static void initRandomSeedLEcuyer(Int32 seed)
{
  state1 = seed;
  if (state1==0) { state1 = 987654321; }
  state2 = state1;
}

/* ------------------------------------------------------------------ */
static void initRandomTabLEcuyer(void)
{
  Int32 j, k;

  for (j=NTAB+7;  j>=0;  j--) {
    k = state1/IQ1;
    state1 = IA1*(state1-k*IQ1)-k*IR1;
    if (state1 < 0) { state1 += IM1; }
    if (j < NTAB) { v[j] = state1; }
  }
  y = v[0];
}

/* ------------------------------------------------------------------ */
void initRandomLEcuyer(Int32 seed)
{
  initRandomSeedLEcuyer(seed);
  initRandomTabLEcuyer();
}

/* ------------------------------------------------------------------ */
static Int32 power(Int32 base, Card64 exp, Int32 modulus)
{
  Int64 temp = 1;
  Card64 mask;

  if (base < 0) { return 0; }

  /* note that at on each entry into the following loop body, the 
     actual value of temp is always positive and fits into an Int32 */
  for (mask = ((Card64)1) << 63;  mask != 0;  mask >>= 1) {
    temp = (temp * temp) % modulus;
    if (exp & mask) {
      temp = (temp * base) % modulus;
    }
  }
  return ((Int32) temp);
}

/* ------------------------------------------------------------------ */
static void forwardRandomLEcuyer(Card64 steps)
{
  Int32 a;

  a = power(IA1, steps, IM1);
  state1 = (Int32) ( (((Int64)a) * state1) % IM1);

  a = power(IA2, steps, IM2);
  state2 = (Int32) ( (((Int64)a) * state2) % IM2);
}

/* ------------------------------------------------------------------ */
/*
 * The following initialization function is intended for use on a
 * parallel machine. 
 * Each PE must call initParallelRandomLEcuyer once at the beginning.
 * Calls to nextRandomLEcuyer will then return different (and hopefully 
 *    pseudo unrelated) pseudo random numbers on different PEs.
 * The calls to initParallelRandomLEcuyer on different PEs
 *    *MUST* *SATISFY* *THE* *FOLLOWING* *CONDITIONS*:
 *    - The parameter seed is the same on all PEs.
 *    - The parameter total is the same on all PEs and it must be 
 *      the total number of PEs using the RNG.
 *    - The parameter pe must be different on each PE and for each
 *      i in the set {0,1,...,total-1} there must be exactly one PE
 *      calling initParallelRandomLEcuyer with parameter pe set to i.
 * Please note:
 *    - There are *NO* run time checks to see whether the above
 *      conditions have been satisfied.
 *    - The numbers generated on different PEs are probably only more or 
 *      less unrelated as long as the number of calls of nextRandomLEcuyer
 *      on each of the PEs is smaller than the period of the RNG
 *      (approx. 10^18) divided by the number of PEs (probably below 10^4).
 *      Since 10^14 calls of nextRandomLEcuyer will take quite some time, 
 *      you are probably safe using this RNG.
 *    - This function uses nextRandomParkMiller. If you want to have 
 *      reproducible results, make sure that you do not use
 *      RandomParkMiller yourself before calling initParallelRandomLEcuyer.
 */
void initParallelRandomLEcuyer(Int32 seed, Int32 pe, Int32 total)
{
  Card64 steps;

  initRandomSeedLEcuyer(seed);

  /* The period of the RNG is roughly 2.3e18, i.e. 2^61,
     which should be distributed onto the PEs approximately equally;
     because we do not know the exact value we are careful and take
     one half of the average length of the interval per PE: */
  steps = (((Card64)1) << 60)/total;

  /* For PE number pe we get the starting point: */
  steps = steps * pe;

  /* Finally the starting point for each PE is randomly shifted
     by an amount which is small compared to the length of
     its interval (as long as there are much less than 2^30 PEs :-).
     Therefore steps will still be far below the end of its interval: */
  steps = steps + (Card64) (nextRandomParkMiller() * (((Card64)1) << 30));
     
  /* Now the RNG is initialized for PE pe as if it had already made steps 
     many steps from the initial seed. */
  forwardRandomLEcuyer(steps);

  initRandomTabLEcuyer();
}

/* ------------------------------------------------------------------ */
Float64 nextRandomLEcuyer(void)
{
  Int32 k;
  Float64 result;
  int j;

  k = state1/IQ1;
  state1 = IA1*(state1-k*IQ1)-k*IR1;
  if (state1 < 0) { state1 += IM1; }

  k = state2/IQ2;
  state2 = IA2*(state2-k*IQ2)-k*IR2;
  if (state2 < 0) { state2 += IM2; }

  j = y/NDIV;
  y = v[j] - state2;
  v[j] = state1;

  if (y < 1) { y += IMM1; }

  result = AM1*y;
  if (result >= 1.0) { result = RNMX; }
  return result;
}
