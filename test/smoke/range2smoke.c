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
			if (gcd(i,k) != 1) continue;
			// fprintf(stderr, "writing [%d] %d\n", i, k);
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
			fprintf(stderr, "when reading %d\n", k);
			errmsg(err, "does not exist");
			return -1;
		}
	}
	return 0;
}

int range(beet_index_t idx, int from, int to, beet_dir_t dir, int hi) {
	beet_err_t   err;
	int *k, *d, *z;
	int f, g, o=-1, c=0;
	beet_range_t range;
	beet_range_t *ptr=NULL;
	beet_iter_t iter;

	range.fromkey = NULL;
	range.tokey = NULL;

	f = dir == BEET_DIR_ASC ? 1 : hi-1;
	if (from >= 0) {
		range.fromkey = &from;
		f = from;
		ptr = &range;
	}
	if (to >= 0) {
		range.tokey = &to;
		ptr = &range;
	}
	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	err = beet_index_range(idx, ptr, dir, iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot init iter");
		beet_iter_destroy(iter);
		return -1;
	}
	o = dir == BEET_DIR_ASC ? f-1 : f+1;
	while((err = beet_iter_move(iter, (void**)&k,
	                                  (void**)&d)) == BEET_OK) {
		// fprintf(stderr, "%d\n", *k);
		if (dir == BEET_DIR_ASC) o++; else o--;
		c++;
		if (*k != o) {
			fprintf(stderr,
			"ERROR: not in order: %d - %d\n", o, *k);
			beet_iter_destroy(iter);
			return -1;
		}
		err = beet_iter_enter(iter);
		if (err != BEET_OK) {
			errmsg(err, "cannot enter");
			beet_iter_destroy(iter);
			return -1;
		}
		g = 1; c=0;
		while((err = beet_iter_move(iter,
		                           (void**)&z,
		                           (void**)&d)) == BEET_OK) {
			// fprintf(stderr, "[%d] %d: %d\n", *k, g, *z);
			if (*z != g) {
				fprintf(stderr, 
				"unexpected: %d for %d (%d)\n", *z, *k, g);
				beet_iter_destroy(iter);
				return -1;
			}
			do g++; while(g<*k && gcd(*k,g) != 1);
			c++;
		}
		err = beet_iter_leave(iter);
		if (err != BEET_OK) {
			errmsg(err, "cannot leave");
			beet_iter_destroy(iter); return -1;
		}
		if (c == 0) {
			fprintf(stderr, "nothing found for %d\n", *k);
			beet_iter_destroy(iter); return -1;
		}
		if (g != *k && *k != 1) {
			fprintf(stderr,
			"unexpected last value %d / %d\n", g, *k);
			beet_iter_destroy(iter); return -1;
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

int rangeAll(beet_index_t idx, beet_dir_t dir, int hi) {
	return range(idx, -1, -1, dir, hi);
}

int getSome(beet_index_t idx, int hi) {
	beet_err_t err;
	beet_state_t state;
	beet_iter_t iter;
	int k = 0, g, c;
	int *z;

	err = beet_state_alloc(idx, &state);
	if (err != BEET_OK) {
		errmsg(err, "could not allocate state\n");
		return -1;
	}
	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "could not allocate iter\n");
		beet_state_destroy(state);
		return -1;
	}
	for(int i=0;i<25;i++) {
		do k=rand()%hi; while(k == 0);
		err = beet_index_getIter(idx, state, &k, iter);
		if (err != BEET_OK) {
			errmsg(err, "could not allocate iter\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		g = 1; c=0;
		while((err = beet_iter_move(iter,
		                           (void**)&z,
		                           NULL)) == BEET_OK) {
			// fprintf(stderr, "[%d] %d: %d\n", k, g, *z);
			if (*z != g) {
				fprintf(stderr, 
				"unexpected: %d for %d (%d)\n", *z, k, g);
				beet_iter_destroy(iter);
				beet_state_destroy(state);
				return -1;
			}
			do g++; while(g<k && gcd(k,g) != 1);
			c++;
		}
		if (c == 0) {
			fprintf(stderr, "nothing found for %d\n", k);
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		if (g != k && k != 1) {
			fprintf(stderr,
			"unexpected last value %d / %d\n", g, k);
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		err = beet_iter_reset(iter);
		if (err != BEET_OK) {
			errmsg(err, "could not reset iter\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		err = beet_state_release(state);
		if (err != BEET_OK) {
			errmsg(err, "could not release state\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
	}
	beet_state_destroy(state);
	beet_iter_destroy(iter);
	return 0;
}

int invalidEnter(beet_index_t idx, int hi) {
	beet_err_t err;
	beet_state_t state;
	beet_iter_t iter;
	int k = 0;
	int *z;

	err = beet_state_alloc(idx, &state);
	if (err != BEET_OK) {
		errmsg(err, "could not allocate state\n");
		return -1;
	}
	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "could not allocate iter\n");
		beet_state_destroy(state);
		return -1;
	}
	for(int i=0;i<5;i++) {
		do k=rand()%hi; while(k == 0);
		err = beet_index_getIter(idx, state, &k, iter);
		if (err != BEET_OK) {
			errmsg(err, "could not get iter\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		err = beet_iter_enter(iter);
		if (err == BEET_OK) {
			fprintf(stderr,
			"enter one-level iterator works!\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}

		err = beet_iter_leave(iter);
		if (err == BEET_OK) {
			fprintf(stderr, "leaving one-level iterator works!");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		while((err = beet_iter_move(iter, (void**)&z, NULL)) == BEET_OK) {
			err = beet_iter_enter(iter);
			if (err == BEET_OK) {
				fprintf(stderr,
				"enter one-level iterator works!\n");
				beet_iter_destroy(iter);
				beet_state_destroy(state);
				return -1;
			}

			err = beet_iter_leave(iter);
			if (err == BEET_OK) {
				fprintf(stderr, "leaving one-level iterator works!");
				beet_iter_destroy(iter);
				beet_state_destroy(state);
				return -1;
			}
		}
		err = beet_iter_reset(iter);
		if (err != BEET_OK) {
			errmsg(err, "could not reset iter\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
		err = beet_state_release(state);
		if (err != BEET_OK) {
			errmsg(err, "could not release state\n");
			beet_iter_destroy(iter);
			beet_state_destroy(state);
			return -1;
		}
	}
	beet_iter_destroy(iter);
	beet_state_destroy(state);
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
	if (rangeAll(idx, BEET_DIR_ASC, 13) != 0) {
		fprintf(stderr, "rangeAll 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 13) != 0) {
		fprintf(stderr, "rangeAll 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (getSome(idx, 13) != 0) {
		fprintf(stderr, "getSome 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 99 (key,value) pairs */
	if (writeRange(idx, 13, 99) != 0) {
		fprintf(stderr, "writeRange 13-99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 99) != 0) {
		fprintf(stderr, "rangeAll 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 99) != 0) {
		fprintf(stderr, "rangeAll 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (getSome(idx, 99) != 0) {
		fprintf(stderr, "getSome 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 150 (key,value) pairs */
	if (writeRange(idx, 15, 150) != 0) {
		fprintf(stderr, "writeRange 15-150 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 150) != 0) {
		fprintf(stderr, "rangeAll 150 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 150) != 0) {
		fprintf(stderr, "rangeAll 150 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (getSome(idx, 150) != 0) {
		fprintf(stderr, "getSome 150 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 300 (key,value) pairs */
	if (writeRange(idx, 150, 300) != 0) {
		fprintf(stderr, "writeRange 150-300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 300) != 0) {
		fprintf(stderr, "rangeAll 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 300) != 0) {
		fprintf(stderr, "rangeAll 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (getSome(idx, 300) != 0) {
		fprintf(stderr, "getSome 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (invalidEnter(idx, 300) != 0) {
		fprintf(stderr, "invalidEnter failed\n");
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
