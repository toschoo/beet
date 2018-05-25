/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Page
 * ========================================================================
 * A page is the unit of I/O
 * ========================================================================
 */
#include <beet/page.h>
#include <string.h>

#define PAGENULL() \
	if (page == NULL) return BEET_ERR_INVALID;

#define STORENULL() \
	if (store == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * Allocate a new page in a file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_alloc(beet_page_t **page, FILE *store, uint32_t sz) {
	ssize_t k;
	beet_err_t err;
	PAGENULL();
	STORENULL();
	if (fseek(store, 0, SEEK_END) != 0) return BEET_OSERR_SEEK;
	k = ftell(store); if (k < 0) return BEET_OSERR_TELL;
	*page = calloc(1, sizeof(beet_page_t));
	if (*page == NULL) return BEET_ERR_NOMEM;
	err = beet_page_init(*page, (uint32_t)sz);
	if (err != BEET_OK) return err;
	if (fwrite((*page)->data, sz, 1, store) != 1) {
		beet_page_destroy(*page); free(*page);
		return BEET_OSERR_WRITE;
	}
	if (fflush(store) != 0) {
		beet_page_destroy(*page); free(*page);
		return BEET_OSERR_WRITE;
	}
	(*page)->pos = k/sz;
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
	page->pos = 0;
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
	size_t s;
	PAGENULL();
	STORENULL();
	s = (size_t)page->pos * (size_t)page->sz;
	if (fseek(store, s, SEEK_SET) != 0) return BEET_OSERR_SEEK;
	if (fread(page->data, page->sz, 1, store) != 1) {
		return BEET_OSERR_READ;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Write page in memory to store
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_store(beet_page_t *page, FILE *store) {
	size_t s;
	PAGENULL();
	STORENULL();
	s = (size_t)page->pos * (size_t)page->sz;
	if (fseek(store, s, SEEK_SET) != 0) return BEET_OSERR_SEEK;
	if (fwrite(page->data, page->sz, 1, store) != 1) {
		return BEET_OSERR_WRITE;
	}
	if (fflush(store) != 0) return BEET_OSERR_FLUSH;
	return BEET_OK;
}
