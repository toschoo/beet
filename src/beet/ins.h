/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Data insert abstraction
 * ========================================================================
 * ========================================================================
 */
#ifndef beet_ins_decl
#define beet_ins_decl

#include <beet/types.h>
#include <tsalgo/types.h>

#include <stdlib.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Generic insert callback
 * -----------------------
 * Parameters: 1) resource ("closure")
 *             2) position in the tree
 *             3) data to add
 * -------------------------------------------------------------------------
 */
typedef beet_err_t (*beet_insert_t)(void*, void*, const void*); 

/* -------------------------------------------------------------------------
 * Generic insert cleanup callback
 * -------------------------------
 * Cleans up the insert closure.
 * The parameter is the closure stored in the tree.
 * -------------------------------------------------------------------------
 */
typedef void (*beet_ins_cleanup_t)(void*);

/* -------------------------------------------------------------------------
 * Generic insert
 * --------------
 * It stores
 * - the resource to be passed to the insert callback,
 * - the insert callback itself and
 * - the destroy function called,
 * when the tree is destroyed.
 * -------------------------------------------------------------------------
 */
typedef struct {
	void                  *rsc;
	beet_insert_t     inserter;
	beet_ins_cleanup_t cleaner;
} beet_ins_t;

#endif
