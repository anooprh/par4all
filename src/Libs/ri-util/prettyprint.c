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

#ifndef lint
char lib_ri_util_prettyprint_c_rcsid[] = "$Id$";
#endif /* lint */

 /*
  * Prettyprint all kinds of ri related data structures
  *
  *  Modifications:
  * - In order to remove the extra parentheses, I made the several changes:
  * (1) At the intrinsic_handler, the third term is added to indicate the
  *     precendence, and accordingly words_intrinsic_precedence(obj) is built
  *     to get the precedence of the call "obj".
  * (2) words_subexpression is created to distinguish the
  *     words_expression.  It has two arguments, expression and
  *     precedence. where precedence is newly added. In case of failure
  *     of words_subexpression , that is, when
  *     syntax_call_p is FALSE, we use words_expression instead.
  * (3) When words_call is firstly called , we give it the lowest precedence,
  *        that is 0.
  *    Lei ZHOU           Nov. 4, 1991
  *
  * - Addition of CMF and CRAFT prettyprints. Only text_loop() has been
  * modified.
  *    Alexis Platonoff, Nov. 18, 1994

  * - Modifications of sentence_area to deal with  the fact that
  *   " only one appearance of a symbolic name as an array name in an
  *     array declarator in a program unit is permitted."
  *     (Fortran standard, number 8.1, line 40)
  *   array declarators now only appear with the type declaration, not with the
  *   area. - BC - october 196.
  * - Modification of text_entity_declaration to ensure that the OUTPUT of PIPS
  *   can also be used as INPUT; in particular, variable declarations must
  *   appear
  *   before common declarations. BC.
  * - neither are DATA statements for non integers (FI/FC)
  * - Also, EQUIVALENCE statements are not generated for the moment. BC.
  *     Thay are now??? FC?
  */

// To have asprintf:
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "linear.h"

#include "genC.h"
#include "text.h"
#include "text-util.h"
#include "ri.h"
#include "ri-util.h"

#include "pipsdbm.h"

#include "misc.h"
#include "properties.h"
#include "prettyprint.h"
#include "preprocessor.h"

#include "STEP_name.h"

/* operator precedences are in the [0,100] range */

#define MAXIMAL_PRECEDENCE 100
#define MINIMAL_ARITHMETIC_PRECEDENCE 19

/* Define the markers used in the raw unstructured output when the
   PRETTYPRINT_UNSTRUCTURED_AS_A_GRAPH property is true: */
#define PRETTYPRINT_UNSTRUCTURED_BEGIN_MARKER "\200Unstructured"
#define PRETTYPRINT_UNSTRUCTURED_END_MARKER "\201Unstructured End"
#define PRETTYPRINT_UNSTRUCTURED_ITEM_MARKER "\202Unstructured Item"
#define PRETTYPRINT_UNSTRUCTURED_SUCC_MARKER "\203Unstructured Successor ->"
#define PRETTYPRINT_UNREACHABLE_EXIT_MARKER "\204Unstructured Unreachable"


/*===================== Variables and Function prototypes for C ===========*/

/* This variable should be made static and accessed from other files
   or at least from other libraries via its functions only */
bool prettyprint_is_fortran = TRUE;

/* To track accesses from other libraries */


/** Get the status of the Fortran prettyprint flag */
bool get_prettyprint_is_fortran()
{
  return prettyprint_is_fortran;
}

/** Select if the prettyprint is done in Fortran or not */
void set_prettyprint_is_fortran_p(bool is_fortran_p)
{
  prettyprint_is_fortran = is_fortran_p;
}

/** Select Fortran prettyprint */
void set_prettyprint_is_fortran()
{
  prettyprint_is_fortran = TRUE;
}

/** Deselect Fortran prettyprint */
void reset_prettyprint_is_fortran()
{
  prettyprint_is_fortran = FALSE;
}

static list words_cast(cast obj);
static list words_sizeofexpression(sizeofexpression obj);
static list words_subscript(subscript s);
static list words_application(application a);
static text text_forloop(entity module,string label,int margin,forloop obj,int n);
static text text_forloop(entity module,string label,int margin,forloop obj,int n);

/* This variable is used to disable the precedence system and hence to
   prettyprint all parentheses, which let the prettyprint reflect the
   AST. */
static bool precedence_p = TRUE;

/******************************************************************* STYLES */

static bool
pp_style_p(string s)
{
    return same_string_p(get_string_property(PRETTYPRINT_PARALLEL), s);
}

#define pp_hpf_style_p() 	pp_style_p("hpf")
#define pp_f90_style_p() 	pp_style_p("f90")
#define pp_craft_style_p() 	pp_style_p("craft")
#define pp_cray_style_p() 	pp_style_p("cray")
#define pp_cmf_style_p()	pp_style_p("cmf")
#define pp_doall_style_p()	pp_style_p("doall")
#define pp_do_style_p()		pp_style_p("do")
#define pp_omp_style_p()	pp_style_p("omp")

/********************************************************************* MISC */

text empty_text(entity __attribute__ ((unused)) e,
		int __attribute__ ((unused)) m,
		statement __attribute__ ((unused)) s) {
  return make_text(NIL);
}

static text (*text_statement_hook)(entity, int, statement) = empty_text;

void init_prettyprint( hook )
text (*hook)(entity, int, statement) ;
{
    /* checks that the prettyprint hook was actually reset...
     */
    pips_assert("prettyprint hook not set", text_statement_hook==empty_text);
    text_statement_hook = hook ;
}

/* because some prettyprint functions may be used for debug, so
 * the last hook set by somebody may have stayed there although
 * being non sense...
 */
void close_prettyprint()
{
    text_statement_hook = empty_text;
}


/* Can this statement be printed on one line, without enclosing braces? */
bool one_liner_p(statement s)
{
  instruction i = statement_instruction(s);
  bool yes = (instruction_test_p(i) || instruction_loop_p(i) || instruction_whileloop_p(i)
	      || instruction_call_p(i) || instruction_forloop_p(i) || instruction_goto_p(i)
	      || return_instruction_p(i));

  yes = yes  && ENDP(statement_declarations(s));

  if(!yes && instruction_sequence_p(i)) {
    list sl = sequence_statements(instruction_sequence(i));
    int sc = gen_length(sl);

    if(sc==1) {
      /* There may be many lines hidden behind another block construct
	 when code is generated in a non canonical way as for
	 {{x=1;y=2;}} */
      instruction ii = statement_instruction(STATEMENT(CAR(sl)));

      if(instruction_sequence_p(ii)) {
	/* OK, we could check deeper, but this is only useful for
	   redundant internal representations. Let's forget about
	   niceties such as skipping useless braces. */
	yes = FALSE;
      }
      else
	yes = ENDP(statement_declarations(s));
    }
    else
      yes = (sc < 1) && ENDP(statement_declarations(s));
  }

  return yes;
}

/***************************************************local varibles handling */

static text local_var;
static bool local_flg = false;

// This function either append the declaration to the text given as a parameter
// or return a new text with  the declaration
static text insert_locals (text r) {
  if (local_flg == true) {
    if ((r != text_undefined) && (r != NULL)){
      MERGE_TEXTS (r, local_var);
    }
    else {
      r = local_var;
    }
    local_flg = false;
  }
  return r;
}

// This function return true if BLOCK boundary markers are required.
// The function also create the maker when needed.
static bool mark_block (unformatted* t_beg, unformatted* t_end, int n, int margin) {
  bool result = false;
  if (!get_bool_property("PRETTYPRINT_FOR_FORESYS") &&
      (get_bool_property("PRETTYPRINT_ALL_EFFECTS") ||
       get_bool_property("PRETTYPRINT_BLOCKS")))
    result = true;
  if (result == true) {
    // here we need to generat block marker for later use
    if (prettyprint_is_fortran == true) {
      // fortran case: comments at the begin of the line
      list pbeg = CHAIN_SWORD (NIL, "BEGIN BLOCK");
      list pend = CHAIN_SWORD (NIL, "END BLOCK");
      *t_beg = make_unformatted (strdup (PIPS_COMMENT_SENTINEL), n, margin, pbeg);
      *t_end = make_unformatted (strdup (PIPS_COMMENT_SENTINEL), n, margin, pend);
    } else {
      // C case: comments alligned with blocks
      list pbeg = CHAIN_SWORD (NIL, strdup (PIPS_COMMENT_SENTINEL));
      list pend = CHAIN_SWORD (NIL, strdup (PIPS_COMMENT_SENTINEL));
      pbeg = CHAIN_SWORD (pbeg, " BEGIN BLOCK");
      pend = CHAIN_SWORD (pend, " END BLOCK");
      *t_beg = make_unformatted (NULL, n, margin, pbeg);
      *t_end = make_unformatted (NULL, n, margin, pend);
    }
  }
  return result;
}

/********************************************************************* WORDS */

static int words_intrinsic_precedence(call);
static int intrinsic_precedence(string);

/* exported for craft
 */
list
words_loop_range(range obj)
{
    list pc;
    call c = syntax_call(expression_syntax(range_increment(obj)));

    pc = words_subexpression(range_lower(obj), 0, TRUE);
    pc = CHAIN_SWORD(pc,", ");
    pc = gen_nconc(pc, words_subexpression(range_upper(obj), 0, TRUE));
    if (/*  expression_constant_p(range_increment(obj)) && */
	 strcmp( entity_local_name(call_function(c)), "1") == 0 )
	return(pc);
    pc = CHAIN_SWORD(pc,", ");
    pc = gen_nconc(pc, words_expression(range_increment(obj)));

    return(pc);
}


/* Output a Fortan-like do-loop range as a C-like for-loop index part.
   Assume that the increment is an integer so we can generate the good
   condition. Since the do-loops are recognized in C program part only
   with this assumptions, it is a good assumption.
*/
list C_loop_range(range obj, entity i)
{
    list pc;
    /* call c = syntax_call(expression_syntax(range_increment(obj))); */

    /* Complete the initialization assignment */
    pc = words_subexpression(range_lower(obj), 0, TRUE);
    pc = CHAIN_SWORD(pc,"; ");

    /* Check the final bound */
    pc = CHAIN_SWORD(pc, entity_user_name(i));

    /* Increasing or decreasing index? */
    expression inc = range_increment(obj);
    // Assume the increment has an integer value with a known sign
    if (expression_negative_integer_value_p(inc))
      /* The increment is negative, that means the index is tested against
	 a lower bound: */
      pc = CHAIN_SWORD(pc," >= ");
    else
      /* Else we assume to test against an upper bound: */
      pc = CHAIN_SWORD(pc," <= ");

    pc = gen_nconc(pc, words_subexpression(range_upper(obj), 0, TRUE));
    pc = CHAIN_SWORD(pc,"; ");

    /* Increment the loop index */
    pc = CHAIN_SWORD(pc, entity_user_name(i));
    pc = CHAIN_SWORD(pc," += ");
    pc = gen_nconc(pc, words_expression(inc));
    pc = CHAIN_SWORD(pc,")");

    return(pc);
}

list /* of string */
words_range(range obj)
{
    list pc = NIL ;

    /* if undefined I print a star, why not!? */
    if (expression_undefined_p(range_lower(obj))) {
	pc = CONS(STRING, MAKE_SWORD("*"), NIL);
    }
    else {
	call c = syntax_call(expression_syntax(range_increment(obj)));

	  pc = CHAIN_SWORD(pc,"(/ (I,I=");
	  pc = gen_nconc(pc, words_expression(range_lower(obj)));
	  pc = CHAIN_SWORD(pc,",");
	  pc = gen_nconc(pc, words_expression(range_upper(obj)));
	if(strcmp( entity_local_name(call_function(c)), "1") != 0) {
	  pc = CHAIN_SWORD(pc,",");
	  pc = gen_nconc(pc, words_expression(range_increment(obj)));
	}
	  pc = CHAIN_SWORD(pc,") /)") ;
    }
    return pc;
}

/* FI: array constructor R433, p. 37 in Fortran 90 standard, can
   be used anywhere in arithmetic expressions whereas the triplet
   notation is restricted to subscript expressions. The triplet
   notation is used to define array sections (see R619, p. 64).
*/

list /* of string */
words_subscript_range(range obj)
{
    list pc = NIL ;

    /* if undefined I print a star, why not!? */
    if (expression_undefined_p(range_lower(obj))) {
	pc = CONS(STRING, MAKE_SWORD("*"), NIL);
    }
    else {
	call c = syntax_call(expression_syntax(range_increment(obj)));

	pc = gen_nconc(pc, words_expression(range_lower(obj)));
	pc = CHAIN_SWORD(pc,":");
	pc = gen_nconc(pc, words_expression(range_upper(obj)));
	if(strcmp( entity_local_name(call_function(c)), "1") != 0) {
	    pc = CHAIN_SWORD(pc,":");
	    pc = gen_nconc(pc, words_expression(range_increment(obj)));
	}
    }
    return pc;
}

/* exported for expression.c
 */
list
words_reference(reference obj)
{
  list pc = NIL;
  string begin_attachment;

  entity e = reference_variable(obj);

  pc = CHAIN_SWORD(pc, entity_user_name(e));
  begin_attachment = STRING(CAR(pc));

  if (reference_indices(obj) != NIL) {
    if (prettyprint_is_fortran)
      {
	pc = CHAIN_SWORD(pc,"(");
	MAPL(pi, {
	  expression subscript = EXPRESSION(CAR(pi));
	  syntax ssubscript = expression_syntax(subscript);

	  if(syntax_range_p(ssubscript)) {
	    pc = gen_nconc(pc, words_subscript_range(syntax_range(ssubscript)));
	  }
	  else {
	    pc = gen_nconc(pc, words_subexpression(subscript, 0, TRUE));
	  }

	  if (CDR(pi) != NIL)
	    pc = CHAIN_SWORD(pc,",");
	}, reference_indices(obj));
	pc = CHAIN_SWORD(pc,")");
      }
    else
      {
	MAPL(pi, {
	  expression subscript = EXPRESSION(CAR(pi));
	  syntax ssubscript = expression_syntax(subscript);
	  pc = CHAIN_SWORD(pc, "[");
	  if(syntax_range_p(ssubscript)) {
	    pc = gen_nconc(pc, words_subscript_range(syntax_range(ssubscript)));
	  }
	  else {
	    pc = gen_nconc(pc, words_subexpression(subscript, 0, TRUE));
	  }
	  pc = CHAIN_SWORD(pc, "]");
	}, reference_indices(obj));
      }
  }
  attach_reference_to_word_list(begin_attachment, STRING(CAR(gen_last(pc))),
				obj);

  return(pc);
}

/* Management of alternate returns */

static list set_of_labels_required_for_alternate_returns = list_undefined;

void set_alternate_return_set()
{
  ifdebug(1) {
  pips_assert("The target list is undefined",
	      list_undefined_p(set_of_labels_required_for_alternate_returns));
  }
  set_of_labels_required_for_alternate_returns = NIL;
}

void reset_alternate_return_set()
{
  ifdebug(1) {
  pips_assert("The target list is initialized",
	      !list_undefined_p(set_of_labels_required_for_alternate_returns));
  }
  gen_free_list(set_of_labels_required_for_alternate_returns);
  set_of_labels_required_for_alternate_returns = list_undefined;
}

void add_target_to_alternate_return_set(entity l)
{
  ifdebug(1) {
  pips_assert("The target list is initialized",
	      !list_undefined_p(set_of_labels_required_for_alternate_returns));
  }
  set_of_labels_required_for_alternate_returns =
    gen_once(l, set_of_labels_required_for_alternate_returns);
}

text generate_alternate_return_targets()
{
  text ral = text_undefined;

  if(!ENDP(set_of_labels_required_for_alternate_returns)) {
    list sl = NIL;
    MAP(ENTITY, le, {
      sentence s1 = sentence_undefined;
      unformatted u1 =
	make_unformatted
	(strdup(label_local_name(le)),
	 STATEMENT_NUMBER_UNDEFINED, 0,
	 CONS(STRING, strdup(prettyprint_is_fortran?"CONTINUE":";"), NIL));

      s1 = make_sentence(is_sentence_unformatted, u1);
      sl = gen_nconc(sl, CONS(SENTENCE, s1, NIL));
    }, set_of_labels_required_for_alternate_returns);
    ral = make_text(sl);
  }
  else {
    ral = make_text(NIL);
  }
  return ral;
}


/* words_regular_call used for user subroutine and user function and
   intrinsics called like user function such as MOD(). */

static list
words_regular_call(call obj, bool is_a_subroutine)
{
  list pc = NIL;

  entity f = call_function(obj);
  value i = entity_initial(f);
  type t = entity_type(f);
  bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

  if(call_arguments(obj) == NIL) {
    if(type_statement_p(t))
      return(CHAIN_SWORD(pc, entity_local_name(f)+strlen(LABEL_PREFIX)));
    if(value_constant_p(i)||value_symbolic_p(i)) {
      if(prettyprint_is_fortran)
	return(CHAIN_SWORD(pc, entity_user_name(f)));
      else {
	if(ENTITY_TRUE_P(f))
	  return(CHAIN_SWORD(pc, "true"));
	if(ENTITY_FALSE_P(f))
	  return(CHAIN_SWORD(pc, "false"));
	return(CHAIN_SWORD(pc, entity_user_name(f)));
      }
    }
  }

  if(type_void_p(functional_result(type_functional(call_compatible_type(entity_type(call_function(obj))))))) {
    if(is_a_subroutine)
      pc = CHAIN_SWORD(pc, prettyprint_is_fortran?"CALL ":"");
    else
      if(prettyprint_is_fortran) /* to avoid this warning for C*/
	pips_user_warning("subroutine '%s' used as a function.\n",
			  entity_name(f));
  }
  else if(is_a_subroutine) {
    if(prettyprint_is_fortran) /* to avoid this warning for C*/
      pips_user_warning("function '%s' used as a subroutine.\n",
			entity_name(f));
    pc = CHAIN_SWORD(pc, prettyprint_is_fortran?"CALL ":"");
  }

  /* special cases for stdarg builtin macros */
  if (ENTITY_VA_END_P(f))
    pc = CHAIN_SWORD(pc, "va_end");
  else if (ENTITY_VA_START_P(f))
    pc = CHAIN_SWORD(pc, "va_start");
  else if (ENTITY_VA_COPY_P(f))
    pc = CHAIN_SWORD(pc, "va_copy");

  /* Special cases for stdio.h */ 
  /* else if (ENTITY__IO_GETC_P(f)) */
/*     pc = CHAIN_SWORD(pc, "getc"); */
/*   else if (ENTITY__IO_PUTC_P(f)) */
/*     pc = CHAIN_SWORD(pc, "putc"); */

  /* the implied complex operator is hidden... [D]CMPLX_(x,y) -> (x,y)
   */
  else if(!ENTITY_IMPLIED_CMPLX_P(f) && !ENTITY_IMPLIED_DCMPLX_P(f))
    pc = CHAIN_SWORD(pc, entity_user_name(f));

  /* The corresponding formal parameter cannot be checked by
     formal_label_replacement_p() because the called modules may not have
     been parsed yet. */

  if(!ENDP(call_arguments(obj))) {
    list pa = list_undefined;
    pc = CHAIN_SWORD(pc, "(");

    for(pa = call_arguments(obj); !ENDP(pa); POP(pa)) {
      expression eap = EXPRESSION(CAR(pa));
      if(get_bool_property("PRETTYPRINT_REGENERATE_ALTERNATE_RETURNS")
	 && expression_call_p(eap) && actual_label_replacement_p(eap)) {
	/* Alternate return actual argument have been replaced by
           character strings by the parser. */
	entity cf = call_function(syntax_call(expression_syntax(eap)));
	string ls = entity_local_name(cf);
	string ls1 = malloc(strlen(ls));
	/* pips_assert("ls has at least four characters", strlen(ls)>=4); */

	/* Get rid of initial and final quotes */
	ls1 = strncpy(ls1, ls+1, strlen(ls)-2);
	*(ls1+strlen(ls)-2) = '\000';
	pips_assert("eap must be a call to a constant string", expression_call_p(eap));
	if(strcmp(get_string_property("PARSER_SUBSTITUTE_ALTERNATE_RETURNS"), "STOP")!=0) {
	  pc = CHAIN_SWORD(pc, ls1);
	  /* free(ls1); */
	}
	else {
	  /* The actual label cannot always be used because it might have been
             eliminated as part of dead code by PIPS since it is not used
             with the STOP option. */
	  if(label_string_defined_in_current_module_p(ls1+1)) {
	    pc = CHAIN_SWORD(pc, ls1);
	  }
	  else {
	    entity els1 = find_label_entity(get_current_module_name(), ls1+1);

	    /* The assertion may be wrong if this piece of code is used to
               print intermediate statements */
	    pips_assert("Label els1 has been defined although it is not used anymore",
			!entity_undefined_p(els1));

	    pc = CHAIN_SWORD(pc, ls1);
	    add_target_to_alternate_return_set(els1);
	  }
	}
      }
      else {
	/* words_expression cannot be called because of the C comma
	   operator which require surrounding parentheses in this
	   context. Be careful with unary minus. */
	pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(pa)),
					       ASSIGN_OPERATOR_PRECEDENCE,
					       TRUE/*FALSE*/));
      }
      if (CDR(pa) != NIL)
	pc = CHAIN_SWORD(pc, space_p? ", ": ",");
    }

    pc = CHAIN_SWORD(pc, ")");
  }
  else if(!type_void_p(functional_result(type_functional(t))) ||
	  !is_a_subroutine || !prettyprint_is_fortran) {
    pc = CHAIN_SWORD(pc, "()");
  }

  return pc;
}


/* To deal with attachment on user module usage. */
static list words_genuine_regular_call(call obj, bool is_a_subroutine)
{
  list pc = words_regular_call(obj, is_a_subroutine);

  if (call_arguments(obj) != NIL) {
    /* The call is not used to code a constant: */
    //entity f = call_function(obj);
    //type t = entity_type(f);
    /* The module name is the first one except if it is a procedure CALL. */
    if (type_void_p(functional_result(type_functional(call_compatible_type(entity_type(call_function(obj)))))))
      attach_regular_call_to_word(STRING(CAR(CDR(pc))), obj);
    else
      attach_regular_call_to_word(STRING(CAR(pc)), obj);
  }

  return pc;
}

static list
words_call_intrinsic(call obj,
		     int __attribute__ ((unused)) precedence,
		     bool __attribute__ ((unused)) leftmost)
{
  return words_regular_call(obj, TRUE);
}

static list
words_assign_op(call obj,
		int precedence,
		bool __attribute__ ((unused)) leftmost)
{
  list pc = NIL, args = call_arguments(obj);
  int prec = words_intrinsic_precedence(obj);
  string fun = entity_local_name(call_function(obj));

  pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(args)), prec, TRUE));

  if (strcmp(fun,MODULO_UPDATE_OPERATOR_NAME) == 0)
    fun = "%=";
  else if (strcmp(fun,BITWISE_AND_UPDATE_OPERATOR_NAME) == 0)
    fun = "&=";
  else if (strcmp(fun,BITWISE_XOR_UPDATE_OPERATOR_NAME) == 0)
    fun = "^=";

  /* FI: space_p could be used here to control spacing around assignment */
  pc = CHAIN_SWORD(pc," ");
  pc = CHAIN_SWORD(pc, fun);
  pc = CHAIN_SWORD(pc," ");
  if(prettyprint_is_fortran) {
    expression e = EXPRESSION(CAR(CDR(args)));
    if(expression_call_p(e)) {
      /* = is not a Fortran operator. No need for parentheses ever,
	 even with the parenthesis option */
      /*
      call c = syntax_call(expression_syntax(e));
      pc = gen_nconc(pc, words_call(c, 0, TRUE, TRUE));
      */
      pc = gen_nconc(pc, words_syntax(expression_syntax(e)));
    }
    else
      pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(CDR(args))), prec, TRUE));
  }
  else { /* C code */
    /* Brace expressions are not allowed in standard assignments */
      expression exp = EXPRESSION(CAR(CDR(args)));

    if(ENTITY_ASSIGN_P(call_function(obj))) {
      if (brace_expression_p(exp))
	//pc = gen_nconc(pc,words_brace_expression(exp));
	pips_user_error("Brace expressions are not allowed in assignments\n");
      else {
	/* Be careful with expression lists, they may require
	   surrounding parentheses. */
	pc = gen_nconc(pc,words_subexpression(exp, prec, TRUE));
      }
    }
    else {
      pc = gen_nconc(pc, words_subexpression(exp, prec, TRUE));
    }
  }

  if(prec < precedence ||  (!precedence_p && precedence>0)) {
    pc = CONS(STRING, MAKE_SWORD("("), pc);
    pc = CHAIN_SWORD(pc, ")");
  }
  return(pc);
}

static list
words_substring_op(call obj,
		   int __attribute__ ((unused)) precedence,
		   bool __attribute__ ((unused)) leftmost) {
  /* The substring function call is reduced to a syntactic construct */
    list pc = NIL;
    expression r = expression_undefined;
    expression l = expression_undefined;
    expression u = expression_undefined;
    /* expression e = EXPRESSION(CAR(CDR(CDR(CDR(call_arguments(obj)))))); */
    int prec = words_intrinsic_precedence(obj);

    pips_assert("words_substring_op", gen_length(call_arguments(obj)) == 3 ||
		gen_length(call_arguments(obj)) == 4);

    r = EXPRESSION(CAR(call_arguments(obj)));
    l = EXPRESSION(CAR(CDR(call_arguments(obj))));
    u = EXPRESSION(CAR(CDR(CDR(call_arguments(obj)))));

    pc = gen_nconc(pc, words_subexpression(r,  prec, TRUE));
    pc = CHAIN_SWORD(pc, "(");
    pc = gen_nconc(pc, words_subexpression(l, prec, TRUE));
    pc = CHAIN_SWORD(pc, ":");

    /* An unknown upper bound is encoded as a call to
       UNBOUNDED_DIMENSION_NAME and nothing must be printed */
    if(syntax_call_p(expression_syntax(u))) {
	entity star = call_function(syntax_call(expression_syntax(u)));
	if(star!=CreateIntrinsic(UNBOUNDED_DIMENSION_NAME))
	    pc = gen_nconc(pc, words_subexpression(u, prec, TRUE));
    }
    else {
	pc = gen_nconc(pc, words_subexpression(u, prec, TRUE));
    }
    pc = CHAIN_SWORD(pc, ")");

    return(pc);
}

static list
words_assign_substring_op(call obj,
			  int __attribute__ ((unused)) precedence,
			  bool __attribute__ ((unused)) leftmost)
{
  /* The assign substring function call is reduced to a syntactic construct */
    list pc = NIL;
    expression e = expression_undefined;
    int prec = words_intrinsic_precedence(obj);

    pips_assert("words_substring_op", gen_length(call_arguments(obj)) == 4);

    e = EXPRESSION(CAR(CDR(CDR(CDR(call_arguments(obj))))));

    pc = gen_nconc(pc, words_substring_op(obj,  prec, TRUE));
    pc = CHAIN_SWORD(pc, " = ");
    pc = gen_nconc(pc, words_subexpression(e, prec, TRUE));

    return(pc);
}

/**
 * @return the external string representation of the operator
 * @param name, the pips internal representation of the operator
 */
static string renamed_op_handling (string name) {
  string result = name;

  if ( strcmp(result,PLUS_C_OPERATOR_NAME) == 0 )
    result = "+";
  else  if ( strcmp(result, MINUS_C_OPERATOR_NAME) == 0 )
    result = "-";
  else  if ( strcmp(result,BITWISE_AND_OPERATOR_NAME) == 0 )
    result = "&";
  else  if ( strcmp(result,BITWISE_XOR_OPERATOR_NAME) == 0 )
    result = "^";
  else  if ( strcmp(result,C_AND_OPERATOR_NAME) == 0 )
    result = "&&";
  else  if ( strcmp(result,C_NON_EQUAL_OPERATOR_NAME) == 0 )
    result = "!=";
  else  if ( strcmp(result,C_MODULO_OPERATOR_NAME) == 0 )
    result = "%";
  else if (!prettyprint_is_fortran){
    if(strcasecmp(result, GREATER_THAN_OPERATOR_NAME)==0)
      result=C_GREATER_THAN_OPERATOR_NAME;
    else if(strcasecmp(result, LESS_THAN_OPERATOR_NAME)==0)
      result=C_LESS_THAN_OPERATOR_NAME;
    else if(strcasecmp(result,GREATER_OR_EQUAL_OPERATOR_NAME)==0)
      result=C_GREATER_OR_EQUAL_OPERATOR_NAME;
    else if(strcasecmp(result,LESS_OR_EQUAL_OPERATOR_NAME)==0)
      result=C_LESS_OR_EQUAL_OPERATOR_NAME;
    else if(strcasecmp(result, EQUAL_OPERATOR_NAME) ==0)
      result=C_EQUAL_OPERATOR_NAME;
    else if(strcasecmp(result,NON_EQUAL_OPERATOR_NAME)==0)
      result= "!=";
    else if(strcasecmp(result,AND_OPERATOR_NAME)==0)
      result="&&";
    else if(strcasecmp(result, OR_OPERATOR_NAME)==0)
      result=C_OR_OPERATOR_NAME;
  }
  return result;
}

/** @return a list of string with the prettyprint of a omp reduction clause
 */
static list
words_omp_red(call obj,
	      int precedence __attribute__ ((unused)),
	      bool leftmost __attribute__ ((unused)))
{
  list result = NIL;
  entity fct = call_function(obj);
  result = CHAIN_SWORD(result, entity_user_name(fct));
  result = CHAIN_SWORD(result, "(");
  // the reduction arguments as an expression list
  list args = call_arguments (obj);
  pips_assert ("no arguments for reduction clause", args != NIL);
  int nb_arg = 0;
  FOREACH (EXPRESSION, arg, args) {
    if (nb_arg == 0) {
      // the first argument is an operator and need to be handle separately
      // because of the intenal management of operator
      string op;
      syntax syn = expression_syntax (arg);
      pips_assert ("should be a reference", syntax_tag (syn) == is_syntax_reference);
      op = entity_local_name (reference_variable (syntax_reference (syn)));
      op = renamed_op_handling (op);
      CHAIN_SWORD(result, op);
    }
    else { // (nb_arg != 0)
      result = (nb_arg == 1)? CHAIN_SWORD(result,":") : CHAIN_SWORD(result,",");
      result = gen_nconc (result, words_expression (arg));
    }
    nb_arg++;
  }
  pips_assert ("reduction clause has at least two arguments", nb_arg > 1);
  result = CHAIN_SWORD(result, ")");
  return result;
}

// Function written by C.A. Mensi to prettyprint C or Fortran code as C code
static list
words_nullary_op_c(call obj,
		 int precedence __attribute__ ((unused)),
		 bool leftmost __attribute__ ((unused)))
{
  list pc = NIL;
  list args = call_arguments(obj);
  entity func = call_function(obj);
  string fname = entity_local_name(func);
  int nargs = gen_length(args);
  bool parentheses_p=TRUE;

  /* STOP and PAUSE and RETURN in Fortran may have 0 or 1 argument.
     STOP and PAUSE are prettyprinted in C using PIPS specific C functions. */

  if(nargs==0){
    if(same_string_p(fname,STOP_FUNCTION_NAME))
      pc = CHAIN_SWORD(pc, "exit(0)");
    else if(same_string_p(fname,RETURN_FUNCTION_NAME)
	    ||same_string_p(fname,C_RETURN_FUNCTION_NAME))
      pc = CHAIN_SWORD(pc, "return");
    else if(same_string_p(fname,PAUSE_FUNCTION_NAME))
      pc = CHAIN_SWORD(pc, "_f77_intrinsics_pause_(0)");
    else if(same_string_p(fname,CONTINUE_FUNCTION_NAME))
      pc = CHAIN_SWORD(pc, "");
    else if ((same_string_p(fname,OMP_OMP_FUNCTION_NAME)) ||
	     (same_string_p(fname,OMP_FOR_FUNCTION_NAME)) ||
	     (same_string_p(fname,OMP_PARALLEL_FUNCTION_NAME)))
      pc = CHAIN_SWORD(pc, fname);
    else
      pips_internal_error("Unknown nullary operator");
  }
  else if(nargs==1){
    expression e = EXPRESSION(CAR(args));

    if(same_string_p(fname,STOP_FUNCTION_NAME)){
      basic b=expression_basic(e);
      if(basic_int_p(b)){
	// Missing: declaration of exit() if Fortran code handled
	pc = CHAIN_SWORD(pc, "exit");
      }
      else if(basic_string_p(b)){
	pc = CHAIN_SWORD(pc, "_f77_intrinsics_stop_");
      }
    }
    else if(same_string_p(fname,RETURN_FUNCTION_NAME)
	    ||same_string_p(fname,C_RETURN_FUNCTION_NAME)){
      pc = CHAIN_SWORD(pc, "return");
      parentheses_p = FALSE;
      //pips_user_error("alternate returns are not supported in C\n");
    }
    else if(same_string_p(fname, PAUSE_FUNCTION_NAME)){
      pc = CHAIN_SWORD(pc, "_f77_intrinsics_pause_");
    }
    else {
      pips_internal_error("unexpected one argument");
    }
    pc = CHAIN_SWORD(pc, parentheses_p?"(":" ");
    pc = gen_nconc(pc, words_subexpression(e, precedence, TRUE));
    pc = CHAIN_SWORD(pc, parentheses_p?")":"");
  }
  else {
    pips_internal_error("unexpected arguments");
  }
  return(pc);
}

// function added for fortran  by A. Mensi
static list words_nullary_op_fortran(call obj,
				     int precedence,
				     bool __attribute__ ((unused)) leftmost)
{
  list pc = NIL;
  list args = call_arguments(obj);
  entity func = call_function(obj);
  string fname = entity_local_name(func);

  if(same_string_p(fname,RETURN_FUNCTION_NAME)
     ||same_string_p(fname,C_RETURN_FUNCTION_NAME))
    pc = CHAIN_SWORD(pc, "return");
  else if (same_string_p(fname,OMP_FOR_FUNCTION_NAME))
    pc = CHAIN_SWORD(pc, "do");
  else
    pc = CHAIN_SWORD(pc, fname);

  // STOP and PAUSE and RETURN in fortran may have 0 or 1 argument.A Mensi
  if(gen_length(args)==1) {
    if(same_string_p(fname,STOP_FUNCTION_NAME)
       || same_string_p(fname,PAUSE_FUNCTION_NAME)
       || same_string_p(fname,RETURN_FUNCTION_NAME)
       || same_string_p(fname, C_RETURN_FUNCTION_NAME)) {
      expression e = EXPRESSION(CAR(args));
      pc = CHAIN_SWORD(pc, " ");
      pc = gen_nconc(pc, words_subexpression(e, precedence, TRUE));
    }
    else {
      pips_internal_error("unexpected arguments");
    }
  }
  else if(gen_length(args)>1) {
    pips_internal_error("unexpected arguments");
  }

  return(pc);
}

static list words_nullary_op(call obj,
			     int precedence,
			     bool __attribute__ ((unused)) leftmost)
{
  return prettyprint_is_fortran? words_nullary_op_fortran(obj, precedence, leftmost)
    : words_nullary_op_c(obj, precedence, leftmost);
}


static list
words_io_control(list *iol,
		 int __attribute__ ((unused)) precedence,
		 bool __attribute__ ((unused)) leftmost)
{
    list pc = NIL;
    list pio = *iol;

    while (pio != NIL) {
	syntax s = expression_syntax(EXPRESSION(CAR(pio)));
	call c;

	if (! syntax_call_p(s)) {
	    pips_error("words_io_control", "call expected");
	}

	c = syntax_call(s);

	if (strcmp(entity_local_name(call_function(c)), IO_LIST_STRING_NAME) == 0) {
	    *iol = CDR(pio);
	    return(pc);
	}

	if (pc != NIL)
	    pc = CHAIN_SWORD(pc, ",");

	pc = CHAIN_SWORD(pc, entity_local_name(call_function(c)));
	pc = gen_nconc(pc, words_expression(EXPRESSION(CAR(CDR(pio)))));

	pio = CDR(CDR(pio));
    }

    if (pio != NIL)
	    pips_error("words_io_control", "bad format");

    *iol = NIL;

    return(pc);
}

static list
words_implied_do(call obj,
		 int __attribute__ ((unused)) precedence,
		 bool __attribute__ ((unused)) leftmost)
{
    list pc = NIL;
    list pcc;
    expression index;
    syntax s;
    range r;
    bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

    pcc = call_arguments(obj);
    index = EXPRESSION(CAR(pcc));

    pcc = CDR(pcc);
    s = expression_syntax(EXPRESSION(CAR(pcc)));
    if (! syntax_range_p(s)) {
	pips_error("words_implied_do", "range expected");
    }
    r = syntax_range(s);

    pc = CHAIN_SWORD(pc, "(");
    MAPL(pcp, {
	pc = gen_nconc(pc, words_expression(EXPRESSION(CAR(pcp))));
	if (CDR(pcp) != NIL)
	    pc = CHAIN_SWORD(pc, space_p? ", " : ",");
    }, CDR(pcc));
    pc = CHAIN_SWORD(pc, space_p? ", " : ",");

    pc = gen_nconc(pc, words_expression(index));
    pc = CHAIN_SWORD(pc, " = ");
    pc = gen_nconc(pc, words_loop_range(r));
    pc = CHAIN_SWORD(pc, ")");

    return(pc);
}

static list
words_unbounded_dimension(call __attribute__ ((unused)) obj,
			  int __attribute__ ((unused)) precedence,
			  bool __attribute__ ((unused)) leftmost)
{
    list pc = NIL;

    pc = CHAIN_SWORD(pc, "*");

    return(pc);
}

static list
words_list_directed(call __attribute__ ((unused)) obj,
		    int __attribute__ ((unused)) precedence,
		    bool __attribute__ ((unused)) leftmost)
{
    list pc = NIL;

    pc = CHAIN_SWORD(pc, "*");

    return(pc);
}

static list
words_io_inst(call obj,
	      int precedence, bool leftmost)
{
  list pc = NIL;
  list pcio = call_arguments(obj);
  list pio_write = pcio;
  boolean good_fmt = FALSE;
  bool good_unit = FALSE;
  bool iolist_reached = FALSE;
  bool complex_io_control_list = FALSE;
  expression fmt_arg = expression_undefined;
  expression unit_arg = expression_undefined;
  string called = entity_local_name(call_function(obj));
  bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

  /* AP: I try to convert WRITE to PRINT. Three conditions must be
     fullfilled. The first, and obvious, one, is that the function has
     to be WRITE. Secondly, "FMT" has to be equal to "*". Finally,
     "UNIT" has to be equal either to "*" or "6".  In such case,
     "WRITE(*,*)" is replaced by "PRINT *,". */
  /* GO: Not anymore for UNIT=6 leave it ... */
  while ((pio_write != NIL) && (!iolist_reached)) {
    syntax s = expression_syntax(EXPRESSION(CAR(pio_write)));
    call c;
    expression arg = EXPRESSION(CAR(CDR(pio_write)));

    if (! syntax_call_p(s)) {
      pips_internal_error("call expected");
    }

    c = syntax_call(s);
    if (strcmp(entity_local_name(call_function(c)), "FMT=") == 0) {
      /* Avoid to use words_expression(arg) because it set some
	 attachments and unit_words may not be used
	 later... RK. */
      entity f;
      /* The * format is coded as a call to "LIST_DIRECTED_FORMAT_NAME" function: */
      good_fmt = syntax_call_p(expression_syntax(arg))
	&& value_intrinsic_p(entity_initial(f =
					    call_function(syntax_call(expression_syntax(arg)))))
	&& (strcmp(entity_local_name(f),
		   LIST_DIRECTED_FORMAT_NAME)==0);
      pio_write = CDR(CDR(pio_write));
      /* To display the format later: */
      fmt_arg = arg;
    }
    else if (strcmp(entity_local_name(call_function(c)), "UNIT=") == 0) {
      /* Avoid to use words_expression(arg) because it set some
	 attachments and unit_words may not be used
	 later... RK. */
      entity f;
      /* The * format is coded as a call to "LIST_DIRECTED_FORMAT_NAME" function: */
      good_unit = syntax_call_p(expression_syntax(arg))
	&& value_intrinsic_p(entity_initial(f =
					    call_function(syntax_call(expression_syntax(arg)))))
	&& (strcmp(entity_local_name(f),
		   LIST_DIRECTED_FORMAT_NAME)==0);
      /* To display the unit later: */
      unit_arg = arg;
      pio_write = CDR(CDR(pio_write));
    }
    else if (strcmp(entity_local_name(call_function(c)), IO_LIST_STRING_NAME) == 0) {
      iolist_reached = TRUE;
      pio_write = CDR(pio_write);
    }
    else {
      complex_io_control_list = TRUE;
      pio_write = CDR(CDR(pio_write));
    }
  }

  if (good_fmt && good_unit && same_string_p(called, "WRITE"))
    {
      /* WRITE (*,*) -> PRINT * */

      if (pio_write != NIL)	/* WRITE (*,*) pio -> PRINT *, pio */
	{
	  pc = CHAIN_SWORD(pc, "PRINT *, ");
	}
      else			/* WRITE (*,*)  -> PRINT *  */
	{
	  pc = CHAIN_SWORD(pc, "PRINT * ");
	}

      pcio = pio_write;
    }
  else if (good_fmt && good_unit && same_string_p(called, "READ"))
    {
      /* READ (*,*) -> READ * */

      if (pio_write != NIL )	/* READ (*,*) pio -> READ *, pio */
	{
	  if(!prettyprint_is_fortran)
	    pc = CHAIN_SWORD(pc, "_f77_intrinsics_read_(");
	  else
	    pc = CHAIN_SWORD(pc, "READ *, ");
	}
      else			/* READ (*,*)  -> READ *  */
	{
	  pc = CHAIN_SWORD(pc, "READ * ");
	}
      pcio = pio_write;
    }
  else if (!complex_io_control_list) {
    list unit_words = words_expression(unit_arg);
    pips_assert("A unit must be defined", !ENDP(unit_words));
    pc = CHAIN_SWORD(pc, entity_local_name(call_function(obj)));
    pc = CHAIN_SWORD(pc, " (");
    pc = gen_nconc(pc, unit_words);

    if (!expression_undefined_p(fmt_arg)) {
      /* There is a FORMAT: */
      pc = CHAIN_SWORD(pc, space_p? ", " : ",");
      pc = gen_nconc(pc, words_expression(fmt_arg));
    }

    pc = CHAIN_SWORD(pc, ") ");
    pcio = pio_write;
  }
  else {
    pc = CHAIN_SWORD(pc, entity_local_name(call_function(obj)));
    pc = CHAIN_SWORD(pc, " (");
    /* FI: missing argument; I use "precedence" because I've no clue;
       see LZ */
    pc = gen_nconc(pc, words_io_control(&pcio, precedence, leftmost));
    pc = CHAIN_SWORD(pc, ") ");
    /*
       free_words(fmt_words);
    */
  }

  /* because the "IOLIST=" keyword is embedded in the list
     and because the first IOLIST= has already been skipped,
     only odd elements are printed */
  MAPL(pp, {
    pc = gen_nconc(pc, words_expression(EXPRESSION(CAR(pp))));
    if (CDR(pp) != NIL) {
      POP(pp);
      if(pp==NIL)
	pips_internal_error("missing element in IO list");
      pc = CHAIN_SWORD(pc, space_p? ", " : ",");
    }
  }, pcio);

  if(!prettyprint_is_fortran)
    pc = CHAIN_SWORD(pc, ") ");

  return(pc) ;
}

static list
null(call __attribute__ ((unused)) obj,
     int __attribute__ ((unused)) precedence,
     bool __attribute__ ((unused)) leftmost)
{
    return(NIL);
}

static list
words_prefix_unary_op(call obj,
		      int  precedence,
		      bool __attribute__ ((unused)) leftmost)
{
  list pc = NIL;
  expression e = EXPRESSION(CAR(call_arguments(obj)));
  int prec = words_intrinsic_precedence(obj);
  string fun = entity_local_name(call_function(obj));
  if (strcmp(fun,PRE_INCREMENT_OPERATOR_NAME) == 0)
    fun = "++";
  else if (strcmp(fun,PRE_DECREMENT_OPERATOR_NAME) == 0)
    fun = "--";
  else if (strcmp(fun,ADDRESS_OF_OPERATOR_NAME) == 0)
    fun = "&";
  else if (strcmp(fun,C_NOT_OPERATOR_NAME) == 0)
    fun = "!";
  else if (strcmp(fun,BITWISE_NOT_OPERATOR_NAME) == 0)
    fun = "~";
  else if (strcmp(fun,DEREFERENCING_OPERATOR_NAME) == 0)
      /* Since we put no spaces around an operator (to not change Fortran), the blank
	 before '*' is used to avoid the confusion in the case of divide operator, i.e
	 d1 = 1.0 / *det  in function inv_j, SPEC2000 quake benchmark.

	 But we do not want this in a lhs and espcially with a double dereferencing. */
    fun = "*";
  else if(!prettyprint_is_fortran){
	if(strcasecmp(fun, NOT_OPERATOR_NAME)==0)
	  fun="!";
	if(strcasecmp(fun, UNARY_PLUS_OPERATOR_NAME)==0) {
	  /* You do not want to transform +1 + +1 into +1++ 1 */
	  /* Maybe the precedence could be useful to avoid adding a
	     useless SPACE, but unary plus is rare enough to reduce
	     the ROI of such anoptimization to zero. */
	  fun=" +";
	}
      }

  pc = CHAIN_SWORD(pc,fun);
  pc = gen_nconc(pc, words_subexpression(e, prec, FALSE));

  if(prec < precedence ||  (!precedence_p && precedence>0)) {
    pc = CONS(STRING, MAKE_SWORD("("), pc);
    pc = CHAIN_SWORD(pc, ")");
  }

  return(pc);
}

static list
words_postfix_unary_op(call obj,
		       int  precedence,
		       bool __attribute__ ((unused)) leftmost)
{
    list pc = NIL;
    expression e = EXPRESSION(CAR(call_arguments(obj)));
    int prec = words_intrinsic_precedence(obj);
    string fun = entity_local_name(call_function(obj));

    pc = gen_nconc(pc, words_subexpression(e, prec, FALSE));

    if (strcmp(fun,POST_INCREMENT_OPERATOR_NAME) == 0)
      fun = "++";
    else if (strcmp(fun,POST_DECREMENT_OPERATOR_NAME) == 0)
     fun = "--";

    pc = CHAIN_SWORD(pc,fun);

    if(prec < precedence ||  (!precedence_p && precedence>0)) {
      pc = CONS(STRING, MAKE_SWORD("("), pc);
      pc = CHAIN_SWORD(pc, ")");
    }

    return(pc);
}


static list
words_unary_minus(call obj, int precedence, bool leftmost)
{
    list pc = NIL;
    expression e = EXPRESSION(CAR(call_arguments(obj)));
    int prec = words_intrinsic_precedence(obj);

    if ( prec < precedence || !leftmost ||  !precedence_p)
	pc = CHAIN_SWORD(pc, "(");
    pc = CHAIN_SWORD(pc, "-");
    pc = gen_nconc(pc, words_subexpression(e, prec, FALSE));
    if ( prec < precedence || !leftmost ||  !precedence_p)
	pc = CHAIN_SWORD(pc, ")");

    return(pc);
}

/*
   The precedence of (1/x) is the same as the multiply operator
   (e.g. a*1/b without parentheses). Moreover, the MAXIMAL precedence is
   used for the (x) subterm (e.g. 1/(a*b) 1/(-2) ...). However, 1/x**2 may
   be a correct prettyprint in Fortran (?) */
/* WARNING : the floating point division is used wether b is an int or not
   ! (1.0/b) -- in fact b should not be an int ! */
static list /* of string */
words_inverse_op(call obj,
		 int precedence,
		 bool __attribute__ ((unused)) leftmost)
{
  list /* of string */ pc = NIL;

  expression e = EXPRESSION(CAR(call_arguments(obj)));
  int prec = words_intrinsic_precedence(obj);

  if ( prec < precedence)
    pc = CHAIN_SWORD(pc, "(");
  pc = CHAIN_SWORD(pc, "1./");
  pc = gen_nconc(pc, words_subexpression(e, MAXIMAL_PRECEDENCE ,
					 FALSE));

  if ( prec < precedence)
    pc = CHAIN_SWORD(pc, ")");

  return(pc);
}

list /* of string */
words_goto_label(string tlabel)
{
    list pc = NIL;
    if (strcmp(tlabel, RETURN_LABEL_NAME) == 0) {
	pc = CHAIN_SWORD(pc, RETURN_FUNCTION_NAME);
    }
    else {
      /* In C, a label cannot begin with a number so "l" is added for this case*/
      pc = CHAIN_SWORD(pc, strdup(prettyprint_is_fortran?"GOTO ":(isdigit(tlabel[0])?"goto l":"goto ")));
      pc = CHAIN_SWORD(pc, tlabel);
      if (!prettyprint_is_fortran)
	pc = CHAIN_SWORD(pc, ";");
    }
    return pc;
}

static list
eole_fmx_specific_op(call obj,
		     int __attribute__ ((unused)) precedence,
		     bool __attribute__ ((unused)) leftmost,
		     bool isadd)
{
  list /* of strings */ pc = NIL;
  list /* of expressions */ args = call_arguments(obj);

  int prec ;

  /* open parenthese one  */
  pc = CHAIN_SWORD(pc, "(");

  /* open parenthese two */
  pc = CHAIN_SWORD(pc, "(");

  /* get precedence for mult operator */
  prec = intrinsic_precedence("*");

  /* first argument */
  pc = gen_nconc(pc,words_subexpression(EXPRESSION(CAR(args)), prec, TRUE));

  /* mult operator */
  pc = CHAIN_SWORD(pc,"*");

  /* second argument */
  args = CDR(args);
  pc = gen_nconc(pc,words_subexpression(EXPRESSION(CAR(args)),prec,TRUE));

  /* close parenthese two */
  pc = CHAIN_SWORD(pc, ")");

  /* get precedence for add operator */
  prec = intrinsic_precedence("+");

  /* add/sub operator */
  pc = CHAIN_SWORD(pc, isadd? "+": "-");

  /* third argument */
  args = CDR(args);
  pc = gen_nconc(pc,words_subexpression(EXPRESSION(CAR(args)),prec,FALSE));

  /* close parenthese one  */
  pc = CHAIN_SWORD(pc,")");

  return pc;
}

/* EOLE : The multiply-add operator is used within the optimize
   transformation ( JZ - sept 98) - fma(a,b,c) -> ((a*b)+c)
 */
list /* of string */
eole_fma_specific_op(call obj, int precedence, bool leftmost)
{
  return eole_fmx_specific_op(obj, precedence, leftmost, TRUE);
}

/* MULTIPLY-SUB operator */
list /* of string */
eole_fms_specific_op(call obj, int precedence, bool leftmost)
{
  return eole_fmx_specific_op(obj, precedence, leftmost, FALSE);
}

/* Check if the given operator is associated with a special
    prettyprint. For instance, n-ary add and multiply operators which are
    used in the EOLE project use "+" and "*" prettyprints instead of the
    entity_local_name (JZ - sept 98) */
static string
get_special_prettyprint_for_operator(call obj){

  static struct special_operator_prettyprint {
    char * name;
    char * op_prettyprint;
  } tab_operator_prettyprint[] = {
    {EOLE_SUM_OPERATOR_NAME,"+"},
    {EOLE_PROD_OPERATOR_NAME,"*"},
    {NULL,NULL}
  };
  int i = 0;
  string op_name;

  /* get the entity name */
  op_name = entity_local_name(call_function(obj));

  while (tab_operator_prettyprint[i].name) {
    if (!strcmp(tab_operator_prettyprint[i].name,op_name))
      return tab_operator_prettyprint[i].op_prettyprint;
    else i++;
  }

  return op_name;
}

/* Extension of "words_infix_binary_op" function for nary operators used
   in the EOLE project - (since "nary" assumes operators with at least 2
   op)  - JZ (Oct. 98)*/

static list /* of string */
words_infix_nary_op(call obj, int precedence, bool leftmost)
{
  list /*of string*/ pc = NIL;
  list /* of expressions */ args = call_arguments(obj);

  /* get current operator precedence */
  int prec = words_intrinsic_precedence(obj);

  expression exp1 = EXPRESSION(CAR(args));
  expression exp2;

  list we1 = words_subexpression(exp1, prec,
				 prec>=MINIMAL_ARITHMETIC_PRECEDENCE? leftmost: TRUE);
  list we2;

  /* open parenthese if necessary */
  if ( prec < precedence )
    pc = CHAIN_SWORD(pc, "(");
  pc = gen_nconc(pc, we1);

  /* reach the second arg */
  args = CDR(args);

  for(; args; args=CDR(args)) { /* for all args */
    exp2 = EXPRESSION(CAR(args));

    /*
     * If the infix operator is either "-" or "/", I prefer not to delete
     * the parentheses of the second expression.
     * Ex: T = X - ( Y - Z ) and T = X / (Y*Z)
     *
     * Lei ZHOU       Nov. 4 , 1991
     */
    if ( strcmp(entity_local_name(call_function(obj)), "/") == 0 )  /* divide operator */
      we2 = words_subexpression(exp2, MAXIMAL_PRECEDENCE, FALSE);
    else if ( strcmp(entity_local_name(call_function(obj)), "-") == 0 ) { /* minus operator */
      if ( expression_call_p(exp2) &&
	   words_intrinsic_precedence(syntax_call(expression_syntax(exp2))) >=
	   intrinsic_precedence("*") )
	/* precedence is greater than * or / */
	we2 = words_subexpression(exp2, prec, FALSE);
      else
	we2 = words_subexpression(exp2, MAXIMAL_PRECEDENCE, FALSE);
    }
    else {
      we2 = words_subexpression(exp2, prec,
				prec<MINIMAL_ARITHMETIC_PRECEDENCE);
    }

    /* operator prettyprint */
    pc = CHAIN_SWORD(pc, get_special_prettyprint_for_operator(obj));

    pc = gen_nconc(pc, we2);
  }
  /* close parenthese if necessary */
  if ( prec < precedence )
    pc = CHAIN_SWORD(pc, ")");

  return(pc);
}

/*
 * If the infix operator is either "-" or "/", I prefer not to delete
 * the parentheses of the second expression.
 * Ex: T = X - ( Y - Z ) and T = X / (Y*Z)
 *
 * Lei ZHOU       Nov. 4 , 1991
 */
static list
words_infix_binary_op(call obj, int precedence, bool leftmost)
{
  list pc = NIL;
  list args = call_arguments(obj);
  int prec = words_intrinsic_precedence(obj);
  list we1 = words_subexpression(EXPRESSION(CAR(args)), prec,
				 prec>=MINIMAL_ARITHMETIC_PRECEDENCE? leftmost: TRUE);
  list we2;
  string fun = entity_local_name(call_function(obj));

  /* handling of internally renamed operators */
  fun = renamed_op_handling (fun);

  if(strcmp(fun, DIVIDE_OPERATOR_NAME) == 0) {
    /* Do we want to add a space in case we2 starts with a dereferencing operator "*"?
     Nga suggests to look at the quake benchmark of SPEC2000. */
    we2 = words_subexpression(EXPRESSION(CAR(CDR(args))), MAXIMAL_PRECEDENCE, FALSE);
  }
  else if (strcmp(fun, MINUS_OPERATOR_NAME) == 0 ) {
    expression exp = EXPRESSION(CAR(CDR(args)));
    if(expression_call_p(exp) &&
       words_intrinsic_precedence(syntax_call(expression_syntax(exp))) >=
       intrinsic_precedence(MULTIPLY_OPERATOR_NAME) )
      /* precedence is greater than * or / */
      we2 = words_subexpression(exp, prec, FALSE);
    else
      we2 = words_subexpression(exp, MAXIMAL_PRECEDENCE, FALSE);
  }
  else if(strcmp(fun, MULTIPLY_OPERATOR_NAME) == 0) {
    expression exp = EXPRESSION(CAR(CDR(args)));
    if(expression_call_p(exp) &&
       ENTITY_DIVIDE_P(call_function(syntax_call(expression_syntax(exp))))) {
      basic bexp = basic_of_expression(exp);

      if(basic_int_p(bexp)) {
	we2 = words_subexpression(exp, MAXIMAL_PRECEDENCE, FALSE);
      }
      else
	we2 = words_subexpression(exp, prec, FALSE);
      free_basic(bexp);
    }
    else
      we2 = words_subexpression(exp, prec, FALSE);
  }
  else {
    we2 = words_subexpression(EXPRESSION(CAR(CDR(args))), prec,
			      prec<MINIMAL_ARITHMETIC_PRECEDENCE);
  }

  /* Use precedence to generate or not parentheses, unless parentheses are always required */
  if(prec < precedence ||  !precedence_p)
    pc = CHAIN_SWORD(pc, "(");
  pc = gen_nconc(pc, we1);
  pc = CHAIN_SWORD(pc, strdup(fun));
  pc = gen_nconc(pc, we2);
  if(prec < precedence || !precedence_p)
    pc = CHAIN_SWORD(pc, ")");

  return(pc);
}

/* Nga Nguyen : this case is added for comma expression in C, but I am
   not sure about its precedence => to be looked at more carefully */

static list words_comma_op(call obj,
			   int precedence,
			   bool __attribute__ ((unused)) leftmost)
{
  list pc = NIL, args = call_arguments(obj);
  int prec = words_intrinsic_precedence(obj);
  bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

  if(prec < precedence || !precedence_p)
    pc = CHAIN_SWORD(pc,"(");
  pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(args)), prec, TRUE));
  while (!ENDP(CDR(args)))
  {
    pc = CHAIN_SWORD(pc,space_p?", " : ",");
    pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(CDR(args))), prec, TRUE));
    args = CDR(args);
  }
  if(prec < precedence || !precedence_p)
    pc = CHAIN_SWORD(pc,")");
  return(pc);
}

static list words_conditional_op(call obj,
				 int precedence,
				 bool __attribute__ ((unused)) leftmost)
{
  list pc = NIL, args = call_arguments(obj);
  int prec = words_intrinsic_precedence(obj);

  if(prec < precedence || !precedence_p)
    pc = CHAIN_SWORD(pc,"(");
  pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(args)), prec, TRUE));
  pc = CHAIN_SWORD(pc,"?");
  pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(CDR(args))), prec, TRUE));
  pc = CHAIN_SWORD(pc,":");
  pc = gen_nconc(pc, words_subexpression(EXPRESSION(CAR(CDR(CDR(args)))), prec, TRUE));
  if(prec < precedence || !precedence_p)
    pc = CHAIN_SWORD(pc,")");
  return(pc);
}


/* precedence needed here
 * According to the Precedence of Operators
 * Arithmetic > Character > Relational > Logical
 * Added by Lei ZHOU    Nov. 4,91
 *
 * A precedence is a integer in [0..MAXIMAL_PRECEDENCE]
 */

static struct intrinsic_handler {
    char * name;
    list (*f)();
    int prec;
} tab_intrinsic_handler[] = {
    {POWER_OPERATOR_NAME, words_infix_binary_op, 30},

    {CONCATENATION_FUNCTION_NAME, words_infix_binary_op, 30},

    /* The Fortran 77 standard does not allow x*-3 or x+-3, but this is dealt
    * with by argument leftmost, not by prorities.
    */
    {UNARY_MINUS_OPERATOR_NAME, words_unary_minus, 25},
    /* {"--", words_unary_minus, 19}, */

    {INVERSE_OPERATOR_NAME, words_inverse_op, 21},

    {PLUS_OPERATOR_NAME, words_infix_binary_op, 20},
    {MINUS_OPERATOR_NAME, words_infix_binary_op, 20},

    /* Non-arithemtic operators have priorities lesser than MINIMAL_ARITHMETIC_PRECEDENCE
     * leftmost is restaured to true for unary minus.
     */

    {LESS_THAN_OPERATOR_NAME, words_infix_binary_op, 15},
    {GREATER_THAN_OPERATOR_NAME, words_infix_binary_op, 15},
    {LESS_OR_EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},
    {GREATER_OR_EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},
    {EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},
    {NON_EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},

    {NOT_OPERATOR_NAME, words_prefix_unary_op, 9},

    {AND_OPERATOR_NAME, words_infix_binary_op, 8},

    {OR_OPERATOR_NAME, words_infix_binary_op, 6},

    {EQUIV_OPERATOR_NAME, words_infix_binary_op, 3},
    {NON_EQUIV_OPERATOR_NAME, words_infix_binary_op, 3},

    {ASSIGN_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},

    {WRITE_FUNCTION_NAME, words_io_inst, 0},
    {READ_FUNCTION_NAME, words_io_inst, 0},
    {PRINT_FUNCTION_NAME, words_io_inst, 0},
    {OPEN_FUNCTION_NAME, words_io_inst, 0},
    {CLOSE_FUNCTION_NAME, words_io_inst, 0},
    {INQUIRE_FUNCTION_NAME, words_io_inst, 0},
    {BACKSPACE_FUNCTION_NAME, words_io_inst, 0},
    {REWIND_FUNCTION_NAME, words_io_inst, 0},
    {ENDFILE_FUNCTION_NAME, words_io_inst, 0},
    {IMPLIED_DO_FUNCTION_NAME, words_implied_do, 0},

    {RETURN_FUNCTION_NAME, words_nullary_op,0},
    {C_RETURN_FUNCTION_NAME, words_nullary_op,0},
    {PAUSE_FUNCTION_NAME, words_nullary_op,0 },
    {STOP_FUNCTION_NAME, words_nullary_op, 0},
    {CONTINUE_FUNCTION_NAME, words_nullary_op,0},
    {END_FUNCTION_NAME, words_nullary_op, 0},


    {FORMAT_FUNCTION_NAME, words_prefix_unary_op, 0},
    {UNBOUNDED_DIMENSION_NAME, words_unbounded_dimension, 0},
    {LIST_DIRECTED_FORMAT_NAME, words_list_directed, 0},

    {SUBSTRING_FUNCTION_NAME, words_substring_op, 0},
    {ASSIGN_SUBSTRING_FUNCTION_NAME, words_assign_substring_op, 0},

    /* These operators are used within the optimize transformation in
order to manipulate operators such as n-ary add and multiply or
multiply-add operators ( JZ - sept 98) */
    {EOLE_FMA_OPERATOR_NAME, eole_fma_specific_op,
                             MINIMAL_ARITHMETIC_PRECEDENCE },
    {EOLE_FMS_OPERATOR_NAME, eole_fms_specific_op,
                             MINIMAL_ARITHMETIC_PRECEDENCE },
    {EOLE_SUM_OPERATOR_NAME, words_infix_nary_op, 20},
    {EOLE_PROD_OPERATOR_NAME, words_infix_nary_op, 21},

    /* show IMA/IMS */
    {IMA_OPERATOR_NAME, eole_fma_specific_op,
	                         MINIMAL_ARITHMETIC_PRECEDENCE },
	{IMS_OPERATOR_NAME, eole_fms_specific_op,
	                         MINIMAL_ARITHMETIC_PRECEDENCE },

    /* 05/08/2003 - Nga Nguyen - Here are C intrinsics.
       The precedence is computed by using Table xx, page 49, book
       "The C programming language" of Kernighan and Ritchie, and by
       taking into account the precedence value of Fortran intrinsics. */

    {FIELD_OPERATOR_NAME, words_infix_binary_op, 30},
    {POINT_TO_OPERATOR_NAME, words_infix_binary_op, 30},
    {POST_INCREMENT_OPERATOR_NAME, words_postfix_unary_op, 30},
    {POST_DECREMENT_OPERATOR_NAME, words_postfix_unary_op, 30},

    {PRE_INCREMENT_OPERATOR_NAME,  words_prefix_unary_op, 25},
    {PRE_DECREMENT_OPERATOR_NAME,  words_prefix_unary_op, 25},
    {ADDRESS_OF_OPERATOR_NAME,     words_prefix_unary_op,25},
    {DEREFERENCING_OPERATOR_NAME,  words_prefix_unary_op, 25},
    {UNARY_PLUS_OPERATOR_NAME, words_prefix_unary_op, 25},
    /*{"-unary", words_prefix_unary_op, 25},*/
    {BITWISE_NOT_OPERATOR_NAME, words_prefix_unary_op, 25},
    {C_NOT_OPERATOR_NAME, words_prefix_unary_op, 25},

    {C_MODULO_OPERATOR_NAME,  words_infix_binary_op, 21},
    {MULTIPLY_OPERATOR_NAME, words_infix_binary_op, 21},
    {DIVIDE_OPERATOR_NAME, words_infix_binary_op, 21},

    {PLUS_C_OPERATOR_NAME, words_infix_binary_op, 20},
    {MINUS_C_OPERATOR_NAME, words_infix_binary_op, 20},

    {LEFT_SHIFT_OPERATOR_NAME, words_infix_binary_op, 19},
    {RIGHT_SHIFT_OPERATOR_NAME, words_infix_binary_op, 19},

    {C_LESS_THAN_OPERATOR_NAME, words_infix_binary_op, 15 },
    {C_GREATER_THAN_OPERATOR_NAME, words_infix_binary_op, 15},
    {C_LESS_OR_EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},
    {C_GREATER_OR_EQUAL_OPERATOR_NAME, words_infix_binary_op, 15},

    {C_EQUAL_OPERATOR_NAME, words_infix_binary_op, 14},
    {C_NON_EQUAL_OPERATOR_NAME, words_infix_binary_op, 14},

    {BITWISE_AND_OPERATOR_NAME, words_infix_binary_op, 13},
    {BITWISE_XOR_OPERATOR_NAME, words_infix_binary_op, 12},
    {BITWISE_OR_OPERATOR_NAME, words_infix_binary_op, 11},

    {C_AND_OPERATOR_NAME, words_infix_binary_op, 8},
    {C_OR_OPERATOR_NAME, words_infix_binary_op, 6},

    {MULTIPLY_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {DIVIDE_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {MODULO_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {PLUS_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {MINUS_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {LEFT_SHIFT_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {RIGHT_SHIFT_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {BITWISE_AND_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {BITWISE_XOR_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},
    {BITWISE_OR_UPDATE_OPERATOR_NAME, words_assign_op, ASSIGN_OPERATOR_PRECEDENCE},

    /* which precedence ? You are safe within an assignment. */
    {CONDITIONAL_OPERATOR_NAME, words_conditional_op, ASSIGN_OPERATOR_PRECEDENCE+1},

    /* which precedence ? You need parentheses within an assignment. */
    {COMMA_OPERATOR_NAME, words_comma_op, ASSIGN_OPERATOR_PRECEDENCE-1},

    /* OMP pragma function part */
    {OMP_OMP_FUNCTION_NAME,       words_nullary_op, 0},
    {OMP_FOR_FUNCTION_NAME,       words_nullary_op, 0},
    {OMP_PARALLEL_FUNCTION_NAME,  words_nullary_op, 0},
    {OMP_REDUCTION_FUNCTION_NAME, words_omp_red,    0},

#include "STEP_RT_intrinsic.h"

    {NULL, null, 0}
};

static list
words_intrinsic_call(call obj, int precedence, bool leftmost)
{
    struct intrinsic_handler *p = tab_intrinsic_handler;
    char *n = entity_local_name(call_function(obj));

    while (p->name != NULL) {
	if (strcmp(p->name, n) == 0) {
	  return((*(p->f))(obj, precedence, leftmost));
	}
	p++;
    }

    return words_regular_call(obj, FALSE);
}

static int
intrinsic_precedence(string n)
{
    struct intrinsic_handler *p = tab_intrinsic_handler;

    while (p->name != NULL) {
	if (strcmp(p->name, n) == 0)
	    return(p->prec);
	p++;
    }

    return 0;
}

static int
words_intrinsic_precedence(call obj)
{
    char *n = entity_local_name(call_function(obj));
    return intrinsic_precedence(n);
}

static list words_va_arg(list obj)
{
  list pc = NIL;
  expression e1 = sizeofexpression_expression(SIZEOFEXPRESSION(CAR(obj)));
  type t2 = sizeofexpression_type(SIZEOFEXPRESSION(CAR(CDR(obj))));
  bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

  pc = CHAIN_SWORD(pc,"va_arg(");
  pc = gen_nconc(pc, words_expression(e1));
  pc = CHAIN_SWORD(pc, space_p? ", " : ",");
  pc = gen_nconc(pc, words_type(t2));
  pc = CHAIN_SWORD(pc,")");
  return pc;
}

/* exported for cmfortran.c
 */
list
words_call(
    call obj,
    int precedence,
    bool leftmost,
    bool is_a_subroutine)
{
  list pc;
  entity f = call_function(obj);
  value i = entity_initial(f);

  if(value_intrinsic_p(i)) {
    int effective_precedence = (precedence_p||precedence<=1)? precedence : MAXIMAL_PRECEDENCE;

    pc = words_intrinsic_call(obj, effective_precedence, leftmost);
  }
  else
    pc = words_genuine_regular_call(obj, is_a_subroutine);
  return pc;
}

/* exported for expression.c
 */
list
words_syntax(syntax obj)
{
    list pc = NIL;

    switch (syntax_tag(obj)) {
    case is_syntax_reference :
      pc = words_reference(syntax_reference(obj));
      break;
    case is_syntax_range:
      pc = words_range(syntax_range(obj));
      break;
    case is_syntax_call:
      pc = words_call(syntax_call(obj), 0, TRUE, FALSE);
      break;
    case is_syntax_cast:
      pc = words_cast(syntax_cast(obj));
      break;
    case is_syntax_sizeofexpression:
      pc = words_sizeofexpression(syntax_sizeofexpression(obj));
      break;
    case is_syntax_subscript:
      pc = words_subscript(syntax_subscript(obj));
      break;
    case is_syntax_application:
      pc = words_application(syntax_application(obj));
      break;
    case is_syntax_va_arg:
      pc = words_va_arg(syntax_va_arg(obj));
      break;
    default:
      pips_internal_error("unexpected tag\n");
    }

    return(pc);
}

/* This one is exported. Outer parentheses are never useful. */
list /* of string */ words_expression(expression obj)
{
    return words_syntax(expression_syntax(obj));
}

/* exported for cmfortran.c
 */
list
words_subexpression(
    expression obj,
    int precedence,
    bool leftmost)
{
    list pc;

    if ( expression_call_p(obj) )
	pc = words_call(syntax_call(expression_syntax(obj)), precedence, leftmost, FALSE);
    else
	pc = words_syntax(expression_syntax(obj));

    return pc;
}


/**************************************************************** SENTENCE */

static sentence
sentence_tail(void)
{
  return MAKE_ONE_WORD_SENTENCE(0, strdup(prettyprint_is_fortran?"END":"}"));
}

/* exported for unstructured.c */
sentence
sentence_goto_label(
    entity __attribute__ ((unused)) module,
    string label,
    int margin,
    string tlabel,
    int n)
{
    list pc = words_goto_label(tlabel);

    return(make_sentence(is_sentence_unformatted,
	    make_unformatted(label?strdup(label):NULL, n, margin, pc)));
}

static sentence
sentence_goto(
    entity module,
    string label,
    int margin,
    statement obj,
    int n)
{
    string tlabel = entity_local_name(statement_label(obj)) +
      strlen(LABEL_PREFIX);
    pips_assert("Legal label required", strlen(tlabel)!=0);
    return sentence_goto_label(module, label, margin, tlabel, n);
}

/********************************************************************* TEXT */
static text text_block (entity module, string label, int margin, list objs,
			int n)
{
  text r = make_text(NIL);

  if (ENDP(objs) && !get_bool_property("PRETTYPRINT_EMPTY_BLOCKS")) {
    return(r) ;
  }

  if(!empty_string_p(label)) {
    pips_user_warning("Illegal label \"%s\". "
		      "Blocks cannot carry a label\n",
		      label);
  }

  unformatted bm_beg = NULL;
  unformatted bm_end = NULL;
  // test if block markers are required
  bool flg_marker = mark_block (&bm_beg, &bm_end, n, margin);

  // print the begin block marker if needed
  if (flg_marker == true) {
    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted, bm_beg));
  }
  else if ((get_bool_property("PRETTYPRINT_ALL_EFFECTS") ||
	    get_bool_property("PRETTYPRINT_BLOCKS"))
	   &&
	   get_bool_property("PRETTYPRINT_FOR_FORESYS"))
    ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
					  strdup("C$BB\n")));

  // append local variables if there is some
  r = insert_locals (r);

  // begin  block marker and declarations have already been printed
  // print the block instructions
  for (; objs != NIL; objs = CDR(objs)) {
    statement s = STATEMENT(CAR(objs));

    text t = text_statement_enclosed(module, margin, s, FALSE, TRUE);
    text_sentences(r) = gen_nconc(text_sentences(r), text_sentences(t));
    text_sentences(t) = NIL;
    free_text(t);
  }

  // print the end block marker if needed
  if (flg_marker == true) {
    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted, bm_end));
  }
  return r;
}

/* @return a list of string with the variable that need to be private in the
 * current context. The context takes care of the kind of output. For example
 * in the case of open mp the variables would be encapsulated into
 * the private() clause like this: private (a,b).
 * @param obj the loop to look at.
 */
static list /* of string */
loop_private_variables(loop obj)
{
    bool
        all_private = get_bool_property("PRETTYPRINT_ALL_PRIVATE_VARIABLES"),
	hpf_private = pp_hpf_style_p(),
	omp_private = pp_omp_style_p(),
	some_before = FALSE;
    list l = NIL;

    // list of local entities
    // In case of openmp the variable declared in the loop body should
    // not be made private, so ask for removing them from the list of locals.
    // If all_private is FALSE -> remove loop indice from the list of locals.
    list locals = loop_private_variables_as_entites (obj, omp_private, !all_private);

    /* comma-separated list of private variables.
     * built in reverse order to avoid adding at the end...
     */
    FOREACH (ENTITY, p, locals) {
      if (some_before)
	l = CHAIN_SWORD(l, ",");
      else
	some_before = TRUE; /* from now on commas, triggered... */
      l = gen_nconc(l, words_declaration(p,TRUE));
    }

    gen_free_list (locals);

    pips_debug(5, "#printed %zd/%zd\n", gen_length(l),
	       gen_length(loop_locals(obj)));

    /* stuff around if not empty
     */
    if (l)
    {
	string private;
	if (hpf_private) private = "NEW(";
	else if (omp_private) private = prettyprint_is_fortran? "PRIVATE(" : "private(";
	else private = "PRIVATE ";
	l = CONS(STRING, MAKE_SWORD(private), l);
	if (hpf_private || omp_private) CHAIN_SWORD(l, ")");
    }

    return l;
}

/* returns a formatted text for the HPF independent and new directive
 * well, no continuations and so, but the directives do not fit the
 * unformatted domain, because the directive prolog would not be well
 * managed there.
 */
static string
marged(
    string prefix,
    int margin)
{
    int len = strlen(prefix), i;
    string result = (string) malloc(strlen(prefix)+margin+1);
    strcpy(result, prefix);
    if(prettyprint_is_fortran) {
      for (i=len; margin-->0;)
	result[i++] = ' '; result[i]='\0';
    }
    return result;
}

static text
text_directive(
    loop obj,   /* the loop we're interested in */
    int margin,
    string basic_directive,
    string basic_continuation,
    string parallel)
{
    string
	dir = marged(basic_directive, margin),
	cont = marged(basic_continuation, margin);
    text t = make_text(NIL);
    char buffer[100]; /* ??? */
    list /* of string */ l = NIL;
    bool is_hpf = pp_hpf_style_p(), is_omp = pp_omp_style_p();
    bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

    /* start buffer */
    buffer[0] = '\0';

    if (execution_parallel_p(loop_execution(obj)))
    {
	add_to_current_line(buffer, dir, cont, t);
	add_to_current_line(buffer, parallel, cont, t);
	l = loop_private_variables(obj);
	if (l && is_hpf)
	    add_to_current_line(buffer, space_p? ", " : ",", cont, t);
    }
    else if (get_bool_property("PRETTYPRINT_ALL_PRIVATE_VARIABLES"))
    {
	l = loop_private_variables(obj);
	if (l)
	{
	    add_to_current_line(buffer, dir, cont, t);
	    if (is_omp) add_to_current_line(buffer, "DO ", cont, t);
	}
    }

    if (strlen(buffer)>0)
	MAP(STRING, s, add_to_current_line(buffer, s, cont, t), l);

    /* what about reductions? should be associated to the ri somewhere.
     */

    close_current_line(buffer, t,cont);
    free(dir);
    free(cont);
    return t;
}

#define HPF_SENTINEL 		"!HPF$"
#define HPF_DIRECTIVE 		HPF_SENTINEL " "
#define HPF_CONTINUATION 	HPF_SENTINEL "x"
#define HPF_INDEPENDENT 	"INDEPENDENT"

static text
text_hpf_directive(loop l, int m)
{
    return text_directive(l, m, "\n" HPF_DIRECTIVE, HPF_CONTINUATION,
			  HPF_INDEPENDENT);
}

#define OMP_SENTINEL 		"!$OMP"
#define OMP_DIRECTIVE 		OMP_SENTINEL " "
#define OMP_CONTINUATION 	OMP_SENTINEL "x"
#define OMP_PARALLELDO		"PARALLEL DO "
#define OMP_C_SENTINEL 		"#pragma omp"
#define OMP_C_DIRECTIVE 	OMP_C_SENTINEL " "
#define OMP_C_CONTINUATION 	OMP_C_SENTINEL "x"
#define OMP_C_PARALLELDO	"parallel for "

text
text_omp_directive(loop l, int m)
{
  text t = text_undefined;

  if(prettyprint_is_fortran)
    t = text_directive(l, m, "\n" OMP_DIRECTIVE, OMP_CONTINUATION,
		       OMP_PARALLELDO);
  else {
    // assume C
    // text_directive function takes care of private variables
    // More should be done to take care of shared variables, reductions
    // and other specific omp clause like lastprivate, copyin ...
    t = text_directive(l, m, OMP_C_DIRECTIVE, OMP_C_CONTINUATION,
		       OMP_C_PARALLELDO);

  }

  return t;
}

/* exported for fortran90.c */
text
text_loop_default(
    entity module,
    string label,
    int margin,
    loop obj,
    int n)
{
    list pc = NIL;
    sentence first_sentence = sentence_undefined;
    unformatted u;
    text r = make_text(NIL);
    statement body = loop_body( obj ) ;
    entity the_label = loop_label(obj);
    string do_label = entity_local_name(the_label)+strlen(LABEL_PREFIX) ;
    bool structured_do = entity_empty_label_p(the_label);
    bool doall_loop_p = FALSE;
    bool hpf_prettyprint = pp_hpf_style_p();
    bool do_enddo_p = get_bool_property("PRETTYPRINT_DO_LABEL_AS_COMMENT");
    bool all_private =  get_bool_property("PRETTYPRINT_ALL_PRIVATE_VARIABLES");

    if(execution_sequential_p(loop_execution(obj))) {
	doall_loop_p = FALSE;
    }
    else {
	doall_loop_p = pp_doall_style_p();
    }

    /* HPF directives before the loop if required (INDEPENDENT and NEW) */
    if (hpf_prettyprint)  MERGE_TEXTS(r, text_hpf_directive(obj, margin));
    /* idem if Open MP directives are required */
    if (pp_omp_style_p()) MERGE_TEXTS(r, text_omp_directive(obj, margin));

    /* LOOP prologue.
     */
    if(prettyprint_is_fortran)
      pc = CHAIN_SWORD(NIL, (doall_loop_p) ? "DOALL " : "DO " );
    else
      pc = CHAIN_SWORD(NIL, (doall_loop_p) ? "forall(" : "for(" );

    if(prettyprint_is_fortran && !structured_do && !doall_loop_p && !do_enddo_p) {
	pc = CHAIN_SWORD(pc, concatenate(do_label, " ", NULL));
    }
    //pc = CHAIN_SWORD(pc, entity_local_name(loop_index(obj)));
    pc = CHAIN_SWORD(pc, entity_user_name(loop_index(obj)));
    pc = CHAIN_SWORD(pc, " = ");


    if(prettyprint_is_fortran) {
      pc = gen_nconc(pc, words_loop_range(loop_range(obj)));
    }
    else {
      /* Assumed to be C */
      pc = gen_nconc(pc, C_loop_range(loop_range(obj), loop_index(obj)));
      if(!one_liner_p(body))
	pc = CHAIN_SWORD(pc," {");
    }

    if(prettyprint_is_fortran) {
      u = make_unformatted(strdup(label), n, margin, pc) ;
      ADD_SENTENCE_TO_TEXT(r, first_sentence =
			   make_sentence(is_sentence_unformatted, u));
    }
    else {
      /* Assumed to be C */
      if ((label != NULL) && (label[0] != '\0')) {
	pips_debug(9, "the label %s need to be print for a for C loop", label);
	u = make_unformatted(strdup(label), 0, 0, NULL);
	ADD_SENTENCE_TO_TEXT(r, first_sentence =
			     make_sentence(is_sentence_unformatted, u));
      }
      u = make_unformatted(NULL, n, margin, pc) ;
      ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));
    }

    /* builds the PRIVATE scalar declaration if required
     */
    if(!ENDP(loop_locals(obj)) && (doall_loop_p || all_private)
       && !hpf_prettyprint)
    {
	list /* of string */ lp = loop_private_variables(obj);

	// initialize the local variable text if needed
	if ((local_flg == false) && (lp)) {
	  local_flg = true;
	  local_var =  make_text(NIL);
	}

	if (lp)
	  ADD_SENTENCE_TO_TEXT
	    ( local_var,
	      make_sentence(is_sentence_unformatted,
			    make_unformatted(NULL, 0, margin+INDENTATION, lp)));
    }

    /* loop BODY
     */
    MERGE_TEXTS(r, text_statement_enclosed(module,
					   margin+INDENTATION,
					   body,
					   !one_liner_p(body),
					   !one_liner_p(body)));

    /* LOOP postlogue
     */

    if(!prettyprint_is_fortran) { /* i.e. is_C for the time being */
      if(!one_liner_p(body))
	 ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
    }
    else if (structured_do || doall_loop_p || do_enddo_p ||
	pp_cray_style_p() || pp_craft_style_p() || pp_cmf_style_p())
    {
	ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"ENDDO"));
    }

    attach_loop_to_sentence_up_to_end_of_text(first_sentence, r, obj);

    return r;
}

/* exported for conversion/look_for_nested_loops.c */
text text_loop(
    entity module,
    string label,
    int margin,
    loop obj,
    int n)
{
  text r = make_text(NIL);
  statement body = loop_body( obj ) ;
  entity the_label = loop_label(obj);
  string do_label = entity_local_name(the_label)+strlen(LABEL_PREFIX) ;
  bool structured_do = entity_empty_label_p(the_label);
  bool do_enddo_p = get_bool_property("PRETTYPRINT_DO_LABEL_AS_COMMENT");

  /* small hack to show the initial label of the loop to name it...
   */
  if(!structured_do && do_enddo_p)
    {
      ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
					    strdup(concatenate("!     INITIALLY: DO ", do_label, "\n", NULL))));
    }

  /* quite ugly management of other prettyprints...
   */
  switch(execution_tag(loop_execution(obj)) ) {
  case is_execution_sequential:
    MERGE_TEXTS(r, text_loop_default(module, label, margin, obj, n));
    break ;
  case is_execution_parallel:
    if (pp_cmf_style_p()) {
      text aux_r;
      if((aux_r = text_loop_cmf(module, label, margin, obj, n, NIL, NIL))
	 != text_undefined) {
	MERGE_TEXTS(r, aux_r);
      }
    }
    else if (pp_craft_style_p()) {
      text aux_r;
      if((aux_r = text_loop_craft(module, label, margin, obj, n, NIL, NIL))
	 != text_undefined) {
	MERGE_TEXTS(r, aux_r);
      }
    }
    else if (pp_f90_style_p() &&
	     instruction_assign_p(statement_instruction(body)) ) {
      MERGE_TEXTS(r, text_loop_90(module, label, margin, obj, n));
    }
    else {
      MERGE_TEXTS(r, text_loop_default(module, label, margin, obj, n));
    }
    break ;
  default:
    pips_internal_error("Unknown tag\n") ;
  }
  return r;
}

static text text_whileloop(
    entity module,
    string label,
    int margin,
    whileloop obj,
    int n)
{
    list pc = NIL;
    sentence first_sentence;
    unformatted u;
    text r = make_text(NIL);
    statement body = whileloop_body( obj ) ;
    entity the_label = whileloop_label(obj);
    string do_label = entity_local_name(the_label)+strlen(LABEL_PREFIX) ;
    bool structured_do = entity_empty_label_p(the_label);
    bool do_enddo_p = get_bool_property("PRETTYPRINT_DO_LABEL_AS_COMMENT");

    evaluation eval = whileloop_evaluation(obj);

    /* Show the initial label of the loop to name it...
     * FI: I believe this is useless for while loops since they cannot
     * be parallelized.
     */
    if(!structured_do && do_enddo_p)
    {
	ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
	  strdup(concatenate("!     INITIALLY: DO ", do_label, "\n", NULL))));
    }

    if (evaluation_before_p(eval))
      {
	if (prettyprint_is_fortran)
	  {
	    /* LOOP prologue.
	     */
	    pc = CHAIN_SWORD(NIL, "DO " );

	    if(!structured_do && !do_enddo_p) {
	      pc = CHAIN_SWORD(pc, concatenate(do_label, " ", NULL));
	    }
	    pc = CHAIN_SWORD(pc, "WHILE (");
	    pc = gen_nconc(pc, words_expression(whileloop_condition(obj)));
	    pc = CHAIN_SWORD(pc, ")");
	    u = make_unformatted(strdup(label), n, margin, pc) ;
	    ADD_SENTENCE_TO_TEXT(r, first_sentence =
				 make_sentence(is_sentence_unformatted, u));

	    /* loop BODY
	     */
	    MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, body));

	    /* LOOP postlogue
	     */
	    if (structured_do) {
	      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"ENDDO"));
	    }
	  }
	else if(one_liner_p(body))
	  {
	    pc = CHAIN_SWORD(NIL,"while (");
	    pc = gen_nconc(pc, words_expression(whileloop_condition(obj)));
	    pc = CHAIN_SWORD(pc,") ");
	    u = make_unformatted(strdup(label), n, margin, pc) ;
	    ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));
	    MERGE_TEXTS(r, text_statement_enclosed(module,
						   margin+INDENTATION,
						   body,
						   !one_liner_p(body),
						   !one_liner_p(body)));

	    //if (structured_do)
	    //ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
	  }
	else
	  {
	    pc = CHAIN_SWORD(NIL,"while (");
	    pc = gen_nconc(pc, words_expression(whileloop_condition(obj)));
	    pc = CHAIN_SWORD(pc,") {");
	    u = make_unformatted(strdup(label), n, margin, pc) ;
	    ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));
	    MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, body));
	    if (structured_do)
	    ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
	  }
      }
    else
      {
	/* C do { s; } while (cond); loop*/
	pc = CHAIN_SWORD(NIL,"do {");
	u = make_unformatted(strdup(label), n, margin, pc) ;
	ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));
	MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, body));
	ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
	pc = CHAIN_SWORD(NIL,"while (");
	pc = gen_nconc(pc, words_expression(whileloop_condition(obj)));
	pc = CHAIN_SWORD(pc, ");");
	u = make_unformatted(NULL, n, margin, pc) ;
	ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));
      }

    /* attach_loop_to_sentence_up_to_end_of_text(first_sentence, r, obj); */
    return r;
}

/* exported for unstructured.c
 */
text
init_text_statement(
    entity module,
    int margin,
    statement obj)
{
    instruction i = statement_instruction(obj);
    text r;

    if (get_bool_property("PRETTYPRINT_ALL_EFFECTS")
	|| !((instruction_block_p(i) &&
	      !get_bool_property("PRETTYPRINT_BLOCKS")) ||
	     (instruction_unstructured_p(i) &&
	      !get_bool_property("PRETTYPRINT_UNSTRUCTURED")))) {
      /* FI: before calling the hook,
       * statement_ordering(obj) should be checked */
	r = (*text_statement_hook)( module, margin, obj );
	if (text_statement_hook != empty_text)
	    attach_decoration_to_text(r);
    }
    else
	r  = make_text( NIL ) ;

    if (get_bool_property("PRETTYPRINT_ALL_EFFECTS") ||
	get_bool_property("PRETTYPRINT_STATEMENT_ORDERING")) {
	char *buffer;
	int so = statement_ordering(obj) ;

	if (!(instruction_block_p(statement_instruction(obj)) &&
	      (! get_bool_property("PRETTYPRINT_BLOCKS")))) {
	  if (so != STATEMENT_ORDERING_UNDEFINED)
	    asprintf(&buffer, "%s (%d,%d)\n", PIPS_COMMENT_SENTINEL,
		     ORDERING_NUMBER(so), ORDERING_STATEMENT(so));
	  else
	    asprintf(&buffer, "%s (statement ordering unavailable)\n",
		     PIPS_COMMENT_SENTINEL);
	  ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
						buffer));
	}
    }
    return( r ) ;
}

static text
text_logical_if(
    entity __attribute__ ((unused)) module,
    string label,
    int margin,
    test obj,
    int n)
{
  text r = make_text(NIL);
  list pc = NIL;
  statement tb = test_true(obj);

  pc = CHAIN_SWORD(pc, strdup(prettyprint_is_fortran?"IF (":"if ("));
  pc = gen_nconc(pc, words_expression(test_condition(obj)));
  pc = CHAIN_SWORD(pc, ") ");
  if(prettyprint_is_fortran) {
    instruction ti = statement_instruction(tb);
    call c = instruction_call(ti);
    pc = gen_nconc(pc, words_call(c, 0, TRUE, TRUE));
    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted,
				       make_unformatted(strdup(label), n,
							margin, pc)));
  }
  else {
    text t = text_statement(module, margin+INDENTATION, tb);
    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted,
				       make_unformatted(strdup(label), n,
							margin, pc)));
    text_sentences(r) = gen_nconc(text_sentences(r), text_sentences(t));
    text_sentences(t) = NIL;
    free_text(t);
  }
  ifdebug(8){
    fprintf(stderr,"logical_if=================================\n");
    print_text(stderr,r);
    fprintf(stderr,"==============================\n");
  }
  return(r);
}

static text
text_block_if(
    entity module,
    string label,
    int margin,
    test obj,
    int n)
{
    text r = make_text(NIL);
    list pc = NIL;
    statement test_false_obj;
    bool one_liner_true_statement = one_liner_p(test_true(obj));
    bool one_liner_false_statement = one_liner_p(test_false(obj));
    bool else_branch_p = FALSE; /* The else branch must be printed */

    pc = CHAIN_SWORD(pc, prettyprint_is_fortran?"IF (":"if (");
    pc = gen_nconc(pc, words_expression(test_condition(obj)));
    if(prettyprint_is_fortran)
      pc = CHAIN_SWORD(pc, ") THEN");
    else if(one_liner_true_statement){
      pc = CHAIN_SWORD(pc, ")");
    }
    else
      pc = CHAIN_SWORD(pc, ") {");

    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted,
				       make_unformatted(strdup(label), n,
							margin, pc)));
    MERGE_TEXTS(r, text_statement_enclosed(module,
					   margin+INDENTATION,
					   test_true(obj),
					   !one_liner_true_statement,
					   !one_liner_true_statement));

    test_false_obj = test_false(obj);
    if(statement_undefined_p(test_false_obj)){
	pips_error("text_test","undefined statement\n");
    }
    if (!statement_with_empty_comment_p(test_false_obj)
	||
	(!empty_statement_p(test_false_obj)
	 && !continue_statement_p(test_false_obj))
	||
	(empty_statement_p(test_false_obj)
	 && (get_bool_property("PRETTYPRINT_EMPTY_BLOCKS")))
	||
	(continue_statement_p(test_false_obj)
	 && (get_bool_property("PRETTYPRINT_ALL_LABELS"))))
      {
	else_branch_p = TRUE;
	if (prettyprint_is_fortran)
	  {
	    ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"ELSE"));
	  }
	else
	  {
	    if(!one_liner_true_statement)
	      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
	    if(one_liner_false_statement) {
	      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"else"));
	    }
	    else {
	      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"else {"));
	    }
	  }
	MERGE_TEXTS(r, text_statement(module, margin+INDENTATION,
				      test_false_obj));
      }

    if(prettyprint_is_fortran)
      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,strdup("ENDIF")))
    else if((!else_branch_p && !one_liner_true_statement)
	    || (else_branch_p && !one_liner_false_statement))
      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,strdup("}")))

    ifdebug(8){
      fprintf(stderr,"text_block_if=================================\n");
      print_text(stderr,r);
      fprintf(stderr,"==============================\n");
    }

    return(r);
}

static text
text_io_block_if(
    entity module,
    string label,
    int margin,
    test obj,
    int n)
{
    text r = make_text(NIL);
    list pc = NIL;
    string strglab= local_name(new_label_name(module))+1;

    if (!empty_statement_p(test_true(obj))) {

      r = make_text(CONS(SENTENCE,
			 sentence_goto_label(module, label, margin,
					     strglab, n),
			 NIL));

      ADD_SENTENCE_TO_TEXT(r,
			   make_sentence(is_sentence_unformatted,
					 make_unformatted(strdup(label), n,
							  margin, pc)));
      MERGE_TEXTS(r, text_statement(module, margin,
				    test_true(obj)));






      ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted,
					    make_unformatted(strdup(strglab), n, margin,
							     CONS(STRING,
								  strdup(prettyprint_is_fortran?"CONTINUE":";"), NIL))));
    }

    if (!empty_statement_p(test_false(obj)))
      MERGE_TEXTS(r, text_statement(module, margin,
				    test_false(obj)));

    return(r);
}

static text
text_block_ifthen(
    entity module,
    string label,
    int margin,
    test obj,
    int n)
{
    text r = make_text(NIL);
    list pc = NIL;
    statement tb=test_true(obj);

    pc = CHAIN_SWORD(pc, prettyprint_is_fortran?"IF (":"if (");
    pc = gen_nconc(pc, words_expression(test_condition(obj)));
    pc = CHAIN_SWORD(pc, prettyprint_is_fortran?") THEN": (one_liner_p(tb)?")":") {"));

    ADD_SENTENCE_TO_TEXT(r,
			 make_sentence(is_sentence_unformatted,
				       make_unformatted(strdup(label), n,
							margin, pc)));
    MERGE_TEXTS(r, text_statement_enclosed(module,
					   margin+INDENTATION,
					   tb,
					   !one_liner_p(tb),
					   !one_liner_p(tb)));
    if (!prettyprint_is_fortran && !one_liner_p(tb))
      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
    return(r);
}

static text text_block_else(entity module,
			    string __attribute__ ((unused)) label,
			    int margin,
			    statement stmt,
			    int __attribute__ ((unused)) n)
{
  text r = make_text(NIL);

  if (!statement_with_empty_comment_p(stmt)
      ||
      (!empty_statement_p(stmt)
       && !continue_statement_p(stmt))
      ||
      (empty_statement_p(stmt)
       && (get_bool_property("PRETTYPRINT_EMPTY_BLOCKS")))
      ||
      (continue_statement_p(stmt)
       && (get_bool_property("PRETTYPRINT_ALL_LABELS"))))
    {
      //code added by Amira Mensi
      if (prettyprint_is_fortran) {
	ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin, "ELSE"));
	MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, stmt));
      }
      else { //C assumed
	if (one_liner_p(stmt)){
	  ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"else"));
	  MERGE_TEXTS(r, text_statement_enclosed(module,
						 margin+INDENTATION,
						 stmt,
						 FALSE,
						 FALSE));
	}
	else {
	  ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin, "else {"));
	  MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, stmt));
	  ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin, "}"));
	}
      }
    }

  return r;
}

static text
text_block_elseif(
    entity module,
    string label,
    int margin,
    test obj,
    int n)
{
  text r = make_text(NIL);
  list pc = NIL;
  statement tb = test_true(obj);
  statement fb = test_false(obj);

  pc = CHAIN_SWORD(pc, strdup(prettyprint_is_fortran?"ELSEIF (":"else if ("));
  pc = gen_nconc(pc, words_expression(test_condition(obj)));
  pc = CHAIN_SWORD(pc, strdup(prettyprint_is_fortran?") THEN":(one_liner_p(tb)?")":") {")));
  ADD_SENTENCE_TO_TEXT(r,
		       make_sentence(is_sentence_unformatted,
				     make_unformatted(strdup(label), n,
						      margin, pc)));

  MERGE_TEXTS(r, text_statement_enclosed(module,
					 margin+INDENTATION,
					 tb,
					 !one_liner_p(tb),
					 !one_liner_p(tb)));

  if (!prettyprint_is_fortran && !one_liner_p(tb)) {
    ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin, strdup("}")));
  }

  if(statement_test_p(fb)
     && empty_comments_p(statement_comments(fb))
     && entity_empty_label_p(statement_label(fb))) {
    MERGE_TEXTS(r, text_block_elseif(module,
				     label_local_name(statement_label(fb)),
				     margin,
				     statement_test(fb), n));

  } else {
    MERGE_TEXTS(r, text_block_else(module, label, margin, fb, n));
  }
  ifdebug(8){
    fprintf(stderr,"elseif=================================\n");
    print_text(stderr,r);
    fprintf(stderr,"==============================\n");
  }
  return(r);
}

static text
text_test(
    entity module,
    string label,
    int margin,
    test obj,
    int n)
{
    text r = text_undefined;
    statement tb = test_true(obj);
    statement fb = test_false(obj);

    /* 1st case: one statement in the true branch => Fortran logical IF */
    if(nop_statement_p(fb)
       && statement_call_p(tb)
       && entity_empty_label_p(statement_label(tb))
       && empty_comments_p(statement_comments(tb))
       && !continue_statement_p(tb)
       && !get_bool_property("PRETTYPRINT_BLOCK_IF_ONLY")
       && !(call_contains_alternate_returns_p(statement_call(tb))
	    && get_bool_property("PRETTYPRINT_REGENERATE_ALTERNATE_RETURNS"))) {
	r = text_logical_if(module, label, margin, obj, n);
    }
    /* 2nd case: one test in the false branch => "ELSEIF" Fortran block or "else if" C construct */
    else if(statement_test_p(fb)
	    && empty_comments_p(statement_comments(fb))
	    && entity_empty_label_p(statement_label(fb))
	    && !get_bool_property("PRETTYPRINT_BLOCK_IF_ONLY")) {


	r = text_block_ifthen(module, label, margin, obj, n);
	MERGE_TEXTS(r, text_block_elseif
		    (module,
		     label_local_name(statement_label(fb)),
		     margin, statement_test(fb), n));

	if(prettyprint_is_fortran)
	  ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"ENDIF"));

	/* r = text_block_if(module, label, margin, obj, n); */
    }
    else {
	syntax c = expression_syntax(test_condition(obj));

	if (syntax_reference_p(c)
	    && io_entity_p(reference_variable(syntax_reference(c)))
	    &&  !get_bool_property("PRETTYPRINT_CHECK_IO_STATEMENTS"))
	  r = text_io_block_if(module, label, margin, obj, n);
	else
	  r = text_block_if(module, label, margin, obj, n);
    }
    ifdebug(8){
    fprintf(stderr,"text_test=================================\n");
    print_text(stderr,r);
    fprintf(stderr,"==============================\n");
  }
    return r;
}

/* hook for adding something in the head. used by hpfc.
 * done so to avoid hpfc->prettyprint dependence in the libs.
 * FC. 29/12/95.
 */
static string (*head_hook)(entity) = NULL;
void set_prettyprinter_head_hook(string(*f)(entity)){ head_hook=f;}
void reset_prettyprinter_head_hook(){ head_hook=NULL;}

static text
text_instruction(
    entity module,
    string label,
    int margin,
    instruction obj,
    int n)
{
  text r = text_undefined;

  switch (instruction_tag(obj)) {
  case is_instruction_block:
    {
      r = text_block(module, label, margin, instruction_block(obj), n) ;
      break;
    }
  case is_instruction_test:
    {
      r = text_test(module, label, margin, instruction_test(obj), n);
      break;
    }
  case is_instruction_loop:
    {
      r = text_loop(module, label, margin, instruction_loop(obj), n);
      break;
    }
  case is_instruction_whileloop:
    {
      r = text_whileloop(module, label, margin, instruction_whileloop(obj), n);
      break;
    }
  case is_instruction_goto:
    {
      r = make_text(CONS(SENTENCE,
			 sentence_goto(module, label, margin,
				       instruction_goto(obj), n), NIL));
      break;
    }
  case is_instruction_call:
    {
      unformatted u;
      sentence s;
      if (instruction_continue_p(obj) &&
	  empty_string_p(label) &&
	  !get_bool_property("PRETTYPRINT_ALL_LABELS")) {
	pips_debug(5, "useless CONTINUE not printed\n");
	r = make_text(NIL);
      }
      else {
	if (prettyprint_is_fortran)
	  u = make_unformatted(strdup(label), n, margin,
			       words_call(instruction_call(obj),
					  0, TRUE, TRUE));
	else // C
	  u = make_unformatted(strdup(label), n, margin,
			     CHAIN_SWORD(words_call(instruction_call(obj),
						    0, TRUE, TRUE),
					 strdup(";")));
	s = make_sentence(is_sentence_unformatted, u);
	r = make_text(CONS(SENTENCE, s, NIL));
      }
      break;
    }
  case is_instruction_unstructured:
    {
      // append local variables if there is some.
      // local variable need to be inserted before diging the
      // unstructured graph.
      r = insert_locals (r);

      text tmp = text_undefined;
      tmp = text_unstructured(module, label, margin,
			      instruction_unstructured(obj), n);

      // append the unstructured to the current text if it exists
      if ((r != text_undefined) && (r != NULL)) {
	MERGE_TEXTS (r, tmp);
      }
      else {
	r = tmp;
      }

      break;
    }
  case is_instruction_forloop:
    {
      r = text_forloop(module, label, margin, instruction_forloop(obj), n);
      break;
    }
  case is_instruction_expression:
    {
      list pc = words_expression(instruction_expression(obj));
      unformatted u;
      pc = CHAIN_SWORD(pc,";");
      u = make_unformatted(strdup(label), n, margin, pc) ;
      r = make_text(CONS(SENTENCE,make_sentence(is_sentence_unformatted, u),NIL));
      break;
    }
  default:
    {
      pips_error("text_instruction", "unexpected tag");
    }
  }
  return(r);
}

/* In case the input code is not C code, non-standard comments have to be detected */
bool  C_comment_p(string c){
  bool is_C_comment=TRUE;
  char * ccp=c;
  char cc=' ';

 init:
  cc=*ccp++;
  if(cc==' '|| cc=='\t' || cc=='\n')
   goto init;
 else if( cc=='/')
   goto slash;
 else if(cc=='\000')
   goto end;
 else {
   is_C_comment=FALSE;
   goto end;
 }

 slash:
  cc=*ccp++;
  if(cc=='*')
   goto slash_star;
 else if(cc=='/')
   goto slash_slash;
 else{
   is_C_comment=FALSE;
   goto end;
 }

 slash_star:
   cc=*ccp++;
 if(cc=='*')
   goto slash_star_star;
  else if(cc=='\0'){
    is_C_comment=FALSE;
    goto end;
  }
 else
   goto slash_star;

 slash_slash:
  cc=*ccp++;
  if(cc=='\n')
   goto init;
  if(cc=='\0') // The comment may not end first with a '\n'
     goto end;
 else
   goto slash_slash;

 slash_star_star:
  cc=*ccp++;
  if(cc=='/')
   goto init;
  else if(cc=='*')
    goto slash_star_star;
  else if(cc=='\0'){
    is_C_comment=FALSE;
    goto end;
  }
  else
   goto slash_star;

 end : return is_C_comment;
}

/* In case comments are not formatted according to C rules, e.g. when
   prettyprinting Fortran code as C code, add // at beginning of lines   */
text C_any_comment_to_text(int r_margin, string c)
{
  string lb = c; /* line beginning */
  string le = c; /* line end */
  string cp = c; /* current position, pointer in comments */
  text ct = make_text(NIL);
  bool is_C_comment = C_comment_p(c);
  int e_margin = r_margin;

  /* We do not need spaces before a line feed */
  if(strcmp(c, "\n")==0)
    e_margin = 0;

  if(strlen(c)>0) {
    for(;*cp!='\0';cp++) {
      if(*cp=='\n') {
	if(cp!=c || TRUE){ // Do not skip \n
	  string cl = gen_strndup0(lb, le-lb);
	  sentence s = sentence_undefined;
	  if(is_C_comment)
	    s = MAKE_ONE_WORD_SENTENCE(e_margin, cl);
	  else if(strlen(cl)>0){
	    list pc = CHAIN_SWORD(NIL, cl); // cl is uselessly duplicated
	    pc = CONS(STRING, MAKE_SWORD("//"), pc);
	    s= make_sentence(is_sentence_unformatted,
			     make_unformatted((char *) NULL, 0, e_margin, pc));
	  }
	  else {
	    s = MAKE_ONE_WORD_SENTENCE(0, cl);
	  }
	  ADD_SENTENCE_TO_TEXT(ct, s);
	  free(cl);
	}
	lb = cp+1;
	le = cp+1;
      }
      else
	le++;
    }
    // Final \n has been removed in the parser presumably by Ronan
    if(lb<cp){
      string s = gen_strndup0(lb,le-lb);
      ADD_SENTENCE_TO_TEXT(ct,MAKE_ONE_WORD_SENTENCE(e_margin,s));
      free(s);
    } else{
      //ADD_SENTENCE_TO_TEXT(ct,MAKE_ONE_WORD_SENTENCE(0,""));
      ;
    }
  }
  else{// Final \n has been removed by Ronan
    //ADD_SENTENCE_TO_TEXT(ct,MAKE_ONE_WORD_SENTENCE(0,""));
    ;
  }

  return ct;
}

// Ronan's improved version is bugged. It returns many lines for a
// unique \n because le is not updated before looping. Has this code
// been validated?
text C_standard_comment_to_text(int margin, string comment)
{
  string line;
  string le = comment; /* position of a line end */
  text ct = make_text(NIL);

  do {
    /* Find the first end of line: */
    le = strchr(comment, '\n');
    if (le == NULL)
      /* No end-of-line, so use all the rest of the comment: */
      line = strdup(comment);
    else {
      /* Skip the '\n' at the end since the line concept is the notion of
	 sentence */
      line = gen_strndup0(comment, le - comment);
      /* Analyze the next line: */
      comment = le + 1;
    }
    /* Do not indent if the line is empty */
    ADD_SENTENCE_TO_TEXT(ct,
			 MAKE_ONE_WORD_SENTENCE(line[0] == '\0' ? 0 : margin,
						line));
  } while (le != NULL);
  return ct;
}

/* Special handling for C comments  with each line indented according to
   the context.

   I do not see the interest if the user code is already indented... RK
   OK, since the blanks outside the comments are removed by the parser.
*/
text C_comment_to_text(int margin, string comment)
{
  text ct = text_undefined;

  if(C_comment_p(comment))
    //ct = C_standard_comment_to_text(margin, comment);
    ct = C_any_comment_to_text(margin, comment);
  else
    ct = C_any_comment_to_text(margin, comment);
  return ct;
}

text text_statement_enclosed(entity module,
			     int imargin,
			     statement stmt,
			     bool braces_p,
			     bool drop_continue_p)
{
  instruction i = statement_instruction(stmt);
  text r= make_text(NIL);
  text temp;
  string label =
    entity_local_name(statement_label(stmt)) + strlen(LABEL_PREFIX);
  string comments = statement_comments(stmt);
  bool braces_added = FALSE;
  int nmargin = imargin;

  // To ease breakpoint setting
  //pips_assert("Blocks have no comments", !instruction_block_p(i)||empty_comments_p(comments));
  if(instruction_block_p(i) && !empty_comments_p(comments)) {
    pips_internal_error("Blocks should have no comments\n");
  }

  // the first thing to do is to print the statement extensions
  string ext =  extensions_to_string (statement_extensions (stmt), TRUE);
  if (ext != string_undefined) {
    ADD_SENTENCE_TO_TEXT(r,make_sentence(is_sentence_formatted, ext));
  }

  /* 31/07/2003 Nga Nguyen : This code is added for C, because a
     statement can have its own declarations */
  list l = statement_declarations(stmt);

  if (!ENDP(l) && !prettyprint_is_fortran) {
    if(!braces_p) {
      braces_added = TRUE;
      ADD_SENTENCE_TO_TEXT(r,
			   MAKE_ONE_WORD_SENTENCE(imargin, "{"));
      nmargin += INDENTATION;
    }
    // initialize the local variable text if needed
    if (local_flg == false) {
      local_flg = true;
      local_var =  make_text(NIL);
    }
    MERGE_TEXTS(local_var, c_text_entities(module,l,nmargin));
  }

  pips_debug(2, "Begin for statement %s with braces_p=%d\n",
	     statement_identification(stmt),braces_p);
  pips_debug(9, "statement_comments: --%s--\n",
	     string_undefined_p(comments)? "<undef>": comments);

  if(statement_number(stmt)!=STATEMENT_NUMBER_UNDEFINED &&
     statement_ordering(stmt)==STATEMENT_ORDERING_UNDEFINED) {
    /* we are in trouble with some kind of dead (?) code...
       but we might as well be dealing with some parsed_code */
    pips_debug(1, "I unexpectedly bumped into dead code?\n");
  }

  if (same_string_p(label, RETURN_LABEL_NAME))
    {
      pips_assert("Statement with return label must be a return statement",
		  return_statement_p(stmt));

      /* do not add a redundant RETURN before an END, unless requested */
      if(get_bool_property("PRETTYPRINT_FINAL_RETURN")
	 || !last_statement_p(stmt))
	{
	  sentence s = MAKE_ONE_WORD_SENTENCE(nmargin, RETURN_FUNCTION_NAME);
	  temp = make_text(CONS(SENTENCE, s ,NIL));
	}
      else {
	temp = make_text(NIL);
      }
    }
  else
    {
      entity m = entity_undefined_p(module)?
	get_current_module_entity()
	: module;
      if(!compilation_unit_p(entity_name(m))) {
	/* Do we need to print this CONTINUE statement in C? */
	string cs = statement_comments(stmt);
	entity l = statement_label(stmt);

	if(!get_prettyprint_is_fortran()
	   && (braces_p || drop_continue_p)
	   && empty_label_p(entity_local_name(l))
	   && instruction_continue_p(i)) {
	  if(string_undefined_p(cs) || cs == NULL || strcmp(cs, "")==0) {
	    sentence s = MAKE_ONE_WORD_SENTENCE(0, "");
	    temp = make_text(CONS(SENTENCE, s ,NIL));
	    //temp = make_text(NIL);
	  }
	  else if(strcmp(cs, "\n")==0) {
	    sentence s = MAKE_ONE_WORD_SENTENCE(0, "\n");
	    temp = make_text(CONS(SENTENCE, s ,NIL));
	  }
	  else
	    temp = text_instruction(module, label, nmargin, i,
				    statement_number(stmt));
	}
	else
	  temp = text_instruction(module, label, nmargin, i,
				  statement_number(stmt));
      }
      else
	temp = make_text(NIL);
    }

  // append local variables  that might
  // have not been inserted previously
  r = insert_locals (r);

  /* note about comments: they are duplicated here, but I'm pretty
   * sure that the free is NEVER performed as it should. FC.
   */
  if(!ENDP(text_sentences(temp))) {
    MERGE_TEXTS(r, init_text_statement(module, nmargin, stmt));
    if (! empty_comments_p(comments)) {
      if(prettyprint_is_fortran) {
	ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
					      strdup(comments)));
      }
      else {
	text ct = text_undefined;

	ct = C_comment_to_text(nmargin, comments);
	MERGE_TEXTS(r, ct);
      }
    }
    MERGE_TEXTS(r, temp);
  }
  else {
    /* Preserve comments and empty C instruction */
    if (! empty_comments_p(comments)) {
      if(prettyprint_is_fortran) {
	ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
					      strdup(comments)));
      }
      else {
	text ct = C_comment_to_text(nmargin, comments);
	MERGE_TEXTS(r, ct);
      }
    }
    else if(!prettyprint_is_fortran && !braces_p && !braces_added) {
      // Because C braces can be eliminated and hence semi-colon
      // may be mandatory in a test branch or in a loop body.
      // A. Mensi
      sentence s = MAKE_ONE_WORD_SENTENCE(nmargin, strdup(";"));
      ADD_SENTENCE_TO_TEXT(r, s);
    }
    free_text(temp);
  }

  if (braces_added) {
    ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(imargin, "}"));
  }
  attach_statement_information_to_text(r, stmt);

  // the last thing to do is close the extension
  string close =  close_extensions (statement_extensions (stmt), TRUE);
  if (close != string_undefined) {
    ADD_SENTENCE_TO_TEXT(r,make_sentence(is_sentence_formatted, close));
  }

  ifdebug(1) {
    if (instruction_sequence_p(i)) {
      if(!(statement_with_empty_comment_p(stmt)
	   && statement_number(stmt) == STATEMENT_NUMBER_UNDEFINED
	   && unlabelled_statement_p(stmt))) {
	user_log("Block statement %s\n"
		 "Block number=%d, Block label=\"%s\", block comment=\"%s\"\n",
		 statement_identification(stmt),
		 statement_number(stmt), label_local_name(statement_label(stmt)),
		 statement_comments(stmt));
	pips_internal_error("This block statement should be labelless,"
			    " numberless and commentless.\n");
      }
    }
  }
  ifdebug(8){
    fprintf(stderr,"text_statement_enclosed=================================\n");
    print_text(stderr,r);
    fprintf(stderr,"==============================\n");
  }

  pips_debug(2, "End for statement %s\n", statement_identification(stmt));

  return(r);
}

/* Handles all statements but tests that are nodes of an unstructured.
 * Those are handled by text_control.
 */
text text_statement(
    entity module,
    int margin,
    statement stmt)
{
  return text_statement_enclosed(module, margin, stmt, TRUE, TRUE);
}

/* Keep track of the last statement to decide if a final return can be omitted
 * or not. If no last statement can be found for sure, for instance because it
 * depends on the prettyprinter, last_statement is set to statement_undefined
 * which is safe.
 */
static statement last_statement = statement_undefined;

statement
find_last_statement(statement s)
{
    statement last = statement_undefined;

    pips_assert("statement is defined", !statement_undefined_p(s));

    if(block_statement_p(s)) {
	list ls = instruction_block(statement_instruction(s));

	last = (ENDP(ls)? statement_undefined : STATEMENT(CAR(gen_last(ls))));
    }
    else if(unstructured_statement_p(s)) {
	unstructured u = instruction_unstructured(statement_instruction(s));
	list trail = unstructured_to_trail(u);

	last = control_statement(CONTROL(CAR(trail)));

	gen_free_list(trail);
    }
    else if(statement_call_p(s)) {
	/* Hopefully it is a return statement.
	 * Since the semantics of STOP is ignored by the parser, a
	 * final STOp should be followed by a RETURN.
	 */
	last = s;
    }
    else {
	/* loop or test cannot be last statements of a module */
	last = statement_undefined;
    }

    /* recursive call */
    if(!statement_undefined_p(last)
       && (block_statement_p(last) || unstructured_statement_p(last))) {
	last = find_last_statement(last);
    }

    /* Too many program transformations and syntheses violate the following assert */
    if(!(statement_undefined_p(last)
	 || !block_statement_p(s)
	 || return_statement_p(last))) {
      if (prettyprint_is_fortran) /* to avoid this warning for C, is it right for C ?*/
	{
	  pips_user_warning("Last statement is not a RETURN!\n");
	}
      last = statement_undefined;
    }

    /* I had a lot of trouble writing the condition for this assert... */
    pips_assert("Last statement is either undefined or a call to return",
	 statement_undefined_p(last) /* let's give up: it's always safe */
     || !block_statement_p(s) /* not a block: any kind of statement... */
		|| return_statement_p(last)); /* if a block, then a return */

    return last;
}

void
set_last_statement(statement s)
{
    statement ls = statement_undefined;
    pips_assert("last statement is undefined",
		statement_undefined_p(last_statement));
    ls = find_last_statement(s);
    last_statement = ls;
}

void
reset_last_statement()
{
    last_statement = statement_undefined;
}

bool
last_statement_p(statement s) {
    pips_assert("statement is defined\n", !statement_undefined_p(s));
    return s == last_statement;
}

/* Build the text of a module.

   The original text of the declarations is used if possible in
   Fortran. Otherwise, the function text_declaration is called.
 */
text
text_named_module(
    entity name, /**< the name of the module */
    entity module,
    statement stat)
{
  text r = make_text(NIL);
  code c = entity_code(module);
  string s = code_decls_text(c);
  text ral = text_undefined;

  debug_on("PRETTYPRINT_DEBUG_LEVEL");
  prettyprint_is_fortran = !get_bool_property("PRETTYPRINT_C_CODE");

  /* This guard is correct but could be removed if find_last_statement()
   * were robust and/or if the internal representations were always "correct".
   * See also the guard for reset_last_statement()
   */
  if(!get_bool_property("PRETTYPRINT_FINAL_RETURN"))
    set_last_statement(stat);

  precedence_p = !get_bool_property("PRETTYPRINT_ALL_PARENTHESES");

  if (prettyprint_is_fortran)
    {
      if ( strcmp(s,"") == 0
	   || get_bool_property("PRETTYPRINT_ALL_DECLARATIONS") )
	{
	  if (get_bool_property("PRETTYPRINT_HEADER_COMMENTS"))
	    /* Add the original header comments if any: */
	    ADD_SENTENCE_TO_TEXT(r, get_header_comments(module));

	  ADD_SENTENCE_TO_TEXT(r,
			       attach_head_to_sentence(sentence_head(name), module));

	  if (head_hook)
	    ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_formatted,
						  head_hook(module)));

	  if (get_bool_property("PRETTYPRINT_HEADER_COMMENTS"))
	    /* Add the original header comments if any: */
	    ADD_SENTENCE_TO_TEXT(r, get_declaration_comments(module));

	  MERGE_TEXTS(r, text_declaration(module));
	  MERGE_TEXTS(r, text_initializations(module));
	}
      else
	{
	  ADD_SENTENCE_TO_TEXT(r,
			       attach_head_to_sentence(make_sentence(is_sentence_formatted,
								     strdup(s)),
						       module));
	}
    }
  else
    {
      /* C prettyprinter */
      pips_debug(3,"Prettyprint function %s\n",entity_name(name));
      if (!compilation_unit_p(entity_name(name)))
	{
	  /* Print function header if the current module is not a compilation unit*/
	  ADD_SENTENCE_TO_TEXT(r,attach_head_to_sentence(sentence_head(name), module));
	  ADD_SENTENCE_TO_TEXT(r,MAKE_ONE_WORD_SENTENCE(0,"{"));
	  /* get the declarations for Fortran codes prettyrinted as C,
	     as the declarations are not located in the module
	     statement. A.Mensi */
	  if(ENDP(statement_declarations(stat)) && fortran_module_p(module)) {
	    list l = code_declarations(value_code(entity_initial(module)));

	    MERGE_TEXTS(r,c_text_entities(module, l, INDENTATION));
	  }
	}
    }

  set_alternate_return_set();
  reset_label_counter();

  if (stat != statement_undefined) {
    MERGE_TEXTS(r,
		text_statement(module,
			       (prettyprint_is_fortran||compilation_unit_p(entity_name(name)))?0:INDENTATION,
			       stat));
  }

  ral = generate_alternate_return_targets();
  reset_alternate_return_set();
  MERGE_TEXTS(r, ral);

  if (!compilation_unit_p(entity_name(name)) || prettyprint_is_fortran)
    {
      /* No need to print TAIL (}) if the current module is a C compilation unit*/
      ADD_SENTENCE_TO_TEXT(r, sentence_tail());
    }

  if(!get_bool_property("PRETTYPRINT_FINAL_RETURN"))
    reset_last_statement();

  debug_off();
  return(r);
}

text
text_module(
    entity module,
    statement stat)
{
    return text_named_module(module, module, stat);
}

text text_graph(), text_control() ;
string control_slabel() ;


/* The node itentifiers are generated from the ordering, more stable than
   the control node address: */
void
add_control_node_identifier_to_text(text r, control c) {
  _int so = statement_ordering(control_statement(c));
  add_one_unformated_printf_to_text(r, "c_%d_%d",
				    ORDERING_NUMBER(so),
				    ORDERING_STATEMENT(so));
}

void output_a_graph_view_of_the_unstructured_successors(text r,
							entity module,
							int margin,
							control c)
{
  _int so = statement_ordering(control_statement(c));
  add_one_unformated_printf_to_text(r, "%s ",
				    PRETTYPRINT_UNSTRUCTURED_ITEM_MARKER);
  add_control_node_identifier_to_text(r, c);
  add_one_unformated_printf_to_text(r, "\n");

  if (get_bool_property("PRETTYPRINT_UNSTRUCTURED_AS_A_GRAPH_VERBOSE")) {
    add_one_unformated_printf_to_text(r, "C Unstructured node %p ->", c);
    MAP(CONTROL, a_successor,
	so = statement_ordering(control_statement(a_successor));
	add_one_unformated_printf_to_text(r, " %p", a_successor),
	control_successors(c));
    add_one_unformated_printf_to_text(r,"\n");
  }

  MERGE_TEXTS(r, text_statement(module,
				margin,
				control_statement(c)));

  add_one_unformated_printf_to_text(r,
				    PRETTYPRINT_UNSTRUCTURED_SUCC_MARKER);
  MAP(CONTROL, a_successor,
      {
	add_one_unformated_printf_to_text(r, " ");
	add_control_node_identifier_to_text(r, a_successor);
      },
      control_successors(c));
  add_one_unformated_printf_to_text(r,"\n");
}


bool output_a_graph_view_of_the_unstructured_from_a_control(text r,
							    entity module,
							    int margin,
							    control begin_control,
							    control exit_control)
{
  bool exit_node_has_been_displayed = FALSE;
  list blocs = NIL;

  CONTROL_MAP(c,
	      {
		/* Display the statements of each node followed by
		   the list of its successors if any: */
		output_a_graph_view_of_the_unstructured_successors(r,
								   module,
								   margin,
								   c);
		if (c == exit_control)
		  exit_node_has_been_displayed = TRUE;
	      },
	      begin_control,
	      blocs);
  gen_free_list(blocs);

  return exit_node_has_been_displayed;
}

void output_a_graph_view_of_the_unstructured(text r,
					     entity module,
					     string __attribute__ ((unused)) label,
					     int margin,
					     unstructured u,
					     int __attribute__ ((unused)) num)
{
  bool exit_node_has_been_displayed = FALSE;
  control begin_control = unstructured_control(u);
  control end_control = unstructured_exit(u);

  add_one_unformated_printf_to_text(r, "%s ",
				    PRETTYPRINT_UNSTRUCTURED_BEGIN_MARKER);
  add_control_node_identifier_to_text(r, begin_control);
  add_one_unformated_printf_to_text(r, " end: ");
  add_control_node_identifier_to_text(r, end_control);
  add_one_unformated_printf_to_text(r, "\n");

  exit_node_has_been_displayed =
    output_a_graph_view_of_the_unstructured_from_a_control(r,
							   module,
							   margin,
							   begin_control,
							   end_control);

  /* If we have not displayed the exit node, that mean that it is not
     connex with the entry node and so the code is
     unreachable. Anyway, it has to be displayed as for the classical
     Sequential View: */
  if (! exit_node_has_been_displayed) {
    /* Note that since the controlizer adds a dummy successor to the
       exit node, use
       output_a_graph_view_of_the_unstructured_from_a_control()
       instead of
       output_a_graph_view_of_the_unstructured_successors(): */
    output_a_graph_view_of_the_unstructured_from_a_control(r,
							   module,
							   margin,
							   end_control,
							   end_control);
    /* Even if the code is unreachable, add the fact that the
       control above is semantically related to the entry node. Add
       a dash arrow from the entry node to the exit node in daVinci,
       for example: */
    add_one_unformated_printf_to_text(r, "%s ",
				      PRETTYPRINT_UNREACHABLE_EXIT_MARKER);
    add_control_node_identifier_to_text(r, begin_control);
    add_one_unformated_printf_to_text(r, " -> ");
    add_control_node_identifier_to_text(r, end_control);
    add_one_unformated_printf_to_text(r, "\n");
    if (get_bool_property("PRETTYPRINT_UNSTRUCTURED_AS_A_GRAPH_VERBOSE"))
      add_one_unformated_printf_to_text(r, "C Unreachable exit node (%p -> %p)\n",
					begin_control,
					end_control);
  }

  add_one_unformated_printf_to_text(r, "%s ",
				    PRETTYPRINT_UNSTRUCTURED_END_MARKER);
  add_control_node_identifier_to_text(r, begin_control);
  add_one_unformated_printf_to_text(r, " end: ");
  add_control_node_identifier_to_text(r, end_control);
  add_one_unformated_printf_to_text(r, "\n");
}


/* ================C prettyprinter functions================= */

static list words_cast(cast obj)
{
  list pc = NIL;
  type t = cast_type(obj);
  expression exp = cast_expression(obj);
  bool space_p = get_bool_property("PRETTYPRINT_LISTS_WITH_SPACES");

  pc = CHAIN_SWORD(pc,"(");
  pc = gen_nconc(pc, c_words_entity(t,NIL));
  pc = CHAIN_SWORD(pc, space_p? ") " : ")");
  pc = gen_nconc(pc, words_expression(exp));
  return pc;
}

static list words_sizeofexpression(sizeofexpression obj)
{
  list pc = NIL;
  pc = CHAIN_SWORD(pc,"sizeof(");
  if (sizeofexpression_type_p(obj)) {
    list pca = words_type(sizeofexpression_type(obj));
    pc = gen_nconc(pc, pca);
  }
  else
    pc = gen_nconc(pc, words_expression(sizeofexpression_expression(obj)));
  pc = CHAIN_SWORD(pc,")");
  return pc;
}

static list words_subscript(subscript s)
{
  list pc = NIL;
  expression a = subscript_array(s);
  list lexp = subscript_indices(s);
  bool first = TRUE;
  /* Parentheses must be added for array expression like __ctype+1 in (__ctype+1)[*np]*/
  pc = CHAIN_SWORD(pc,"(");
  pc = gen_nconc(pc, words_expression(a));
  pc = CHAIN_SWORD(pc,")[");
  MAP(EXPRESSION,exp,
  {
    if (!first)
      pc = CHAIN_SWORD(pc,",");
    pc = gen_nconc(pc, words_expression(exp));
    first = FALSE;
  },lexp);
  pc = CHAIN_SWORD(pc,"]");
  return pc;
}

static list words_application(application a)
{
  list pc = NIL;
  expression f = application_function(a);
  list lexp = application_arguments(a);
  bool first = TRUE;
  /* Parentheses must be added for function expression */
  pc = CHAIN_SWORD(pc,"(");
  pc = gen_nconc(pc, words_expression(f));
  pc = CHAIN_SWORD(pc,")(");
  MAP(EXPRESSION,exp,
  {
    if (!first)
      pc = CHAIN_SWORD(pc,",");
    pc = gen_nconc(pc, words_expression(exp));
    first = FALSE;
  },lexp);
  pc = CHAIN_SWORD(pc,")");
  return pc;
}

static text text_forloop(entity module,
			 string label,
			 int margin,
			 forloop obj,
			 int n)
{
    list pc = NIL;
    unformatted u;
    text r = make_text(NIL);
    statement body = forloop_body(obj) ;
    //instruction i = statement_instruction(body);

    pc = CHAIN_SWORD(pc,"for (");
    if (!expression_undefined_p(forloop_initialization(obj)))
      pc = gen_nconc(pc, words_expression(forloop_initialization(obj)));
    pc = CHAIN_SWORD(pc,";");
    if (!expression_undefined_p(forloop_condition(obj))) {
      /* To restitute for(;;) */
      expression cond = forloop_condition(obj);
      if(!expression_one_p(cond))
	pc = gen_nconc(pc, words_expression(forloop_condition(obj)));
    }
    pc = CHAIN_SWORD(pc,";");
    if (!expression_undefined_p(forloop_increment(obj)))
      pc = gen_nconc(pc, words_expression(forloop_increment(obj)));
    pc = CHAIN_SWORD(pc,one_liner_p(body)?")":") {");
    u = make_unformatted(strdup(label), n, margin, pc) ;
    ADD_SENTENCE_TO_TEXT(r, make_sentence(is_sentence_unformatted, u));

    if(one_liner_p(body)) {
      MERGE_TEXTS(r, text_statement_enclosed(module,
					     margin+INDENTATION,
					     body,
					     !one_liner_p(body),
					     !one_liner_p(body)));
    }
    else {
      // ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"{"));
      MERGE_TEXTS(r, text_statement(module, margin+INDENTATION, body));
      ADD_SENTENCE_TO_TEXT(r, MAKE_ONE_WORD_SENTENCE(margin,"}"));
    }

    return r;
}
