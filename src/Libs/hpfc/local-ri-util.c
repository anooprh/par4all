/* $Id$
 *
 * HPFC (c) Fabien Coelho May 1993
 */

#include "defines-local.h"

type type_variable_dup(t)
type t;
{
    if(type_variable_p(t))
    {
	variable v = type_variable(t);

	return MakeTypeVariable(variable_basic(v),
				ldimensions_dup(variable_dimensions(v)));
    }
    else
	return t; /* !!! means sharing */
}

/*  library functions...
 */
static string fortran_library[] =
{ 
  "TIME",
  (string) NULL
} ;

bool fortran_library_entity_p(e)
entity e;
{
    string *s, name=entity_local_name(e);

    if (!top_level_entity_p(e)) return(FALSE);
    for (s=fortran_library; *s!=(string) NULL; s++)
	if (same_string_p(*s, name)) return TRUE;

    return FALSE; /* else not found */
}

reference expression_to_reference(e)
expression e;
{
    syntax s = expression_syntax(e);
    message_assert("reference", syntax_reference_p(s));
    return syntax_reference(s);
}

list expression_list_to_entity_list(l)
list /* of expressions */ l;
{
    list /* of entities */ n = NIL;
    MAP(EXPRESSION, e, n = CONS(ENTITY, expression_to_entity(e), n), l);
    return gen_nreverse(n);		 
}

list entity_list_to_expression_list(list /* of entities */ l)
{
    list /* of expressions */ n = NIL;
    MAP(ENTITY, e, n = CONS(EXPRESSION, entity_to_expression(e), n), l);
    return gen_nreverse(n);
}

/* that is all
 */
