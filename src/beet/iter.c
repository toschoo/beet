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
#include <beet/index.h>

/* ------------------------------------------------------------------------
 * Reset the iterator to start position
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_init(beet_iter_t    iter,
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
	iter->pos  = -1;
	iter->node = NULL;

	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Destroy the iterator
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
		free(iter->node); iter->node = NULL;
		if (err != BEET_OK) return err;
	}
	iter->pos = -1;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Helper: get start node for 'from'
 * ------------------------------------------------------------------------
 */
static inline beet_err_t getfrom(beet_iter_t iter) {
	char x;
	beet_err_t err;
	beet_node_t *tmp;

	for(;;) {
		iter->pos = beet_node_search(iter->node,
		                             iter->tree->ksize,
		                             iter->from,
		                             iter->tree->cmp,
		                             iter->tree->rsc);
		if (iter->pos < 0) {
			beet_tree_release(iter->tree, iter->node);
			return BEET_ERR_PANIC;
		}
		if (iter->pos == iter->node->size) {
			/* is this case possible -- 
			 * or should we rather treat it as error ? */
			if (iter->dir == BEET_DIR_ASC) {
				err = beet_tree_next(iter->tree, iter->node, &tmp);
				if (err != BEET_OK) return err;
				
				err = beet_tree_release(iter->tree, iter->node);
				if (err != BEET_OK) return err;

				iter->node = tmp; continue;

			} else iter->pos--;
		}
		/*
		fprintf(stderr, "from: %d, found %d (%d)\n",
		                *(int*)iter->from, iter->pos,
		                *(int*)(iter->node->keys+iter->pos*iter->tree->ksize));
		*/
		for(;;) {
			/*
			fprintf(stderr, "looking at %d (%d)\n",
			       iter->pos,
			      *(int*)(iter->node->keys+iter->pos*iter->tree->ksize));
			*/
			x = iter->tree->cmp(iter->from,
			                    iter->node->keys+
			                    iter->pos*
			                    iter->tree->ksize,
			                    iter->tree->rsc);
			if ((iter->dir == BEET_DIR_ASC  && 
			     x != BEET_CMP_GREATER)     ||
			    (iter->dir == BEET_DIR_DESC &&
			     x != BEET_CMP_LESS)) {
				return BEET_OK;
			}
			if (iter->dir == BEET_DIR_ASC) {
				iter->pos++;
				if (iter->pos >= iter->node->size)
					return BEET_OK;
			} else {
				iter->pos--;
				if (iter->pos < 0) {
					/* is this case possible -- 
					 * or should we rather
					 * treat it as error ? */
					err = beet_tree_prev(iter->tree,
					                     iter->node,
					                          &tmp);
					if (err != BEET_OK) return err;
				
					err = beet_tree_release(iter->tree,
					                        iter->node);
					if (err != BEET_OK) return err;
					iter->node = tmp; break;
				}
			}
		}
	}
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
	if (iter->level == 1) {
		return beet_iter_move(iter->sub, key, data);
	}
	if (iter->node != NULL && (
	   (iter->dir == BEET_DIR_ASC  && iter->pos == iter->node->size) ||
	   (iter->dir == BEET_DIR_DESC && iter->pos == -1))) {
		if (iter->dir == BEET_DIR_ASC) {
			err = beet_tree_next(iter->tree, iter->node, &tmp);
		} else {
			err = beet_tree_prev(iter->tree, iter->node, &tmp);
		}
		if (err != BEET_OK) return err;

		iter->pos = iter->dir == BEET_DIR_ASC?0:tmp->size-1;

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

		if (iter->from != NULL) {
			err = getfrom(iter);
			if (err != BEET_OK) return err;
		} else {
			iter->pos = iter->dir == BEET_DIR_ASC?0:
			            iter->node->size-1;
		}
	}

	*key = iter->node->keys+iter->pos*iter->tree->ksize;
	if (data != NULL) {
		*data = iter->node->kids+iter->pos*iter->tree->dsize;
	}

	if (iter->to != NULL) {
		if (iter->dir == BEET_DIR_ASC) {
			if (iter->tree->cmp(*key, iter->to,
			    iter->tree->rsc) == BEET_CMP_GREATER)
				return BEET_ERR_EOF;
		} else {
			if (iter->tree->cmp(*key, iter->to,
			    iter->tree->rsc) == BEET_CMP_LESS)
				return BEET_ERR_EOF;
		}
	}
	if (iter->dir == BEET_DIR_ASC) iter->pos++; else iter->pos--;
	return BEET_OK;
}

/* ------------------------------------------------------------------------
 * Enter embedded tree 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_enter(beet_iter_t iter) {
	int32_t pos;
	if (iter == NULL) return BEET_ERR_NOITER;
	if (iter->sub == NULL) return BEET_ERR_NOSUB;
	if (iter->sub->tree == NULL) return BEET_ERR_NOTREE;
	if (!iter->use) return BEET_ERR_ONEWAY;
	if (iter->node == NULL) return BEET_ERR_NONODE;
	if (iter->level == 1) return BEET_OK;
	pos = iter->dir==BEET_DIR_ASC?iter->pos-1:iter->pos+1;
	if (pos < 0 || pos >= iter->node->size) return BEET_ERR_BADSTAT;
	iter->sub->root = (beet_pageid_t*)(iter->node->kids+pos*
	                                   sizeof(beet_pageid_t));
	iter->level = 1;
	return beet_iter_reset(iter->sub);
}

/* ------------------------------------------------------------------------
 * Change back to host tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_leave(beet_iter_t iter) {
	beet_err_t err;

	if (iter == NULL) return BEET_ERR_NOITER;
	if (iter->sub == NULL) return BEET_ERR_NOSUB;
	if (iter->sub->tree == NULL) return BEET_ERR_NOTREE;
	if (!iter->use) return BEET_ERR_ONEWAY;
	if (iter->level == 0) return BEET_OK;
	err = beet_iter_reset(iter->sub);
	if (err != BEET_OK) return err;
	iter->sub->root = NULL;
	iter->level = 0;
	return BEET_OK;
}

