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
#ifndef beet_iter_internal_decl
#define beet_iter_internal_decl

#include <beet/types.h>
#include <beet/node.h>
#include <beet/tree.h>
#include <beet/iter.h>

#include <stdint.h>

struct beet_iter_t {
	beet_iter_t    sub;
	beet_tree_t   *tree;
	beet_pageid_t *root;
	beet_node_t   *node;
	const void    *from;
	const void    *to;
	int32_t       pos;
	char          level;
	beet_dir_t    dir;
};

beet_err_t beet_iter_init(beet_iter_t    iter,
                          beet_tree_t   *tree,
                          beet_pageid_t *root,
                          const void    *from,
                          const void    *to,
                          char           dir);
#endif
