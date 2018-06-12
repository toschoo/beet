/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * File Rider
 * ========================================================================
 * A cached File consisting of fix-sized pages
 * TODO:
 * - for delete, we will need a free-list
 *   which contains empty nodes. Instead of always allocating a new page,
 *   when a new node is needed, we will first consult free-list. 
 * - for compression, we will need one more layer: storage blocks.
 *   Those blocks would contain compressed pages (or parts thereof).
 *   As part of its definition, a page would contain a reference to the
 *   block that stores the first part of the page.
 *   The issue is that, with each change to a node,
 *   the compression size changes. We therefore need to be flexible
 *   about where to put the page data.
 * ========================================================================
 */
#ifndef beet_rider_decl
#define beet_rider_decl

#include <beet/types.h>
#include <beet/lock.h>
#include <beet/page.h>

#include <tsalgo/list.h>
#include <tsalgo/tree.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------------
 * "Smart" file access using a page cache
 * ------------------------------------------------------------------------
 */
typedef struct {
	beet_latch_t   latch; /* in-memory latch           */
	ts_algo_tree_t *tree; /* page cache                */
	ts_algo_list_t queue; /* page cache                */
	char           *base; /* base path                 */
	char           *name; /* file name                 */
	FILE           *file; /* the file                  */
	off_t            fsz; /* current file size         */
	uint32_t      pagesz; /* size of one page          */
	uint32_t          sz; /* # of pages in the cache   */
	uint32_t         max; /* max of pages in the cache */
} beet_rider_t;

/* ------------------------------------------------------------------------
 * Initialise the rider
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_init(beet_rider_t    *rider,
                           char *base, char *name,
                           uint32_t        pagesz,
                           uint32_t          max);

/* ------------------------------------------------------------------------
 * Destroy the rider
 * ------------------------------------------------------------------------
 */
void beet_rider_destroy(beet_rider_t *rider);

/* ------------------------------------------------------------------------
 * Get the page identified by 'pageid' for reading
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_getRead(beet_rider_t  *rider,
                              beet_pageid_t pageid,
                              beet_page_t  **page);

/* ------------------------------------------------------------------------
 * Get the page identified by 'pageid' for writing
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_getWrite(beet_rider_t *rider,
                               beet_pageid_t pageid,
                               beet_page_t **page);

/* ------------------------------------------------------------------------
 * Allocate a new page
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_alloc(beet_rider_t  *rider,
                            beet_page_t  **page);

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for reading 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseRead(beet_rider_t *rider,
                                  beet_page_t  *page);

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseWrite(beet_rider_t *rider,
                                   beet_page_t  *page);

/* ------------------------------------------------------------------------
 * Store page to disk 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_store(beet_rider_t *rider,
                            beet_page_t  *page);
#endif
