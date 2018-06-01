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
	uint32_t        lsize; /* leaf node size          */
	uint32_t        nsize; /* internal nodes size     */
	uint32_t        ksize; /* key  size               */
	uint32_t        dsize; /* data size               */
	beet_rider_t   *nolfs; /* rider for non-leafs     */
	beet_rider_t     *lfs; /* rider for leafs         */
	ts_algo_comprsc_t cmp; /* key compare callback    */
	beet_ins_t       *ins; /* data insertion callback */
	FILE            *roof; /* root file               */
	beet_lock_t     rlock; /* root file protection    */
} beet_tree_t;

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
                          beet_ins_t       *ins);

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
 * Insert data into the tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_tree_insert(beet_tree_t   *tree,
                            beet_pageid_t *root,
                            const void     *key,
                            const void    *data);

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
beet_err_t beet_tree_height(beet_tree_t  *tree,
                            beet_pageid_t root,
                            uint32_t       *h);
#endif

