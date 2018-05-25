/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * File Rider
 * ========================================================================
 * A cached File consisting of fix-sized pages
 * ========================================================================
 */
#ifndef beet_rider_decl
#define beet_rider_decl

#include <beet/types.h>
#include <beet/lock.h>
#include <beet/page.h>

#include <tsalgo/lru.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------------
 * "Smart" file access using a page cache
 * ------------------------------------------------------------------------
 */
typedef struct {
	beet_lock_t  lock; /* central lock            */
	ts_algo_lru_t lru; /* page cache              */
	char        *base; /* base path               */
	char        *name; /* file name               */
	FILE        *file; /* the file                */
	uint32_t   pagesz; /* size of one page        */
	uint32_t    lrusz; /* # of pages in the cache */
} beet_rider_t;

/* ------------------------------------------------------------------------
 * Initialise the rider
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_init(beet_rider_t    *rider,
                           char *base, char *name,
                           uint32_t        pagesz,
                           uint32_t        lrusz);

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
 * Release the page identified by 'pageid'
 * and obtained before for reading 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseRead(beet_rider_t *rider,
                                  beet_pageid_t pageid);

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseWrite(beet_rider_t *rider,
                                   beet_pageid_t pageid);

/* ------------------------------------------------------------------------
 * Release the page identified by 'pageid'
 * and obtained before for writing 
 * promising it has not been written
 * ------------------------------------------------------------------------
 */
beet_err_t beet_rider_releaseClean(beet_rider_t *rider,
                                   beet_pageid_t pageid);
#endif
