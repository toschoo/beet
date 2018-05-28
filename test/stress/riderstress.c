/* -----------------------------------------------------------------------
 * Stress test for riders
 * -----------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#include <beet/lock.h>
#include <beet/rider.h>

#include <common/bench.h>
#include <common/cmd.h>

#define PSIZE    64
#define PSIZEINT 16

int global_pages=100;

void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg,
	             beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "OSERR: %s\n",
	             beet_oserrdesc(err));
	}
}

void randomRider(beet_rider_t *rider, int *errs) {
	beet_err_t err;
	beet_page_t *page;
	int *buf;
	beet_pageid_t pid;
	int x;
	int k,p;

	for(int i=0; i<1000; i++) {
		pid = rand()%global_pages;
		x = rand()%2;

		if (x) {
			err = beet_rider_getRead(rider, pid, &page);
		} else {
			err = beet_rider_getWrite(rider, pid, &page);
		}
		if (err != BEET_OK) {
			errmsg(err, "cannot get page");
			break;
		}
		/* do something with the page */
		buf = (int*)page->data;
		p = buf[0];
		for(int z=1; z<PSIZEINT; z++) {
			k = buf[z];
			if (k != p+1) {
				fprintf(stderr, "ERROR %d/%d\n", p, k);
				(*errs)++;
			}
			p = k;
		}
		if (x) {
			err = beet_rider_releaseRead(rider, page);
		} else {
			err = beet_rider_releaseWrite(rider, page);
		}
		if (err != BEET_OK) {
			errmsg(err, "cannot release page");
			break;
		}
	}
}

typedef struct {
	beet_latch_t  latch;
	pthread_mutex_t  mu;
	pthread_cond_t cond;
	beet_rider_t  rider;
	int         running;
	int             err;
} params_t;

int params_init(params_t *params, char *base, char *name, int ts, int max) {
	beet_err_t err;
	int x;

	params->running = 0;
	params->err = 0;
	err = beet_latch_init(&params->latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot init latch");
		return -1;
	}
	x = pthread_mutex_init(&params->mu, NULL);
	if (x != 0) {
		fprintf(stderr, "cannot init mutex: %d\n", x);
		return -1;
	}
	x = pthread_cond_init(&params->cond, NULL);
	if (x != 0) {
		fprintf(stderr, "cannot init conditional: %d\n", x);
		return -1;
	}
	err = beet_rider_init(&params->rider, base, name, PSIZE, max);
	if (err != BEET_OK) {
		errmsg(err, "cannot init rider");
		return -1;
	}
	return 0;
}

void params_destroy(params_t *params) {
	beet_rider_destroy(&params->rider);
	pthread_mutex_destroy(&params->mu);
	pthread_cond_destroy(&params->cond);
	beet_latch_destroy(&params->latch);
}

params_t params;

void *task(void *p) {
	beet_err_t err;
	int x;

	/* protect the conditional */
	x = pthread_mutex_lock(&params.mu);
	if (x != 0) {
		fprintf(stderr, "cannot lock mutex: %d\n", x);
		return NULL;
	}

	/* protect params */
	err = beet_latch_lock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock latch\n");
		return NULL;
	}
	/* count up */
	params.running++;
	err = beet_latch_unlock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock latch\n");
		return NULL;
	}
	/* wait for conditional */
	x = pthread_cond_wait(&params.cond, &params.mu);
	if (x != 0) {
		fprintf(stderr, "cannot wait on conditional: %d\n", x);
		return NULL;
	}
	x = pthread_mutex_unlock(&params.mu);
	if (x != 0) {
		fprintf(stderr, "cannot unlock mutex: %d\n", x);
		return NULL;
	}

	/* protect params */
	err = beet_latch_lock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock latch\n");
		return NULL;
	}

	/* do the job */
	randomRider(&params.rider, &params.err);

	/* count down */
	params.running--;

	err = beet_latch_unlock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock latch\n");
		return NULL;
	}
	return NULL;
}

void nap() {
	struct timespec tp = {0,1000000};
	nanosleep(&tp,NULL);
}

int64_t waitForEvent(int event) {
	struct timespec t1, t2;
	beet_err_t err;
	int count;

	timestamp(&t1);
	for(;;) {
		err = beet_latch_lock(&params.latch);
		if (err != BEET_OK) {
			errmsg(err, "cannot lock");
			return -1;
		}
		count = params.running;
		err = beet_latch_unlock(&params.latch);
		if (err != BEET_OK) {
			errmsg(err, "cannot unlock");
			return -1;
		}
		if (count == event) break;
		// fprintf(stderr, "waiting: %d\n", count);
		nap();
	}
	timestamp(&t2);
	return minus(&t2,&t1);
}

int initThreads(pthread_t **tids, int threads) {
	int x;

	*tids = calloc(threads,sizeof(pthread_t));
	if (*tids == NULL) {
		fprintf(stderr, "out-of-memory\n");
		return -1;
	}
	for(int i=0;i<threads;i++) {
		x = pthread_create((*tids)+i, NULL, &task, NULL);
		if (x != 0) {
			fprintf(stderr, "cannot create pthread: %d\n", x);
			return -1;
		}
	}
	return 0;
}

void destroyThreads(pthread_t *tids, int threads) {
	int x;
	for(int i=0; i<threads; i++) {
		if (tids[i] == 0) break;
		x = pthread_join(tids[i], NULL);
		if (x != 0) {
			fprintf(stderr, "cannot create pthread: %d\n", x);
		}
		pthread_detach(tids[i]);
	}
	free(tids);
}

int bench(int threads, int max) {
	pthread_t *tids;
	int x;
	int64_t m;

	if (params_init(&params, "rsc", "test10.bin", threads, max) != 0) {
		fprintf(stderr, "cannot init params\n");
		return -1;
	}
	if (initThreads(&tids, threads) != 0) {
		params_destroy(&params);
		fprintf(stderr, "cannot init threads\n");
		return -1;
	}
	if (waitForEvent(threads) < 0) {
		destroyThreads(tids, threads);
		params_destroy(&params);
		fprintf(stderr, "cannot wait for event (up)\n");
		return -1;
	}
	x = pthread_cond_broadcast(&params.cond);
	if (x != 0) {
		destroyThreads(tids, threads);
		params_destroy(&params);
		fprintf(stderr, "cannot init conditional: %d\n", x);
		return -1;
	}
	m = waitForEvent(0);
	if (m < 0) {
		destroyThreads(tids, threads);
		params_destroy(&params);
		fprintf(stderr, "cannot wait for event (down)\n");
		return -1;
	}
	destroyThreads(tids, threads);
	fprintf(stdout, "bench %d/%d: waited %ldus\n", threads, max, m/1000);
	if (params.err != 0) {
		params_destroy(&params);
		fprintf(stderr, "%d errors occurred\n", params.err);
		return -1;
	}
	params_destroy(&params);
	return 0;
}

int createFile(char *path, char *name) {
	FILE *f;
	char *p = NULL;
	size_t s;

	s = strlen(path);
	s += strlen(name);

	p = malloc(s+2);
	if (p == NULL) {
		fprintf(stderr, "cannot allocate path\n");
		return -1;
	}
	sprintf(p, "%s/%s", path, name);
	f = fopen(p, "w"); free(p);
	if (f == NULL) {
		perror("cannot open file");
		return -1;
	}
	if (fclose(f) != 0) {
		perror("cannot close file");
		return -1;
	}
	return 0;
}

int createPages(char *path, char *name) {
	beet_rider_t rider;
	beet_err_t err;
	beet_page_t *page;
	int k=0;

	err = beet_rider_init(&rider, path, name, PSIZE, 100);
	if (err != BEET_OK) {
		errmsg(err, "cannot init rider");
		return -1;
	}
	for (int i=0; i<global_pages; i++) {
		err = beet_rider_alloc(&rider, &page);
		if (err != BEET_OK) {
			errmsg(err, "cannot alloc page");
			return -1;
		}
		for(int z=0; z<PSIZE; z+=sizeof(int)) {
			memcpy(page->data+z, &k, sizeof(int)); k++;
		}
		err = beet_rider_store(&rider,page);
		if (err != BEET_OK) {
			errmsg(err, "cannot store page");
			return -1;
		}
		err = beet_rider_releaseWrite(&rider, page);
		if (err != BEET_OK) {
			errmsg(err, "cannot release page");
			return -1;
		}
	}
	beet_rider_destroy(&rider);
	return 0;
}

int global_threads = 10;
int global_max     = 100;

int parsecmd(int argc, char **argv) {
	int err = 0;

	global_threads = (int)ts_algo_args_findUint(
	            argc, argv, 1, "threads", 10, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}

	global_max = (int)ts_algo_args_findUint(
	            argc, argv, 1, "max", 100, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	global_pages = 10 * global_max;
	return 0;
}

int main(int argc, char **argv) {
	if (parsecmd(argc, argv) != 0) {
		return EXIT_FAILURE;
	}
	fprintf(stderr,
	"benchmarking with %d threads, page cache %d and %d pages\n",
	global_threads, global_max, global_pages);

	if (createFile("rsc", "test10.bin") != 0) {
		fprintf(stderr, "cannot create file\n");
		return EXIT_FAILURE;
	}
	if (createPages("rsc", "test10.bin") != 0) {
		fprintf(stderr, "cannot create pages\n");
		return EXIT_FAILURE;
	}

	if (bench(global_threads, global_max) != 0) {
		fprintf(stderr, "cannot perform benchmark\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
