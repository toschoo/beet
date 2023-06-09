/* ========================================================================
 * (c) Tobias Schoofs, 2018
 * ========================================================================
 * Data insert abstraction
 * ========================================================================
 */
#include <beet/ins.h>
#include <beet/node.h>
#include <beet/tree.h>

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Plain inserter
 * -------------------------------------------------------------------------
 */
beet_err_t beet_ins_plain(void *ignore, char upd, uint32_t sz, void* trg,
                                                        const void *data) {
	memcpy(trg, data, sz);
	return BEET_OK;
}

void beet_ins_plainclean(void *ignore) {}
void beet_ins_plaininit(void *ignore, uint32_t n, void *kids) {}
void beet_ins_plainclear(void *ignore, void *kids) {}

beet_err_t beet_ins_setPlain(beet_ins_t *ins) {
	ins->rsc = NULL;
	ins->inserter = &beet_ins_plain;
	ins->cleaner = &beet_ins_plainclean;
	ins->ninit = &beet_ins_plaininit;
	ins->clear = &beet_ins_plainclear;
	return BEET_OK;
}

/* -------------------------------------------------------------------------
 * Embedded inserter
 * -------------------------------------------------------------------------
 */
#define PAIR(x) \
	((beet_pair_t*)x)

beet_err_t beet_ins_embedded(void *tree, char upd, uint32_t sz, void *root,
                                                          const void *data) {
	beet_err_t err;
	if (*(beet_pageid_t*)root == BEET_PAGE_NULL) {
		err = beet_tree_makeRoot(tree, root);
		if (err != BEET_OK) return err;
		// fprintf(stderr, "new root is %u\n", *(beet_pageid_t*)root);
	}
	if (upd) return beet_tree_upsert(tree, root, PAIR(data)->key,
 	                                             PAIR(data)->data);
	return beet_tree_insert(tree, root, PAIR(data)->key,
	                                    PAIR(data)->data);
}

#define INS(x) \
	((beet_ins_t*)x)

void beet_ins_embeddedclean(void *ins) {
	/* we should not destroy it here!
	 * the index will destroy it!
	if (ins == NULL) return;
	if (INS(ins)->rsc != NULL) {
		beet_tree_destroy(INS(ins)->rsc);
		free(INS(ins)->rsc); INS(ins)->rsc = NULL;
	}
	*/
}

void beet_ins_embeddedinit(void *ignore, uint32_t n, void *kids) {
	for(int i=0; i<n; i++) {
		((beet_pageid_t*)kids)[i] = BEET_PAGE_NULL;
	}
}

void beet_ins_embeddedclear(void *ignore, void *kid) {
		*(beet_pageid_t*)kid = BEET_PAGE_NULL;
}

beet_err_t beet_ins_setEmbedded(beet_ins_t *ins, void *subtree) {
	ins->rsc = subtree;
	ins->inserter = &beet_ins_embedded;
	ins->cleaner = &beet_ins_embeddedclean;
	ins->ninit = &beet_ins_embeddedinit;
	ins->clear = &beet_ins_embeddedclear;
	return BEET_OK;
}
