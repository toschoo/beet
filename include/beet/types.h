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
 * BEET Base Types
 * ========================================================================
 */
#ifndef beet_types_decl
#define beet_types_decl

/* -----------------------------------------------------------------------
 * Error indicator
 * -----------------------------------------------------------------------
 */
typedef int beet_err_t;

/* ------------------------------------------------------------------------
 * Compare function
 * ------------------------------------------------------------------------
 */
typedef char (*beet_compare_t)(const void*, const void*, void*);

#define BEET_CMP_LESS   -1
#define BEET_CMP_EQUAL   0
#define BEET_CMP_GREATER 1

/* ------------------------------------------------------------------------
 * resource initialisation and destruction
 * ------------------------------------------------------------------------
 */
typedef beet_err_t (*beet_rscinit_t)(void**);
typedef void (*beet_rscdest_t)(void**);

/* -----------------------------------------------------------------------
 * insert (key, data) pairs into embedded b+tree
 * -----------------------------------------------------------------------
 */
typedef struct {
	void *key;
	void *data;
} beet_pair_t;

/* -----------------------------------------------------------------------
 * Return a constant string describing the error
 * -----------------------------------------------------------------------
 */
const char *beet_errdesc(beet_err_t err);

/* -----------------------------------------------------------------------
 * Return the OS description of the error in errno
 * -----------------------------------------------------------------------
 */
const char *beet_oserrdesc();

/* -----------------------------------------------------------------------
 * Error constants
 * -----------------------------------------------------------------------
 */
#define BEET_OK 0

#define BEET_ERR_NOMEM    2
#define BEET_ERR_INVALID  3
#define BEET_ERR_UNKNKEY  4
#define BEET_ERR_NORSC    5
#define BEET_ERR_BADF     6
#define BEET_ERR_NOFILE   7
#define BEET_ERR_NOTREE   8
#define BEET_ERR_NONODE   9
#define BEET_ERR_NOPAGE  10
#define BEET_ERR_NORIDER 11
#define BEET_ERR_NOFD    12
#define BEET_ERR_NOKEY   13
#define BEET_ERR_NOROOT  14
#define BEET_ERR_NOSLOT  15
#define BEET_ERR_BADSIZE 16
#define BEET_ERR_BADNODE 17
#define BEET_ERR_BADPAGE 18
#define BEET_ERR_NONAME  19
#define BEET_ERR_TOOBIG  20
#define BEET_ERR_NOLATCH 21
#define BEET_ERR_NOLOCK  22
#define BEET_ERR_NOMAGIC 23
#define BEET_ERR_NOVER   24
#define BEET_ERR_UNKNVER 25
#define BEET_ERR_BADCFG  26
#define BEET_ERR_UNKNTYP 27
#define BEET_ERR_NOTSUPP 28
#define BEET_ERR_KEYNOF  29
#define BEET_ERR_NOSYM   30
#define BEET_ERR_TEST   199
#define BEET_ERR_PANIC  999

/* -----------------------------------------------------------------------
 * Errors returned from OS services
 * -----------------------------------------------------------------------
 */
#define BEET_OSERR_BUSY   -2
#define BEET_OSERR_NOMEM  -3
#define BEET_OSERR_INVAL  -4
#define BEET_OSERR_AGAIN  -5
#define BEET_OSERR_PERM   -6
#define BEET_OSERR_DEADLK -7
#define BEET_OSERR_INTRP  -8

/* -----------------------------------------------------------------------
 * Errors with further information in errno
 * -----------------------------------------------------------------------
 */
#define BEET_OSERR_ERRNO -100

#define BEET_OSERR_SEEK  -101
#define BEET_OSERR_TELL  -102
#define BEET_OSERR_OPEN  -103
#define BEET_OSERR_CLOSE -104
#define BEET_OSERR_READ  -105
#define BEET_OSERR_WRITE -106
#define BEET_OSERR_FLUSH -107
#define BEET_OSERR_SLEEP -108
#define BEET_OSERR_MKDIR -109
#define BEET_OSERR_REMOV -110

#define BEET_OSERR_UNKN  -9999
#endif

