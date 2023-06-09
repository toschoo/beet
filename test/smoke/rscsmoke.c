/* ========================================================================
 * Simple Tests for host index with resource
 * ========================================================================
 */
#include <beet/index.h>

#include <common/math.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define BASE "rsc"
#define IDX "idx40"

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

beet_config_t config;

int createIndex(char *base, char *path, beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create(base, path, 1, cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
	return 0;
}

beet_index_t openIndex(char *base, char *path, void *handle) {
	beet_err_t   err;
	beet_index_t idx=NULL;
	beet_open_config_t cfg;

	cfg.leafCacheSize = BEET_CACHE_IGNORE;
	cfg.intCacheSize = BEET_CACHE_IGNORE;
	cfg.compare = NULL;
	cfg.rscinit = NULL;
	cfg.rscdest = NULL;

	err = beet_index_open(base, path, handle, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int createDropIndex(char *base, char *path) {
	beet_err_t err;
	if (createIndex(base, path, &config) != 0) return -1;
	err = beet_index_drop(base, path);
	if (err != BEET_OK) {
		errmsg(err, "cannot drop index");
		return -1;
	}
	return 0;
}

int writeRange(beet_index_t idx, int lo, int hi) {
	beet_err_t err;

	fprintf(stderr, "writing %d to %d\n", lo, hi);
	for(double n=lo; n<hi; n++) {
		double k = sqrt(n);
		err = beet_index_upsert(idx, &k, &n);
		if (err != BEET_OK) {
			errmsg(err, "cannot write");
			return -1;
		}
	}
	return 0;
}

int testDoesExist(beet_index_t idx, int hi) {
	beet_err_t err;
	double k=0, r;
	for(int i=0; i<25; i++) {
		do k=rand()%hi; while(k==0);
		r = sqrt(k);
		// fprintf(stderr, "searching %.4f\n", r);
		err = beet_index_doesExist(idx, &r);
		if (err != BEET_OK) {
			errmsg(err, "does not exist");
			return -1;
		}
	}
	return 0;
}

int testRead(beet_index_t idx, int hi) {
	beet_err_t err;
	double k=0, r, d;
	for(int i=0; i<25; i++) {
		do k=rand()%hi; while(k==0);
		r = sqrt(k);
		err = beet_index_copy(idx, &r, &d);
		if (err != BEET_OK) {
			errmsg(err, "does not exist");
			return -1;
		}
		if (d != k) {
			fprintf(stderr, "% .4f != % .4f\n", k, d);
			return -1;
		}
		// fprintf(stderr, "found %.4f %.4f == % .4f\n", r, k, d);
	}
	return 0;
}

int dontfind(beet_index_t idx, int hi) {
	beet_err_t err;
	double     k=0;

	for(double i=hi;i<2*hi;i++) {
		k=sqrt(i);
		err = beet_index_doesExist(idx, &k);
		if (err == BEET_OK) {
			fprintf(stderr, "found: % .4f\n", i);
			return -1;
		}
		if (err != BEET_ERR_KEYNOF) {
			errmsg(err, "wrong error");
			return -1;
		}
	}
	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;
	beet_index_t idx;
	int haveIndex = 0;
	void *handle = NULL;

	srand(time(NULL) ^ (uint64_t)&printf);

	config.indexType = BEET_INDEX_PLAIN;
	config.leafPageSize = 256;
	config.intPageSize = 256;
	config.leafNodeSize = 14;
	config.intNodeSize = 14;
	config.leafCacheSize = 10000;
	config.intCacheSize = 10000;
	config.keySize = 8;
	config.dataSize = 8;
	config.subPath = NULL;
	config.compare = "beetSmokeDoubleCompare";
	config.rscinit = "beetSmokeDoubleInit";
	config.rscdest = "beetSmokeDoubleDestroy";

	handle = beet_lib_init("libcmp.so");
	if (handle == NULL) {
		fprintf(stderr, "cannot open library\n");
		return EXIT_FAILURE;
	}
	if (createIndex(BASE, IDX, &config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	idx = openIndex(BASE, IDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	
	/* test with 13 (key,value) pairs */
	if (writeRange(idx, 0, 13) != 0) {
		fprintf(stderr, "writeRange 1-13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, 13) != 0) {
		fprintf(stderr, "testDoesExist 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testRead(idx, 13) != 0) {
		fprintf(stderr, "testRead 13 failed\n");
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
	if (testRead(idx, 64) != 0) {
		fprintf(stderr, "testRead 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 99 (key,value) pairs */
	if (writeRange(idx, 50, 99) != 0) {
		fprintf(stderr, "writeRange 50-99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* close/open in between */
	beet_index_close(idx); haveIndex = 0;
	idx = openIndex(BASE, IDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex (2) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	if (testDoesExist(idx, 99) != 0) {
		fprintf(stderr, "testDoesExist 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testRead(idx, 99) != 0) {
		fprintf(stderr, "testRead 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 200 (key,value) pairs */
	if (writeRange(idx, 99, 200) != 0) {
		fprintf(stderr, "writeRange 99-200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, 200) != 0) {
		fprintf(stderr, "testDoesExist 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testRead(idx, 200) != 0) {
		fprintf(stderr, "testRead 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (dontfind(idx, 200) != 0) {
		fprintf(stderr, "dontfind 200 failed\n");
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
