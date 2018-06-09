/* ========================================================================
 * Create a b+tree
 * ========================================================================
 */
#include <beet/types.h>
#include <beet/config.h>
#include <beet/index.h>

#include <common/cmd.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

uint32_t global_lsize = 0;
uint32_t global_nsize = 0;
uint32_t global_ksize = 0;
uint32_t global_dsize = 0;
uint32_t global_cache = 1000;
char    *global_compare = NULL;
char    *global_rscinit = NULL;
char    *global_rscdest = NULL;
char    *global_path    = NULL;
int      global_stndaln = 1;
int      global_type    = 1;

/* ------------------------------------------------------------------------
 * helptxt
 * ------------------------------------------------------------------------
 */
void helptxt(char *prog) {
	fprintf(stderr, "%s <path> [options]\n", prog);
	fprintf(stderr,
	"-standalone: this is a standalone index (default: true)\n");
	fprintf(stderr, "-type: type of index to create (default: PLAIN)\n");
	fprintf(stderr, "       1: NULL  Index\n");
	fprintf(stderr, "       2: PLAIN Index\n");
	fprintf(stderr, "       3: HOST  Index\n");
	fprintf(stderr,
	"-subpath: path to embedded index (mandatory if type = HOST\n");
	fprintf(stderr,
	"-leaf:     number of keys in leaf nodes (mandatory)\n");
	fprintf(stderr, 
	"-internal: number of keys in internal nodes (mandatory)\n");
	fprintf(stderr, "-key: size of key (mandatory)\n");
	fprintf(stderr, "-data: size of data (mandatory if type = PLAIN)\n");
	fprintf(stderr, "-cache: size of cache (default: 10000\n");
	fprintf(stderr,
	"-compare: symbol of user-defined compare function (mandatory)\n");
	fprintf(stderr,
	"-init: symbol of user-defined init function (default: NULL)\n");
	fprintf(stderr,
	"-destroy: symbol of user-defined destroy function (default: NULL)\n");
}

/* ------------------------------------------------------------------------
 * get options
 * ------------------------------------------------------------------------
 */
int parsecmd(int argc, char **argv) {
	int err = 0;

	global_lsize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 2, "leaf", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	if (global_lsize == 0) return -1;

	global_nsize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 2, "internal", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	if (global_nsize == 0) return -1;

	global_ksize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 2, "key", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	if (global_ksize == 0) return -1;

	global_type = (uint32_t)ts_algo_args_findUint(
	 argc, argv, 2, "type", BEET_INDEX_PLAIN, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	switch(global_type) {
	case BEET_INDEX_NULL:
		global_dsize = 0; break;

	case BEET_INDEX_HOST: 
		global_dsize = sizeof(beet_pageid_t);
		global_path = ts_algo_args_findString(
		 argc, argv, 2, "subpath", NULL, &err);
		if (err != 0) {
			fprintf(stderr, "command line error: %d\n", err);
			return -1;
		}
		if (global_path == NULL) return -1;
		break;

	case BEET_INDEX_PLAIN:
		global_dsize = (uint32_t)ts_algo_args_findUint(
		                argc, argv, 2, "data", 0, &err);
		if (err != 0) {
			fprintf(stderr, "command line error: %d\n", err);
			return -1;
		}
		if (global_dsize == 0) return -1;
		break;
	default:
		fprintf(stderr, "unknown type: %d\n", global_type);
		return -1;
	}

	global_cache = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 2, "data", 10000, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_stndaln = ts_algo_args_findUint(
	            argc, argv, 2, "standalone", 1, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_compare = ts_algo_args_findString(
	            argc, argv, 2, "compare", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	if (global_compare == NULL) return -1;

	global_rscinit = ts_algo_args_findString(
	            argc, argv, 2, "init", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_rscdest = ts_algo_args_findString(
	            argc, argv, 2, "destroy", NULL, &err);
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

int createIndex(char *path) {
	beet_config_t cfg;
	beet_err_t    err;
	struct stat    st;

	if (stat(path, &st) == 0) return 0;

	beet_config_init(&cfg);

	cfg.leafPageSize = global_ksize * global_lsize +
	                   global_dsize * global_lsize + 16;

	cfg.intPageSize = global_ksize          * global_nsize +
                          sizeof(beet_pageid_t) * global_nsize + 8;
	               

	cfg.indexType = global_type;
	cfg.leafNodeSize = global_lsize;
	cfg.intNodeSize = global_nsize;
	cfg.leafCacheSize = global_cache;
	cfg.intCacheSize = global_cache;
	cfg.keySize = global_ksize;
	cfg.dataSize = global_dsize;
	cfg.subPath = global_path;
	cfg.compare = global_compare;
	cfg.rscinit = global_rscinit;
	cfg.rscdest = global_rscdest;

	err = beet_index_create(path, 1, &cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot create index");
		return -1;
	}
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

	fprintf(stderr, "%s %s\n", argv[0], argv[1]);
	if (createIndex(argv[1]) != 0) {
		fprintf(stderr, "cannot create index\n");
		return EXIT_FAILURE;
	}
	return rc;
}



