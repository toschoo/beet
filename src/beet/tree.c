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
 * Macro: lock roof
 * ------------------------------------------------------------------------
 */
#define LOCK() \
	if (tree->roof != NULL) {\
		err = beet_latch_lock(&tree->rlock); \
		if (err != BEET_OK) return err; \
		tree->locked = 1; \
	}

/* ------------------------------------------------------------------------
 * Macro: unlock roof
 * ------------------------------------------------------------------------
 */
#define UNLOCK() \
	if (tree->roof != NULL && tree->locked) { \
		err = beet_latch_unlock(&tree->rlock); \
		if (err != BEET_OK) return err; \
		tree->locked = 0; \
	}

/* ------------------------------------------------------------------------
 * Macro: store root
 * ------------------------------------------------------------------------
 */
#define STOREROOT(root) \
	if (tree->roof != NULL) { \
		if (fseek(tree->roof, 0, SEEK_SET) !=0) \
			return BEET_OSERR_SEEK; \
		if (fwrite(root, sizeof(beet_pageid_t), 1, tree->roof) != 1) \
			return BEET_OSERR_SEEK; \
		if (fflush(tree->roof) != 0) return BEET_OSERR_FLUSH; \
	}

/* ------------------------------------------------------------------------
 * Destroy B+Tree
 * ------------------------------------------------------------------------
 */
void beet_tree_destroy(beet_tree_t *tree) {
	if (tree == NULL) return;
	beet_latch_destroy(&tree->rlock);
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

	for(;;) {
		err = beet_rider_alloc(tree->lfs, &page);
		if (err == BEET_OK) break;
		if (err == BEET_ERR_NORSC) continue;
		if (err != BEET_OK) return err;
	}

	*node = calloc(1, sizeof(beet_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	beet_node_init(*node, page, tree->lsize, tree->ksize, 1);

	(*node)->next = BEET_PAGE_NULL;
	(*node)->prev = BEET_PAGE_NULL;
	(*node)->mode = WRITE;

	/* insert-specific initialiser */
	if (tree->ins != NULL) {
		tree->ins->ninit(tree->ins, tree->lsize, (*node)->kids);
	}
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

	for(;;) {
		err = beet_rider_alloc(tree->nolfs, &page);
		if (err == BEET_OK) break;
		if (err == BEET_ERR_NORSC) continue;
		if (err != BEET_OK) return err;
	}

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
	beet_page_t *page=NULL;
	beet_err_t    err;
	beet_rider_t  *rd;
	char         leaf;
	uint32_t       sz;
	beet_pageid_t pid;

	if (isLeaf(pge)) {
		rd = tree->lfs;
		leaf = 1;
		sz = tree->lsize;
		pid = fromLeaf(pge);
	} else {
		rd = tree->nolfs;
		leaf = 0;
		sz = tree->nsize;
		pid = pge;
	}

	*node = calloc(1,sizeof(beet_node_t));
	if (*node == NULL) return BEET_ERR_NOMEM;

	for(;;) {
		if (mode == READ) {
			err = beet_rider_getRead(rd, pid, &page);
		} else {
			err = beet_rider_getWrite(rd, pid, &page);
		}
		if (err == BEET_OK && page == NULL) {
			fprintf(stderr, "getting page %u: %p\n", pid, page);
			return BEET_ERR_PANIC;
		}
		if (err == BEET_OK) break;
		if (err == BEET_ERR_NORSC) continue;
		if (err != BEET_OK) {
			free(*node); *node = NULL;
			return err;
		}
	}

	beet_node_init(*node, page, sz, tree->ksize, leaf);

	(*node)->mode = mode;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: store node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t storeNode(beet_tree_t *tree,
                                   beet_node_t *node) {
	beet_err_t err;

	beet_node_serialise(node);

	if (node->leaf) {
		err = beet_rider_store(tree->lfs, node->page);
	} else {
		err = beet_rider_store(tree->nolfs, node->page);
	}
	return err;
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
 * Allocate first node in tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_makeRoot(beet_tree_t *tree, beet_pageid_t *root) {
	beet_node_t *node;
	beet_err_t    err;

	err = newLeaf(tree, &node);
	if (err != BEET_OK) return err;

	*root = toLeaf(node->self);

	STOREROOT(root);

	err = storeNode(tree, node);
	if (err != BEET_OK) {
		free(node); return err;
	}

	err = releaseNode(tree, node);
	if (err != BEET_OK) {
		free(node); return err;
	}
	free(node);
	return BEET_OK;
}

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
                          FILE            *roof,
                          ts_algo_comprsc_t cmp,
                          beet_ins_t       *ins) {
	beet_err_t    err;

	TREENULL();

	tree->lsize  = lsize;
	tree->nsize  = nsize;
	tree->ksize  = ksize;
	tree->dsize  = dsize;
	tree->nolfs  = nolfs;
	tree->lfs    = lfs;
	tree->roof   = roof;
	tree->cmp    = cmp;
	tree->ins    = ins;
	tree->locked = 0;

	/* init root file latch */
	err = beet_latch_init(&tree->rlock);
	if (err != BEET_OK) return err;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: set previous of node.next to node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t setPrev(beet_tree_t *tree,
                                 beet_node_t *node) {
	beet_node_t *node2;
	beet_err_t     err;

	/* if not next: ok */
	if (node->next == BEET_PAGE_NULL) return BEET_OK;

	/* load next */
	 err = getNode(tree, toLeaf(node->next), WRITE, &node2);
	if (err != BEET_OK) return err;
	
	/* set prev of node.next to node */
	node2->prev = node->self;

	/* store and release */
	err = storeNode(tree, node2);
	if (err != BEET_OK) {
		releaseNode(tree, node2); free(node2);
		return err;
	}
	err = releaseNode(tree, node2);
	if (err != BEET_OK) {
		free(node2); return err;
	}
	free(node2);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: split node
 * ------------------------------------------------------------------------
 */
static inline beet_err_t split(beet_tree_t *tree,
                               beet_node_t  *src,
                               beet_node_t **trg) {
	beet_err_t err;
	uint32_t off, sz;
	uint32_t bsz,nsz;
	char *srk;

	if (src->leaf) {
		err = newLeaf(tree, trg);
	} else {
		err = newNonLeaf(tree, trg);
	}
	if (err != BEET_OK) return err;

	/* half of the keys stay in the old node,
 	 * the other half goes to the new node.
 	 * If size is odd (e.g. 7):
 	 * -> 7/2 = 3 (old node)
 	 * -> 7-3 = 4 (new node) */
	off = src->size/2 * tree->ksize;
	sz  = src->size * tree->ksize - off;
	srk = src->keys + off;
	(*trg)->size = src->size - src->size/2;

	/* in the case of a nonleaf node,
 	 * the first key in trg has no function
 	 * anymore: it is not a splitter!
 	 * so, we step over it */
	if (!src->leaf) {
		sz -= tree->ksize;
		srk += tree->ksize;
		(*trg)->size--;
	}

	memcpy((*trg)->keys, srk, sz);

	/* same logic as above, but, in a leaf,
 	 * we need to consider the next and prev pointers
 	 * and, in a nonleaf, we need to consider
 	 * the last pointer (which stays in the old node!) */
	if (src->leaf) {

		/* a -> node ->         b =>
		 * a -> node -> trg  -> b and
		 * a <- node         <- b =>
		 * a <- node <- trg  <- b */
		(*trg)->next = src->next;
		src->next  = (*trg)->self;
		(*trg)->prev = src->self;

		err = setPrev(tree,*trg);
		if (err != BEET_OK) {
			releaseNode(tree, *trg);
			free(*trg); *trg = NULL;
			return err;
		}

		bsz = tree->dsize;
		if (bsz > 0) {
			off = src->size/2 * bsz;
			sz  = src->size * bsz - off;
			srk = src->kids + off;
		}
	} else {
		/* the following diagram shows
		 * what is copied and what stays;
		 * fields shaded '\\' stay,
		 * those shaded '//' move.
		 *
		 * The field left blank is ignored
		 * and used as splitter for the nodes.
		 *
		 * 1) size is even
		 *    +-----------+
		 *    |\\|\\|  |//|
		 *    +-----------+--+
		 *    |\\|\\|\\|//|//|
		 *    +-----------+--+
		 *
		 * 2) size is odd    
		 *    +-----------+--+
		 *    |\\|\\|  |//|//|
		 *    +-----------+--+--+
		 *    |\\|\\|\\|//|//|//|
		 *    +-----------+--+--+
		 */
		bsz = sizeof(beet_pageid_t);
		nsz = src->size+1;   
		sz  = nsz/2 * bsz;    
		off = nsz * bsz - sz; 
		srk = src->kids + off;
	}

	if (bsz > 0) memcpy((*trg)->kids, srk, sz);
	src->size /= 2;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: add key to predecessor (predeclaration)
 * ------------------------------------------------------------------------
 */
static beet_err_t add2mom(beet_tree_t   *tree,
                          beet_pageid_t *root,
                          beet_node_t  *node1,
                          beet_node_t  *node2,
                          const void     *key,
                   ts_algo_list_node_t *nodes);

/* ------------------------------------------------------------------------
 * Helper: insert (key,data) into node
 * ------------------------------------------------------------------------
 */
static beet_err_t insert(beet_tree_t   *tree,
                         beet_pageid_t *root,
                         beet_node_t   *node,
                         const void     *key,
                         const void    *data,
                 ts_algo_list_node_t  *nodes) 
{
	beet_node_t *node2=NULL;
	void *s;
	beet_err_t err;
	char     wrote;

	err = beet_node_add(node, tree->nsize,
			          tree->ksize,
			          tree->dsize,
			            key, data,        
			          tree->cmp,
			          tree->ins,
	                          &wrote);
	if (err != BEET_OK) return err;
	if (wrote) {
		err = storeNode(tree, node);
		if (err != BEET_OK) return err;
	}

	/* we need to split */
	if ((node->leaf && node->size == tree->lsize) ||
	   (!node->leaf && node->size == tree->nsize)) {

		err = split(tree, node, &node2);
		if (err != BEET_OK) return err;

		/* get splitter (see diagram above) */
		s = node->leaf ? node2->keys :
		                 node->keys  +
		                 node->size * tree->ksize;

		/* add the splitter to parent node */
		err = add2mom(tree, root, node, node2, s, nodes);
		if (err != BEET_OK) return err;

		/* store new node */
		err = storeNode(tree, node2);
		if (err != BEET_OK) return err;

		/* release new node */
		err = releaseNode(tree, node2);
		if (err != BEET_OK) return err;
		free(node2);  /* on error, we leak memory */
	}
	return storeNode(tree, node);
}

/* ------------------------------------------------------------------------
 * Helper: add2mom
 * ------------------------------------------------------------------------
 */
static beet_err_t add2mom(beet_tree_t   *tree,
                          beet_pageid_t *root,
                          beet_node_t  *node1,
                          beet_node_t  *node2,
                          const void     *key,
                   ts_algo_list_node_t *nodes)
{
	beet_err_t   err;
	beet_node_t *mom;
	beet_pageid_t p1, p2;

	if (node1->leaf) {
		p1 = toLeaf(node1->self);
		p2 = toLeaf(node2->self);
	} else {
		p1 = node1->self;
		p2 = node2->self;
	}

	/* we ran out of nodes. That means
	 * we reached the top of the tree
	 * and need a new root node. */
	if (nodes == NULL) {
		err = newNonLeaf(tree, &mom);
		if (err != BEET_OK) return err;

		mom->size = 1;

		memcpy(mom->keys, key, tree->ksize);
		memcpy(mom->kids, &p1, sizeof(beet_pageid_t));
		memcpy(mom->kids+sizeof(beet_pageid_t), &p2,
		                 sizeof(beet_pageid_t));

		/*
		fprintf(stderr, "root goes from %u to %u\n", *root, mom->self);
		*/

		*root = mom->self;

		STOREROOT(root);
		UNLOCK();

		err = storeNode(tree, mom);
		if (err != BEET_OK) return err;

		err = releaseNode(tree, mom); free(mom);
		return err;
	}

	/* otherwise, we insert here:
	 * into the next node in the list,
	 * the splitter (key) and the nodeid
	 * of the new node */
	return insert(tree, root, nodes->cont, key, &p2, nodes->nxt);
}

/* ------------------------------------------------------------------------
 * Helper: node is a barrier (it has enough room for at least 1 more node)
 * ------------------------------------------------------------------------
 */
static inline int isBarrier(beet_tree_t *tree, 
                            beet_node_t *node) {
	return (node->size + 1 < tree->nsize);
}

/* ------------------------------------------------------------------------
 * Helper: unlock all nodes in opposite order of acquistion
 * ------------------------------------------------------------------------
 */
static inline beet_err_t unlockAll(beet_tree_t *tree, 
                               ts_algo_list_t *nodes) {
	beet_err_t err;
	ts_algo_list_node_t *runner, *tmp;
	beet_node_t *node;

	runner=nodes->head;
	while(runner != NULL) {
		node = runner->cont;
		err = releaseNode(tree, node); free(node);
		if (err != BEET_OK) return err;
		tmp = runner->nxt;
		ts_algo_list_remove(nodes, runner);
		free(runner); runner = tmp;
	}
	UNLOCK();
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: findNode where to add a (key,data) pair
 * ------------------------------------------------------------------------
 */
static beet_err_t findNode(beet_tree_t     *tree,
                           beet_node_t      *src,
                           beet_node_t     **trg,
                           char             mode,
                           const void       *key,
                           ts_algo_list_t *nodes) {
	beet_err_t    err;
	beet_pageid_t pge;

	if (src->leaf) {
		*trg = src; return BEET_OK;
	}
	if (mode == WRITE) {
		if (ts_algo_list_insert(nodes, src) != TS_ALGO_OK) {
			releaseNode(tree, src); free(src);
			return BEET_ERR_NOMEM;
		}
	}
	pge = beet_node_searchPageid(src, tree->ksize, key, tree->cmp);
	if (pge == BEET_PAGE_NULL) {
		releaseNode(tree, src); free(src);
		return BEET_ERR_PANIC;
	}
	err = getNode(tree, pge, mode, trg);
	if (err != BEET_OK) {
		releaseNode(tree, src); free(src);
		return err;
	}
	if (mode == WRITE && isBarrier(tree, *trg)) {
		err = unlockAll(tree, nodes);
		if (err != BEET_OK) {
			releaseNode(tree, src); free(src);
			return err;
		}
	} else if (mode == READ) {
		err = releaseNode(tree, src); free(src);
		if (err != BEET_OK) {
			releaseNode(tree, *trg);
			free(*trg);
			return err;
		}
	}
	return findNode(tree, *trg, trg, mode, key, nodes);
}

/* ------------------------------------------------------------------------
 * Insert data into the tree
 * TODO: check resource handling in case of error!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_insert(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            const void     *key,
                            const void    *data) {
	beet_err_t err;
	ts_algo_list_t nodes;
	beet_node_t *node, *leaf;

	TREENULL();
	if (root == NULL) return BEET_ERR_INVALID;
	if (key  == NULL) return BEET_ERR_INVALID;

	ts_algo_list_init(&nodes);

	LOCK();

	err = getNode(tree, *root, WRITE, &node);
	if (err != BEET_OK) {
		UNLOCK();
		return err;
	}

	err = findNode(tree, node, &leaf, WRITE, key, &nodes);	
	if (err != BEET_OK) {
		UNLOCK();
		return err;
	}

	err = insert(tree, root, leaf, key, data, nodes.head);
	if (err != BEET_OK) return err;
	
	err = releaseNode(tree, leaf); free(leaf);
	if (err != BEET_OK) return err;

	err = unlockAll(tree, &nodes);
	if (err != BEET_OK) return err;
	
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get the node that contains the given key
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_get(beet_tree_t  *tree,
                         beet_pageid_t root,
                         const void    *key,
                         beet_node_t **node) {
	beet_err_t err;
	beet_node_t *tmp;

	err = getNode(tree, root, READ, &tmp);
	if (err != BEET_OK) return err;

	err = findNode(tree, tmp, node, READ, key, NULL);
	if (err != BEET_OK) return err;

	return BEET_OK;
}

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
 * Release a node obtained by get, left or right
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_release(beet_tree_t *tree,
                             beet_node_t *node) {
	return releaseNode(tree, node);
}

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
