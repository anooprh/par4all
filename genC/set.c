/*

	-- NewGen Project

	The NewGen software has been designed by Remi Triolet and Pierre
	Jouvelot (Ecole des Mines de Paris). This prototype implementation
	has been written by Pierre Jouvelot.

	This software is provided as is, and no guarantee whatsoever is
	provided regarding its appropriate behavior. Any request or comment
	should be sent to newgen@isatis.ensmp.fr.

	(C) Copyright Ecole des Mines de Paris, 1989

*/


/* - set.c
   
   Pierre Jouvelot (3 Avril 1989)

   Set package for any type of pointer. 

   To avoid sharing problem, all the routines are 3-adress: S1 = S2 op S3.
   It is up to the user to know what to do (e.g., freeing some temporary
   memory storage) before S1 is assigned a new value. */

#include <stdio.h>
#include <stdlib.h>
#include "genC.h"
#include "newgen_set.h"

#define INITIAL_SET_SIZE 10

void set_clear(), set_free();

/* Implementation of the Set package. */
    
set set_make( typ )
set_type typ ;
{
    set hp = (set)malloc( sizeof( set_chunk )) ;

    if( hp == (set)NULL ) {
	(void) fprintf( stderr, "set_make: cannot allocate\n" ) ;
	exit( 1 ) ;
    }
    hp->table = hash_table_make( typ, INITIAL_SET_SIZE ) ;
    hp->type = typ ;
    return( hp ) ;
}

set set_singleton(set_type type, void * p) 
{
    set s = set_make( type ) ;
    hash_put( s->table, p, p ) ;
    return( s ) ;
}

set set_assign( s1, s2 )
set s1, s2 ;
{
    if( s1 == s2 ) {
	return( s1 ) ;
    }
    else {
	set_clear( s1 ) ;
	HASH_MAP( k, v, {hash_put( s1->table, k, v ) ;}, s2->table ) ;
	return( s1 ) ;
    }
}

set set_add_element(set s1, set s2, void * e)
{
    if( s1 == s2 ) {
	if (! set_belong_p(s1, e))
	    hash_put(s1->table, e, e);
	return( s1 ) ;
    }
    else {
	set_clear( s1 ) ;
	HASH_MAP( k, v, {hash_put( s1->table, k, v ) ;}, s2->table ) ;
	if (! set_belong_p(s1, e))
	    hash_put(s1->table, e, e);
	return( s1 ) ;
    }
}

bool set_belong_p(set s, void * e)
{
    /* GO 7/8/95:
       Problem for set_string type because the value returned by
       hash_get is not the same than the pointer value, only the
       content of the string is the same ...
       
       return( hash_get(s->table, (char *) e) == (char *) e) ;
       */

    return hash_get(s->table, (char *) e) != HASH_UNDEFINED_VALUE;
}

set set_union(set s1, set s2, set s3)
{
    if( s1 != s3 ) {
	set_assign( s1, s2 ) ;
	HASH_MAP( k, v, {hash_put( s1->table, k, v ) ;}, s3->table ) ;
    }
    else {
	HASH_MAP( k, v, {hash_put( s1->table, k, v ) ;}, s2->table ) ;
    }
    return( s1 ) ;
}

set set_intersection(set s1, set s2, set s3)
{
    if( s1 != s2 && s1 != s3 ) {
	set_clear( s1 ) ;
	HASH_MAP( k, v, {if( hash_get( s2->table, k )
			    != (char *)HASH_UNDEFINED_VALUE ) 
				 hash_put( s1->table, k, v ) ;}, 
		 s3->table ) ;
	return( s1 ) ;
    }
    else {
	set tmp = set_make( s1->type ) ;

	HASH_MAP( k, v, {if( hash_get( s1->table, k ) 
			    != (char *)HASH_UNDEFINED_VALUE ) 
				 hash_put( tmp->table, k, v ) ;}, 
		 (s1 == s2) ? s3->table : s2->table ) ;
	set_assign( s1, tmp ) ;
	set_free( tmp ) ;
	return( s1 ) ;
    }
}

set set_difference(set s1, set s2, set s3)
{
    set_assign( s1, s2 ) ;
    HASH_MAP( k, ignore, {hash_del( s1->table, k );}, s3->table ) ;
    return( s1 ) ;
}

set set_del_element(set s1, set s2, void * e)
{
    set_assign( s1, s2 ) ;
    hash_del( s1->table, e );
    return( s1 ) ;
}

bool set_equal(set s1, set s2)
{
    bool equal ;
    
    equal = TRUE ;
    HASH_MAP( k, ignore, {
	if( hash_get( s2->table, k ) == HASH_UNDEFINED_VALUE ) 
		return( FALSE );
    }, s1->table ) ;
    HASH_MAP( k, ignore, {
	if( hash_get( s1->table, k ) == HASH_UNDEFINED_VALUE )
		return( FALSE );
    }, s2->table ) ;
    return( equal ) ;
}

void set_clear(set s)
{
    hash_table_clear( s->table ) ;
}

void set_free(set s)
{
    hash_table_free(s->table);
    gen_free_area((void**) s, sizeof(set_chunk));
}

bool 
set_empty_p(set s)
{
    SET_MAP(x, return FALSE, s);
    return TRUE;
}

void
gen_set_closure_iterate(
    void (*iterate)(void *, set),
    set initial,
    bool dont_iterate_twice)
{
    set curr, next, seen;
    set_type t = initial->type;

    seen = set_make(t);
    curr = set_make(t);
    next = set_make(t);

    set_assign(curr, initial);

    while (!set_empty_p(curr))
    {
	SET_MAP(x, iterate(x, next), curr);
	if (dont_iterate_twice)
	{
	    (void) set_union(seen, seen, curr);
	    set_difference(curr, next, seen);
	}
	else
	{
	    set_assign(curr, next);
	}
	set_clear(next);
    }

    set_free(curr);
    set_free(seen);
    set_free(next);

}

/* a set-based implementation of gen_closure
 * that does not go twice in the same object.
 * FC 27/10/95.
 */
void gen_set_closure(
    void (*iterate)(void *, set),
    set initial)
{
    gen_set_closure_iterate(iterate, initial, TRUE);
}

int set_own_allocated_memory(set s)
{
    return sizeof(set_chunk)+hash_table_own_allocated_memory(s->table);
}
