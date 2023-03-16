/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * B+Tree Abstraction
 * ========================================================================
 * ========================================================================
 */
#ifndef beet_tree_decl
#define beet_tree_decl

#include <beet/types.h>
#include <beet/lock.h>
#include <beet/rider.h>
#include <beet/node.h>
#include <beet/ins.h>

#include <tsalgo/types.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------------
 * B+Tree
 * ------------------------------------------------------------------------
 */
typedef struct beet_tree_st {
	uint32_t        lsize; /* leaf node size           */
	uint32_t        nsize; /* internal nodes size      */
	uint32_t        ksize; /* key  size                */
	uint32_t        dsize; /* data size                */
	beet_rider_t   *nolfs; /* rider for non-leaves     */
	beet_rider_t     *lfs; /* rider for leaves         */
	beet_compare_t    cmp; /* key compare callback     */
	beet_rscinit_t  rinit; /* rsc init callback        */
	beet_rscdest_t  rdest; /* rsc destruction callback */
	void             *rsc; /* user resources           */
	beet_ins_t       *ins; /* data insertion callback  */
	FILE            *roof; /* root file                */
	beet_lock_t     rlock; /* root file protection     */
} beet_tree_t;

/* ------------------------------------------------------------------------
 * Init B+Tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_init(beet_tree_t    *tree,
                          uint32_t       lsize,
                          uint32_t       nsize,
                          uint32_t       ksize,
                          uint32_t       dsize,
                          beet_rider_t  *nolfs,
                          beet_rider_t    *lfs,
                          FILE           *roof,
                          beet_compare_t   cmp,
                          beet_rscinit_t rinit,
                          beet_rscdest_t rdest,
                          void            *rsc,
                          beet_ins_t     *ins);

/* ------------------------------------------------------------------------
 * Destroy B+Tree
 * ------------------------------------------------------------------------
 */
void beet_tree_destroy(beet_tree_t *tree);

/* ------------------------------------------------------------------------
 * Create first node in the tree (if needed)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_makeRoot(beet_tree_t   *tree,
                              beet_pageid_t *root);

/* ------------------------------------------------------------------------
 * Insert data into the tree without updating data of existing keys
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_insert(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            const void     *key,
                            const void    *data);

/* ------------------------------------------------------------------------
 * Insert data into the tree updating if the key already exists
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_upsert(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            const void     *key,
                            const void    *data);

/* ------------------------------------------------------------------------
 * Hide a key in the tree without removing data physically
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_hide(beet_tree_t   *tree,
                          beet_pageid_t *root,
                          const void     *key);

/* ------------------------------------------------------------------------
 * Uncover a hidden key in the tree.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_unhide(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            const void     *key);

/* ------------------------------------------------------------------------
 * Get the node that contains the given key
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_get(beet_tree_t   *tree,
                         beet_pageid_t *root,
                         const void    *key,
                         beet_node_t **node);

/* ------------------------------------------------------------------------
 * Get the leftmost node
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_left(beet_tree_t   *tree,
                          beet_pageid_t *root,
                          beet_node_t  **node);

/* ------------------------------------------------------------------------
 * Get the rightmost node
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_right(beet_tree_t   *tree,
                           beet_pageid_t *root,
                           beet_node_t  **node);

/* ------------------------------------------------------------------------
 * Get next
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_next(beet_tree_t  *tree,
                          beet_node_t   *cur,
                          beet_node_t **next);

/* ------------------------------------------------------------------------
 * Get prev
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_prev(beet_tree_t  *tree,
                          beet_node_t   *cur,
                          beet_node_t **prev);

/* ------------------------------------------------------------------------
 * Release a node obtained by get, left, right, etc.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_release(beet_tree_t *tree,
                             beet_node_t *node);

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
beet_err_t beet_tree_height(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            uint32_t      *h);
#endif

