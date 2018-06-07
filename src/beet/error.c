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
	case BEET_ERR_UNKNKEY:  return "key not found";
	case BEET_ERR_NORSC:  return "resource is busy; try again";
	case BEET_ERR_BADF:  return "bad file";
	case BEET_ERR_NOFILE: return "file does not exist";
	case BEET_ERR_NOTREE:
		return "tree parameter is NULL";
	case BEET_ERR_NONODE:
		return "node parameter is NULL";
	case BEET_ERR_NOPAGE:
		return "page parameter is NULL";
	case BEET_ERR_NORIDER:
		return "rider parameter is NULL";
	case BEET_ERR_NOFD:
		return "file parameter is NULL";
	case BEET_ERR_NOSLOT:
		return "key not found in node";
	case BEET_ERR_NOKEY:
		return "key parameter is null";
	case BEET_ERR_NOROOT:
		return "root parameter is null";
	case BEET_ERR_BADSIZE:
		return "bad node size (tree is inconsistent)";
	case BEET_ERR_BADNODE:
		return "bad node (tree is inconsistent)";
	case BEET_ERR_BADPAGE:
		return "bad page (tree is inconsistent)";
	case BEET_ERR_NONAME:
		return "no file name or path given";
	case BEET_ERR_TOOBIG:
		return "file name or path too long";
	case BEET_ERR_NOLATCH:
		return "latch parameter is NULL (this is a bug!)";
	case BEET_ERR_NOLOCK:
		return "lock parameter is NULL (this is a bug!)";
	case BEET_ERR_NOMAGIC:
		return "invalid config file: magic number wrong";
	case BEET_ERR_NOVER:
		return "invalid config file: no version";
	case BEET_ERR_UNKNVER:
		return "invalid config file: unknown version";
	case BEET_ERR_BADCFG:
		return "bad config file";
	case BEET_ERR_UNKNTYP:
		return "unknown index type";
	case BEET_ERR_NOTSUPP:
		return "not supported";
	case BEET_ERR_KEYNOF:
		return "key not found";
	case BEET_ERR_EOF:
		return "end of file";
	case BEET_ERR_NOSYM:
		return "symbol not found";
	case BEET_ERR_NOITER:
		return "iter parameter is NULL";
	case BEET_ERR_NOSUB:
		return "no subtree in iterator";
	case BEET_ERR_NOTLEAF:
		return "leaf operation on nonleaf";
	case BEET_ERR_BADSTAT:
		return "iterator in wrong state";
	case BEET_ERR_TEST:
		return "this is an injected error!";
	case BEET_ERR_PANIC:
		return "PANIC! This is a bug!";

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
	case BEET_OSERR_SLEEP: return "nanosleep error (see errno)";
	case BEET_OSERR_MKDIR: return "mkdir error (see errno)";
	case BEET_OSERR_REMOV: return "remove error (see errno)";

	default: return "unknown error";
	}
}
