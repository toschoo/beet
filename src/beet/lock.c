/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Lock: read/write locks
 * ========================================================================
 */
#include <beet/lock.h>
#include <stdio.h>
#include <errno.h>

static inline beet_err_t geterr(int x) {
	switch(x) {
	case 0: return 0;
	case EINVAL: return BEET_OSERR_INVAL;
	case EBUSY: return BEET_OSERR_BUSY;
	case ENOMEM: return BEET_OSERR_NOMEM;
	case EAGAIN: return BEET_OSERR_AGAIN;
	case EPERM: return BEET_OSERR_PERM;
	case EDEADLK: return BEET_OSERR_DEADLK;
	case EINTR: return BEET_OSERR_INVAL;
	default: return BEET_OSERR_UNKN;
	}
}

#define LATCHNULL() \
	if (latch == NULL) return BEET_ERR_INVALID;

#define LOCKNULL() \
	if (lock == NULL) return BEET_ERR_INVALID;

#define PTHREADERR(x) \
	if (x != 0) return geterr(x);

/* ------------------------------------------------------------------------
 * init latch
 * ------------------------------------------------------------------------
 */
beet_err_t beet_latch_init(beet_latch_t *latch) {
	LATCHNULL();
	int x = pthread_mutex_init(latch, NULL);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * destroy latch
 * ------------------------------------------------------------------------
 */
void beet_latch_destroy(beet_latch_t *latch) {
	if (latch == NULL) return;
	pthread_mutex_destroy(latch);
}

/* ------------------------------------------------------------------------
 * lock latch
 * ------------------------------------------------------------------------
 */
beet_err_t beet_latch_lock(beet_latch_t *latch) {
	LATCHNULL();
	int x = pthread_mutex_lock(latch);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * unlock latch
 * ------------------------------------------------------------------------
 */
beet_err_t beet_latch_unlock(beet_latch_t *latch) {
	LATCHNULL();
	int x = pthread_mutex_unlock(latch);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * init read/write lock
 * ------------------------------------------------------------------------
 */
beet_err_t beet_lock_init(beet_lock_t *lock) {
	LOCKNULL();
	int x = pthread_rwlock_init(lock, NULL);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * destroy read/write lock
 * ------------------------------------------------------------------------
 */
void beet_lock_destroy(beet_lock_t *lock) {
	if (lock == NULL) return;
	pthread_rwlock_destroy(lock);
}

/* ------------------------------------------------------------------------
 * lock read/write lock for reading
 * ------------------------------------------------------------------------
 */
beet_err_t beet_lock_read(beet_lock_t *lock) {
	LOCKNULL();
	int x = pthread_rwlock_rdlock(lock);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * unlock read/write lock acquired for reading
 * ------------------------------------------------------------------------
 */
beet_err_t beet_unlock_read(beet_lock_t *lock) {
	LOCKNULL();
	int x = pthread_rwlock_unlock(lock);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * lock read/write lock for reading
 * ------------------------------------------------------------------------
 */
beet_err_t beet_lock_write(beet_lock_t *lock) {
	LOCKNULL();
	int x = pthread_rwlock_wrlock(lock);
	PTHREADERR(x);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * unlock read/write lock acquired for writing
 * ------------------------------------------------------------------------
 */
beet_err_t beet_unlock_write(beet_lock_t *lock) {
	LOCKNULL();
	int x = pthread_rwlock_unlock(lock);
	PTHREADERR(x);
	return BEET_OK;
}
