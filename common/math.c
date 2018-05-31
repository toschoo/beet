/* =======================================================================
 * Some math puzzles to be used in tests and benchmarks
 * (c) Tobias Schoofs, 2018
 * =======================================================================
 */
#include <time.h>
#include <stdint.h>
#include <math.h>

#include <common/math.h>

/* ------------------------------------------------------------------------
 * computes the first 'num' fibonacci numbers and stores them in buf
 * ------------------------------------------------------------------------
 */
void fibonacci(uint64_t *buf, int num) {
	fibonacci_r(buf, num, 1, 1);
}

/* ------------------------------------------------------------------------
 * computes 'num' fibonacci numbers starting from f1 and f2
 * ------------------------------------------------------------------------
 */
void fibonacci_r(uint64_t *buf, int num, uint64_t f1, uint64_t f2) {
	buf[0] = f1+f2;
	buf[1] = f2+buf[0];

	for(int i=2; i<num; i++) {
		buf[i] = buf[i-1] + buf[i-2];
	}
}

/* ------------------------------------------------------------------------
 * computes the factorial of n 
 * ------------------------------------------------------------------------
 */
uint64_t fac(uint64_t n) {
	uint64_t f = 1;
	for(uint64_t i=2; i<=n; i++) f*=i;
	return f;
}

/* ------------------------------------------------------------------------
 * computes the falling factorial of (n,k) 
 * ------------------------------------------------------------------------
 */
uint64_t falling(uint64_t n, uint64_t k) {
	uint64_t f = 1;
	if (n == 0) return 1;
	if (k == 0) return 1;
	if (k == 1) return n;
	if (k > n) return 0;
	for(uint64_t i=0; i<k; i++) f*=(n-i);
	return f;
}

/* ------------------------------------------------------------------------
 * computes the binomial coefficient (n k)
 * ------------------------------------------------------------------------
 */
uint64_t choose(uint64_t n, uint64_t k) {
	if (k == 0)  return 1;
	if (k == 1) return n;
	if (k > n) return 0;
	if (2*k>n) choose(n, n-k);
	return (falling(n,k)/fac(k));
}

