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
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------------
 * Node
 * ------------------------------------------------------------------------
 */
typedef struct {
	beet_pageid_t      pageid;
	beet_page_t         *page;
	ts_algo_list_node_t *list;
	int                  used;
} beet_rider_node_t;

/* ------------------------------------------------------------------------
 * Create a new node
 * ------------------------------------------------------------------------
 */
static beet_err_t newNode(beet_rider_node_t **node,
                          beet_rider_t      *rider,
                          beet_pageid_t     pageid) {
	beet_err_t err;

	*node = calloc(1,sizeof(beet_rider_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	if (pageid == BEET_PAGE_NULL) {
		err = beet_page_alloc(&(*node)->page, rider->file,
		                                      rider->fsz,
		                                      rider->pagesz);
		if (err != BEET_OK) return err;
		rider->fsz += rider->pagesz;
	} else {
		(*node)->page = calloc(1, sizeof(beet_page_t));
		if ((*node)->page == NULL) {
			free(*node); return BEET_ERR_NOMEM;
		}
		err = beet_page_init((*node)->page, rider->pagesz);
		(*node)->page->pageid = pageid;
	}
	(*node)->pageid = (*node)->page->pageid;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Destroy node
 * ------------------------------------------------------------------------
 */
static void destroyNode(beet_rider_node_t *node) {
	beet_page_destroy(node->page); free(node->page);
}

/* ------------------------------------------------------------------------
 * MACRO: lock rider
 * ------------------------------------------------------------------------
 */
#define LOCK() \
	err = beet_latch_lock(&rider->latch); \
	if (err != BEET_OK) return err;

/* ------------------------------------------------------------------------
 * MACRO: unlock rider
 * ------------------------------------------------------------------------
 */
#define UNLOCK() \
	err = beet_latch_unlock(&rider->latch); \
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
	if (CONV(one)->pageid <
	    CONV(two)->pageid) return ts_algo_cmp_less;
	if (CONV(one)->pageid >
	    CONV(two)->pageid) return ts_algo_cmp_greater;
	return ts_algo_cmp_equal;
}

static ts_algo_rc_t update(void *tree, void *o, void *n) {
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
	struct stat st;
	size_t s;
	char *path;

	s = strlen(rider->base);
	s += strlen(rider->name);

	path = malloc(s+2);
	if (path == NULL) return BEET_ERR_NOMEM;

	sprintf(path, "%s/%s", rider->base, rider->name);

	if (stat(path, &st) != 0) return BEET_ERR_NOFILE;
	rider->fsz = st.st_size;

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
	err = beet_latch_init(&rider->latch);
	if (err != BEET_OK) return err;

	rider->tree = ts_algo_tree_new(
	                      &compare, NULL,
	                      &update,
	                      &delete, &delete);
	if (rider->tree == NULL) {
		beet_latch_destroy(&rider->latch);
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
	beet_latch_destroy(&rider->latch);
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

/* ------------------------------------------------------------------------
 * Helper: make room for more
 * ------------------------------------------------------------------------
 */
static beet_err_t makeRoom(beet_rider_t *rider) {
	ts_algo_list_node_t *runner;
	beet_rider_node_t *node;

	for(runner=rider->queue.last;runner!=NULL;runner=runner->prv) {
		node = runner->cont;
		if (node->used == 0) {
			// fprintf(stderr, "removing %u\n", node->pageid);
			ts_algo_list_remove(&rider->queue, runner);
			ts_algo_tree_delete(rider->tree, node);
			free(runner); break;
		}
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: check if there is room for more
 * ------------------------------------------------------------------------
 */
static inline char hasRoom(beet_rider_t *rider) {
	return (rider->max == 0 || rider->max > rider->tree->count);
}

#define READ   0
#define WRITE  1
#define CREATE 2

/* ------------------------------------------------------------------------
 * Get and lock a page for reading or writing
 * ------------------------------------------------------------------------
 */
static beet_err_t getpage(beet_rider_t *rider,
                          beet_pageid_t pageid,
                          char          x,
                          beet_page_t **page) {
	beet_err_t err = BEET_OK;
	beet_rider_node_t *node=NULL;
	beet_rider_node_t pattern;

	LOCK();
	if (x != CREATE) {
		pattern.pageid = pageid;
		node = ts_algo_tree_find(rider->tree, &pattern);
		if (node != NULL) {
			ts_algo_list_promote(&rider->queue, node->list);
			goto found;
		}
	}
	if (!hasRoom(rider)) {
		err = makeRoom(rider);
		if (err != BEET_OK) {
			UNLOCK();
			return err;
		}
	}
	if (!hasRoom(rider)) {
		UNLOCK();
		return BEET_ERR_NORSC;
	}
	if (x == CREATE) {
		err = newNode(&node, rider, BEET_PAGE_NULL);
		if (err != BEET_OK) {
			UNLOCK();
			return err;
		}
	} else {
		err = newNode(&node, rider, pageid);
		if (err != BEET_OK) {
			UNLOCK();
			return err;
		}
		err = beet_page_load(node->page, rider->file);
		if (err != BEET_OK) {
			destroyNode(node); free(node);
			UNLOCK();
			return err;
		}
	}
	if (ts_algo_list_insert(&rider->queue, node) != TS_ALGO_OK) {
		destroyNode(node); free(node);
		UNLOCK();
		return BEET_ERR_NOMEM;
	}
	node->list = rider->queue.head;
	if (ts_algo_tree_insert(rider->tree, node) != TS_ALGO_OK) {
		ts_algo_list_remove(&rider->queue, node->list);
		free(node->list);
		destroyNode(node); free(node);
		UNLOCK();
		return BEET_ERR_NOMEM;
	}
found:

	node->used++;

	UNLOCK();
	if (x == READ) {
		err = beet_lock_read(&node->page->lock);
	} else {
		err = beet_lock_write(&node->page->lock);
	}
	if (err != BEET_OK) return err;

	*page = node->page;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Release a page for reading or writing
 * ------------------------------------------------------------------------
 */
static beet_err_t releasepage(beet_rider_t *rider,
                              beet_pageid_t pageid,
                              char          x) {
	beet_err_t err;
	beet_rider_node_t pattern;
	beet_rider_node_t *node;

	pattern.pageid = pageid;
	LOCK();
	
	node = ts_algo_tree_find(rider->tree, &pattern);
	if (node == NULL) {
		UNLOCK();
		return BEET_ERR_UNKNKEY;
	}
	if (x == READ) {
		err = beet_unlock_read(&node->page->lock);
	} else {
		err = beet_unlock_write(&node->page->lock);
	}
	if (err != BEET_OK) {
		UNLOCK();
		return err;
	}
	node->used--;
	UNLOCK();
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
                                  beet_page_t   *page) {
	RIDERNULL();
	PAGENULL();
	return releasepage(rider, page->pageid, READ);
}

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseWrite(beet_rider_t *rider,
                                   beet_page_t   *page) {
	RIDERNULL();
	PAGENULL();
	return releasepage(rider, page->pageid, WRITE);
}

/* ------------------------------------------------------------------------
 * Allocate a new page
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_alloc(beet_rider_t  *rider,
                            beet_page_t  **page) {
	RIDERNULL();
	PAGENULL();
	return getpage(rider,0,CREATE,page);
}

/* ------------------------------------------------------------------------
 * Store page to disk 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_store(beet_rider_t *rider,
                            beet_page_t  *page) {
	RIDERNULL();
	PAGENULL();
	return beet_page_store(page, rider->file);
}

