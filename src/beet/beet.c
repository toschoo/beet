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

#include <beet/beet.h>

/* ------------------------------------------------------------------------
 * Create an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_create(char *path, beet_config_t *cfg);

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
                                 beet_compare_t cmp,
                                 beet_index_t  *idx);

/* ------------------------------------------------------------------------
 * Close an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_close(beet_index_t idx);

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
 * Get data by key (simple)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_copy(beet_index_t idx, const void *key, void *data);

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
 * Release all locks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_release(beet_state_t state);

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
 * Removes a key and all its data from the index or its subtree
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_delete(beet_index_t  idx,
                             beet_state_t  state,
                             uint16_t      flags,
                             void         *key);

/* ------------------------------------------------------------------------
 * range scan
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_range(beet_index_t    idx,
                            beet_state_t  state,
                            uint16_t      flags,
                            beet_range_t *range,
                            beet_iter_t   iter);
