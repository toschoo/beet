/* ========================================================================
 * Simple Tests for rangescan with embeeded index
 * ========================================================================
 */
#include <beet/config.h>
#include <beet/index.h>

#include <common/math.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define EMBIDX "rsc/idx60"
#define HOSTIDX "rsc/idx70"

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

beet_config_t config;

int createIndex(char *path, char standalone, beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create(path, standalone, cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
	return 0;
}

beet_index_t openIndex(char *path, void *handle) {
	beet_err_t   err;
	beet_index_t idx=NULL;
	beet_open_config_t cfg;

	beet_open_config_ignore(&cfg);

	err = beet_index_open(path, handle, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int writeRange(beet_index_t idx, int lo, int hi) {
	beet_err_t err;
	beet_pair_t  p;

	fprintf(stderr, "writing %d to %d\n", lo, hi);
	for(int i=lo; i<hi; i++) {
		for(int k=1;k<=i;k++) {
			if (k != 1 && gcd(i,k) != 1) continue;
			p.key = &k;
			p.data = NULL;
			err = beet_index_insert(idx, &i, &p);
			if (err != BEET_OK) {
				errmsg(err, "cannot insert");
				return -1;
			}
		}
	}
	return 0;
}

int testDoesExist(beet_index_t idx, int hi) {
	beet_err_t err;
	int k=0;
	for(int i=0; i<100; i++) {
		do k=rand()%hi; while(k==0);

		// fprintf(stderr, "reading %d\n", k);

		err = beet_index_doesExist(idx, &k);
		if (err != BEET_OK) {
			errmsg(err, "cannot copy from index");
			return -1;
		}
	}
	return 0;
}

int range(beet_index_t idx, int from, int to, beet_dir_t dir, int hi) {
	beet_iter_t iter;
	beet_err_t   err;
	int *k, *d, o=-1, c=0;
	int g,z;
	int expected;
	int f,t;
	beet_range_t range;
	beet_range_t *ptr=NULL;

	range.fromkey = NULL;
	range.tokey = NULL;

	if (dir == BEET_DIR_ASC) {
		f = 1; t = hi-1;
	} else {
		t = 1; f = hi-1;
	}

	if (from > 0) {
		range.fromkey = &from;
		f = from;
		ptr = &range;
	}
	if (to > 0) {
		range.tokey = &to;
		t = to;
		ptr = &range;
	}
	err = beet_index_range(idx, NULL, ptr, dir, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	o = dir == BEET_DIR_ASC ? f-1 : f+1;
	while((err = beet_iter_move(iter, (void**)&k,
	                                  (void**)&d)) == BEET_OK) {

		// fprintf(stderr, "%d: %d\n", *k, *d);

		if (dir == BEET_DIR_ASC) o++; else o--;
		c++;
		if (*k != o) {
			fprintf(stderr,
			"ERROR: not in order: %d - %d\n", o, *k);
			beet_iter_destroy(iter);
			return -1;
		}

		fprintf(stderr, "iter: %p\n", iter);

		err = beet_iter_enter(iter);
		if (err != BEET_OK) {
			errmsg(err, "cannot enter");
			return -1;
		}
		g = 1;
		while((err = beet_iter_move(iter,
		                           (void**)&z,
		                           (void**)&d)) == BEET_OK) {
			if (iter == NULL) {
				fprintf(stderr, "iter is NULL!!!\n");
				return -1;
			}
			if (z != g) {
				fprintf(stderr, 
				"unexpected: %d for %d (%d)\n", z, *k, g);
				beet_iter_destroy(iter);
				return -1;
			}
			do g++; while(g<*k && gcd(*k,g) != 1);
		}
		err = beet_iter_leave(iter);
		if (err != BEET_OK) {
			errmsg(err, "cannot leave");
			return -1;
		}
	}
	if (err != BEET_ERR_EOF) {
		errmsg(err, "could not move iter");
		beet_iter_destroy(iter);
		return -1;
	}
	expected = dir == BEET_DIR_ASC ? t-f+1 : f-t+1;
	if (c != expected) {
		fprintf(stderr,
		"range incomplete or too big: %d - %d (%d - %d = %d)\n",
		                                 c, hi, t, f, expected);
		beet_iter_destroy(iter); return -1;
	}
	beet_iter_destroy(iter);
	return 0;
}

int rangeAll(beet_index_t idx, beet_dir_t dir, int hi) {
	return range(idx, -1, -1, dir, hi);
}

int rangeOutOfRange(beet_index_t idx, beet_dir_t dir, int from, int to) {
	beet_iter_t iter;
	beet_err_t   err;
	int *k, *d, o=-1;
	beet_range_t range;
	beet_range_t *rptr=NULL;

	range.fromkey = NULL;
	range.tokey = NULL;

	if (from > 0) {
		range.fromkey = &from;
		rptr = &range;
	}
	if (to > 0) {
		range.tokey = &to;
		rptr = &range;
	}
	err = beet_index_range(idx, NULL, rptr, dir, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	while((err = beet_iter_move(iter, (void**)&k,
	                                  (void**)&d)) == BEET_OK) {

		// fprintf(stderr, "%d: %d\n", *k, *d);

		if (o == -1) {
			o = *k; continue;
		}
		if (dir == BEET_DIR_ASC) o++; else o--;
		if (*k != o) {
			fprintf(stderr,
			"ERROR: not in order: %d - %d\n", o, *k);
			beet_iter_destroy(iter);
			return -1;
		}
	}
	if (err != BEET_ERR_EOF) {
		errmsg(err, "could not move iter");
		beet_iter_destroy(iter);
		return -1;
	}
	beet_iter_destroy(iter);
	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;
	beet_index_t idx;
	int haveIndex = 0;
	void *handle=NULL;

	srand(time(NULL) ^ (uint64_t)&printf);

	config.indexType = BEET_INDEX_NULL;
	config.leafPageSize = 128;
	config.intPageSize = 128;
	config.leafNodeSize = 14;
	config.intNodeSize = 14;
	config.leafCacheSize = 10000;
	config.intCacheSize = 10000;
	config.keySize = 4;
	config.dataSize = 0;
	config.subPath = NULL;
	config.compare = "beetSmokeIntCompare";
	config.rscinit = NULL;
	config.rscdest = NULL;

	handle = beet_lib_init("libcmp.so");
	if (handle == NULL) {
		fprintf(stderr, "could not load library\n");
		return EXIT_FAILURE;
	}
	if (createIndex(EMBIDX, 0, &config) != 0) {
		fprintf(stderr, "createIndex %s failed\n", EMBIDX);
		rc = EXIT_FAILURE; goto cleanup;
	}
	config.indexType = BEET_INDEX_HOST;
	config.dataSize = 4; /* should be obvious from HOST */
	config.subPath = EMBIDX;
	if (createIndex(HOSTIDX, 1, &config) != 0) {
		fprintf(stderr, "createIndex %s failed\n", HOSTIDX);
		rc = EXIT_FAILURE; goto cleanup;
	}
	idx = openIndex(HOSTIDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;

	/* test with 13 (key,value) pairs */
	if (writeRange(idx, 1, 13) != 0) {
		fprintf(stderr, "writeRange 0-13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, 13) != 0) {
		fprintf(stderr, "testDoesExist 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 13) != 0) {
		fprintf(stderr, "rangeAll 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (haveIndex) beet_index_close(idx);
	if (handle != NULL) beet_lib_close(handle);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}
