/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Data insert abstraction
 * ========================================================================
 * Inserting data into a B+tree depends on the structure of the tree.
 * We may:
 * - insert nothing
 * - insert plain data to the node ('primary key')
 * - insert data to an embedded tree
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
 *             2) data size
 *             3) position in the tree
 *             4) data to add
 * -------------------------------------------------------------------------
 */
typedef beet_err_t (*beet_insert_t)(void*, uint32_t sz, void*, const void*); 

/* -------------------------------------------------------------------------
 * Generic insert cleanup callback
 * -------------------------------
 * Cleans up the insert closure.
 * The parameter is the closure stored in the tree.
 * -------------------------------------------------------------------------
 */
typedef void (*beet_ins_cleanup_t)(void*);

/* -------------------------------------------------------------------------
 * Generic node initialiser callback
 * ---------------------------------
 * Initilises the data segment of nodes according to
 * the requirements of the specific insert closure. 
 * Parameters: 1) resource ("closure")
 *             2) The number of of kids in that segment
 *             3) The kids segment of the node
 * -------------------------------------------------------------------------
 */
typedef void (*beet_ins_nodeinit_t)(void*, uint32_t, void*);

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
	beet_ins_nodeinit_t  ninit;
} beet_ins_t;

/* -------------------------------------------------------------------------
 * Plain inserter
 * -------------------------------------------------------------------------
 */
beet_err_t beet_ins_plain(void *ignore, uint32_t sz, void* trg,
                                             const void *data);
void beet_ins_plainclean(void *ignore);
void beet_ins_plaininit(void *ignore, uint32_t n, void *kids);
beet_err_t beet_ins_setPlain(beet_ins_t *ins);

/* -------------------------------------------------------------------------
 * Embedded inserter
 * -------------------------------------------------------------------------
 */
typedef struct {
	void *key;
	void *data;
} beet_ins_pair_t;

beet_err_t beet_ins_embedded(void *tree, uint32_t sz, void* root,
                                               const void *data);
void beet_ins_embeddedclean(void *ins);
void beet_ins_embeddedinit(void *ignore, uint32_t n, void *kids);
beet_err_t beet_ins_setEmbedded(beet_ins_t *ins, void *subtree);

#endif
