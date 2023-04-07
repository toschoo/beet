/* ========================================================================
 * Simple Tests for plain index
 * ========================================================================
 */
#include <beet/config.h>
#include <beet/index.h>

#include <tsalgo/map.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

#define DEREF(x) \
	(*(int*)x)

char compare(const void *one, const void *two, void *ignore) {
	if (DEREF(one) < DEREF(two)) return BEET_CMP_LESS;
	if (DEREF(one) > DEREF(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}

int createIndex(beet_config_t *cfg) {
	beet_err_t err;

	err = beet_index_create("rsc", "idx10", 1, cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
	return 0;
}

beet_index_t openIndex(char *path) {
	beet_err_t   err;
	beet_index_t idx=NULL;
	beet_open_config_t cfg;

	cfg.leafCacheSize = BEET_CACHE_IGNORE;
	cfg.intCacheSize = BEET_CACHE_IGNORE;
	cfg.compare = &compare;
	cfg.rscinit = NULL;
	cfg.rscdest = NULL;

	err = beet_index_open("rsc", "idx10", NULL, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int createDropIndex(beet_config_t *cfg) {
	beet_err_t err;
	if (createIndex(cfg) != 0) return -1;
	err = beet_index_drop("rsc", "idx10");
	if (err != BEET_OK) {
		errmsg(err, "cannot drop index");
		return -1;
	}
	return 0;
}

int writeRange(beet_index_t idx, ts_algo_map_t *hidden, int lo, int hi) {
	beet_err_t err;

	fprintf(stderr, "writing %d to %d\n", lo, hi);
	for(int i=lo; i<hi; i++) {
		err = beet_index_insert(idx, &i, &i);
		if (err != BEET_OK) {
			errmsg(err, "cannot insert");
			return -1;
		}

		uint64_t x = (uint64_t)i;
		if (ts_algo_map_getId(hidden, x) != NULL) {
			if (ts_algo_map_removeId(hidden, x) == NULL) {
				fprintf(stderr, "cannot remove key %d from map\n", i);
				return -1;
			}
		}
	}
	return 0;
}

int easyRead(beet_index_t idx, ts_algo_map_t *hidden, int hi) {
	beet_err_t err;
	int k;
	int d;
	for(int i=0; i<100; i++) {

		uint64_t x;
		do {
			k=rand()%hi;
			x = (uint64_t)k;
		} while (ts_algo_map_getId(hidden, x) != NULL);

		// fprintf(stderr, "reading %d\n", k);

		err = beet_index_copy(idx, &k, &d);
		if (err != BEET_OK) {
			errmsg(err, "cannot copy from index");
			return -1;
		}
		if (d != k) {
			fprintf(stderr, "wrong data: %d - %d\n", k, d);
			return -1;
		}
	}
	return 0;
}

int testDoesExist(beet_index_t idx, ts_algo_map_t *hidden, int hi) {
	beet_err_t err;
	int k;
	for(int i=0; i<100; i++) {

		uint64_t x;
		do {
			k=rand()%hi;
			x = (uint64_t)k;
		} while (ts_algo_map_getId(hidden, x) != NULL);

		// fprintf(stderr, "reading %d\n", k);

		err = beet_index_doesExist(idx, &k);
		if (err != BEET_OK) {
			errmsg(err, "cannot copy from index");
			return -1;
		}
	}
	return 0;
}

int testDoesNotExist(beet_index_t idx, int hi) {
	beet_err_t err;
	int k;
	for(int i=0; i<100; i++) {

		k=rand()%hi;

		// fprintf(stderr, "reading %d\n", k);

		err = beet_index_doesExist(idx, &k);
		if (err != BEET_ERR_KEYNOF) {
			errmsg(err, "key exists");
			return -1;
		}
	}
	return 0;
}

int zerocopyRead(beet_index_t idx, ts_algo_map_t *hidden, int hi) {
	beet_state_t state=NULL;
	beet_err_t err;
	int k;
	int *d;

	err = beet_state_alloc(idx, &state);
	if (err != BEET_OK) {
		errmsg(err, "cannot allocate state");
		return -1;
	}
	for(int i=0; i<100; i++) {

		uint64_t x;
		do {
			k=rand()%hi;
			x = (uint64_t)k;
		} while (ts_algo_map_getId(hidden, x) != NULL);

		err = beet_index_get(idx, state, 0, &k, (void**)&d);
		if (err != BEET_OK) {
			errmsg(err, "cannot get data from index");
			beet_state_destroy(state);
			return -1;
		}
		if (*d != k) {
			fprintf(stderr, "wrong data: %d - %d\n", k, *d);
			beet_state_destroy(state);
			return -1;
		}
		err = beet_state_release(state);
		if (err != BEET_OK) {
			errmsg(err, "cannot release state");
			beet_state_destroy(state);
			return -1;
		}
		beet_state_reinit(state);
	}
	beet_state_destroy(state);
	return 0;
}

#define FAKEDATA (char*)0x1
int hideAndSeek(beet_index_t idx, ts_algo_map_t *hidden, int hi) {
	beet_err_t err;
	int k;
	int d;
	for(int i=0; i<100; i++) {
		char isHidden = 0;
		char doHide   = 0;

		k=rand()%hi;
		uint64_t x = (uint64_t)k;

		if (ts_algo_map_getId(hidden, x) != NULL) {
			isHidden = 1;
		} else {
			doHide = rand()%10;
		}

		if (doHide == 1) {
			fprintf(stderr, "hiding %u\n", k);
			err = beet_index_hide(idx, &k);
			if (ts_algo_map_addId(hidden, x, FAKEDATA) != TS_ALGO_OK) {
				fprintf(stderr, "cannot add key %lu to map\n", x);
				return -1;
			}
			return 0;
		}

		// fprintf(stderr, "reading %d\n", k);

		err = beet_index_copy(idx, &k, &d);
		if (isHidden) {
			fprintf(stderr, "key %u isHidden: %d\n", k, isHidden);
			if (err != BEET_ERR_KEYNOF) {
				errmsg(err, "key is hidden");
				return -1;
			}
			return 0;
		} 
		if (err != BEET_OK) {
			errmsg(err, "cannot copy from index");
			return -1;
		}
		if (d != k) {
			fprintf(stderr, "wrong data: %d - %d\n", k, d);
			return -1;
		}
	}
	return 0;
}

int main() {
	beet_config_t config;
	int rc = EXIT_SUCCESS;
	beet_index_t idx;
	ts_algo_map_t hidden;
	int haveIndex = 0;
	int haveMap   = 0;

	srand(time(NULL) ^ (uint64_t)&printf);

	beet_config_init(&config);

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
	config.compare = NULL;
	config.rscinit = NULL;
	config.rscdest = NULL;

	if (createDropIndex(&config) != 0) {
		fprintf(stderr, "createDropIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (createIndex(&config) != 0) {
		fprintf(stderr, "createIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	idx = openIndex("rsc/idx10");
	if (idx == NULL) {
		fprintf(stderr, "openIndex failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;

	if (ts_algo_map_init(&hidden, 0, ts_algo_hash_id, NULL) != TS_ALGO_OK) {
		fprintf(stderr, "cannot init map\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveMap = 1;

	/* no key shall exist */
	if (testDoesNotExist(idx, 1) != 0) {
		fprintf(stderr, "testDoesNotExist 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	
	/* test with 13 (key,value) pairs */
	if (writeRange(idx, &hidden, 0, 13) != 0) {
		fprintf(stderr, "writeRange 0-13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, &hidden, 13) != 0) {
		fprintf(stderr, "testDoesExist 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (easyRead(idx, &hidden, 13) != 0) {
		fprintf(stderr, "easyRead 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (zerocopyRead(idx, &hidden, 13) != 0) {
		fprintf(stderr, "zerocopyRead 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (hideAndSeek(idx, &hidden, 13) != 0) {
		fprintf(stderr, "hideAndSeek 13 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 64 (key,value) pairs */
	if (writeRange(idx, &hidden, 13, 64) != 0) {
		fprintf(stderr, "writeRange 13-64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testDoesExist(idx, &hidden, 64) != 0) {
		fprintf(stderr, "testDoesExist 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (easyRead(idx, &hidden, 64) != 0) {
		fprintf(stderr, "easyRead 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (zerocopyRead(idx, &hidden, 64) != 0) {
		fprintf(stderr, "zerocopyRead 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (hideAndSeek(idx, &hidden, 64) != 0) {
		fprintf(stderr, "hideAndSeek 64 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* test with 99 (key,value) pairs */
	if (writeRange(idx, &hidden, 50, 99) != 0) {
		fprintf(stderr, "writeRange 50-99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	/* close/open in between */
	beet_index_close(idx); haveIndex = 0;
	idx = openIndex("rsc/idx10");
	if (idx == NULL) {
		fprintf(stderr, "openIndex (2) failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveIndex = 1;
	if (testDoesExist(idx, &hidden, 99) != 0) {
		fprintf(stderr, "testDoesExist 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (easyRead(idx, &hidden, 99) != 0) {
		fprintf(stderr, "easyRead 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (zerocopyRead(idx, &hidden, 99) != 0) {
		fprintf(stderr, "zerocopyRead 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (hideAndSeek(idx, &hidden, 99) != 0) {
		fprintf(stderr, "hideAndSeek 99 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

	if (writeRange(idx, &hidden, 99, 200) != 0) {
		fprintf(stderr, "writeRange 99-200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	/* test with 200 (key,value) pairs */
	if (testDoesExist(idx, &hidden, 200) != 0) {
		fprintf(stderr, "testDoesExist 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (easyRead(idx, &hidden, 200) != 0) {
		fprintf(stderr, "easyRead 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (zerocopyRead(idx, &hidden, 200) != 0) {
		fprintf(stderr, "zerocopyRead 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (hideAndSeek(idx, &hidden, 200) != 0) {
		fprintf(stderr, "hideAndSeek 200 failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (haveMap) ts_algo_map_destroy(&hidden);
	if (haveIndex) beet_index_close(idx);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}
