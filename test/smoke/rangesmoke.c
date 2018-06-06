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

#define IDX "rsc/idx50"

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

beet_config_t config;

int createIndex(beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create(IDX, 1, cfg);
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

	err = beet_index_open(IDX, handle, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int createDropIndex() {
	beet_err_t err;
	if (createIndex(&config) != 0) return -1;
	err = beet_index_drop(IDX);
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

int rangeAll(beet_index_t idx, int hi) {
	beet_iter_t iter;
	beet_err_t   err;
	int *k, *d, o=-1, c=0;

	err = beet_index_range(idx, NULL, NULL, 
	                  BEET_DIR_ASC, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot create iter");
		return -1;
	}
	while((err = beet_iter_move(iter, (void**)&k,
	                                  (void**)&d)) == BEET_OK) {
		// fprintf(stderr, "%d: %d\n", *k, *d);
		o++; c++;
		if (*k != o) {
			fprintf(stderr,
			"ERROR: not ascending: %d - %d\n", o, *k);
			beet_iter_destroy(iter);
			return -1;
		}
	}
	if (c != hi) {
		fprintf(stderr,
		"range incomplete or too big: %d - %d\n", c, hi);
		beet_iter_destroy(iter); return -1;
	}
	if (err != BEET_ERR_EOF) {
		errmsg(err, "could not move iter");
		beet_iter_destroy(iter);
		return -1;
	}
	beet_iter_destroy(iter);
	return 0;
}

int range(beet_index_t idx, int lo, int hi) {
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
	if (rangeAll(idx, 13) != 0) {
		fprintf(stderr, "rangeAll 13 failed\n");
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
	if (rangeAll(idx, 64) != 0) {
		fprintf(stderr, "rangeAll 64 failed\n");
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
	if (rangeAll(idx, 99) != 0) {
		fprintf(stderr, "rangeAll 99 failed\n");
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
	if (rangeAll(idx, 200) != 0) {
		fprintf(stderr, "rangeAll 200 failed\n");
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
	if (rangeAll(idx, 300) != 0) {
		fprintf(stderr, "rangeAll 300 failed\n");
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
