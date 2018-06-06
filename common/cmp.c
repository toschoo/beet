/* ========================================================================
 * Compare library for tests
 * ========================================================================
 */
#include <beet/types.h>
#include <stdlib.h>

#define DEREFINT(x) \
	(*(int*)x)

#define DEREFUINT64(x) \
	(*(int*)x)

#define DEREFFLOAT(x) \
	(*(double*)x)

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

typedef struct {
	double tolerance;
} beetSmokeDouble_t;

#define DEREFTOL(x) \
	((beetSmokeDouble_t*)x)->tolerance

beet_err_t beetSmokeDoubleInit(void **rsc) {
	*rsc = malloc(sizeof(beetSmokeDouble_t));
	if (*rsc == NULL) return BEET_ERR_NOMEM;
	DEREFTOL(*rsc) = 0.01;
	return BEET_OK;
}

void beetSmokeDoubleDestroy(void **rsc) {
	if (rsc == NULL) return;
	if (*rsc == NULL) return;
	free(*rsc); *rsc = NULL;
}

char beetSmokeDoubleCompare(const void *one, const void *two, void *rsc) {
	if (DEREFFLOAT(one) + DEREFTOL(rsc) < DEREFFLOAT(two)) return BEET_CMP_LESS;
	if (DEREFFLOAT(one) - DEREFTOL(rsc) > DEREFFLOAT(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}




