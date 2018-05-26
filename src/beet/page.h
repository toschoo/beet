/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Page
 * ========================================================================
 * A page is the unit of I/O
 * ========================================================================
 */
#ifndef beet_page_decl
#define beet_page_decl

#include <beet/types.h>
#include <beet/lock.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t beet_pageid_t;

#define BEET_PAGE_NULL 0xffffffff
#define BEET_PAGE_FILE 0x80000000

/* ------------------------------------------------------------------------
 * Memory representation of a page
 * ------------------------------------------------------------------------
 */
typedef struct {
	char           *data; /* binary data associated with this page   */
	beet_lock_t     lock; /* read/write lock to work on this page    */
	beet_pageid_t pageid; /* file position where this page is stored */
	uint32_t          sz; /* size of the page in byte                */
} beet_page_t;

/* ------------------------------------------------------------------------
 * Allocate a new page in a file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_alloc(beet_page_t **page, FILE *store, uint32_t sz);

/* ------------------------------------------------------------------------
 * Initialise an already allocated page in a file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_init(beet_page_t *page, uint32_t sz);

/* ------------------------------------------------------------------------
 * Destroy page
 * ------------------------------------------------------------------------
 */
void beet_page_destroy(beet_page_t *page);

/* ------------------------------------------------------------------------
 * Load page from store into memory
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_load(beet_page_t *page, FILE *store);

/* ------------------------------------------------------------------------
 * Write page in memory to store
 * ------------------------------------------------------------------------
 */
beet_err_t beet_page_store(beet_page_t *page, FILE *store);

#endif

