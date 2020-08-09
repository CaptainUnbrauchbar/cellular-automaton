#include <limits.h>
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

/* C++ compatibility */
#ifdef __cplusplus
#define CC extern "C"
#else
#define CC
#endif

/* try to find out how 32 bit and 64 bit
 * interger types look like in this compiler
 * this may fail
 * e.g., if the compiler does not support 64 data types...
 */
#if UINT_MAX >> 31 == 1
typedef int Int32;
typedef unsigned int Card32;
#elif USHRT_MAX >> 31 == 1
typedef short Int32;
typedef unsigned short Card32;
#elif ULONG_MAX >> 31 == 1
typedef short Int32;
typedef unsigned short Card32;
#else /* provoke error */
typedef nonexisting Int32;
typedef nonexisting Card32;
#endif

#if UINT_MAX >> 63 == 1
typedef int Int64;
typedef unsigned int Card64;
#elif ULONG_MAX >> 63 == 1
typedef long Int64;
typedef unsigned long Card64;
#else
typedef long long int Int64;
typedef unsigned long long int Card64;
#endif

typedef double Float64;

/* =====================================================================
 * The Minimal Standard pseudo RNG (Numerical Recipes, page 279)
 *    by Park and Miller
 * I added an initialization function and could therefore remove
 *    the MASK mechanism from the original ran0 function.
 */
CC void initRandomParkMiller(Int32 seed);
CC Float64 nextRandomParkMiller (void);


/* =====================================================================
 * Now a pseudo RNG with a longer period (roughly 10^18) by lEcuyer
 *    (Numerical Recipes, page 282).
 */
CC void initRandomLEcuyer(Int32 seed);
CC Float64 nextRandomLEcuyer (void);


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
CC void initParallelRandomLEcuyer(Int32 seed, Int32 pe, Int32 total);
