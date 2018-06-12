/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * 
 * This file is part of the BEET Library.
 *
 * The BEET Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * The BEET Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the BEET Library; if not, see
 * <http://www.gnu.org/licenses/>.
 *  
 * ========================================================================
 * BEET API
 * TODO:
 * - delete
 * - map, fold?
 * ========================================================================
 */
#ifndef beet_index_decl
#define beet_index_decl

#include <beet/types.h>
#include <beet/config.h>
#include <beet/iter.h>

/* ------------------------------------------------------------------------
 * The BEET Index
 * ------------------------------------------------------------------------
 */
typedef struct beet_index_t *beet_index_t;

/* ------------------------------------------------------------------------
 * Create an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_create(char *path,
                             char standalone,
                             beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Remove an index physically from disk
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_drop(char *path);

/* ------------------------------------------------------------------------
 * Open an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_open(char *path, void *handle,
                           beet_open_config_t  *cfg,
                           beet_index_t       *idx);

/* ------------------------------------------------------------------------
 * Close an index
 * ------------------------------------------------------------------------
 */
void beet_index_close(beet_index_t idx);

/* ------------------------------------------------------------------------
 * Get index type
 * ------------------------------------------------------------------------
 */
int beet_index_type(beet_index_t idx); 

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index without updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_insert(beet_index_t idx, void *key, void *data);

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_upsert(beet_index_t idx, void *key, void *data);

/* ------------------------------------------------------------------------
 * Removes a key and all its data from the index
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_deleteKey(beet_index_t idx, void *key);

/* ------------------------------------------------------------------------
 * Removes data from a subtree and, if all data were removed
 * from that subtree, also removes the key from the main tree.
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_delete(beet_index_t  idx,
                             void        *key1,
                             void        *key2);

/* ------------------------------------------------------------------------
 * Compute the height of the tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_height(beet_index_t idx, uint32_t *h);

/* ------------------------------------------------------------------------
 * Get data by key (simple)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_copy(beet_index_t idx, const void *key, void *data);

/* ------------------------------------------------------------------------
 * State
 * ------------------------------------------------------------------------
 */
typedef struct beet_state_t *beet_state_t;

/* ------------------------------------------------------------------------
 * Allocate new state
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_alloc(beet_index_t idx, beet_state_t *state);

/* ------------------------------------------------------------------------
 * Destroy and free state
 * ------------------------------------------------------------------------
 */
void beet_state_destroy(beet_state_t state);

/* ------------------------------------------------------------------------
 * Clean state for reuse
 * ------------------------------------------------------------------------
 */
void beet_state_reinit(beet_state_t state);

/* ------------------------------------------------------------------------
 * Release all locks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_release(beet_state_t state);

/* ------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------
 */
#define BEET_FLAGS_RELEASE 2
#define BEET_FLAGS_ROOT    4
#define BEET_FLAGS_SUBTREE 8

/* ------------------------------------------------------------------------
 * Get data with state and finer control
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_get(beet_index_t  idx,
                          beet_state_t  state,
                          uint16_t      flags,
                          const void   *key,
                          void        **data);

/* ------------------------------------------------------------------------
 * Mere existence test
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_doesExist(beet_index_t  idx,
                                const void   *key);

/* ------------------------------------------------------------------------
 * Get data for subtree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_get2(beet_index_t  idx,
                           beet_state_t  state,
                           uint16_t      flags,
                           const void   *key1,
                           const void   *key2,
                           void        **data);

/* ------------------------------------------------------------------------
 * Existence test for embedded tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_doesExist2(beet_index_t   idx,
                                 const void   *key1,
                                 const void   *key2);

/* ------------------------------------------------------------------------
 * Get iterator into subtree for key
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_getIter(beet_index_t  idx,
                              beet_state_t  state,
                              const void   *key,
                              beet_iter_t  iter);

/* ------------------------------------------------------------------------
 * Allocate reusable iter
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_alloc(beet_index_t idx,
                           beet_iter_t *iter);

/* ------------------------------------------------------------------------
 * range scan
 * ------------------------------------------------------------------------
 */
typedef struct {
	void *fromkey;
	void *tokey;
} beet_range_t;

/* ------------------------------------------------------------------------
 * range scan
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_range(beet_index_t  idx,
                            beet_range_t *range,
                            beet_dir_t    dir,
                            beet_iter_t   iter);
#endif
