/*

  $Id$

  Copyright 1989-2010 MINES ParisTech

  This file is part of PIPS.

  PIPS is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  PIPS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.

  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PIPS.  If not, see <http://www.gnu.org/licenses/>.

*/

 /* package sc */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
	
/* void sc_error(va_dcl va_alist) should be called to terminate execution
 * and to core dump when data structures are corrupted or when an undefined
 * operation is requested (zero divide for instance).
 * SC_ERROR should be called as:
 * 
 *   SC_ERROR(function_name, format, expression-list)
 * 
 * where function_name is a string containing the name of the function
 * calling SC_ERROR, and where format and expression-list are passed as
 * arguments to vprintf. SC_ERROR terminates execution with abort.
 */
void sc_error(char * name, char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    /* print name of function causing error */
    (void) fprintf(stderr, "sc error in %s: ", name);

    /* print out remainder of message */
    (void) vfprintf(stderr, fmt, args);
    va_end(args);

    /* create a core file for debug */
    (void) abort();
}
