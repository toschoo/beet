/* ========================================================================
 * Benchmark: reading indices with simple uint64_t key
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

uint64_t global_count= 1000;
uint32_t global_iter = 1;

void *global_lib=NULL;

/* ------------------------------------------------------------------------
 * helptxt
 * ------------------------------------------------------------------------
 */
void helptxt(char *prog) {
	fprintf(stderr, "%s <type> <path> [options]\n", prog);
	fprintf(stderr, "type: null|plain|host\n");
	fprintf(stderr, "-iter : number of iterations\n");
	fprintf(stderr, "-count: number of keys in the index\n");
}

/* ------------------------------------------------------------------------
 * get options
 * ------------------------------------------------------------------------
 */
int parsecmd(int argc, char **argv) {
	int err = 0;

	global_count = ts_algo_args_findUint(
	               argc, argv, 3, "count", 1000, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_iter = (uint32_t)ts_algo_args_findUint(
	               argc, argv, 3, "iter", 1, &err);
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

int readplain(beet_index_t idx, uint64_t count, int *found) {
	beet_err_t err;
	uint64_t k;
	uint64_t d;

	k = rand()%count;
	err = beet_index_copy(idx, &k, &d);
	if (err == BEET_OK) {
		(*found)++; return 0;
	}
	if (err == BEET_ERR_KEYNOF) return 0;
	if (err != BEET_OK) {
		errmsg(err, "cannot not copy data");
		return -1;
	}
	return 0;
}

int readhost(beet_index_t   idx,
             beet_state_t state,
             beet_iter_t   iter,
             uint64_t     count,
             int         *found) {
	beet_err_t err;
	uint64_t k, *z;

	k = rand()%count;
	err = beet_index_getIter(idx, state, &k, iter);
	if (err == BEET_ERR_KEYNOF) return 0;
	if (err != BEET_OK) {
		errmsg(err, "cannot not copy data");
		return -1;
	}
	while((err = beet_iter_move(iter, (void**)&z, NULL)) == BEET_OK) {
		(*found)++;
	}
	if (err != BEET_ERR_EOF) {
		errmsg(err, "cannot iterate");
		return -1;
	}
	err = beet_state_release(state);
	if (err != BEET_OK) {
		errmsg(err, "cannot release state");
		return -1;
	}
	err = beet_iter_reset(iter);
	if (err != BEET_OK) {
		errmsg(err, "cannot release state");
		return -1;
	}
	return 0;
}

int bench(int type, char *path) {
	beet_err_t    err;
	beet_index_t  idx;
	struct timespec t1, t2;
	uint64_t *d, s=0;
	uint64_t mx=0, mn=-1;
	uint32_t it=global_iter;
	int found = 0;
	beet_iter_t iter;
	beet_state_t state;

	fprintf(stderr, "%s\n", path);

	idx = openIndex(path);
	if (idx == NULL) return -1;

	d = calloc(it, sizeof(uint64_t));
	if (d == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}

	if (type == BEET_INDEX_HOST) {
		err = beet_state_alloc(idx, &state);
		if (err != BEET_OK) {
			errmsg(err, "cannot allocate state");
			return -1;
		}
		err = beet_iter_alloc(idx, &iter);
		if (err != BEET_OK) {
			errmsg(err, "cannot allocate iter");
			return -1;
		}
	}
	for(uint32_t i=0; i<it; i++) {

		timestamp(&t1);
		switch(type) {
		case BEET_INDEX_PLAIN:
			if (readplain(idx, global_count, &found) != 0) {
				free(d); return -1;
			}
			break;
		case BEET_INDEX_HOST:
			if (readhost(idx, state, iter,
			    global_count, &found) != 0) {
				beet_iter_destroy(iter);
				beet_state_destroy(state);
				free(d); return -1;
			}
			break;
		default:
			fprintf(stderr, "unknown index type\n");
			free(d); return -1;
		}
		timestamp(&t2);

		d[i]  = minus(&t2,&t1);
		s    += minus(&t2,&t1);
		if (mx < d[i]) mx = d[i];
		if (mn > d[i]) mn = d[i];
	}
	if (type == BEET_INDEX_HOST) {
		beet_iter_destroy(iter);
		beet_state_destroy(state);
	}
	beet_index_close(idx);
	qsort(d, it, sizeof(uint64_t), &compare);
	fprintf(stderr, "found : %u\n", found);
	fprintf(stderr, "avg   : %010luus\n", (s/it)/1000);
	fprintf(stderr, "median: %010luus\n", median(d, it)/1000);
	fprintf(stderr, "max   : %010luus\n", mx/1000);
	fprintf(stderr, "min   : %010luus\n", mn/1000);
	fprintf(stderr, "99    : %010luus\n", percentile(d, it, 99)/1000);
	fprintf(stderr, "50    : %010luus\n", percentile(d, it, 50)/1000);
	free(d);
	return 0;
}

int gettype(char *type) {
	if (strnlen(type, 129) > 128) {
		fprintf(stderr, "unknown type\n");
		return -1;
	}
	if (strcasecmp(type, "plain") == 0) return BEET_INDEX_PLAIN;
	if (strcasecmp(type, "host") == 0) return BEET_INDEX_HOST;
	fprintf(stderr, "unknown type: '%s'\n", type);
	return -1;
}

int checkpath(char *path) {
	if (strnlen(path, 4097) > 4096) {
		fprintf(stderr, "path too long\n");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv) {
	int rc = EXIT_SUCCESS;
	int t;
	char *type;
	char *path;

	if (argc < 3) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	type = argv[1];
	t = gettype(type);
	if (t < 0) {
		helptxt(argv[0]);
		return EXIT_FAILURE;
	}
	path = argv[2];
	if (checkpath(path) != 0) {
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

	if (bench(t, path) != 0) rc = EXIT_FAILURE;

	beet_lib_close(global_lib);
	return rc;
}
