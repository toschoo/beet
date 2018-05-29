/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * B+Tree Abstraction
 * ========================================================================
 */

#include <beet/tree.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ------------------------------------------------------------------------
 * Read/Write 
 * ------------------------------------------------------------------------
 */
#define READ  0
#define WRITE 1

/* ------------------------------------------------------------------------
 * Macro: tree not null
 * ------------------------------------------------------------------------
 */
#define TREENULL() \
	if (tree == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * Init B+Tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_init(beet_tree_t     *tree,
                          uint32_t        lsize,
                          uint32_t        nsize,
                          uint32_t        ksize,
                          uint32_t        dsize,
                          beet_rider_t   *nolfs,
                          beet_rider_t     *lfs,
                          ts_algo_comprsc_t cmp,
                          beet_ins_t       *ins) {
	TREENULL();

	tree->lsize = lsize;
	tree->nsize = nsize;
	tree->ksize = ksize;
	tree->dsize = dsize;
	tree->nolfs = nolfs;
	tree->lfs   = lfs;
	tree->cmp   = cmp;
	tree->ins   = ins;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Destroy B+Tree
 * ------------------------------------------------------------------------
 */
void beet_tree_destroy(beet_tree_t *tree) {
	if (tree == NULL) return;
	if (tree->ins != NULL) {
		tree->ins->cleaner(tree->ins);
		free(tree->ins); tree->ins = NULL;
	}
	if (tree->nolfs != NULL) {
		beet_rider_destroy(tree->nolfs);
		free(tree->nolfs); tree->nolfs = NULL;
	}
	if (tree->lfs != NULL) {
		beet_rider_destroy(tree->lfs);
		free(tree->lfs); tree->lfs = NULL;
	}
}

/* ------------------------------------------------------------------------
 * Helper: check whether pageid is leaf
 * ------------------------------------------------------------------------
 */
static inline char isLeaf(beet_pageid_t pge) {
	if (pge & BEET_PAGE_LEAF) return 1;
	return 0;
}

/* ------------------------------------------------------------------------
 * Helper: convert pageid to leaf
 * ------------------------------------------------------------------------
 */
static inline beet_pageid_t toLeaf(beet_pageid_t pge) {
	return (pge | BEET_PAGE_LEAF);
}

/* ------------------------------------------------------------------------
 * Helper: convert pageid from leaf
 * ------------------------------------------------------------------------
 */
static inline beet_pageid_t fromLeaf(beet_pageid_t pge) {
	return (pge ^ BEET_PAGE_LEAF);
}

/* ------------------------------------------------------------------------
 * Helper: allocate leaf node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t newLeaf(beet_tree_t  *tree,
                                 beet_node_t **node) {
	beet_err_t err;
	beet_page_t *page;

	err = beet_rider_alloc(tree->lfs, &page);
	if (err != BEET_OK) return err;

	*node = calloc(1, sizeof(beet_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	beet_node_init(*node, page, tree->lsize, tree->ksize, 1);

	(*node)->next = BEET_PAGE_NULL;
	(*node)->prev = BEET_PAGE_NULL;
	(*node)->mode = WRITE;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: allocate nonleaf node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t newNonLeaf(beet_tree_t  *tree,
                                    beet_node_t **node) {
	beet_err_t err;
	beet_page_t *page;

	err = beet_rider_alloc(tree->nolfs, &page);
	if (err != BEET_OK) return err;

	*node = calloc(1, sizeof(beet_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	beet_node_init(*node, page, tree->nsize, tree->ksize, 0);

	(*node)->mode = WRITE;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: Get node from rider
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getNode(beet_tree_t  *tree,
                                 beet_pageid_t  pge,
                                 char          mode,
                                 beet_node_t **node) 
{
	beet_err_t    err;
	beet_page_t *page;
	beet_rider_t  *rd;
	char         leaf;
	uint32_t       sz;

	if (isLeaf(pge)) {
		rd = tree->lfs;
		leaf = 1;
		sz = tree->lsize;
	} else {
		rd = tree->nolfs;
		leaf = 0;
		sz = tree->nsize;
	}

	*node = calloc(1,sizeof(beet_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	if (mode == READ) {
		err = beet_rider_getRead(rd, fromLeaf(pge), &page);
	} else {
		err = beet_rider_getWrite(rd, fromLeaf(pge), &page);
	}
	if (err != BEET_OK) {
		free(*node); *node = NULL;
		return err;
	}

	beet_node_init(*node, page, sz, tree->ksize, leaf);

	(*node)->mode = mode;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: Release node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t releaseNode(beet_tree_t *tree,
                                     beet_node_t *node) {
	beet_err_t err;
	beet_rider_t *rd = node->leaf ? tree->lfs : tree->nolfs;

	if (node->mode == READ) {
		err = beet_rider_releaseRead(rd, node->page);
	} else {
		err = beet_rider_releaseWrite(rd, node->page);
	}
	if (err != BEET_OK) return err;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: split node
 * ------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------
 * Helper: insert (key,data) into node
 * ------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------
 * Insert data into the tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_insert(beet_tree_t  *tree,
                            beet_pageid_t root,
                            const void    *key,
                            const void   *data) {
	TREENULL();
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get the node that contains the given key
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_get(beet_tree_t  *tree,
                         beet_pageid_t root,
                         const void    *key,
                         beet_node_t **node);

/* ------------------------------------------------------------------------
 * Get the leftmost node
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_left(beet_tree_t  *tree,
                          beet_pageid_t root,
                          beet_node_t **node);

/* ------------------------------------------------------------------------
 * Get the rightmost node
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_right(beet_tree_t  *tree,
                           beet_pageid_t root,
                           beet_node_t **node);

/* ------------------------------------------------------------------------
 * Apply function on all (key,data) pairs in range (a.k.a. 'map')
 * ------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------
 * Fold all (key,data) pairs in range to result (a.k.a. 'reduce')
 * ------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------
 * Height of the tree (debugging)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_height(beet_tree_t  *tree,
                            beet_pageid_t root,
                            uint32_t       *h);
