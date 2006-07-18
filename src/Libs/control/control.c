/* $Id$ */

#ifndef lint
char vcid_control_control[] = 
"$Id$\nCopyright (c) �cole des Mines de Paris Proprietary.";
#endif /* lint */

/* - control.c

   Computes the Hierarchical Control Flow Graph of a given statement. 

   WARNINGS: 

   . Temporary locations malloced while recursing in the process are
     not freed (to be done latter ... if required)
   . The desugaring of DO loops is not perfect (in case of side-effects
     inside loop ranges.

   Pierre Jouvelot (27/5/89) <- this is a French date:-)

   MODIFICATIONS:

   . hash_get interface modification: in one hash table an undefined key
   meant an error; in another one an undefined key was associated to
   the default value empty_list; this worked as long as NULL was returned
   as NOT_FOUND value (i.e. HASH_UNDEFINED_VALUE); this would work again
   if HASH_UNDEFINED_VALUE can be user definable; Francois Irigoin, 7 Sept. 90

*/

/*
 * $Id$
 */

#include <stdio.h>
#include <strings.h>

#include "linear.h"

#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "control.h"
#include "properties.h"

#include "misc.h"

#include "constants.h"

#include "c_syntax.h"

#define LABEL_TABLES_SIZE 10
 
/* UNREACHABLE is the hook used as a predecessor of statements that are  
   following a goto. The list of unreachable statements is kept in the
   successors list. */

static list Unreachable;		

/* LABEL_STATEMENTS maps label names to the list of statements where
   they appear (either as definition or reference). */

static hash_table Label_statements;	

/* LABEL_CONTROL maps label names to their (possible forward) control
   nodes. */

static hash_table Label_control;	


#define MAKE_CONTINUE_STATEMENT() make_continue_statement(entity_undefined) 



/* HASH_GET_DEFAULT_EMPTY_LIST: returns an empty list instead of
   HASH_UNDEFINED_VALUE when a key is not found */

static list hash_get_default_empty_list(h, k)
hash_table h;
char * k;
{
    list l = (list) hash_get(h, k);

    return (l == (list) HASH_UNDEFINED_VALUE)? NIL : l;
}


/* PUSHNEW pushes a control X on the list L if it's not here. */

static cons * pushnew(x, l)
control x;
cons *l;
{
    cons *ll = l;

    MAPL(elts, {if(CONTROL(CAR(elts)) == x) return(l);},
	  ll)
    return(CONS(CONTROL, x, l));
}

/* Add control "pred" to the predecessor set of control c */
#define ADD_PRED(pred,c) (pushnew(pred,control_predecessors(c)))
/* Make a one element list from succ */
#define ADD_SUCC(succ,c) (CONS(CONTROL, succ, NIL))
/* Update control c by setting its statement to s, by unioning its predecessor
 * set with pd, and by setting its successor set to sc (i.e. previous successors
 * are lost, but not previous predecessors).
 */
#define UPDATE_CONTROL(c,s,pd,sc) { \
	control_statement(c)=s; \
	MAPL(preds, {control_predecessors(c) = \
			      ADD_PRED(CONTROL(CAR(preds)), c);}, \
	      pd); \
	control_successors(c)=sc; \
	}

/* PATCH_REFERENCES replaces all occurrences of FNODE by TNODE in the
   predecessors or successors lists of its predecessors
   or successors list (according to HOW). */

#define PREDS_OF_SUCCS 1
#define SUCCS_OF_PREDS 2

static void patch_references(how, fnode, tnode)
int how;
control fnode, tnode;
{
    MAPL(preds, {
	control pred = CONTROL(CAR(preds));
	
	MAPL(succs, {
	    if(CONTROL(CAR(succs)) == fnode)
		    CONTROL_(CAR(succs)) = tnode;
	}, (how == SUCCS_OF_PREDS) ? 
	     control_successors(pred) :
	     control_predecessors(pred));
    }, (how == SUCCS_OF_PREDS) ? 
	 control_predecessors(fnode) : 
	 control_successors(fnode));
}

/* MAKE_CONDITIONAL_CONTROL is make_control except when the statement ST
   has a label and is thus already in Label_control. */

static control make_conditional_control(st) 
statement st;
{
    string label = entity_name(statement_label(st));

    if(empty_label_p(label)) {
	return(make_control(st, NIL, NIL));
    }
    else {
	return((control)hash_get_default_empty_list(Label_control, label));
    }
}

/* GET_LABEL_CONTROL returns the control node corresponding to a
   useful label NAME in the Label_control table. */

static control get_label_control(name)
string name;
{
    control c;

    pips_assert("get_label_control", !empty_label_p(name)) ;
    c = (control)hash_get(Label_control, name);
    pips_assert("get_label_control", c != (control) HASH_UNDEFINED_VALUE);
    return(c);
}

/* UPDATE_USED_LABELS adds the reference to the label NAME in the
   statement ST. A used_label is a hash_table that maps the label
   name to the list of statements that references it. */

static void update_used_labels(used_labels, name, st)
hash_table used_labels;
string name;
statement st;
{
    cons *sts ;

    if( !empty_label_p(name) ) {
	list new_sts;
	sts = hash_get_default_empty_list(used_labels, name) ;
	new_sts = CONS(STATEMENT, st, sts);
	if (hash_defined_p(used_labels, name))
	    hash_update(used_labels, name, (char*) new_sts);
	else
	    hash_put(used_labels, name, (char*) new_sts);
	debug(5, "update_used_labels", "Reference to statement %d seen\n", 
	      statement_number( st )) ;
    }
}

/* UNION_USED_LABELS unions the used-labels list L1 and L2 and returns the
   result in L1 */

static hash_table union_used_labels(l1, l2)
hash_table l1, l2;
{
    HASH_MAP(name, sts, {
	MAPL(stts, {
	    update_used_labels(l1, name, STATEMENT(CAR(stts)));
	}, (cons *)sts);
    }, l2);
    return( l1 ) ;
}

/* COVERS_LABELS_P returns whether a USED_LABELS list for statement ST
   covers all the references to its labels. */

static bool covers_labels_p(st,used_labels)
statement st ;
hash_table used_labels;
{
    if( get_debug_level() >= 5 ) {
	pips_debug(0, "Statement %d (%p): \n ", statement_number( st ), st);
	print_statement(st);
    }
    HASH_MAP(name, sts, {
	cons *stats = (cons *)sts;
	
	MAPL(defs, {
	    bool found = FALSE;
	    statement def = STATEMENT(CAR(defs));

	    MAPL(sts, {
		found |= (STATEMENT(CAR(sts))==def);
	    }, stats);

	    if(!found) {
		pips_debug(5, "does not cover label %s\n", (char *) name);
		return(FALSE);
	    }
	}, (cons *)hash_get_default_empty_list(Label_statements, name));
    }, used_labels);

    if( get_debug_level() >= 5 ) {
	fprintf( stderr, "covers its label usage\n" ) ;
    }
    return(TRUE);
}

static void add_proper_successor_to_predecessor(control pred, control c_res)
{
  /* Replaces the following statement: */
  /* control_successors(pred) = ADD_SUCC(c_res, pred); */

  /* Usually, too much of a mess: do not try to be too strict! */
  /*
  if(statement_test_p(control_statement(pred))) {
    control_successors(pred) = gen_nconc(control_successors(pred),
					 CONS(CONTROL, c_res, NIL));
    pips_assert("While building the CFG, "
		"a test control node may have one or two successors",
		gen_length(control_successors(pred))<=2);
  }
  else {
    if(gen_length(control_successors(pred))==0) {
      control_successors(pred) = CONS(CONTROL, c_res, NIL);
    }
    else if(gen_length(control_successors(pred))==1) {
      if(gen_in_list_p(succ,control_successors(pred))) {
	;
      }
      else {
	pips_internal_error("Two or more candidate successors "
			    "for a standard statement: %p and %p\n",
			    succ, CONTROL(CAR(control_successors(pred))));
      }
    }
    else {
      pips_internal_error("Two or more successors for non-test node %p\n",
			  pred);
    }
  }
  */

  if(statement_test_p(control_statement(pred))) {
    if(!gen_in_list_p(c_res, control_successors(pred))) {
      control_successors(pred) = gen_nconc(control_successors(pred),
					   CONS(CONTROL, c_res, NIL));
    }
    pips_assert("While building the CFG, "
		"a test control node may have one or two successors",
		gen_length(control_successors(pred))<=2);
  }
  else {
    /* Do whatever was done before and let memory leak! */
    control_successors(pred) = ADD_SUCC(c_res, pred);
  }
}

/* CONTROLIZE computes in C_RES the control node of the statement ST
   whose predecessor control node is PRED and successor SUCC. The
   USED_LABELS is modified to deal with local use of labels. Returns TRUE
   if the current statement isn't a structured control. The invariant is
   that CONTROLIZE links predecessors and successors of C_RES, updates the
   successors of PRED and the predecessors of SUCC.

   In fact, it cannot update the successors of PRED because it cannot know
   which successor of PRED C_RES is when PRED is associated to a
   test. PRED and C_RES must be linked together when you enter
   controlize(), or they must be linked later by the caller. But they
   cannot be linked here thru the successor list of PRED and, if the consistency
   is true here, they cannot be linked either by the predecessor list of
   SUCC. If they are linked later, it is useless to pass PRED down. If
   they are linked earlier, they might have to be unlinked when structured
   code is found. */

bool controlize(
    statement st,
    control pred, 
    control succ,
    control c_res,
    hash_table used_labels)
{
    instruction i = statement_instruction(st);
    string label = entity_name(statement_label(st));
    bool controlize_list(), controlize_test(), controlize_loop(),
	controlize_whileloop(), controlize_call();
    bool controlized=FALSE;

    ifdebug(5) {
	pips_debug(1, 
	   "(st = %p, pred = %p, succ = %p, c_res = %p)\nst at entry:\n",
		   st, pred, succ, c_res);
	print_statement(st);
	/*
	pips_assert("pred is a predecessor of c_res",
		    gen_in_list_p(pred, control_predecessors(c_res)));
	pips_assert("c_res is a successor of pred",
		    gen_in_list_p(c_res, control_successors(pred)));
	*/
	pips_debug(1, "Result c_res %p:\n", c_res);
	display_linked_control_nodes(c_res);
	check_control_coherency(pred);
	check_control_coherency(succ);
	check_control_coherency(c_res);
    }
    
    switch(instruction_tag(i)) {
    case is_instruction_block: {
	controlized = controlize_list(st, instruction_block(i),
				      pred, succ, c_res, used_labels);
	/* If st carries local declarations, so should the statement associated to c_res. */
	if(!ENDP(statement_declarations(st))
	   && ENDP(statement_declarations(control_statement(c_res)))) {
	  pips_internal_error("Lost local declarations\n");
	}
	break;
    }
    case is_instruction_test:
	controlized = controlize_test(st, instruction_test(i), 
				      pred, succ, c_res, used_labels);
	break;
    case is_instruction_loop:
	controlized = controlize_loop(st, instruction_loop(i),
				      pred, succ, c_res, used_labels);
	break;
    case is_instruction_whileloop:
	controlized = controlize_whileloop(st, instruction_whileloop(i),
					   pred, succ, c_res, used_labels);
	break;
    case is_instruction_goto: {
	string name = entity_name(statement_label(instruction_goto(i)));
        statement nop = make_continue_statement(statement_label(st));

        statement_number(nop) = statement_number(st);
        statement_comments(nop) = statement_comments(st);
	succ = get_label_control(name);
	/* Memory leak in CONS(CONTROL, pred, NIL). Also forgot to
           unlink the predecessor of the former successor of pred. RK */
	/* control_successors(pred) = ADD_SUCC(c_res, pred); */
	add_proper_successor_to_predecessor(pred, c_res);
	UPDATE_CONTROL(c_res, nop, 
		       CONS(CONTROL, pred, NIL), 
		       ADD_SUCC(succ, c_res )) ;
	control_predecessors(succ) = ADD_PRED(c_res, succ);
	/* I do not know why, but my following code does not work. So
           I put back former one above... :-( RK. */
#if 0
	/* Use my procedures instead to set a GOTO from pred to
           c_res. RK */
	if (gen_length(control_successors(pred)) == 1)
	    unlink_2_control_nodes(pred, CONTROL(CAR(control_successors(pred))));
	link_2_control_nodes(pred, c_res);
	link_2_control_nodes(c_res, succ);
	/* Hmmm... A memory leak on the previous statement of c_res? */
	control_statement(c_res) = nop;
#endif
	update_used_labels(used_labels, name, st);
	controlized = TRUE;
	break;
    }
    case is_instruction_call:
	/* FI: IO calls may have control effects; they should be handled here! */
	controlized = controlize_call(st, instruction_call(i), 
				      pred, succ, c_res, used_labels);
	break;
    case is_instruction_forloop:
      pips_assert("We are really dealing with a for loop",
		  instruction_forloop_p(statement_instruction(st)));
      controlized = controlize_forloop(st, instruction_forloop(i), 
				       pred, succ, c_res, used_labels);
      break;
    case is_instruction_return:
      controlized =  TRUE;
      break;
    case is_instruction_expression:
      controlized =  TRUE;
      break;
    default:
	pips_error("controlize", 
		   "Unknown instruction tag %d\n", instruction_tag(i));
    }

    ifdebug(5) {
	pips_debug(1, "st at exit:\n");
	print_statement(st);
	pips_debug(1, "Resulting Control c_res %p at exit:\n", c_res);
	display_linked_control_nodes(c_res);
	fprintf(stderr, "---\n");
	if(!ENDP(statement_declarations(st))
	   && ENDP(statement_declarations(control_statement(c_res)))) {
	  pips_internal_error("Lost local declarations\n");
	}
	check_control_coherency(pred);
	check_control_coherency(succ);
	check_control_coherency(c_res);
    }
    
    update_used_labels(used_labels, label, st);
    return(controlized);
}


/* CONTROLIZE_CALL controlizes the call C of statement ST in C_RES. The deal
   is to correctly manage STOP; since we don't know how to do it, so we
   assume this is a usual call with a continuation !!

   To avoid non-standard successors, IO statement with multiple
   continuations are not dealt with here. The END= and ERR= clauses are
   simulated by hidden tests. */

bool controlize_call(st, c, pred, succ, c_res, used_labels)
statement st;
call c;
control pred, succ;
control c_res;
hash_table used_labels;
{
  pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	     st, pred, succ, c_res);
        
  pips_assert("Useless parameters", c==c && used_labels==used_labels);

  UPDATE_CONTROL(c_res, st,
		 ADD_PRED(pred, c_res), 
		 CONS(CONTROL, succ, NIL));

  /* control_successors(pred) = ADD_SUCC(c_res, pred); */
  add_proper_successor_to_predecessor(pred, c_res);
   
  control_predecessors(succ) = ADD_PRED(c_res, succ);
  return(FALSE);
}

/* LOOP_HEADER, LOOP_TEST and LOOP_INC build the desugaring phases of a
   do-loop L for the loop header (i=1), the test (i<10) and the increment
   (i=i+1). */

statement loop_header(statement sl) 
{
  loop l = statement_loop(sl);
  statement hs = statement_undefined;

  expression i = make_entity_expression(loop_index(l), NIL);

  hs = make_assign_statement(i, range_lower(loop_range(l)));
  statement_number(hs) = statement_number(sl);

  return hs;
}

statement loop_test(statement sl)
{
  loop l = statement_loop(sl);
  statement ts = statement_undefined;
  string cs = string_undefined;
  call c = make_call(entity_intrinsic(".GT."),
		     CONS(EXPRESSION,
			  make_entity_expression(loop_index(l), NIL),
			  CONS(EXPRESSION,
			       range_upper(loop_range(l)),
			       NIL)));
  test t = make_test(make_expression(make_syntax(is_syntax_call, c),
				     normalized_undefined), 
		     MAKE_CONTINUE_STATEMENT(), 
		     MAKE_CONTINUE_STATEMENT());
  string csl = statement_comments(sl);
  string prev_comm = empty_comments_p(csl)? strdup("") : strdup(csl);
  string lab = string_undefined;

  if(entity_empty_label_p(loop_label(l)))
    lab = "";
  else 
    lab = label_local_name(loop_label(l));

  if (get_bool_property("PRETTYPRINT_C_CODE"))
    cs = prev_comm;
  else 
    cs = strdup(concatenate(prev_comm,
			    "C     DO loop ",
			    lab,
			    " with exit had to be desugared\n",
			    NULL));

  ts = make_statement(entity_empty_label(), 
		      statement_number(sl),
		      STATEMENT_ORDERING_UNDEFINED,
		      cs,
		      make_instruction(is_instruction_test, t),NIL,NULL);

  return ts;
}

statement loop_inc(statement sl)
{
  loop l = statement_loop(sl);
  expression I = make_entity_expression(loop_index(l), NIL);
  expression II = make_entity_expression(loop_index(l), NIL);
  call c = make_call(entity_intrinsic("+"), 
		     CONS(EXPRESSION, 
			  I, 
			  CONS(EXPRESSION, 
			       range_increment(loop_range(l)), 
			       NIL)));
  expression I_plus_one = 
    make_expression(make_syntax(is_syntax_call, c), 
		    normalized_undefined);
  statement is = statement_undefined;

  is = make_assign_statement(II, I_plus_one);
  statement_number(is) = statement_number(sl);

  return is;
}

/* CONTROLIZE_LOOP computes in C_RES the control graph of the loop L (of
   statement ST) with PREDecessor and SUCCessor. */

bool controlize_loop(st, l, pred, succ, c_res, used_labels)
statement st;
loop l;
control pred, succ;
control c_res;
hash_table used_labels;
{
    hash_table loop_used_labels = hash_table_make(hash_string, 0);
    control c_body = make_conditional_control(loop_body(l));
    control c_inc = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
    control c_test = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
    bool controlized;

    pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	       st, pred, succ, c_res);
    
    controlize(loop_body(l), c_test, c_inc, c_body, loop_used_labels);

    if(covers_labels_p(loop_body(l),loop_used_labels)) {
	loop new_l = make_loop(loop_index(l),
			       loop_range(l),
			       control_statement(c_body),
			       loop_label(l),
			       loop_execution(l),
			       loop_locals(l));

	UPDATE_CONTROL(c_res,
		       make_statement(statement_label(st),
				      statement_number(st),
				      STATEMENT_ORDERING_UNDEFINED,
				      statement_comments(st),
				      make_instruction(is_instruction_loop, new_l),
				      statement_declarations(st),
				      statement_decls_text(st)),
		       ADD_PRED(pred, c_res),
		       ADD_SUCC(succ, c_res )) ;
	controlized = FALSE;
	control_predecessors(succ) = ADD_PRED(c_res, succ);
    }
    else {
	control_statement(c_test) = loop_test(st);
	control_predecessors(c_test) =
		CONS(CONTROL, c_res, CONS(CONTROL, c_inc, NIL)),
	control_successors(c_test) =
		CONS(CONTROL, succ, CONS(CONTROL, c_body, NIL));
	control_statement(c_inc) = loop_inc(st);
	control_successors(c_inc) = CONS(CONTROL, c_test, NIL);
	UPDATE_CONTROL(c_res,
		       loop_header(st),
		       ADD_PRED(pred, c_res),
		       CONS(CONTROL, c_test, NIL));
	controlized = TRUE ;
	control_predecessors(succ) = ADD_PRED(c_test, succ);
    }
    add_proper_successor_to_predecessor(pred, c_res);
    /* control_successors(pred) = ADD_SUCC(c_res, pred); */

    union_used_labels( used_labels, loop_used_labels);
    hash_table_free(loop_used_labels);
    
    pips_debug(5, "Exiting\n");
    
    return(controlized);
}


/* Generate a test statement ts for exiting loop sl.
 * There should be no sharing between sl and ts.
 */
statement whileloop_test(statement sl)
{
    whileloop l = instruction_whileloop(statement_instruction(sl));
    statement ts = statement_undefined;
    string cs = string_undefined;
    call c = make_call(entity_intrinsic(get_bool_property("PRETTYPRINT_C_CODE")?"!":".NOT."),
		       CONS(EXPRESSION,
			    copy_expression(whileloop_condition(l)),
			    NIL));
    test t = make_test(make_expression(make_syntax(is_syntax_call, c),
				       normalized_undefined), 
		       MAKE_CONTINUE_STATEMENT(), 
		       MAKE_CONTINUE_STATEMENT());
    string csl = statement_comments(sl);
    /* string prev_comm = empty_comments_p(csl)? "" : strdup(csl); */
    string prev_comm = empty_comments_p(csl)? strdup("") : strdup(csl);

    if (get_bool_property("PRETTYPRINT_C_CODE"))
      cs = prev_comm;
    else 
      {
	if(entity_empty_label_p(whileloop_label(l))) { 
	  cs = strdup(concatenate(prev_comm,
				  "C     DO WHILE loop ",
				  "with GO TO exit had to be desugared\n",
				  NULL));
	}
	else {
	  string lab = label_local_name(whileloop_label(l));
	  cs = strdup(concatenate(prev_comm,
				  "C     DO WHILE loop ",
				  lab,
				  " with GO TO exit had to be desugared\n",
				  NULL));
	}
      }
    
    ts = make_statement(entity_empty_label(), 
			statement_number(sl),
			STATEMENT_ORDERING_UNDEFINED,
			cs,
			make_instruction(is_instruction_test, t),NIL,NULL);

    return ts;
}


/* CONTROLIZE_WHILELOOP computes in C_RES the control graph of the loop L (of
 *  statement ST) with PREDecessor and SUCCessor
 *
 * Derived by FI from controlize_loop()
 */

/* NN : What about other kind of whileloop, evaluation = after ? TO BE IMPLEMENTED   */

bool controlize_whileloop(st, l, pred, succ, c_res, used_labels)
statement st;
whileloop l;
control pred, succ;
control c_res;
hash_table used_labels;
{
    hash_table loop_used_labels = hash_table_make(hash_string, 0);
    control c_body = make_conditional_control(whileloop_body(l));
    bool controlized;

    pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	       st, pred, succ, c_res);
    
    controlize(whileloop_body(l), c_res, c_res, c_body, loop_used_labels);

    if(covers_labels_p(whileloop_body(l),loop_used_labels)) {
	whileloop new_l = make_whileloop(whileloop_condition(l),
					 control_statement(c_body),
					 whileloop_label(l),
					 whileloop_evaluation(l));

	/* The edges between c_res and c_body, created by the above call to 
	 * controlize are useless. The edge succ
	 * from c_res to c_body is erased by the UPDATE_CONTROL macro.
	 */
	gen_remove(&control_successors(c_body), c_res);
	gen_remove(&control_predecessors(c_body), c_res);
	gen_remove(&control_predecessors(c_res), c_body);

	UPDATE_CONTROL(c_res,
		       make_statement(statement_label(st),
				      statement_number(st),
				      STATEMENT_ORDERING_UNDEFINED,
				      statement_comments(st),
				      make_instruction(is_instruction_whileloop, 
						       new_l),
				      statement_declarations(st),
				      statement_decls_text(st)),
		       ADD_PRED(pred, c_res),
		       ADD_SUCC(succ, c_res )) ;
	controlized = FALSE;
    }
    else {
	control_statement(c_res) = whileloop_test(st);
	/* control_predecessors(c_res) =
	   CONS(CONTROL, pred, control_predecessors(c_res)); */
	/* ADD_PRED(pred, c_res); */
	control_predecessors(c_res) =
	   gen_once(pred, control_predecessors(c_res));
	control_successors(c_res) =
		CONS(CONTROL, succ, control_successors(c_res));
	controlized = TRUE ;
	/* Cannot be consistent yet! */
	/* ifdebug(5) check_control_coherency(c_res); */
    }
    control_predecessors(succ) = ADD_PRED(c_res, succ);
    add_proper_successor_to_predecessor(pred, c_res);
    /* control_successors(pred) = ADD_SUCC(c_res, pred); */

    ifdebug(5) check_control_coherency(c_res);

    union_used_labels( used_labels, loop_used_labels);
    hash_table_free(loop_used_labels);
    
    pips_debug(5, "Exiting\n");
    
    return(controlized);
}


statement forloop_header(statement sl) 
{
  forloop l = instruction_forloop(statement_instruction(sl));
  statement hs = instruction_to_statement(make_instruction_expression(forloop_initialization(l)));
  statement_number(hs) = statement_number(sl);

  return hs;
}

statement forloop_test(statement sl)
{
  forloop l = instruction_forloop(statement_instruction(sl));
  call c =  make_call(entity_intrinsic("!"),
		      CONS(EXPRESSION,
			   copy_expression(forloop_condition(l)),
			   NIL));
  test t = make_test(make_expression(make_syntax(is_syntax_call, c),
				     normalized_undefined), 
		     MAKE_CONTINUE_STATEMENT(), 
		     MAKE_CONTINUE_STATEMENT());
  string csl = statement_comments(sl);
  string cs = empty_comments_p(csl)? strdup("") : strdup(csl);

  statement ts = make_statement(entity_empty_label(), 
				statement_number(sl),
				STATEMENT_ORDERING_UNDEFINED,
				cs,
				make_instruction(is_instruction_test, t),NIL,NULL);
  
  return ts;
}

statement forloop_inc(statement sl)
{
  forloop l = instruction_forloop(statement_instruction(sl));
  statement is = instruction_to_statement(make_instruction_expression(forloop_increment(l)));
  statement_number(is) = statement_number(sl);
  
  return is;
}

static bool init_expression_to_index_and_initial_bound(expression init,
						       entity * pli,
						       expression * plb)
{
  bool success = FALSE;
  syntax init_s = expression_syntax(init);

  pips_debug(5, "Begin\n");

  if(syntax_call_p(init_s)) {
    call c = syntax_call(init_s);
    entity op = call_function(c);

    if(ENTITY_ASSIGN_P(op)) {
      expression lhs = EXPRESSION(CAR(call_arguments(c)));
      syntax lhs_s = expression_syntax(lhs);
      if(syntax_reference_p(lhs_s)) {
	reference r = syntax_reference(lhs_s);
	entity li = reference_variable(r);
	type lit = entity_type(li);

	if(ENDP(reference_indices(r)) /* scalar reference */
	   && type_variable_p(lit) 
	   && basic_int_p(variable_basic(type_variable(lit)))) {
	  *pli = li;
	  /* To avoid sharing and intricate free operations */
	  *plb = copy_expression(EXPRESSION(CAR(CDR(call_arguments(c)))));
	  success = TRUE;
	}
      }
    }
  }

  ifdebug(5) {
    if(success) {
      pips_debug(5, "End with loop index %s and init expression:\n", entity_local_name(*pli));
      print_expression(*plb);
    }
    else {
      pips_debug(5, "End: DO conversion failed with initialization expression\n");
    }
  }

  return success;
}

/* Depending on cond, the so-called upper bound may end up being a lower
   bound with a decreasing steop loop. */
static bool condition_expression_to_final_bound(expression cond,
						entity li,
						bool* p_is_upper_p,
						bool* p_is_lower_p,
						expression * pub)
{
  bool success = FALSE;
  syntax cond_s = expression_syntax(cond);
  bool strict_p = FALSE;

  ifdebug(5) {
    pips_debug(5, "Begin with expression\n");
    print_expression(cond);
  }

  if(syntax_call_p(cond_s)) {
    call c = syntax_call(cond_s);
    entity op = call_function(c);

    /* Four operators are accepted */
    if (ENTITY_LESS_THAN_P(op) || ENTITY_LESS_OR_EQUAL_P(op)
	|| ENTITY_GREATER_THAN_P(op) || ENTITY_GREATER_OR_EQUAL_P(op)) {
      expression e1 = EXPRESSION(CAR(call_arguments(c)));
      expression e2 = EXPRESSION(CAR(CDR(call_arguments(c))));
      syntax e1_s = expression_syntax(e1);
      syntax e2_s = expression_syntax(e2);

      strict_p = ENTITY_LESS_THAN_P(op) || ENTITY_GREATER_THAN_P(op);

      if(syntax_reference_p(e1_s) && reference_variable(syntax_reference(e1_s))==li) {
	*p_is_upper_p = ENTITY_LESS_THAN_P(op) || ENTITY_LESS_OR_EQUAL_P(op);
	*pub = convert_bound_expression(e2, TRUE, ENTITY_LESS_OR_EQUAL_P(op));
	success = TRUE;
      }
      else if(syntax_reference_p(e2_s) && reference_variable(syntax_reference(e2_s))==li) {
	*p_is_lower_p = ENTITY_GREATER_THAN_P(op) || ENTITY_GREATER_OR_EQUAL_P(op);
	*pub = convert_bound_expression(e2, FALSE, ENTITY_GREATER_OR_EQUAL_P(op));
	success = TRUE;
      }
    }
  }

  ifdebug(5) {
    if(success) {
      pips_debug(5, "End with expression\n");
      print_expression(cond);
      pips_debug(5, "Loop counter is increasing: %s\n", bool_to_string(*p_is_upper_p))
	pips_debug(5, "Loop condition is strict: %s\n", bool_to_string(*p_is_lower_p))
	}
    else {
      pips_debug(5, "End: no final bound available\n");
    }
  }

  return success;
}

static bool incrementation_expression_to_increment(expression incr,
						   entity li,
						   bool * is_increasing_p,
						   bool * is_decreasing_p,
						   expression * pincrement)
{
  bool success = FALSE;
  syntax incr_s = expression_syntax(incr);

  if(syntax_call_p(incr_s)) {
    call incr_c = syntax_call(incr_s);
    entity op = call_function(incr_c);

    if((ENTITY_POST_INCREMENT_P(op) || ENTITY_PRE_INCREMENT_P(op))
       && is_expression_reference_to_entity_p(EXPRESSION(CAR(call_arguments(incr_c))), li)) {
      * is_increasing_p = TRUE;
      * pincrement = int_to_expression(1);
      success = TRUE;
    }
    else if((ENTITY_POST_DECREMENT_P(op) || ENTITY_PRE_DECREMENT_P(op))
       && is_expression_reference_to_entity_p(EXPRESSION(CAR(call_arguments(incr_c))), li)) {
      * is_decreasing_p = TRUE;
      * pincrement = int_to_expression(-1);
      success = TRUE;
    }
  }

  return success;
}

static loop for_to_do_loop_conversion(expression init,
				      expression cond,
				      expression incr,
				      statement body)
{
  loop l = loop_undefined;
  entity li = entity_undefined; /* loop index */
  range lr = range_undefined; /* loop bound and increment expressions */
  expression lb = expression_undefined;
  expression ub = expression_undefined;
  expression increment = expression_undefined;

  /* Pattern checked:

   * The init expression is an assignment to an integer scalar (not fully necessary)
     It failed on the first submitted for loop which had two assignments as
     initialization steap. One of the them was the loop index assignment.
     More work is needed to retrieve all DO loops programmed as for loops.

   * The condition is a relational operation with the index

   * The incrementation expression does not really matter as long as it
   * updates the loop index.

   */

  if(init_expression_to_index_and_initial_bound(init, &li, &lb)) {
      bool is_lower_p = FALSE;
      bool is_upper_p = FALSE;

    if(condition_expression_to_final_bound(cond, li, &is_upper_p, &is_lower_p, &ub)) {
      bool is_increasing_p = FALSE;
      bool is_decreasing_p = FALSE;

      if(incrementation_expression_to_increment(incr, li, &is_increasing_p,
						&is_decreasing_p, &increment)) {
	if(is_lower_p && is_increasing_p)
	  pips_user_warning("Loop with lower bound and increasing index %s\n",
			    entity_local_name(li));
	if(is_upper_p && is_decreasing_p)
	  pips_user_warning("Loop with upper bound and decreasing index %s\n",
			    entity_local_name(li));
	lr = make_range(lb, ub, increment);
	l = make_loop(li, lr, body, entity_empty_label(),
		      make_execution(is_execution_sequential, UU), NIL);
      }
    }
  }

  return l;
}

static sequence for_to_while_loop_conversion(expression init,
					     expression cond,
					     expression incr,
					     statement body)
{
  syntax s_init = expression_syntax(init);
  syntax s_cond = expression_syntax(cond);
  syntax s_incr = expression_syntax(incr);
  sequence wlseq = sequence_undefined;

  pips_debug(5, "Begin\n");

  if(!syntax_call_p(s_init) || !syntax_call_p(s_cond) || !syntax_call_p(s_incr)) {
    pips_internal_error("expression arguments must be calls\n");
  }
  else {
    statement init_st = make_statement(entity_empty_label(), 
				       STATEMENT_NUMBER_UNDEFINED,
				       STATEMENT_ORDERING_UNDEFINED,
				       strdup(""),
				       make_instruction(is_instruction_call, 
							syntax_call(s_init)),
				       NIL,NULL);
    statement incr_st =  make_statement(entity_empty_label(), 
					STATEMENT_NUMBER_UNDEFINED,
					STATEMENT_ORDERING_UNDEFINED,
					strdup(""),
					make_instruction(is_instruction_call,
							 syntax_call(s_incr)),
					NIL,NULL);
    sequence body_seq = make_sequence(CONS(STATEMENT, body,
					CONS(STATEMENT, incr_st, NIL)));
    statement n_body = make_statement(entity_empty_label(), 
				      STATEMENT_NUMBER_UNDEFINED,
				      STATEMENT_ORDERING_UNDEFINED,
				      string_undefined,
				      make_instruction(is_instruction_sequence,
						       body_seq),
				     NIL,NULL);
    whileloop wl_i = make_whileloop(cond, n_body, entity_empty_label(),
				    make_evaluation_before() );
    statement wl_st =  make_statement(entity_empty_label(), 
				     STATEMENT_NUMBER_UNDEFINED,
				     STATEMENT_ORDERING_UNDEFINED,
				     strdup(""),
				     make_instruction(is_instruction_whileloop,
						      wl_i),
				     NIL,NULL);

    ifdebug(5) {
      pips_debug(5, "Initialization statement:\n");
      print_statement(init_st);
      pips_debug(5, "Incrementation statement:\n");
      print_statement(incr_st);
      pips_debug(5, "New body statement with incrementation:\n");
      print_statement(n_body);
      pips_debug(5, "New whileloop statement:\n");
      print_statement(wl_st);
    }

    wlseq = make_sequence(CONS(STATEMENT, init_st,
			       CONS(STATEMENT, wl_st, NIL)));

    /* Clean-up: only cond is reused */

    /* They cannot be freed because a debugging statement at the end of
       controlize does print the initial statement */

    syntax_call(s_init) = call_undefined;
    syntax_call(s_incr) = call_undefined;
    free_expression(init);
    free_expression(incr);
  }

  ifdebug(5) {
    statement d_st =  make_statement(entity_empty_label(), 
				     STATEMENT_NUMBER_UNDEFINED,
				     STATEMENT_ORDERING_UNDEFINED,
				     string_undefined,
				     make_instruction(is_instruction_sequence,
						      wlseq),
				     NIL,NULL);

    pips_debug(5, "End with statement:\n");
    print_statement(d_st);
    statement_instruction(d_st) = instruction_undefined;
    free_statement(d_st);
  }

  return wlseq;
}

bool controlize_forloop(st, l, pred, succ, c_res, used_labels)
statement st;
forloop l;
control pred, succ;
control c_res;
hash_table used_labels;
{
  hash_table loop_used_labels = hash_table_make(hash_string, 0);
  control c_body = make_conditional_control(forloop_body(l));
  control c_inc = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
  control c_test = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
  bool controlized = FALSE; /* To avoid gcc warning about possible
			       non-initialization */

  pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	     st, pred, succ, c_res);
    
  controlize(forloop_body(l), c_test, c_inc, c_body, loop_used_labels);

  if (covers_labels_p(forloop_body(l),loop_used_labels)) {
    instruction ni = instruction_undefined;

    /* Try an unsafe conversion to a Fortran style DO loop: it assumes
       that the loop body does not define the loop index, but effects are
       not yet available when the controlizer is run. */
    if(get_bool_property("FOR_TO_DO_LOOP_IN_CONTROLIZER")) {
	loop new_l = for_to_do_loop_conversion(forloop_initialization(l),
						  forloop_condition(l),
						  forloop_increment(l),
						  control_statement(c_body));

	if(!loop_undefined_p(new_l))
	  ni = make_instruction(is_instruction_loop, new_l);
    }

    /* If the DO conversion has failed, the WHILE conversion may be requested */
    if(instruction_undefined_p(ni)) {
      if(get_bool_property("FOR_TO_WHILE_LOOP_IN_CONTROLIZER")) {
	sequence wls = for_to_while_loop_conversion(forloop_initialization(l),
						    forloop_condition(l),
						    forloop_increment(l),
						    control_statement(c_body));

	/* These three fields have been re-used or freed by the previous call */
	forloop_initialization(l) = expression_undefined;
	forloop_condition(l) = expression_undefined;
	forloop_increment(l) = expression_undefined;

	ni = make_instruction(is_instruction_sequence, wls);
      }
      else {
	forloop new_l = make_forloop(forloop_initialization(l),
				     forloop_condition(l),
				     forloop_increment(l),
				     control_statement(c_body));

	ni = make_instruction(is_instruction_forloop, new_l);
      }
    }

    gen_remove(&control_successors(c_body), c_res);
    gen_remove(&control_predecessors(c_body), c_res);
    gen_remove(&control_predecessors(c_res), c_body);
      
    UPDATE_CONTROL(c_res,
		   make_statement(statement_label(st),
				  statement_number(st),
				  STATEMENT_ORDERING_UNDEFINED,
				  statement_comments(st),
				  ni, 
				  statement_declarations(st),
				  statement_decls_text(st)),
		   ADD_PRED(pred, c_res),
		   ADD_SUCC(succ, c_res )) ;
    controlized = FALSE;
    control_predecessors(succ) = ADD_PRED(c_res, succ);
  }
  else 
    {
      /* NN : I do not know how to deal with this, the following code does not always work 
	   
	 pips_internal_error("Forloop with goto not implemented yet\n");*/
	
      control_statement(c_test) = forloop_test(st);
      control_predecessors(c_test) =
	CONS(CONTROL, c_res, CONS(CONTROL, c_inc, NIL)),
	control_successors(c_test) =
	CONS(CONTROL, succ, CONS(CONTROL, c_body, NIL));
      control_statement(c_inc) = forloop_inc(st);
      control_successors(c_inc) = CONS(CONTROL, c_test, NIL);
      UPDATE_CONTROL(c_res,
		     forloop_header(st),
		     ADD_PRED(pred, c_res),
		     CONS(CONTROL, c_test, NIL));
      controlized = TRUE ;
      control_predecessors(succ) = ADD_PRED(c_test, succ);
	
	
    }
  add_proper_successor_to_predecessor(pred, c_res);
  /* control_successors(pred) = ADD_SUCC(c_res, pred); */
    
  ifdebug(5) check_control_coherency(c_res);
    
  union_used_labels( used_labels, loop_used_labels);
  hash_table_free(loop_used_labels);
    
  pips_debug(5, "Exiting\n");
    
  return(controlized);
}


/* COMPACT_LIST takes a list of controls CTLS coming from a
   CONTROLIZE_LIST and compacts the successive assignments,
   i.e. concatenates (i=1) followed by (j=2) in a single control with
   a block statement (i=1;j=2). The LAST control node is returned in
   case on terminal compaction.

   Added a set to avoid investigating a removed node.
   Many memory leaks removed. RK.   
   In fact this procedure could be replaced by
   fuse_sequences_in_unstructured()... RK.
   */
static control
compact_list(list ctls,
	     control c_end)
{
    control c_res;
    set processed_nodes;
    /* Pointer to the end of the current unstructured: */
    control c_last = c_end ;

    ifdebug(5) {
	pips_debug(0, "List c_end %p, ctls:", c_end);
	display_address_of_control_nodes(ctls);
	fprintf(stderr, "\n");
    }

    if( ENDP( ctls )) {
	return( c_last ) ;
    }

    processed_nodes = set_make(set_pointer);
    c_res = CONTROL(CAR(ctls));

    for(ctls = CDR(ctls); !ENDP(ctls); ctls = CDR(ctls)) {
	cons *succs, *succs_of_succ;
	instruction i, succ_i;
	statement st, succ_st;
	control succ;

	if (set_belong_p(processed_nodes, (char *) c_res)) {
	    /* Do not reprocess an already seen node. */
	    c_res = CONTROL(CAR(ctls));
	    continue;
	}

	if (gen_length(succs=control_successors(c_res)) != 1 ||
	    gen_length(control_predecessors(succ=CONTROL(CAR(succs)))) != 1 ||
	    gen_length(succs_of_succ=control_successors(succ)) != 1 ||
	    CONTROL(CAR(succs_of_succ)) == c_res ) {
	    /* We are at a non structured node or at the exit: keep
               the node and go on inspecting next node from the
               list. RK */
	    c_res = CONTROL(CAR(ctls));
	    continue;
	}
	
	st = control_statement(c_res) ;
	/* Ok, succ is defined. RK */
	succ_st = control_statement(succ);
	set_add_element(processed_nodes, processed_nodes, (char *) succ);
	
	if(c_res != succ) {
	    /* If it is not a loop on c_res, fuse the nodes: */
	    if(!instruction_block_p(i=statement_instruction(st))) {
		i = make_instruction_block(CONS(STATEMENT, st, NIL));
		control_statement(c_res) =
		    make_statement(entity_empty_label(), 
				   STATEMENT_NUMBER_UNDEFINED,
				   STATEMENT_ORDERING_UNDEFINED,
				   string_undefined,
				   i,NIL,NULL);
	    }
	    if(instruction_block_p(succ_i=statement_instruction(succ_st))){
		instruction_block(i) = 
		    gen_nconc(instruction_block(i), 
			      instruction_block(succ_i));
		statement_instruction(succ_st) = instruction_undefined;
		free_statement(succ_st);
	    }
	    else {
		instruction_block(i) =
		    gen_nconc(instruction_block(i), 
			      CONS(STATEMENT, succ_st, NIL));
	    }
	    /* Skip the useless control: */
	    control_statement(succ) = statement_undefined;
	    remove_a_control_from_an_unstructured(succ);
#if 0	    
	    gen_free_list(control_successors(c_res));
	    control_successors(c_res) = control_successors(succ);
	    patch_references(PREDS_OF_SUCCS, succ, c_res);
	    /* Now remove the useless control: */
	    gen_free_list(control_predecessors(succ));
	    control_successors(succ) = NIL;
	    control_predecessors(succ) = NIL;
	    control_statement(succ) = statement_undefined;
	    free_control(succ);
#endif
	}

	if(succ == c_last) {
	    /* We are at the end and the last node has
               disappeared... Update the pointer to the new one: */
	    c_last = c_res;
	    break;
	}
    }
    set_free(processed_nodes);
    return c_last;
}


/* CONTROLIZE_LIST_1 is the equivalent of a mapcar of controlize on STS.
   The trick is to keep a list of the controls to compact them latter. Note
   that if a statement is controlized, then the predecessor has to be
   computed (i.e. is not the previous control on STS).; this is the purpose
   of c_in. */

cons * controlize_list_1(sts, pred, succ, c_res, used_labels)
cons *sts;
control pred, succ;
control c_res;
hash_table used_labels;
{
    cons *ctls = NIL;

    for(; !ENDP(sts); sts = CDR(sts)) {
	statement st = STATEMENT(CAR(sts));
	control c_next = 
		ENDP(CDR(sts)) ? succ : 
			make_conditional_control(STATEMENT(CAR(CDR(sts))));
	bool controlized;
	bool unreachable;

	ifdebug(5) {
	    pips_debug(0, "Nodes linked with pred %p:\n", pred);
	     display_linked_control_nodes(pred);
	}

	ifdebug(1) {
	    check_control_coherency(pred);
	    check_control_coherency(succ);
	    check_control_coherency(c_res);
	}
    
	controlized = controlize(st, pred, c_next, c_res, used_labels);
	unreachable = ENDP(control_predecessors(c_next));

	ctls = CONS(CONTROL, c_res, ctls);

	if(unreachable) {
	    Unreachable = CONS(STATEMENT, st, Unreachable);
	}
	if(controlized) {
	    control c_in = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
	    
	    ctls = CONS(CONTROL, c_in, ctls);
	    control_predecessors(c_in) = control_predecessors(c_next);
	    control_successors(c_in) = CONS(CONTROL, c_next, NIL);
	    patch_references(SUCCS_OF_PREDS, c_next, c_in);
	    control_predecessors(c_next) = CONS(CONTROL, c_in, NIL) ;
	    pred = c_in;
	}
	else {
	    pred = (unreachable) ? 
		    make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL) :
		    c_res;
	}
	c_res = c_next ; 
    }
    ifdebug(1) {
	check_control_coherency(pred);
	check_control_coherency(succ);
	check_control_coherency(c_res);
	pips_debug(5, "(pred = %p, succ = %p, c_res = %p)\n",
		   pred, succ, c_res);
	pips_debug(5, "Nodes from pred %p\n", pred);
	display_linked_control_nodes(pred);
	pips_debug(5, "Nodes from succ %p\n", succ);
	display_linked_control_nodes(succ);
	pips_debug(5, "Nodes from c_res %p\n", c_res);
	display_linked_control_nodes(c_res);
    }
    
    return(gen_nreverse(ctls));
}

/* CONTROLIZE_LIST computes in C_RES the control graph of the list
   STS (of statement ST) with PREDecessor and SUCCessor. We try to
   minize the number of graphs by looking for graphs with one node
   only and picking the statement in that case.
   */
bool controlize_list(st, sts, pred, succ, c_res, used_labels)
statement st;
cons *sts;
control pred, succ;
control c_res;
hash_table used_labels;
{
    hash_table block_used_labels = hash_table_make(hash_string, 0);
    control c_block = control_undefined;
    control c_end = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
    control c_last = c_end;
    list ctls;
    bool controlized;

    pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	       st, pred, succ, c_res);
    ifdebug(1) {
	check_control_coherency(pred);
	check_control_coherency(succ);
	check_control_coherency(c_res);
    }

    if(ENDP(sts)) {
      list d = gen_copy_seq(statement_declarations(st));
      string dt
	= (statement_decls_text(st)==NULL || string_undefined_p(statement_decls_text(st))) ? 
	NULL
	: strdup(statement_decls_text(st));
      string ct = string_undefined_p(statement_comments(st))?
	string_undefined /* Should be empty_comment() ? */
	: strdup(statement_comments(st));

      c_block = make_control(make_empty_statement_with_declarations_and_comments(d, dt, ct),
			     NIL, NIL);
    }
    else {
      /* What happens to the declarations and comments attached to st? */
      c_block = make_conditional_control(STATEMENT(CAR(sts)));
    }
    
    ctls = controlize_list_1(sts, pred, c_end, c_block, block_used_labels);
    c_last = compact_list(ctls, c_end);
    /* To avoid compact list: c_last = c_end; */

    ifdebug(5) {
	pips_debug(0, "Nodes from c_block %p\n", c_block);
	display_linked_control_nodes(c_block);
	pips_debug(0, "Nodes from c_last %p\n", c_last);
	display_linked_control_nodes(c_last);
    }
    
    if(covers_labels_p(st,block_used_labels)) {
	/* There is no GOTO to/from  outside the statement list:
           hierarchize the control graph. */
	statement new_st; 

	/* Unlink the c_block from the unstructured. RK. */
	unlink_2_control_nodes(pred, c_block);
	unlink_2_control_nodes(c_block, c_end);

	if(ENDP(control_predecessors(c_block)) &&
	    ENDP(control_successors(c_block))) {
	    /* c_block is a lonely control node: */
	    new_st = control_statement(c_block);
	    control_statement(c_block) = statement_undefined;
	    free_control(c_block);

	    /* new_st must inherit the declarations of st, if not its comments! */
	    if(ENDP(statement_declarations(new_st))) {
	      statement_declarations(new_st) = gen_copy_seq(statement_declarations(st));
	      if(statement_decls_text(st)!=NULL) {
		statement_decls_text(new_st) = 
		  string_undefined_p(statement_decls_text(st))? 
		  string_undefined
		  : strdup(statement_decls_text(st));
	      }
	    }
	    else if(!ENDP(statement_declarations(st))) {
	      /* Both new_st and st carry declarations */
	      pips_assert("No variable is declared twice in st", gen_once_p(statement_declarations(st)));
	      pips_assert("No variable is declared twice in new_st", gen_once_p(statement_declarations(new_st)));
	      MAP(ENTITY, v, 
	      {if(!gen_in_list_p(v, statement_declarations(new_st))) {
		pips_debug(1, "Variable %s added to declarations of new_st\n", entity_name(v));
		statement_declarations(new_st)
		  = CONS(ENTITY, v, statement_declarations(new_st));}},
		  statement_declarations(st));
	      pips_assert("No variable is declared twice in resulting new_st", gen_once_p(statement_declarations(new_st)));
	      /* pips_internal_error("Declaration conflict in controlizer\n"); */
	    }
	}
	else {
	    /* The control is kept in an unstructured: */
	    unstructured u = make_unstructured(c_block, c_last);
	    instruction i =
		make_instruction(is_instruction_unstructured, u);

	    ifdebug(1) {
		check_control_coherency(unstructured_control(u));
		check_control_coherency(unstructured_exit(u));
	    }
	    new_st = make_statement(entity_empty_label(), 
				    statement_number(st),
				    STATEMENT_ORDERING_UNDEFINED,
				    statement_comments(st),
				    i,
				    statement_declarations(st),
				    statement_decls_text(st));
	}
	/* Not a good idea from mine to add this free... RK
	   free_statement(control_statement(c_res)); */
	control_statement(c_res) = new_st;
	link_2_control_nodes(pred, c_res);
	link_2_control_nodes(c_res, succ);
#if 0	
	UPDATE_CONTROL(c_res, new_st,
		        ADD_PRED(pred, c_res),
		        CONS(CONTROL, succ, NIL));
	control_predecessors(succ) = ADD_PRED(c_res, succ);
	add_proper_successor_to_predecessor(pred, c_res);
	/* control_successors(pred) = CONS(CONTROL, c_res, NIL); */
#endif
	controlized = FALSE;
    }
    else {
	/* There are GOTO to/from outside this statement list: update
           c_res to reflect c_block infact: */
	/* We alredy have pred linked to c_block and the exit node
           linked to succ. RK */
	UPDATE_CONTROL(c_res,
		        control_statement(c_block),
		        control_predecessors(c_block),
		        control_successors(c_block));
	control_predecessors(succ) = ADD_PRED(c_end, succ);
	control_successors(c_end) = CONS(CONTROL, succ, NIL);
	patch_references(PREDS_OF_SUCCS, c_block, c_res);
	patch_references(SUCCS_OF_PREDS, c_block, c_res);
	controlized = TRUE;
    }
    union_used_labels( used_labels, block_used_labels);
    
    hash_table_free(block_used_labels);

    pips_debug(5, "Exiting\n");
    ifdebug(1) {
	check_control_coherency(pred);
	check_control_coherency(succ);
	check_control_coherency(c_res);
    }
    
    return(controlized);
}
	
/* CONTROL_TEST builds the control node of a statement ST in C_RES which is a 
   test T. */

bool controlize_test(st, t, pred, succ, c_res, used_labels)
test t;
statement st;
control pred, succ;
control c_res;
hash_table used_labels;
{
  hash_table 
    t_used_labels = hash_table_make(hash_string, 0), 
    f_used_labels = hash_table_make(hash_string, 0);
  control c1 = make_conditional_control(test_true(t));
  control c2 = make_conditional_control(test_false(t));
  control c_join = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
  statement s_t = test_true(t);
  statement s_f = test_false(t);
  bool controlized;

  pips_debug(5, "(st = %p, pred = %p, succ = %p, c_res = %p)\n",
	     st, pred, succ, c_res);
    
  ifdebug(5) {
    pips_debug(1, "THEN at entry:\n");
    print_statement(s_t);
    pips_debug(1, "c1 at entry:\n");
    print_statement(control_statement(c1));
    pips_debug(1, "ELSE at entry:\n");
    print_statement(s_f);
    pips_debug(1, "c2 at entry:\n");
    print_statement(control_statement(c2));
    check_control_coherency(pred);
    check_control_coherency(succ);
    check_control_coherency(c_res);
  }

  controlize(s_t, c_res, c_join, c1, t_used_labels);	
  controlize(s_f, c_res, c_join, c2, f_used_labels);

  if(covers_labels_p(s_t, t_used_labels) && 
     covers_labels_p(s_f, f_used_labels)) {
    test it = make_test(test_condition(t), 
			control_statement(c1),
			control_statement(c2));

    UPDATE_CONTROL(c_res, 
		   make_statement(statement_label(st), 
				  statement_number(st),
				  STATEMENT_ORDERING_UNDEFINED,
				  statement_comments(st),
				  make_instruction(is_instruction_test, it),
				  statement_declarations(st),
				  statement_decls_text(st)),
		   ADD_PRED(pred, c_res),
		   CONS(CONTROL, succ, NIL));
    control_predecessors(succ) = ADD_PRED(c_res, succ);
    controlized = FALSE;
  }
  else {
    UPDATE_CONTROL(c_res, st, 
		   ADD_PRED(pred, c_res),
		   CONS(CONTROL, c1, CONS(CONTROL, c2, NIL)));
    test_true(t) = MAKE_CONTINUE_STATEMENT();
    test_false(t) = MAKE_CONTINUE_STATEMENT();
    control_predecessors(succ) = ADD_PRED(c_join, succ);
    control_successors(c_join) = CONS(CONTROL, succ, NIL);
    controlized = TRUE;
  }

  /* Be very careful when chaining c_res as successor of pred: if pred is
     associated to a test, the position of c_res as first or second
     successor of c_res is unknown. It should have been set by the
     caller.

     You might survive, using the fact that the TRUE branch has been
     processed first because of the order of the two recursive calls to
     controlize(). The FALSE branch whill be linked as second
     successor. But another controlize_xxx might have decided to link pred
     and c_res before calling controlize() and controlize_test(). Too much
     guessing IMHO (FI). */

  /* control_successors(pred) = ADD_SUCC(c_res, pred); */
  add_proper_successor_to_predecessor(pred, c_res);

  union_used_labels(used_labels, 
		    union_used_labels(t_used_labels, f_used_labels));

  hash_table_free(t_used_labels);
  hash_table_free(f_used_labels);

  ifdebug(5) {
    pips_debug(1, "IF at exit:\n");
    print_statement(st);
    display_linked_control_nodes(c_res);
    check_control_coherency(pred);
    check_control_coherency(succ);
    check_control_coherency(c_res);
  }    
  pips_debug(5, "Exiting\n");
    
  return(controlized);
}

/* INIT_LABEL puts the reference in the statement ST to the label NAME
   int the Label_statements table and allocate a slot in the Label_control
   table. */

void init_label(name, st ) 
string name;
statement st;
{
    if(!empty_label_p(name)) {
	list used = (list) hash_get_default_empty_list(Label_statements, name);
	list sts = CONS(STATEMENT, st, used);
	/* Just append st to the list of statements pointing to
	   this label. */
	if (hash_defined_p(Label_statements, name))
	    hash_update(Label_statements, name, (char *) sts);
	else
	    hash_put(Label_statements, name, (char *) sts);

	if (! hash_defined_p(Label_control, name)) {
	    statement new_st = make_continue_statement(statement_label(st)) ;
	    control c = make_control( new_st, NIL, NIL);
	    
	    hash_put(Label_control, name, (char *)c);
	}
    }
}

/* CREATE_STATEMENTS_OF_LABELS gathers in the Label_statements table all
   the references to the useful label of the statement ST. Note that for
   loops, the label in the DO statement is NOT introduced. Label_control is
   also created. */

void create_statements_of_label(st)
statement st;
{
    string name = entity_name(statement_label(st));
    instruction i;

    init_label(name, st);

    switch(instruction_tag(i = statement_instruction(st))) {
    case is_instruction_goto: {
	string where = entity_name(statement_label(instruction_goto(i)));

	init_label(where, st);
	break;
    }
    case is_instruction_unstructured:
	pips_error("create_statement_of_labels", "Found unstructured", "");
    }
}

void create_statements_of_labels(st)
statement st ;
{
    gen_recurse(st, 
		statement_domain,
		gen_true,
		create_statements_of_label);
}
	

/* SIMPLIFIED_UNSTRUCTURED tries to get rid of top-level and useless
   unstructure nodes.

   top is the entry node, bottom the exit node. result is ? (see
   assert below... Looks like it is the second node. RK)

   All the comments below are from RK who is reverse engineering all
   that stuff... :-(

   Looks like there are many memory leaks. */
static unstructured
simplified_unstructured(control top,
			control bottom,
			control res)
{
    list succs;
    statement st;
    unstructured u;
    instruction i;

    ifdebug(4) {
	pips_debug(0, "Accessible nodes from top:\n");
	display_linked_control_nodes(top);
	check_control_coherency(top);
	pips_debug(1, "Accessible nodes from bottom:\n");
	display_linked_control_nodes(bottom);
	check_control_coherency(bottom);
	pips_debug(1, "Accessible nodes from res:\n");
	display_linked_control_nodes(res);
	check_control_coherency(res);
    }
    
    u = make_unstructured(top, bottom);

    if(!ENDP(control_predecessors(top))) {
	/* There are goto on the entry node: */
	return(u);
    }
    
    if(gen_length(succs=control_successors(top)) != 1) {
	/* The entry node is not a simple node sequence: */
	return(u);
    }
    
    pips_assert("simplify_control", CONTROL(CAR(succs)) == res);
    
    if(gen_length(control_predecessors(res)) != 1) {
	/* The second node has more than 1 goto on it: */
	return(u);
    }
    
    if(gen_length(succs=control_successors(res)) != 1) {
	/* Second node is not a simple node sequence: */
	return(u);
    }
    
    if(CONTROL(CAR(succs)) != bottom) {
	/* The third node is not the exit node: */
	return(u);
    }

    if(gen_length(control_predecessors(bottom)) != 1) {
	/* The exit node has more than 1 goto on it: */
	return(u);
    }
    
    if(!ENDP(control_successors(bottom))) {
	/* The exit node has a successor: */
	return(u);
    }

    /* Here we have a sequence of 3 control node: top, res and
       bottom. */
    control_predecessors(res) = control_successors(res) = NIL;
    st = control_statement(res);

    if(instruction_unstructured_p(i=statement_instruction(st))) {
	/* If the second node is an unstructured, just return it
           instead of top and bottom: (??? Lot of assumptions. RK) */
	return(instruction_unstructured(i));
    }

    /* Just keep the second node as an unstructured with only 1
       control node: */
    unstructured_control(u) = unstructured_exit(u) = res;
    return(u);
}


/* CONTROL_GRAPH returns the control graph of the statement ST. */

unstructured control_graph(st) 
statement st;
{
    control result, top, bottom;
    hash_table used_labels = hash_table_make(hash_string, 0);
    unstructured u;

    ifdebug(1) {
	pips_assert("Statement should be OK.", statement_consistent_p(st));
	set_bool_property("PRETTYPRINT_BLOCKS", TRUE);
	set_bool_property("PRETTYPRINT_EMPTY_BLOCKS", TRUE);
    }

    /* Since the controlizer does not seem to accept GOTO inside
       sequence from outside but it appears in the code with
       READ/WRITE with I/O exceptions (end=, etc), first remove
       useless blocks. RK */
    clean_up_sequences(st);

    Label_statements = hash_table_make(hash_string, LABEL_TABLES_SIZE);
    Label_control = hash_table_make(hash_string, LABEL_TABLES_SIZE);
    create_statements_of_labels(st);

    result = make_conditional_control(st);
    top = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
    bottom = make_control(MAKE_CONTINUE_STATEMENT(), NIL, NIL);
    Unreachable = NIL;
    controlize(st, top, bottom, result, used_labels);

    if(!ENDP(Unreachable)) {
	pips_user_warning("Some statements are unreachable\n");
	ifdebug(2) {
	    pips_debug(0, "Unreachable statements:\n");
	    MAP(STATEMENT, s, {
		pips_debug(0, "Statement %p:\n", s);
		print_statement(s);
	    }, Unreachable);
	}
    }
    hash_table_free(Label_statements);
    hash_table_free(Label_control);
    hash_table_free(used_labels);

    u = simplified_unstructured(top, bottom, result);

    if( get_debug_level() > 5) {
	pips_debug(1, 
	  "Nodes in unstructured %p (entry %p, exit %p) from entry:\n",
		   u, unstructured_control(u), unstructured_exit(u));
	display_linked_control_nodes(unstructured_control(u));
	pips_debug(1, "Accessible nodes from exit:\n");
	display_linked_control_nodes(unstructured_exit(u));
    }

    reset_unstructured_number();
    unstructured_reorder(u);

    ifdebug(1) {
	check_control_coherency(unstructured_control(u));
	check_control_coherency(unstructured_exit(u));
	pips_assert("Unstructured should be OK.", unstructured_consistent_p(u));
    }

    return(u);
}
