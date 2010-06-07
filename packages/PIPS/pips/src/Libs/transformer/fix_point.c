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
#ifdef HAVE_CONFIG_H
    #include "pips_config.h"
#endif
 /* transformer package - fix point computations
  *
  * Several algorithms are available:
  *
  *  - two untested versions using Halwachs approach; sc_elarg() was found to
  *    be very slow in an earlier implementation by Malik; experimentally,
  *    we did not find scientific Frotran programs that required such an
  *    algorithm.
  *
  *  - a fix point algorithm based on the transfer matrix linking the new values
  *    to the old values; this is sufficient for induction variables but only
  *    equations can be found.
  *
  *  - a fix point algorithm based on pattern matching which was developped
  *    experimentally for examples presented at FORMA; once the constant terms
  *    are eliminated using a loop counter, the constraints which are their
  *    own fixpoints are easy to pattern match; equations and inequalities
  *    can both be found.
  *
  *  - a fix point algorithm combining the last two algorithms above. This
  *    algorithm adds difference variables d_i = i#new - i#old for each variable
  *    i. All non d_i variables are thn projected. If the projection algorithm is
  *    sophisticated enough, the projection ordering will be "good". Left over
  *    inequalities are invariant or useless. Let N be the strictly positive iteration count:
  *
  *    sum ai di <= -k implies N sum ai di <= -Nk <= -k (OK)
  *
  *    sum ai di <= k implies N sum ai di <= Nk <= infinity (useless unless k == 0)
  *
  *    Left over equations can stay equations if the
  *    constant term is 0, or be degraded into an inequation if not. The nw inequation
  *    depends on the sign of the constant:
  *
  *    sum ai di == -k implies N sum ai di == -Nk <= -k
  *
  *    sum ai di == k implies N sum ai di == Nk >= k
  *
  *    sum ai di == 0 implies N sum ai di == 0
  *
  *    In a second step, the constant terms are eliminated to obtain new constraints
  *    leading to new invariants, using the same rules as above.
  *
  *    This algorithm was designed for while loops such as:
  *
  *    WHILE( I > 0 )
  *      J = J + I
  *      I = I - 1
  *
  *    to find as loop transformer T(I,J) {I#new <= I#old - 1, J#new >= J#old + 1,
  *                                        I#new+J#new >= I#old + J#old}
  *
  *    Note that the second constraint is redundant with the other two, but the last
  *    one cannot be found before the constant terms are eliminated.
  *
  * Francois Irigoin, 21 April 1990
  */

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "genC.h"
#include "linear.h"
#include "ri.h"
#include "ri-util.h"

#include "misc.h"

#include "boolean.h"
#include "vecteur.h"
#include "contrainte.h"
#include "sc.h"
#include "ray_dte.h"
#include "sommet.h"
#include "sg.h"
#include "polyedre.h"

#include "matrice.h"

#include "transformer.h"

/*
transformer 
transformer_fix_point(t1, t2)
transformer t1;
transformer t2;
{
    Psysteme r1;
    Psysteme r2;
    Psysteme r;
    transformer t = transformer_identity();

    debug(8,"transformer_fix_point","begin\n");

    pips_assert("transformer_fix_point",
		arguments_equal_p(transformer_arguments(t1),
				  transformer_arguments(t2)));
    transformer_arguments(t) = gen_copy_seq(transformer_arguments(t1));

    r1 = (Psysteme) predicate_system(transformer_relation(t1));
    r2 = (Psysteme) predicate_system(transformer_relation(t2));
    r = sc_elarg(r1, r2);

    predicate_system(transformer_relation(t)) = (char *) r;

    debug(8,"transformer_fix_point","end\n");

    return t;
}
*/

transformer 
transformer_halbwachs_fix_point(tf)
transformer tf;
{
    /* THIS FUNCTION WAS NEVER CORRECT AND HAS NOT BEEN REWRITTEN FOR
       THE NEW VALUE PACKAGE */

    /* simple fix-point for inductive variables: loop bounds are
       ignored */

    /* cons * args = effects_to_arguments(e); */
    /* tf1: transformer for one loop iteration */
    transformer tf1;
    /* tf2: transformer for two loop iterations */
    transformer tf2;
    /* tf12: transformer for one or two loop iterations */
    transformer tf12;
    /* tf23: transformer for two or three loop iterations */
    transformer tf23;
    /* tf_star: transformer for one, two, three,... loop iterations
       should be called tf_plus by I do not use the one_trip flag */
    transformer tf_star = transformer_undefined;

    pips_error("transformer_halbwachs_fix_point","not implemented\n");

    /* preserve transformer of loop body s */
    /* temporarily commented out */
    /*
      tf1 = transformer_rename(transformer_dup(tf), args);
      */
    tf1 = transformer_dup(tf);

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer",
		"tf1 after renaming =");
	(void) print_transformer(tf1);
    }

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer", "tf1 =");
	(void) print_transformer(tf1);
    }

    /* duplicate tf1 because transformer_combine might not be able
       to use twice the same argument */
    tf2 = transformer_dup(tf1);
    tf2 = transformer_combine(tf2, tf1);

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer", "tf2 =");
	(void) print_transformer(tf2);
    }

    /* input/output relation for one or two iteration of the loop */
    tf12 = transformer_convex_hull(tf1, tf2);

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer", "tf12 =");
	(void) print_transformer(tf12);
    }

    /* apply one iteration on tf12 */
    tf23 = transformer_combine(tf12, tf1);

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer", "tf23 =");
	(void) print_transformer(tf23);
    }

    /* fix-point */
    /* should be done again based on Chenikova */
    /* tf_star = transformer_fix_point(tf12, tf23); */

    ifdebug(8) {
	(void) fprintf(stderr,"%s: %s\n","loop_to_transformer", "tf_star =");
	(void) print_transformer(tf_star);
    }

    /* free useless transformers */
    transformer_free(tf1);
    transformer_free(tf2);
    transformer_free(tf12);
    transformer_free(tf23);

    tf = tf_star;

    return tf;
}


/* Must be declared explicity to keep a logical order in this C file.
 * Also, the matrice type is now obsolete and should not appear in
 * transformer.h. Hence buil_transfer_matrix() must be static.
 */
static void 
build_transfer_matrix(matrice *, Pcontrainte, int, Pbase);

/* Let A be the affine loop transfert function. The affine transfer fix-point T
 * is such that T(A-I) = 0
 *
 * T A = 0 also gives a fix-point when merged with the initial state. It can only be
 * used to compute preconditions.
 *
 * Algorithm (let H be A's Smith normal form):
 *
 * A = A - I (if necessary)
 * H = P A Q
 * A = P^-1 H Q^-1
 * T A = T P^-1 H Q^-1 = 0
 * T P^-1 H = 0 (by multipliying by Q)
 * 
 * Soit X t.q. X H = 0
 * A basis for all solutions is given by X chosen such that
 * X is a rectangular matrix (0 I) selecting the zero submatrix of H
 *
 * T P^-1 = X
 * T = X P
 *
 * Note: I (FI) believe this functions is wrong because it does not
 * return the appropriate arguments. The fix point transformer
 * should modify as many variables as the input tf. A new basis
 * should not be recomputed according to the transition matrix.
 * This problem seems to be fixed in the callers, see
 * loop_to_transformer() and whileloop_to_transformer().
 */

transformer 
transformer_equality_fix_point(tf)
transformer tf;
{
    /* result */
    transformer fix_tf = transformer_identity();

    /* do not modify an input argument */
    transformer tf_copy = transformer_dup(tf);

    /* number of transfer equalities plus the homogeneous constraint which
       states that 1 == 1 */
    int n_eq = 1;
    /* use a matrix a to store these equalities in a dense form */
    matrice a = MATRICE_UNDEFINED;
    matrice h = MATRICE_UNDEFINED;
    matrice p = MATRICE_UNDEFINED;
    matrice q = MATRICE_UNDEFINED;
    Pbase b_new = BASE_NULLE;
    Pcontrainte lteq = CONTRAINTE_UNDEFINED;
    Pcontrainte leq = sc_egalites((Psysteme) predicate_system(transformer_relation(tf_copy)));

    Psysteme sc_inv = SC_UNDEFINED;
    int n_inv = 0;

    int i = 0;
    /* Pbase t = BASE_UNDEFINED; */

    ifdebug(8) {
	debug(8, "transformer_equality_fix_point", "begin for transformer %p\n", tf);
	fprint_transformer(stderr, tf, (get_variable_name_t) external_value_name);
    }
 
    /* If the input transformer is not feasible, so is not its fixpoint
     * because the number of iterations may be zero which implies identity.
     */
    if(transformer_empty_p(tf)) {
      /* fix_tf = transformer_identity(); */
	ifdebug(8) {
	    debug(8, "transformer_equality_fix_point", "fix-point fix_tf=\n");
	    fprint_transformer(stderr, fix_tf, (get_variable_name_t) external_value_name);
	    debug(8, "transformer_equality_fix_point", "end\n");
	}
	return fix_tf;
    }

    /* find or build explicit transfer equations: v#new = f(v1#old, v2#old,...)
     * and the corresponding sub-basis
     *
     * FI: for each constant variable, whose constance is implictly implied by not having it
     * in the argument field, an equation should be added...
     *
     * lieq = build_implicit_equalities(tf);
     * lieq->succ = leq;
     * leq = lieq;
     */
    build_transfer_equations(leq, &lteq, &b_new);

    /* build matrix a from lteq and b_new: this should be moved in a function */
    n_eq = base_dimension(b_new)+1;
    build_transfer_matrix(&a , lteq, n_eq, b_new);
    ifdebug(8) {
	debug(8, "transformer_equality_fix_point", "transfer matrix:\n");
	matrice_fprint(stderr, a, n_eq, n_eq);
    }

    /* Subtract the identity matrix */
    for(i=1; i<= n_eq; i++) {
	value_substract(ACCESS(a, n_eq, i, i),VALUE_ONE);
    }

    /* Compute the Smith normal form: H = P A Q */
    h = matrice_new(n_eq, n_eq);
    p = matrice_new(n_eq, n_eq);
    q = matrice_new(n_eq, n_eq);
    DENOMINATOR(h) = VALUE_ONE;
    DENOMINATOR(p) = VALUE_ONE;
    DENOMINATOR(q) = VALUE_ONE;
    matrice_smith(a, n_eq, n_eq, p, h, q);
 
    ifdebug(8) {
	debug(8, "transformer_equality_fix_point", "smith matrix h=p.a.q:\n");
	matrice_fprint(stderr, h, n_eq, n_eq);
	debug(8, "transformer_equality_fix_point", "p  matrix:\n");
	matrice_fprint(stderr, p, n_eq, n_eq);
	debug(8, "transformer_equality_fix_point", "q  matrix:\n");
	matrice_fprint(stderr, q, n_eq, n_eq);
    }

    /* Find out the number of invariants n_inv */
    for(i=1; i <= n_eq && 
	    value_notzero_p(ACCESS(h, n_eq, i, i)) ; i++)
	;
    n_inv = n_eq - i + 1;
    debug(8, "transformer_equality_fix_point", "number of useful invariants: %d\n", n_inv-1);
    pips_assert("transformer_equality_fix_point", n_inv >= 1);

    /* Convert p's last n_inv rows into constraints */
    /* FI: maybe I should retrieve fix_tf system... */
    sc_inv = sc_new();

    for(i = n_eq - n_inv + 1 ; i <= n_eq; i++) {
	int j;
	/* is it a non-trivial invariant? */
	for(j=1; j<= n_eq-1 && value_zero_p(ACCESS(p, n_eq, i, j)); j++)
	    ;
	if( j != n_eq) {
	    Pvecteur v_inv = VECTEUR_NUL;
	    Pcontrainte c_inv = CONTRAINTE_UNDEFINED;

	    for(j = 1; j<= n_eq; j++) {
		Value coeff = ACCESS(p, n_eq, i, j);

		if(value_notzero_p(coeff)) {
		    entity n_eb = (entity) variable_of_rank(b_new, j);
		    entity o_eb = new_value_to_old_value(n_eb);

		    vect_add_elem(&v_inv, (Variable)n_eb, coeff);
		    vect_add_elem(&v_inv, (Variable)o_eb, value_uminus(coeff));
		}
	    }
	    c_inv = contrainte_make(v_inv);
	    sc_add_egalite(sc_inv, c_inv);
	}
    }

    sc_creer_base(sc_inv);

    ifdebug(8) {
	debug(8, "transformer_equality_fix_point", "fix-point sc_inv=\n");
	sc_fprint(stderr, sc_inv, (char * (*)(Variable)) external_value_name);
	debug(8, "transformer_equality_fix_point", "end\n");
    }

    /* Set fix_tf's argument list */
    /*
    for(t = b_new; !BASE_NULLE_P(t); t = t->succ) {
	I should use a conversion function from value_to_variable()
	entity arg = (entity) vecteur_var(t);

	transformer_arguments(fix_tf) = arguments_add_entity(transformer_arguments(fix_tf), arg);
    }
    */
    transformer_arguments(fix_tf) = dup_arguments(transformer_arguments(tf));
    /* Fix the basis and the dimension */
    sc_base(sc_inv) = base_dup(sc_base(predicate_system(transformer_relation(tf))));
    sc_dimension(sc_inv) = sc_dimension(predicate_system(transformer_relation(tf)));
    transformer_relation(fix_tf) = make_predicate(sc_inv);
 
    /* get rid of dense matrices */
    matrice_free(a);
    matrice_free(h);
    matrice_free(p);
    matrice_free(q);

    free_transformer(tf_copy);

    ifdebug(8) {
	debug(8, "transformer_equality_fix_point", "fix-point fix_tf=\n");
	fprint_transformer(stderr, fix_tf, (get_variable_name_t) external_value_name);
	debug(8, "transformer_equality_fix_point", "end\n");
    }

    return fix_tf;
}

void 
build_transfer_equations(leq, plteq, pb_new)
Pcontrainte leq;
Pcontrainte *plteq;
Pbase * pb_new;
{
    Pcontrainte lteq = CONTRAINTE_UNDEFINED;
    Pbase b_new = BASE_NULLE;
    Pbase b_old = BASE_NULLE;
    Pbase b_diff = BASE_NULLE;
    Pcontrainte eq = CONTRAINTE_UNDEFINED;

    ifdebug(8) {
	debug(8, "build_transfer_equations", "begin\ninput equations:\n");
	egalites_fprint(stderr, leq, (char * (*)(Variable)) external_value_name);
    }

    /* FI: this is the simplest version;
     * It would be possible to build a larger set of transfer equations
     * by adding indirect transfer equations using a new basis
     * till the set is stable, and then by removing equations containing
     * old values which are not defined, again till the result is stable
     *
     * I guess that these new equations would not change the fix-point
     * and/or the preconditions.
     */
    for(eq = leq; !CONTRAINTE_UNDEFINED_P(eq); eq = eq->succ) {
	if(transfer_equation_p(contrainte_vecteur(eq))) {
	    Pcontrainte neq = contrainte_dup(eq);

	    neq->succ = lteq;
	    lteq = neq;
	}
    }

    ifdebug(8) {
	debug(8, "build_transfer_equations", "preliminary transfer equations:\n");
	egalites_fprint(stderr, lteq, (char * (*)(Variable)) external_value_name);
    }

    /* derive the new basis and the old basis */
    equations_to_bases(lteq, &b_new, &b_old);

    /* check that the old basis is included in the new basis (else no fix-point!) */
    ifdebug(8) {
	debug(8, "build_transfer_equations", "old basis:\n");
	base_fprint(stderr, b_old, (char * (*)(Variable)) external_value_name);
	debug(8, "build_transfer_equations", "new basis:\n");
	base_fprint(stderr, b_new, (char * (*)(Variable)) external_value_name);
    }

    /* Remove equations as long b_old is not included in b_new */
    b_diff = base_difference(b_old, b_new);
    while(!BASE_NULLE_P(b_diff)) {
	Pcontrainte n_lteq = CONTRAINTE_UNDEFINED;
	Pcontrainte next_eq = CONTRAINTE_UNDEFINED;

	for(eq = lteq; !CONTRAINTE_UNDEFINED_P(eq); eq = next_eq) {
	    bool preserve = TRUE;
	    Pvecteur t = VECTEUR_UNDEFINED;
	    for(t = contrainte_vecteur(eq); !VECTEUR_UNDEFINED_P(t) && preserve; t = t->succ) {
		entity e = (entity) vecteur_var(t);

		preserve = base_contains_variable_p(b_diff, (Variable) e);
	    }
	    next_eq = eq->succ;
	    if(preserve) {
		eq->succ = n_lteq;
		n_lteq = eq;
	    }
	    else {
		contrainte_rm(eq);
	    }
	}
	lteq = n_lteq; /* could be avoided by using only lteq... */
	equations_to_bases(lteq, &b_new, &b_old);
	b_diff = base_difference(b_old, b_new);
    }

    ifdebug(8) {
	debug(8, "build_transfer_equations", "final transfer equations:\n");
	egalites_fprint(stderr, lteq, (char * (*)(Variable)) external_value_name);
	debug(8, "build_transfer_equations", "old basis:\n");
	base_fprint(stderr, b_old, (char * (*)(Variable)) external_value_name);
	debug(8, "build_transfer_equations", "new basis:\n");
	base_fprint(stderr, b_new, (char * (*)(Variable)) external_value_name);
    }

    if(!sub_basis_p(b_old, b_new)) {
	pips_error("build_transfer_equations", "Old basis is too large\n");
    }
    base_rm(b_old);
    b_old = BASE_UNDEFINED;
    *plteq = lteq;
    *pb_new = b_new;

    ifdebug(8) {
	debug(8, "build_transfer_equations", "results\ntransfer equations:\n");
	egalites_fprint(stderr, lteq, (char * (*)(Variable)) external_value_name);
	debug(8, "build_transfer_equations", "results\ntransfer basis:\n");
	base_fprint(stderr, b_new, (char * (*)(Variable)) external_value_name);
	debug(8, "build_transfer_equations", "end\n");
    }
}

/* A transfer equation is an explicit equation giving one new value
 * as a function of old values and a constant term. Because we are
 * using diophantine equations, the coefficient of the new value must
 * one or minus one. We assume that the equation is reduced (i.e.
 * gcd(coeffs) == 1).
 *
 * FI: Non-unary coefficients may appear because equations were combined, for instance
 * by a previous internal fix-point as in KINETI of mdg-fi.f (Perfect Club). Maybe, something
 * could be done for these nested fix-points.
 */
bool 
transfer_equation_p(eq)
Pvecteur eq;
{
    Pvecteur t;
    int n_new = 0;
    Value coeff = VALUE_ZERO;

    for(t=eq; !VECTEUR_UNDEFINED_P(t) && n_new <= 1; t = t->succ) {
	entity e = (entity) vecteur_var(t);

	if( e != (entity) TCST && new_value_entity_p(e)) {
	    coeff = vecteur_val(t);
	    n_new++;
	}
    }

    return (n_new==1) && (value_one_p(coeff) || value_mone_p(coeff));
}

entity 
new_value_in_transfer_equation(eq)
Pvecteur eq;
{
    Pvecteur t;
    int n_new = 0;
    Value coeff = VALUE_ZERO; /* for gcc */
    entity new_value = entity_undefined;

    for(t=eq; !VECTEUR_UNDEFINED_P(t) && n_new <= 1; t = t->succ) {
	entity e = (entity) vecteur_var(t);

	if( e != (entity) TCST && new_value_entity_p(e) && 
	   (value_one_p(vecteur_val(t)) || value_mone_p(vecteur_val(t)))) {
	    new_value = e;
	    coeff = vecteur_val(t);
	    n_new++;
	}
    }

    if(value_mone_p(coeff)) {
	for(t=eq; !VECTEUR_UNDEFINED_P(t) && n_new <= 1; t = t->succ) {
	    value_oppose(vecteur_val(t));
	}
    }

    pips_assert("new_value_in_transfer_equation", n_new==1);

    return new_value;
}



static void 
build_transfer_matrix(pa, lteq, n_eq, b_new)
matrice * pa;
Pcontrainte lteq;
int n_eq;
Pbase b_new;
{
    matrice a = matrice_new(n_eq, n_eq);
    Pcontrainte eq = CONTRAINTE_UNDEFINED;

    matrice_nulle(a, n_eq, n_eq);

    for(eq = lteq; !CONTRAINTE_UNDEFINED_P(eq); eq = eq->succ) {
	Pvecteur t = contrainte_vecteur(eq);
	entity nv = new_value_in_transfer_equation(t);
	int nv_rank = rank_of_variable(b_new, (Variable) nv);

	for( ; !VECTEUR_UNDEFINED_P(t); t = t->succ) {
	    entity e = (entity) vecteur_var(t);

	    if( e != (entity) TCST ) {
		if(new_value_entity_p(e)) {
		    pips_assert("build_transfer_matrix", 
				value_one_p(vecteur_val(t)));
		}
		else {
		    entity ov = old_value_to_new_value(e);
		    int ov_rank = rank_of_variable(b_new, (Variable) ov);
		    debug(8,"build_transfer_matrix", "nv_rank=%d, ov_rank=%d\n",
			  nv_rank, ov_rank);
		    ACCESS(a, n_eq, nv_rank, ov_rank) = 
			value_uminus(vecteur_val(t));
		}
	    }
	    else {
		ACCESS(a, n_eq, nv_rank, n_eq) = 
		    value_uminus(vecteur_val(t));
	    }
	}
    }
    /* add the homogeneous coordinate */
    ACCESS(a, n_eq, n_eq, n_eq) = VALUE_ONE;

    *pa = a;
}

/* FI: should be moved in base.c */

/* sub_basis_p(Pbase b1, Pbase b2): check if b1 is included in b2 */
bool 
sub_basis_p(b1, b2)
Pbase b1;
Pbase b2;
{
    bool is_a_sub_basis = TRUE;
    Pbase t;

    for(t=b1; !VECTEUR_UNDEFINED_P(t) && is_a_sub_basis; t = t->succ) {
	is_a_sub_basis = base_contains_variable_p(b2, vecteur_var(t));
    }

    return is_a_sub_basis;
}

void 
equations_to_bases(lteq, pb_new, pb_old)
Pcontrainte lteq;
Pbase * pb_new;
Pbase * pb_old;
{
    Pbase b_new = BASE_UNDEFINED;
    Pbase b_old = BASE_UNDEFINED;
    Pcontrainte eq = CONTRAINTE_UNDEFINED;

    for(eq = lteq; !CONTRAINTE_UNDEFINED_P(eq); eq = eq->succ) {
	Pvecteur t;

	for(t=contrainte_vecteur(eq); !VECTEUR_UNDEFINED_P(t); t = t->succ) {
	    entity e = (entity) vecteur_var(t);

	    if( e != (entity) TCST ) {
		if(new_value_entity_p(e)) {
		    b_new = vect_add_variable(b_new, (Variable) e);
		}
		else {
		    b_old = vect_add_variable(b_old, (Variable) old_value_to_new_value(e));
		}
	    }
	}
    }

    *pb_new = b_new;
    *pb_old = b_old;
}


/* This fixpoint function was developped to present a talk at FORMA. I used
 * examples published by Pugh and I realized that the fixpoint constraints 
 * were obvious in the loop body transformer. You just had to identify them.
 * This is not a clever algorithm. It certainly would not resist any messing up
 * with the constraints, i.e. a change of basis. But it provides very good result
 * for a class of applications.
 *
 * Algorithm:
 *  1. Find a loop counter
 *  2. Use it to eliminate all constant terms
 *  3. Look for invariant constraints
 *
 * Let d_i = i#new - i#old. An invariant constraint is a sum of d_i which is
 * equal to zero or greater than zero. Such a constraint is its own fixpoint:
 * if you combine it with itself after renaming i#new as i#int on one hand
 * and i#old as i#int on the other, the global sum for and equation or its sign
 * for an inequality is unchanged.
 */

transformer 
transformer_pattern_fix_point(tf)
transformer tf;
{
    /* result */
    transformer fix_tf =  transformer_dup(tf);
    Psysteme fix_sc = (Psysteme) predicate_system(transformer_relation(fix_tf));
    Pvecteur v_inc = VECTEUR_UNDEFINED;
    int inc = 1;

    ifdebug(8) {
	debug(8, "transformer_pattern_fix_point", "Begin for transformer %p:\n", tf);
	fprint_transformer(stderr, tf, (get_variable_name_t) external_value_name);
    }

    /* Look for the best loop counter: the smallest increment is chosen */
    v_inc = look_for_the_best_counter(sc_egalites(fix_sc));

    if(!VECTEUR_UNDEFINED_P(v_inc)) {

	ifdebug(8) {
	    debug(8, "transformer_pattern_fix_point", "incrementation vector=\n");
	    vect_fprint(stderr, v_inc, (char * (*)(Variable)) external_value_name);
	}

	/* eliminate sharing between v_inc and fix_sc */
	v_inc = vect_dup(v_inc);

	/* retrieve inc from v_inc */
	inc = vect_coeff(TCST, v_inc);

	/* Replace constant terms in equalities and inequalities by
	 * loop counter differences
	 */
	fix_sc = sc_eliminate_constant_terms(fix_sc, v_inc);

	ifdebug(8) {
	    debug(8, "transformer_pattern_fix_point", "after constant term elimination=\n");
	    sc_fprint(stderr, fix_sc, (char * (*)(Variable)) external_value_name);
	}

	fix_sc = sc_elim_redund(fix_sc);

	ifdebug(8) {
	    debug(8, "transformer_pattern_fix_point", "after normalization=\n");
	    sc_fprint(stderr, fix_sc, (char * (*)(Variable)) external_value_name);
	}

	fix_sc = sc_keep_invariants_only(fix_sc);

	ifdebug(8) {
	    debug(8, "transformer_pattern_fix_point", "after non-invariant elimination=\n");
	    sc_fprint(stderr, fix_sc, (char * (*)(Variable)) external_value_name);
	}

	fix_sc = sc_elim_redund(fix_sc);

	ifdebug(8) {
	    debug(8, "transformer_pattern_fix_point", "after 2nd normalization=\n");
	    sc_fprint(stderr, fix_sc, (char * (*)(Variable)) external_value_name);
	}
    }
    else {
	debug(8, "transformer_pattern_fix_point", "No counter found\n");
	/* Remove all constraints to build the invariant transformer */
	contraintes_free(sc_egalites(fix_sc));
	sc_egalites(fix_sc) = CONTRAINTE_UNDEFINED;
	sc_nbre_egalites(fix_sc) = 0;
	contraintes_free(sc_inegalites(fix_sc));
	sc_inegalites(fix_sc) = CONTRAINTE_UNDEFINED;
	sc_nbre_inegalites(fix_sc) = 0;
    }

    ifdebug(8) {
	debug(8, "transformer_pattern_fix_point", "fix-point fix_tf=\n");
	fprint_transformer(stderr, fix_tf, (get_variable_name_t) external_value_name);
	debug(8, "transformer_pattern_fix_point", "end\n");
    }

    return fix_tf;
}


/* Try to identify a loop counter among the equation egs.
 * If the transformer has been build naively, a loop counter
 * should have a transformer equation like i#new = i#old + K_i
 * where K_i is a numerical constant.
 * 
 * Since the loop counter is later used to eliminate constant
 * terms in other constraints, variable i with the minimal absolute
 * value K_i shuold be chosen.
 */

Pvecteur
look_for_the_best_counter(Pcontrainte egs)
{
    Pvecteur v_inc = VECTEUR_UNDEFINED;
    entity old_index = entity_undefined;
    entity new_index = entity_undefined;
    Pcontrainte leq = CONTRAINTE_UNDEFINED;
    int inc = 0;

    for(leq = egs;
	!CONTRAINTE_UNDEFINED_P(leq);
	leq = contrainte_succ(leq)) {

	Pvecteur v = contrainte_vecteur(leq);
	int c_inc = 0;

	if(vect_size(v)==3) {
	    entity p_old_index = entity_undefined;
	    entity p_new_index = entity_undefined;
	    Pvecteur lv = VECTEUR_UNDEFINED;
	    bool failed = FALSE;
	    for(lv = v; !VECTEUR_UNDEFINED_P(lv) && !failed; lv = vecteur_succ(lv)) {
		if(vecteur_var(lv) == TCST) {
		    c_inc = (int) vecteur_val(lv);
		}
		else if(old_value_entity_p((entity) vecteur_var(lv))) {
		    if(entity_undefined_p(p_old_index))
			p_old_index = (entity) vecteur_var(lv);
		    else
			failed = TRUE;
		}
		else if(new_value_entity_p((entity) vecteur_var(lv))) {
		    if(entity_undefined_p(p_new_index))
			p_new_index = (entity) vecteur_var(lv);
		    else
			failed = TRUE;
		}
		else {
		    pips_error("look_for_the_best_counter", "Unexpected value entity %s",
			       entity_local_name((entity) vecteur_var(lv)));
		}
	    }
	    if(!failed && value_to_variable(p_old_index) == value_to_variable(p_new_index)) {
		if(ABS(c_inc) < ABS(inc) || c_inc == 1) {
		    inc = c_inc;
		    old_index = p_old_index;
		    new_index = p_new_index;
		    v_inc = v;
		}
	    }
	}
    }

    return v_inc;
}

/* Eliminate all constant terms in sc using v.
 * No sharing between sc and v is assumed as sc is updated!
 *
 * This function should be located in Linear/sc
 */
Psysteme
sc_eliminate_constant_terms(Psysteme sc, Pvecteur v)
{
    int cv = vect_coeff(TCST, v);

    assert(cv!=0);

    sc_egalites(sc) = constraints_eliminate_constant_terms(sc_egalites(sc), v);
    sc_inegalites(sc) = constraints_eliminate_constant_terms(sc_inegalites(sc), v);

    return sc;
}

Pcontrainte
constraints_eliminate_constant_terms(Pcontrainte lc, Pvecteur v)
{
    Value c1 = vect_coeff(TCST, v);
    Pcontrainte cc = CONTRAINTE_UNDEFINED;

    if(c1==0) {
	abort();
    }

    for(cc = lc; !CONTRAINTE_UNDEFINED_P(cc); cc = contrainte_succ(cc)) {
	Pvecteur cv = contrainte_vecteur(cc);
	Value c2;

	if((c2 = vect_coeff(TCST, cv))!=0) {
	    cv = vect_multiply(cv, c1);
	    cv = vect_cl(cv, -c2, v);
	}

	/* cv may now be the null vector */
	contrainte_vecteur(cc) = cv;
    }

    return lc;
}

/* This function cannot be moved into the Linear library.
 * It relies on the concepts of old and new values.
 *
 * It could be improved by using an approximate initial fix-point
 * to evaluate v_sum (see invariant_vector_p) and to degrade
 * equations into inequalities or to relax inequalities.
 *
 * For instance, p = p + a won't generate an invariant.
 * However, if the lousy fix-point is sufficient to prove a >= 0,
 * it is possible to derive p#new >= p#old. Or even better
 * if a's value turns out to be known.
 */
Psysteme
sc_keep_invariants_only(Psysteme sc)
{
    sc_egalites(sc) = constraints_keep_invariants_only(sc_egalites(sc));
    sc_inegalites(sc) = constraints_keep_invariants_only(sc_inegalites(sc));
    return sc;
}


Pcontrainte
constraints_keep_invariants_only(Pcontrainte lc)
{
    Pcontrainte cc;

    for(cc = lc; !CONTRAINTE_UNDEFINED_P(cc); cc = contrainte_succ(cc)) {
	Pvecteur cv = contrainte_vecteur(cc);
	if(!invariant_vector_p(cv)) {
	    vect_rm(cv);
	    contrainte_vecteur(cc) = VECTEUR_NUL;
	}
    }

    return lc;
}

/* A vector (in fact, a constraint) represents an invariant
 * if it is a sum of delta for each variable.
 * Let di = i#new - i#old. If v = sigma (di) = 0, v can
 * be used to build a loop invariant.
 *
 * It is assumed that constant terms have been substituted
 * earlier using a loop counter.
 */

bool
invariant_vector_p(Pvecteur v)
{
    bool invariant = TRUE;
    Pvecteur cv = VECTEUR_UNDEFINED;
    Pvecteur v_old = VECTEUR_NUL;
    Pvecteur v_new = VECTEUR_NUL;
    Pvecteur v_sum = VECTEUR_UNDEFINED;

    ifdebug(8) {
	debug(8, "invariant_vector_p", "begin for vector: ");
	vect_fprint(stderr, v, (char * (*)(Variable)) external_value_name);
    }

    for(cv=v; !VECTEUR_UNDEFINED_P(cv); cv = vecteur_succ(cv)) {
	if(vecteur_var(cv)==TCST) {
	    /* OK, you could argue for invariant = FALSE,
	     * but this would not help my debugging!
	     */
	    pips_error("invariant_vector_p",
		       "Constant term in a potential invariant vector!\n");
	}
	else {
	    entity e = (entity) vecteur_var(cv);
	    entity var = value_to_variable(e);


	    if(old_value_entity_p(e)) {
		vect_add_elem(&v_old, (Variable) var, vecteur_val(cv));
	    }
	    else if(new_value_entity_p(e)) {
		vect_add_elem(&v_new, (Variable) var, vecteur_val(cv));
	    }
	    else {
		pips_error("invariant_vector_p",
			   "Non value entity %s in invariant vector\n",
			   entity_name(e));
	    }
	}
    }

    v_sum = vect_add(v_new, v_old);

    ifdebug(8) {
	debug(8, "invariant_vector_p", "vector v_new: ");
	vect_fprint(stderr, v_new, (char * (*)(Variable)) external_value_name);
	debug(8, "invariant_vector_p", "vector v_old: ");
	vect_fprint(stderr, v_old, (char * (*)(Variable)) external_value_name);
	debug(8, "invariant_vector_p", "vector v_sum: ");
	vect_fprint(stderr, v_sum, (char * (*)(Variable)) external_value_name);
    }

    if(VECTEUR_NUL_P(v_sum)) {
	invariant = TRUE;
    }
    else {
	/* if v_sum is non zero, you might try to bound it
	 * wrt the loop precondition?
	 */
	invariant = FALSE;
	vect_rm(v_sum);
    }

    vect_rm(v_new);
    vect_rm(v_old);

    debug(8, "invariant_vector_p", "end invariant=%s\n",
	  bool_to_string(invariant));

    return invariant;
}

/* Computation of a fix point using constraints on the discrete derivative.
 *
 * Implicit equations, n#new - n#old = 0, are not added. Instead, invariant
 * constraints in tf are obtained by projection and added.
 *
 * Intermediate values are used to encode the differences. For instance,
 * i#int = i#new - i#old
 */
transformer transformer_derivative_fix_point(transformer tf)
{
  transformer fix_tf = transformer_dup(tf);

  ifdebug(8) {
    pips_debug(8, "Begin for transformer %p:\n", tf);
    fprint_transformer(stderr, tf, (get_variable_name_t) external_value_name);
  }

  if(transformer_empty_p(tf)) {
    /* I tried to use the standard procedure but arguments are not removed
       when the transformer tf is not feasible. Since we compute the * fix
       point and not the + fix point, the fix point is the identity. */
    free_transformer(fix_tf);
    fix_tf = transformer_identity();
  }
  else {
    //transformer inv_tf = transformer_undefined; /* invariant constraints of tf */
    /* sc is going to be modified and destroyed and eventually
       replaced in fix_tf */
    Psysteme sc = predicate_system(transformer_relation(fix_tf));
    Psysteme sc_homo = SC_UNDEFINED;
    /* Do not handle variable which do not appear explicitly in constraints! */
    Pbase b = sc_to_minimal_basis(sc);
    Pbase ib = base_dup(sc_base(sc)); /* initial and final basis */
    Pbase bv = BASE_NULLE; /* basis vector */
    Pbase diffb = BASE_NULLE; /* basis of difference vectors */
    Pcontrainte ceq = CONTRAINTE_UNDEFINED; /* loop index */
    Pcontrainte cineq = CONTRAINTE_UNDEFINED; /* loop index */
    Pcontrainte leq = CONTRAINTE_UNDEFINED; /* fix point equations */
    Pcontrainte lineq = CONTRAINTE_UNDEFINED; /* fix point inequalities */

    ifdebug(8) {
      pips_debug(8, "Begin for transformer %p:\n", tf);
      fprint_transformer(stderr, tf, (get_variable_name_t) external_value_name);
    }

    /* Compute constraints with difference equations */

    for(bv = b; !BASE_NULLE_P(bv); bv = bv->succ) {
      entity oldv = (entity) vecteur_var(bv);

      /* Only generate difference equations if the old value is used */
      if(old_value_entity_p(oldv)) {
	entity var = value_to_variable(oldv);
	entity newv = entity_to_new_value(var);
	entity diffv = entity_to_intermediate_value(var);
	Pvecteur diff = VECTEUR_NUL;
	Pcontrainte eq = CONTRAINTE_UNDEFINED;

	diff = vect_make(diff,
			 (Variable) diffv, VALUE_ONE,
			 (Variable) newv,  VALUE_MONE,
			 (Variable) oldv, VALUE_ONE,
			 TCST, VALUE_ZERO);

	eq = contrainte_make(diff);
	sc = sc_equation_add(sc, eq);
      }
    }

    ifdebug(8) {
      pips_debug(8, "with difference equations=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* Project all variables but differences */

    sc = sc_projection_ofl_along_variables(sc, b);

    ifdebug(8) {
      pips_debug(8, "Non-homogeneous constraints on derivatives=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* Eliminate constant terms to get homogeneous constraints.
     * The constant term elimination is equivalent to a simultaneous
     * integration of all derivatives: all variables are modified by
     * the same number of iterations.
     */

    sc_homo = sc_dup(sc);
    sc_homo = sc_projection_ofl(sc_homo, TCST);

    ifdebug(8) {
      pips_debug(8,
		 "derivative constraints after elimination of constant terms=\n");
      sc_fprint(stderr, sc_homo, (char * (*)(Variable)) external_value_name);
    }

    sc = sc_append(sc, sc_homo);
    sc_rm(sc_homo);

    ifdebug(8) {
      pips_debug(8, "All constraints on derivatives=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* Generate invariants from the derivative constraints */

    /* For each equation, keep an equation if the constant term is zero
     * or transform it in an inequality if not. The constant term of the
     * inequality must be zero because T* is computed, not T+
     */
    for(ceq = sc_egalites(sc); !CONTRAINTE_UNDEFINED_P(ceq);
	ceq = contrainte_succ(ceq)) {
      Pvecteur eq = vect_dup(contrainte_vecteur(ceq));
      Pcontrainte new_c = contrainte_make(eq);
      Value cst = vect_coeff(TCST, eq);

      if(cst == VALUE_ZERO) {
	/* leave the equation as it is and put it in the equation list */
	new_c->succ = leq;
	leq = new_c;
      }
      else {
	vect_chg_coeff(&(contrainte_vecteur(new_c)), TCST, VALUE_ZERO);
	new_c->succ = lineq;
	lineq = new_c;

	if(cst >= VALUE_ONE) {
	  /* the signs are OK */
	  ;
	}
	else /* cst <= VALUE_MONE */ {
	  vect_chg_sgn(contrainte_vecteur(new_c));
	}
      }
    }

    /* For each inequality, keep an equality if the constant term is positive
     * (i.e. the lhs is decreasing). Constant term in
     * inequality must be zero because T* is conputed, not T+. T+ can be
     * derived from T* as T o T*.
     */
    for(cineq = sc_inegalites(sc); !CONTRAINTE_UNDEFINED_P(cineq);
	cineq = contrainte_succ(cineq)) {
      Value cst = vect_coeff(TCST, contrainte_vecteur(cineq));

      if(cst >= VALUE_ZERO) {
	/* OK for decreasing derivative
	   (Note: the constant term is in the lhs) */
	Pvecteur ineq = vect_dup(contrainte_vecteur(cineq));
	Pcontrainte new_c = CONTRAINTE_UNDEFINED;

	vect_chg_coeff(&ineq, TCST, VALUE_ZERO);
	new_c = contrainte_make(ineq);
	new_c->succ = lineq;
	lineq = new_c;
      }
      else {
	/* The sign of the derivative is unknown:
	   no invariant can be deduced */
	;
      }
    }

    ifdebug(8) {
      pips_debug(8, "First set of equality invariants=\n");
      egalites_fprint(stderr, leq, (char * (*)(Variable)) external_value_name);
      pips_debug(8, "First set of inequality invariants=\n");
      inegalites_fprint(stderr, lineq, (char * (*)(Variable)) external_value_name);
    }

    /* sc is not needed anymore, it can be updated with leq and lineq */

    sc_rm(sc);
    sc = sc_make(leq, lineq);

    /* Difference variables must substituted back to differences
     * between old and new values.
     */

    ifdebug(8) {
      pips_debug(8, "All invariants on derivatives=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    for(bv = b; !BASE_NULLE_P(bv); bv = bv->succ) {
      entity oldv = (entity) vecteur_var(bv);

      /* Only generate difference equations if the old value is used */
      if(old_value_entity_p(oldv)) {
	entity var = value_to_variable(oldv);
	entity newv = entity_to_new_value(var);
	entity diffv = entity_to_intermediate_value(var);
	Pvecteur diff = VECTEUR_NUL;
	Pcontrainte eq = CONTRAINTE_UNDEFINED;

	diff = vect_make(diff,
			 (Variable) diffv, VALUE_ONE,
			 (Variable) newv,  VALUE_MONE,
			 (Variable) oldv, VALUE_ONE,
			 TCST, VALUE_ZERO);

	eq = contrainte_make(diff);
	sc = sc_equation_add(sc, eq);
	diffb = base_add_variable(diffb, (Variable) diffv);
      }
    }

    ifdebug(8) {
      pips_debug(8,
		 "All invariants on derivatives with difference variables=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* Project all difference variables */

    sc = sc_projection_ofl_along_variables(sc, diffb);

    ifdebug(8) {
      pips_debug(8, "All invariants on differences=\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* The full basis must be used again */
    base_rm(sc_base(sc)), sc_base(sc) = BASE_NULLE;
    sc_base(sc) = ib;
    sc_dimension(sc) = vect_size(ib);
    base_rm(b), b = BASE_NULLE;

    ifdebug(8) {
      pips_debug(8, "All invariants with proper basis =\n");
      sc_fprint(stderr, sc, (char * (*)(Variable)) external_value_name);
    }

    /* Plug sc back into fix_tf */
    predicate_system(transformer_relation(fix_tf)) = sc;

    /* Add constraints about invariant variables. This could be avoided if
       implicit equalities were added before the derivative are processed:
       each invariant variable would generate an implicit null
       difference.

       Such invariant constraints do not exist in general. We have them when
       array references are trusted.

       This is wrong because we compute f* and not f+ and this is not true
       for the initial store.

    */
    /*
       inv_tf = transformer_arguments_projection(transformer_dup(tf));
       fix_tf = transformer_image_intersection(fix_tf, inv_tf);
       free_transformer(inv_tf);
    */
  }
  /* That's all! */

  ifdebug(8) {
    pips_debug(8, "fix-point fix_tf=\n");
    fprint_transformer(stderr, fix_tf, (get_variable_name_t) external_value_name);
    transformer_consistency_p(fix_tf);
    pips_debug(8, "end\n");
  }

  return fix_tf;
}

/* Computation of a fix point: drop all constraints, remember which
 * variables are changed. Except if the transformer is syntactically
 * empty since its fix point is easy to compute. Without
 * normalization, mathematically empty transformers are lost.
 *
*/
transformer transformer_basic_fix_point(transformer tf)
{
  transformer fix_tf = copy_transformer(tf);

  if(!transformer_empty_p(tf)) {
    /* get rid of equalities and inequalities but keep the basis */
    Psysteme sc = predicate_system(transformer_relation(fix_tf));
    Pcontrainte eq = sc_egalites(sc);
    Pcontrainte ineq = sc_inegalites(sc);
    contraintes_free(eq);
    contraintes_free(ineq);
    sc_egalites(sc) = CONTRAINTE_UNDEFINED;
    sc_inegalites(sc) = CONTRAINTE_UNDEFINED;
    sc_nbre_egalites(sc) = 0;
    sc_nbre_inegalites(sc) = 0;
  }

  return fix_tf;
}
