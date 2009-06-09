/*

  $Id$

  Copyright 1989-2009 MINES ParisTech

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
 /* Predicate transformer package: sc complexity level
  */

#include <stdio.h>

#include "boolean.h"
#include "vecteur.h"
#include "contrainte.h"
#include "sc.h"

#include "genC.h"
#include "linear.h"
#include "ri.h"
#include "ri-util.h"

#include "misc.h"
/* #include "semantics.h" */

#include "transformer.h"

static Psysteme sc_identity(Psysteme sc)
{
  return sc;
}

/* e and f are assumed to be values, Type independent. */
transformer 
simple_equality_to_transformer(entity e, entity f, bool assignment)
{
  transformer tf = generic_equality_to_transformer(e, f, assignment, FALSE);

  return tf;
}

transformer 
simple_unary_minus_to_transformer(entity e, entity f)
{
  transformer tf = generic_equality_to_transformer(e, f, FALSE, TRUE);

  return tf;
}

transformer 
generic_equality_to_transformer(entity e, entity f, bool assignment, bool unary_minus_p)
{
  transformer tf = transformer_undefined;
  Pvecteur eq = vect_new((Variable) e, VALUE_ONE);
  cons * tf_args = NIL;
  Pcontrainte c;

  ifdebug(9) {
    pips_debug(9, "entity e: %s, entity f: %s, %s\n",
	       entity_local_name(e), entity_local_name(f),
	       assignment? "Is an assignment" : "Is not an assignment");
  }

  if(assignment)
    tf_args = CONS(ENTITY, e, NIL);

  vect_add_elem(&eq, (Variable) f, unary_minus_p? VALUE_ONE : VALUE_MONE);
  c = contrainte_make(eq);
  tf = make_transformer(tf_args,
			make_predicate(sc_make(c, CONTRAINTE_UNDEFINED)));

  ifdebug(9) {
    pips_debug(9, "end with tf=%p\n", tf);
    dump_transformer(tf);
  }

  return tf;
}

/* e and e1 and e2 are assumed to be values. State e=e1+e2 or e=e1-e2. Type independent. */
transformer 
simple_addition_to_transformer(entity e, entity e1, entity e2, bool addition_p)
{
  transformer tf = transformer_undefined;
  Pvecteur eq = vect_new((Variable) e, VALUE_ONE);
  Pcontrainte c;

  ifdebug(9) {
    pips_debug(9, "entity e: %s, entity e1: %s, entity e2: %s, %s\n",
	       entity_local_name(e), entity_local_name(e1), entity_local_name(e2),
	       addition_p? "Is an addition" : "Is a subtraction");
  }

  vect_add_elem(&eq, (Variable) e1, VALUE_MONE);
  vect_add_elem(&eq, (Variable) e2, addition_p? VALUE_MONE : VALUE_ONE);
  c = contrainte_make(eq);
  tf = make_transformer(NIL,
			make_predicate(sc_make(c, CONTRAINTE_UNDEFINED)));

  ifdebug(9) {
    pips_debug(9, "end with tf=%p\n", tf);
    dump_transformer(tf);
  }

  return tf;
}

/* e and f are assumed to be values. Operator op is overloaded and the
   result is operator and type dependent */
transformer 
relation_to_transformer(entity op, entity e1, entity e2, bool veracity)
{
  transformer tf = transformer_undefined;
  basic b1 = variable_basic(type_variable(entity_type(e1)));
  basic b2 = variable_basic(type_variable(entity_type(e2)));
  Pvecteur eq = VECTEUR_NUL;
  Pvecteur ineq = VECTEUR_NUL;
  Pcontrainte ceq = CONTRAINTE_UNDEFINED;
  Pcontrainte cineq = CONTRAINTE_UNDEFINED;

  ifdebug(9) {
    pips_debug(9, "Begin for entity e1: %s of basic %s, "
	       "entity e2: %s of basic %s and operator %s\n",
	       entity_local_name(e1), basic_to_string(b1),
	       entity_local_name(e2), basic_to_string(b2),
	       module_local_name(op));
  }

  /* Beware of type coercion... Fabien conjectures that values with
     incompatible types won't get mixed up... */

  if((ENTITY_EQUAL_P(op) && veracity)
     || (ENTITY_NON_EQUAL_P(op) && !veracity)) {
    /* type independent */
    eq = vect_new((Variable) e1, VALUE_ONE);
    vect_add_elem(&eq, (Variable) e2, VALUE_MONE);
  }
  else if((ENTITY_NON_EQUAL_P(op) && veracity)
	  || (ENTITY_EQUAL_P(op) && !veracity)) {
    /* Non convex information */
    ;
  }
  else if((ENTITY_LESS_OR_EQUAL_P(op) && veracity)
	  || (ENTITY_GREATER_THAN_P(op) && !veracity)) {
    ineq = vect_new((Variable) e1, VALUE_ONE);
    vect_add_elem(&ineq, (Variable) e2, VALUE_MONE);
  }
  else if((ENTITY_LESS_THAN_P(op) && veracity)
	  ||(ENTITY_GREATER_OR_EQUAL_P(op) && !veracity)) {
    ineq = vect_new((Variable) e1, VALUE_ONE);
    vect_add_elem(&ineq, (Variable) e2, VALUE_MONE);
    if(basic_int_p(b1) && basic_int_p(b2)) {
      vect_add_elem(&ineq, TCST, VALUE_ONE);
    }
  }
  else if((ENTITY_GREATER_OR_EQUAL_P(op) && veracity)
	  || (ENTITY_LESS_THAN_P(op) && !veracity)) {
    ineq = vect_new((Variable) e1, VALUE_MONE);
    vect_add_elem(&ineq, (Variable) e2, VALUE_ONE);
  }
  else if((ENTITY_GREATER_THAN_P(op) && veracity)
	  || (ENTITY_LESS_OR_EQUAL_P(op) && !veracity)) {
    ineq = vect_new((Variable) e1, VALUE_MONE);
    vect_add_elem(&ineq, (Variable) e2, VALUE_ONE);
    if(basic_int_p(b1) && basic_int_p(b2)) {
      vect_add_elem(&ineq, TCST, VALUE_ONE);
    }
  }
  else {
    pips_internal_error("Unexpected relational operator %s\n", entity_name(op));
  }

  ceq = VECTEUR_NUL_P(eq)? CONTRAINTE_UNDEFINED : contrainte_make(eq);
  cineq = VECTEUR_NUL_P(ineq)? CONTRAINTE_UNDEFINED : contrainte_make(ineq);

  if(ceq!=CONTRAINTE_UNDEFINED||cineq!=CONTRAINTE_UNDEFINED) {
    tf = make_transformer(NIL,
			  make_predicate(sc_make(ceq, cineq)));
  }

  ifdebug(9) {
    pips_debug(9, "end with tf=%p\n", tf);
    dump_transformer(tf);
  }

  return tf;
}

/* transformer transformer_combine(transformer t1, transformer t2):
 * compute the composition of transformers t1 and t2 (t1 then t2)
 *
 * t1 := t2 o t1
 * return t1
 *
 * t1 is updated, but t2 is preserved
 */
transformer transformer_combine(transformer t1, transformer t2)
{
  /* algorithm: 
     let a1 be t1 arguments, a2 be t2 arguments,
     let ints be the intersection of a1 and a2
     let r1 be t1 relation and r2 be a copy of t2 relation
     let a be a1 union a2
     rename entities in ints in r1 (new->int) and r2 (old->int)
     rename entities in a2-ints in r1 (new->old)
     build a system b with r1 and r2
     project b along ints
     build t1 with a and b
  */
  cons * a1 = transformer_arguments(t1);
  cons * a2 = transformer_arguments(t2);
  /* Newgen does not generate the proper castings */
  /* Automatic variables read in a CATCH block need to be declared volatile as
   * specified by the documentation*/
  Psysteme volatile r1 = (Psysteme) predicate_system(transformer_relation(t1));
  Psysteme r2 = sc_dup((Psysteme)predicate_system(transformer_relation(t2)));
  /* ints: list of intermediate value entities */
  cons * ints = NIL;
  /* local variable ce2: why don't you use MAPL :-)! */
  cons * ce2;

  pips_debug(8,"begin\n");

  pips_debug(8,"arg. t1=%p\n",t1);
  ifdebug(8) (void) dump_transformer(t1);
  /* The consistencies of transformers t1 and t2 cannot be checked with
     respect to the current environment because t1 or t2 may be relative
     to a callee as in user_function_call_to_transformer(). Hence a
     debug level of 10. */
  ifdebug(10) pips_assert("consistent t1", transformer_consistency_p(t1));

  pips_debug(8,"arg. t2=%p\n",t2);
  ifdebug(8) (void) dump_transformer(t2);
  ifdebug(10) pips_assert("consistent t2", transformer_consistency_p(t2));

  if(!sc_empty_p(r1)) {

    if(sc_empty_p(r2)) {
      t1 = empty_transformer(t1);
    }
    else { /* both t1 and t2 are not obviously unfeasible */

    /* build new argument list and rename old and intermediate values,
       as well as new (i.e. unmodified) variables in t1 */

    for(ce2 = a2; !ENDP(ce2); POP(ce2)) {
      entity e2 = ENTITY(CAR(ce2));
      if(entity_is_argument_p(e2, a1)) {
	/* renaming of intermediate values in r1 and r2 */
	entity e_int = entity_to_intermediate_value(e2);
	entity e_old = entity_to_old_value(e2);
	r1 = sc_variable_rename(r1, (Variable) e2, (Variable) e_int);
	r2 = sc_variable_rename(r2, (Variable) e_old, (Variable) e_int);
	ints = arguments_add_entity(ints, e_int);
      }
      else {
	/* if ever e2 is used as e2#new in r1 it must now be
	   replaced by e2#old */
	if(base_contains_variable_p(r1->base, (Variable) e2)) {
	  entity e_old = entity_to_old_value(e2);
	  r1 = sc_variable_rename(r1, (Variable) e2, (Variable) e_old);
	}
	/* e2 must be appended to a1 as new t1's arguments;
	   hopefully we are not iterating on a1; but 
	   entity_is_argument_p() receives a longer argument each time;
	   possible improvements? */
	a1 = gen_nconc(a1, CONS(ENTITY, e2, NIL));
      }
    }

    /* build global linear system: r1 is destroyed, r2 is preserved
     */
    r1 = sc_append(r1, r2);

    /* ??? The base returned may be empty... FC...
     * boumbadaboum in the projection later on.
     */
    sc_rm(r2);
    r2 = SC_UNDEFINED;
    ifdebug(9) {
      pips_debug(9, "global linear system r1 before projection\n");
      sc_fprint(stderr, r1, (char * (*)(Variable)) dump_value_name);
      sc_dump(r1);
    }
    
    /* get rid of intermediate values, if any.
     * ??? guard added to avoid an obscure bug, but I guess it should
     * never get here with en nil base... FC
     */
    if (sc_base(r1)) {
      MAP(ENTITY, e_temp,
      {
	if (sc_expensive_projection_p(r1,(Variable) e_temp)) {
	  ifdebug(9) {
	    pips_debug(9, "expensive projection on %s with\n",
		       entity_local_name(e_temp));
	    sc_fprint(stderr, r1, (char * (*)(Variable)) entity_local_name);
	  }
	  sc_elim_var(r1,(Variable) e_temp);
	  sc_base_remove_variable(r1,(Variable) e_temp);
	  ifdebug(9) {
	    pips_debug(9, "simplified tranformer\n");
	    sc_fprint(stderr, r1,(char * (*)(Variable)) entity_local_name);
	  }
	}
	else {	
	  CATCH(overflow_error) 
	    {
	      /* CA */
	      /*PIER: problem with e_temp that should be volatile because of
	      * the catch structure. Not easy make it volatile because of 
	      * the MAP MACRO */
	      pips_user_warning("overflow error in projection of %s, "
				"variable eliminated\n",
				entity_name(e_temp)); 
	      r1 = sc_elim_var(r1, (Variable) e_temp);
	    }
	  TRY 
	    {
	      sc_and_base_projection_along_variable_ofl_ctrl
		(&r1, (Variable) e_temp, NO_OFL_CTRL);
	      UNCATCH(overflow_error);
	    }
		
	  if (! sc_empty_p(r1)) {
	    r1 = sc_normalize2(r1);
	    if(SC_EMPTY_P(r1)) {
	      r1 = sc_empty(BASE_NULLE);
	      break;
	    }
	  }
	}
      },
	  ints);
    }

    ifdebug(9) {
      pips_debug(9, "global linear system r1 after projection\n");
      sc_fprint(stderr, r1, (char * (*)(Variable)) dump_value_name);
      sc_dump(r1);
    }

    /* get rid of ints */
    gen_free_list(ints);
    ints = NIL;

    /* update t1 */
    if(sc_empty_p(r1)) {
      /* No old values should be left in r1's basis. */
      MAP(ENTITY, v, {
	entity oldv = entity_to_old_value(v);
	if(base_contains_variable_p(sc_base(r1), (Variable) oldv))
	  sc_base_remove_variable(r1, (Variable) oldv);
      }, a1);
      free_arguments(a1);
      transformer_arguments(t1) = NIL;
    }
    else {
      transformer_arguments(t1) = a1;
    }
    predicate_system(transformer_relation(t1)) = r1;
    }
  }
     
  pips_debug(8,"res. t1=%p\n",t1);
  ifdebug(8) dump_transformer(t1);
  pips_debug(8,"end\n");

  return t1;
}

/* Transformer tf1 and tf2 are supposed to be independent but they may
   interfere, for instance because subexpressions have non-standard
   conformant side effects. tf12 is a newly allocated transformer with no
   sharing with tf1 or tf2 (theoretically). */
transformer transformer_safe_combine_with_warnings(transformer tf1, transformer tf2)
{
  transformer tf12 = transformer_undefined;

  /* Intersection is not powerful enough to cope with side effects. But
     side effects can be dealt with only if the operation order is known
     when the standard is violated. We assume here a right to left evaluation. */
  if(transformer_safe_affect_transformer_p(tf1, tf2)) {
    pips_debug(9, "Side effects of tf2 on tf1\n");
    pips_user_warning("Non standard compliant code: side effect in part\n"
                      "of an expression affects variable(s) used in a later part\n");
    tf12 = transformer_combine(tf1, tf2);
  }
  else if (transformer_safe_affect_transformer_p(tf2, tf1)){
    pips_debug(9, "Side effects of tf2 on tf1\n");
    pips_user_warning("Non standard compliant code: side effect in part\n"
                      "of an expression affect variables used in an earlier part\n");
    tf12 = transformer_combine(tf1, tf2);
  }
  else {
    pips_debug(9, "No adversary side effects\n");
    if(transformer_undefined_p(tf1)
       || transformer_undefined_p(tf2)
       || (ENDP(transformer_arguments(tf1)) && ENDP(transformer_arguments(tf2)))) {
      /* No side effects at all */
      pips_debug(9, "No side effects at all\n");
      tf12 = transformer_safe_intersection(tf1, tf2);
      free_transformer(tf1);
    }
    else {
      pips_debug(9, "Side effects on other variables\n");
      tf12 = transformer_combine(tf1, tf2);
    }
  }
  return tf12;
}

/* Allocate a new transformer with constraints in t1 and t2.
 *
 * If t2 has no arguments, it restrains the domain of t1. This
 * is necessary if image_only is true.
 *
 * If not, the two transformers are supposed to be two separate
 * abstraction of the same transformation and an intersection
 * of the relation graphs is performed.
 */
static transformer transformer_general_intersection(transformer t1,
						    transformer t2,
						    bool image_only)
{
  transformer t = transformer_undefined;

  pips_debug(9, "Begins with t1 = %p and t2 = %p, image_only=%s\n",
	     t1, t2, bool_to_string(image_only));


  if(transformer_empty_p(t1)||transformer_empty_p(t2)) {
    t = transformer_empty();
  }
  else {
    Psysteme s1 = sc_dup((Psysteme) predicate_system(transformer_relation(t1)));
    Psysteme s2 = sc_dup((Psysteme) predicate_system(transformer_relation(t2)));

    t = transformer_identity();
    /*
      pips_debug(9, "begin with s1 and s2:\n");
      sc_dump(s1);
      sc_dump(s2);
    */

    s1 = sc_append(s1, s2);

    /*
      pips_debug(9, "new s1:\n");
      sc_dump(s1);
    */

    predicate_system(transformer_relation(t)) = s1;

    if(image_only) {
      /* Do not restrict the transition but the image of the relation t1
	 with constraints in t2. */
      if(!ENDP(transformer_arguments(t2))) {
	dump_transformer(t2);
	pips_assert("Transformer t2 has no arguments", ENDP(transformer_arguments(t2)));
      }
      transformer_arguments(t) = dup_arguments(transformer_arguments(t1));
    }
    else {
      /* intersect transition t1 and transition t2 */
      transformer_arguments(t) = arguments_intersection(transformer_arguments(t1),
							transformer_arguments(t2));
    }
  }

  pips_debug(9, "Exit with t=%p, for t1 = %p and t2 = %p\n", t, t1, t2);

  return t;
}

/* tf receives the constraints in t1 and t2. For implicit equalities
   carried by args, this implies that the args for tf is the intersection
   of the args. And that the resulting transformer may be empty: for
   instance, a variable may be untouched in t1 and incremented in t2,
   which is impossible. */
transformer transformer_intersection(transformer t1, transformer t2)
{
  transformer t = transformer_general_intersection(t1, t2, FALSE);
  return t;
}
transformer transformer_image_intersection(transformer t1, transformer t2)
{
  transformer t = transformer_undefined;

  pips_debug(9, "begins with t1 = %p and t2 = %p\n", t1, t2);

  t = transformer_general_intersection(t1, t2, TRUE);

  pips_debug(9, "ends with t = %p, t1 = %p and t2 = %p\n", t, t1, t2);

  return t;
}

static transformer transformer_safe_general_intersection(transformer t1,
							 transformer t2,
							 bool image_only)
{
  transformer tf = transformer_undefined;

  if(transformer_undefined_p(t1))
    tf = copy_transformer(t2);
  else if(transformer_undefined_p(t2))
    tf = copy_transformer(t1);
  else
    tf = transformer_general_intersection(t1, t2, image_only);

  return tf;
}

transformer transformer_safe_intersection(transformer t1, transformer t2)
{
  transformer tf = transformer_safe_general_intersection(t1, t2, FALSE);

  return tf;
}

transformer transformer_safe_image_intersection(transformer t1, transformer t2)
{
  transformer tf = transformer_safe_general_intersection(t1, t2, TRUE);

  return tf;
}

/* Restrict the domain of the relation tf with pre. pre is assumed to be
 restricted to a store predicate: its argument list must be empty.

 For a restriction on the image of tf, see transformer_image_intersection

 tf is updated by side effect although transformer_image_intersection()
allocates a fresh new transformer.  */
transformer transformer_domain_intersection(transformer tf,
					    transformer pre)
{
  transformer dom = transformer_dup(pre);
  transformer tf_inter = transformer_undefined;

  pips_assert("pre does not involve old values and has no arguments",
	      ENDP(transformer_arguments(pre)));

  pips_debug(9, "Begin with tf=%p and pre=%p\n", tf, pre);

  /* if a value in pre is modified by tf, it must be renamed as an old
     value */
  MAP(ENTITY, a, {
    entity na = entity_to_new_value(a);
    entity oa = entity_to_old_value(a);
    Psysteme pre_sc = predicate_system(transformer_relation(dom));
    pre_sc = sc_variable_rename(pre_sc, (Variable) na, (Variable) oa);
  }, transformer_arguments(tf));

  /* transformer dom is not consistent since it references old values but
     has no arguments */

  tf_inter = transformer_image_intersection(tf, dom);
  tf = move_transformer(tf, tf_inter);
  free_transformer(dom);

  return tf;
}

/* If tf and pre are defined, update tf.
 * If tf is defined and pre is undefined, return tf unchanged.
 * If tf and pre are undefined,  return tf unchanged.
 * If tf is undefined and pre is defined, we could exploit pre or return undefined.
 *.*/
transformer transformer_safe_domain_intersection(transformer tf,
						 transformer pre)
{
  if(!transformer_undefined_p(pre)) {
    if(transformer_undefined_p(tf)) {
      tf = transformer_domain_intersection(transformer_identity(), pre);
    }
    else {
      tf = transformer_domain_intersection(tf, pre);
    }
  }
    
  return tf;
}

/* Return the range of relation tf in a newly allocated transformer.
 * Projection of all old values.
 */
transformer transformer_range(transformer tf)
{
  transformer rtf = transformer_dup(tf);
  list args = NIL;
  Psysteme sc = predicate_system(transformer_relation(rtf));
  Pbase b = sc_base(sc);

  MAP(ENTITY, a, {
    entity ov = entity_to_old_value(a);

    /* A variable may be modified but its old value does not have to
       appear in the basis. Although the opposite is wrong. */
    if(base_contains_variable_p(b, (Variable)  ov)) {
      args = CONS(ENTITY, ov, args);
    }
  }, transformer_arguments(rtf));

  /* rtf = transformer_projection(rtf, args); */
  rtf = transformer_projection_with_redundancy_elimination(rtf, args,
							   sc_identity);

  gen_free_list(args);

  gen_free_list(transformer_arguments(rtf));
  transformer_arguments(rtf) = NIL;

  return rtf;
}
transformer transformer_safe_range(transformer tf)
{
  transformer rtf = transformer_undefined;

  if(!transformer_undefined_p(tf)) {
    rtf = transformer_range(tf);
  }
  return rtf;
}

/* Return the domain of relation tf in a newly allocated transformer.
 * Projection of all new values of modified variables. Renaming of old
 * values as new values. The transformer returned is a predicate on the
 * input state (i.e. not really a transformer).
 *
 */
transformer transformer_to_domain(transformer tf)
{
  transformer dtf = transformer_dup(tf);
  list new_args = NIL;
  Psysteme sc = predicate_system(transformer_relation(dtf));
  Pbase b = sc_base(sc);

  MAP(ENTITY, a, {
    entity nv = entity_to_new_value(a);

    if(base_contains_variable_p(b, (Variable)  nv)) {
      new_args = CONS(ENTITY, nv, new_args);
    }
  }, transformer_arguments(dtf));

  /* dtf = transformer_projection(dtf, args); */
  /* dtf = transformer_projection_with_redundancy_elimination(dtf, new_args,
							   sc_identity); */
  /* The resulting transformer is going to be inconsistent because old
     values appear although the argument list is empty. */
  dtf = transformer_projection_without_check(dtf, new_args,
					     sc_identity);

  /* Careful, sc and b have been updated by the projections */
  sc = predicate_system(transformer_relation(dtf));
  b = sc_base(sc);

  MAP(ENTITY, a, {
    entity ov = entity_to_old_value(a);
    entity nv = entity_to_new_value(a);

    /* A variable may be modified but its old value does not have to
       appear in the basis. Although the opposite is wrong. */
    if(base_contains_variable_p(b, (Variable)  ov)) {
      dtf = transformer_value_substitute(dtf, ov, nv);
    }
  }, new_args);

  gen_free_list(new_args);

  return dtf;
}
transformer transformer_safe_domain(transformer tf)
{
  transformer dtf = transformer_undefined;

  if(!transformer_undefined_p(tf)) {
    dtf = transformer_to_domain(tf);
  }
  return dtf;
}

/* Restrict the range of relation tf with the range r.
 *
 * As a range, r is assumed to have no arguments.
 */
transformer transformer_range_intersection(transformer tf, transformer r)
{
  pips_assert("r does not involve old values and has no arguments",
	      ENDP(transformer_arguments(r)));
  tf = transformer_image_intersection(tf, r);

  return tf;
}

static int varval_value_name_is_inferior_p(Pvecteur * pvarval1, Pvecteur * pvarval2)
{
    int is_inferior = TRUE;
    string s1 = generic_value_name((entity) vecteur_var(*pvarval1));
    string s2 = generic_value_name((entity) vecteur_var(*pvarval2));
    
    is_inferior = (strcmp(s1, s2) > 0 );

    return is_inferior; 
}

/* Eliminate (some) rational or integer redundancy. 

   Remember that integer
   redundancy elimination may degrade results because some transformer
   operator such as convex hull use a rational interpretation of the
   constraints.

   Does not take into account value types. So s=="hello" and
   s=="world" do not result into an empty transformer.
 */
transformer
transformer_normalize(transformer t, int level)
{
  /* Automatic variables read in a CATCH block need to be declared volatile as
   * specified by the documentation*/
  Psysteme volatile r = (Psysteme) predicate_system(transformer_relation(t));

  ifdebug(1) {
    pips_assert("Transformer t is consistent on entrance",
		transformer_consistency_p(t));
  }

  if (!sc_empty_p(r)) {
    Pbase b = base_dup(sc_base(r));
    /* Automatic variables read in a CATCH block need to be declared volatile as
     * specified by the documentation*/
    Psysteme r2 = sc_dup(r);
    /* Select one tradeoff between speed and accuracy:
     * enumerated by increasing speeds according to Beatrice
     */
    
    CATCH(overflow_error) 
      {
	/* CA */
	pips_user_warning("overflow error in  redundancy elimination\n");
	sc_rm(r);
	r = r2;
      }
    TRY 
      {
	switch(level) {
	  
	case 0:
	  /* Our best choice for accuracy, but damned slow on ocean */
	  r = sc_safe_elim_redund(r);
	  break;
		    
	case 1:
	  /* Beatrice's best choice: does not deal with minmax2 (only)
	   * but still requires 74 minutes of real time
	   * (55 minutes of CPU time) for ocean preconditions, 
	   * when applied to each precondition stored.
	   *
	   * Only 64 s for ocean, if preconditions are not normalized.
	   * But andne, callabsval, dead2, hind, negand, negand2, or,
	   * validation_dead_code are not validated any more. Redundancy
	   * could always be detected in a trivial way after propagating
	   * values from equations into inequalities.
	   */
	  sc_nredund(&r);
	  predicate_system(transformer_relation(t)) = r;
	  break;
	  
	case 2:
	  /* Francois' own: does most of the easy stuff.
	   * Fails on mimax2 and sum_prec, but it is somehow
	   * more user-friendly because trivial preconditions are
	   * not destroyed as redundant. It makes you feel safer.
	   *
	   * Result for full precondition normalization on ocean: 114 s
	   * for preconditions, 4 minutes between split ocean.f and
	   * OCEAN.prec
	   */
	  r = sc_strong_normalize(r);
	  break;
	  
	case 5:
	  /* Same plus a good feasibility test
	   */
	  r = sc_strong_normalize3(r);
	  break;
	  
	case 3:
	  /* Similar, but variable are actually substituted
	   * which is sometimes painful when a complex equations
	   * is used to replace a simple variable in a simple
	   * inequality.
	   */
	  r = sc_strong_normalize2(r);
	  break;
	case 6:
	  /* Similar, but variables are substituted if they belong to
	   * a more or less simple equation, and simpler equations
	   * are processed first and a lexicographically minimal
	   * variable is chosen when equivalent variables are
	   * available.
	   */
	  r = sc_strong_normalize4(r, 
				   (char * (*)(Variable)) external_value_name);
	  break;
	  
	case 7:
	  /* Same plus a good feasibility test, plus variable selection 
	   * for elimination, plus equation selection for elimination
	   */
	  r = sc_strong_normalize5(r, 
				   (char * (*)(Variable)) external_value_name);
	  break;
	  
	case 4:
	  vect_sort_in_place(&sc_base(r), varval_value_name_is_inferior_p);
	  r = sc_normalize2(r);
	  break;
	  
	case 8:
	  /* Very expensive: the system is rebuilt by adding constraints
	   * one by one
	   */
	  sc_safe_build_sc_nredund_1pass(&r);
	  break;
	  
	default:
	  pips_internal_error("unknown level %d\n", level);
	}

	sc_rm(r2), r2 = NULL;
	UNCATCH(overflow_error);
      } /* end of TRY */
			
    if (SC_EMPTY_P(r)) {
      r = sc_empty(BASE_NULLE);
    }
    else 
      base_rm(b), b=BASE_NULLE;
    
    r->dimension = vect_size(r->base);

    if(sc_empty_p(r)) {
      free_arguments(transformer_arguments(t));
      transformer_arguments(t) = NIL;
    }

    predicate_system(transformer_relation(t)) = r;
  }

  ifdebug(8) {
    fprintf(stderr, "After normalization of transformer t=%p at level %d:\n",
	    t, level);
    fprint_transformer(stderr, t, (get_variable_name_t) entity_local_name);
  }

  ifdebug(1) {
    pips_assert("Transformer t is consistent on exit",
		transformer_consistency_p(t));
  }

  return t;
}

transformer transformer_safe_normalize(transformer t, int level)
{
  if(!transformer_undefined_p(t)) {
    t = transformer_normalize(t, level);
  }
  return t;
}

transformer transformer_temporary_value_projection(transformer tf)
{
  list tv = NIL;

  if(number_of_temporary_values()>0) {
    Psysteme r = (Psysteme) predicate_system(transformer_relation(tf));
    Pbase b = BASE_NULLE;

    for(b = sc_base(r); !BASE_NULLE_P(b); b = vecteur_succ(b)) {
      entity e = (entity) vecteur_var(b);
      if(local_temporary_value_entity_p(e)) {
	tv = CONS(ENTITY, e, tv);
      }
    }
    /* tf = transformer_projection(tf, tv); */
    /* tf = transformer_projection_with_redundancy_elimination(tf, tv, sc_identity); */
    tf = transformer_projection_with_redundancy_elimination(tf, tv, sc_safe_normalize);
  }
  else ifdebug(1) {
    Psysteme r = (Psysteme) predicate_system(transformer_relation(tf));
    Pbase b = BASE_NULLE;

    for(b = sc_base(r); !BASE_NULLE_P(b); b = vecteur_succ(b)) {
      entity e = (entity) vecteur_var(b);
      if(local_temporary_value_entity_p(e)) {
	tv = CONS(ENTITY, e, tv);
      }
    }
    pips_assert("No temporary values exist in the system since a reset "
		"counter for them has been performed\n", ENDP(tv));
  }
  

  gen_free_list(tv);

  return tf;
}

/* t may be undefined, args may contain values unrelated to t */
transformer safe_transformer_projection(transformer t, list args)
{
  transformer nt = transformer_undefined;
  if(!transformer_undefined_p(t)) {
    Psysteme r = (Psysteme) predicate_system(transformer_relation(t));
    list nargs = NIL;

    /* keep only values of args related to the transformer t */
    MAP(ENTITY, v, {
      if(base_contains_variable_p(sc_base(r), (Variable) v)) {
	nargs = gen_nconc(nargs, CONS(ENTITY, v, NIL));
      }
    }, args);

    nt = transformer_projection(t, nargs);
    gen_free_list(nargs);
  }
  return nt;
}

transformer 
transformer_formal_parameter_projection(transformer t)
{
  Psysteme sc = predicate_system(transformer_relation(t));
  Pbase b = sc_base(sc);
  Pbase cd = BASE_UNDEFINED;
  list fpl = NIL;

  for(cd = b; !BASE_NULLE_P(cd); cd = vecteur_succ(cd)) {
    entity val = (entity) vecteur_var(cd);
    entity var = value_to_variable(val);
    storage s = entity_storage(var);

    if(storage_formal_p(s))
      fpl = CONS(ENTITY, var, fpl);
  }

  t = transformer_projection(t, fpl);

  gen_free_list(fpl);

  return t;
}

/* values in args must be in t's base */
/* transformer transformer_projection(transformer t, cons * args):
 * projection of t along the hyperplane defined by values in args;
 * this generate a projection and not a cylinder based on the projection
 *
 * use the most complex/complete redundancy elimination in Linear
 *
 * args is not modified. t is modified by side effects.
 */
transformer 
transformer_projection(transformer t, list args)
{
  /* sc_safe_elim_redund() may increase the rational generating system */
  /* t = transformer_projection_with_redundancy_elimination(t, args,
     sc_safe_elim_redund); */
  t = transformer_projection_with_redundancy_elimination(t, args,
							 sc_safe_normalize);
  return t;
}

/* transformer transformer_projection(transformer t);
 * projection of t along the hyperplane defined by values of variables in arguments;
 * this generate a projection and not a cylinder based on the projection
 *
 * use the most complex/complete redundancy elimination in Linear
 *
 * args is not modified. t is modified by side effects.
 */
transformer 
transformer_arguments_projection(transformer t)
{
  list args = NIL;
  Psysteme sc = predicate_system(transformer_relation(t));
  Pbase b = sc_base(sc);

  MAP(ENTITY, a, {
    entity ov = entity_to_old_value(a);
    entity nv = entity_to_new_value(a);

    if(base_contains_variable_p(b, (Variable) ov))
      args = CONS(ENTITY, ov, args);
    if(base_contains_variable_p(b, (Variable)  nv))
      args = CONS(ENTITY, nv, args);
  }, transformer_arguments(t));

  t = transformer_projection(t, args);
  gen_free_list(args);
  return t;
}

Psysteme 
no_elim(Psysteme ps)
{
    return ps;
}

/* It is not clear if this function projects values or variables. If
   variables were projected, all values associated to a variable should
   also be projected. If values are projected and the transformer argument
   updated using args, old values should not be left in the basis when a
   new value is projected and its associated variable removed from tthe
   argument. 

   New values are identical to variables which makes it confusing.

   The implementation, and the signature, are aware of the nature of the
   underlying predicate.  */
transformer transformer_projection_with_redundancy_elimination(
    transformer t,
    list args,
    Psysteme (*elim)(Psysteme))
{
  return transformer_projection_with_redundancy_elimination_and_check
    (t, args, elim, TRUE);
}

/* In some cases, you know the projection will result in a non-consistent
   transformer that will be fixed later. The input transformer is
   nevertheless expected weakly consistent. */
transformer transformer_projection_without_check(
    transformer t,
    list args,
    Psysteme (*elim)(Psysteme))
{
  return transformer_projection_with_redundancy_elimination_and_check
    (t, args, elim, FALSE);
}

transformer transformer_projection_with_redundancy_elimination_and_check(
    transformer t,
    list args,
    Psysteme (*elim)(Psysteme),
    bool check_consistency_p)
{
  /* Library Linear/sc contains several reundancy elimination functions:
   *  sc_elim_redund()
   *  build_sc_nredund_2pass_ofl_ctrl() --- if it had the same profile...
   *  ...
   * no_elim() is provided here to obtain the fastest possible projection
   */
  list new_args = NIL;
  /* Automatic variables read in a CATCH block need to be declared volatile as
   * specified by the documentation*/
  Psysteme volatile r = (Psysteme) predicate_system(transformer_relation(t));
  extern string entity_global_name(entity); /* useless with ri-util.h */

  ifdebug(9) {
    pips_debug(9, "Begin for transformer %p\n", t);
    /* sc_fprint(stderr, r, exernal_value_name); */
    /* sc_fprint(stderr, r, (char * (*)(Variable)) entity_local_name); */
    fprint_transformer(stderr, t, (get_variable_name_t) entity_global_name);
    pips_debug(9, "and entities to be projected: ");
    print_arguments(args);
    pips_assert("t is weakly consistent", transformer_weak_consistency_p(t));
  }

  /* A side effect of transformer_empty_p() is to normalize the transformer. */
  if(transformer_empty_p(t)) {
    empty_transformer(t);
  }
  else if(!ENDP(args)) {
    list cea;

    /* Step 1: get rid of unwanted values in the relation r and in the basis */
    for (cea = args ; !ENDP(cea); POP(cea)) {
      /* Automatic variables read in a CATCH block need to be declared volatile as
       * specified by the documentation*/
      entity volatile e = ENTITY(CAR(cea));
      pips_assert("base contains variable to project...",
		  base_contains_variable_p(sc_base(r), (Variable) e));
 
      pips_debug(9, "Projection of %s\n", entity_name(e));

      CATCH(overflow_error) 
	{
	  /* FC */
	  pips_user_warning("overflow error in projection of %s, "
			    "variable eliminated\n",
			    entity_name(e)); 
	  r = sc_elim_var(r, (Variable) e);
	}
      TRY 
	{
	  /* sc_projection_along_variable_ofl_ctrl_timeout_ctrl */
	  sc_projection_along_variable_ofl_ctrl
	    (&r,(Variable) e, NO_OFL_CTRL);
	  UNCATCH(overflow_error);
	}

      sc_base_remove_variable(r,(Variable) e);
	 
      /* could eliminate redundancy at each projection stage to avoid
       * explosion of the constraint number...  however it is pretty
       * expensive to do so. But we explode with NPRIO in FPPP (Spec
       * CFP'95 benchmark). A heuristic could apply redundacy elimination
       * from time to time?
       *
       */

      if(TRUE) {
	// if (!sc_empty_p(r)) {
	// Pbase b = base_dup(sc_base(r));

	r = elim(r);
	/* if (SC_EMPTY_P(r)) {
	   r = sc_empty(BASE_NULLE);
	   sc_base_remove_variable(r,(Variable) e);
	   }
	   else base_rm(b);
	   }*/

      }

      ifdebug(9) {
	pips_debug(9, "System after projection of %s\n", entity_name(e));
	/* sc_fprint(stderr, r, exernal_value_name); */
	sc_fprint(stderr, r, (char * (*)(Variable)) entity_global_name);
      }
    }

    /* Step 2: eliminate redundancy only/again once projections have all
     * been performed because redundancy elimination is
     * expensive and because most variables are exactly 
     * projected because they appear in at least one equation
     */
    if (!sc_empty_p(r)) {
      Pbase b = base_dup(sc_base(r));
      r = elim(r);
      if (SC_EMPTY_P(r)) {
	/* Should we use b or not? It does make some mathematical sense
	   but it is not compatible with the argument list which should
	   not be empty if old values appear in the basis. And the basis
	   should not be used, even in convex hulls if the emptiness is
	   detected first. */
	r = sc_empty(BASE_NULLE);
	base_rm(b);
      }
      else {
	base_rm(b);
      }
    }
    else {
      /* get rid of a useless basis */
      base_rm(sc_base(r));
      sc_base(r) = BASE_NULLE;
    }

    r->dimension = vect_size(r->base);

    ifdebug(9) {
      pips_debug(9, "System after redundancy elimination\n");
      /* sc_fprint(stderr, r, exernal_value_name); */
      sc_fprint(stderr, r, (char * (*)(Variable)) entity_global_name);
    }

    /* Step 3: compute new_args, but beware of left over old values! */
    FOREACH(ENTITY, e, 	transformer_arguments(t))
    { 
      if(!local_temporary_value_entity_p(e)) {
	entity v = value_to_variable(e);

	if((entity) gen_find_eq(e, args) == (entity) chunk_undefined)
	  {
	    /* e must be kept if it is not in args */
	    new_args = arguments_add_entity(new_args, e);
	  }
	else {
	  /* The variable is going to be dropped from the argument list */
	  entity old_e = entity_undefined;

	  if(entity_has_values_p(v)) {
	    old_e = entity_to_old_value(v);
	  }
	  else {
	    /* Must be a variable from a module which is not the current module */
	    old_e = global_new_value_to_global_old_value(v);
	  }

	  if(check_consistency_p
	     && base_contains_variable_p(sc_base(r), (Variable) old_e)) {
	    fprintf(stderr, "Value %s should have been eliminated earlier\n",
		    entity_name(old_e));
	    fprint_transformer(stderr, t, (get_variable_name_t) entity_global_name);
	    pips_internal_error("Wrong set of projected variables\n");
	  }
	}
      }
    }

    /* Step 4: update the relation and the arguments field for t */

    /* the relation is updated by side effect FI ?
     * Maybe not if SC_EMPTY(r) 1 Feb. 94 */
    predicate_system_(transformer_relation(t)) = newgen_Psysteme(r);

    /* replace the old arguments by the new one */
    gen_free_list(transformer_arguments(t));
    transformer_arguments(t) = new_args;
  }

  ifdebug(9) {
    pips_debug(9, "Transformer after argument list update\n");
    /* sc_fprint(stderr, r, exernal_value_name); */
    fprint_transformer(stderr, t, (get_variable_name_t) entity_global_name);
  }

  ifdebug(1) {
    /* Weak, because return value may still be present for functions. */
    if(check_consistency_p)
      pips_assert("After projection and redundancy elimination,"
		  " transformer t is consistent",
		  transformer_weak_consistency_p(t));
  }
  pips_debug(9, "End for t=%p\n", t);

  return t;
}

/* transformer transformer_apply(transformer tf, transformer pre):
 * apply transformer tf on precondition pre to obtain postcondition post
 *
 * post = tf(pre) = pre o tf
 *
 * There is (should be!) no sharing between pre and tf. No sharing is
 * introduced between pre or tf and post. Neither pre nor tf are modified.
 */
transformer transformer_apply(transformer tf, transformer pre)
{
    transformer post;
    transformer copy_pre;

    pips_debug(8,"begin\n");
    pips_assert("tf is not undefined", tf!=transformer_undefined);
    pips_debug(8,"tf=%p\n", tf);
    ifdebug(8) (void) dump_transformer(tf);
    pips_assert("pre is not undefined", pre!=transformer_undefined);
    pips_debug(8,"pre=%p\n", pre);
    ifdebug(8) (void) dump_transformer(pre);

    /* post = tf o pre ; pre would be modified by transformer_combine */
    copy_pre = transformer_dup(pre);
    post = transformer_combine(copy_pre, tf);

    pips_assert("post is not undefined", post!=transformer_undefined);
    pips_debug(8,"post=%p\n", post);
    ifdebug(8) (void) dump_transformer(post);
    pips_assert("unexpected sharing",post != pre);
    pips_debug(8,"end\n");

    return post;
}

transformer transformer_safe_apply(transformer tf, transformer pre)
{
  transformer post = transformer_undefined;

  if(!transformer_undefined_p(tf) && !transformer_undefined_p(pre))
    post = transformer_apply(tf, pre);

  return post;
}

/* transformer transformer_inverse_apply(transformer tf, transformer post):
 * apply transformer tf on precondition pre to obtain postcondition post
 *
 * pre = post(tf) = tf o post
 *
 * There is (should be!) no sharing between post and tf. No sharing is
 * introduced between post or tf and pre. Neither post nor tf are modified.
 */
transformer transformer_inverse_apply(transformer tf, transformer post)
{
    transformer pre = transformer_undefined;
    transformer copy_tf = transformer_dup(tf);

    pips_debug(8,"begin with\n");
    pips_assert("tf is not undefined", tf!=transformer_undefined);
    pips_debug(8,"tf=%p\n", tf);
    ifdebug(8) (void) dump_transformer(tf);
    pips_assert("post is not undefined", post!=transformer_undefined);
    pips_debug(8,"post=%p\n", post);
    ifdebug(8) {
      (void) dump_transformer(post);
      pips_assert("tf is consistent", transformer_consistency_p(tf));
      pips_assert("post is consistent", transformer_consistency_p(post));
    }

    /* pre = post o tf ; tf would be modified by transformer_combine */
    pre = transformer_combine(copy_tf, post);

    pips_assert("pre is not undefined", pre!=transformer_undefined);
    pips_debug(8,"return: pre=%p\n", pre);
    ifdebug(8) (void) dump_transformer(pre);
    pips_assert("unexpected sharing",post != pre);
    pips_debug(8,"end\n");

    return pre;
}

transformer transformer_safe_inverse_apply(transformer tf, transformer post)
{
  transformer pre = transformer_undefined;

  if(!transformer_undefined_p(tf) && !transformer_undefined_p(pre))
    pre = transformer_inverse_apply(tf, post);

  return pre;
}

/* transformer transformer_filter(transformer t, cons * args):
 * projection of t along the hyperplane defined by entities in args;
 * this generate a projection and not a cylinder based on the projection;
 *
 * if the relation associated to t is empty, t is not modified although
 * it should have a basis and this basis should be cleaned up. Since
 * no basis is carried in the current implementation of an empty system,
 * this cannot be performed (FI, 7/12/92).
 *
 * formal argument args is not modified. t is updated by side effect.
 *
 * Note: this function is almost equal to transformer_projection();
 * however, entities of args do not all have to appear in t's relation;
 * thus transformer_filter has a larger definition domain than 
 * transformer_projection; on transformer_projection's domain, both
 * functions are equal
 *
 * transformer_projection is useful to get cores when you know all entities
 * in args should appear in the relation.
 */
transformer 
transformer_filter(t, args)
transformer t;
cons * args;
{
  cons * new_args = NIL;
  /* Automatic variables read in a CATCH block need to be declared volatile as
   * specified by the documentation*/
  Psysteme volatile r = (Psysteme) predicate_system(transformer_relation(t));
  extern string entity_global_name(entity); /* useless with ri-util.h */

  ifdebug(9) {
    pips_debug(9, "Begin for transformer %p\n", t);
    /* sc_fprint(stderr, r, exernal_value_name); */
    /* sc_fprint(stderr, r, (char * (*)(Variable)) entity_local_name); */
    fprint_transformer(stderr, t, (get_variable_name_t) entity_global_name);
    pips_debug(9, "and entities to be projected: ");
    dump_arguments(args);
    pips_assert("t is weakly consistent", transformer_weak_consistency_p(t));
  }

  if(!ENDP(args) && !SC_EMPTY_P(r)) {
    /* get rid of unwanted values in the relation r and in the basis */
    list cea = list_undefined;

    for(cea=args; !ENDP(cea); POP(cea)) {
      /* Automatic variables read in a CATCH block need to be declared volatile as
       * specified by the documentation*/
      entity volatile e = ENTITY(CAR(cea));
      if(base_contains_variable_p(r->base, (Variable) e)) {
	/* r = sc_projection(r, (Variable) e); */
	/*
	  sc_projection_along_variable_ofl_ctrl(&r, (Variable) e,
	  NO_OFL_CTRL);  */
	CATCH(overflow_error) 
	  {				    
				/* CA */
	    pips_user_warning("overflow error in projection of %s, "
			      "variable eliminated\n",
			      entity_name(e)); 
	    r = sc_elim_var(r, (Variable) e);
	  }
	TRY 
	  {
	    /* sc_projection_along_variable_ofl_ctrl_timeout_ctrl */
 	    sc_projection_along_variable_ofl_ctrl
	      (&r, (Variable) e, NO_OFL_CTRL);
	    UNCATCH(overflow_error);
	  }
	/*       sc_projection_along_variable_ofl_ctrl(&r, (Variable) e,
		 OFL_CTRL);*/
	sc_base_remove_variable(r,(Variable) e);}
    }
    r->dimension = vect_size(r->base);
	
    /* compute new_args */
    /* use functions on arguments instead of in-lining !
       MAPL(ce, { entity e = ENTITY(CAR(ce));
       if((entity) gen_find_eq(e, args)== (entity) chunk_undefined) {
       -- e must be kept if it is not in args --
       new_args = arguments_add_entity(new_args, e);
       }},
       transformer_arguments(t));
    */
    new_args = arguments_difference(transformer_arguments(t), args);

    /* update the relation and the arguments field for t */

    /* Is the relation updated by side effect?
     * Yes, in general. No if the system is non feasible
     */

    predicate_system_(transformer_relation(t)) = newgen_Psysteme(r);

    /* replace the old arguments by the new one */
    free_arguments(transformer_arguments(t));
    transformer_arguments(t) = new_args;
  } 

  ifdebug(9) {
    pips_debug(9, "Transformer after argument list update\n");
    /* sc_fprint(stderr, r, exernal_value_name); */
    fprint_transformer(stderr, t, (get_variable_name_t) entity_global_name);
  }

  ifdebug(1) {
    pips_assert("After filtering,"
		" transformer t is consistent",
		transformer_weak_consistency_p(t));
  }
  pips_debug(9, "End for t=%p\n", t);

  return t;
}

/* bool transformer_affect_linear_p(transformer tf, Pvecteur l): returns TRUE
 * if there is a state s such that eval(l, s) != eval(l, tf(s));
 * returns FALSE if l is invariant w.r.t. tf, i.e. for all state s,
 * eval(l, s) == eval(l, tf(s))
 */
bool 
transformer_affect_linear_p(tf, l)
transformer tf;
Pvecteur l;
{
    if (!transformer_undefined_p(tf)){
	list args = transformer_arguments(tf);

	MAP(ENTITY, e, 
	{
	    Value v = vect_coeff((Variable) e, l);
	    if(value_notzero_p(v)) return TRUE;
	},
	    args);
    }

    return FALSE;
}

/* Transformer tf1 affects transformer tf2 if values modified by tf1
   appear in any constraint of tf2. The two transformer do not commute and
   tf1 o tf2 does not equal tf2 o tf1. */

bool 
transformer_affect_transformer_p(transformer tf1, transformer tf2)
{
  bool affect_p = FALSE;

  /* No need to check anything if tf1 does not change the memory state */
  if(!ENDP(transformer_arguments(tf1))) {
    Psysteme s2 = predicate_system(transformer_relation(tf2));
    Pcontrainte ceq = sc_egalites(s2);
    Pcontrainte cineq = sc_inegalites(s2);

    for(; !CONTRAINTE_UNDEFINED_P(ceq) && !affect_p; ceq = contrainte_succ(ceq)) {
      Pvecteur v = contrainte_vecteur(ceq);
      affect_p = transformer_affect_linear_p(tf1, v);
    }

    for(; !CONTRAINTE_UNDEFINED_P(cineq) && !affect_p; cineq = contrainte_succ(cineq)) {
      Pvecteur v = contrainte_vecteur(cineq);
      affect_p = transformer_affect_linear_p(tf1, v);
    }
  }

  return affect_p;
}

bool 
transformer_safe_affect_transformer_p(transformer tf1, transformer tf2)
{
  bool affect_p = FALSE;

  if(!transformer_undefined_p(tf1) && !transformer_undefined_p(tf2))
    affect_p = transformer_affect_transformer_p(tf1, tf2);

  return affect_p;
}

/* Generates a transformer abstracting a totally unknown modification of
 * the values associated to variables in list le.
 */
transformer 
args_to_transformer(le)
list le; /* list of entities */
{
    transformer tf = transformer_identity();
    cons * args = transformer_arguments(tf);
    Pbase b = VECTEUR_NUL;
    Psysteme s = sc_new();

    MAPL(ce, { 
      entity e = ENTITY(CAR(ce));
      entity new_val = entity_to_new_value(e);

      args = arguments_add_entity(args, new_val);
      b = vect_add_variable(b, (Variable) new_val);
      }, le);

    transformer_arguments(tf) = args;
    s->base = b;
    s->dimension = vect_size(b);
    predicate_system_(transformer_relation(tf)) = s;
    return tf;
}

/* transformer invariant_wrt_transformer(transformer p, transformer tf):
 * Assume that tf is a fix-point operator.
 *
 * Old version:
 * keep the invariant part of predicat p wrt tf in a VERY crude way;
 * old and new values related to an entity modified by tf are discarded
 * by projection, regardless of the way they are modified; information
 * that they are modified is preserved; in fact, this is *not* a projection
 * but a cylinder based on the projection.
 *                                                      inf
 * A real fix-point a la Halbwachs should be used p' = UNION(tf^k(p))
 *                                                      k=0
 * or simply one of PIPS loop fix-points.
 *
 * Be careful if tf is not feasible because the result is p itself which may not
 * be what you expect.
 *
 * p is not modified.
 */
transformer invariant_wrt_transformer(transformer p, transformer tf)
{
  transformer inv = transformer_undefined;
  transformer fptf = transformer_undefined;

  if(!transformer_undefined_p(p)) {
  if(FALSE)
  {
    fptf = args_to_transformer(transformer_arguments(tf));
  }
  else 
  {
    /* if it is expensive, maybe it should not be computed over and over...
     */
    fptf = transformer_derivative_fix_point(tf); 
  }

  inv = transformer_apply(fptf, p); /* tf? fptf? */

  transformer_free(fptf); /* must be freed, otherwise it is leaked. */
  }
  else {
    inv = transformer_undefined;
  }
  return inv;
}

/* transformer transformer_value_substitute(transformer t,
 *                                         entity e1, entity e2):
 * if e2 does not appear in t initially:
 *    replaces occurences of value e1 by value e2 in transformer t's arguments
 *    and relation fields; 
 * else
 *    error
 * fi
 *
 * "e2 must not appear in t initially": this is the general case; 
 * the second case may occur when procedure A calls B and C and when B and C 
 * share a global variable X which is not seen from A. A may contain 
 * relations between B:X and C:X...
 * See hidden.f in Bugs or Validation...
 */
transformer 
transformer_value_substitute(transformer t, entity e1, entity e2)
{
  /* updates are performed by side effects */

  cons * a = transformer_arguments(t);
  Psysteme s = (Psysteme) predicate_system(transformer_relation(t));

  pips_assert("e1 and e2 are defined entities",
	      e1 != entity_undefined && e2 != entity_undefined);
  /*
    pips_assert("transformer_value_substitute", 
    !base_contains_variable_p(s->base, (Variable) e2));
  */

  /* update only if necessary */
  if(base_contains_variable_p(s->base, (Variable) e1)) {

    if(!base_contains_variable_p(s->base, (Variable) e2)) {

      (void) sc_variable_rename(s,(Variable) e1, (Variable)e2);

      /* rename value e1 in argument a; e1 does not necessarily
	 appear in a because it's not necessarily the new value of
	 a modified variable */
      MAPL(ce, {entity e = ENTITY(CAR(ce));
      if( e == e1) ENTITY_(CAR(ce)) = e2;},
	   a);
    }
    else {
      pips_internal_error("cannot substitute e1=%s by e2=%s: e2 already in basis\n",
			  entity_name(e1), entity_name(e2));
    }
  }

  return t;
}

/* If e1 does not appear in t, it is substitutable. If e1 does appear in t but not e2, again it is substitutable. Else, it if not. */
bool transformer_value_substitutable_p(transformer t, entity e1, entity e2)
{
  bool substitutable_p = TRUE;
  Psysteme s = (Psysteme) predicate_system(transformer_relation(t));

  pips_assert("e1 and e2 are defined entities",
	      e1 != entity_undefined && e2 != entity_undefined);

  if(base_contains_variable_p(s->base, (Variable) e1)) {
    substitutable_p = !base_contains_variable_p(s->base, (Variable) e2);
  }

  return substitutable_p;
}

transformer 
transformer_safe_value_substitute(transformer t, entity e1, entity e2)
{
  if(!transformer_undefined_p(t))
    t = transformer_value_substitute(t, e1, e2);

  return t;
}

static bool constant_constraint_check(Pvecteur v, bool is_equation_p)
{
  string s1 = string_undefined;
  int i1 = 0;
  string s2 = string_undefined;
  int i2 = 0;
  double x = 0.;
  bool is_string = FALSE;
  bool is_float = FALSE;
  bool type_undecided_p = TRUE;
  Pvecteur cv = VECTEUR_UNDEFINED;
  bool is_checked = TRUE;
  int number_of_strings = 0;

  for(cv=v; !VECTEUR_UNDEFINED_P(cv); cv = vecteur_succ(cv)) {
    entity e = (entity) vecteur_var(cv);
    Value val = vecteur_val(cv);

    pips_assert("e is a constant", (Variable) e == TCST || entity_constant_p(e));

    if((Variable) e!=TCST) {
      basic b = constant_basic(e);

      switch(basic_tag(b)) {
      case is_basic_int:
      case is_basic_logical:
	/* Logical are represented by integer values*/
	pips_internal_error("Unexpected integer or logical type for constant %s\n",
			    entity_name(e));
	break;
      case is_basic_float:
	/* PIPS does not represent negative constants: call to unary_minus */
	if(type_undecided_p) {
	  type_undecided_p = FALSE;
	  is_float = TRUE;
	}
	else {
	  if(is_string) {
	    pips_internal_error("String constant mixed up with float constant %s\n",
				entity_name(e));
	  }
	}
	x = x + ((double) val) * float_constant_to_double(e);
	break;
      case is_basic_string:
	if(type_undecided_p) {
	  type_undecided_p = FALSE;
	  is_string = TRUE;
	}
	else {
	  if(is_float) {
	    pips_internal_error("Float constant mixed up with string constant %s\n",
				entity_name(e));
	  }
	}
	if(number_of_strings==0) {
	  /* s1 = module_local_name(e); */
	  s1 = entity_name(e);
	  i1 = (int) val;
	}
	else if(number_of_strings==1) {
	  /* s2 = module_local_name(e); */
	  s2 = entity_name(e);
	  i2 = (int) val;
	}
	else
	  pips_internal_error("Too many strings in a string constraint\n");

	number_of_strings++;
	break;
      case is_basic_complex:
	/* PIPS does not represent complex constants: call to CMPLX */
	pips_internal_error("Unexpected complex type for constant %s\n",
			    entity_name(e));
	break;
      case is_basic_overloaded:
	pips_internal_error("Unexpected overloaded type for constant %s\n",
			    entity_name(e));
	break;
      default:
	pips_internal_error("unknown basic b=%d\n", basic_tag(b));
      }
    }
    else {
      if(vect_size(v)==1) {
	/* Unfeasible equation or trivial inequality */
	is_checked = (is_equation_p ? FALSE : (val <= 0));
      }
      else {
	pips_internal_error("Unexpected integer constant mixed up with "
			    "non-integer constants\n");
      }
    }
  }

  pips_assert("It can't be a float and a string simultaneously",
	      !(is_float && is_string));

  if(is_string) {
    if(number_of_strings!=2)
      pips_internal_error("Illegal number of strings in string constraint\n");
    if(is_equation_p) {
      if(i1+i2==0)
	is_checked = (fortran_string_compare(s1, s2)==0);
      else
	pips_internal_error("Unexpected string coefficients i1=%d, i2=%d for equality\n",
			    i1, i2);
    }
    else {
      if(i1>0 && i1+i2==0)
	is_checked = (fortran_string_compare(s1, s2) <= 0);
      else if(i1<0 && i1+i2==0)
	is_checked = (fortran_string_compare(s1, s2) >= 0);
      else
	pips_internal_error("Unexpected string coefficients i1=%d, i2=%d for inequality\n",
			    i1, i2);
    }
  }
  else if(is_float) {
    if(is_equation_p)
      is_checked = (x==0.);
    else
      is_checked = (x<0.);
  }
    else {
      /* Must be a trivial integer constraint. Already processed*/
      ;
    }

  return is_checked;
}

/* If TRUE is returned, the transformer certainly is empty.
 * If FALSE is returned,
 * the transformer still might be empty, it all depends on the normalization
 * procedure power. Beware of its execution time!
 */
static bool 
parametric_transformer_empty_p(transformer t,
			       Psysteme (*normalize)(Psysteme,
						     char * (*)(Variable)))
{
  /* FI: the arguments seem to have no impact on the emptiness
   * (i.e. falseness) of t
   */
  predicate pred = transformer_relation(t);
  Psysteme ps = predicate_system(pred);
  bool empty_p = FALSE;
  bool consistent_p = TRUE;
  /* Automatic variables read in a CATCH block need to be declared volatile as
   * specified by the documentation*/
  Psysteme volatile new_ps = sc_dup (ps);

  pips_debug(9,"Begin for t=%p\n", t);

  /* empty_p = !sc_faisabilite(ps); */
  /* empty_p = !sc_rational_feasibility_ofl_ctrl(ps, OFL_CTRL, TRUE); */

  /* Normalize the transformer, use all "reasonnable" equations
   * to reduce the problem
   * size, check feasibility on the projected system
   */
  /* new_ps = normalize(new_ps, (char * (*)(Variable)) external_value_name); */
  /* FI: when dealing with interprocedural preconditions, the value mappings
   * are initialized for the caller but the convex hull, which calls this function,
   * must be performed in the calle value space.
   */
  new_ps = normalize(new_ps, (char * (*)(Variable)) entity_local_name);

  if(SC_EMPTY_P(new_ps)) {
    empty_p = TRUE;
  }
  else if(sc_empty_p(new_ps)) {
    /* Depending on the instance of "normalize", might always be trapped
       by the previous test. */
    empty_p = TRUE;
  }
  else {
    if(string_analyzed_p() || float_analyzed_p()) {
      /* Inconsistent equalities or inequalities between constants may be
         present. Project variable and temporary values and analyze resulting system. No
         equations should be left over unless equal constants are encoded
         differently. Inequations should be interpreted and checked. */
      Pvecteur b = VECTEUR_UNDEFINED;
      Pcontrainte eq = CONTRAINTE_UNDEFINED;
      Pcontrainte ineq = CONTRAINTE_UNDEFINED;
      Pbase b_min = sc_to_minimal_basis(new_ps);

      for (b = b_min ; !VECTEUR_UNDEFINED_P(b) && !empty_p; b = vecteur_succ(b)) {
	/* Automatic variables read in a CATCH block need to be declared volatile as
	 * specified by the documentation*/
	Variable volatile var = vecteur_var(b);
	entity volatile e_var = (entity) var;
 
	if(!entity_constant_p(e_var)) {

	  pips_debug(9, "Projection of %s\n", entity_name(e_var));

	  CATCH(overflow_error) 
	    {
	      /* FC */
	      pips_user_warning("overflow error in projection of %s, "
				"variable eliminated\n",
				entity_name(e_var)); 
	      new_ps = sc_elim_var(new_ps, var);
	    }
	  TRY 
	    {
	      /* sc_projection_along_variable_ofl_ctrl_timeout_ctrl */
	      sc_projection_along_variable_ofl_ctrl
		(&new_ps, var, NO_OFL_CTRL);
	      UNCATCH(overflow_error);
	    }

	  sc_base_remove_variable(new_ps, var);
	  /* No redundancy elimination... */
	  /* new_ps = elim(new_ps); */

	  ifdebug(10) {
	    pips_debug(10, "System after projection of %s\n", entity_name(e_var));
	    /* sc_fprint(stderr, r, exernal_value_name); */
	    sc_fprint(stderr, new_ps, (char * (*)(Variable)) value_full_name);
	  }
	  empty_p = sc_empty_p(new_ps);
	}
      }

      base_rm(b_min);

      /* Check remaining equalities and inequalities */

      ifdebug(9) {
	pips_debug(9, "System after all projections or emptiness detection:\n");
	/* sc_fprint(stderr, r, exernal_value_name); */
	sc_fprint(stderr, new_ps, (char * (*)(Variable)) value_full_name);
      }

      if(!empty_p) {
	for(eq = sc_egalites(new_ps);
	    !CONTRAINTE_UNDEFINED_P(eq) && consistent_p;
	    eq = contrainte_succ(eq)) {
	  consistent_p = constant_constraint_check(contrainte_vecteur(eq), TRUE);
	}

	for(ineq = sc_inegalites(new_ps);
	    !CONTRAINTE_UNDEFINED_P(ineq) && consistent_p;
	    ineq = contrainte_succ(ineq)) {
	  consistent_p = constant_constraint_check(contrainte_vecteur(ineq), FALSE);
	}
	empty_p = !consistent_p;
      }
    }
    else {
      empty_p = FALSE;
    }

    sc_rm(new_ps);
  }

  pips_debug(9,"End: %sfeasible\n", empty_p? "not " : "");

  return empty_p;
}

/* If TRUE is returned, the transformer certainly is empty.
 * If FALSE is returned,
 * the transformer still might be empty, but it's not too likely...
 */
bool 
transformer_empty_p(transformer t)
{
    bool empty_p = parametric_transformer_empty_p(t, sc_strong_normalize4);
    return empty_p;
}

/* If TRUE is returned, the transformer certainly is empty.
 * If FALSE is returned,
 * the transformer still might be empty, but it's not likely at all...
 */
bool 
transformer_strongly_empty_p(transformer t)
{
    bool empty_p = parametric_transformer_empty_p(t, sc_strong_normalize5);
    return empty_p;
}
