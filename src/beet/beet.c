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
#include <beet/rider.h>
#include <beet/node.h>
#include <beet/tree.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAGIC 0x8ee7
#define VERSION 1

/* ------------------------------------------------------------------------
 * index
 * ------------------------------------------------------------------------
 */
struct beet_index_t {
	beet_tree_t   *tree;
	beet_pageid_t *root;
	struct beet_index_t *subidx;
};

/* ------------------------------------------------------------------------
 * Helper: check config
 * ------------------------------------------------------------------------
 */
static inline beet_err_t checkcfg(beet_config_t *cfg) {
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: check whether path exists; if not: create
 * ------------------------------------------------------------------------
 */
static inline beet_err_t mkpath(char *path) {
	struct stat st;

	if (stat(path, &st) == 0) return BEET_OK;
	if (mkdir(path, S_IRWXU) != 0) return BEET_OSERR_MKDIR;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: write configuration
 * ------------------------------------------------------------------------
 */
static inline beet_err_t writecfg(FILE *f, beet_config_t *cfg) {
	uint32_t m;

	m = MAGIC; m<<=16; m+=VERSION;
	if (fwrite(&m, 4, 1, f) != 1) return BEET_OSERR_WRITE;

	if (fwrite(&cfg->indexType, 4, 1, f) != 1) return BEET_OSERR_WRITE;

	if (fwrite(&cfg->leafPageSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;
	if (fwrite(&cfg->intPageSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;
	if (fwrite(&cfg->leafNodeSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;
	if (fwrite(&cfg->intNodeSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;

	if (fwrite(&cfg->keySize, 4, 1, f) != 1) return BEET_OSERR_WRITE;
	if (fwrite(&cfg->dataSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;

	if (fwrite(&cfg->leafCacheSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;
	if (fwrite(&cfg->intCacheSize, 4, 1, f) != 1) return BEET_OSERR_WRITE;

	if (cfg->subPath == NULL) {
		m = 0;
		if (fwrite(&m, 1, 1, f) != 1) return BEET_OSERR_WRITE;
	} else {
		if (fwrite(cfg->subPath, strlen(cfg->subPath)+1, 1, f) != 1) {
			return BEET_OSERR_WRITE;
		}
	}
	if (cfg->compare == NULL) {
		m = 0;
		if (fwrite(&m, 1, 1, f) != 1) return BEET_OSERR_WRITE;
	} else {
		if (fwrite(cfg->compare, strlen(cfg->compare)+1, 1, f) != 1) {
			return BEET_OSERR_WRITE;
		}
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: check version
 * ------------------------------------------------------------------------
 */
static inline beet_err_t chkver(uint32_t v) {
	switch(v) {
	case 1: return BEET_OK;
	case 0: return BEET_ERR_NOVER;
	default: return BEET_ERR_UNKNVER;
	}
}

/* ------------------------------------------------------------------------
 * Helper: read configuration
 * ------------------------------------------------------------------------
 */
static inline beet_err_t readcfg(FILE *f, size_t sz, beet_config_t *cfg) {
	beet_err_t err;
	char  *buf;
	int k, s, i=0;
	uint32_t m;
	uint32_t v;

	if (fread(&m, 4, 1, f) != 1) return BEET_OSERR_READ;
	v = (m << 16); v >>= 16;
	if ((m>>16) != MAGIC) return BEET_ERR_NOMAGIC; 
	err = chkver(v);
	if (err != BEET_OK) return err;

	i+=4;

	if (fread(&cfg->indexType, 4, 1, f) != 1) return BEET_OSERR_READ;

	i+=4;

	if (fread(&cfg->leafPageSize, 4, 1, f) != 1) return BEET_OSERR_READ;
	if (fread(&cfg->intPageSize, 4, 1, f) != 1) return BEET_OSERR_READ;
	if (fread(&cfg->leafNodeSize, 4, 1, f) != 1) return BEET_OSERR_READ;
	if (fread(&cfg->intNodeSize, 4, 1, f) != 1) return BEET_OSERR_READ;

	i+=16;

	if (fread(&cfg->keySize, 4, 1, f) != 1) return BEET_OSERR_READ;
	if (fread(&cfg->dataSize, 4, 1, f) != 1) return BEET_OSERR_READ;

	i+=8;

	if (fread(&cfg->leafCacheSize, 4, 1, f) != 1) return BEET_OSERR_READ;
	if (fread(&cfg->intCacheSize, 4, 1, f) != 1) return BEET_OSERR_READ;

	i+=8;

	s = sz - i;

	buf = malloc(s);
	if (buf == NULL) return BEET_ERR_NOMEM;

	if (fread(buf, s, 1, f) != 1) return BEET_OSERR_READ;

	for(i=0;buf[i] != 0 && i < s; i++) {}
	if (i >= s) {
		free(buf);
		return BEET_ERR_BADCFG;
	}
	if (i == 0) cfg->subPath = NULL;
	else {
		cfg->subPath = malloc(i+1);
		if (cfg->subPath == NULL) {
			free(buf);
			return BEET_ERR_NOMEM;
		}
		strcpy(cfg->subPath, buf);
	}
	i++; k=i;
	for(;buf[i] != 0 && i < s; i++) {}
	if (i > s) {
		free(buf);
		free(cfg->subPath);
		cfg->subPath = NULL;
		return BEET_ERR_BADCFG;
	}
	if (i==k) cfg->compare = NULL;
	else {
		cfg->compare = malloc(i-k+1);
		if (cfg->compare == NULL) {
			free(buf);
			free(cfg->subPath);
			cfg->subPath = NULL;
			return BEET_ERR_NOMEM;
		}
		strcpy(cfg->compare, buf+k);
	}
	free(buf);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: make configuration file
 * ------------------------------------------------------------------------
 */
static inline beet_err_t mkcfg(char *path, beet_config_t *cfg) {
	beet_err_t err = BEET_OK;
	FILE *f;
	size_t s;
	char *p;

	s = strlen(path) + 7;
	p = malloc(s+1);

	if (p == NULL) return BEET_ERR_NOMEM;
	
	sprintf(p, "%s/config", path);

	f = fopen(p, "wb"); free(p);
	if (f == NULL) return BEET_OSERR_OPEN;

	err = writecfg(f, cfg);
	if (err != BEET_OK) {
		fclose(f); return err;
	}
	if (fclose(f) != 0) return BEET_OSERR_CLOSE;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: make leaf/noleaf
 * ------------------------------------------------------------------------
 */
static inline beet_err_t mkempty(char *path, char *name) {
	FILE *f;
	size_t s;
	char *p;

	s = strlen(path) + strlen(name) + 1;
	p = malloc(s+1);

	if (p == NULL) return BEET_ERR_NOMEM;
	
	sprintf(p, "%s/%s", path, name);

	f = fopen(p, "wb"); free(p);
	if (f == NULL) return BEET_OSERR_OPEN;

	if (fclose(f) != 0) return BEET_OSERR_CLOSE;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: make roof
 * ------------------------------------------------------------------------
 */
static inline beet_err_t mkroof(char *path) {
	beet_pageid_t root = 0;
	FILE *f;
	size_t s;
	char *p;

	s = strlen(path) + 5;
	p = malloc(s+1);

	if (p == NULL) return BEET_ERR_NOMEM;
	
	sprintf(p, "%s/roof", path);

	f = fopen(p, "wb"); free(p);
	if (f == NULL) return BEET_OSERR_OPEN;

	if (fwrite(&root, sizeof(beet_pageid_t), 1, f) != 1) {
		fclose(f);
		return BEET_OSERR_WRITE;
	}

	if (fclose(f) != 0) return BEET_OSERR_CLOSE;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Create an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_create(char         *path,
                             char    standalone,
                             beet_config_t *cfg) {
	beet_err_t err;

	if (path == NULL) return BEET_ERR_INVALID;
	if (cfg  == NULL) return BEET_ERR_INVALID;

	if (strnlen(path, 4097) > 4096) return BEET_ERR_TOOBIG;
	
	err = checkcfg(cfg);
	if (err != BEET_OK) return err;

	err = mkpath(path);
	if (err != BEET_OK) return err;

	err = mkcfg(path, cfg);
	if (err != BEET_OK) return err;

	err = mkempty(path, "leaf");
	if (err != BEET_OK) return err;

	err = mkempty(path, "nonleaf");
	if (err != BEET_OK) return err;

	if (standalone) {
		err = mkroof(path);
		if (err != BEET_OK) return err;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Remove an index physically from disk
 * ------------------------------------------------------------------------
 */
#define REMOVE(p,f,s) \
	p = malloc(s+strlen(f)+2); \
	if (p == NULL) return BEET_ERR_NOMEM; \
	sprintf(p, "%s/%s", path, f); \
	if (stat(p, &st) == 0) { \
		if (remove(p) != 0) { \
			free(p); \
			return BEET_OSERR_REMOV; \
		} \
	} \
	free(p);

beet_err_t beet_index_drop(char *path) {
	struct stat st;
	char *p;
	size_t s;

	s = strnlen(path, 4097);
	if (s > 4096) return BEET_ERR_TOOBIG;

	if (stat(path, &st) != 0) return BEET_ERR_NOFILE;

	REMOVE(p, "leaf", s);
	REMOVE(p, "nonleaf", s);
	REMOVE(p, "config", s);
	REMOVE(p, "rook", s);

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Open an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_open(char *path, void *handle,
                           beet_open_config_t  *cfg,
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
