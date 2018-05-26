/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * File Rider
 * ========================================================================
 * A cached File consisting of fix-sized pages
 * ========================================================================
 */
#include <beet/rider.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------
 * Node
 * ------------------------------------------------------------------------
 */
typedef struct {
	beet_page_t *page;
	char used;
} beet_rider_node_t;

/* ------------------------------------------------------------------------
 * MACRO: lock rider
 * ------------------------------------------------------------------------
 */
#define LOCK() \
	err = beet_lock_write(&rider->lock); \
	if (err != BEET_OK) return err;

/* ------------------------------------------------------------------------
 * MACRO: unlock rider
 * ------------------------------------------------------------------------
 */
#define UNLOCK() \
	err = beet_unlock_write(&rider->lock); \
	if (err != BEET_OK) return err;

/* ------------------------------------------------------------------------
 * MACRO: rider not null
 * ------------------------------------------------------------------------
 */
#define RIDERNULL() \
	if (rider == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * MACRO: page not null
 * ------------------------------------------------------------------------
 */
#define PAGENULL() \
	if (page == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * tree callbacks
 * ------------------------------------------------------------------------
 */
#define CONV(x) \
	((beet_rider_node_t*)x)

#define DEREF(x) \
	(*(beet_rider_node_t**)x)

static ts_algo_cmp_t compare(void *tree,
                             void *one,
                             void *two) {
	if (CONV(one)->page->pageid <
	    CONV(two)->page->pageid) return ts_algo_cmp_less;
	if (CONV(one)->page->pageid >
	    CONV(two)->page->pageid) return ts_algo_cmp_greater;
	return ts_algo_cmp_equal;
}

static ts_algo_rc_t update(void *tree,
                           void *o,
                           void *n) {
	return TS_ALGO_OK;
}

static void delete(void  *tree,
                   void **node) {
	if (node == NULL) return;
	if (DEREF(node)->page != NULL) {
		beet_page_destroy(DEREF(node)->page);
		free(DEREF(node)->page);
	}
	free(*node); *node = NULL;
}

/* ------------------------------------------------------------------------
 * Helper: open file
 * ------------------------------------------------------------------------
 */
static inline beet_err_t openFile(beet_rider_t *rider) {
	size_t s;
	char *path;

	s = strlen(rider->base);
	s += strlen(rider->name);

	path = malloc(s+2);
	if (path == NULL) return BEET_ERR_NOMEM;

	sprintf(path, "%s/%s", rider->base, rider->name);

	rider->file = fopen(path, "rb+"); free(path);
	if (rider->file == NULL) return BEET_OSERR_OPEN;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Initialise the rider
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_init(beet_rider_t    *rider,
                           char *base, char *name,
                           uint32_t        pagesz,
                           uint32_t           max) {
	beet_err_t err;
	size_t s;
	
	RIDERNULL();
	if (base == NULL || name == NULL) return BEET_ERR_INVALID;

	rider->max = max;
	rider->pagesz = pagesz;
	rider->base = NULL;
	rider->name = NULL;
	rider->file = NULL;
	rider->tree = NULL;

	ts_algo_list_init(&rider->queue);
	err = beet_lock_init(&rider->lock);
	if (err != BEET_OK) return err;

	rider->tree = ts_algo_tree_new(
	                      &compare, NULL,
	                      &update,
	                      &delete, &delete);
	if (rider->tree == NULL) {
		beet_lock_destroy(&rider->lock);
		ts_algo_list_destroy(&rider->queue);
		return BEET_ERR_NOMEM;
	}

	s = strnlen(base, 4097);
	if (s >= 4096) {
		err = BEET_ERR_INVALID;
		goto cleanup;
	}

	rider->base = strdup(base);
	if (rider->base == NULL) {
		err = BEET_ERR_NOMEM;
		goto cleanup;
	}

	s = strnlen(name, 4097);
	if (s >= 4096) {
		err = BEET_ERR_INVALID;
		goto cleanup;
	}

	rider->name = strdup(name);
	if (rider->name == NULL) {
		err = BEET_ERR_INVALID;
		goto cleanup;
	}

	err = openFile(rider);
	if (err != BEET_OK) goto cleanup;

	return BEET_OK;
	
cleanup:
	beet_rider_destroy(rider);
	return err;
}

/* ------------------------------------------------------------------------
 * Destroy the rider
 * ------------------------------------------------------------------------
 */
void beet_rider_destroy(beet_rider_t *rider) {
	if (rider == NULL) return;
	beet_lock_destroy(&rider->lock);
	ts_algo_list_destroy(&rider->queue);
	if (rider->base != NULL) {
		free(rider->base); rider->base = NULL;
	}
	if (rider->name != NULL) {
		free(rider->name); rider->name = NULL;
	}
	if (rider->tree != NULL) {
		ts_algo_tree_destroy(rider->tree);
		free(rider->tree); rider->tree = NULL;
	}
	if (rider->file != NULL) {
		fclose(rider->file); rider->file = NULL;
	}
}

#define READ  0
#define WRITE 1

/* ------------------------------------------------------------------------
 * Get and lock a page for reading or writing
 * ------------------------------------------------------------------------
 */
static beet_err_t getpage(beet_rider_t *rider,
                          beet_pageid_t pageid,
                          char          x,
                          beet_page_t **page) {
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Release a page for reading or writing
 * ------------------------------------------------------------------------
 */
static beet_err_t releasepage(beet_rider_t *rider,
                              beet_pageid_t pageid,
                              char          x) {
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get the page identified by 'pageid' for reading
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_getRead(beet_rider_t  *rider,
                              beet_pageid_t pageid,
                              beet_page_t  **page) {
	RIDERNULL();
	PAGENULL();
	return getpage(rider, pageid, READ, page);
}

/* ------------------------------------------------------------------------
 * Get the page identified by 'pageid' for writing
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_getWrite(beet_rider_t *rider,
                               beet_pageid_t pageid,
                               beet_page_t **page) {
	RIDERNULL();
	PAGENULL();
	return getpage(rider, pageid, WRITE, page);
}

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for reading 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseRead(beet_rider_t *rider,
                                  beet_pageid_t pageid) {
	RIDERNULL();
	return releasepage(rider, pageid, READ);
}

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseWrite(beet_rider_t *rider,
                                   beet_pageid_t pageid) {
	RIDERNULL();
	return releasepage(rider, pageid, READ);
}

/* ------------------------------------------------------------------------
 * Allocate a new page
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_alloc(beet_rider_t  *rider,
                            beet_page_t  **page);

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * promising it has not been written
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseClean(beet_rider_t *rider,
                                   beet_pageid_t pageid);
