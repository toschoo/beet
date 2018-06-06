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
#include <beet/config.h>

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
 * Close external library,
 * i.e. call dlclose
 * ------------------------------------------------------------------------
 */
void beet_lib_close(void *handle) {
	if (handle == NULL) return;
	dlclose(handle);
}

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
 * Init config: sets all values to zero/NULL
 * ------------------------------------------------------------------------
 */
void beet_open_config_ignore(beet_open_config_t *cfg) {
	if (cfg == NULL) return;

	cfg->leafCacheSize = BEET_CACHE_IGNORE;
	cfg->intCacheSize = BEET_CACHE_IGNORE;
	cfg->compare = NULL;
	cfg->rscinit = NULL;
	cfg->rscdest = NULL;
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
 * Destroy open config
 * ------------------------------------------------------------------------
 */
void beet_open_config_destroy(beet_open_config_t *cfg) {}

/* ------------------------------------------------------------------------
 * Check config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_validate(beet_config_t *cfg) {
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Override config
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_override(beet_config_t      *fcfg,
                                beet_open_config_t *ocfg) 
{
	if (ocfg == NULL) return BEET_OK;

	if (ocfg->leafCacheSize != BEET_CACHE_IGNORE) {
		fcfg->leafCacheSize = ocfg->leafCacheSize;
	}
	if (ocfg->intCacheSize != BEET_CACHE_IGNORE) {
		fcfg->intCacheSize = ocfg->intCacheSize;
	}
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Get compare callback
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_getCompare(beet_config_t      *fcfg,
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
 * Get rsc callbacks
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_getRsc(beet_config_t      *fcfg,
                              beet_open_config_t *ocfg,
                              void               *handle,
                              beet_rscinit_t     *rscinit,
                              beet_rscdest_t     *rscdest) 
{
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
 * Create configuration file
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_create(char *path, beet_config_t *cfg) {
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
 * Get configuration
 * ------------------------------------------------------------------------
 */
beet_err_t beet_config_get(char *path, beet_config_t *cfg) {
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

