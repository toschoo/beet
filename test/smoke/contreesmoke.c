/* =======================================================================
 * Concurrent writing and reading on a tree
 * =======================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#include <beet/lock.h>
#include <beet/node.h>
#include <beet/tree.h>

#include <common/bench.h>
#include <common/cmd.h>

#define NODESZ 14
#define BYTES 128
#define KEYSZ   4
#define DATASZ  4

/* -----------------------------------------------------------------------
 * Print errmsg
 * -----------------------------------------------------------------------
 */
void errmsg(beet_err_t err, char *msg) {
	fprintf(stderr, "%s: %s (%d)\n", msg,
	             beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "OSERR: %s\n",
	             beet_oserrdesc(err));
	}
}

/* -----------------------------------------------------------------------
 * Parameters to control threads
 * -----------------------------------------------------------------------
 */
typedef struct {
	beet_latch_t   latch;
	beet_latch_t stopper;
	beet_tree_t     tree;
	beet_pageid_t   root;
	FILE           *roof;
	int          running;
	int              err;
} params_t;

/* -----------------------------------------------------------------------
 * Range of keys to write
 * -----------------------------------------------------------------------
 */
typedef struct {
	int lo;
	int hi;
} range_t;

/* -----------------------------------------------------------------------
 * Have a nap
 * -----------------------------------------------------------------------
 */
void nap() {
	struct timespec tp = {0,1000000};
	nanosleep(&tp,NULL);
}

int initRider(beet_rider_t *rider, char *base, char *name) {
	beet_err_t err;
	err = beet_rider_init(rider, base, name, BYTES, 1000);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * Create file
 * -----------------------------------------------------------------------
 */
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

/* -----------------------------------------------------------------------
 * Create the roof
 * -----------------------------------------------------------------------
 */
FILE *createRoof(char *path) {
	FILE *roof;
	char *p;

	if (createFile(path, "roof") != 0) {
		fprintf(stderr, "cannot create roof\n");
		return NULL;
	}
	p = malloc(strlen(path) + 6);
	if (p == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return NULL;
	}
	sprintf(p, "%s/roof", path);

	roof = fopen(p, "rb+"); free(p);
	if (roof == NULL) {
		fprintf(stderr, "cannot open roof\n");
		return NULL;
	}
	return roof;
}

/* -----------------------------------------------------------------------
 * Compare operation
 * -----------------------------------------------------------------------
 */
#define DEREF(x) \
	(*(int*)x)

char cmp(const void *one, const void *two, void *ignore) {
	if (DEREF(one) < DEREF(two)) return BEET_CMP_LESS;
	if (DEREF(one) > DEREF(two)) return BEET_CMP_GREATER;
	return BEET_CMP_EQUAL;
}

/* -----------------------------------------------------------------------
 * initialise tree
 * -----------------------------------------------------------------------
 */
int initTree(beet_tree_t *tree, char *base,
                                char *name1,
                                char *name2,
                                FILE *roof,
                       beet_pageid_t *root) {
	beet_err_t err;
	beet_rider_t *nlfs, *lfs;
	beet_ins_t *ins;

	if (createFile(base, name1) != 0) return -1;
	if (createFile(base, name2) != 0) return -1;

	nlfs = calloc(1, sizeof(beet_rider_t));
	if (nlfs == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	lfs = calloc(1, sizeof(beet_rider_t));
	if (lfs == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	ins = calloc(1, sizeof(beet_ins_t));
	if (ins == NULL) {
		fprintf(stderr, "out-of-mem\n");
		return -1;
	}
	beet_ins_setPlain(ins);
	
	if (initRider(lfs, base, name1) != 0) return -1;
	if (initRider(nlfs, base, name2) != 0) return -1;

	err = beet_tree_init(tree, NODESZ, NODESZ, KEYSZ, DATASZ,
	            nlfs, lfs, roof, &cmp, NULL, NULL, NULL, ins);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
	err = beet_tree_makeRoot(tree, root);
	if (err != BEET_OK) {
		errmsg(err, "cannot make root");
		return -1;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * Write keys in range lo - hi
 * -----------------------------------------------------------------------
 */
int writeRange(beet_tree_t *tree, beet_pageid_t *root, int lo, int hi) {
	beet_err_t    err;

	for(int z=hi-1;z>=lo;z--) {
		err = beet_tree_upsert(tree, root, &z, &z);
		if (err != BEET_OK) {
			errmsg(err, "cannot upsert into tree");
			return -1;
		}
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * Read data up to hi
 * -----------------------------------------------------------------------
 */
int readRandom(beet_tree_t *tree, beet_pageid_t *root, int hi) {
	beet_err_t    err;
	beet_node_t *node;
	int32_t slot;
	int k;

	for(int i=0;i<10;i++) {
		k=rand()%hi;

		err = beet_tree_get(tree, root, &k, &node);
		if (err != BEET_OK) return -1;

		// fprintf(stderr, "got node %u for %d\n", node->self, k);

		slot = beet_node_search(node, KEYSZ, &k, &cmp, NULL);
		if (slot < 0) {
			fprintf(stderr, "unexpected result: %d\n", k);
			beet_tree_release(tree, node); free(node);
			return -1;
		}
		if (!beet_node_equal(node, slot, KEYSZ, &k, &cmp, NULL)) {
			fprintf(stderr,
			"%lu: key not found: %d in %u (%d - %d)\n",
			pthread_self(), k, node->self,
			(*(int*)node->keys),
			(*(int*)(node->keys+KEYSZ*node->size-KEYSZ)));
			beet_tree_release(tree, node); free(node);
			return -1;
		}
		// fprintf(stderr, "found: %d in %d\n", k, slot);
		if (memcmp(node->keys+slot*KEYSZ,
		           node->kids+slot*DATASZ, KEYSZ) != 0) {
			fprintf(stderr, "key and data differ: %d - %d\n",
				(*(int*)(node->keys+slot*KEYSZ)),
				(*(int*)(node->kids+slot*KEYSZ)));
			beet_tree_release(tree, node); free(node);
			return -1;
		}

		err = beet_tree_release(tree, node);
		if (err != BEET_OK) {
			errmsg(err, "cannot release node");
			free(node);
			return -1;
		}
		free(node);
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * What the threads do
 * -----------------------------------------------------------------------
 */
void randomReadOrWrite(params_t *params,
                       int lo, int hi) {
	int x;

	for(int i=0; i<10; i++) {
		x = rand()%2;

		if (x) {
			/*
			fprintf(stderr, "%lu writing to %d\n",
			                 pthread_self(), hi);
			*/
			if (writeRange(&params->tree,
			               &params->root, lo, hi) != 0) {
				beet_latch_lock(&params->latch);
				params->err++;
				beet_latch_unlock(&params->latch);
				return;
			}
		} else {
			/*
			fprintf(stderr, "%lu reading to %d\n",
			                 pthread_self(), NODESZ-1);
			*/
			if (readRandom(&params->tree,
			               &params->root, NODESZ-1) != 0) {
				beet_latch_lock(&params->latch);
				params->err++;
				beet_latch_unlock(&params->latch);
				return;
			}
		}
	}
}

/* -----------------------------------------------------------------------
 * Init parameters
 * -----------------------------------------------------------------------
 */
int params_init(params_t *params, int ts) {
	beet_err_t err;

	params->running = 0;
	params->err = 0;

	err = beet_latch_init(&params->latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot init latch");
		return -1;
	}
	err = beet_latch_init(&params->stopper);
	if (err != BEET_OK) {
		errmsg(err, "cannot init latch");
		return -1;
	}
	params->roof = createRoof("rsc");
	if (params->roof == NULL) {
		fprintf(stderr, "cannot create roof\n");
		return -1;
	}
	if (initTree(&params->tree, "rsc", "test20.leaf", "test20.noleaf",
	              params->roof,
                     &params->root) != 0) {
		fprintf(stderr, "cannot init tree\n");
		return -1;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * Destroy parameters
 * -----------------------------------------------------------------------
 */
void params_destroy(params_t *params) {
	beet_tree_destroy(&params->tree);
	beet_latch_destroy(&params->stopper);
	beet_latch_destroy(&params->latch);
	fclose(params->roof);
}

/* -----------------------------------------------------------------------
 * Global params variable
 * -----------------------------------------------------------------------
 */
params_t params;

/* -----------------------------------------------------------------------
 * Thread entry point
 * -----------------------------------------------------------------------
 */
void *task(void *p) {
	beet_err_t err;
	range_t *range = p;

	/* protect params */
	err = beet_latch_lock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock latch\n");
		return NULL;
	}
	/* count up */
	params.running++;

	/* unlock params */
	err = beet_latch_unlock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock latch\n");
		return NULL;
	}

	/* wait for others */
	err = beet_latch_lock(&params.stopper);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock stopper\n");
		return NULL;
	}

	/* let others pass */
	err = beet_latch_unlock(&params.stopper);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock stopper\n");
		return NULL;
	}

	/* do the job */
	randomReadOrWrite(&params,
	                  range->lo,
	                  range->hi);

	free(range);

	/* protect params */
	err = beet_latch_lock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock latch\n");
		return NULL;
	}

	/* count down */
	params.running--;

	/* unlock params */
	err = beet_latch_unlock(&params.latch);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock latch\n");
		return NULL;
	}
	// fprintf(stderr, "%lu is leaving\n", pthread_self());
	return NULL;
}

/* -----------------------------------------------------------------------
 * Wait for something (either all threads running or all threads stopped)
 * -----------------------------------------------------------------------
 */
int64_t waitForEvent(int event) {
	struct timespec t1, t2;
	beet_err_t err;
	int count;
	int max = 60000;
	int i;

	timestamp(&t1);
	for(i=0;i<max;i++) {
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
		nap();
	}
	timestamp(&t2);
	if (i == max) return -1;
	if (event == 0) fprintf(stderr, "ALL THREADS FINISHED\n");
	else fprintf(stderr, "THREADS RUNNING: %d\n", params.running);
	nap();
	return minus(&t2,&t1);
}

/* -----------------------------------------------------------------------
 * Start threads
 * -----------------------------------------------------------------------
 */
int initThreads(pthread_t **tids, int threads) {
	int x;

	*tids = calloc(threads,sizeof(pthread_t));
	if (*tids == NULL) {
		fprintf(stderr, "out-of-memory\n");
		return -1;
	}
	for(int i=0;i<threads;i++) {
		range_t *range = malloc(sizeof(range_t));
		if (range == NULL) {
			fprintf(stderr, "cannot allocate range\n");
			return -1;
		}
		range->lo = 2*(i+1)*NODESZ-1;
		range->hi = (2*(i+1)+1)*NODESZ-1;
		x = pthread_create((*tids)+i, NULL, &task, range);
		if (x != 0) {
			fprintf(stderr, "cannot create pthread: %d\n", x);
			return -1;
		}
	}
	nap();
	return 0;
}

/* -----------------------------------------------------------------------
 * Stop and detach threads
 * -----------------------------------------------------------------------
 */
void destroyThreads(pthread_t *tids, int threads) {
	int x;
	for(int i=0; i<threads; i++) {
		if (tids[i] == 0) break;
		x = pthread_join(tids[i], NULL);
		if (x != 0) {
			fprintf(stderr, "cannot join pthread: %d\n", x);
		}
	}
	free(tids);
}

/* -----------------------------------------------------------------------
 * Perform test
 * -----------------------------------------------------------------------
 */
int testrun(int threads) {
	beet_err_t err;
	pthread_t *tids;
	int64_t m;

	/* initialise parameters */
	if (params_init(&params, threads) != 0) {
		fprintf(stderr, "cannot init params\n");
		return -1;
	}
	/* write the first node */
	if (writeRange(&params.tree, &params.root, 0, NODESZ-1) != 0) {
		fprintf(stderr, "cannot write first node\n");
		return -1;
	}
	/* make threads stop here */
	err = beet_latch_lock(&params.stopper);
	if (err != BEET_OK) {
		errmsg(err, "cannot lock stopper");
		return -1;
	}
	/* start threads */
	if (initThreads(&tids, threads) != 0) {
		fprintf(stderr, "cannot init threads\n");
		return -1;
	}
	/* wait for all threads running */
	fprintf(stderr, "waiting for threads to start\n");
	if (waitForEvent(threads) < 0) {
		fprintf(stderr, "cannot wait for event (up)\n");
		return -1;
	}

	/* let threads run */
	err = beet_latch_unlock(&params.stopper);
	if (err != BEET_OK) {
		errmsg(err, "cannot unlock stopper");
		return -1;
	}

	/* wait that threads are ready */
	fprintf(stderr, "waiting for threads to finish\n");
	m = waitForEvent(0);
	if (m < 0) {
		fprintf(stderr, "cannot wait for event (down)\n");
		return -1;
	}

	/* destroy threads */
	destroyThreads(tids, threads);

	/* print benchmark result */
	fprintf(stdout, "test with %d threads: waited %ldus\n",
	        threads, m/1000);

	/* check for errors during running time */
	if (params.err != 0) {
		params_destroy(&params);
		fprintf(stderr, "%d errors occurred\n", params.err);
		return -1;
	}

	/* final read */
	if (readRandom(&params.tree, &params.root, NODESZ-1) != 0) {
		params_destroy(&params);
		fprintf(stderr, "final readRandom failed\n");
		return -1;
	}

	/* cleanup */
	params_destroy(&params);
	return 0;
}

/* -----------------------------------------------------------------------
 * Command line arguments
 * -----------------------------------------------------------------------
 */
int global_threads = 10;

int parsecmd(int argc, char **argv) {
	int err = 0;

	global_threads = (int)ts_algo_args_findUint(
	            argc, argv, 1, "threads", 10, &err);
	if (err != 0) {
		fprintf(stderr, "command line error: %d\n", err);
		return -1;
	}
	return 0;
}

/* -----------------------------------------------------------------------
 * Main
 * -----------------------------------------------------------------------
 */
int main(int argc, char **argv) {
	if (parsecmd(argc, argv) != 0) {
		return EXIT_FAILURE;
	}
	fprintf(stderr,
	"testing with %d threads\n", global_threads);

	srand(time(NULL) ^ (uint64_t)&printf);

	if (testrun(global_threads) != 0) {
		fprintf(stderr, "FAILED\n");
		return EXIT_FAILURE;
	}
	fprintf(stderr, "PASSED\n");
	return EXIT_SUCCESS;
}
