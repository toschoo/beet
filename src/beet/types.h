/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Base Types
 * ========================================================================
 */
#ifndef beet_types_decl
#define beet_types_decl

/* -----------------------------------------------------------------------
 * Error indicator
 * -----------------------------------------------------------------------
 */
typedef int beet_err_t;

/* -----------------------------------------------------------------------
 * Return a constant string describing the error
 * -----------------------------------------------------------------------
 */
const char *beet_errdesc(beet_err_t err);

/* -----------------------------------------------------------------------
 * Return the OS description of the error in errno
 * -----------------------------------------------------------------------
 */
const char *beet_oserrdesc();

/* -----------------------------------------------------------------------
 * Error constants
 * -----------------------------------------------------------------------
 */
#define BEET_OK 0

#define BEET_ERR_NOMEM    2
#define BEET_ERR_INVALID  3
#define BEET_ERR_UNKNKEY  4
#define BEET_ERR_NORSC    5
#define BEET_ERR_PANIC    6
#define BEET_ERR_BADF     7

/* -----------------------------------------------------------------------
 * Errors returned from OS services
 * -----------------------------------------------------------------------
 */
#define BEET_OSERR_BUSY   -2
#define BEET_OSERR_NOMEM  -3
#define BEET_OSERR_INVAL  -4
#define BEET_OSERR_AGAIN  -5
#define BEET_OSERR_PERM   -6
#define BEET_OSERR_DEADLK -7
#define BEET_OSERR_INTRP  -8

/* -----------------------------------------------------------------------
 * Errors with further information in errno
 * -----------------------------------------------------------------------
 */
#define BEET_OSERR_ERRNO -100
#define BEET_OSERR_SEEK  -101
#define BEET_OSERR_TELL  -102
#define BEET_OSERR_OPEN  -103
#define BEET_OSERR_CLOSE -104
#define BEET_OSERR_READ  -105
#define BEET_OSERR_WRITE -106
#define BEET_OSERR_FLUSH -107
#define BEET_OSERR_SLEEP -108

#define BEET_OSERR_UNKN  -9999

/* -----------------------------------------------------------------------
 * insert (key, data) pairs into embedded b+tree
 * -----------------------------------------------------------------------
 */
typedef struct {
	void *key;
	void *data;
} beet_pair_t;

#endif

