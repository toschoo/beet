/* ========================================================================
 * Simple Tests for rider
 * ========================================================================
 */
#include <beet/rider.h>
#include <common/math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SIZE   16
#define BIG   160
#define BYTES 128

uint64_t buf[SIZE];
uint64_t bigbuf[BIG];

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
	err = beet_rider_init(rider, base, name, BYTES, 8);
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

int testInitDestroy() {
	beet_rider_t rider;
	char *name = "test1.bin";
	char *path = "rsc";

	if (createFile(path, name) != 0) return -1;
	if (initRider(&rider, path, name) != 0) return -1;
	beet_rider_destroy(&rider);

	return 0;
}

int testWriteFibs(beet_rider_t *rider) {
	beet_page_t *page;
	beet_err_t    err;
	uint64_t f1=1, f2=1;

	for(int i=0;i<10;i++) {
		fprintf(stderr, "allocating page %d\n", i);
		err = beet_rider_alloc(rider, &page);
		if (err != BEET_OK) {
			errmsg(err, "cannot allocate page");
			return -1;
		}
		fibonacci_r((uint64_t*)page->data, SIZE, f1, f2);
		memcpy(&f1, page->data+112, 8);
		memcpy(&f2, page->data+120, 8);
		err = beet_page_store(page, rider->file);
		if (err != BEET_OK) {
			errmsg(err, "cannot store page");
			return -1;
		}
		err = beet_rider_releaseWrite(rider, page);
		if (err != BEET_OK) {
			errmsg(err, "cannot store page");
			return -1;
		}
	}
	return 0;
}

int comp(uint64_t *buf1, uint64_t *buf2, int size) {
	for(int i=0; i<size; i++) {
		/*
		fprintf(stderr, "%lu == %lu\n", buf1[i], buf2[i]);
		*/
		if (buf1[i] != buf2[i]) {
			fprintf(stderr, "%d differs: %lu - %lu\n",
			                     i, buf1[i], buf2[i]);
			return -1;
		}
	}
	return 0;
}

int testReadFibs(beet_rider_t *rider) {
	beet_page_t *page;
	beet_err_t    err;
	int x = 0;

	fibonacci(bigbuf, BIG);

	for(uint32_t i=0;i<10;i++) {
		err = beet_rider_getRead(rider, i, &page);
		if (err != BEET_OK) {
			errmsg(err, "cannot get page for reading");
			return -1;
		}
		if (comp((uint64_t*)page->data, bigbuf+x, SIZE) != 0) {
			return -1;
		}
		x+=BYTES/8;
		err = beet_rider_releaseRead(rider, page);
		if (err != BEET_OK) {
			errmsg(err, "cannot release page");
			return -1;
		}
	}
	return 0;
}

int testRandomRead(beet_rider_t *rider) {
	beet_page_t *page;
	beet_err_t    err;

	for(int i=0;i<10;i++) {
		uint32_t pid = rand()%10;
		fprintf(stderr, "obtaining page %u\n", pid);
		err = beet_rider_getRead(rider, pid, &page);
		if (err != BEET_OK) {
			errmsg(err, "cannot get page for reading");
			return -1;
		}
		if (page->pageid != pid) {
			fprintf(stderr, "wrong page: %u != %u\n",
			                      page->pageid, pid);
		}
		err = beet_rider_releaseRead(rider, page);
		if (err != BEET_OK) {
			errmsg(err, "cannot release page");
			return -1;
		}
	}
	return 0;
}

int main() {
	char *path = "rsc";
	char *name = "test1.bin";
	int rc = EXIT_SUCCESS;
	beet_rider_t rider;
	char haveRider = 0;

	srand(time(NULL));

	if (testInitDestroy() != 0) {
		fprintf(stderr, "testInitDestroy failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (initRider(&rider, path, name) != 0) {
		fprintf(stderr, "cannot init rider\n");
		rc = EXIT_FAILURE; goto cleanup;
		return -1;
	}
	haveRider = 1;
	if (testWriteFibs(&rider) != 0) {
		fprintf(stderr, "testWriteFibs failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadFibs(&rider) != 0) {
		fprintf(stderr, "testReadFibs failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testRandomRead(&rider) != 0) {
		fprintf(stderr, "testRandomRead failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (haveRider) beet_rider_destroy(&rider);
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}

