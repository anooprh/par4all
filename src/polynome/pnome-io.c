/******************************************************************** pnome-io.c
 *
 * POLYNOMIAL INPUT/OUTPUT FUNCTIONS
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "boolean.h"
#include "vecteur.h"
#include "polynome.h"


/* void float_to_frac(float x, char** ps)
 *  PRIVATE
 *  returns the simplest representation of floating-point number x
 *  tries to make an integer, then a small fraction, then a floating-point,
 *  then a floating-point with exponent.
 */
void float_to_frac(x, ps)
char **ps;
float x;
{
    int i;
    float fprecision = (float) intpower(10.0, -PNOME_FLOAT_N_DECIMALES);

    if (((x <= fprecision) && (x >= -fprecision)) ||          /* if too little */
	((x>PNOME_FLOAT_TO_EXP_LEVEL) | (x<-PNOME_FLOAT_TO_EXP_LEVEL)))/* or too big */
	sprintf(*ps, "%.*E", PNOME_FLOAT_N_DECIMALES, x); /* print it with an exponent */
    else {
	sprintf(*ps, "%.*f", PNOME_FLOAT_N_DECIMALES, x);/* default printing: as a float */
	for (i=1; i<PNOME_FLOAT_TO_FRAC_LEVEL; i++)
	    if ((((x*i) - ((int) (x*i+0.5))) < (fprecision)) &&  /* if x is close enough */
		(((x*i) - ((int) (x*i+0.5))) > (-fprecision))) { /* to a little fraction */
		if ((((int) (x*i+0.5)) < PNOME_FLOAT_TO_FRAC_LEVEL) || (i=1))
		    /*print it as a fraction */
		    if (i==1)
			sprintf(*ps, "%d", (int) (x*i+0.5));
		    else
			sprintf(*ps, "%d/%d", (int) (x*i+0.5), i);
		break;
	    }
    }
}

/* void monome_fprint(FILE* fd, Pmonome pm, Pbase pb, boolean plus_sign, char* (*variable_name)())
 *  PRIVATE
 *  Outputs to file fd an ASCII form of monomial pm, naming variables
 *  with the "variable-name" function, ordering them with the basis pb.
 *  the "+" sign is printed if plus_sign == TRUE.
 */
void monome_fprint(fd, pm, pb, plus_sign, variable_name)
FILE *fd;
Pmonome pm;
Pbase pb;
boolean plus_sign;
char * (*variable_name)();
{
    char *s = monome_sprint(pm, pb, plus_sign, variable_name);

    fprintf(fd, "%s", s);
    free(s);
}

/* char *monome_sprint(Pmonome pm, Pbase pb, boolean plus_sign, char* (*variable_name)())
 *  PRIVATE
 *  Outputs a string representing monomial pm, naming variables
 *  with the "variable-name" function, ordering them with the basis pb.
 *  the "+" sign is printed if plus_sign == TRUE.
 */
char *monome_sprint(pm, pb, plus_sign, variable_name)
Pmonome pm;
Pbase pb;
boolean plus_sign;
char * (*variable_name)();
{
    float x;
    char t[99];
    char *r, *s;
    char *u;

    u = (char *) malloc(99);
    r = t;

    if (MONOME_UNDEFINED_P(pm))
	sprintf(r, "%s", MONOME_UNDEFINED_SYMBOL);
    else {
	x = monome_coeff(pm);
	if (x==0) 
	    sprintf(r, "%s", MONOME_NUL_SYMBOL);
	else {
	    if (x<0) {
		*(r++) = '-';
		if (plus_sign) 
		    *(r++) = ' ';
		x = - x; 
	    }
	    else if (plus_sign) {
		*(r++) = '+';
		*(r++) = ' ';
	    }

	    float_to_frac(x, &u);

	    if (vect_coeff(TCST, monome_term(pm)) == 0) {
		if (x != 1) {
		    sprintf(r, "%s%s", u, MONOME_COEFF_MULTIPLY_SYMBOL);
		    r = strchr(r, NULL);
		}
		s = vect_sprint_as_monome(monome_term(pm), pb,
					  variable_name, MONOME_VAR_MULTIPLY_SYMBOL);
		strcpy(r, s);
		free(s);
	    }
	    else 
		sprintf(r, "%s", u);
	}
    }
    free(u);
    return(strdup((char *) t));
}

/* void polynome_fprint(FILE* fd, Ppolynome pp,
 *                      char* (*variable_name)(), 
 *                      boolean (*is_inferior_var)())
 *  Outputs to file fd an ASCII form of polynomial pp, using
 *  the user-provided function variable_name(Variable var) to associate
 *  the "Variable" pointers with the variable names.
 *  is_inferior_var(Variable var1, Variable var2) is also given by the user:
 *  it must return TRUE if var1 must be printed before var2 in monomials.
 *
 *  For the moment, monomials are not sorted.
 *  No "\n" is printed after the polynomial.
 */
void polynome_fprint(fd, pp, variable_name, is_inferior_var)
FILE *fd;
Ppolynome pp;
char * (*variable_name)();
boolean (*is_inferior_var)();
{
    char *s = polynome_sprint(pp, variable_name, is_inferior_var);

    fprintf(fd, "%s", s);
    free(s);
}

/* char *polynome_sprint(Ppolynome pp,
 *                      char* (*variable_name)(), 
 *                      boolean (*is_inferior_var)())
 *  Outputs to file fd an ASCII form of polynomial pp, using
 *  the user-provided function variable_name(Variable var) to associate
 *  the "Variable" pointers with the variable names.
 *  is_inferior_var(Variable var1, Variable var2) is also given by the user:
 *  it must return TRUE if var1 must be printed before var2 in monomials.
 *
 *  For the moment, monomials are not sorted.
 *  No "\n" is printed after the polynomial.
 */
char *polynome_sprint(pp, variable_name, is_inferior_var)
Ppolynome pp;
char * (*variable_name)();
boolean (*is_inferior_var)();
{
#define POLYNOME_BUFFER_SIZE 1024
    static char t[POLYNOME_BUFFER_SIZE];
    char *r, *s;

    r = t;

    if (POLYNOME_UNDEFINED_P(pp))
	sprintf(r, "%s", POLYNOME_UNDEFINED_SYMBOL);
    else if (POLYNOME_NUL_P(pp))
	sprintf(r, "%s", POLYNOME_NUL_SYMBOL);
    else {
	Pbase pb = (Pbase) polynome_used_var(pp, is_inferior_var);
	boolean print_plus_sign = FALSE;

	/* The following line is added by L.Zhou    Mar. 26, 91 */
	pp = polynome_sort(&pp, is_inferior_var);

	while (!POLYNOME_NUL_P(pp)) {
	    s =	monome_sprint(polynome_monome(pp), pb, print_plus_sign, variable_name);
	    strcpy(r, s);
	    r = strchr(r, NULL);
	    pp = polynome_succ(pp);
	    print_plus_sign = TRUE;
	    if (!POLYNOME_NUL_P(pp)) *(r++) = ' ';
	    free(s);
	}
    }
    assert( strlen(t) < POLYNOME_BUFFER_SIZE);
    return (strdup((char *) t));
}


/* char *default_variable_name(Variable var)
 *  returns for variable var the name "Vxxxx" where xxxx are
 *  four letters computed form (int) var.
 */
char *default_variable_name(var)
Variable var;
{
    char *s = (char *) malloc(6);
    int i = (int) var;
    
    if (var != TCST) {
	sprintf(s, "V%c%c%c%c",
		(char) 93 + (i % 26),
		(char) 93 + ((i / 26) % 26),
		(char) 93 + ((i / 26 / 26) % 26),
		(char) 93 + ((i / 26 / 26 / 26) % 26));
    }
    else 
	sprintf(s, "TCST");
    return(s);
}


/* boolean default_is_inferior_var(Variable var1, Variable var2)
 *  return TRUE if var1 is before var2, lexicographically,
 *  according to the "default_variable_name" naming.
 */
boolean default_is_inferior_var(var1, var2)
Variable var1, var2;
{
    return (0 < strcmp(default_variable_name(var1), default_variable_name(var2)));
}

static void remove_blanks(ps)
char **ps;
{
    char *s = *ps,
         *t = *ps;

    do {
	while (isspace(**ps)) 
	    (*ps)++;
	*t++ = *(*ps)++;
    } while (**ps != '\0');
    *t++ = '\0';
    *ps = s;
}

static float parse_coeff(ps)
char **ps;
{
    float coeff = 0;

    if (isdigit(**ps) || (**ps == '.') || (**ps == '+') || (**ps == '-')) {
	sscanf(*ps, "%f", &coeff);
	if ((coeff == 0) && (**ps == '+')) 
	    coeff = 1;
	if ((coeff == 0) && (**ps == '-')) 
	    coeff = -1;
    }
    else 
	coeff = 1;

    if ((**ps == '-') || (**ps == '+')) 
	(*ps)++;
    while (isdigit(**ps) || (**ps == '.') || (**ps == '*')) 
	(*ps)++;
    
    if (**ps == '/') {  /* handle fractionnal coefficients */
	float denom;
	(*ps)++;
	denom = parse_coeff(ps);
	coeff /= denom;
    }

    return(coeff);
}

static char *parse_var_name(ps)
char **ps;
{
    char *name, *n;

    name = (char *) malloc(MAX_NAME_LENGTH);
    n = name;

    if (isalpha(**ps))                      /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	do { *n++ = *((*ps)++); }
    while (isalnum(**ps) || (**ps == ':')); /* THE ':' STANDS FOR MODULE_SEP_STRING OF PIPS */
                                            /* TO BE ABLE TO HANDLE VARIABLES SUCH AS P:I   */
    *n = '\0';

    return(name);
}

/* Ppolynome polynome_sscanf(char *sp, (*name_to_variable)())
 *  converts into polynomial structure the expression passed
 *  in ASCII form in string sp. (for debug only)
 *  pas vraiment quick mais bien dirty
 */
Ppolynome polynome_sscanf(sp, name_to_variable)
char *sp;
Variable (*name_to_variable)();
{
    Ppolynome pp = POLYNOME_NUL;
    Pmonome curpm;
    boolean constructing_monome = FALSE;
    float coeff;
    char *varname;
    int power;
    char *s;

    s = strdup(sp);
    remove_blanks(&s);

    while (*s != '\0')
    {
	/*fprintf(stderr, "\ns='%s'\n", s);*/
	power = 1;
	if (!constructing_monome) { coeff = parse_coeff(&s);
				    /*fprintf(stderr, "coeff = %f\n", coeff); */
				}
	varname = parse_var_name(&s);
	if (strlen(varname)!=0) {
	    if (*s == '^') {
		s++;
		power = parse_coeff(&s);
	    }
	    else 
		while ((*s == '.') || (*s == '*')) s++;
	}
	else varname = strdup("TCST");

	if (constructing_monome) {
	    vect_add_elem(&(monome_term(curpm)), name_to_variable(varname), power);

	    /*
	      fprintf(stderr, "vect_add_elem  '%s' : #%d               --> monome ", varname, (int) name_to_variable(varname));
	      monome_fprint(stderr, curpm, BASE_NULLE, FALSE, default_variable_name);
	      fprintf(stderr, "\n");
	      */
	}
	else {
	    curpm = make_monome(coeff, name_to_variable(varname), power);
	    /*
	      fprintf(stderr, "new monome: ");
	      monome_fprint(stderr, curpm, BASE_NULLE, FALSE, default_variable_name);
	      fprintf(stderr, "\n");
	      */
            constructing_monome = TRUE;
	}
	/*fprintf(stderr, "au milieu: s='%s'\n", s);*/

	if ((*s == '+') || (*s == '-'))
	{
	    /*
	      fprintf(stderr, "adding monome ");
	      monome_fprint(stderr, curpm, BASE_NULLE, FALSE, default_variable_name);
	      */
	    polynome_monome_add(&pp, curpm);
	    /*
	      fprintf(stderr, "\n--> polynome ");
	      polynome_fprint(stderr, pp, default_variable_name, default_is_inferior_var);
	      fprintf(stderr, "\n");
	      */
	    monome_rm(&curpm);
            constructing_monome = FALSE;
	}
    }
    if (!MONOME_NUL_P(curpm)) {
	polynome_monome_add(&pp, curpm);
	monome_rm(&curpm);
    }

    return (pp);
}
