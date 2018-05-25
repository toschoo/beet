/* ========================================================================
 * Simple Tests from page
 * ========================================================================
 */
#include <beet/page.h>
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

void printFibs() {
	for(int i=0;i<SIZE;i++) {
		fprintf(stderr, "%lu ", buf[i]);
	}
	fprintf(stderr, "\n");
}

int testFibo() {
	fibonacci(bigbuf, BIG);
	return 0;
}

int writeFibs(FILE *store) {
	uint64_t f1=1, f2=1;
	beet_err_t err;
	beet_page_t *page;
	
	for(int i=0; i<10;i++) {
		err = beet_page_alloc(&page,store,BYTES);
		if (err != BEET_OK) {
			fprintf(stderr, "cannot allocate page: %d\n", err);
			return -1;
		}
		fprintf(stderr, "writing page %d\n", page->pageid);
		fibonacci_r((uint64_t*)page->data, SIZE, f1, f2);
		memcpy(&f1, page->data+112, 8);
		memcpy(&f2, page->data+120, 8);
		err = beet_page_store(page,store);
		if (err != BEET_OK) {
			fprintf(stderr, "cannot store page: %d\n", err);
			return -1;
		}
		beet_page_destroy(page); free(page);
	}
	return 0;
}

int testWriteFibs(char *path) {
	FILE *store;

	store = fopen(path, "wb");
	if (store == NULL) {
		fprintf(stderr, "cannot create file\n");
		return -1;
	}
	if (writeFibs(store) != 0) {
		fclose(store); return -1;
	}
	if (fclose(store) != 0) {
		fprintf(stderr, "cannot close file\n");
		return -1;
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

int testReadFibs(char *path) {
	FILE *store;
	beet_page_t *page;
	beet_err_t err;
	int x = 0;

	store = fopen(path, "rb");
	if (store == NULL) {
		fprintf(stderr, "cannot create file\n");
		return -1;
	}
	page = calloc(1,sizeof(beet_page_t));
	if (page == NULL) {
		fprintf(stderr, "out-of-mem\n");
		fclose(store); return -1;
	}
	err = beet_page_init(page,BYTES);
	if (err != BEET_OK) {
		fprintf(stderr, "cannot init page: %d\n", err);
		fclose(store); beet_page_destroy(page);
		free(page); return -1;
	}
	for(int i=0; i<10; i++) {
		fprintf(stderr, "reading page %d\n", page->pageid);
		err = beet_page_load(page, store);
		if (err != BEET_OK) {
			fprintf(stderr, "cannot load page: %d\n", err);
			if (err < BEET_OSERR_ERRNO) {
				fprintf(stderr, "%s\n", beet_oserrdesc());
			} else {
				fprintf(stderr, "%s\n", beet_errdesc(err));
			}
			beet_page_destroy(page); free(page);
			fclose(store); return -1;
		}
		if (comp((uint64_t*)page->data, bigbuf+x, SIZE) != 0) {
			beet_page_destroy(page); free(page);
			fclose(store); return -1;
		}
		x+=BYTES/8;
		page->pageid++;
	}
	beet_page_destroy(page); free(page);
	if (fclose(store) != 0) {
		fprintf(stderr, "cannot close file\n");
		return -1;
	}
	return 0;
}

int main() {
	int rc = EXIT_SUCCESS;
	
	if (testFibo() != 0) {
		fprintf(stderr, "testFibo_r failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testWriteFibs("rsc/pages.bin") != 0) {
		fprintf(stderr, "testWriteFibo failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}
	if (testReadFibs("rsc/pages.bin") != 0) {
		fprintf(stderr, "testReadFibo failed\n");
		rc = EXIT_FAILURE; goto cleanup;
	}

cleanup:
	if (rc == EXIT_SUCCESS) {
		fprintf(stderr, "PASSED\n");
	} else {
		fprintf(stderr, "FAILED\n");
	}
	return rc;
}
