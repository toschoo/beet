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

