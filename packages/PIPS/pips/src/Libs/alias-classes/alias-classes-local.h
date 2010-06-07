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
#include "points_to_private.h"
#include "effects.h"
#define SEQUENTIAL_POINTS_TO_SETS_SUFFIX ".pt"
#define USER_POINTS_TO_SETS_SUFFIX ".upt"

/* The definition of *ANYWHERE*:*ANYWHERE* should be removed from
   ri-util-local.h */
#define ANY_MODULE_NAME "*ANY_MODULE*"
#define ANYWHERE_LOCATION "*ANYWHERE*"
#define NOWHERE_LOCATION "*NOWHERE*"
#define NULL_POINTER_NAME "*NULL_POINTER*"
