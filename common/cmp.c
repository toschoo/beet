/* ========================================================================
 * Compare library for tests
 * ========================================================================
 */
#include <beet/types.h>

#define DEREF(x) \
	(*(int*)x)

char beetSmokeCompare(const void *one, const void *two) {
	if (DEREF(one) < DEREF(two)) return BEET_CMP_LESS;
	if (DEREF(one) > DEREF(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}


