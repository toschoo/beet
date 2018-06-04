/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * B+Node Abstraction
 * ========================================================================
 * ========================================================================
 */
#ifndef beet_node_decl
#define beet_node_decl

#include <beet/types.h>
#include <beet/page.h>
#include <beet/rider.h>
#include <beet/ins.h>

#include <tsalgo/types.h>

#include <stdlib.h>
#include <stdint.h>

/* ------------------------------------------------------------------------
 * Node in a b+tree
 * ------------------------------------------------------------------------
 */
typedef struct {
	beet_pageid_t self; /* reference to this node     */
	beet_pageid_t next; /* next node (leaf only)      */
	beet_pageid_t prev; /* previous node (leaf only)  */
	uint32_t      size; /* number of keys in the node */
	char         *keys; /* array of keys              */
	char         *kids; /* array of pointers          */
	beet_page_t  *page; /* the page data              */
	char          mode; /* reading or writing         */
	char          leaf; /* is leaf node               */
} beet_node_t;

/* ------------------------------------------------------------------------
 * Initialise the node from a page
 * ------------------------------------------------------------------------
 */
void beet_node_init(beet_node_t *node,
                    beet_page_t *page,
                    uint32_t   nodesz,
                    uint32_t    keysz,
                    char        leaf);

/* ------------------------------------------------------------------------
 * Serialise the node to its page
 * ------------------------------------------------------------------------
 */
void beet_node_serialise(beet_node_t *node);

/* ------------------------------------------------------------------------
 * Add (key,data) to node
 * The new entry must still fit into the node,
 * otherwise, the function returns with error.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_node_add(beet_node_t     *node,
                         uint32_t        nsize,
                         uint32_t        ksize,
                         uint32_t        dsize,
                         const void       *key,
                         const void      *data,
                         beet_compare_t    cmp,
                         beet_ins_t       *ins,
                         char              upd,
                         char           *wrote);

/* ------------------------------------------------------------------------
 * Get the key at 'slot'
 * ------------------------------------------------------------------------
 */
void *beet_node_getKey(beet_node_t *node,
                       uint32_t     slot,
                       uint32_t   keysz);

/* ------------------------------------------------------------------------
 * Get the data at 'slot'
 * ------------------------------------------------------------------------
 */
void *beet_node_getData(beet_node_t *node,
                        uint32_t     slot,
                        uint32_t  datasz);

/* ------------------------------------------------------------------------
 * Get pageid at slot (internal node only!)
 * ------------------------------------------------------------------------
 */
beet_pageid_t beet_node_getPageid(beet_node_t *node,
                                  uint32_t     slot);

/* ------------------------------------------------------------------------
 * Search link to follow
 * ------------------------------------------------------------------------
 */
beet_pageid_t beet_node_searchPageid(beet_node_t *node,
                                     uint32_t    keysz,
                                      const void  *key, 
                                   beet_compare_t cmp);

/* ------------------------------------------------------------------------
 * Generic search
 * ------------------------------------------------------------------------
 */
int32_t beet_node_search(beet_node_t  *node,
                         uint32_t     keysz,
                         const void    *key,
                         beet_compare_t cmp);

/* ------------------------------------------------------------------------
 * Test that key at slot is equal to 'key'
 * ------------------------------------------------------------------------
 */
char beet_node_equal(beet_node_t *node,
                     uint32_t     slot,
                     uint32_t    keysz,
                     const void   *key,
                   beet_compare_t cmp);
#endif
