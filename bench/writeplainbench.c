/* ========================================================================
 * Benchmark: writing plain index with simple uint64_t key
 * ========================================================================
 */
#include <beet/types.h>
#include <beet/config.h>
#include <beet/index.h>

#include <common/cmd.h>
#include <common/bench.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <math.h>

uint64_t global_count = 1000;
uint32_t global_iter = 1;

void *global_lib=NULL;

/* ------------------------------------------------------------------------
 * helptxt
 * ------------------------------------------------------------------------
 */
void helptxt(char *prog) {
	fprintf(stderr, "%s <path> [options]\n", prog);
	fprintf(stderr, "-count: number of keys we (try) to insert\n");
	fprintf(stderr, "-iter : number of iterations\n");
}

/* ------------------------------------------------------------------------
 * get options
 * ------------------------------------------------------------------------
 */
int parsecmd(int argc, char **argv) {
	int err = 0;

	global_count = (uint64_t)ts_algo_args_findUint(
	            argc, argv, 2, "count", 1000, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	global_iter = (uint32_t)ts_algo_args_findUint(
	               argc, argv, 2, "iter", 1, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	return 0;
}

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

int dropIndex(char *path) {
	beet_err_t err;
	struct stat st;

	if (stat(path, &st) != 0) return 0;

	err = beet_index_drop(path);
	if (err != BEET_OK) {
		errmsg(err, "cannot drop index");
		return -1;
	}
	return 0;
}

int createIndex(char *path) {
	beet_config_t cfg;
	beet_err_t    err;

	beet_config_init(&cfg);

	cfg.indexType = BEET_INDEX_PLAIN;
	cfg.leafPageSize = 4096;
	cfg.intPageSize = 4096;
	cfg.leafNodeSize = 250;
	cfg.intNodeSize = 340;
	cfg.leafCacheSize = 10000;
	cfg.intCacheSize = 10000;
	cfg.keySize = 8;
	cfg.dataSize = 8;
	cfg.subPath = NULL;
	cfg.compare = "beetSmokeUInt64Compare";
	cfg.rscinit = NULL;
	cfg.rscdest = NULL;

	err = beet_index_create(path, 1, &cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
	return 0;
}

beet_index_t openIndex(char *path) {
	beet_index_t idx;
	beet_open_config_t cfg;
	beet_err_t err;

	beet_open_config_ignore(&cfg);

	err = beet_index_open(path, global_lib, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

int writeidx(beet_index_t idx, uint64_t count) {
	beet_err_t err;
	int x;
	uint64_t k;

	for(uint64_t i=0;i<count;i++) {
		k = rand();
		x = rand()%4;
		switch(x) {
		case 1: k*=10;break;
		case 2: k*=100;break;
		case 3: k*=1000;break;
		default: break;
		}
		
		err = beet_index_insert(idx, &k, &k);
		if (err != BEET_OK) {
			errmsg(err, "cannot not insert");
			return -1;
		}
	}
	return 0;
}

int bench(char *path) {
	beet_index_t  idx;
	struct timespec t1, t2;
	uint64_t d = 0;

	fprintf(stderr, "%s\n", path);

	for(uint32_t i=0; i<global_iter; i++) {
		// if (dropIndex(path) != 0) return -1;
		if (createIndex(path) != 0) return -1;

		idx = openIndex(path);
		if (idx == NULL) return -1;

		// measure
		timestamp(&t1);
		if (writeidx(idx, global_count) != 0) return -1;
		timestamp(&t2);

		d += minus(&t2,&t1)/1000;

		beet_index_close(idx);
	}
	fprintf(stderr, "avg: %luus\n", d/global_iter);
	return 0;
}

int main(int argc, char **argv) {
	int rc = EXIT_SUCCESS;

	if (argc < 2) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	if (parsecmd(argc, argv) != 0) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	srand(time(NULL) ^ (uint64_t)&printf);

	global_lib = beet_lib_init("libcmp.so");
	if (global_lib == NULL) {
		fprintf(stderr, "cannot init compare library\n");
		return EXIT_FAILURE;
	}
	fprintf(stderr, "%s %s\n", argv[0], argv[1]);

	if (bench(argv[1]) != 0) rc = EXIT_FAILURE;

	beet_lib_close(global_lib);
	return rc;
}
