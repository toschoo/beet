/* ========================================================================
 * Simple Tests for host index
 * ========================================================================
 */
#include <beet/index.h>

#include <common/math.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BASE   "rsc"
#define EMBIDX "idx20"
#define HOSTIDX "idx30"

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
	beet_pair_t pair;
	beet_err_t err;

	fprintf(stderr, "writing %d to %d\n", lo, hi);
	for(uint64_t n=lo; n<hi; n++) {
		for(uint64_t i=1; i<=n; i++) {
			if (i>1 && gcd(n,i) != 1) continue;
			pair.key = &i;
			pair.data = NULL;
			err = beet_index_insert(idx, &n, &pair);
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
	uint64_t k=0;
	for(int i=0; i<25; i++) {
		do k=rand()%hi; while(k==0);
		err = beet_index_doesExist(idx, &k);
		if (err != BEET_OK) {
			errmsg(err, "does not exist");
			return -1;
		}
	}
	return 0;
}

int deepExists(beet_index_t idx, int hi) {
	beet_err_t err;
	uint64_t k=0;

	for(int i=0; i<25; i++) {
		do k=rand()%hi; while(k==0);

		for(uint64_t z=1;z<=k; z++) {
			if (gcd(k,z) != 1) continue;
			// fprintf(stderr, "reading %lu/%lu\n",k,z);
			err = beet_index_doesExist2(idx, &k, &z);
			if (err != BEET_OK) {
				fprintf(stderr, 
				"%lu does not exist for %lu\n", z, k);
				errmsg(err, "error");
				return -1;
			}
		}
	}
	return 0;
}

int readDeep(beet_index_t idx, int hi) {
	beet_state_t state=NULL;
	beet_err_t err;
	uint64_t k=0;

	err = beet_state_alloc(idx, &state);
	if (err != BEET_OK) {
		errmsg(err, "cannot allocate state");
		return -1;
	}
	for(int i=0; i<25; i++) {
		do k=rand()%hi; while (k==0);

		for(uint64_t z=1;z<=k; z++) {
			if (gcd(k,z) != 1) continue;
			// fprintf(stderr, "reading %lu/%lu\n", k,z);
			err = beet_index_get2(idx, state, BEET_FLAGS_RELEASE,
			                                       &k, &z, NULL);
			if (err != BEET_OK) {
				errmsg(err, "cannot get root from index");
				beet_state_destroy(state);
				return -1;
			}
			beet_state_reinit(state);
		}
	}
	beet_state_destroy(state);
	return 0;
}

int dontfind(beet_index_t idx, int hi) {
	beet_state_t state=NULL;
	beet_err_t err;
	uint64_t k=0;

	for(uint64_t i=hi;i<2*hi;i++) {
		err = beet_index_doesExist(idx, &i);
		if (err == BEET_OK) {
			fprintf(stderr, "found: %lu\n",i);
			return -1;
		}
		if (err != BEET_ERR_KEYNOF) {
			errmsg(err, "wrong error");
			return -1;
		}
	}
	err = beet_state_alloc(idx, &state);
	if (err != BEET_OK) {
		errmsg(err, "cannot allocate state");
		return -1;
	}
	for(int i=0; i<25; i++) {
		do k=rand()%hi; while (k==0);

		for(uint64_t z=2;z<k; z++) {
			if (gcd(k,z) == 1) continue;
			int x = rand()%2;
			if (x) {
				err = beet_index_get2(idx, state,
				              BEET_FLAGS_RELEASE,
			                           &k, &z, NULL);
			} else {
				err = beet_index_doesExist2(idx, &k, &z);
			}
			if (err == BEET_OK) {
				fprintf(stderr, "found %lu for %lu\n",z,k);
				fprintf(stderr, "gcd: %lu\n",gcd(k,z));
				return -1;
			}
			if (err != BEET_ERR_KEYNOF) {
				errmsg(err, "wrong error");
				beet_state_destroy(state);
				return -1;
			}
			beet_state_reinit(state);
		}
	}
	beet_state_destroy(state);
	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;
	beet_index_t idx;
	int haveIndex = 0;
	void *handle = NULL;

	srand(time(NULL) ^ (uint64_t)&printf);

	config.indexType = BEET_INDEX_NULL;
	config.leafPageSize = 256;
	config.intPageSize = 256;
	config.leafNodeSize = 14;
	config.intNodeSize = 14;
	config.leafCacheSize = 10000;
	config.intCacheSize = 10000;
	config.keySize = 8;
	config.dataSize = 0;
	config.subPath = NULL;
	config.compare = "beetSmokeUInt64Compare";
	config.rscinit = NULL;
	config.rscdest = NULL;

	handle = beet_lib_init("libcmp.so");
	if (handle == NULL) {
		fprintf(stderr, "cannot load library\n");
		return EXIT_FAILURE;
	}
	if (createIndex(BASE, EMBIDX, &config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	config.indexType = BEET_INDEX_HOST;
	config.subPath = EMBIDX;
	config.dataSize = 4;

	if (createIndex(BASE, HOSTIDX, &config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	idx = openIndex(BASE, HOSTIDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	
	/* test with 13 (key,value) pairs */
	if (writeRange(idx, 1, 13) != 0) {
		fprintf(stderr, "writeRange 1-13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, 13) != 0) {
		fprintf(stderr, "testDoesExist 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (deepExists(idx, 13) != 0) {
		fprintf(stderr, "deepExists 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (readDeep(idx, 13) != 0) {
		fprintf(stderr, "readDeep 13 failed\n");
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
	if (deepExists(idx, 64) != 0) {
		fprintf(stderr, "testDoesExist 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (readDeep(idx, 64) != 0) {
		fprintf(stderr, "readDeep 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 99 (key,value) pairs */
	if (writeRange(idx, 50, 99) != 0) {
		fprintf(stderr, "writeRange 50-99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* close/open in between */
	beet_index_close(idx); haveIndex = 0;
	idx = openIndex(BASE, HOSTIDX, handle);
	if (idx == NULL) {
		fprintf(stderr, "openIndex (2) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	if (testDoesExist(idx, 99) != 0) {
		fprintf(stderr, "testDoesExist 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (deepExists(idx, 99) != 0) {
		fprintf(stderr, "deepExists 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (readDeep(idx, 99) != 0) {
		fprintf(stderr, "readDeep 99 failed\n");
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
	if (deepExists(idx, 200) != 0) {
		fprintf(stderr, "testDoesExist 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (readDeep(idx, 200) != 0) {
		fprintf(stderr, "readDeep 200 failed\n");
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
