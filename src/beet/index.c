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
#include <beet/rider.h>
#include <beet/node.h>
#include <beet/tree.h>
#include <beet/iterimp.h>
#include <beet/iter.h>
#include <beet/config.h>
#include <beet/index.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define LEAF "leaf"
#define INTERN "nonleaf"

/* ------------------------------------------------------------------------
 * index
 * ------------------------------------------------------------------------
 */
struct beet_index_t {
	beet_tree_t   *tree;
	beet_pageid_t  root;
	FILE          *roof;
	char     standalone;
	beet_index_t subidx;
};

/* ------------------------------------------------------------------------
 * state
 * ------------------------------------------------------------------------
 */
struct beet_state_t {
	beet_tree_t   *tree[2];
	beet_pageid_t *root;
	beet_node_t   *node[2];
};

/* ------------------------------------------------------------------------
 * Macro: index not NULL
 * ------------------------------------------------------------------------
 */
#define IDXNULL() \
	if (idx == NULL) return BEET_ERR_NOIDX;

/* ------------------------------------------------------------------------
 * Macro: state not NULL
 * ------------------------------------------------------------------------
 */
#define STATENULL() \
	if (state == NULL) return BEET_ERR_NOSTAT;

/* ------------------------------------------------------------------------
 * Macro: clean state
 * ------------------------------------------------------------------------
 */
#define CLEANSTATE(x) \
	memset(x, 0, sizeof(struct beet_state_t));

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
	beet_pageid_t root = BEET_PAGE_LEAF;
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
 * Helper: make root
 * ------------------------------------------------------------------------
 */
static inline beet_err_t mkroot(beet_index_t idx,
                                char       *path) {
	beet_err_t err;
	struct stat st;
	char *p;

	/* we could reuse this path */
	p = malloc(strlen(path) + strlen(LEAF) + 2);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/%s", path, LEAF);

	if (stat(p, &st) != 0) {
		free(p); return BEET_ERR_NOFILE;
	}
	free(p);

	if (st.st_size > 0) return BEET_OK;

	err = beet_tree_makeRoot(idx->tree, &idx->root);
	if (err != BEET_OK) return err;

	rewind(idx->roof);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get roof
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getroof(beet_index_t idx, char *path) {
	char *p;

	p = malloc(strlen(path) + 6);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/roof", path);

	idx->roof = fopen(p, "rb+"); free(p);
	if (idx->roof == NULL) return BEET_OSERR_OPEN;
	
	if (fread(&idx->root, sizeof(beet_pageid_t), 1, idx->roof) != 1) {
		fclose(idx->roof); idx->roof = NULL;
		return BEET_OSERR_READ;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get leaf rider
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getLeafRider(beet_index_t   sidx,
                                      char          *path,
                                      beet_config_t *cfg,
                                      beet_rider_t  **rider) {
	beet_err_t err;
	*rider = calloc(1,sizeof(beet_rider_t));
	if (*rider == NULL) return BEET_ERR_NOMEM;
	err = beet_rider_init(*rider, path, LEAF,
	 cfg->leafPageSize, cfg->leafCacheSize); 
	if (err != BEET_OK) {
		free(*rider); *rider = NULL; return err;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get inernal rider
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getIntRider(beet_index_t   sidx,
                                     char          *path,
                                     beet_config_t *cfg,
                                     beet_rider_t  **rider) {
	beet_err_t err;
	*rider = calloc(1,sizeof(beet_rider_t));
	if (*rider == NULL) return BEET_ERR_NOMEM;
	err = beet_rider_init(*rider, path, INTERN,
	     cfg->intPageSize, cfg->intCacheSize); 
	if (err != BEET_OK) {
		free(*rider); *rider = NULL; return err;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get inserter
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getins(beet_index_t   idx,
                                beet_config_t *cfg,
                                beet_ins_t   **ins) {
	switch(cfg->indexType) {
	case BEET_INDEX_NULL: *ins = NULL; return BEET_OK;
	case BEET_INDEX_PLAIN:
	case BEET_INDEX_HOST:
		*ins = calloc(1, sizeof(beet_ins_t));
		if (*ins == NULL) return BEET_ERR_NOMEM;
		if (cfg->indexType == BEET_INDEX_PLAIN) beet_ins_setPlain(*ins);
		else beet_ins_setEmbedded(*ins, idx->subidx->tree);
		return BEET_OK;
	default:
		*ins = NULL; return BEET_ERR_UNKNTYP;
	}
}

/* ------------------------------------------------------------------------
 * Create an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_create(char *base,
                             char *path,
                             char  standalone,
                             beet_config_t *cfg) {
	beet_err_t err;
	char *p;
	size_t s;

	if (base == NULL) return BEET_ERR_INVALID;
	if (path == NULL) return BEET_ERR_INVALID;
	if (cfg  == NULL) return BEET_ERR_INVALID;

	s = strnlen(base, 4097);
	s += strnlen(path, 4097);
	if (s > 4096) return BEET_ERR_TOOBIG;
	
	err = beet_config_validate(cfg);
	if (err != BEET_OK) return err;

	p = malloc(s + 2);
	sprintf(p, "%s/%s", base, path); 

	err = mkpath(p);
	if (err != BEET_OK) {
		free(p); return err;
	}

	err = beet_config_create(p, cfg);
	if (err != BEET_OK) {
		free(p); return err;
	}

	err = mkempty(p, LEAF);
	if (err != BEET_OK) {
		free(p); return err;
	}

	err = mkempty(p, INTERN);
	if (err != BEET_OK) {
		free(p); return err;
	}

	if (standalone) {
		err = mkroof(p);
		if (err != BEET_OK) {
			free(p); return err;
		}
	}
	free(p);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Remove an index physically from disk
 * ------------------------------------------------------------------------
 */
#define REMOVE(p,ip,f,s) \
	ip = malloc(s+strlen(f)+3); \
	if (ip == NULL) return BEET_ERR_NOMEM; \
	sprintf(ip, "%s/%s", p, f); \
	if (stat(ip, &st) == 0) { \
		if (remove(ip) != 0) { \
			free(ip); free(p); \
			return BEET_OSERR_REMOV; \
		} \
	} \
	free(ip);

beet_err_t beet_index_drop(char *base, char *path) {
	struct stat st;
	char *ip;
	char *p;
	size_t s;

	s = strnlen(base, 4097);
	s += strnlen(path, 4097);
	if (s > 4096) return BEET_ERR_TOOBIG;

	p = malloc(s + 2);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/%s", base, path);

	if (stat(p, &st) != 0) return BEET_ERR_NOFILE;

	REMOVE(p, ip, LEAF, s);
	REMOVE(p, ip, INTERN, s);
	REMOVE(p, ip, "config", s);
	REMOVE(p, ip, "roof", s);

	free(p);

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: open an index
 * ------------------------------------------------------------------------
 */
static beet_err_t openIndex(char *base, char   *path,
                            void             *handle,
                            beet_open_config_t *ocfg,
                            char          standalone,
                            beet_index_t       *idx) {
	beet_index_t sidx;
	beet_rider_t *lfs, *nolfs;
	beet_config_t fcfg;
	beet_ins_t    *ins;
	beet_compare_t cmp=NULL;
	beet_rscinit_t rinit=NULL;
	beet_rscdest_t rdst=NULL;
	beet_err_t     err;
	size_t s;
	char *p;

	if (idx == NULL) return BEET_ERR_NOIDX;
	*idx = NULL;

	if (base == NULL) return BEET_ERR_INVALID;
	if (path == NULL) return BEET_ERR_INVALID;
	if (handle == NULL && ocfg == NULL) return BEET_ERR_INVALID;

	s = strnlen(base, 4097);
	s += strnlen(path, 4097);
	if (s > 4096) return BEET_ERR_TOOBIG;

	memset(&fcfg, 0, sizeof(beet_config_t));

	p = malloc(s + 2);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/%s", base, path);

	err = beet_config_get(p, &fcfg);
	if (err != BEET_OK) {
		free(p); return err;
	}
	
	err = beet_config_override(&fcfg, ocfg);
	if (err != BEET_OK) {
		free(p); return err;
	}

	sidx = calloc(1,sizeof(struct beet_index_t));
	if (sidx == NULL) {
		beet_config_destroy(&fcfg); free(p);
		return BEET_ERR_NOMEM;
	}

	sidx->standalone = standalone;

	/* open roof and set root */
	if (standalone) {
		err = getroof(sidx, p);
		if (err != BEET_OK) {
			beet_config_destroy(&fcfg);
			beet_index_close(sidx); free(p);
			return err;
		}
	}

	/* init leaf rider */
	err = getLeafRider(sidx, p, &fcfg, &lfs);
	if (err != BEET_OK) {
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return err;
	}

	/* init nonleaf rider */
	err = getIntRider(sidx, p, &fcfg, &nolfs);
	if (err != BEET_OK) {
		beet_rider_destroy(lfs); free(lfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return err;
	}

	/* open sub index */
	if (fcfg.indexType == BEET_INDEX_HOST &&
	    fcfg.subPath != NULL) {
		err = openIndex(base,
                                fcfg.subPath,
		                handle,ocfg,0,
		                &sidx->subidx);
		if (err != BEET_OK) {
			beet_rider_destroy(lfs); free(lfs);
			beet_rider_destroy(nolfs); free(nolfs);
			beet_config_destroy(&fcfg);
			beet_index_close(sidx); free(p);
			return err;
		}
	}

	/* get inserter */
	err = getins(sidx, &fcfg, &ins);
	if (err != BEET_OK) {
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return err;
	}

	/* get compare */
	err = beet_config_getCompare(&fcfg, ocfg, handle, &cmp);
	if (err != BEET_OK) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return err;
	}

	/* get rsc */
	err = beet_config_getRsc(&fcfg, ocfg, handle, &rinit, &rdst);
	if (err != BEET_OK) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return err;
	}

	/* allocate tree */
	sidx->tree = calloc(1,sizeof(beet_tree_t));
	if (sidx->tree == NULL) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx); free(p);
		return BEET_ERR_NOMEM;
	}

	/* init tree */
	err = beet_tree_init(sidx->tree,
	                     fcfg.leafNodeSize,
	                     fcfg.intNodeSize,
	                     fcfg.keySize,
	                     fcfg.dataSize,
	                     nolfs, lfs,
	                     sidx->roof,
	                     cmp,rinit,rdst,
	                     ocfg->rsc, ins);
	if (err != BEET_OK) {
		free(ins); free(p);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* make first root node */
	if (standalone) {
		err = mkroot(sidx, p);
		if (err != BEET_OK) {
			beet_config_destroy(&fcfg);
			beet_index_close(sidx); free(p);
			return err;
		}
	}
	beet_config_destroy(&fcfg); free(p);
	*idx = sidx;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Open an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_open(char *base,   char *path,
                           void             *handle,
                           beet_open_config_t *ocfg,
                           beet_index_t        *idx) {
	return openIndex(base, path, handle, ocfg, 1, idx);
}

/* ------------------------------------------------------------------------
 * Close an index
 * ------------------------------------------------------------------------
 */
void beet_index_close(beet_index_t idx) {

	if (idx == NULL) return;
	if (idx->roof != NULL) {
		fclose(idx->roof); idx->roof = NULL;
	}
	if (idx->subidx != NULL) {
		beet_index_close(idx->subidx); idx->subidx = NULL;
	}
	if (idx->tree != NULL) {
		beet_tree_destroy(idx->tree);
		free(idx->tree); idx->tree = NULL;
	}
	free(idx);
}

/* ------------------------------------------------------------------------
 * Get Index type
 * ------------------------------------------------------------------------
 */
int beet_index_type(beet_index_t idx) {
	if (idx == NULL) return -1;
	if (idx->subidx != NULL) return BEET_INDEX_HOST;
	if (idx->tree->dsize == 0) return BEET_INDEX_NULL;
	return BEET_INDEX_PLAIN;
}

/* ------------------------------------------------------------------------
 * Get index compare method
 * ------------------------------------------------------------------------
 */
beet_compare_t beet_index_getCompare(beet_index_t idx) {
	if (idx == NULL) return NULL;
	if (idx->tree == NULL) return NULL;
	return idx->tree->cmp;
}

/* ------------------------------------------------------------------------
 * Get user-defined resource
 * ------------------------------------------------------------------------
 */
void *beet_index_getResource(beet_index_t idx) {
	if (idx == NULL) return NULL;
	if (idx->tree == NULL) return NULL;
	return idx->tree->rsc;
}

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index without updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_insert(beet_index_t idx, void *key, void *data) {
	IDXNULL();
	return beet_tree_insert(idx->tree,
	                       &idx->root, key, data);
}

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_upsert(beet_index_t idx, void *key, void *data) {
	IDXNULL();
	return beet_tree_upsert(idx->tree,
	                       &idx->root, key, data);
}

/* ------------------------------------------------------------------------
 * Hide a key from the index. The key won't be found any more
 * but is physically still in the tree. This operation is much
 * faster than deleting keys. It is recommended to schedule
 * regular delete operations to completely remove hidden keys.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_hide(beet_index_t idx, void *key) {
	IDXNULL();
	return beet_tree_hide(idx->tree,
	                     &idx->root, key);
}

/* ------------------------------------------------------------------------
 * Uncover a hidden key in the index, i.e. undo beet_index_hide.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_unhide(beet_index_t idx, void *key) {
	IDXNULL();
	return beet_tree_unhide(idx->tree,
	                       &idx->root, key);
}

/* ------------------------------------------------------------------------
 * Removes a key and all its data from the index
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_deleteKey(beet_index_t idx, void *key);

/* ------------------------------------------------------------------------
 * Removes a single data point from a subindex and, 
 * if this was the last one, also its key from the main index.
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_delete(beet_index_t  idx,
                             void         *key1,
                             void         *key2);

/* ------------------------------------------------------------------------
 * Helper: release state
 * TODO:
 * what to do when release fails (leaking memory?)
 * ------------------------------------------------------------------------
 */
static inline beet_err_t staterelease(beet_state_t state) {
	beet_err_t err;

	for(int i=1; i>=0; i--) {
		if (state->node[i] != NULL &&
		    state->tree[i] != NULL) {
			err = beet_tree_release(state->tree[i],
			                        state->node[i]);
			if (err != BEET_OK) return err;
			free(state->node[i]);
			state->node[i] = NULL;
			state->tree[i] = NULL;
		}
	}
	CLEANSTATE(state);
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get data
 * ------------------------------------------------------------------------
 */
static beet_err_t getdata(beet_index_t idx,
                          beet_state_t state,
                          const void  *key,
                          void       **data) {
	beet_err_t    err;
	beet_node_t *node;
	int32_t      slot;

	if (state->root != NULL) {
		err = beet_tree_get(idx->tree, state->root,
		                               key, &node);
	} else {
		err = beet_tree_get(idx->tree, &idx->root, key, &node);
	}
	if (err != BEET_OK) return err;

	slot = beet_node_search(node, idx->tree->ksize,
	                        key,  idx->tree->cmp,
	                              idx->tree->rsc);
	if (slot < 0 || slot > idx->tree->lsize) {
		beet_tree_release(idx->tree, node); free(node);
		return BEET_ERR_KEYNOF;
	}
	if (!beet_node_equal(node, slot, idx->tree->ksize,
	                           key,  idx->tree->cmp,
	                                 idx->tree->rsc)) {
		beet_tree_release(idx->tree, node); free(node);
		return BEET_ERR_KEYNOF;
	}
	if (beet_node_hidden(node, slot)) {
		beet_tree_release(idx->tree, node); free(node);
		return BEET_ERR_KEYNOF;
	}
	if (data != NULL) {
		*data = beet_node_getData(node, slot,
		                   idx->tree->dsize);
	}
	for(int i=0;i<2;i++) {
		if (state->node[i] == NULL) {
			state->node[i] = node;
			state->tree[i] = idx->tree;
			break;
		}
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Compute height of the tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_height(beet_index_t idx, uint32_t *h) {
	IDXNULL();
	return beet_tree_height(idx->tree, &idx->root, h);
}

/* ------------------------------------------------------------------------
 * Get data by key (simple)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_copy(beet_index_t idx, const void *key, void *data) {
	struct beet_state_t state;
	beet_err_t err;
	void *tmp;

	IDXNULL();

	CLEANSTATE(&state);

	err = getdata(idx, &state, key, &tmp);
	if (err != BEET_OK) return err;

	memcpy(data, tmp, idx->tree->dsize);

	err = staterelease(&state);
	if (err != BEET_OK) return err;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Allocate new state
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_alloc(beet_index_t idx, beet_state_t *state) {
	*state = calloc(1, sizeof(struct beet_state_t));
	if (*state == NULL) return BEET_ERR_NOMEM;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Destroy and free state
 * ------------------------------------------------------------------------
 */
void beet_state_destroy(beet_state_t state) {
	if (state == NULL) return;
	free(state);
}

/* ------------------------------------------------------------------------
 * Clean state for reuse
 * ------------------------------------------------------------------------
 */
void beet_state_reinit(beet_state_t state) {
	if (state == NULL) return;
	CLEANSTATE(state);
}

/* ------------------------------------------------------------------------
 * Release all locks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_release(beet_state_t state) {
	STATENULL();
	return staterelease(state);
}

/* ------------------------------------------------------------------------
 * Get data with state and finer control
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_get(beet_index_t  idx,
                          beet_state_t  state,
                          uint16_t      flags,
                          const void   *key,
                          void       **data) {
	beet_err_t err, err2;

	IDXNULL();
	STATENULL();

	if (flags & BEET_FLAGS_SUBTREE) {
		if (idx->subidx == NULL) return BEET_ERR_NOSUB;
		if (state->root == NULL) return BEET_ERR_INVALID;
		err = getdata(idx->subidx, state, key, data);
	} else if (flags & BEET_FLAGS_ROOT) {
		err = getdata(idx, state, key,
		             (void**)&state->root);
	} else {
		err = getdata(idx, state, key, data);
	}
	if (flags & BEET_FLAGS_RELEASE) {
		err2 = staterelease(state);
		if (err2 != BEET_OK) return err2;
	} 
	if (err != BEET_OK) return err;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get data with state and finer control for subindex
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_get2(beet_index_t  idx,
                           beet_state_t  state,
                           uint16_t      flags,
                           const void   *key1,
                           const void   *key2,
                           void        **data) {
	beet_err_t err;

	err = beet_index_get(idx, state,
	            BEET_FLAGS_ROOT, key1, NULL);
	if (err != BEET_OK) return err;

	err = beet_index_get(idx, state, flags |
	                 BEET_FLAGS_SUBTREE, key2, data);
	if (err != BEET_OK) return err;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Mere existence test
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_doesExist(beet_index_t  idx,
                                const void   *key) {
	struct beet_state_t state;
	CLEANSTATE(&state);
	return beet_index_get(idx, &state, BEET_FLAGS_RELEASE, key, NULL);
}

/* ------------------------------------------------------------------------
 * Existence test for embedded index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_doesExist2(beet_index_t   idx,
                                 const void   *key1,
                                 const void   *key2) {
	beet_err_t err;
	struct beet_state_t state;
	CLEANSTATE(&state);

	err = beet_index_get(idx, &state, BEET_FLAGS_ROOT, key1, NULL);
	if (err != BEET_OK) return err;

	err = beet_index_get(idx, &state, BEET_FLAGS_SUBTREE |
	                                  BEET_FLAGS_RELEASE, key2, NULL);
	if (err != BEET_OK) {
		staterelease(&state);
		return err;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get iterator into subtree for key
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_getIter(beet_index_t  idx,
                              beet_state_t  state,
                              const void   *key,
                              beet_iter_t  iter) {
	beet_err_t     err;

	err = beet_index_get(idx, state, BEET_FLAGS_ROOT, key, NULL);
	if (err != BEET_OK) return err;

	err = beet_iter_init(iter, idx->subidx->tree,
	                     state->root, NULL, NULL,
	                     BEET_DIR_ASC);
	if (err != BEET_OK) {
		staterelease(state);
		return err;
	}
	iter->use = 0; /* second level may not be used */
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * range scan
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_range(beet_index_t    idx,
                            beet_range_t *range,
                            beet_dir_t      dir,
                            beet_iter_t    iter) {
	beet_err_t   err;
	void *from, *to;

	if (idx == NULL) return BEET_ERR_NOIDX;
	if (iter == NULL) return BEET_ERR_NOITER;

	if ((idx->subidx != NULL && iter->sub == NULL) ||
	    (idx->subidx == NULL && iter->sub != NULL)) {
		return BEET_ERR_BADITER;
	}

	if (range == NULL) {
		from = NULL; to = NULL;
	} else {
		from = range->fromkey;
		to   = range->tokey;
	}

	/* if we have a subidx, we create a subiter */
	if (idx->subidx != NULL && iter->sub != NULL) {
		err = beet_index_range(idx->subidx, NULL,
		                 BEET_DIR_ASC, iter->sub);
		if (err != BEET_OK) return err;
		iter->use = 1; /* second level may be used */
	}

	/* create main iter */
	err = beet_iter_init(iter,
		             idx->tree,
	                     &idx->root,
		             from,to,dir);
	if (err != BEET_OK) return err;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Allocate reusable iter
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_alloc(beet_index_t idx,
                           beet_iter_t *iter) {
	beet_err_t err;

	if (idx == NULL) return BEET_ERR_NOIDX;
	if (iter == NULL) return BEET_ERR_NOITER;
	*iter = calloc(1, sizeof(struct beet_iter_t));
	if (*iter == NULL) return BEET_ERR_NOMEM;
	(*iter)->tree = idx->tree;
	(*iter)->root = &idx->root;
	(*iter)->use  = 0;
	(*iter)->sub  = NULL;
	(*iter)->from = NULL;
	(*iter)->to   = NULL;
	(*iter)->pos  = -1;
	(*iter)->node = NULL;
	if (idx->subidx != NULL) {
		err = beet_iter_alloc(idx->subidx, &(*iter)->sub);
		if (err != BEET_OK) {
			free(*iter); *iter = NULL;
			return err;
		}
	}
	return BEET_OK;
}
