/* ========================================================================
 * Simple Tests for rangescan
 * ========================================================================
 */
#include <beet/config.h>
#include <beet/index.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BASE "rsc"
#define IDX "idx50"

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

beet_config_t config;

int createIndex(beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create(BASE, IDX, 1, cfg);
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

	err = beet_index_open(BASE, IDX, handle, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int createDropIndex() {
	beet_err_t err;
	if (createIndex(&config) != 0) return -1;
	err = beet_index_drop(BASE, IDX);
	if (err != BEET_OK) {
		errmsg(err, "cannot drop index");
		return -1;
	}
	return 0;
}

int writeRange(beet_index_t idx, int lo, int hi) {
	beet_err_t err;

	fprintf(stderr, "writing %d to %d\n", lo, hi);
	for(int i=lo; i<hi; i++) {
		err = beet_index_insert(idx, &i, &i);
		if (err != BEET_OK) {
			errmsg(err, "cannot insert");
			return -1;
		}
	}
	return 0;
}

int testDoesExist(beet_index_t idx, int hi) {
	beet_err_t err;
	int k;
	for(int i=0; i<100; i++) {
		k=rand()%hi;

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
	int expected;
	int f,t;
	beet_range_t range;
	beet_range_t *ptr=NULL;

	range.fromkey = NULL;
	range.tokey = NULL;

	if (dir == BEET_DIR_ASC) {
		f = 0; t = hi-1;
	} else {
		t = 0; f = hi-1;
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
	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	err = beet_index_range(idx, ptr, dir, iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		beet_iter_destroy(iter);
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
	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	err = beet_index_range(idx, rptr, dir, iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise iter");
		beet_iter_destroy(iter);
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

	config.indexType = BEET_INDEX_PLAIN;
	config.leafPageSize = 128;
	config.intPageSize = 128;
	config.leafNodeSize = 14;
	config.intNodeSize = 14;
	config.leafCacheSize = 10000;
	config.intCacheSize = 10000;
	config.keySize = 4;
	config.dataSize = 4;
	config.subPath = NULL;
	config.compare = "beetSmokeIntCompare";
	config.rscinit = NULL;
	config.rscdest = NULL;

	handle = beet_lib_init("libcmp.so");
	if (handle == NULL) {
		fprintf(stderr, "could not load library\n");
		return EXIT_FAILURE;
	}
	if (createDropIndex() != 0) {
		fprintf(stderr, "createDropIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (createIndex(&config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	idx = openIndex(IDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	
	/* test with 13 (key,value) pairs */
	if (writeRange(idx, 0, 13) != 0) {
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
	if (rangeAll(idx, BEET_DIR_DESC, 13) != 0) {
		fprintf(stderr, "rangeAll (desc) 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 5, 10, BEET_DIR_ASC, 13) != 0) {
		fprintf(stderr, "range 5-10 (asc) 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 10, 5, BEET_DIR_DESC, 13) != 0) {
		fprintf(stderr, "range 10-5 (desc) 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 64 (key,value) pairs */
	if (writeRange(idx, 13, 64) != 0) {
		fprintf(stderr, "writeRange 13-64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, 64) != 0) {
		fprintf(stderr, "testDoesExist 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 64) != 0) {
		fprintf(stderr, "rangeAll 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 64) != 0) {
		fprintf(stderr, "rangeAll (desc) 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 10, 50, BEET_DIR_ASC, 64) != 0) {
		fprintf(stderr, "range 10-50 (desc) 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 50, 10, BEET_DIR_DESC, 64) != 0) {
		fprintf(stderr, "range 50-10 (desc) 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 99 (key,value) pairs */
	if (writeRange(idx, 50, 99) != 0) {
		fprintf(stderr, "writeRange 50-99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* close/open in between */
	beet_index_close(idx); haveIndex = 0;
	idx = openIndex(IDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex (2) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	if (testDoesExist(idx, 99) != 0) {
		fprintf(stderr, "testDoesExist 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 99) != 0) {
		fprintf(stderr, "rangeAll 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 99) != 0) {
		fprintf(stderr, "rangeAll (desc) 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 7, 98, BEET_DIR_ASC, 99) != 0) {
		fprintf(stderr, "range 7-98 (desc) 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (range(idx, 98, 7, BEET_DIR_DESC, 99) != 0) {
		fprintf(stderr, "range 98-7 (desc) 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 200 (key,value) pairs */
	if (writeRange(idx, 99, 200) != 0) {
		fprintf(stderr, "writeRange 99-200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx,200) != 0) {
		fprintf(stderr, "testDoesExist 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 200) != 0) {
		fprintf(stderr, "rangeAll 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 200) != 0) {
		fprintf(stderr, "rangeAll (desc) 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 300 (key,value) pairs */
	if (writeRange(idx, 150, 300) != 0) {
		fprintf(stderr, "writeRange 150-300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx,300) != 0) {
		fprintf(stderr, "testDoesExist 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_ASC, 300) != 0) {
		fprintf(stderr, "rangeAll 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeAll(idx, BEET_DIR_DESC, 300) != 0) {
		fprintf(stderr, "rangeAll (desc) 300 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeOutOfRange(idx, BEET_DIR_DESC, 301, 0) != 0) {
		fprintf(stderr, "out-of-range 301 (desc) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (writeRange(idx, 305, 350) != 0) {
		fprintf(stderr, "writeRange 305-350 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeOutOfRange(idx, BEET_DIR_DESC, 300, 0) != 0) {
		fprintf(stderr, "out-of-range 300 (desc) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeOutOfRange(idx, BEET_DIR_ASC, 300, 320) != 0) {
		fprintf(stderr, "out-of-range 300 (asc) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (writeRange(idx, 1024, 1100) != 0) {
		fprintf(stderr, "writeRange 1024-1100 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeOutOfRange(idx, BEET_DIR_DESC, 500, 305) != 0) {
		fprintf(stderr, "out-of-range 300 (desc) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (rangeOutOfRange(idx, BEET_DIR_ASC, 500, 1050) != 0) {
		fprintf(stderr, "out-of-range 300 (asc) failed\n");
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
