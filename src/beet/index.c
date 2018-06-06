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

#include <beet/index.h>
#include <beet/rider.h>
#include <beet/node.h>
#include <beet/tree.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#define MAGIC 0x8ee7
#define VERSION 1

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
 * Init config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_config_init(beet_config_t *cfg) {
	if (cfg == NULL) return;
	memset(cfg, 0, sizeof(beet_config_t));
}

/* ------------------------------------------------------------------------
 * Init config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_open_config_init(beet_open_config_t *cfg) {
	if (cfg == NULL) return;
	memset(cfg, 0, sizeof(beet_open_config_t));
}

/* ------------------------------------------------------------------------
 * Macro: index not NULL
 * ------------------------------------------------------------------------
 */
#define IDXNULL() \
	if (idx == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * Macro: state not NULL
 * ------------------------------------------------------------------------
 */
#define STATENULL() \
	if (state == NULL) return BEET_ERR_INVALID;

/* ------------------------------------------------------------------------
 * Macro: Convert beet_index_t to struct
 * ------------------------------------------------------------------------
 */
#define TOIDX(x) \
	((struct beet_index_t*)x)

/* ------------------------------------------------------------------------
 * Macro: Convert beet_state_t to struct
 * ------------------------------------------------------------------------
 */
#define TOSTATE(x) \
	((struct beet_state_t*)x)

/* ------------------------------------------------------------------------
 * Macro: clean state
 * ------------------------------------------------------------------------
 */
#define CLEANSTATE(x) \
	memset(x, 0, sizeof(struct beet_state_t));

/* ------------------------------------------------------------------------
 * Helper: check config
 * ------------------------------------------------------------------------
 */
static inline beet_err_t checkcfg(beet_config_t *cfg) {
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: update config
 * ------------------------------------------------------------------------
 */
static inline beet_err_t updcfg(beet_config_t      *fcfg,
                                beet_open_config_t *ocfg) {
	if (ocfg->leafCacheSize != BEET_CACHE_IGNORE) {
		fcfg->leafCacheSize = ocfg->leafCacheSize;
	}
	if (ocfg->intCacheSize != BEET_CACHE_IGNORE) {
		fcfg->intCacheSize = ocfg->intCacheSize;
	}
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
	if (cfg->rscinit == NULL) {
		m = 0;
		if (fwrite(&m, 1, 1, f) != 1) return BEET_OSERR_WRITE;
	} else {
		if (fwrite(cfg->rscinit, strlen(cfg->rscinit)+1, 1, f) != 1) {
			return BEET_OSERR_WRITE;
		}
	}
	if (cfg->rscdest == NULL) {
		m = 0;
		if (fwrite(&m, 1, 1, f) != 1) return BEET_OSERR_WRITE;
	} else {
		if (fwrite(cfg->rscdest, strlen(cfg->rscdest)+1, 1, f) != 1) {
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

	/* get path to subindex */
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

	/* get name of compare function */
	i++; k=i;
	for(;buf[i] != 0 && i < s; i++) {}
	if (i > s) {
		free(buf);
		if (cfg->subPath != NULL) {
			free(cfg->subPath);
			cfg->subPath = NULL;
		}
		cfg->compare = NULL;
		return BEET_ERR_BADCFG;
	}
	if (i==k) cfg->compare = NULL;
	else {
		cfg->compare = malloc(i-k+1);
		if (cfg->compare == NULL) {
			free(buf);
			if (cfg->subPath != NULL) {
				free(cfg->subPath);
				cfg->subPath = NULL;
			}
			return BEET_ERR_NOMEM;
		}
		strcpy(cfg->compare, buf+k);
	}

	/* get name of rscinit function */
	i++; k=i;
	for(;buf[i] != 0 && i < s; i++) {}
	if (i > s) {
		free(buf);
		if (cfg->subPath != NULL) {
			free(cfg->subPath); cfg->subPath = NULL;
		}
		if (cfg->compare != NULL) {
			free(cfg->compare); cfg->compare = NULL;
		}
		cfg->rscinit = NULL;
		return BEET_ERR_BADCFG;
	}
	if (i==k) cfg->rscinit = NULL;
	else {
		cfg->rscinit = malloc(i-k+1);
		if (cfg->rscinit == NULL) {
			free(buf);
			if (cfg->subPath != NULL) {
				free(cfg->subPath); cfg->subPath = NULL;
			}
			if (cfg->compare != NULL) {
				free(cfg->compare); cfg->compare = NULL;
			}
			return BEET_ERR_NOMEM;
		}
		strcpy(cfg->rscinit, buf+k);
	}

	/* get name of rscdest function */
	i++; k=i;
	for(;buf[i] != 0 && i < s; i++) {}
	if (i > s) {
		free(buf);
		if (cfg->subPath != NULL) {
			free(cfg->subPath); cfg->subPath = NULL;
		}
		if (cfg->compare != NULL) {
			free(cfg->compare); cfg->compare = NULL;
		}
		if (cfg->rscinit != NULL) {
			free(cfg->rscinit); cfg->rscinit = NULL;
		}
		cfg->rscdest = NULL;
		return BEET_ERR_BADCFG;
	}
	if (i==k) cfg->rscdest = NULL;
	else {
		cfg->rscdest = malloc(i-k+1);
		if (cfg->rscdest == NULL) {
			free(buf);
			if (cfg->subPath != NULL) {
				free(cfg->subPath); cfg->subPath = NULL;
			}
			if (cfg->compare != NULL) {
				free(cfg->compare); cfg->compare = NULL;
			}
			if (cfg->rscinit != NULL) {
				free(cfg->rscinit); cfg->rscinit = NULL;
			}
			return BEET_ERR_NOMEM;
		}
		strcpy(cfg->rscdest, buf+k);
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
 * Helper: get configuration
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getcfg(char *path, beet_config_t *cfg) {
	beet_err_t err;
	struct stat st;
	FILE        *f;
	char        *p;

	memset(cfg, 0, sizeof(beet_config_t));

	p = malloc(strlen(path) + 8);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/config", path);

	if (stat(p, &st) != 0) {
		free(p); return BEET_ERR_NOFILE;
	}

	f = fopen(p, "rb"); free(p);
	if (f == NULL) return BEET_OSERR_OPEN;

	err = readcfg(f, st.st_size, cfg);
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
static inline beet_err_t mkroot(struct beet_index_t *idx,
                                char *path) {
	beet_err_t err;
	struct stat st;
	char *p;

	/* we could reuse this path */
	p = malloc(sizeof(path) + sizeof(LEAF) + 2);
	if (p == NULL) return BEET_ERR_NOMEM;

	sprintf(p, "%s/%s", path, LEAF);

	if (stat(p, &st) != 0) {
		free(p); return BEET_ERR_NOFILE;
	}
	free(p);

	if (st.st_size > 0) return BEET_OK;

	err = beet_tree_makeRoot(idx->tree, &idx->root);
	if (err != BEET_OK) return err;

	rewind(idx->roof); // != 0) return BEET_OSERR_SEEK;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get roof
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getroof(struct beet_index_t *idx, char *path) {
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
static inline beet_err_t getins(struct beet_index_t *idx,
                                beet_config_t       *cfg,
                                beet_ins_t         **ins) {
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
 * Helper: get compare
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getcmp(beet_config_t      *fcfg,
                                beet_open_config_t *ocfg,
                                void               *handle,
                                beet_compare_t     *cmp) {

	/* if this is a host index, we must have a handle and a symbol */
	if (fcfg->indexType == BEET_INDEX_HOST &&
	    (fcfg->compare == NULL || handle == NULL)) {
		return BEET_ERR_INVALID;
	}
	if (ocfg->compare != NULL) {
		*cmp = ocfg->compare;
		return BEET_OK;
	}
	if (fcfg->compare == NULL) return BEET_ERR_INVALID;
	*cmp = dlsym(handle, fcfg->compare);
	if (*cmp == NULL) {
		fprintf(stderr, "NO SYMBOL for %s: %s\n",
		               fcfg->compare, dlerror());
		return BEET_ERR_NOSYM;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get rsc
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getrsc(beet_config_t      *fcfg,
                                beet_open_config_t *ocfg,
                                void               *handle,
                                beet_rscinit_t    *rscinit,
                                beet_rscdest_t    *rscdest) {
	*rscinit = NULL; *rscdest = NULL;

	if (ocfg->rscinit != NULL) *rscinit = ocfg->rscinit;
	if (ocfg->rscdest != NULL) *rscinit = ocfg->rscdest;
	if (*rscinit != NULL && *rscdest != NULL) return BEET_OK;

	if (handle == NULL) return BEET_OK;
	
	if (fcfg->rscinit != NULL) {
		*rscinit = dlsym(handle, fcfg->rscinit);
		if (*rscinit == NULL) {
			fprintf(stderr, "NO SYMBOL for %s: %s\n",
			                fcfg->rscinit, dlerror());
			return BEET_ERR_NOSYM;
		}
	}
	if (fcfg->rscdest != NULL) {
		*rscdest = dlsym(handle, fcfg->rscdest);
		if (*rscdest == NULL) {
			fprintf(stderr, "NO SYMBOL for %s: %s\n",
			                fcfg->rscdest, dlerror());
			return BEET_ERR_NOSYM;
		}
	}
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

	err = mkempty(path, LEAF);
	if (err != BEET_OK) return err;

	err = mkempty(path, INTERN);
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

	REMOVE(p, LEAF, s);
	REMOVE(p, INTERN, s);
	REMOVE(p, "config", s);
	REMOVE(p, "rook", s);

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: open an index
 * ------------------------------------------------------------------------
 */
static beet_err_t openIndex(char *path, void *handle,
                            beet_open_config_t *ocfg,
                            char          standalone,
                            beet_index_t       *idx) {
	struct beet_index_t *sidx;
	beet_rider_t *lfs, *nolfs;
	beet_config_t fcfg;
	beet_ins_t    *ins;
	beet_compare_t cmp=NULL;
	beet_rscinit_t rinit=NULL;
	beet_rscdest_t rdst=NULL;
	beet_err_t     err;

	if (idx == NULL) return BEET_ERR_INVALID;
	*idx = NULL;

	if (path == NULL) return BEET_ERR_INVALID;
	if (handle == NULL && ocfg == NULL) return BEET_ERR_INVALID;
	if (strnlen(path, 4097) > 4096) return BEET_ERR_TOOBIG;

	memset(&fcfg, 0, sizeof(beet_config_t));

	err = getcfg(path, &fcfg);
	if (err != BEET_OK) return err;
	
	err = updcfg(&fcfg, ocfg);
	if (err != BEET_OK) return err;

	sidx = calloc(1,sizeof(struct beet_index_t));
	if (sidx == NULL) {
		beet_config_destroy(&fcfg);
		return BEET_ERR_NOMEM;
	}

	sidx->standalone = standalone;

	/* open roof and set root */
	err = getroof(sidx, path);
	if (err != BEET_OK) {
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* init leaf rider */
	err = getLeafRider(sidx, path, &fcfg, &lfs);
	if (err != BEET_OK) {
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* init nonleaf rider */
	err = getIntRider(sidx, path, &fcfg, &nolfs);
	if (err != BEET_OK) {
		beet_rider_destroy(lfs); free(lfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* open sub index */
	if (fcfg.indexType == BEET_INDEX_HOST &&
	    fcfg.subPath != NULL) {
		err = openIndex(fcfg.subPath,
		                handle,ocfg,0,
		                &sidx->subidx);
		if (err != BEET_OK) {
			beet_rider_destroy(lfs); free(lfs);
			beet_rider_destroy(nolfs); free(nolfs);
			beet_config_destroy(&fcfg);
			beet_index_close(sidx);
			return err;
		}
	}

	/* get inserter */
	err = getins(sidx, &fcfg, &ins);
	if (err != BEET_OK) {
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* get compare */
	err = getcmp(&fcfg, ocfg, handle, &cmp);
	if (err != BEET_OK) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* get rsc */
	err = getrsc(&fcfg, ocfg, handle, &rinit, &rdst);
	if (err != BEET_OK) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* allocate tree */
	sidx->tree = calloc(1,sizeof(beet_tree_t));
	if (sidx->tree == NULL) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
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
                             ins);
	if (err != BEET_OK) {
		free(ins);
		beet_rider_destroy(lfs); free(lfs);
		beet_rider_destroy(nolfs); free(nolfs);
		beet_config_destroy(&fcfg);
		beet_index_close(sidx);
		return err;
	}

	/* make first root node */
	if (standalone) {
		err = mkroot(sidx, path);
		if (err != BEET_OK) {
			beet_config_destroy(&fcfg);
			beet_index_close(sidx);
			return err;
		}
	}
	beet_config_destroy(&fcfg);
	*idx = sidx;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Open an index
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_open(char *path, void *handle,
                           beet_open_config_t *ocfg,
                           beet_index_t        *idx) {
	return openIndex(path, handle, ocfg, 1, idx);
}

/* ------------------------------------------------------------------------
 * Close an index
 * ------------------------------------------------------------------------
 */
void beet_index_close(beet_index_t uidx) {
	struct beet_index_t *idx = (struct beet_index_t*)uidx;

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
 * Destroy config
 * ------------------------------------------------------------------------
 */
void beet_config_destroy(beet_config_t *cfg) {
	if (cfg == NULL) return;
	if (cfg->subPath != NULL) {
		free(cfg->subPath); cfg->subPath = NULL;
	}
	if (cfg->compare != NULL) {
		free(cfg->compare); cfg->compare = NULL;
	}
	if (cfg->rscinit != NULL) {
		free(cfg->rscinit); cfg->rscinit = NULL;
	}
	if (cfg->rscdest != NULL) {
		free(cfg->rscdest); cfg->rscdest = NULL;
	}
}

/* ------------------------------------------------------------------------
 * Initialise external library,
 * i.e. call dlopen
 * ------------------------------------------------------------------------
 */
void *beet_lib_init(char *lib) {
	void *handle = dlopen(lib, RTLD_LAZY | RTLD_LOCAL);
	if (handle == NULL) {
		fprintf(stderr, "cannot load library: %s\n", dlerror());
		return NULL;
	}
	return handle;
}

/* ------------------------------------------------------------------------
 * Destroy open config
 * ------------------------------------------------------------------------
 */
void beet_open_config_destroy(beet_open_config_t *cfg) {}

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index without updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_insert(beet_index_t idx, void *key, void *data) {
	IDXNULL();
	return beet_tree_insert(TOIDX(idx)->tree,
	                       &TOIDX(idx)->root, key, data);
}

/* ------------------------------------------------------------------------
 * Insert a (key, data) pair into the index updating the data
 * if the key already exists.
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_upsert(beet_index_t idx, void *key, void *data) {
	IDXNULL();
	return beet_tree_upsert(TOIDX(idx)->tree,
	                       &TOIDX(idx)->root, key, data);
}

/* ------------------------------------------------------------------------
 * Removes a key and all its data from the index
 * TODO: implement!
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_deleteKey(beet_index_t idx, void *key);

/* ------------------------------------------------------------------------
 * Helper: release state
 * TODO:
 * what to do when release fails (leaking memory?)
 * ------------------------------------------------------------------------
 */
static inline beet_err_t staterelease(struct beet_state_t *state) {
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
static beet_err_t getdata(struct beet_index_t *idx,
                          struct beet_state_t *state,
                          const void *key, void **data) {
	beet_err_t    err;
	beet_node_t *node;
	int32_t      slot;

	if (TOSTATE(state)->root != NULL) {
		err = beet_tree_get(idx->tree, TOSTATE(state)->root,
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
		return BEET_ERR_NOSLOT;
	}
	if (!beet_node_equal(node, slot, idx->tree->ksize,
	                           key,  idx->tree->cmp,
	                                 idx->tree->rsc)) {
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
 * Get data by key (simple)
 * ------------------------------------------------------------------------
 */
beet_err_t beet_index_copy(beet_index_t idx, const void *key, void *data) {
	struct beet_state_t state;
	beet_err_t err;
	void *tmp;

	IDXNULL();

	CLEANSTATE(&state);

	err = getdata(TOIDX(idx), &state, key, &tmp);
	if (err != BEET_OK) return err;

	memcpy(data, tmp, TOIDX(idx)->tree->dsize);

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
	CLEANSTATE(TOSTATE(state));
}

/* ------------------------------------------------------------------------
 * Release all locks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_state_release(beet_state_t state) {
	STATENULL();
	return staterelease(TOSTATE(state));
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
		if (TOIDX(idx)->subidx == NULL) return BEET_ERR_INVALID;
		if (TOSTATE(state)->root == NULL) return BEET_ERR_INVALID;
		err = getdata(TOIDX(idx)->subidx,
		              TOSTATE(state), key, data);
	} else if (flags & BEET_FLAGS_ROOT) {
		err = getdata(TOIDX(idx), TOSTATE(state), key,
		                 (void**)&TOSTATE(state)->root);
	} else {
		err = getdata(TOIDX(idx), TOSTATE(state), key, data);
	}
	if (flags & BEET_FLAGS_RELEASE) {
		err2 = staterelease(TOSTATE(state));
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

	err = beet_index_get(idx, TOSTATE(state),
	            BEET_FLAGS_ROOT, key1, NULL);
	if (err != BEET_OK) return err;

	err = beet_index_get(idx, TOSTATE(state), flags |
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
beet_err_t beet_index_range(beet_index_t   idx,
                            beet_state_t  state,
                            uint16_t      flags,
                            beet_range_t *range,
                            beet_iter_t   iter);
