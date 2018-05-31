/* ========================================================================
 * Some math puzzles to be used in tests and benchmarks
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 */
#ifndef mymath_decl
#define mymath_decl

#include <time.h>
#include <stdint.h>
#include <math.h>

/* ------------------------------------------------------------------------
 * computes the first 'num+2' fibonacci numbers and stores them in buf
 * starting with 2, the third fibonacci number.
 * fibonacci(buf, num) is equivalent to fibonacci_r(buf, num, 1, 1)
 * ------------------------------------------------------------------------
 */
void fibonacci(uint64_t *buf, int num);

/* ------------------------------------------------------------------------
 * computes 'num' fibonacci numbers starting with f1+f2 and f2+f1+f2
 * ------------------------------------------------------------------------
 */
void fibonacci_r(uint64_t *buf, int num, uint64_t f1, uint64_t f2);

/* ------------------------------------------------------------------------
 * computes the factorial of n 
 * ------------------------------------------------------------------------
 */
uint64_t fac(uint64_t n);

/* ------------------------------------------------------------------------
 * computes the falling factorial of (n,k) 
 * ------------------------------------------------------------------------
 */
uint64_t falling(uint64_t n, uint64_t k);

/* ------------------------------------------------------------------------
 * computes the binomial coefficient (n k)
 * ------------------------------------------------------------------------
 */
uint64_t choose(uint64_t n, uint64_t k);

#endif

