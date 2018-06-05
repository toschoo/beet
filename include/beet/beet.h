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
#ifndef beet_decl
#define beet_decl

#include <beet/types.h>
#include <stdint.h>

/* ------------------------------------------------------------------------
 * Config
 * ------------------------------------------------------------------------
 */
typedef struct {
	uint8_t  indexType;     /* index type (see below)           */
	uint32_t leafPageSize;  /* page size of leaf nodes          */
	uint32_t intPageSize;   /* page size of internal nodes      */
	uint32_t leafNodeSize;  /* number of keys in leaf nodes     */
	uint32_t intNodeSize;   /* number of keys in internal nodes */
	uint32_t keySize;       /* size of one key                  */
	uint32_t dataSize;      /* data size                        */
	char    *subPath;       /* path to the embedded index       */
	int32_t  leafCacheSize; /* cache size for leaf nodes        */
	int32_t  intCacheSize;  /* cache size for internal nodes    */
	char    *compare;       /* name of compare function         */
	char    *rscinit;       /* name of rsc init function        */
	char    *rscdest;       /* name of rsc destroyer function   */
} beet_config_t;

/* ------------------------------------------------------------------------
 * Init config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_config_init(beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Index Types:
 * - NULL  has no data (keys only)
 * - PLAIN has data stored within the tree (primary key)
 * - HOST  has an embedded B+Tree as data (which itself may be
 *                                         NULL or PLAIN -
 *                                         or even HOST)
 * ------------------------------------------------------------------------
 */
#define BEET_INDEX_NULL  1
#define BEET_INDEX_PLAIN 2
#define BEET_INDEX_HOST  3

/* ------------------------------------------------------------------------
 * Cache Size
 * ------------------------------------------------------------------------
 */
#define BEET_CACHE_UNLIMITED 0
#define BEET_CACHE_DEFAULT  -1
#define BEET_CACHE_IGNORE   -2

/* ------------------------------------------------------------------------
 * Destroy config
 * ------------------------------------------------------------------------
 */
void beet_config_destroy(beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Light Config
 * ------------------------------------------------------------------------
 */
typedef struct {
	int32_t  leafCacheSize; /* cache size for leaf nodes         */
	int32_t   intCacheSize; /* cache size for internal nodes     */
	beet_compare_t compare; /* pointer to compare function       */
	beet_rscinit_t rscinit; /* pointer to rsc init function      */
	beet_rscinit_t rscdest; /* pointer to rsc destroyer function */
} beet_open_config_t;

/* ------------------------------------------------------------------------
 * Init config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_open_config_init(beet_open_config_t *cfg);

/* ------------------------------------------------------------------------
 * Destroy open config
 * ------------------------------------------------------------------------
 */
void beet_open_config_destroy(beet_open_config_t *cfg);

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
 * Get data with state and finer control
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_get(beet_index_t  idx,
                          beet_state_t  state,
                          uint16_t      flags,
                          const void   *key,
                          void        **data);

/* ------------------------------------------------------------------------
 * Flags
 * ------------------------------------------------------------------------
 */
#define BEET_FLAGS_RELEASE 2
#define BEET_FLAGS_ROOT    4
#define BEET_FLAGS_SUBTREE 8

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
typedef struct {
	void *fromkey;
	void *tokey;
} beet_range_t;

/* ------------------------------------------------------------------------
 * iterator
 * ------------------------------------------------------------------------
 */
typedef struct beet_iter_t *beet_iter_t;

/* ------------------------------------------------------------------------
 * range scan
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_range(beet_index_t  idx,
                            beet_state_t  state,
                            uint16_t      flags,
                            beet_range_t *range,
                            beet_iter_t   iter);
#endif
