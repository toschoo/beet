/* ========================================================================
 * Swiss Army Knife supporting the commands:
 * - create
 * - height
 * - count nodes | leaves | internals | keys
 * - config
 * - version
 * - show node(s)
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

/* ------------------------------------------------------------------------
 * command line parameters
 * ------------------------------------------------------------------------
 */
uint32_t global_lsize = 0;
uint32_t global_nsize = 0;
uint32_t global_ksize = 0;
uint32_t global_dsize = 0;
uint32_t global_cache = 1000;
char    *global_compare = NULL;
char    *global_rscinit = NULL;
char    *global_rscdest = NULL;
char    *global_lib     = NULL;
char    *global_path    = NULL;
int      global_stndaln = 1;
int      global_type    = 1;

void *global_handle=NULL;

/* ------------------------------------------------------------------------
 * helptxt
 * ------------------------------------------------------------------------
 */
void helptxt(char *prog) {
	fprintf(stderr, "%s <command> <path> [options]\n", prog);
	fprintf(stderr, "\n");
	fprintf(stderr, "command:\n");
	fprintf(stderr, "--------\n");
	fprintf(stderr, "'help'   : print this message\n");
	fprintf(stderr, "'version': print current version of the library\n");
	fprintf(stderr, "'create' : create a b+tree physically on disk\n");
	fprintf(stderr, "'config' : show the tree configuration\n");
	fprintf(stderr, "'height' : compute the height of the tree\n");
	fprintf(stderr, "'count' 'leaves'|'internals'|'nodes'|'keys':\n");
	fprintf(stderr, "         count the leaves, etc. in the tree\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "--------\n");
	fprintf(stderr,
	"-standalone: this is a standalone index (default: true)\n");
	fprintf(stderr, "-type: type of index to create (default: PLAIN)\n");
	fprintf(stderr, "       1: NULL  Index\n");
	fprintf(stderr, "       2: PLAIN Index\n");
	fprintf(stderr, "       3: HOST  Index\n");
	fprintf(stderr,
	"-subpath: path to embedded index (mandatory if type = HOST)\n");
	fprintf(stderr,
	"-leaf:     number of keys in leaf nodes (mandatory)\n");
	fprintf(stderr, 
	"-internal: number of keys in internal nodes (mandatory)\n");
	fprintf(stderr, "-key: size of key (mandatory)\n");
	fprintf(stderr, "-data: size of data (mandatory if type = PLAIN)\n");
	fprintf(stderr, "-cache: size of cache (default: 10000)\n");
	fprintf(stderr,
	"-compare: symbol of user-defined compare function (mandatory)\n");
	fprintf(stderr,
	"-init: symbol of user-defined init function (default: NULL)\n");
	fprintf(stderr,
	"-destroy: symbol of user-defined destroy function (default: NULL)\n");
	fprintf(stderr,
	"-lib: library to load dyanmically (default: NULL)\n");
}

/* ------------------------------------------------------------------------
 * get create options
 * ------------------------------------------------------------------------
 */
int parsecreatecmd(int argc, char **argv) {
	int err = 0;

	global_lsize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 3, "leaf", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error (lsize): %d\n", err);
		return -1;
	}
	if (global_lsize == 0) return -1;

	global_nsize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 3, "internal", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error (nsize): %d\n", err);
		return -1;
	}
	if (global_nsize == 0) return -1;

	global_ksize = (uint32_t)ts_algo_args_findUint(
	            argc, argv, 3, "key", 0, &err);
	if (err != 0) {
		fprintf(stderr, "command line error (ksize): %d\n", err);
		return -1;
	}
	if (global_ksize == 0) return -1;

	global_type = (uint32_t)ts_algo_args_findUint(
	 argc, argv, 3, "type", BEET_INDEX_PLAIN, &err);
	if (err != 0) {
		fprintf(stderr, "command line error (type): %d\n", err);
		return -1;
	}
	switch(global_type) {
	case BEET_INDEX_NULL:
		global_dsize = 0; break;

	case BEET_INDEX_HOST: 
		global_dsize = sizeof(beet_pageid_t);
		global_path = ts_algo_args_findString(
		 argc, argv, 3, "subpath", NULL, &err);
		if (err != 0) {
			fprintf(stderr, "command line error: %d\n", err);
			return -1;
		}
		if (global_path == NULL) return -1;
		break;

	case BEET_INDEX_PLAIN:
		global_dsize = (uint32_t)ts_algo_args_findUint(
		                argc, argv, 3, "data", 0, &err);
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
	            argc, argv, 3, "cache", 10000, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_stndaln = ts_algo_args_findBool(
	            argc, argv, 3, "standalone", 1, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_compare = ts_algo_args_findString(
	            argc, argv, 3, "compare", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	if (global_compare == NULL) return -1;

	global_rscinit = ts_algo_args_findString(
	            argc, argv, 3, "init", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_rscdest = ts_algo_args_findString(
	            argc, argv, 3, "destroy", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------------
 * get options for height and count
 * ------------------------------------------------------------------------
 */
int parsecountcmd(int argc, char **argv) {
	int err = 0;

	global_lib = ts_algo_args_findString(
	            argc, argv, 3, "lib", NULL, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------------
 * print nice error message
 * ------------------------------------------------------------------------
 */
void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "%s\n", beet_oserrdesc());
	}
}

/* ------------------------------------------------------------------------
 * create the index
 * ------------------------------------------------------------------------
 */
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

/* ------------------------------------------------------------------------
 * open the index
 * ------------------------------------------------------------------------
 */
beet_index_t openIndex(char *path) {
	beet_index_t idx;
	beet_open_config_t cfg;
	beet_err_t err;

	beet_open_config_ignore(&cfg);

	err = beet_index_open(path, global_handle, &cfg, &idx);
	if (err != BEET_OK) {
		errmsg(err, "cannot open index");
		return NULL;
	}
	return idx;
}

/* ------------------------------------------------------------------------
 * compute the height
 * ------------------------------------------------------------------------
 */
int height(char *path) {
	beet_err_t   err;
	beet_index_t idx;
	uint32_t       h;

	idx = openIndex(path);
	if (idx == NULL) return -1;

	err = beet_index_height(idx, &h);
	if (err != BEET_OK) {
		errmsg(err, "cannot get height");
		beet_index_close(idx);
		return -1;
	}
	beet_index_close(idx);
	fprintf(stdout, "height %s: %u\n", path, h);
	return 0;
}

/* ------------------------------------------------------------------------
 * count nodes
 * ------------------------------------------------------------------------
 */
int count(char *path, char *name) {
	char *p;
	struct stat st;

	p = malloc(strlen(path) + strlen(name) + 2);
	if (p == NULL) {
		fprintf(stderr, "out-of-memory\n");
		return -1;
	}
	sprintf(p, "%s/%s", path, name);
	if (stat(p, &st) != 0) {
		fprintf(stderr, "index does not exist\n");
		free(p); return -1;
	}
	free(p);
	return st.st_size;
}

/* ------------------------------------------------------------------------
 * count keys
 * ------------------------------------------------------------------------
 */
int64_t countkeys(char *path) {
	beet_err_t   err;
	beet_index_t idx;
	beet_iter_t iter;
	int64_t c=0;
	void *k=NULL;

	idx = openIndex(path);
	if (idx == NULL) return -1;

	err = beet_iter_alloc(idx, &iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot allocate iterator");
		beet_index_close(idx);
		return -1;
	}
	err = beet_index_range(idx, NULL, BEET_DIR_ASC, iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot range");
		beet_iter_destroy(iter);
		beet_index_close(idx);
		return -1;
	}
	while((err = beet_iter_move(iter, &k, NULL)) == BEET_OK) c++;
	if (err != BEET_ERR_EOF) {
		errmsg(err, "cannot range");
		beet_iter_destroy(iter);
		beet_index_close(idx);
		return -1;
	}
	beet_iter_destroy(iter);
	beet_index_close(idx);
	return c;
}

/* ------------------------------------------------------------------------
 * handle count commands
 * ------------------------------------------------------------------------
 */
int handlecount(int argc, char **argv,
                          char  *subcmd,
                          char  *path) {
	beet_err_t    err;
	beet_config_t cfg;
	int64_t k;
	int l=0;
	int i=0;
	int x=-1;

	memset(&cfg, 0, sizeof(beet_config_t));
	err = beet_config_get(path, &cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot open config");
		return -1;
	}
	if (strcasecmp(subcmd, "nodes") == 0) {
		l = count(path, "leaf");
		if (l < 0) {
			beet_config_destroy(&cfg);
			return -1;
		}
		i = count(path, "nonleaf"); 
		if (i < 0) {
			beet_config_destroy(&cfg);
			return -1;
		}

		l /= cfg.leafPageSize;
		i /= cfg.intPageSize;

		fprintf(stdout, "nodes in %s: %d + %d = %d\n",
		                             path, l, i, l+i);
		x = 0;
	}
	else if (strcasecmp(subcmd, "leaves") == 0) {
		l = count(path,"leaf");
		if (l < 0) {
			beet_config_destroy(&cfg);
			return -1;
		}
		l /= cfg.leafPageSize;
		fprintf(stdout, "leaves in %s: %d\n", path, l);
		x = 0;
	}
	else if (strcasecmp(subcmd, "internals") == 0) {
		i = count(path,"nonleaf");
		if (i < 0) {
			beet_config_destroy(&cfg);
			return -1;
		}
		i /= cfg.intPageSize;
		fprintf(stdout, "internals in %s: %d\n", path, i);
		x = 0;
	}
	else if (strcasecmp(subcmd, "keys") == 0) {
		global_handle = beet_lib_init(global_lib);
		if (global_handle == NULL) {
			beet_config_destroy(&cfg);
			return -1;
		}
		k = countkeys(path); if (k < 0) {
			beet_config_destroy(&cfg);
			return -1;
		}
		fprintf(stdout, "keys in %s: %lu\n", path, k);
		x = 0;
	}
	beet_config_destroy(&cfg);
	if (x == 0) return 0;
	fprintf(stderr, "unknown subcommand: %s\n", subcmd);
	return -1;
}

/* ------------------------------------------------------------------------
 * print nice type
 * ------------------------------------------------------------------------
 */
const char *typedesc(int t) {
	switch(t) {
	case BEET_INDEX_NULL: return "NULL";
	case BEET_INDEX_PLAIN: return "PLAIN";
	case BEET_INDEX_HOST: return "HOST";
	default: return "unknown";
	}
}

/* ------------------------------------------------------------------------
 * show config
 * ------------------------------------------------------------------------
 */
int handleconfig(int argc, char **argv, char *path) {
	beet_err_t    err;
	beet_config_t cfg;

	memset(&cfg, 0, sizeof(beet_config_t));
	err = beet_config_get(path, &cfg);
	if (err != BEET_OK) {
		errmsg(err, "cannot open config");
		return -1;
	}
	fprintf(stdout, "Config for %s:\n", path);
	fprintf(stdout, "type           : %s\n", typedesc(cfg.indexType));
	fprintf(stdout, "leaf page size : %u\n", cfg.leafPageSize);
	fprintf(stdout, "int. page size : %u\n", cfg.intPageSize);
	fprintf(stdout, "keys per leaf  : %u\n", cfg.leafNodeSize);
	fprintf(stdout, "keys per int.  : %u\n", cfg.intNodeSize);
	fprintf(stdout, "key size       : %u\n", cfg.keySize);
	fprintf(stdout, "data size      : %u\n", cfg.dataSize);
	fprintf(stdout, "leaf cache size: %u\n", cfg.leafCacheSize);
	fprintf(stdout, "int. cache size: %u\n", cfg.intCacheSize);
	fprintf(stdout, "sub path       : %s\n", cfg.subPath);
	fprintf(stdout, "compare symbol : %s\n", cfg.compare);
	fprintf(stdout, "init    symbol : %s\n", cfg.rscinit);
	fprintf(stdout, "destroy symbol : %s\n", cfg.rscdest);

	beet_config_destroy(&cfg);
	return 0;
}

/* ------------------------------------------------------------------------
 * show version
 * ------------------------------------------------------------------------
 */
int handleversion() {
	beet_version_print();
	return 0;
}

/* ------------------------------------------------------------------------
 * handle command
 * ------------------------------------------------------------------------
 */
int handlecmd(int argc, char **argv, char *cmd, char *path) {
	int x = 0;
	char *subcmd=NULL;
	char *path2;

	if (strcasecmp(cmd, "create") == 0) {
		if (parsecreatecmd(argc, argv) != 0) return -1;
		return createIndex(path);
	}
	if (parsecountcmd(argc, argv) != 0) return -1;
	if (strcasecmp(cmd, "height") == 0) {
		global_handle = beet_lib_init(global_lib);
		if (global_handle == NULL) return -1;
		x = height(path);
		return x;
	}
	if (strcasecmp(cmd, "count") == 0) {
		subcmd = argv[2];
		path2 = argv[3];
		return handlecount(argc, argv, subcmd, path2);
	}
	if (strcasecmp(cmd, "config") == 0) {
		return handleconfig(argc, argv, path);
	}
	fprintf(stderr, "not implemented: %s\n", cmd);
	return -1;
}

/* ------------------------------------------------------------------------
 * check command
 * ------------------------------------------------------------------------
 */
int checkcmd(char *cmd) {
	if (cmd == NULL) return -1;
	if (strnlen(cmd, 4097) > 4096) {
		fprintf(stderr, "unknown command\n");
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------------
 * check path
 * ------------------------------------------------------------------------
 */
int checkpath(char *path) {
	if (path == NULL) return -1;
	if (strnlen(path, 4097) > 4096) {
		fprintf(stderr, "path too long\n");
		return -1;
	}
	return 0;
}

/* ------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
	int rc = EXIT_SUCCESS;
	char *cmd;
	char *path;

	if (argc < 2) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}

	cmd = argv[1];
	if (checkcmd(cmd) != 0) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	if (strcasecmp(cmd, "help") == 0) {
		helptxt(argv[0]);
		return EXIT_SUCCESS;
	}
	if (strcasecmp(cmd, "version") == 0) {
		handleversion();
		return EXIT_SUCCESS;
	}
	path = argv[2];
	if (checkpath(path) != 0) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	if (handlecmd(argc, argv, cmd, path) != 0) {
		helptxt(argv[0]);
		rc = EXIT_FAILURE;
	}
	if (global_handle != NULL) {
		beet_lib_close(global_handle);
		global_handle = NULL;
	}
	return rc;
}
