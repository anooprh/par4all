/*

  $Id$

  Copyright 1989-2009 MINES ParisTech

  This file is part of NewGen.

  NewGen is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  NewGen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU General Public License along with
  NewGen.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef SET_INCLUDED
#define SET_INCLUDED
/*

   Pierre Jouvelot (3 Avril 1989)

   Set (of CHAR *) package interface.

   WARNING: You aren't allowed to use = or == between sets. Always use
   SET_ASSIGN and SET_EQUAL.
*/

#include "newgen_types.h"
#include "newgen_hash.h"

/* Note: hash_chunk is not included in set_type */
typedef enum {
    set_string = hash_string ,
    set_int = hash_int,
    set_pointer = hash_pointer,
    set_private = hash_private
} set_type ;

/* FI: I do not understand why the type is duplicated at the set
   level. Is there a potential consistency problem with the hash
   table type? Is this a consequence of the decision to hide the
   actual hash_table data structure? */
typedef struct {
    hash_table table ;
    set_type type ;
} set_chunk, *set ;

#define set_undefined ((set)(-16))
#define set_undefined_p(s) ((s)==set_undefined)

#define SET_MAP(element,code,set) \
    { HASH_MAP(_set_map_key, element, code, (set)->table); }

struct cons;

/* functions declared in set.c */
extern set set_generic_make(set_type typ,
			    hash_equals_t equals_p,
			    hash_rank_t rank);
extern set set_make(set_type typ);
extern set set_singleton(set_type type, void *p);
extern set set_assign(set s1, set s2);
extern set set_add_element(set s1, set s2, void *e);
extern bool set_belong_p(set s, void *e);
extern set set_union(set s1, set s2, set s3);
extern set set_intersection(set s1, set s2, set s3);
extern set set_difference(set s1, set s2, set s3);
extern set set_del_element(set s1, set s2, void *e);
extern set set_delfree_element(set s1, set s2, void *e);
extern bool set_equal(set s1, set s2);
extern void set_clear(set s);
extern void set_free(set s);
extern bool set_empty_p(set s);
extern void gen_set_closure_iterate(void (*iterate)(void *, set), set initial, bool dont_iterate_twice);
extern void gen_set_closure(void (*iterate)(void *, set), set initial);
extern int set_own_allocated_memory(set s);
extern struct cons* set_to_list(set s);
extern set list_to_set(struct cons* l, set_type st);

#endif
