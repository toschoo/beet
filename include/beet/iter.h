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
 * BEET Iterator
 * ========================================================================
 */
#ifndef beet_iter_decl
#define beet_iter_decl

#include <beet/types.h>

/* ------------------------------------------------------------------------
 * Iterator
 * ------------------------------------------------------------------------
 */
typedef struct beet_iter_t *beet_iter_t;

/* ------------------------------------------------------------------------
 * Destroy iterator
 * ------------------------------------------------------------------------
 */
void beet_iter_destroy(beet_iter_t iter);

/* ------------------------------------------------------------------------
 * Reset the iterator to start a new search
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_reset(beet_iter_t iter);

/* ------------------------------------------------------------------------
 * Move the iterator forward
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_move(beet_iter_t iter, void **key, void **data);

/* ------------------------------------------------------------------------
 * Enter embedded tree 
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_enter(beet_iter_t iter);

/* ------------------------------------------------------------------------
 * Change back to host tree
 * ------------------------------------------------------------------------
 */
beet_err_t beet_iter_leave(beet_iter_t iter);

#endif
