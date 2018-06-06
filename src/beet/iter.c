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
 * Internal iterator API
 * ========================================================================
 */
#include <beet/iter.h>
#include <beet/iterimp.h>

#define TOITER(x) \
	((struct beet_iter_t*)x)

/* ------------------------------------------------------------------------
 * Reset the iterator to start position
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_init(beet_iter_t    iter,
                          beet_iter_t    sub,
                          beet_tree_t   *tree,
                          beet_pageid_t *root,
                          const void    *from,
                          const void    *to,
                          beet_dir_t     dir) {

	if (iter == NULL) return BEET_ERR_NOITER;
	if (tree == NULL) return BEET_ERR_NOTREE;

	iter->tree = tree;
	iter->root = root;
	iter->from = from;
	iter->to   = to;
	iter->dir  = dir;
	iter->sub  = sub;
	iter->pos  = -1;
	iter->node = NULL;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Reset the iterator to start position
 * ------------------------------------------------------------------------
 */
void beet_iter_destroy(beet_iter_t iter) {
	if (iter == NULL) return;
	if (iter->sub != NULL) beet_iter_destroy(iter->sub);
	if (iter->node != NULL) {
		beet_tree_release(iter->tree, iter->node);
		free(iter->node); iter->node = NULL;
	}
	free(iter);
}

/* ------------------------------------------------------------------------
 * Reset the iterator to start position
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_reset(beet_iter_t iter) {
	beet_err_t err;

	if (iter == NULL) return BEET_ERR_NOITER;
	if (iter->level == 1) return beet_iter_reset(iter->sub);
	if (iter->node != NULL) {
		err = beet_tree_release(iter->tree, iter->node);
		if (err != BEET_OK) return err;
	}
	iter->pos = -1;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Move the iterator one key forward
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_move(beet_iter_t iter, void **key, void **data) {
	beet_err_t   err;
	beet_node_t *tmp;

	if (iter == NULL) return BEET_ERR_NOITER;

	if (iter->level == 1) return beet_iter_move(iter->sub, key, data);

	if (iter->node != NULL && iter->pos >= iter->node->size) {
		err = beet_tree_next(iter->tree, iter->node, &tmp);
		if (err != BEET_OK) return err;
		iter->pos = 0;

		err = beet_tree_release(iter->tree, iter->node);
		free(iter->node); iter->node = tmp;
	}
	if (iter->node == NULL) {
		if (iter->pos != -1) return BEET_ERR_EOF;
		if (iter->from != NULL) {
			err = beet_tree_get(iter->tree, iter->root,
			                                iter->from,
			                               &iter->node);
		} else if (iter->dir == BEET_DIR_ASC) {
			err = beet_tree_left(iter->tree, iter->root,
			                                &iter->node);
		} else {
			err = beet_tree_right(iter->tree, iter->root,
			                                 &iter->node);
		}
		if (err != BEET_OK) return err;
		iter->pos = 0;
	}
	*key = iter->node->keys+iter->pos*iter->tree->ksize;
	*data = iter->node->kids+iter->pos*iter->tree->dsize;
	iter->pos++;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Enter embedded tree 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_enter(beet_iter_t iter) {
	if (iter == NULL) return BEET_ERR_NOITER;
	if (iter->sub == NULL) return BEET_ERR_NOSUB;
	if (iter->node == NULL) return BEET_ERR_NOSUB;
	if (iter->level == 1) return BEET_OK;
	iter->sub->root = (beet_pageid_t*)(iter->node->kids+
	                                   iter->pos*
	                                   sizeof(beet_pageid_t));
	iter->level = 1;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Change back to host tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_leave(beet_iter_t iter) {
	beet_err_t err;

	if (iter == NULL) return BEET_ERR_NOITER;
	if (iter->level == 0) return BEET_OK;
	if (iter->sub == NULL) return BEET_ERR_NOSUB;
	err = beet_iter_reset(iter->sub);
	if (err != BEET_OK) return err;
	iter->sub->root = NULL;
	iter->level = 0;
	return BEET_OK;
}

