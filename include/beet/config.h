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
 * BEET Config
 * ========================================================================
 */
#ifndef beet_config_decl
#define beet_config_decl

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
 * Create config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_create(char *path, beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Read config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_get(char *path, beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Validate config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_validate(beet_config_t *cfg);

/* ------------------------------------------------------------------------
 * Initialise external library,
 * i.e. call dlopen
 * ------------------------------------------------------------------------
 */
void *beet_lib_init(char *lib);

/* ------------------------------------------------------------------------
 * Close external library,
 * i.e. call dlclose
 * ------------------------------------------------------------------------
 */
void beet_lib_close(void *handle);

/* ------------------------------------------------------------------------
 * Open Config (overrides Create Config)
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
 * Init open config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_open_config_init(beet_open_config_t *cfg);

/* ------------------------------------------------------------------------
 * Destroy open config
 * ------------------------------------------------------------------------
 */
void beet_open_config_destroy(beet_open_config_t *cfg);

/* ------------------------------------------------------------------------
 * Override config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_override(beet_config_t      *fcfg,
                                beet_open_config_t *ocfg);

/* ------------------------------------------------------------------------
 * Get compare callback
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_getCompare(beet_config_t      *fcfg,
                                  beet_open_config_t *ocfg,
                                  void               *handle,
                                  beet_compare_t     *cmp); 

/* ------------------------------------------------------------------------
 * Get Resource callbacks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_getRsc(beet_config_t      *fcfg,
                              beet_open_config_t *ocfg,
                              void               *handle,
                              beet_rscinit_t     *rscinit,
                              beet_rscdest_t     *rscdest); 
#endif

