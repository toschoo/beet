/* ========================================================================
 * Simple Tests for node
 * ========================================================================
 */
#include <beet/rider.h>
#include <beet/node.h>
#include <common/math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define NODESZ 14
#define BIG   160
#define BYTES 128
#define KEYSZ   4

void errmsg(beet_err_t err, char *s) {
	fprintf(stderr, "%s: %s (%d)\n",
	     s, beet_errdesc(err), err);
	if (err < BEET_OSERR_ERRNO) {
		fprintf(stderr, "OSERR: %s\n",
		            beet_oserrdesc());
	}
}

int initRider(beet_rider_t *rider, char *base, char *name) {
	beet_err_t err;
	err = beet_rider_init(rider, base, name, BYTES, 10);
	if (err != BEET_OK) {
		errmsg(err, "cannot initialise rider");
		return -1;
	}
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

#define DEREF(x) \
	(*(int*)x)

ts_algo_cmp_t compare(void *ignore, void *one, void *two) {
	if (DEREF(one) < DEREF(two)) return ts_algo_cmp_less;
	if (DEREF(one) > DEREF(two)) return ts_algo_cmp_greater;
	return ts_algo_cmp_equal;
}

int testWriteNums(beet_rider_t *rider) {
	beet_page_t *page;
	beet_node_t  node;
	beet_err_t    err;
	char        wrote;

	err = beet_rider_alloc(rider, &page);
	if (err != BEET_OK) {
		errmsg(err, "cannot allocate page");
		return -1;
	}
	beet_node_init(&node, page, NODESZ, KEYSZ, 1);
	node.next = BEET_PAGE_NULL;
	node.prev = BEET_PAGE_NULL;

	for(int z=NODESZ-1;z>=0;z--) {
		err = beet_node_add(&node, NODESZ, KEYSZ, 0, 
		       &z, NULL, &compare, NULL, 1, &wrote);
		if (err != BEET_OK) {
			errmsg(err, "cannot write to node");
			return -1;
		}
		if (!wrote) {
			fprintf(stderr, "didn't write???\n");
			return -1;
		}
	}

	beet_node_serialise(&node);

	err = beet_rider_store(rider, node.page);
	if (err != BEET_OK) {
		errmsg(err, "cannot store page");
		return -1;
	}
	err = beet_rider_releaseWrite(rider, node.page);
	if (err != BEET_OK) {
		errmsg(err, "cannot store page");
		return -1;
	}
	return 0;
}

int testReadRandom(beet_rider_t *rider) {
	beet_page_t *page;
	beet_node_t  node;
	beet_err_t    err;
	int k;
	int32_t slot;

	err = beet_rider_getRead(rider, 0, &page);
	if (err != BEET_OK) {
		errmsg(err, "cannot get page");
		return -1;
	}

	beet_node_init(&node, page, NODESZ, KEYSZ, 1);

	for(int i=0;i<50;i++) {
		k=rand()%NODESZ;
		slot = beet_node_search(&node, KEYSZ, &k, compare);
		if (slot < 0) {
			fprintf(stderr, "unexpected result: %d\n", k);
			return -1;
		}
		if (!beet_node_equal(&node, slot, KEYSZ, &k, compare)) {
			fprintf(stderr, "key not found: %d (%d)\n", k, slot);
			return -1;
		}
		fprintf(stderr, "found: %d in %d\n", k, slot);
	}
	err = beet_rider_releaseRead(rider, page);
	if (err != BEET_OK) {
		errmsg(err, "cannot release page");
		return -1;
	}
	return 0;

}

int main() {
	char *path = "rsc";
	char *name = "test5.bin";
	int rc = EXIT_SUCCESS;
	beet_rider_t rider;
	char haveRider = 0;

	srand(time(NULL));

	if (createFile(path, name) != 0) {
		fprintf(stderr, "cannot create file\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (initRider(&rider, path, name) != 0) {
		fprintf(stderr, "cannot init rider\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	haveRider = 1;
	if (testWriteNums(&rider) != 0) {
		fprintf(stderr, "testWriteNums failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadRandom(&rider) != 0) {
		fprintf(stderr, "testReadRandom failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	// find pageid
	// get pageid
	// getData
	// getKey

cleanup:
	if (haveRider) beet_rider_destroy(&rider);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}

