/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Page
 * ========================================================================
 * A page is the unit of I/O.
 * A page can be allocated, loaded and stored.
 * TODO:
 * - consider signals?
 * ========================================================================
 */
#include <beet/page.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGENULL() \
	if (page == NULL) return BEET_ERR_NOPAGE;

#define STORENULL() \
	if (store == NULL) return BEET_ERR_NOFD;

/* ------------------------------------------------------------------------
 * Allocate a new page in a file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_alloc(beet_page_t **page,
                           FILE *store, off_t pos,
                           uint32_t sz) {
	beet_err_t err;

	PAGENULL();
	STORENULL();

	/*
	fprintf(stderr, "new page at %d.%lu: %lu\n",
	                     fileno(store), pos, k);
	*/

	*page = calloc(1, sizeof(beet_page_t));
	if (*page == NULL) return BEET_ERR_NOMEM;

	err = beet_page_init(*page, (uint32_t)sz);
	if (err != BEET_OK) return err;

	if (pwrite(fileno(store), (*page)->data, sz, pos) != sz) {
		beet_page_destroy(*page); free(*page);
		return BEET_OSERR_WRITE;
	}
	(*page)->pageid = (beet_pageid_t)(pos/sz);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Initialise an already allocated page in a file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_init(beet_page_t *page, uint32_t sz) {
	beet_err_t err;

	PAGENULL();

	page->data = NULL;
	page->sz = sz;
	page->pageid = 0;
	err = beet_lock_init(&page->lock);
	if (err != BEET_OK) return err;
	page->data = calloc(1,sz);
	if (page->data == NULL) {
		beet_lock_destroy(&page->lock);
		return BEET_ERR_NOMEM;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Destroy page
 * ------------------------------------------------------------------------
 */
void beet_page_destroy(beet_page_t *page) {
	if (page == NULL) return;
	if (page->data != NULL) {
		free(page->data); page->data = NULL;
	}
	beet_lock_destroy(&page->lock);
}

/* ------------------------------------------------------------------------
 * Load page from store into memory
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_load(beet_page_t *page, FILE *store) {
	off_t s;

	PAGENULL();
	STORENULL();

	s = (size_t)page->pageid * (size_t)page->sz;
	if (pread(fileno(store), page->data, page->sz, s) != page->sz) {
		return BEET_OSERR_READ;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Write page in memory to store
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_store(beet_page_t *page, FILE *store) {
	off_t s;

	PAGENULL();
	STORENULL();

	s = (size_t)page->pageid * (size_t)page->sz;

	if (pwrite(fileno(store), page->data, page->sz, s) != page->sz) {
		return BEET_OSERR_WRITE;
	}
	return BEET_OK;
}
