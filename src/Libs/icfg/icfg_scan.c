/* 
   icfg_scan.c
   module_to_icfg(0, mod) recursively to_icfgs module "mod" and its callees
   and writes its icfg in indented form
*/
#include <stdio.h>
#include <string.h>

#include "genC.h"
#include "ri.h"
#include "control.h"      /* CONTROL_MAP is defined there */
#include "text.h"
#include "text-util.h"
#include "ri-util.h"
#include "properties.h"  /* get_bool_property */
#include "misc.h"
#include "database.h"
#include "effects-generic.h"
#include "effects-simple.h"
#include "effects-convex.h"
#include "resources.h"
#include "semantics.h"
#include "prettyprint.h"
#include "complexity_ri.h"
#include "complexity.h"
#include "pipsdbm.h"      /* DB_PUT_FILE_RESOURCE is defined there */
#include "text-util.h"
#include "icfg.h"

#define ICFG_SCAN_INDENT 4
#define MAX_LINE_LENGTH 256 /* hmmm... (never checked I guess:-) */

static int current_margin;

#define st_DO	 "do"
#define st_ENDDO "enddo"
#define st_IF	 "if"
#define st_THEN	 "then"
#define st_ELSE	 "else"
#define st_ENDIF "endif"
#define st_WHILE "while"
#define st_ENDWHILE "endwhile"

#define some_text_p(t) (t!=text_undefined && text_sentences(t)!=NIL)

/* We want to keep track of the current statement inside the recurse */
DEFINE_LOCAL_STACK(current_stmt, statement)

/* We store the text for each statement in a mapping during a code traversal
   in order to print it afterwards 
*/
GENERIC_LOCAL_MAPPING(icfg, text, statement)

/* static drivers
 */

static bool 
    print_do_loops = FALSE, 
    print_ifs = FALSE;
static text (*decoration)(string) = NULL;

static void 
append_icfg_file(text t, string module_name)
{
    string filename, localfilename, dir;
    FILE *f_called;
    string buf;
    char textbuf[MAX_LINE_LENGTH]; /* argh... */
       
    localfilename = db_get_memory_resource(DBR_ICFG_FILE, module_name, TRUE);
    dir = db_get_current_workspace_directory();
    filename = strdup(concatenate(dir, "/", localfilename, 0));
    free(dir);
    
    pips_debug (2, "Inserting ICFG for module %s\n", module_name);

    /* Get the Icfg from the callee */
    f_called = safe_fopen (filename, "r");

    while ((buf=safe_readline(f_called))) {
	/* add sentences ... */
        pips_assert("static buf length", 
                    current_margin+strlen(buf)+1<MAX_LINE_LENGTH);
	sprintf(textbuf, "%*s%s\n", current_margin ,"",buf);
        free(buf);
	ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
					      strdup(textbuf)));
    }
    
    /* push resulting text */
    safe_fclose (f_called, filename);
    free(filename); 
}

/* STATEMENT
 */
static bool 
statement_filter(statement s)
{
    bool res = TRUE;
    text t = make_text (NIL);

    pips_debug (5,"going down\n");

    store_statement_icfg (s, t);
    res = current_stmt_filter (s);
    return res;
}

static void 
statement_rewrite(statement s)
{
    pips_debug (5,"going up\n");
    ifdebug(9) print_text(stderr,(text) load_statement_icfg (s));

    current_stmt_rewrite (s);
    return;
}

/* CALL
 */
static void 
call_filter(call c)
{
    entity e_callee = call_function(c);
    string callee_name = module_local_name(e_callee);

    /* current_stmt_head() */
    pips_debug (5,"called entity is %s\n", entity_name(e_callee));

    /* If this is a "real function" (defined in the code elsewhere) */
    if (value_code_p(entity_initial(e_callee))) {
	text r = (text) load_statement_icfg (current_stmt_head());

	/* hum... pushes the current entity... */
	entity e_caller = get_current_module_entity();
	reset_current_module_entity();

	switch (get_int_property (ICFG_DECOR)) {
	case ICFG_DECOR_NONE:
	    break;
	case ICFG_DECOR_COMPLEXITIES:
	    MERGE_TEXTS(r,get_text_complexities(callee_name));
	    break;
	case ICFG_DECOR_TRANSFORMERS:
	    MERGE_TEXTS(r,get_text_transformers(callee_name));
	    break;
	case ICFG_DECOR_PRECONDITIONS:
	    MERGE_TEXTS(r, call_site_to_module_precondition_text
			(e_caller, e_callee, current_stmt_head(), c));
	    break;
	case ICFG_DECOR_PROPER_EFFECTS:
	    MERGE_TEXTS(r,get_text_proper_effects(callee_name));
	    break;
	case ICFG_DECOR_CUMULATED_EFFECTS:
	    MERGE_TEXTS(r,get_text_cumulated_effects(callee_name));
	    break;
	case ICFG_DECOR_REGIONS:
	    MERGE_TEXTS(r,get_text_regions(callee_name));
	    break;
	case ICFG_DECOR_IN_REGIONS:
	    MERGE_TEXTS(r,get_text_in_regions(callee_name));
	    break;
	case ICFG_DECOR_OUT_REGIONS:
	    MERGE_TEXTS(r,get_text_out_regions(callee_name));
	    break;
	default:
	    pips_error("module_to_icfg",
		       "unknown ICFG decoration for module %s\n",
		       callee_name);
	}
	/* retrieve the caller entity */
	set_current_module_entity(e_caller);
	/* append the callee' icfg */
	append_icfg_file (r, callee_name);
	/* store it to the statement mapping */
	update_statement_icfg (current_stmt_head(), r);
    }
    return;
}

/* LOOP
 */
static bool 
loop_filter (loop l)
{
    pips_debug (5, "Loop begin\n");
    if (print_do_loops) current_margin += ICFG_SCAN_INDENT;
    return TRUE;
}

static void 
loop_rewrite (loop l)
{
    text inside_the_loop = text_undefined;
    text inside_the_do = text_undefined;
    bool text_in_do_p, text_in_loop_p;
    text t = make_text (NIL);
    char textbuf[MAX_LINE_LENGTH];

    pips_debug (5,"Loop end\n");

    if (print_do_loops) current_margin -= ICFG_SCAN_INDENT;

    inside_the_do = (text) load_statement_icfg (current_stmt_head());
    text_in_do_p = some_text_p(inside_the_do);

    inside_the_loop = (text) load_statement_icfg (loop_body (l));
    text_in_loop_p = some_text_p(inside_the_loop);

    /* Print the text inside do expressions (before the DO!)
     */
    if (text_in_do_p) 
    {
	pips_debug(9, "something inside_the_do\n");
	MERGE_TEXTS (t, inside_the_do);
    }

    /* Print the DO 
     */
    if ((text_in_loop_p || text_in_do_p) && print_do_loops) 
    {
	sprintf(textbuf, "%*s" st_DO " %s\n", current_margin, "",
		entity_local_name(loop_index(l)));
	ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
					      strdup(textbuf)));
    }

    /* Print the text inside the loop
     */
    if (text_in_loop_p) {
	pips_debug(9, "something inside_the_loop\n");
	MERGE_TEXTS (t, inside_the_loop);
    }

    /* Print the ENDDO 
     */
    if ((text_in_loop_p || text_in_do_p) && print_do_loops) 
    {
	sprintf(textbuf, "%*s" st_ENDDO "\n", current_margin, "");
	ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
					      strdup(textbuf)));
    }

    /* store it to the statement mapping */
    update_statement_icfg (current_stmt_head(), t);
    
    return ;
}

/* INSTRUCTION
 */
static bool
instruction_filter (instruction i)
{
    if(instruction_unstructured_p(i)
       && unstructured_while_p(instruction_unstructured(i))) {
	pips_debug (5, "While begin\n");
	if (print_do_loops) current_margin += ICFG_SCAN_INDENT;
    }
    return TRUE;
}

static void 
instruction_rewrite (instruction i)
{
    text t = make_text (NIL);
    char textbuf[MAX_LINE_LENGTH];
    bool text_in_unstructured_p = FALSE;

    pips_debug (5,"going up\n");
    pips_debug (9,"instruction tag = %d\n", instruction_tag (i));

    switch (instruction_tag (i)) {
    case is_instruction_block:
    {
	pips_debug (5,"dealing with a block, appending texts\n");

	MAP(STATEMENT, s, 
	    MERGE_TEXTS(t, load_statement_icfg(s)),
	    instruction_block(i));

	/* store it to the statement mapping */
	update_statement_icfg (current_stmt_head (), t);
	break;
    }
    case is_instruction_unstructured:
    {
	unstructured u = instruction_unstructured (i);
	list blocs = NIL ;
	control ct = unstructured_control(u) ;
	text inside_the_unstructured = make_text(NIL);
	bool while_p = FALSE;

	pips_debug (5,"dealing with an unstructured, appending texts\n");

	if(unstructured_while_p(u)) {
	    while_p = TRUE;
	    pips_debug (5,"dealing with a WHILE\n");
	}

	/* SHARING! Every statement gets a pointer to the same precondition!
	   I do not know if it's good or not but beware the bugs!!! */
	FORWARD_CONTROL_MAP(c, {
	    statement st = control_statement(c) ;
	    MERGE_TEXTS(inside_the_unstructured, load_statement_icfg (st));
	}, ct, blocs) ;
	
	gen_free_list(blocs) ;

	text_in_unstructured_p = some_text_p(inside_the_unstructured);

	/* Print the WHILE
	 */
	if(print_do_loops && while_p) {
	    current_margin -= ICFG_SCAN_INDENT;
	    if(text_in_unstructured_p) {
		sprintf(textbuf, "%*s" st_WHILE "\n", current_margin, "");
		ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
						      strdup(textbuf)));
	    }
	}

	/* Print the text inside the unstructured (possibly the while body)
	 */
	if(text_in_unstructured_p) {
	    pips_debug(9, "something inside_the_loop\n");
	    MERGE_TEXTS (t, inside_the_unstructured);
	}

	/* Print the ENDDWHILE
	 */
	if (text_in_unstructured_p && print_do_loops && while_p) 
	{
	    sprintf(textbuf, "%*s" st_ENDWHILE "\n", current_margin, "");
	    ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
						  strdup(textbuf)));
	}

	update_statement_icfg (current_stmt_head (), t);
	break;
    }
    }
}

/* RANGE
 * functions to avoid the indentation when dealing with DO expressions.
 */
static bool 
range_filter(range r)
{
    statement s = current_stmt_head();

    if (statement_loop_p(s) && loop_range(statement_loop(s)) && print_do_loops)
       current_margin -= ICFG_SCAN_INDENT;
    return TRUE;
}

static void 
range_rewrite(range r)
{
    statement s = current_stmt_head();

    if (statement_loop_p(s) && loop_range(statement_loop(s)) && print_do_loops)
        current_margin += ICFG_SCAN_INDENT;
}

/* TEST
 */
static bool 
test_filter (test l)
{
    pips_debug (5, "Test begin\n");
    if (print_ifs) current_margin += ICFG_SCAN_INDENT;
    return TRUE;
}

static void 
test_rewrite (test l)
{
    text inside_then = text_undefined;
    text inside_else = text_undefined;
    text inside_if = text_undefined;
    text t = make_text (NIL);
    char textbuf[MAX_LINE_LENGTH];
    bool something_to_print;
    
    pips_debug (5,"Test end\n");
    
    inside_if = copy_text((text) load_statement_icfg (current_stmt_head ()));
    inside_then = copy_text((text) load_statement_icfg (test_true (l)));
    inside_else = copy_text((text) load_statement_icfg (test_false (l)));

    something_to_print = (some_text_p(inside_else) ||
			  some_text_p(inside_then) ||
			  some_text_p(inside_if));
    
    if (print_ifs) current_margin -= ICFG_SCAN_INDENT;
    
    /* Print the IF */
    if (something_to_print && print_ifs) {
	sprintf(textbuf, "%*s" st_IF "\n", current_margin, "");
	ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
					      strdup(textbuf)));
    }
    
    /* print things in the if expression*/
    if (some_text_p(inside_if))
	MERGE_TEXTS (t, inside_if);

    
    /* print then statements */
    if (some_text_p(inside_then)) {
	/* Print the THEN */
	if (something_to_print && print_ifs) {
	    sprintf(textbuf, "%*s" st_THEN "\n", current_margin, "");
	    ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
						  strdup(textbuf)));
	}
	MERGE_TEXTS (t, inside_then);
    }    

    /* print then statements */
    if (some_text_p(inside_else)){
	/* Print the ELSE */
	if (something_to_print && print_ifs) {
	    sprintf(textbuf, "%*s" st_ELSE "\n", current_margin, "");
	    ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
						  strdup(textbuf)));
	}
	MERGE_TEXTS (t, inside_else);
    }    

    /* Print the ENDIF */
    if (something_to_print && print_ifs) {
	sprintf(textbuf, "%*s" st_ENDIF "\n", current_margin, "");
	ADD_SENTENCE_TO_TEXT(t, make_sentence(is_sentence_formatted,
					      strdup(textbuf)));
    }
    
    /* store it to the statement mapping */
    update_statement_icfg (current_stmt_head(), t);
    
    return;
}

void 
print_module_icfg(entity module)
{
    string module_name = module_local_name(module);
    statement s =(statement)db_get_memory_resource(DBR_CODE,module_name,TRUE);
    text txt = make_text (NIL);
    char buf[MAX_LINE_LENGTH];

    set_current_module_entity (module);

    /* allocate the mapping  */
    make_icfg_map();
    make_current_stmt_stack();

    sprintf(buf,"%s\n",module_name);
    ADD_SENTENCE_TO_TEXT(txt, make_sentence(is_sentence_formatted,
					    strdup(buf)));

    current_margin = ICFG_SCAN_INDENT;

    print_do_loops = get_bool_property(ICFG_DOs);
    print_ifs = get_bool_property(ICFG_IFs);

    gen_multi_recurse
	(s,
	 statement_domain, statement_filter, statement_rewrite,
	 call_domain     , call_filter     , gen_null,
	 loop_domain     , loop_filter     , loop_rewrite,
	 instruction_domain    , instruction_filter  , instruction_rewrite,
	 test_domain     , test_filter     , test_rewrite,
         range_domain    , range_filter    , range_rewrite,
	 NULL);

    pips_assert("stack is empty", current_stmt_empty_p());

    MERGE_TEXTS (txt, (text) load_statement_icfg (s));

    make_text_resource(module_name, DBR_ICFG_FILE,
		       get_bool_property(ICFG_IFs) ? ".icfgc" :
		       (get_bool_property(ICFG_DOs) ? ".icfgl" : 
			".icfg"),
		       txt);
    
    free_text (txt);
    free_icfg_map();
    free_current_stmt_stack();
    reset_current_module_entity();
}

/* parametrized by the decoration...
 */
void 
print_module_icfg_with_decoration(
    entity module,
    text (*deco)(string))
{
    decoration = deco;
    print_module_icfg(module);
}
