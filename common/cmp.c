/* ========================================================================
 * Compare library for tests
 * ========================================================================
 */
#include <beet/types.h>

#define DEREFINT(x) \
	(*(int*)x)

#define DEREFUINT64(x) \
	(*(int*)x)

char beetSmokeIntCompare(const void *one, const void *two, void *ignore) {
	if (DEREFINT(one) < DEREFINT(two)) return BEET_CMP_LESS;
	if (DEREFINT(one) > DEREFINT(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}

char beetSmokeUInt64Compare(const void *one, const void *two, void *ignore) {
	if (DEREFUINT64(one) < DEREFUINT64(two)) return BEET_CMP_LESS;
	if (DEREFUINT64(one) > DEREFUINT64(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}


