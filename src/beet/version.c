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
 * BEET Version Information
 * ========================================================================
 */
#include <beet/types.h>

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------
 * Get current version
 * ------------------------------------------------------------------------
 */
void beet_version_get(beet_version_t *ver) {
	if (ver == NULL) return;
	ver->first = BEET_VERSION_FIRST;
	ver->major = BEET_VERSION_MAJOR;
	ver->minor = BEET_VERSION_MINOR;
	ver->hotfx = BEET_VERSION_HOTFX;
}

/* ------------------------------------------------------------------------
 * Print version to stdout
 * ------------------------------------------------------------------------
 */
void beet_version_print() {
	fprintf(stdout, "%hu.%hu.%hu-%hu\n", 
	                BEET_VERSION_FIRST,
	                BEET_VERSION_MAJOR,
	                BEET_VERSION_MINOR,
	                BEET_VERSION_HOTFX);
}

