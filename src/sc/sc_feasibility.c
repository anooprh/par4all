/* package sc : $RCSfile: sc_feasibility.c,v $ version $Revision$
 * date: $Date: 1997/07/17 20:32:19 $, 
 * got on %D%, %T%
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * This file provides functions to test the feasibility of a system 
 * of constraints. 
 *
 * Arguments of these functions :
 * 
 * - s or sc is the system of constraints.
 * - ofl_ctrl is the way overflow errors are handled
 *     ofl_ctrl == NO_OFL_CTRL
 *               -> overflow errors are not handled
 *     ofl_ctrl == OFL_CTRL
 *               -> overflow errors are handled in the called function
 *     ofl_ctrl == FWD_OFL_CTRL
 *               -> overflow errors must be handled by the calling function
 * - ofl_res is the result of the feasibility test when ofl_ctrl == OFL_CTRL
 *   and there is an overflow error.
 * - integer_p (low_level function only) is a boolean :
 *     integer_p == TRUE to test if there exists at least one integer point 
 *               in the convex polyhedron defined by the system of constraints.
 *     integer_p == FALSE to test if there exists at least one rational point 
 *               in the convex polyhedron defined by the system of constraints.
 *     (This has an impact only upon Fourier-Motzkin feasibility test).
 *
 * Last modified by Beatrice Creusillet, 13/12/94.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include "boolean.h"
#include "arithmetique.h"
#include "vecteur.h"
#include "contrainte.h"
#include "sc.h"
#include "sc-private.h"

/* 
 * INTERFACES
 */

boolean 
sc_rational_feasibility_ofl_ctrl(sc, ofl_ctrl, ofl_res)
Psysteme sc;
int ofl_ctrl;
boolean ofl_res;
{
    return sc_feasibility_ofl_ctrl(sc, FALSE, ofl_ctrl, ofl_res);
}

boolean 
sc_integer_feasibility_ofl_ctrl(sc,ofl_ctrl, ofl_res)
Psysteme sc;
int ofl_ctrl;
boolean ofl_res;
{
    return sc_feasibility_ofl_ctrl(sc, TRUE, ofl_ctrl, ofl_res);
}

/*
 * LOW LEVEL FUNCTIONS 
 */
/*  just a test to improve the Simplex/FM decision.
 * c is a list of constraints, equalities or inequalities
 * pc is the number of constraints in the list
 * pv is the number of non-zero coefficients in the system
 *
 * pc and pv MUST be initialized. They are multiplied by weight.
 */
static void 
decision_data(c, pc, pv, weight)
Pcontrainte c;
int *pc, *pv, weight;
{
    Pvecteur v;
    
    for(; c!=NULL; c=c->succ)
    {
	v=c->vecteur;
	if (v!=NULL) 
	{
	    (*pc)+=weight;
	    for(; v!=NULL; v=v->succ) 
		if (var_of(v)!=TCST) (*pv)+=weight;
	}
    }
}

boolean 
sc_feasibility_ofl_ctrl(sc, integer_p, ofl_ctrl, ofl_res)
Psysteme sc;
boolean integer_p;
int ofl_ctrl;
boolean ofl_res;
{
    int
	n_var = sc->dimension,
	n_cont=0, n_ref=0;
    boolean 
	ok = FALSE,
	use_simplex = FALSE;

    decision_data(sc_egalites(sc), &n_cont, &n_ref, 2);
    decision_data(sc_inegalites(sc), &n_cont, &n_ref, 1);

    use_simplex = (n_cont >= NB_CONSTRAINTS_MAX_FOR_FM || 
		   (n_cont>=10 && n_ref>2*n_cont));

    if (sc_rn_p(sc)) 
	return TRUE;

    /* else
     */
    switch (ofl_ctrl) 
    {
    case OFL_CTRL :
	ofl_ctrl = FWD_OFL_CTRL;
	CATCH(overflow_error) {
	    ok = ofl_res;
	    /* 
	     *   PLEASE do not remove this warning.
	     *
	     *   FC 30/01/95
	     */
	    fprintf(stderr, "[sc_feasibility_ofl_ctrl] "
		    "arithmetic error (%s[%d,%d,%d]) -> %s\n",
		    use_simplex ? "Simplex" : "Fourier-Motzkin", 
		    n_var, n_cont, n_ref,
		    ofl_res ? "TRUE" : "FALSE");
	    break;
	}		
    default:
	if (use_simplex)
	{
	    ok = sc_simplexe_feasibility_ofl_ctrl(sc, ofl_ctrl);
	}
	else 
	{
	    ok = sc_fourier_motzkin_feasibility_ofl_ctrl(sc, integer_p, 
							 ofl_ctrl);
	}
    }

    return(ok);
}

/* chose the next variable in base b for projection in system s.
 * tries to avoid Fourier potential explosions when combining inequalities.
 * - if there are equalities, chose the var with the min |coeff| (not null)
 * - if there are only inequalities, chose the var that will generate the
 *   minimum number of constraints with pairwise combinations.
 *
 * (c) FC 21 July 1995
 */
static Variable
chose_variable_to_project_for_feasability(s, b)
Psysteme s;
Pbase b;
{
    Pcontrainte c = sc_egalites(s);
    Pvecteur v;
    Variable var;
    Value val;
    int size = vect_size(b);

    ifscdebug(8)
    {
	fprintf(stderr, "[chose_variable_to_project_for_feasability] b/s:\n");
	vect_fprint(stderr, b, default_variable_to_string);
	sc_fprint(stderr, s, default_variable_to_string);
    }

    if (size==1) return var_of(b);
    assert(size>1);
    
    if (c)
    {
	/* find the lowest coeff 
	 */
	Variable minvar = TCST;
	Value minval = VALUE_ZERO;

	for (; c; c=c->succ)
	{
	    for (v = contrainte_vecteur(c); v; v=v->succ)
	    {
		 var = var_of(v);

		 if (var!=TCST)
		 {
		     val = value_abs(val_of(v));
		     if ((value_notzero_p(minval) && value_lt(val,minval))
			 || value_zero_p(minval)) 
			 minval = val, minvar = var;
		     
		     if (value_one_p(minval)) return minvar;
		 }
	    }
	}

	assert(minvar!=TCST); /* shouldn't find empty equalities ?? */
	var = minvar;
    }
    else
    {
	/* only inequalities, reduce the explosion
	 */
	int i;
	two_int_infop t = (two_int_infop) malloc(2*size*sizeof(int));
	Pbase tmp;
	int min_new;

	c = sc_inegalites(s);

	/* initialize t
	 */
	for (i=0; i<size; i++) t[i][0]=0, t[i][1]=0;

	/* t[x][0 (resp. 1)] = number of negative (resp. positive) coeff
	 */
	for (; c; c=c->succ)
	{
	    for (v = contrainte_vecteur(c); v; v=v->succ)
	    {
		var = var_of(v); 
		if (var!=TCST)
		{
		    ifscdebug(9) 
			fprintf(stderr, "%s\n", 
				default_variable_to_string(var));

		    for (i=0, tmp=b; tmp && var_of(tmp)!=var; 
			 i++, tmp=tmp->succ);
		    assert(tmp);
		    
		    t[i][value_posz_p(val_of(v))]++;
		}
	    }
	}

	/* t[x][0] = number of combinations, i.e. new created constraints.
	 */
	for (i=0; i<size; i++) t[i][0] *= t[i][1];

	for (tmp=b->succ, var=var_of(b), min_new=t[0][0], i=1;
	     min_new && i<size; 
	     i++, tmp=tmp->succ) {
	    if (t[i][0]<min_new)
		min_new = t[i][0], var=var_of(tmp); 
	}

	free(t);
    }

    ifscdebug(8)
	fprintf(stderr, "[chose_variable_to_project_for_feasability] "
		"suggesting %s\n", default_variable_to_string(var));

    return var;
}

/* boolean sc_fourier_motzkin_faisabilite_ofl(Psysteme s):
 * test de faisabilite d'un systeme de contraintes lineaires, par projections
 * successives du systeme selon les differentes variables du systeme
 *
 *  resultat retourne par la fonction :
 *
 *  boolean	: TRUE si le systeme est faisable
 *		  FALSE sinon
 *
 *  Les parametres de la fonction :
 *
 *  Psysteme s    : systeme lineaire 
 *
 * Le controle de l'overflow est effectue et traite par le retour 
 * du contexte correspondant au dernier CATCH(overflow_error) effectue.
 */
boolean 
sc_fourier_motzkin_feasibility_ofl_ctrl(s, integer_p, ofl_ctrl)
Psysteme s;
boolean integer_p;
int ofl_ctrl;
{
    Psysteme s1;
    boolean faisable = TRUE;
    extern jmp_buf overflow_error;

    if (s == NULL) return TRUE;
    s1 = sc_dup(s);

    ifscdebug(8)
    {
	fprintf(stderr, "[sc_fourier_motzkin_feasibility_ofl_ctrl] system:\n");
	sc_fprint(stderr, s1, default_variable_to_string);
    }

    s1 = sc_elim_db_constraints(s1);

    if (s1 != NULL)
    {
	/* projection successive selon les  variables du systeme
	 */
	Variable var;
	Pbase b = base_dup(sc_base(s1));
	
	while (b && faisable)
	{
	    var = chose_variable_to_project_for_feasability(s1, b);
	    vect_erase_var(&b, var);

	    ifscdebug(8)
	    {
		fprintf(stderr, "sc_fourier_motzkin_feasibility_ofl_ctrl]"
			" system before %s projection:\n", var);
		sc_fprint(stderr, s1, default_variable_to_string);
	    }
	    
	    sc_projection_along_variable_ofl_ctrl(&s1, var, ofl_ctrl);

	    ifscdebug(8)
	    {
		fprintf(stderr, "sc_fourier_motzkin_feasibility_ofl_ctrl]"
			" system after projection:\n");
		sc_fprint(stderr, s1, default_variable_to_string);
	    }
	    
	    if (sc_empty_p(s1))
	    {
		faisable = FALSE;
		break;
	    }

	    if (integer_p) {
		s1 = sc_normalize(s1);
		if (SC_EMPTY_P(s1)) 
		{ 
		    faisable = FALSE; 
		    break;
		}
	    }
	}
	
	sc_rm(s1);
	base_rm(b);
    }
    else 
	/* sc_kill_db_eg a de'sallouer s1 a` la detection de 
	   sa non-faisabilite */
	faisable = FALSE;

    return faisable;
}



