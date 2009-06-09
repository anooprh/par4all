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
 /* 
    Function for statement, and its subtypes:
     - instruction

    Lei ZHOU         12 Sep. 1991
    Francois IRIGOIN
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "linear.h"

#include "genC.h"
#include "misc.h"

#include "text.h"
#include "text-util.h"
#include "ri.h"

#include "ri-util.h"

#include "properties.h"


/******************************************************* EMPTY STATEMENT */
/* detects a statement with no special effect...
 */ 

/* Define the static buffer size */
#define STATIC_BUFFER_SZ 100


static bool statement_is_empty;

static bool cannot_be_empty(void)
{
    statement_is_empty = FALSE;
    gen_recurse_stop(NULL);
    return FALSE;
}

static bool call_filter(call c)
{
    entity e = call_function(c);
    if (ENTITY_CONTINUE_P(e) || ENTITY_RETURN_P(e))
	return FALSE;
    else
	return cannot_be_empty();
}

bool 
empty_code_p(statement s)
{
    if ((!s) || statement_undefined_p(s)) 
	return TRUE;

    statement_is_empty = TRUE;
    gen_multi_recurse(s,
		      call_domain, call_filter, gen_null,
		      NULL);

    pips_debug(3, "returning %d\n", statement_is_empty);

    return statement_is_empty;
}

bool 
empty_code_list_p(list l)
{
    MAP(STATEMENT, s, if (!empty_code_p(s)) return FALSE, l);
    return TRUE;
}

/*************************************************************** COUNT LOOPS */

static int nseq, npar;
static void loop_rwt(loop l)
{
    if (execution_parallel_p(loop_execution(l)))
	npar++;
    else
	nseq++;
}

void number_of_sequential_and_parallel_loops(
    statement stat,
    int * pseq,
    int * ppar)
{
    nseq=0, npar=0;
    gen_multi_recurse(stat, loop_domain, gen_true, loop_rwt, NULL);
    *pseq=nseq, *ppar=npar;
}

void print_number_of_loop_statistics(
    FILE * out,
    string msg,
    statement s)
{
    int seq, par;
    number_of_sequential_and_parallel_loops(s, &seq, &par);
    fprintf(out, "%s: %d seq loops, %d par loops\n", msg, seq, par);
}

/* print out the number of sequential versus parallel loops.
 */
void print_parallelization_statistics(
    string module, /* the module name */
    string msg,    /* an additional message */
    statement s    /* the module statement to consider */)
{
    if (get_bool_property("PARALLELIZATION_STATISTICS"))
    {
	fprintf(stderr, "%s %s parallelization statistics", module, msg);
	print_number_of_loop_statistics(stderr, "", s);
    }
}

/****************************************************************************/

bool
empty_comments_p(string s)
{
  /* Could be replaced by a macro. See macro empty_comments */
  pips_assert("comments cannot be NULL", s!=NULL);
  return (s == NULL || string_undefined_p(s));
}

/** @defgroup statements_p Predicates on statements

    @{
*/


/* Test if a statement is an assignment. */
bool
assignment_statement_p(statement s) {
  instruction i = statement_instruction(s);

  return instruction_assign_p(i);
}


/* Test if a statement is a "return" */
bool return_statement_p(statement s) {
  instruction i = statement_instruction(s);
  return return_instruction_p(i);
}


/* Test if a statement is a CONTINUE, that is the FORTRAN nop, the ";" in
   C or the "pass" in Python... according to the language.
*/
bool
continue_statement_p(statement s) {
  instruction i = statement_instruction(s);

  return instruction_continue_p(i);
}


/* Test if a statement is a Fortran STOP.
*/
bool
stop_statement_p(statement s) {
  instruction i = statement_instruction(s);

  return instruction_stop_p(i);
}


/* Test if a statement is a Fortran FORMAT.
*/
bool
format_statement_p(statement s) {
  instruction i = statement_instruction(s);

  return instruction_format_p(i);
}


bool
write_statement_p(s)
statement s;
{
    instruction i = statement_instruction(s);

    return (native_instruction_p(i, WRITE_FUNCTION_NAME));
}

bool 
statement_less_p(st1, st2)
statement st1, st2;
{
    int o1 = statement_ordering( st1 ) ;
    int o2 = statement_ordering( st2 ) ;

    if (ORDERING_NUMBER( o1 ) != ORDERING_NUMBER( o2 )) {
	fprintf(stderr, "cannot compare %td (%d,%d) and %td (%d,%d)\n",
		statement_number(st1), 
		ORDERING_NUMBER( o1 ), ORDERING_STATEMENT( o1 ),
		statement_number(st2), 
		ORDERING_NUMBER( o2 ), ORDERING_STATEMENT( o2 ));

	abort();
    }

    return( ORDERING_STATEMENT(o1) < ORDERING_STATEMENT(o2)) ;
}

bool 
statement_possible_less_p(st1, st2)
statement st1, st2;
{
    int o1 = statement_ordering( st1 ) ;
    int o2 = statement_ordering( st2 ) ;

    if (ORDERING_NUMBER( o1 ) != ORDERING_NUMBER( o2 )) {
	return(TRUE);
    }
    else
	return( ORDERING_STATEMENT(o1) < ORDERING_STATEMENT(o2));
}

/* Statement classes induced from instruction type 
 *
 * Naming conflict between English, block_statement_p(), and NewGen convention
 * isntruction_loop_p()
 */

/* See also macro statement_block_p() */
bool 
block_statement_p(statement s)
{
    instruction i = statement_instruction(s);
    bool r = instruction_sequence_p(i);

    return r;
}

bool 
statement_test_p(statement s)
{
    return(instruction_test_p(statement_instruction(s)));
}

bool 
statement_loop_p(s)
statement s;
{
    return(instruction_loop_p(statement_instruction(s)));
}

bool 
statement_whileloop_p(s)
statement s;
{
    return(instruction_whileloop_p(statement_instruction(s)));
}

bool 
statement_forloop_p(s)
statement s;
{
    return(instruction_forloop_p(statement_instruction(s)));
}

bool 
unstructured_statement_p(statement s)
{
    return(instruction_unstructured_p(statement_instruction(s)));
}

/* Test if a statement is empty. */
bool
empty_statement_p(st)
statement st;
{
    instruction i;

    return(entity_empty_label_p(statement_label(st)) &&
	   instruction_block_p(i=statement_instruction(st)) &&
	   ENDP(instruction_block(i)) &&
	   ENDP(statement_declarations(st)));
}


bool
unlabelled_statement_p(st)
statement st;
{
    return(entity_empty_label_p(statement_label(st)));
}

bool
nop_statement_p(statement s)
{
    /* NOP statements are useful to fill in empty test branches.
     * The definition of NOP in PIPS has changed over the years.
     * A NOP statement is an empty block. Like blocks, it cannot be
     * labelled nor commented. It has no statement number because it
     * is invisible to users.
     *
     * Dangling comments, like labels, are attached to CONTINUE statements.
     *
     * Note 1: blocks are now called "sequences"
     * Note 2: see also empty_statement_p()
     */
    bool nop = FALSE;
    instruction i = statement_instruction(s);

    if(instruction_block_p(i) && ENDP(instruction_block(i))) {
	pips_assert("No label!", entity_empty_label_p(statement_label(s)));
	pips_assert("No comments", empty_comments_p(statement_comments(s)));
	pips_assert("No statement number", statement_number(s) == STATEMENT_NUMBER_UNDEFINED);
	nop = TRUE;
    }

    return nop;
}

/* Return true if the statement is an empty instruction block without
   label or a continue without label or a recursive combination of
   above.

   FI: I add a check on declarations. With their initializations, they
   have side effects. See C_syntax/block01.c.
 */
bool
empty_statement_or_labelless_continue_p(statement st)
{
   instruction i;

   if (!entity_empty_label_p(statement_label(st)))
      return FALSE;
   if (continue_statement_p(st))
      return TRUE;
   i = statement_instruction(st);
   if (instruction_block_p(i) && ENDP(statement_declarations(st))) {
       MAP(STATEMENT, s,
           {
	       if (!empty_statement_or_labelless_continue_p(s))
		   /* Well there is at least one possibly usefull thing... */
		   return FALSE;
           },
	   instruction_block(i));
       return TRUE;
   }
   return FALSE;
}


/* Return true if the statement is an empty instruction block or a
   continue or a recursive combination of above. */
bool
empty_statement_or_continue_p(statement st)
{
   instruction i;

   if (continue_statement_p(st))
      return TRUE;
   i = statement_instruction(st);
   if (instruction_block_p(i)) {
       MAP(STATEMENT, s,
           {
	       if (!empty_statement_or_continue_p(s))
		   /* Well there is at least one possibly usefull thing... */
		   return FALSE;
           },
              instruction_block(i));
       return TRUE;
   }
   return FALSE;
}


/* Return true if the statement is an empty instruction block or a
   continue without comments or without LABEL or a recursive
   combination of above. */
bool
empty_statement_or_continue_without_comment_p(statement st)
{
   instruction i;
   string the_comments = statement_comments(st);

   /* The very last condition should be sufficient */
   if (!empty_comments_p(the_comments))
       return FALSE;

   if (!entity_empty_label_p(statement_label(st)))
      return FALSE;
   if (continue_statement_p(st))
      return TRUE;
   
   i = statement_instruction(st);
   if (instruction_block_p(i)) {
       MAP(STATEMENT, s,
	   {
	       if (!empty_statement_or_continue_without_comment_p(s))
		   return FALSE;
	   },
	   instruction_block(i));
       /* Everything in the block are commentless continue or empty
          statements: */
       return TRUE;
   }
   /* Everything else useful: */
   return FALSE;
}


bool 
statement_call_p(s)
statement s;
{
    return(instruction_call_p(statement_instruction(s)));
}


bool 
check_io_statement_p(statement s)
{
  bool check_io = FALSE;
  instruction i = statement_instruction(s);

  if(instruction_test_p(i)) {
    syntax c = expression_syntax(test_condition(instruction_test(i)));

    if(syntax_reference_p(c)) {
      check_io = io_entity_p(reference_variable(syntax_reference(c)));
    }
  }

  return check_io;
}

bool 
perfectly_nested_loop_p(stat)
statement stat;
{
    instruction ins = statement_instruction(stat);
    tag t = instruction_tag(ins);

    switch( t ) {
    case is_instruction_block: {
	list lb = instruction_block(ins);

	if ( lb != NIL && (lb->cdr) != NIL && (lb->cdr)->cdr == NIL 
	    && ( continue_statement_p(STATEMENT(CAR(lb->cdr))) ) ) {
	    if ( assignment_statement_p(STATEMENT(CAR(lb))) )
		return TRUE;
	    else
		return(perfectly_nested_loop_p(STATEMENT(CAR(lb))));
	}
	else if ( lb != NIL && (lb->cdr) == NIL )
	    return(perfectly_nested_loop_p(STATEMENT(CAR(lb))));
	else if ( lb != NIL ) {
	    /* biased for WP65 */
	    return assignment_block_p(ins);
	}
	else
	    /* extreme case: empty loop nest */
	    return TRUE;
	break;
    }
    case is_instruction_loop: {
	loop lo = instruction_loop(ins);
	statement sbody = loop_body(lo);
	    
	if ( assignment_statement_p(sbody) ) 
	    return TRUE;
	else
	    return(perfectly_nested_loop_p(sbody));
	break;
    }
    default:
	break;
    }

    return FALSE;
}

/** @} */


/* functions to generate statements */

statement make_empty_statement_with_declarations_and_comments(list d, string dt, string c)
{
    return(make_statement(entity_empty_label(),
			  STATEMENT_NUMBER_UNDEFINED,
			  STATEMENT_ORDERING_UNDEFINED,
			  c,
			  make_instruction_block(NIL),
			  d,
			  dt,
			  empty_extensions () ));
}

statement make_empty_statement()
{
    return(make_statement(entity_empty_label(),
			  STATEMENT_NUMBER_UNDEFINED,
			  STATEMENT_ORDERING_UNDEFINED,
			  empty_comments,
			  make_instruction_block(NIL),NIL,NULL,
			  empty_extensions ()));
}

/* to be compared with instruction_to_statement() which is a macro (thanks to FC?) ! */

statement
make_stmt_of_instr(instr)
instruction instr;
{
    return(make_statement(entity_empty_label(), 
			  STATEMENT_NUMBER_UNDEFINED,
			  STATEMENT_ORDERING_UNDEFINED, 
			  empty_comments,
			  instr,NIL,NULL,
			  empty_extensions ()));
}

instruction
make_assign_instruction(expression l,
                        expression r)
{
   call c = call_undefined;
   instruction i = instruction_undefined;

   pips_assert("make_assign_statement",
               syntax_reference_p(expression_syntax(l)));
   c = make_call(entity_intrinsic(ASSIGN_OPERATOR_NAME),
                 CONS(EXPRESSION, l, CONS(EXPRESSION, r, NIL)));
   i = make_instruction(is_instruction_call, c);

   return i;
}


statement
make_assign_statement(expression l,
                      expression r)
{
    return make_stmt_of_instr(make_assign_instruction(l, r));
}


statement
make_nop_statement()
{
    /* Attention: this function and the next one produce the same result,
     * but they are not semantically equivalent. An empty block may be
     * defined to be filled in, not to be used as a NOP statement.
     */
    statement s = make_empty_block_statement();

    return s;
}

statement 
make_empty_block_statement()
{
    statement b;

    b = make_block_statement(NIL);

    return b;
}

statement 
make_block_statement(body)
list body;
{
    statement b;

    b = make_statement(entity_empty_label(),
			  STATEMENT_NUMBER_UNDEFINED,
			  STATEMENT_ORDERING_UNDEFINED,
			  empty_comments,
			  make_instruction_block(body),NIL,NULL,
			  empty_extensions ());

    return b;
}


instruction make_instruction_block(list statements)
{
    return make_instruction_sequence(make_sequence(statements));
}

statement 
make_return_statement(module)
entity module;
{
    char *module_name = entity_local_name(module);
    string name = concatenate( module_name, MODULE_SEP_STRING, LABEL_PREFIX,
		       RETURN_LABEL_NAME,NULL);
    entity l = gen_find_tabulated(name, entity_domain);
    if (entity_undefined_p(l)) l = make_label(strdup(name));
    return make_call_statement(RETURN_FUNCTION_NAME, NIL, l, empty_comments);
}

/*----------------------------

  This function returns a stop statement with an error message

------------------------------*/

statement make_stop_statement(string message)
{
     list args=NIL; 
     expression e;
  
     e = make_call_expression(MakeConstant(message,is_basic_string),NIL);
       
     args = CONS(EXPRESSION,e,NIL);

     return make_call_statement(STOP_FUNCTION_NAME, args, entity_undefined, empty_comments);
   
}


/* adds a RETURN statement to *ps if necessary
 */
void
insure_return_as_last_statement(
    entity module,
    statement *ps)
{
    statement last = find_last_statement(*ps);
    if (statement_undefined_p(last) || !return_statement_p(last))
    {
	statement ret = make_return_statement(module);
	if (statement_block_p(*ps))
	{
	    sequence_statements(instruction_sequence(
		statement_instruction(*ps))) = 
		gen_nconc(sequence_statements(instruction_sequence(
		    statement_instruction(*ps))),
			  CONS(STATEMENT, ret, NIL));
	}
	else
	{
	    *ps = make_block_statement(CONS(STATEMENT, *ps,
				       CONS(STATEMENT, ret, NIL)));
	}
    }
}


statement
make_continue_statement(l)
entity l;
{
    return make_call_statement(CONTINUE_FUNCTION_NAME, NIL, l,
			       empty_comments);
}

/* Build a while loop statement.

   If \a before is true, build a
   while (\a condition)
     \a s;

   else build a
   do
     \a s;
   while (\a condition);

  @param line_number is used to specify a source-line number to the statement
*/
statement make_whileloop_statement(expression condition,
				   statement s,
				   int line_number,
				   bool before) {
  whileloop w;
  statement smt;

  w  = make_whileloop(condition, s, entity_empty_label(),
		      before ? make_evaluation_before() : make_evaluation_after());
  smt = make_statement(entity_empty_label(),
		       line_number,
		       STATEMENT_ORDERING_UNDEFINED,
		       //pop_current_C_comment(),
		       string_undefined,
		       make_instruction_whileloop(w),
		       NIL, string_undefined,
		       empty_extensions ());
  return smt;
}


statement
make_call_statement(function_name, args, l, c)
string function_name;
list args;
entity l; /* label, default entity_undefined */
string c; /* comments, default empty_comments (was: "" (was: string_undefined)) */
{
  entity called_function;
  statement cs;

  called_function = entity_intrinsic(function_name);

  l = (l==entity_undefined)? entity_empty_label() : l;
  cs = make_statement(l,
		      STATEMENT_NUMBER_UNDEFINED,
		      STATEMENT_ORDERING_UNDEFINED,
		      c,
		      make_instruction(is_instruction_call,
				       make_call(called_function,args)),
		      NIL,
		      NULL,
		      empty_extensions ());

  ifdebug(8) {
    pips_debug(8, "cs is call to %s\n", function_name);
    safe_print_statement(cs);
  }

  return cs;
}


/* Extract the body of a perfectly nested loop body.
 */
statement
perfectly_nested_loop_to_body(loop_nest)
statement loop_nest;
{
    instruction ins = statement_instruction(loop_nest);

    switch(instruction_tag(ins)) {

    case is_instruction_call:
    case is_instruction_whileloop:
    case is_instruction_test:
      /* By hypothesis we are in a perfectly nested loop and since it is
	 not a loop, we've reached the loop body: */
        return loop_nest;

    case is_instruction_block: {
	list lb = instruction_block(ins);
	if (lb == NIL)
	  /* The loop body is an empty block, such as { } in C: */
	  return loop_nest;
	statement first_s = STATEMENT(CAR(lb));
	instruction first_i = statement_instruction(first_s);

	if(instruction_call_p(first_i))
	    return loop_nest;
	else {
	    if(instruction_block_p(first_i))
		return perfectly_nested_loop_to_body(STATEMENT(CAR(instruction_block(first_i))));
	    else {
		pips_assert("perfectly_nested_loop_to_body",
			    instruction_loop_p(first_i));
		return perfectly_nested_loop_to_body( first_s);
	    }
	}
	break;
    }
    case is_instruction_loop: {
      /* It is another loop: dig into it to reach the loop body: */
	statement sbody = loop_body(instruction_loop(ins));
	return (perfectly_nested_loop_to_body(sbody));
	break;
    }
    default:
	pips_error("perfectly_nested_loop_to_body","illegal tag\n");
	break;
    }
    return(statement_undefined); /* just to avoid a warning */
}

/* Direct accesses to second level fields */

loop
statement_loop(s)
statement s;
{
    pips_assert("statement_loop", statement_loop_p(s));

    return(instruction_loop(statement_instruction(s)));
}

test
statement_test(s)
statement s;
{
    pips_assert("statement_test", statement_test_p(s));

    return(instruction_test(statement_instruction(s)));
}

call 
statement_call(s)
statement s;
{
    pips_assert("statement_call", statement_call_p(s));

    return(instruction_call(statement_instruction(s)));
}

list 
statement_block(s)
statement s;
{
    pips_assert("statement_block", statement_block_p(s));

    return(instruction_block(statement_instruction(s)));
}


/*
  returns the numerical value of loop l increment expression.
  aborts if this expression is not an integral constant expression.
  modification : returns the zero value when it isn't constant
  Y.Q. 19/05/92
*/

int 
loop_increment_value(l)
loop l;
{
    range r = loop_range(l);
    expression ic = range_increment(r);
    normalized ni;
    int inc;

    ni = NORMALIZE_EXPRESSION(ic);

    if (! EvalNormalized(ni, &inc)){
	/*user_error("loop_increment_value", "increment is not constant");*/
	debug(8,"loop_increment_value", "increment is not constant");
	return(0);
    }
    return(inc);
}

bool 
assignment_block_or_statement_p(s)
statement s;
{
    instruction i = statement_instruction(s);

    switch(instruction_tag(i)) {
    case is_instruction_block:
	return assignment_block_p(statement_instruction(s));
	break;
    case is_instruction_test:
	break;
    case is_instruction_loop:
	break;
    case is_instruction_whileloop:
	break;
    case is_instruction_goto:
	pips_error("assignment_block_p", "unexpected GO TO\n");
    case is_instruction_call:
	return assignment_statement_p(s);
    case is_instruction_unstructured:
	break;
    default: pips_error("assignment_block_or_statement_p",
			"ill. instruction tag %d\n", instruction_tag(i));
    }
    return FALSE;
}

void 
print_statement_set(fd, r)
FILE *fd;
set r;
{
    fprintf(fd, "Set contains statements");

    SET_MAP(s, {
	fprintf(fd, " %02td", statement_number((statement) s));
    }, r);

    fprintf(fd, "\n");
}

/* (the text is not freed, massive memory leak:-) 
 *
 * See text_named_module() for improvements.
 */ 
void print_statement(statement s)
{
  debug_on("TEXT_DEBUG_LEVEL");
  set_alternate_return_set();
  reset_label_counter();
  push_current_module_statement(s);
  print_text(stderr, text_statement(entity_undefined, 0, s));
  pop_current_module_statement();
  reset_alternate_return_set();
  debug_off();
}

void print_statement_of_module(statement s, string mn)
{
  if(entity_undefined_p(get_current_module_entity())) {
    entity m = local_name_to_top_level_entity(mn);
    set_current_module_entity(m);
    reset_label_counter();  
    print_statement(s);
    reset_current_module_entity();
  }
  else
    print_statement(s);
}

text statement_to_text(statement s)
{
  text t = text_undefined;
  
  debug_on("PRETTYPRINT_DEBUG_LEVEL");
  set_alternate_return_set();
  reset_label_counter();
  t = text_statement(entity_undefined, 0, s);
  reset_alternate_return_set();
  debug_off();

  return t;
}

void safe_print_statement(statement s)
{
  if(statement_undefined_p(s)) {
    fprintf(stderr, "Statement undefined: %s\n",
	    statement_identification(s));
  }
  else if(continue_statement_p(s)
     && entity_return_label_p(statement_label(s))) {
    /* The return label only can be associated to a RETURN call,
       however the controlizer does not follow this consistency
       rule. */
    fprintf(stderr, "%s\n", statement_identification(s));
  }
  else
    print_statement(s);
}

void print_parallel_statement(statement s)
{
  string cstyle = strdup(get_string_property(PRETTYPRINT_PARALLEL));
  set_string_property(PRETTYPRINT_PARALLEL, "doall");
  print_statement(s);
  set_string_property(PRETTYPRINT_PARALLEL, cstyle);
  free(cstyle);
}


/* Mapping from statement number to statement 
 *
 * Warning: STATEMENT_NUMBER_UNDEFINED may be equal to HASH_ENTRY_FREE_FOR_PUT
 * because integer keys are not well supported by the newgen hash package.
 *
 * Two statements in the same piece of code may have the same number because:
 *  - they were generated by the parser to simulate a unique user statement
 *    (e.g. a computed GO TO)
 *  - they were generated by a program transformation from the same user
 *    statement (e.g. DO statements by a loop distribution)
 *
 * A statement may be not numbered because:
 *  - the parser assumed that it did not matter (e.g. a GO TO transformed into
 *    an arc by the controlizer); but the parser is not really consistent...
 *  - a program generation phase has no way to generate a realistic statement
 *    number from a user statement number (e.g. code for banks in WP65)
 */

/* static variable temporarily required to use gen_recurse() */
static hash_table number_to_statement = hash_table_undefined;
/* To keep track of duplicate numbers */
static set duplicate_numbers = set_undefined;

static void 
update_number_to_statement(s)
statement s;
{
    if(statement_number(s)!=STATEMENT_NUMBER_UNDEFINED) {
	if(hash_get(number_to_statement, (char *) statement_number(s))
	   != HASH_UNDEFINED_VALUE) {
	    duplicate_numbers = set_add_element(duplicate_numbers, duplicate_numbers, 
						(char *) statement_number(s));
	}
	else {
	    hash_put(number_to_statement, (char *) statement_number(s), (char *) s);
	}
    }
}

statement
apply_number_to_statement(hash_table nts, _int n)
{
    /* This function used to be inline in prettyprinting functions for user views.
     * It was assumed that all statements produced by the parser had a defined
     * statement number. In order to keep a nice statement numbering scheme,
     * GO TO statements are not (always) numbered. So n hasa to be tested.
     */

    statement s = statement_undefined;

    if(n!=STATEMENT_NUMBER_UNDEFINED) {
	s = (statement) hash_get(nts, (char *) n);
	if (s == (statement) HASH_UNDEFINED_VALUE) {
	    s = statement_undefined;
	}
    }

    return s;
}

hash_table 
build_number_to_statement(nts, s)
hash_table nts;
statement s;
{
    pips_assert("build_number_to_statement", nts!=hash_table_undefined
		&& !statement_undefined_p(s) && set_undefined_p(duplicate_numbers));

    number_to_statement = nts;
    duplicate_numbers = set_make(set_int);

    gen_recurse(s, statement_domain, gen_true, update_number_to_statement);

    /* Eliminate duplicate and hence meaningless numbers */
    SET_MAP(n, {
	hash_del(number_to_statement, n);
    }, duplicate_numbers);

    /* nts is updated by side effect on number_to_statement */
    number_to_statement = hash_table_undefined;
    set_free(duplicate_numbers);
    duplicate_numbers = set_undefined;
    return nts;
}

void 
print_number_to_statement(nts)
hash_table nts;
{
    HASH_MAP(number, stmt, {
	fprintf(stderr,"%td\t", (_int) number);
	print_statement((statement) stmt);
    }, nts);
}

hash_table 
allocate_number_to_statement()
{
    hash_table nts = hash_table_undefined;

    /* let's assume that 50 statements is a good approximation of a module
     * size 
     */
    nts = hash_table_make(hash_int, 50);

    return nts;
}

/* get rid of all labels in controlized code before duplication: all
 * labels have become useless and they cannot be freely duplicated.
 *
 * One caveat: FORMAT statements!
 */
statement 
clear_labels(s)
statement s;
{
    gen_multi_recurse(s, statement_domain, gen_true, clear_label, NULL);
    return s;
}

void 
clear_label(s)
statement s;
{
    if(format_statement_p(s)) {
	user_error("clear_label", "Cannot clear label for FORMAT %s!\n",
		   label_local_name(statement_label(s)));
    }

    statement_label(s) = entity_empty_label();

    if(instruction_loop_p(statement_instruction(s))) 
	loop_label(instruction_loop(statement_instruction(s))) = 
	    entity_empty_label();
}

/* Build a statement sequence from a statement list if there are more than
 * one statement or return the statement itself after discarding the list.
 */
statement
list_to_statement(l)
list l;
{
    switch (gen_length(l))
    {
    case 0:
	return(statement_undefined);
    case 1:
    {
	statement stat=STATEMENT(CAR(l));
	gen_free_list(l);
	return(stat);
    }
    default:
	return(make_block_statement(l));
    }

    return(statement_undefined);
}

statement 
st_make_nice_test(condition, ltrue, lfalse)
expression condition;
list ltrue,lfalse;
{
    statement
	stattrue = list_to_statement(ltrue),
	statfalse = list_to_statement(lfalse);
    bool
	notrue=(stattrue==statement_undefined),
	nofalse=(statfalse==statement_undefined);
    
    if ((notrue) && (nofalse)) 
	return(make_continue_statement(entity_undefined)); 

    if (nofalse) 
    {
	return
	    (make_stmt_of_instr
	     (make_instruction
	      (is_instruction_test,
	       make_test(condition,
			 stattrue,
			 make_continue_statement(entity_undefined)))));
    }

    if (notrue) 
    {
	expression
	    newcond = MakeUnaryCall(entity_intrinsic(NOT_OPERATOR_NAME),
				    condition);

	return
	    (make_stmt_of_instr
	     (make_instruction
	      (is_instruction_test,
	       make_test(newcond,
			 statfalse,
			 make_continue_statement(entity_undefined)))));
    }

    return(make_stmt_of_instr(make_instruction(is_instruction_test,
					       make_test(condition,
							 stattrue,
							 statfalse))));
}


/* statement makeloopbody(l, s_old) 
 * make a statement for a loop body, using the fields of a previously existing statement
 *
 * Preserving the labels may be sometimes a good thing (hyperplane or
 * tiling transformation, outermostloop) or a bad thing for innermost
 * loops, sometimes replicated loops
 *
 * FI: the name of this function is not well chosen.
 */
statement makeloopbody(loop l, statement s_old, bool inner_p)
{
    statement state_l;
    instruction instr_l;
    statement l_body;

    instr_l = make_instruction(is_instruction_loop,l);
    state_l = make_statement(inner_p? entity_empty_label() : statement_label(s_old),
			     statement_number(s_old),
			     statement_ordering(s_old),
			     statement_comments(s_old),
			     instr_l,NIL,NULL,
			     statement_extensions(s_old));
    l_body = make_statement(entity_empty_label(),
			    STATEMENT_NUMBER_UNDEFINED,
			    STATEMENT_ORDERING_UNDEFINED,
			    empty_comments,
			    make_instruction_block(CONS(STATEMENT,state_l,NIL)),
			    NIL,NULL,
			    empty_extensions ());

    return(l_body);
}


/* statement make_block_with_stmt(statement stmt): makes sure that the given
 * statement "stmt" is a block of instructions. If it is not the case, this
 * function returns a new statement with a block of one statement. This
 * statement is "stmt".
 * If "stmt" is already a block, it is returned unmodified.
 */
statement 
make_block_with_stmt(stmt)
statement stmt;
{
    if (instruction_tag(statement_instruction(stmt)) != is_instruction_block)
    {
	/* We create a new statement with an empty block of instructions.
	 * "make_empty_statement()" is defined in Lib/ri-util, it creates a statement
	 * with a NIL instruction block. */
	statement block_stmt = make_empty_statement();

	/* Then, we put the statement "stmt" in the block. */
	instruction_block(statement_instruction(block_stmt)) = CONS(STATEMENT,
								    stmt, NIL);
	return (block_stmt);
    }
    return (stmt);
}

/* Does work neither with undefined statements nor with defined
   statements with undefined instructions. Returns a statement
   identity, its number and the breakdown of its ordering, as well as
   information about the instruction. */

string 
external_statement_identification(statement s)
{
    static char buffer[STATIC_BUFFER_SZ];
    instruction i = statement_instruction(s);
    string instrstring = instruction_identification(i);
    int so = statement_ordering(s);
    entity called = entity_undefined;
    int nb_char = 0;

    if(same_string_p(instrstring, "CALL")) {
	called = call_function(instruction_call(i));
    }

    nb_char = snprintf(buffer, STATIC_BUFFER_SZ, "%td (%d, %d): %s %s\n",
	    statement_number(s),
	    ORDERING_NUMBER(so),
	    ORDERING_STATEMENT(so),
	    instrstring,
	    entity_undefined_p(called)? "" : module_local_name(called));

    pips_assert ("checking static buffer overflow", nb_char < STATIC_BUFFER_SZ);

    return buffer;
}

/* Like external_statement_identification(), but with internal
   information, the hexadecimal address of the statement */
string 
statement_identification(statement s)
{
    static char buffer[STATIC_BUFFER_SZ];
    instruction i = statement_instruction(s);
    string instrstring = instruction_identification(i);
    int so = statement_ordering(s);
    entity called = entity_undefined;
    int nb_char = 0;

    if(same_string_p(instrstring, "CALL")) {
	called = call_function(instruction_call(i));
    }

    nb_char = snprintf(buffer, STATIC_BUFFER_SZ, "%td (%d, %d) at %p: %s %s\n",
		 statement_number(s),
		 ORDERING_NUMBER(so),
		 ORDERING_STATEMENT(so),
		 s,
		 instrstring,
		 entity_undefined_p(called)? "" : module_local_name(called));

    pips_assert ("static buffer overflow, increase the buffer size", nb_char < STATIC_BUFFER_SZ);

    return buffer;
}

string 
safe_statement_identification(statement s)
{
  if(statement_undefined_p(s)) {
    return "undefined statement\n";
  }
  else if(instruction_undefined_p(statement_instruction(s))) {
    return "statement with undefined intruction\n";
  }
  else
    return statement_identification(s);
}



/* Return true if the statement has an empty statement: */
bool
statement_with_empty_comment_p(statement s)
{
    string the_comments = statement_comments(s);
    return empty_comments_p(the_comments);
}


static string gather_all_comments_of_a_statement_string;


static bool 
gather_all_comments_of_a_statement_filter(statement s)
{
  string the_comments = statement_comments(s);
  if (!empty_comments_p(the_comments)) {
    string old = gather_all_comments_of_a_statement_string;
    if (old == NULL)
	/* For now, no comment has been gathered: */
	old = strdup("");
    gather_all_comments_of_a_statement_string =
      strdup(concatenate(old, the_comments, NULL));
    free(old);
  }
  return TRUE;
}


/* Gather all the comments recursively found in the given statement
   and return them in a strduped string (NULL if no comment found).

   Do not forget to free the string returned later when no longer
   used. */
string
gather_all_comments_of_a_statement(statement s)
{
    gather_all_comments_of_a_statement_string = NULL;
    gen_multi_recurse(s, statement_domain,
		gather_all_comments_of_a_statement_filter, gen_null,
		      NULL);
    
    if (gather_all_comments_of_a_statement_string == NULL)
	return empty_comments;
    else
	return gather_all_comments_of_a_statement_string;
}


/* Find the first statement of a statement, if any: */
string
find_first_statement_comment(statement s)
{
    instruction i = statement_instruction(s);
    if (instruction_sequence_p(i)) {
	MAP(STATEMENT, st, {
	    string comment = find_first_statement_comment(st);
	    if (!empty_comments_p(comment))
		/* We've found it! */
		return comment;
	}, sequence_statements(instruction_sequence(statement_instruction(s))));
	/* No comment found: */
	return empty_comments;
    }
    else
	/* Ok, plain statement: */
	return statement_comments(s);
}


/* Put a comment on a statement in a safe way. That is it find the
   first non-block statement to attach it or insert a CONTINUE and put
   the statement on it. You should free the old one...

   Return TRUE on success, FALSE else. */
bool
try_to_put_a_comment_on_a_statement(statement s,
				    string the_comments)
{
    instruction i = statement_instruction(s);
    if (instruction_sequence_p(i)) {
	/* We are not allowed to put a comment on a sequence, find the
           first non sequence statement if any: */
	MAP(STATEMENT, st,
	    {
		if (try_to_put_a_comment_on_a_statement(st, the_comments))
		    /* Ok, we succeed to put the comment: */
		    return TRUE;
	    },
	    sequence_statements(instruction_sequence(i)));
	/* Hmm, no good statement found to attach a comment: */
	return FALSE;
    }
    else {
	/* Ok, it is a plain statement, we can put a comment on it: */
	statement_comments(s) = the_comments;
	return TRUE;
    }
}


/* Similar to try_to_put_a_comment_on_a_statement() but insert a
   CONTINUE to put the comment on it if there is only empty
   sequence(s): */
void
put_a_comment_on_a_statement(statement s,
			     string the_comments)
{
    if (empty_comments_p(the_comments))
	/* Nothing to add... */
	return;

    if (!try_to_put_a_comment_on_a_statement(s, the_comments)) {
	/* It failed because it is an empty sequence. So add a
           CONTINUE to attach the statement on it: */
	sequence seq = instruction_sequence(statement_instruction(s));
	statement s_cont = make_continue_statement(entity_empty_label());
	statement_comments(s_cont) = the_comments;
	sequence_statements(seq) = CONS(STATEMENT,
					s_cont,
					sequence_statements(seq));
    }
}


/* Append a comment string (if non empty) to the comments of a
   statement, if the c.

   @param the_comments is strdup'ed in this function.
*/
void
append_comments_to_statement(statement s,
			     string the_comments)
{
    string old;

    if (empty_comments_p(the_comments))
	/* Nothing to add... */
	return;

    old = find_first_statement_comment(s);
    if (empty_comments_p(old))
	/* There is no comment yet: */
	put_a_comment_on_a_statement(s, strdup(the_comments));
    else {
	put_a_comment_on_a_statement(s, strdup(concatenate(old, the_comments, NULL)));
	free(old);
    }
}


/* Insert a comment string (if non empty) at the beginning of the
   comments of a statement.

   @param the_comments is strdup'ed in this function.
*/
void
insert_comments_to_statement(statement s,
			     string the_comments)
{
    string old;

    if (empty_comments_p(the_comments))
	/* Nothing to add... */
	return;

    old  = find_first_statement_comment(s);
    if (empty_comments_p(old))
	/* There are no comments yet: */
	put_a_comment_on_a_statement(s, strdup(the_comments));
    else {
	put_a_comment_on_a_statement(s, strdup(concatenate(the_comments, old, NULL)));
	free(old);
    }
}



/* Since blocks are not represented in Fortran, they cannot
 * carry a label. A continue could be added by the prettyprinter
 * but it was decided not to support this facility.
 * For debugging, it's better to have a transparent prettyprinter
 * and no ghost statement with no ordering and no number.
 *
 * Insert a CONTINUE and move the label, comment and statement number
 * if any from the statement to the CONTINUE, if label there is.
 */
void
fix_sequence_statement_attributes(statement s) 
{
    pips_assert("Should be an instruction block...",
		instruction_block_p(statement_instruction(s)));

    if (unlabelled_statement_p(s) && empty_comments_p(statement_comments(s))) {
	/* The statement block has no label and no comment: just do
           nothing. */
	statement_number(s) = STATEMENT_NUMBER_UNDEFINED;
    }
    else {
	/* There are some informations we need to keep: just add a
	   CONTINUE to keep them: */
	list instructions;
	statement continue_s;
	string label_name = 
	    entity_local_name(statement_label(s)) + strlen(LABEL_PREFIX);
   
	instructions = instruction_block(statement_instruction(s));

	if (strcmp(label_name, RETURN_LABEL_NAME) == 0)
	    /* This the label of a RETURN, do not forward it: */
	    continue_s = make_continue_statement(entity_empty_label()); 
	else
	    continue_s = make_continue_statement(statement_label(s));
	
	statement_label(s) = entity_empty_label();
	statement_comments(continue_s) = statement_comments(s);
	statement_comments(s) = empty_comments;
	statement_number(continue_s) = statement_number(s);
	statement_number(s) = STATEMENT_NUMBER_UNDEFINED;

	instructions = CONS(STATEMENT, continue_s, instructions);
	instruction_block(statement_instruction(s)) = instructions;
    }
}


/* Apply fix_sequence_statement_attributes() on the statement only if
   it really a sequence. */
void
fix_sequence_statement_attributes_if_sequence(statement s)
{
    instruction i = statement_instruction(s);
    if (!instruction_undefined_p(i)) {
	if (instruction_sequence_p(i))
	    fix_sequence_statement_attributes(s);
    }
}


/* See if statement s is labelled and can be reached by a GO TO */
entity
statement_to_label(statement s)
{
  entity l = statement_label(s);

  if(format_statement_p(s)) {
    /* You cannot jump onto a non-executable statement */
    l = entity_empty_label();
  }
  else if(entity_empty_label_p(l)) {
    instruction i = statement_instruction(s);

    switch(instruction_tag(i)) {

    case is_instruction_sequence:
      /* The initial statement may be an unlabelled CONTINUE... */
      MAPL(cs, {
	statement stmt = STATEMENT(CAR(cs));
	l = statement_to_label(stmt);
	if(!entity_empty_label_p(l)||!continue_statement_p(stmt))
	  break;
      }, sequence_statements(instruction_sequence(i)));
      break;
    case is_instruction_unstructured:
      l = statement_to_label(control_statement(unstructured_control(instruction_unstructured(i))));
      break;
    case is_instruction_call:
    case is_instruction_test:
    case is_instruction_loop:
    case is_instruction_whileloop:
    case is_instruction_goto:
    case is_instruction_forloop:
    case is_instruction_expression:
      break;
    default:
      pips_error("statement_to_label", "Ill. tag %d for instruction",
		  instruction_tag(i));
    }
  }

  debug(8, "statement_to_label", "stmt %s, pointed by = %s\n",
	statement_identification(s), entity_local_name(l));

  return l;
}


/* Add a label to a statement. If the statement cannot accept a label (it
   is a sequence or it has already a label...), add a CONTINUE in front of
   the statement to carry it.

   @return \param s with the new \param label if it was possible to add it
   directly, or a new statement sequence with the new \param label added
   to a CONTINUE followed by the old statement \param s.

   \param label is the label to add to the statement

   \param s is the statement to be labelled

   \param labeled_statement is a pointer to a statement. It is initialized
   by the function to the statement that really get the label on. It is
   useful when we need to track it, for exemple for "goto" generation in
   the parser.

   The caller is responsible to *avoid* garbage collecting on \param s if a
   new statements are allocated in this fonction since it is included in
   the return statement.
 */
statement
add_label_to_statement(entity label,
		       statement s,
		       statement *labeled_statement) {
  /* Test with statement_to_label to deal with a label inside an
     instruction block: */
  entity old_label = statement_to_label(s);
  if (old_label == entity_undefined
      || (!instruction_sequence_p(statement_instruction(s))
	  && unlabelled_statement_p(s))) {
    statement_label(s) = label;
    *labeled_statement = s;
    return s;
  }
  else {
    /* Add a continue as a label landing pad since a statement can not
       have more than 1 label and a sequence cannot have a label: */
    statement c = make_continue_statement(label);
    *labeled_statement = c;
    list stmts = gen_statement_cons(s, NIL);
    stmts = gen_statement_cons(c, stmts);
    statement seq = make_block_statement(stmts);
    return seq;
  }
}


/* Returns FALSE is no syntactic control path exits s (i.e. even if TRUE is returned
 * there might be no control path). Subroutines and
 * functions are assumed to always return to keep the analysis intraprocedural.
 * See the continuation library for more advanced precondition-based analyses.
 *
 * TRUE is a safe default value.
 *
 * The function name is misleading: a RETURN statement does not return...
 * It should be called "statement_does_continue()"
 */
bool
statement_does_return(statement s)
{
    bool returns = TRUE;
    instruction i = statement_instruction(s);
    test t = test_undefined;

    switch(instruction_tag(i)) {
    case is_instruction_sequence:
	MAPL(sts,
	     {
		 statement st = STATEMENT(CAR(sts)) ;
		 if (!statement_does_return(st)) {
		     returns = FALSE;
		     break;
		 }
	     },
		 sequence_statements(instruction_sequence(i)));

	break;    
    case is_instruction_unstructured:
	returns = unstructured_does_return(instruction_unstructured(i));
	break;
    case is_instruction_call:
	/* the third condition is due to a bug/feature of unspaghettify */
	returns = !stop_statement_p(s) && !return_statement_p(s) &&
	    !(continue_statement_p(s) && entity_return_label_p(statement_label(s)));
	break;
    case is_instruction_test:
	t = instruction_test(i);
	returns = statement_does_return(test_true(t)) ||
	    statement_does_return(test_false(t));
	break;
    case is_instruction_loop:
    case is_instruction_whileloop:
	/* No precise answer, unless you can prove the loop executes at
	 * least one iteration.
	 */
	returns = TRUE;
	break;
    case is_instruction_goto:
	/* returns = statement_does_return(instruction_goto(i)); */
	returns = FALSE;
	break;
    case is_instruction_forloop:
      break;
    case is_instruction_expression:
      break;
    default:
	pips_error("statement_does_return", "Ill. tag %d for instruction",
		   instruction_tag(i));
    }

    debug(8, "statement_does_return", "stmt %s, does return= %d\n",
	  statement_identification(s), returns);

    return returns;
}

bool
unstructured_does_return(unstructured u)
{
  bool returns = FALSE;
  control entry = unstructured_control(u);
  control exit = unstructured_exit(u);
  list nodes = NIL;

  FORWARD_CONTROL_MAP(c, {
      returns = returns || (c==exit);
  }, entry, nodes);
  gen_free_list(nodes);  

  return returns;
}


/* Insert some statements before or after a given statement.

   It should not be used with parsed code containing "goto", i.e. before being
   controllized, because "goto" points to a given statement and here we
   change the statement that could be pointed to...
*/
void insert_or_append_a_statement_list(statement target,
				       list s_list,
				       bool after_p)
{
  if (statement_block_p(target)) {
    sequence seq = instruction_sequence(statement_instruction(target));
    if (after_p) // append
	sequence_statements(seq) = gen_nconc(sequence_statements(seq), s_list);
    else // insert	
	sequence_statements(seq) = gen_nconc(s_list, sequence_statements(seq));
  }
  else {
    statement new_statement = make_stmt_of_instr(statement_instruction(target));
    /* Create the new statement sequence with s_list first: */
    if (after_p) // append
      statement_instruction(target) =
	make_instruction_block(CONS(STATEMENT,
				    new_statement,
				    s_list));
    else // insert
      statement_instruction(target) =
	make_instruction_block(gen_nconc(s_list, CONS(STATEMENT,
						      new_statement,
						      NIL)));
    statement_label(new_statement) = statement_label(target);
    statement_label(target) = entity_empty_label();
    statement_number(new_statement) = statement_number(target);
    statement_number(target) = STATEMENT_NUMBER_UNDEFINED;
    statement_ordering(target) = STATEMENT_ORDERING_UNDEFINED;
    statement_comments(new_statement) = statement_comments(target);
    statement_comments(target) = empty_comments;
    statement_extensions(new_statement) = statement_extensions(target);
    statement_extensions(target) = empty_extensions ();
  }
}

void insert_a_statement_list_in_a_statement(statement target,
					    list s_list)
{
  insert_or_append_a_statement_list(target, s_list, FALSE);
}

void append_a_statement_list_to_a_statement(statement target,
					    list s_list)
{
  insert_or_append_a_statement_list(target, s_list, TRUE);
}

/* Insert one single statement before or after a target statement.

   It should not be used with parsed code containing "goto", i.e. before being
   controllized, because "goto" points to a given statement and here we
   change the statement that could be pointed to...
*/
void insert_or_append_a_statement(statement target,
				  statement new,
				  bool after_p)
{
  list s_list = CONS(STATEMENT, new, NIL);
  insert_or_append_a_statement_list(target, s_list, after_p);
}

void insert_a_statement(statement target,
			statement new)
{
  insert_or_append_a_statement(target, new, FALSE);
}

void append_a_statement(statement target,
			statement new)
{
  insert_or_append_a_statement(target, new, TRUE);
}


static list gather_and_remove_all_format_statements_list;


void
gather_and_remove_all_format_statements_rewrite(statement s)
{
    instruction i = statement_instruction(s);
    if (instruction_format_p(i)) {
	/* Put the instruction with the statement attributes in
           new_format. */
	statement new_format = make_stmt_of_instr(i);
	statement_label(new_format) = statement_label(s);
	statement_number(new_format) = statement_number(s);
	statement_comments(new_format) = statement_comments(s);
	statement_extensions(new_format) = statement_extensions(s);
	/* Replace the old FORMAT with a NOP: */
	statement_instruction(s) = make_instruction_block(NIL);
	statement_label(s) = entity_empty_label();
	statement_number(s) = STATEMENT_NUMBER_UNDEFINED;
	statement_ordering(s) = STATEMENT_ORDERING_UNDEFINED;
	statement_comments(s) = empty_comments;
	statement_extensions(s) = empty_extensions();

	gather_and_remove_all_format_statements_list = CONS(STATEMENT,
							    new_format,
							    gather_and_remove_all_format_statements_list);
    }
}


/* Used to keep aside the FORMAT before many code transformation that
   could remove them either. It just return a list of all the FORMAT
   statement and replace them with NOP. */
list
gather_and_remove_all_format_statements(statement s)
{
    gather_and_remove_all_format_statements_list = NIL;
    
    gen_multi_recurse(s, statement_domain,
		gen_true,
		gather_and_remove_all_format_statements_rewrite,
		      NULL);
    
    gather_and_remove_all_format_statements_list =
	gen_nreverse(gather_and_remove_all_format_statements_list);
    
    return gather_and_remove_all_format_statements_list;
}


/* Transfer all the FORMATs at the very beginning of a module: */
void
put_formats_at_module_beginning(statement s)
{
    /* Pick up all the FORMATs of the module: */
    list formats = gather_and_remove_all_format_statements(s);
    ifdebug (1)
	pips_assert("Incorrect statements...", statement_consistent_p(s));
    /* And put them at the very beginning of the module: */
    insert_a_statement_list_in_a_statement(s, formats);
    ifdebug (1)
	pips_assert("Incorrect statements...", statement_consistent_p(s));
}


/* Transfer all the FORMATs at the very end of a module: */
void
put_formats_at_module_end(statement s)
{
    /* Pick up all the FORMATs of the module: */
    list formats = gather_and_remove_all_format_statements(s);
    ifdebug (1)
	pips_assert("Incorrect statements...", statement_consistent_p(s));
    /* And put them at the very beginning of the module: */
    append_a_statement_list_to_a_statement(s, formats);
    ifdebug (1)
	pips_assert("Incorrect statements...", statement_consistent_p(s));
}


static bool format_inside_statement_has_been_found;


bool
figure_out_if_it_is_a_format(instruction i)
{
    if (instruction_format_p(i)) {
	format_inside_statement_has_been_found = TRUE;
	/* Useless to go on further: */
	gen_recurse_stop(NULL);
	return FALSE;
    }
    return TRUE;
}


/* Return TRUE only if there is a FORMAT inside the statement: */
bool
format_inside_statement_p(statement s)
{
    format_inside_statement_has_been_found = FALSE;
    
    gen_multi_recurse(s, instruction_domain,
		figure_out_if_it_is_a_format,
		gen_null, NULL);

    return format_inside_statement_has_been_found;
}


/* Number of comment line *directly* attached to a statement.
 * Should be zero for sequences.
 */
int
statement_to_comment_length(statement stmt)
{
    int length = 0;
    string cmt = statement_comments(stmt);

    if(string_undefined_p(cmt)) {
	/* Is it allowed? Should be the empty string... */
	length = 0;
    }
    else {
	char c;
	while((c=*cmt++)!= '\0')
	    if(c=='\n')
		length++;
    }

    return length;
}


/* Poor attempt at associating physical line numbers to statement.
 * 
 * Lines used for declarations are counted elsewhere: see
 * module_to_declaration_length()
 *
 * It is assumed that each statement fits on one physical line,
 * excepts sequences which require 0 line. Non statement such as
 * ENDIF, ELSE, ENDDO,... must be taken care of too.
 */

static int current_line = -1;
static persistant_statement_to_int stmt_to_line = persistant_statement_to_int_undefined;

static bool
down_counter(statement s)
{
    instruction i = statement_instruction(s);

    current_line += statement_to_comment_length(s);

    /* Is it printable on one line? */
    if(instruction_sequence_p(i)) {
	current_line += 0;
    }
    else if(empty_statement_or_labelless_continue_p(s)) {
	/* Must be an unlabelled CONTINUE */
	current_line += 0;
    }
    else {
	current_line += 1;
    }

    extend_persistant_statement_to_int(stmt_to_line, s, current_line);

    return TRUE;
}

static void
up_counter(statement s)
{
    instruction i = statement_instruction(s);

    if(instruction_loop_p(i)) {
	loop l = instruction_loop(i);

	if(entity_empty_label_p(loop_label(l))) {
	    /* There must be an ENDDO here */
	    current_line += 1;
	}
    }
    else if(instruction_test_p(i) 
	    && statement_number(s) != STATEMENT_NUMBER_UNDEFINED) {
	/* There must be an ENDIF here */
	/* I doubt it's a real good way to detect synthetic IFs */
	    current_line += 1;
    }
    /* What could be done for ELSE and ELSEIF? */
    /* An else stack should be built. Use
     * DEFINE_LOCAL_STACK(name, type)
     * DEFINE_LOCAL_STACK(else_stack, statement)
     * Note that it should be pushed and poped in the down_counter() function
     */
}

persistant_statement_to_int
statement_to_line_number(statement s)
{
    persistant_statement_to_int s_to_l = persistant_statement_to_int_undefined;

    stmt_to_line = make_persistant_statement_to_int();

    current_line = 0;

    gen_multi_recurse(s, statement_domain, down_counter, up_counter, NULL);

    s_to_l = stmt_to_line;
    stmt_to_line = persistant_statement_to_int_undefined;
    return s_to_l;
}

/* insert statement s1 before or after statement s
 *  
 * If statement s is a sequence, simply insert s1 at the begining
 * or at the end of the sequence s.
 *
 * If not, create a new statement s2 with s's fields and update
 * s as a sequence with no comments and undefined number and ordering.
 * The sequence is either "s1;s2" if "before" is TRUE or "s2;s1" else.
 *
 *
 * ATTENTION !!! : this version is not for unstructured case
 *
 */

void insert_statement(statement s,
		      statement s1,
		      bool before)
{  
  list ls;
  instruction i = statement_instruction(s);
  if (instruction_sequence_p(i))
    {
      ls = instruction_block(i);
      if (before)	
	ls = CONS(STATEMENT,s1,ls);
      else
	ls = gen_nconc(ls,CONS(STATEMENT,s1,NIL));    
      instruction_block(i) = ls;
    }
  else
    {
      statement s2 = copy_statement(s);   
      if (before)  
	ls = CONS(STATEMENT,s1,CONS(STATEMENT,s2,NIL));
      else
	ls = CONS(STATEMENT,s2,CONS(STATEMENT,s1,NIL));	
      
      statement_comments(s) = empty_comments;
      statement_label(s)= entity_empty_label();
      statement_number(s) = STATEMENT_NUMBER_UNDEFINED;
      statement_ordering(s) = STATEMENT_ORDERING_UNDEFINED;
      
      /* free_instruction(statement_instruction(s));*/
      statement_instruction(s) = make_instruction(is_instruction_sequence,
						  make_sequence(ls));
    } 
}


/* Replace the instruction in statement s by instruction i.
 *
 * Free the old instruction. 
 *
 * If the new instruction is a sequence,
 * add a CONTINUE to carry the label and/or the comments. The
 * statement number and ordering are set to undefined.
 *
 * Else if the new instruction is not a sequence, the statement number
 * and the ordering are also set to undefined because the connexion
 * with the parsed line is lost and the ordering most likely obsolete.
 * If the parsed statement number is still meaningfull, the caller must
 * take care of it.
 *
 * If the initial instruction has been re-used, do not forget
 * to dereference it before calling this function:
 *
 * statement_instruction(s) = instruction_undefined;
 *
 * Be careful with the label and the comments too: they may have
 * been reused.
 *
 *
 * ATTENTION !!! :  this version is not for unstructured case
 *
 *
 */

statement update_statement_instruction(statement s,instruction i)
{
  list seq = NIL;
  statement cs = statement_undefined;


  statement_number(s) = STATEMENT_NUMBER_UNDEFINED;
  statement_ordering(s) = STATEMENT_ORDERING_UNDEFINED;

  if (instruction_sequence_p(i) &&
      ((!statement_with_empty_comment_p(s)) || (!unlabelled_statement_p(s))))
    {
      cs = make_call_statement(CONTINUE_FUNCTION_NAME,
			       NIL,
			       statement_label(s),
			       statement_comments(s));

      statement_comments(s) = empty_comments;
      statement_label(s)= entity_empty_label();

      /* add the CONTINUE statement before the sequence instruction i */
      seq = CONS(STATEMENT, cs, CONS(STATEMENT,instruction_to_statement(i),NIL));

      free_instruction(statement_instruction(s));
      statement_instruction(s) =  make_instruction(is_instruction_sequence,make_sequence(seq));
    }

  else
    {
      free_instruction(statement_instruction(s));
      statement_instruction(s) = i;
    }
  return s;
}

/* as name indicate, a comment is added.
 */
#define ERROR_PREFIX "!ERROR: "
#define BUFSIZE 1024

void add_one_line_of_comment(statement s, string format, ...)
{
  char buffer[BUFSIZE];
  int error;
  va_list some_arguments;

  va_start(some_arguments, format);
  error = vsnprintf(buffer, BUFSIZE, format, some_arguments);
  if (error<0) {
      pips_internal_error("buffer too small");
  }
  va_end(some_arguments);

  if (s) 
  {
      if (string_undefined_p(statement_comments(s)) || statement_comments(s)==NULL) 
      {
	  statement_comments(s) = strdup(concatenate(ERROR_PREFIX, buffer, "\n", NULL));
      }
      else 
      {
	  string newcom = 
	      strdup(concatenate(statement_comments(s), ERROR_PREFIX, buffer, "\n", NULL));
	  free(statement_comments(s));
	  statement_comments(s) = newcom;
      }
  }
  else
  {
      fprintf(stderr, ERROR_PREFIX "%s\n", buffer);
  }
}

static bool find_implicit_goto(statement s, list * tl)
{
  bool found = FALSE;
  
  if(statement_call_p(s)) {
    call c = instruction_call(statement_instruction(s));
    entity f = call_function(c);

    if(ENTITY_WRITE_P(f) || ENTITY_READ_P(f) || ENTITY_OPEN_P(f) || ENTITY_CLOSE_P(f)) {
      list ce = list_undefined;
    
      for(ce = call_arguments(c); !ENDP(ce); ce = CDR(CDR(ce))) {
	expression e = EXPRESSION(CAR(ce));
	entity f1 = call_function(syntax_call(expression_syntax(e)));
      
	pips_assert("expression e must be a call", expression_call_p(e));
      
	if (strcmp(entity_local_name(f1), "ERR=") == 0 || 
	    strcmp(entity_local_name(f1), "END=") == 0) {
	  expression ne = EXPRESSION(CAR(CDR(ce))); /* Next Expression */
	  entity l = call_function(syntax_call(expression_syntax(ne)));
      
	  pips_assert("expression ne must be a call", expression_call_p(ne));
	  pips_assert("l is a label", entity_label_p(l));
	  
	  *tl = CONS(ENTITY, l, *tl);
	}
      }
    }
    /* No need to go down in call statements */
    found = TRUE;
  }
  
  return !found;
}

/* Look for labels appearing in END= or ERR= IO clauses and allocate a
   label list. */
list statement_to_implicit_target_labels(statement s)
{
  list ll = NIL; /* Label List */
   
  gen_context_recurse(s, (void *) &ll,statement_domain, find_implicit_goto, gen_null);
  
  return ll;
}

/* Make sure that s and all its substatements are defined */

static bool undefined_statement_found_p(statement s, bool * p_undefined_p)
{
  if(!(* p_undefined_p)) {
    ifdebug(9) {
      fprintf(stderr, "----\n");
      /* The recursive descent in s is fatal when there is a problem. */
      /* print_statement(s); */
      if(statement_undefined_p(s)) {
	/* You probably want a breakpoint here... */
	abort();
      }
      else {
	pips_debug(9,"checking statement %03td (%td,%td) with address %p:\n",
		   statement_number(s), ORDERING_NUMBER(statement_ordering(s)), 
		   ORDERING_STATEMENT(statement_ordering(s)), s);
      }
    }
    * p_undefined_p = statement_undefined_p(s);
  }
  return !(* p_undefined_p);
}

bool all_statements_defined_p(statement s)
{
  bool undefined_p = FALSE;

  gen_context_recurse(s, (void *) &undefined_p ,statement_domain,
		      undefined_statement_found_p, gen_null);
  return !undefined_p;
}

/* Since block cannot carry comments nor line numbers, they must be
   moved to an internal continue statement.

   A prettier version could be to make a new block containing the
   continue and then the old block. Comments might be better located.
 */
statement add_comment_and_line_number(statement s, string sc, int sn)
{
  string osc = statement_comments(s);
  statement ns = s;

  if(osc!=NULL && !string_undefined_p(osc)) {
    free(osc);
  }

  if(!statement_block_p(s)) {
    statement_comments(s) = sc;
    statement_number(s) = sn;
  }
  else if(!string_undefined_p(sc)) {
    /* A continue statement must be inserted as first block statement*/
    statement nops = make_continue_statement(entity_undefined);
    list sss = sequence_statements(instruction_sequence(statement_instruction(s)));
    statement_comments(nops) = sc;
    statement_number(nops) = sn;
    sequence_statements(instruction_sequence(statement_instruction(s))) =
      CONS(STATEMENT, nops, sss);
  }
  return ns;
}

static list statement_to_all_included_declarations;

static bool add_statement_declarations(statement s)
{
  list dl = statement_declarations(s);
  if(!ENDP(dl)) {
    statement_to_all_included_declarations
      = gen_nconc(statement_to_all_included_declarations,
		  gen_copy_seq(dl));
  }
  return TRUE;
}

/* Get a list of all variables declared recursively within a statement */
list statement_to_declarations(statement s)
{
  statement_to_all_included_declarations = NIL;

  gen_multi_recurse(s, statement_domain, add_statement_declarations, gen_null, NULL);

  return statement_to_all_included_declarations;
}

static list statement_to_all_included_referenced_entities;

static bool add_statement_referenced_entities(reference r)
{
  /* Sometimes, a function may be referenced, for instance in a
     function pointer assignment. */
  entity v = reference_variable(r);

  statement_to_all_included_referenced_entities
    = gen_nconc(statement_to_all_included_referenced_entities,
		CONS(ENTITY, v, NIL));
  return TRUE;
}

/* Get a list of all variables referenced recursively within a statement */
list statement_to_referenced_entities(statement s)
{
  statement_to_all_included_referenced_entities = NIL;

  gen_multi_recurse(s, reference_domain, add_statement_referenced_entities, gen_null, NULL);

  return statement_to_all_included_referenced_entities;
}

static list statement_to_all_included_called_user_entities;

static bool add_statement_called_user_entities(call c)
{
  entity f = call_function(c);

  if(!intrinsic_entity_p(f)) {
    statement_to_all_included_called_user_entities
      = gen_nconc(statement_to_all_included_called_user_entities,
		  CONS(ENTITY, f, NIL));
  }
  return TRUE;
}

/* Get a list of all user function called recursively within a statement */
list statement_to_called_user_entities(statement s)
{
  statement_to_all_included_called_user_entities = NIL;

  gen_multi_recurse(s, statement_domain, add_statement_called_user_entities, gen_null, NULL);

  return statement_to_all_included_declarations;
}

/* Return first reference found */
static reference first_reference_to_v = reference_undefined;
static entity variable_searched = entity_undefined;
static bool first_reference_to_v_p(reference r)
{
  bool result= TRUE;
  if (reference_undefined_p(first_reference_to_v)) {
    entity rv = reference_variable(r);
    if (rv == variable_searched) {
      first_reference_to_v = r;
      result = FALSE;
    }
  }
  else
    result = FALSE;

  return result;
}

reference find_reference_to_variable(statement s, entity v) 
{
  reference r = reference_undefined;
  first_reference_to_v = reference_undefined;
  variable_searched = v;
  gen_recurse(s, reference_domain, first_reference_to_v_p, gen_null);
  r = copy_reference(first_reference_to_v);
  first_reference_to_v = reference_undefined;
  variable_searched = entity_undefined;
  return r;
}


/* That's all folks */
