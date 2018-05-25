/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Error Helpers
 * ========================================================================
 */
#include <beet/types.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

const char *beet_oserrdesc() {
	return strerror(errno);
}

const char *beet_errdesc(beet_err_t err) {
	switch(err) {
	case BEET_ERR_NOMEM: return "out of memory";
	case BEET_ERR_INVALID:  return "invalid parameter";

	case BEET_OSERR_BUSY: return "resource is busy";
	case BEET_OSERR_NOMEM: return "out of memory";
	case BEET_OSERR_INVAL: return "invalid parameter to POSIX service";
	case BEET_OSERR_AGAIN: return "try again";
	case BEET_OSERR_PERM: return "insufficient permission";
	case BEET_OSERR_DEADLK: return "deadlock detected";
	case BEET_OSERR_INTRP: return "service was interrupted";
	
	case BEET_OSERR_SEEK: return "seek error (see errno)";
	case BEET_OSERR_TELL: return "tell error (see errno)";
	case BEET_OSERR_OPEN: return "open error (see errno)";
	case BEET_OSERR_CLOSE: return "close error (see errno)";
	case BEET_OSERR_READ: return "read error (see errno)";
	case BEET_OSERR_WRITE: return "write error (see errno)";
	case BEET_OSERR_FLUSH: return "flush error (see errno)";

	default: return "unknown error";
	}
}

