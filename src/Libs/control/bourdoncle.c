/* bourdoncle.c

   Decomposition of a CFG into SCC's and sub-SCC's. Algorithm 3.9, Page
   43, Francois Bourdoncle's PhD. Define heuristically SCC and sub-SCC
   heads.

   Build a set of new data structures to represent any CFG as a recursive
   structure of CFG's (scc_map) and one parent DAG (ndu) and one node
   traversal order (partition list) and a correspondance between the new
   nodes and the original nodes (ancestor_map).

   Some predecessors or successors must be deleted in the recursive
   descent. These vertices are given a statement_undefined statement and
   empty lists of predecessors and successors. Empty successors must be
   maintained so as to know if they are true or false successors. I
   suppose that we do not really care about irrelevant predecessors and
   that we might as well delete them. In fact, only the true branch
   successor absolutely must be preserved.

   Since the new CFG's are not deterministic, the number of successors is
   not bounded to two and any statement can have more than one
   successor. Standard consistency check are likely to fail if applied to
   the unstructured produced by this decomposition.

   When a node pointing to a scc is replicated, two options are
   available. Either all copies point to a unique scc (initial
   implementation) or each control copy points towards its own copy of the
   scc. In the initial implementation and first option, only ancestor
   control nodes can point to a scc, and ancestor_map is used to reach the
   scc_associated to a node. In the second case, any control node,
   replicated or not, can point to a scc. The first option makes sense
   when transformers are not context dependent: all identical cycles can
   be analyzed as one cycle, although I do not understand the miracle for
   the preconditions, since they clearly are context-sensitive. The second
   option is useful when transformers are computed in context, since all
   replicates of a cycle can operate in diferrent contexts. This is
   performed as a post-processing step conditional to property
   SEMANTICS_COMPUTE_TRANSFORMERS_IN_CONTEXT.

   Structure of the code:
     bourdoncle_partition()
       bourdoncle_visit()
         bourdoncle_component()
           bourdoncle_visit() recursively
           update_partition()
           scc_to_dag() [Not part of Bourdoncle's algorithm: transforms a scc 
	                 into a non-deterministic while loop]
       replicate_cycles() [Not part of Bourdoncle's algorithm: allocate a copy
                           of each scc, i.e. cycle, i.e. while loop for each 
			   of its "call" sites]

 $Id$

 $Log: bourdoncle.c,v $
 Revision 1.15  2005/08/24 15:33:46  irigoin
 Function replicate_cycles() added to cope with context-dependent
 transformers. Lots of comments added. Some debugging statement. A few
 auxiliary functions added to cope with the two different modes: the basic
 mode without cycle replication where cycles are identified by the ancestor
 common to all the cycle call sites, and the context sensitive mode where
 cycles are identified by their head or their unique call site node.

 Revision 1.14  2003/12/20 17:42:05  irigoin
 Bug fix in clean_up_embedding_graph(). Iteration over a list whose
 elements might be freed by a previous iteration. Why didn't it show up
 earlier?

 Revision 1.13  2003/08/02 13:34:49  irigoin
 Four useless/obsolete static functions removed to reduce gcc warnings.

 Revision 1.12  2003/07/23 14:29:01  irigoin
 Bug fix in bourdoncle_unstructured_free()

 Revision 1.11  2003/07/23 13:50:02  irigoin
 Bug fix in bourdoncle_unstructured_free()

 Revision 1.10  2003/07/16 15:39:54  irigoin
 New debugging statements to locate a bug revealed by unstruc12.f and
 unstruc12b.f. The bug turned out to be in gen_position(), a newgen
 function from list.c

 Revision 1.9  2003/07/11 09:45:05  irigoin
 bourdoncle_free() added, some presentation imrpovements

 Revision 1.8  2003/06/19 08:25:58  irigoin
 Improvements of debugging information

 Revision 1.7  2002/07/22 17:12:26  irigoin
 A few bug fixes, assertions added, plus support functions added for
 semantics/unstructured.c. Memory cleanup function still missing.

 Revision 1.6  2002/07/09 14:55:06  irigoin
 Version debugged with card.f and readin.f in SemanticsPrivate.  Profile of
 bourdoncle_partition() modified to return more information.

 Revision 1.5  2002/07/05 12:53:10  irigoin
 Intermediate version with bug in update_successor_in_copy() for SemanticsPrivate/readin.f

 Revision 1.4  2002/07/02 17:50:48  irigoin
 Intermediate bugged version, checked in to keep track of the current state.

 Revision 1.3  2002/06/05 14:42:02  irigoin
 First good restructuring for semantics/Validation/unstruc10.f

 Revision 1.2  2002/06/04 16:07:04  irigoin
 Lots of new development. Still not working recursively. Only the deepest
 cycle in unstruc10.f is handled.

 Revision 1.1  2002/05/28 15:04:03  irigoin
 Initial revision */

#include <stdio.h>
#include <strings.h>

#include "linear.h"

#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "control.h"
#include "properties.h"

#include "misc.h"

/* #include "constants.h" */


/* Data structures for Bourdoncle's heuristics:
 *
 * dfn = depth first number
 *
 * num = current vertex number
 *
 * vertex_stack = stack of visited nodes
 *
 */

static hash_table dfn = hash_table_undefined;

static void reset_dfn(control c)
{
  hash_put(dfn, (void *) c, (void *) 0);
}

static void copy_dfn(control new_c, control old_c)
{
  int d = 0;
  
  if((d = (int) hash_get(dfn, (void *) old_c)) == (int) (HASH_UNDEFINED_VALUE))
    pips_internal_error("No dfn value for control %p\n", old_c);
  
  hash_put(dfn, (void *) new_c, (void *) d);
}

static int get_dfn(control c)
{
  int d = 0;
  
  if((d = (int) hash_get(dfn, (void *) c)) == (int) (HASH_UNDEFINED_VALUE))
    pips_internal_error("No dfn value for control %p\n", c);

  return d;
}

static void update_dfn(control c, int d)
{
  hash_update(dfn, (void *) c, (void *) d);
}


static int num = 0;

DEFINE_LOCAL_STACK(vertex, control)


     /* With non-deterministic graph, the outcome of a test may be known
        in advance. Never taken branches are filled in with meaningless
        nodes. */

static control make_meaningless_control(list preds, list succs)
{
  control c =  make_control(statement_undefined, preds, succs);
  pips_assert("A meaningless control has no successors", ENDP(succs));
  return c;
}

/* Is exported to exploit non-deterministic control flow graphs */
bool meaningless_control_p(control c)
{
  return statement_undefined_p(control_statement(c));
}

static void free_meaningless_control(control c)
{
  /* free_control() cannot be used because you do not want to free the
     successors and the predecessors */
  pips_assert("No statement is associated", statement_undefined_p(control_statement(c)));
  gen_free_list(control_predecessors(c));
  gen_free_list(control_successors(c));
  control_predecessors(c)= list_undefined;
  control_successors(c)= list_undefined;
  free_control(c);
}


/* Check that a node is used as a test in a CFG. The associated statement
   must be a test, the number of successors must be two and the TRUE and
   FALSE branches must be empty. Exported for user of ND CFG's. */
bool control_test_p(control c)
{
  bool test_p = FALSE;
  if(!meaningless_control_p(c)) {
    if(statement_test_p(control_statement(c))) {
      if(gen_length(control_successors(c))>=2) {
	test t = statement_test(control_statement(c));
	statement ts = test_true(t);
	statement fs = test_false(t);
	test_p = (empty_statement_or_labelless_continue_p(ts)
	  && empty_statement_or_labelless_continue_p(fs));
      }
    }
  }
  return test_p;
}


     /* Code partly replicated from semantics/unstructured.c to be able to
        taylor it to the exact needs. */

static bool check_control_statement(control c)
{
  static void print_control_node_without_check(control);
  
  if(!meaningless_control_p(c)) {
    if(statement_test_p(control_statement(c))) {
      if(gen_length(control_successors(c))==2) {
	/* The TRUE and FALSE branches should be empty most of the
           time. But a structured test used a non-deterministic node might
           very well have two successors or more. The assert below is too
           strong. */
	/*
	test t = statement_test(control_statement(c));
	statement ts = test_true(t);
	statement fs = test_false(t);
	*/
	/* Since some test statements are left structured and since
           structure and unstructured test statements may be
           non-deterministic here, nothing can be said. */
	/*
	pips_assert("The TRUE and FALSE branches must be empty",
		    empty_statement_or_labelless_continue_p(ts)
		    && empty_statement_or_labelless_continue_p(fs));
	*/
	;
      }
      else {
	/* Non-deterministic environment */
	/*
	pips_assert("A structured test has one successor at most",
		    gen_length(control_successors(c))<2);
	*/
	;
      }
    }
    else {
      /* Non-deterministic environment */
      /*
      pips_assert("A non-test has at most one successor",
		  gen_length(control_successors(c))<2);
      */
      ;
    }
  }
  else {
    ifdebug(4) {
      /* A meaningless control can be shared in intermediate states */
      }
    else ifdebug(1){
    /* It should not be shared, not only because memory management would
       be impossible locally but also because gen_recurse() would follow
       forbidden paths. */
    pips_assert("A meaningless control has one and ony one predecessor",
		gen_length(control_predecessors(c))==1);
    pips_assert("A meaningless control has no successor",
		gen_length(control_successors(c))==0);
    }

  }

  MAP(CONTROL, pred,
  {
    if(!gen_in_list_p(c, control_successors(pred))) {
      print_control_node_without_check(pred);
      pips_assert("c is a successor of each of its predecessors", FALSE);
    }
  }
      , control_predecessors(c));

  MAP(CONTROL, succ,
  {
    if(!gen_in_list_p(c, control_predecessors(succ))) {
      print_control_node_without_check(succ);
      pips_assert("c is a predecessor of each of its successors", FALSE);
    }
  }
      , control_successors(c));
  return TRUE;
}

static void print_and_check_control_node(control c, bool check_p)
{
  fprintf(stderr,
	  "ctr %p, %d preds, %d succs: %s", 
          c,
	  gen_length(control_predecessors(c)),
	  gen_length(control_successors(c)),
	  safe_statement_identification(control_statement(c)));
  fprintf(stderr,"\tsuccessors:\n");
  MAP(CONTROL, s, {
    fprintf(stderr, "\t\t%p %s", s,
	    safe_statement_identification(control_statement(s)));
  }, control_successors(c));
  fprintf(stderr,"\tpredecessors:\n");
  MAP(CONTROL, p, {
    fprintf(stderr, "\t\t%p %s", p,
	    safe_statement_identification(control_statement(p)));
  }, control_predecessors(c));
  fprintf(stderr, "\n");
  if(check_p)
    (void) check_control_statement(c);
}

static void print_control_node(control c)
{
  print_and_check_control_node(c, TRUE);
}

static void print_control_node_without_check(control c)
{
  print_and_check_control_node(c, FALSE);
}

static void print_control_nodes(list l)
{
  MAP(CONTROL, c, {
    fprintf(stderr, "%p, %s", c,
	    safe_statement_identification(control_statement(c)));
    (void) check_control_statement(c);
  }, l);
  fprintf(stderr, "\n");
}

static void print_control_nodes_without_check(list l)
{
  MAP(CONTROL, c, {
    fprintf(stderr, "%p, %s", c,
	    safe_statement_identification(control_statement(c)));
  }, l);
  fprintf(stderr, "\n");
}

static void print_unstructured(unstructured u)
{
  pips_debug(2, "Unstructured %p with nodes:\n", u);
  /* Do not go down into nested unstructured */
  gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, print_control_node, NULL);
  pips_debug(2, "With entry nodes\n");
  print_control_node(unstructured_control(u));
  pips_debug(2, "And exit node\n");
  print_control_node(unstructured_exit(u));
}

/* Output CFG in daVinci format */
static void davinci_print_control_node(control c, FILE * f,
				       bool entry_p,
				       bool exit_p,
				       bool fix_point_p)
{
  char attributes[256];
  bool arc_kind = TRUE;
  
  /* The same node can be an entry and an exit node. */
  attributes[0] = '\000';
  if(entry_p) {
    (void) strcpy(&attributes[0], ",a(\"BORDER\",\"double\")");
  }
  if(exit_p){
    (void) strcat(&attributes[0], ",a(\"COLOR\",\"red\")");
  }
  if(fix_point_p){
    (void) strcat(&attributes[0], ",a(\"FONTSTYLE\",\"bold_italic\")");
  }
  
  if(meaningless_control_p(c)) {
    fprintf(f, "l(\"%p\",n(\"\",[a(\"_GO\",\"text\"),a(\"OBJECT\",\"%p\")],\n\t[\n", c, c);
  }
  else if(control_test_p(c)) {
    int so = statement_ordering(control_statement(c));
    fprintf(f,
	    "l(\"%p\",n(\"\",[a(\"_GO\",\"rhombus\"),a(\"OBJECT\",\"%p\\n(%d,%d)\")%s],\n\t[\n",
	    c, c,
	    ORDERING_NUMBER(so), ORDERING_STATEMENT(so), attributes);
  }
  else {
    int so = statement_ordering(control_statement(c));
    fprintf(f, "l(\"%p\",n(\"\",[a(\"OBJECT\",\"%p\\n(%d,%d)\")%s],\n\t[\n", c, c,
	    ORDERING_NUMBER(so), ORDERING_STATEMENT(so), attributes);
  }
  
  /* Declare arcs */
  MAPL(c_s, {
    control s = CONTROL(CAR(c_s));

    if(control_test_p(c)) {
    (void) strcpy(&attributes[0], arc_kind? "green":"red");
      fprintf(f, "\tl(\"%p->%p\",e(\"\",[a(\"EDGECOLOR\",\"%s\")],r(\"%p\")))",
	      c, s, attributes, s);
    }
    else {
      fprintf(f, "\tl(\"%p->%p\",e(\"\",[],r(\"%p\")))", c, s, s);
    }
    
    fprintf(f, "%s\n", ENDP(CDR(c_s))? "" : ",");
    arc_kind = !arc_kind;
  }, control_successors(c));

  fprintf(f,"\t]))");

  (void) check_control_statement(c);
}

static int control_cons_compare(list l1, list l2)
{
  int diff = 0;
  control c1 = CONTROL(CAR(l1));
  control c2 = CONTROL(CAR(l2));

  if(meaningless_control_p(c1)) {
    diff = 0;
  }
  else if(meaningless_control_p(c2)) {
    diff = 0;
  }
  else {
    statement s1 = control_statement(c1);
    statement s2 = control_statement(c2);
    
    diff = statement_ordering(s1) - statement_ordering(s2);
  }
  return diff;
}

/* This counter is pre-incremented each time a new graph is stored to
   generate a new file name. */
static int davinci_count = 0;

static void set_davinci_count()
{
  davinci_count = 0;
}


static void davinci_print_control_nodes(list l, string msg)
{
  extern string itoa(int);
  extern string db_get_current_workspace_directory(void);
  string dn = db_get_current_workspace_directory();
  FILE * f = safe_fopen(concatenate(dn, "/",
				    get_current_module_name(), "/", 
				    get_current_module_name(),
				    "-",
				    itoa(++davinci_count),
				    ".daVinci", NULL),
		   "w");
  list sl = gen_copy_seq(l);

  free(dn);
  gen_sort_list(sl, control_cons_compare);
  
  fprintf(f, "[\n");
  fprintf(f, "l(\"%s\",n(\"\",[a(\"_GO\",\"text\"),a(\"OBJECT\",\"%s\\nSerial number for module %s: %d\\nEntry node: double border\\nExit node: red\\nCycles: bold italic\\nTrue arc: green\\nFalse arc: red\")],\n\t[])),\n",
	  msg, msg, get_current_module_name(), davinci_count);
  /*
  fprintf(f, "l(\"%d\",n(\"\",[a(\"_GO\",\"text\"),a(\"OBJECT\",\"serial number: %d\")],\n\t[])),\n",
	  davinci_count, davinci_count);
  */

  MAPL(c_c, {
    control c = CONTROL(CAR(c_c));
    
    davinci_print_control_node(c, f, c==CONTROL(CAR(l)), FALSE, FALSE);
    fprintf(f, "%s\n", ENDP(CDR(c_c))? "" : ",");
  }, sl);

  fprintf(f, "]\n");
  gen_free_list(sl);
  (void) safe_fclose(f, concatenate(get_current_module_name(),
				    "-",
				    itoa(davinci_count),
				    ".daVinci", NULL));
  
}

static void davinci_print_non_deterministic_unstructured
(unstructured u, string msg, hash_table scc_map, hash_table ancestor_map)
{
  extern string itoa(int);
  extern string db_get_current_workspace_directory(void);
  string dn = db_get_current_workspace_directory();
  FILE * f = safe_fopen(concatenate(dn, "/",
				    get_current_module_name(), "/", 
				    get_current_module_name(),
				    "-",
				    itoa(++davinci_count),
				    ".daVinci", NULL),
		   "w");
  control entry_c = unstructured_control(u);
  control exit_c = unstructured_exit(u);
  list l = NIL;
  list sl = list_undefined;
  
  free(dn);
  forward_control_map_get_blocs(entry_c, &l);

  if(!gen_in_list_p(exit_c, l)) {
    pips_debug(1, "Exit node %p in unstructured %p is not reachable\n", exit_c, u);
  }
  
  sl = gen_copy_seq(l);

  gen_sort_list(sl, control_cons_compare);
  
  fprintf(f, "[\n");
  fprintf(f, "l(\"%s\",n(\"\",[a(\"_GO\",\"text\"),a(\"OBJECT\",\"%s\\nSerial number for module %s: %d\\nEntry node: double border\\nExit node: red\\nCycles: bold italic\\nTrue arc: green\\nFalse arc: red\")],\n\t[])),\n",
	  msg, msg, get_current_module_name(), davinci_count);
  /*
  fprintf(f, "l(\"%d\",n(\"\",[a(\"_GO\",\"text\"),a(\"OBJECT\",\"serial number: %d\")],\n\t[])),\n",
	  davinci_count, davinci_count);
  */

  MAPL(c_c, {
    control c = CONTROL(CAR(c_c));
    control a = control_undefined;

    if((a = hash_get(ancestor_map, c))==HASH_UNDEFINED_VALUE)
      a = c;
    
    /* It may be called with an empty ancestor and scc maps. */
    davinci_print_control_node(c, f,
			       c==entry_c,
			       c==exit_c,
			       hash_table_entry_count(ancestor_map)!=0
			       && !meaningless_control_p(c)
			       && cycle_head_p(c, ancestor_map, scc_map));
    fprintf(f, "%s\n", ENDP(CDR(c_c))? "" : ",");
  }, sl);

  fprintf(f, "]\n");
  gen_free_list(sl);
  (void) safe_fclose(f, concatenate(get_current_module_name(),
				    "-",
				    itoa(davinci_count),
				    ".daVinci", NULL));
  
}

static list embedding_control_list = NIL;

void add_control_to_embedding_control_list(control c)
{
  embedding_control_list = CONS(CONTROL, c, embedding_control_list);
}

static void print_embedding_graph(control c, string msg)
{
  ifdebug(2) {
    bool consistent_embedding_graph_p = TRUE;

    pips_debug(2, "Embedding graph for vertex %p:\n", c);
    /* Do not go down into nested unstructured */
    gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, print_control_node, NULL);

    /* Check its structure: make sure that each node is a successor of its
       predecessors and a predecessor of all its successors */
    pips_assert("The embedding control list is NIL", embedding_control_list == NIL);
    gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, add_control_to_embedding_control_list, NULL);
    MAP(CONTROL, ec, {
      MAP(CONTROL, pred, {
	if(!gen_in_list_p(ec, control_successors(pred))){
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p is a predecessor of control %p "
		  "but control %p is not a successor of control %p\n",
		  pred, ec, ec, pred);
	}
	
      }
	  , control_predecessors(ec));
      MAP(CONTROL, succ, {
	if(!gen_in_list_p(ec, control_predecessors(succ))){
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p is a successor of control %p "
		  "but control %p is not a predecessor of control %p\n",
		  succ, ec, ec, succ);
	}
	
      }
	  , control_successors(ec));
    }
	, embedding_control_list);

    /* Make sure that no meaningful node only has meaningless successors */
    MAP(CONTROL, ec, {
      if(!meaningless_control_p(ec)) {
	bool meaningless_p = !ENDP(control_successors(ec));
      
	MAP(CONTROL, succ, {
	  if(!meaningless_control_p(succ)) {
	    meaningless_p = FALSE;
	  }
	}, control_successors(ec));

	if(meaningless_p) {
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p only has meaningless successors\n",
		  ec);
	}
      }
    },  embedding_control_list);

    /* Make sure that no node has any meaningless predecessor */
    MAP(CONTROL, ec, {
      MAP(CONTROL, pred, {
	if(meaningless_control_p(pred)) {
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p has meaningless predecessor %p\n",
		  ec, pred);
	}
      }, control_predecessors(ec));
    }, embedding_control_list);

    /* Make sure that destructured test statements always have two
       successors... not ture anymore with non-determinacy. */
    MAP(CONTROL, ec, {
      (void) check_control_statement(ec);
    }, embedding_control_list);

    davinci_print_control_nodes(embedding_control_list, msg);

    pips_assert("The embedding graph is consistent", consistent_embedding_graph_p);
    
    gen_free_list(embedding_control_list);
    embedding_control_list = NIL;
    pips_debug(2, "End: the embedding graph for vertex %p is consistent.\n", c);
  }
}

/* Allocate a list of control nodes transitively linked to control c */
static list node_to_linked_nodes(control c)
{
  list l = list_undefined;
  
  pips_debug(8, "Linked nodes for vertex %p:\n", c);
  pips_assert("The embedding control list is NIL", embedding_control_list == NIL);

  gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, add_control_to_embedding_control_list, NULL);
  l = embedding_control_list;
  embedding_control_list = NIL;

  ifdebug(8) {
    pips_debug(8, "End with list:\n");
    print_control_nodes(l);
  }

  return l;
}

/* Take care of two problems: meaningless control nodes may end up shared
   by effective nodes and they may end up uselessly numerous.

   FI: Two more problems could be tackled. The same node should not be
   twice a true or twice a false successor, although a given node can be a
   true and a false successor. gen_once() should be used to build t_succs
   and f_succs but the assertions and the memory management should then be
   updated. Duplicate could be chained to the u_succs(), the useless successors */

static int clean_up_control_test(control c)
{
  list succs = control_successors(c);
  list t_succs = NIL; /* true branch successors */
  list f_succs = NIL; /* false branch successors */
  list e_succs = NIL; /* empty successors */
  int ns = gen_length(succs);
  int nts = 0;
  int nfs = 0;
  int nes = 0;
  int nel = 0; /* Number of eliminations */
  int rank = 0;
  
  pips_assert("A control test has at least two successors", ns>=2);
  
  /* Partition the successors */
  MAP(CONTROL, s, 
  {
    rank++;
    if(meaningless_control_p(s)) {
      e_succs = CONS(CONTROL, s, e_succs);
    }
    else if(rank%2==1) {
      t_succs = CONS(CONTROL, s, t_succs);
    }
    else {
      f_succs = CONS(CONTROL, s, f_succs);
    }
  }
      , succs);

  nts = gen_length(t_succs);
  nfs = gen_length(f_succs);
  nes = gen_length(e_succs);
  pips_assert("The rank of the last successors is the number of successors", rank==ns);
  pips_assert("true, false and empty successors are a partition", ns==nts+nfs+nes);
  
  /* Rebuild the successor list if it can be useful */
  if(nes > nts-nfs && nes > nfs-nts) {
    list n_succs = NIL; /* new successor list */
    list c_t_succs = t_succs;
    list c_f_succs = f_succs;
    list c_e_succs = e_succs;
  
    for(rank=1; rank <= nts || rank <= nfs; rank++) {
      if(!ENDP(c_t_succs)) {
	n_succs = gen_nconc(n_succs, CONS(CONTROL, CONTROL(CAR(c_t_succs)), NIL));
	POP(c_t_succs);
      }
      else {
	pips_assert("The empty successor list is not empty", !ENDP(c_e_succs));
	n_succs = gen_nconc(n_succs, CONS(CONTROL, CONTROL(CAR(c_e_succs)), NIL));
	POP(c_e_succs);
      }
      if(!ENDP(c_f_succs)) {
	n_succs = gen_nconc(n_succs, CONS(CONTROL, CONTROL(CAR(c_f_succs)), NIL));
	POP(c_f_succs);
      }
      else {
	pips_assert("The empty successor list is not empty", !ENDP(c_e_succs));
	n_succs = gen_nconc(n_succs, CONS(CONTROL, CONTROL(CAR(c_e_succs)), NIL));
	POP(c_e_succs);
      }
    }
    pips_assert("No more true successors", ENDP(c_t_succs));
    pips_assert("No more false successors", ENDP(c_f_succs));
    /* FI: I do not see why you are sure not to need all meaningless
       controls... There might be no meaningless control at all. The next
       two assertions seem too strong. */
    pips_assert("More empty successors", !ENDP(c_e_succs));

    nel = ns - gen_length(n_succs);
    pips_assert("The successor list is reduced", nel>0);
    
    /* Courageously, free the remaining meaningless successors */
    MAP(CONTROL, e, {
      ifdebug(9) fprintf(stderr,"control %p is consistent and is going to be freed\n", e);
      free_meaningless_control(e);
    }, c_e_succs);

    /* Update the successor list of c */
    gen_free_list(control_successors(c));
    control_successors(c) = n_succs;
  }
  /* Free the partition lists */
  gen_free_list(t_succs);
  gen_free_list(f_succs);
  gen_free_list(e_succs);

  return nel;
}

static void clean_up_embedding_graph(control c)
{
  list el = node_to_linked_nodes(c);
  list tl = NIL;
  int nor = 0; /* number of replications */
  int nel = 0; /* number of eliminations */
  
  /* FI: A level of 1 makes more sense, but it is very slow on large
     graphs. The same nodes are checked several times because the cache
     managed by control_consistent_p() is lost between iterations. */
  /* ifdebug(1) { */
  ifdebug(8) {
    pips_assert("c is consistent", control_consistent_p(c));
    MAP(CONTROL, ec, {
      pips_assert("ec is consistent first", control_consistent_p(ec));
      ifdebug(10) fprintf(stderr,"control %p is consistent first\n", ec);
    }
	, el);
  }

  /* A meaningless control must have only one predecessor and no
     successor. */
  MAP(CONTROL, ec, {
    if(meaningless_control_p(ec)) {
      pips_assert("No successor", ENDP(control_successors(ec)));
      
      if(gen_length(control_predecessors(ec))>1) {
	list c_pred = list_undefined;
	
	for(c_pred=CDR(control_predecessors(ec)); !ENDP(c_pred); POP(c_pred)) {
	  control pred = CONTROL(CAR(c_pred));
	  control new_ec = make_meaningless_control(CONS(CONTROL, pred, NIL), NIL);
	  gen_list_patch(control_successors(pred), ec, new_ec);
	  nor++;
	}
	gen_free_list(CDR(control_predecessors(ec)));
	CDR(control_predecessors(ec)) = NIL;
      }
    }
  }
      , el);

    ifdebug(1) {
      MAP(CONTROL, ec, {
	pips_assert("ec is consistent second", control_consistent_p(ec));
	ifdebug(9) fprintf(stderr,"control %p is consistent second\n", ec);
      }
	  , el);
    }

  /* A non-deterministic test does not need too many meaningless
     successors. A new list must be built first because
     clean_up_control_test() destroys elements of el, which cannot be
     scanned any longer. */
  MAP(CONTROL, ec, {
    ifdebug(10) fprintf(stderr,"is control %p is consistent?\n", ec);
    ifdebug(1) pips_assert("ec is consistent third", control_consistent_p(ec));
    if(control_test_p(ec)) {
      tl = CONS(CONTROL, ec, tl);
    }
  }
      , el);

  MAP(CONTROL, ec, {
    ifdebug(10) fprintf(stderr,"is control %p is consistent?\n", ec);
    ifdebug(1) pips_assert("ec is consistent fourth", control_consistent_p(ec));
    nel += clean_up_control_test(ec);
  }
      , tl);

  gen_free_list(el);
  gen_free_list(tl);

  pips_debug(2, "End: %d useless successors destroyed\n", nel);
}

static void print_control_to_control_mapping(string message, hash_table map)
{
  fprintf(stderr, "%s\n", message);
  HASH_MAP(c1, c2, 
  {
    fprintf(stderr, "%p -> %p\n", c1, c2);
  }
	  , map);
  fprintf(stderr, "\n");
}


/* Functions to deal with the ancestor_map */

static bool ancestor_control_p(hash_table ancestor_map, control c)
{
  bool ancestor_p = FALSE;
  
  HASH_MAP(c_new, c_old, 
  {
    if(c_old==(void *) c) {
      ancestor_p = TRUE;
      break;
    }
  }
	   , ancestor_map);
  return ancestor_p;
}

/* If vertex is an ancestor control node from the input control graph,
   return vertex, else return its ancestor. Is used in other libraries. */
control control_to_ancestor(control vertex, hash_table ancestor_map)
{
  control ancestor = control_undefined; 

  if((ancestor = hash_get(ancestor_map, vertex))==HASH_UNDEFINED_VALUE)
    ancestor = vertex;
  ifdebug(2) {
    /* This assert may be too strong, but all control nodes are copied
       when just after entering bourdoncle_partition which should make it
       correct. */
    /* Theoretically only useful when HASH_UNDEFINED_VALUE has been
       returned, i.e. when ancestor==vertex */
    if(!ancestor_control_p(ancestor_map, ancestor)) {
      statement vs = control_statement(vertex);
      statement as = control_statement(ancestor);

      pips_debug(2, "vertex=%p with statement %s\n",
		 vertex, statement_identification(vs));
      pips_debug(2, "ancestor=%p with statement %s\n",
		 ancestor, statement_identification(as));
      pips_assert("ancestor really is an ancestor",
		  ancestor_control_p(ancestor_map, ancestor));
    }
  }
  
  return ancestor;
}

static bool registered_controls_p(hash_table ancestor_map, list l)
{
  bool registered_p = TRUE;

  MAP(CONTROL, c, 
  {
    if(!meaningless_control_p(c)) {
      if(hash_get(ancestor_map, c)==HASH_UNDEFINED_VALUE) {
	if(!ancestor_control_p(ancestor_map, c)) {
	  ifdebug(2) {
	    pips_debug(2, "Control %p is not registered:\n", c);
	    print_control_node(c);
	  }
	  registered_p = FALSE;
	}
      }
    }
  }
      , l);
  return registered_p;
}

/* No control can be an ancestor and a child */
static bool ancestor_map_consistent_p(hash_table ancestor_map)
{
  bool check_p = TRUE;
  list  ancestors = NIL;
  list children = NIL;
  
  HASH_MAP(c_new, c_old, 
  {
    ancestors = gen_once((control) c_old, ancestors);
    children = CONS(CONTROL, (control) c_new, children);
  }
	   , ancestor_map);
  gen_list_and(&ancestors, children);
  check_p = ENDP(ancestors);

  ifdebug(2) {
    if(!ENDP(ancestors)) {
      pips_debug(2, "Bug some control are children and ancestors:\n");
      print_control_nodes(ancestors);
      print_control_to_control_mapping("Child to ancestor map:", ancestor_map);
    }
  }

  gen_free_list(ancestors);
  gen_free_list(children);
  
  return check_p;
}

static void add_child_parent_pair(hash_table ancestor_map,
				  control child,
				  control parent)
{
  control ancestor = control_undefined;

  /* The child will inherit the parent's ancestor, if any */
  if((ancestor = (control) hash_get(ancestor_map, parent))==(control) HASH_UNDEFINED_VALUE) {
    ancestor = parent;
  }
  ifdebug(2) {
    /* The child should not already be in ancestor_map */
    pips_assert("The child should not already be in ancestor_map",
		hash_get(ancestor_map, child)==HASH_UNDEFINED_VALUE);
    /* The child cannot be an ancestor */
    pips_assert("The child is not an ancestor", !ancestor_control_p(ancestor_map, child));
  }
  hash_put(ancestor_map, (void *) child, (void *) ancestor);
}

/* Replication of unstructured (i.e. CFG) and control nodes
   (i.e. vertices) */

static hash_table replicate_map = hash_table_undefined;

static void print_replicate_map()
{
  fprintf(stderr,"\nDump of replicate_map:\n");
  HASH_MAP(old_c, new_c, 
  {
    fprintf(stderr, "Old %p -> New %p\n", old_c, new_c);
  }
	   , replicate_map);
  fprintf(stderr,"\n");
}

control control_shallow_copy(control c)
{
  control new_c = control_undefined;

  new_c = make_control(control_statement(c),
		       gen_copy_seq(control_predecessors(c)),
		       gen_copy_seq(control_successors(c)));
  hash_put(replicate_map, (void *) c, (void *) new_c);
  
  return new_c;
}

static void control_translate_arcs(control c)
{
  MAPL(c_c,
  {
    control old_c = CONTROL(CAR(c_c));
    control new_c = hash_get(replicate_map, (void *) old_c);
    pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
    CONTROL(CAR(c_c)) = new_c;
  }
      , control_predecessors(c));
  MAPL(c_c,
  {
    control old_c = CONTROL(CAR(c_c));
    control new_c = hash_get(replicate_map, (void *) old_c);
    pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
    CONTROL(CAR(c_c)) = new_c;
  }
      , control_successors(c));
}

static control control_to_replicate(control old_c)
{
  control new_c =  hash_get(replicate_map, (void *) old_c);
  pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
  return new_c;
}

unstructured unstructured_shallow_copy(unstructured u, hash_table ancestor_map)
{
  unstructured new_u = unstructured_undefined;
  
  pips_assert("replicate_map is undefined",
	      hash_table_undefined_p(replicate_map));
  
  replicate_map = hash_table_make(hash_pointer, 0);

  /* Do not go down into nested unstructured and replicate all control
     nodes */
  gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, control_shallow_copy, NULL);

  ifdebug(2) print_replicate_map();

  /* Update predecessor and successor arcs of the new control nodes to
     point to the new control nodes using the global conversion mapping
     replicate_map */
  HASH_MAP(old_c, new_c,
  {
    control_translate_arcs((control) new_c);
  }
	   , replicate_map);

  /* Generate new unstructured with relevant entry and exit nodes */
  new_u = make_unstructured(control_to_replicate(unstructured_control(u)),
			    control_to_replicate(unstructured_exit(u)));

  /* Generate ancestor_map as the inverse function of replicate_map */
  HASH_MAP(old_n, new_n,{
    add_child_parent_pair(ancestor_map, (control) new_n, (control) old_n);
  }, replicate_map);
  
  hash_table_free(replicate_map);
  replicate_map = hash_table_undefined;

  return new_u;
}


/* Build a new unstructured (CFG) for partition. vertex is the entry and
   exit point. New nodes must be allocated because the parent graph is
   untouched. vertex is supposed to be included into partition. */
unstructured partition_to_unstructured(control vertex, list partition)
{
  unstructured new_u = unstructured_undefined;
  list useless = NIL;
  
  replicate_map = hash_table_make(hash_pointer, 0);

  /* Create the translation table replicate_map */
  MAP(CONTROL, c,
  {
    (void) control_shallow_copy(c);
  }
      , partition);

  /* Generate new unstructured with relevant entry and exit node vertex */
  new_u = make_unstructured(control_to_replicate(vertex),
			    control_to_replicate(vertex));

  /* Update arcs */

  /* Update predecessors */
  MAP(CONTROL, c,
  {
    control c_new = control_to_replicate(c);

    /* The only predecessors that are useful are in partition */
    MAPL(c_c,
    {
      control old_c = CONTROL(CAR(c_c));
      control new_c = control_undefined;

      if(gen_in_list_p(old_c, partition)) {
	new_c = hash_get(replicate_map, (void *) old_c);
	pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
	CONTROL(CAR(c_c)) = new_c;
      }
      else {
	/* This predecessor is irrelevant */
	useless = CONS(CONTROL, old_c, useless);
      }
    }
	 , control_predecessors(c_new));

    gen_list_and_not(&control_predecessors(c_new), useless);
    gen_free_list(useless);
    useless = NIL;

    /* Handle successors */
    MAPL(c_c,
    {
      control old_c = CONTROL(CAR(c_c));
      control new_c = control_undefined;

      if(gen_in_list_p(old_c, partition)) {
	new_c = hash_get(replicate_map, (void *) old_c);
	pips_assert("new_c is defined",
		    new_c!=(control) HASH_UNDEFINED_VALUE);
      }
      else {
	pips_assert("If a successor is not in partition,"
		    " then there must be two successors",
		    gen_length(control_successors(c_new))==2)
	/* This successor is irrelevant but irrelevant true branches must
           be preserved */
	if(c_c==control_successors(c_new)) {
	  pips_assert("The second successor is in partition and has been copied",
		      gen_in_list_p(CONTROL(CAR(CDR(c_c))),partition) );
	  new_c = make_meaningless_control(CONS(CONTROL, c_new, NIL), NIL);
	}
	else {
	  /* A second irrelevant successor is not needed. But are we ready
             to have IF nodes with only one successor? */
	  /* useless = CONS(CONTROL, old_c, useless); */
	  new_c = make_meaningless_control(CONS(CONTROL, c_new, NIL), NIL);
	}
      }
	
      if(!control_undefined_p(new_c))
	CONTROL(CAR(c_c)) = new_c;
    }
	 , control_successors(c_new));
  }
      , partition);

  /*
    control_predecessors(c_new) = gen_list_and_not(control_predecessors(c_new), useless);
    gen_free_list(useless);
    useless = NIL;
  */

  hash_table_free(replicate_map);
  replicate_map = hash_table_undefined;

  return new_u;
}

static bool entry_or_exit_control_p(control c,
				    list partition,
				    bool check_entry)
{
  bool is_entry_or_exit_p = FALSE;
  list nodes = check_entry?control_predecessors(c):control_successors(c);

  MAP(CONTROL, pred, 
  {
    if(!meaningless_control_p(pred) && !gen_in_list_p(pred, partition)) {
      is_entry_or_exit_p = TRUE;
      break;
    }
  }
      , nodes);

  return is_entry_or_exit_p;
}
static bool entry_control_p(control c, list partition)
{
  return entry_or_exit_control_p(c, partition, TRUE);
}

static bool exit_control_p(control c, list partition)
{
  return entry_or_exit_control_p(c, partition, FALSE);
}


/* If complement_p, preserve successors in the complement of
   partition. Else preserve successors in partition. Take care of test
   nodes wich must keep two successors no matter what. */
void intersect_successors_with_partition_or_complement(control c,
						       list partition,
						       bool complement_p)
{
  list * succs = &control_successors(c);
  
  pips_assert("We have at least one successor", gen_length(*succs)>0);
  
  if(gen_length(*succs)==1) {
    control c = CONTROL(CAR(*succs));
    /* The CFG may not be connexe or the exit path may be hidden by a STOP statement */
    /*
      pips_assert("The unique successor must be in partition",
      gen_in_list_p(c, partition)==!complement_p);
    */
    if(gen_in_list_p(c, partition)==complement_p) {
      if(complement_p)
	gen_list_and_not(succs, partition);
      else
	gen_list_and(succs, partition);
    }
  }
  else if (control_test_p(c)) {
    /* Let's boldy assume the parent control node is used as a CFG test
       statement and preserve the number of successors. */
    MAPL(succ_c, {
      control succ = CONTROL(CAR(succ_c));

      if(gen_in_list_p(succ, partition)==complement_p)
	CONTROL(CAR(succ_c)) = make_meaningless_control(CONS(CONTROL, c, NIL), NIL);
    } , *succs);
  }
  else {
    /* It is not a CFG test node: get rid of useless  */
    if(complement_p)
      gen_list_and_not(succs, partition);
    else
      gen_list_and(succs, partition);
  }
  /* Not true: you may have non-connexe CFG and loops with no exit. */
  /* pips_assert("We still have at least one successor", gen_length(*succs)>0); */
}

void intersect_successors_with_partition(control c,
						       list partition)
{
  intersect_successors_with_partition_or_complement(c, partition, FALSE);
}

void intersect_successors_with_partition_complement(control c,
						    list partition)
{
  intersect_successors_with_partition_or_complement(c, partition, TRUE);
}

static void insert_non_deterministic_control_node(list succs,
						  control pred,
						  control new_c,
						  control old_c)
{
  if(FALSE) {
    /* Can we do without a extra-node? */
  }
  else{
    /* */  
  statement nop = make_continue_statement(entity_empty_label());
  control cnop = make_control(nop,
			      CONS(CONTROL, pred, NIL),
			      CONS(CONTROL, new_c, CONS(CONTROL, old_c, NIL)));
  pips_assert("succs points to old_c", CONTROL(CAR(succs))==old_c);
  pips_debug(2, "Allocate new control node %p as CONTINUE for test control node %p"
	     " with two successors, a new one, %p, and an old one %p\n",
	     cnop, pred, new_c, old_c);
  CONTROL(CAR(succs)) = cnop;
  gen_list_patch(control_predecessors(old_c), pred, cnop);
  gen_list_patch(control_predecessors(new_c), pred, cnop);
  }
}

/* new_c is not consistent on entry and might not be on exit because it is
   called from within a loop */
static void update_successors_of_predecessor(control pred, control new_c, control old_c)
{
  pips_assert("old_c is already a successor of pred",
	      gen_in_list_p(old_c, control_successors(pred)));
  pips_assert("new_c is not yet a successor of pred",
	      !gen_in_list_p(new_c, control_successors(pred)));
  
  if(control_test_p(pred)) {
    int r = 0;
    bool insert_p = FALSE; /* A meaningless control node must be inserted? */
    
    if((r=gen_position(old_c, control_successors(pred)))==0) {
      pips_internal_error("old_c %p must be a successor of pred %p\n", old_c, pred);
    }
    else if(r%2==1) {
      /* TRUE successor */
      insert_p = (gen_length(control_successors(pred))%2==1);
    }
    else /* r%2==0 */ {
      insert_p = (gen_length(control_successors(pred))%2==0);
    }
    if(insert_p) {
      control mlc = make_meaningless_control(CONS(CONTROL, pred, NIL),NIL);
      
      control_successors(pred) = gen_nconc(control_successors(pred),
					   CONS(CONTROL, mlc, NIL));
    }
    control_successors(pred) = gen_nconc(control_successors(pred),
					 CONS(CONTROL, new_c, NIL));

    ifdebug(8) {
      pips_debug(8, "%s meaningless control node added. New successor list of pred %p:\n",
		 insert_p? "A" : "No", pred);
      print_control_nodes_without_check(control_successors(pred));
    }
  }
  else {
    /* No problem this kind of node may have several successors */
    control_successors(pred) = CONS(CONTROL, new_c, control_successors(pred));
  }
}

static void update_predecessors_of_successor(control succ, control new_c, control old_c)
{
  /* These asserts might become wrong when intermediate CONTINUE nodes are
     used to carry many TRUE and/or FALSE successors */
  pips_assert("old_c is already a predecessor of succ",
	      gen_in_list_p(old_c, control_predecessors(succ)));
  pips_assert("new_c is not yet a predecessor of succ",
	      !gen_in_list_p(new_c, control_predecessors(succ)));
  
  control_predecessors(succ) = CONS(CONTROL, new_c, control_predecessors(succ));
}

/* Called from within a loop where neither t nor new_s are consistent */
static void add_test_successor(control t, control new_s, bool is_true_successor)
{
  bool slot_found = FALSE;
  int rank = 0;
  int pos = 0;

  pips_debug(8, "Begin with t=%p, new_s=%p and is_true_successor=%d",
	     t, new_s, is_true_successor);
  
  pips_assert("t is a control with a test", control_test_p(t));
  pips_assert("is_true_successor is 0 or 1",
	      is_true_successor==0 || is_true_successor==1);

  MAPL(s_c, {
    control s = CONTROL(CAR(s_c));

    rank = 1 - rank;
    pos++;
    
    if(rank==is_true_successor && meaningless_control_p(s)) {
      pips_debug(2, "Free meaningless control node %p\n", s);
      free_meaningless_control(s);
      CONTROL(CAR(s_c)) = new_s;
      slot_found = TRUE;
      break;
    }
  } , control_successors(t));

  if(!slot_found) {
    if( (gen_length(control_successors(t))%2) == is_true_successor) {
      /* Allocate a meaningless control */
      control mlc = make_meaningless_control(CONS(CONTROL, t, NIL), NIL);
      
      control_successors(t) = gen_nconc(control_successors(t),
					CONS(CONTROL, 
					     mlc,
					     CONS(CONTROL, new_s, NIL)));
      pos += 2;
    }
    else {
      control_successors(t) = gen_nconc(control_successors(t),
					CONS(CONTROL, new_s, NIL));
      pos++;
    }
  }

  pips_debug(8, "End with slot_found=%s, rank=%d and pos=%d\n",
	     bool_to_string(slot_found), rank, pos);

  pips_assert("The position is consistent with is_true_successor",
	      pos%2==is_true_successor);
  
  /* t may not be consistent because of the caller
  ifdebug(1) {
    pips_assert("t is consistent", check_control_statement(t));
  }
  */
}

/* Make new_c a successor of new_pred, the same way c is a successor of
   pred. new_c is not consistent on entry: it points towards pred but is
   not a successor of pred. Also, this function is called from within a
   loop and new_c is onsistent only on loop exit.*/
static void update_successor_in_copy(control new_pred,
				     control pred,
				     control c,
				     control new_c)
{
  ifdebug(3) {
    pips_debug(3, "Begin for new_pred=%p, pred=%p, c=%p, new_c=%p\n",
	       new_pred, pred, c, new_c);
    print_control_node(new_pred);
    print_control_node(pred);
    print_control_node(c);
    /* new_c is not consistent, this is why this function is called! */
    print_control_node_without_check(new_c);

    pips_assert("pred is predecessor of new_c",
		gen_in_list_p(pred, control_predecessors(new_c)));
  }

  /* c and new_c have pred as predecessor */
  /* new_c must have new_pred as predecessor instead */
  pips_assert("c is a successor of pred",
	      gen_in_list_p(c, control_successors(pred)));
  pips_assert("pred is a predecessor of new_c",
	      gen_in_list_p(pred, control_predecessors(new_c)));
  pips_assert("pred and new_pred share the same statement",
	      control_statement(pred) ==control_statement(new_pred));
  pips_assert("c and new_c share the same statement",
	      control_statement(c) ==control_statement(new_c));
  
  if(control_test_p(pred)) {
    int pos = gen_position(c, control_successors(pred));
    bool is_true_succ = pos%2;

    pips_assert("The position is not zero since c is among the sucessors of pred"
		" (see previous assert)", pos!=0);

    pips_debug(4, "position=%d, is_true_succ=%d \n", pos, is_true_succ);
    pips_assert("pred is a test, new_pred is a test too", control_test_p(new_pred));
    pips_assert("pred is still a predecessor of new_c",
		gen_in_list_p(pred, control_predecessors(new_c)));
    add_test_successor(new_pred, new_c, is_true_succ);
  }
  else {
    control_successors(new_pred) = gen_nconc(control_successors(new_pred),
					     CONS(CONTROL, new_c, NIL));
  }
  
  gen_list_patch(control_predecessors(new_c), pred, new_pred);
  
  ifdebug(3) {
    pips_debug(3, "End with new_pred=%p\n", new_pred);
    print_control_node(new_pred);
    pips_debug(3, "and pred=%p\n", pred);
    print_control_node(pred);
    pips_debug(3, "and c=%p\n", c);
    print_control_node(c);
    pips_debug(3, "and new_c=%p\n", new_c);
    print_control_node_without_check(new_c);
  }
}

/* The nodes in scc partition but root must be removed. Replicated nodes
   must be added to build all input and output paths to and from root
   using control nodes in partition.

   This is the key change to Bourdoncle's algorithm. When moving back up
   in the recursion, the CFG is altered to obtain pure cycles with only
   one entry and one exit nodes.

   The cycle head must be a non-deterministic control node. Hence it
   cannot be a test since tests stay deterministics (thanks to Pierre's
   data structure). Another node in the partition could be chosen as new
   cycle head, but the partition may only contain tests with side
   effects. Instead we choose to insert a non-deterministic node in front
   in case it is needed. The initial CFG is less disturbed.

   Well, I changed my mind again because inserting new control nodes does
   not make it easy to stay compatible with Bourdoncle's algorithm and
   previous assumptions made about the cycle head. Instead, tests can
   become non-deterministic. The number of successors must be even or odd. Odd
   successors are TRUE successors. Even successors are FALSE successors.

  */

static unstructured scc_to_dag(control root, list partition, hash_table ancestor_map)
{
  unstructured u = unstructured_undefined;
  control new_root = control_undefined;
  hash_table replicated_input_controls = hash_table_make(hash_pointer, 0);
  hash_table replicated_output_controls = hash_table_make(hash_pointer, 0);
  bool stable_graph_p = FALSE;
  int number_in = 0;
  int number_out = 0;
  int iteration = 0;
  char msg[200];

  ifdebug(3) {
    pips_debug(3, "Begin for vertex %p and partition:\n", root);
    print_control_nodes(partition);
  }
  
  while(!stable_graph_p) {
    int number = 0;
    list c_c = list_undefined;

    stable_graph_p = TRUE;
    iteration++;
    
    /* Look for new input nodes in partition */
    for(c_c = partition; !ENDP(c_c); POP(c_c)) {
      control c = CONTROL(CAR(c_c));
      
      if(c!=root && hash_get(replicated_input_controls, c)==HASH_UNDEFINED_VALUE) {
	if(entry_control_p(c, partition)) {
	  control new_c1 = control_undefined;
	  
	  /* c cannot have been replicated earlier or it would not be in partition */
	  pips_assert("c has not yet been input replicated",
		      hash_get(replicated_input_controls, c)==HASH_UNDEFINED_VALUE);
	  new_c1 = make_control(control_statement(c),
				gen_copy_seq(control_predecessors(c)),
				gen_copy_seq(control_successors(c)));
	  add_child_parent_pair(ancestor_map, new_c1, c);
	  copy_dfn(new_c1, c);

	  pips_debug(4, "Allocate new input control node new_c1=%p"
		     " as a copy of node %p with depth %d\n",
		     new_c1, c, get_dfn(c));
	  
	  /* Keep only predecessors out of partition for new_c1. */
	  gen_list_and_not(&control_predecessors(new_c1), partition);
	  /* Make sure that new_c1 is a successor of its
             predecessors... Oupss, there is a problem with test to encode
             the fact that is is a true or a false successor? */
	  MAP(CONTROL, pred_c1, {
	    gen_list_patch(control_successors(pred_c1), c, new_c1);
	  },control_predecessors(new_c1) );
	  
	  
	  /* Keep only c's predecessors in partition. 

	     That's probably wrong and damaging because we need them for
	     paths crossing directly the SCC and useless because these
	     nodes will be destroyed in parent graph. */
	  gen_list_and(&control_predecessors(c), partition);
	  /* Update the successor of its predecessors */
	  
	  /* Update successors of new_c1 which have already been replicated */
	  MAPL(succ_new_c1_c,
	  {
	    control succ_new_c1 = CONTROL(CAR(succ_new_c1_c));
	    control new_succ_new_c1 = hash_get(replicated_input_controls, succ_new_c1);
	    if(new_succ_new_c1!=HASH_UNDEFINED_VALUE) {
	      CONTROL(CAR(succ_new_c1_c)) = new_succ_new_c1;
	      /* Add new_c1 as predecessor of new_succ */
	      control_predecessors(new_succ_new_c1) =
		gen_once(new_c1, control_predecessors(new_succ_new_c1));
	    }
	    else {
	      update_predecessors_of_successor(succ_new_c1, new_c1, c);
	    }
	  }
	       , control_successors(new_c1));

	  stable_graph_p = FALSE;
	  number++;
	  number_in++;
	  hash_put(replicated_input_controls, c, new_c1);
	}
      }
    }

    ifdebug(9) {
      if(number>0) {
	pips_debug(4, "Embedding graph after replication"
		   " of %d input control node(s) at iteration %d:\n",
		   number, iteration);
	sprintf(msg, "Embedding graph after replication"
		   " of %d input control node(s) at iteration %d:",
		   number, iteration);
	print_embedding_graph(root, msg);
	pips_debug(4, "End of embedding graph after replication"
		   " of input control nodes\n");
      }
      else {
	pips_debug(4, "No new input control node at iteration %d\n",
		   iteration);
      }
    }
    
    /* Look for new output nodes */
    number = 0;
    for(c_c = partition; !ENDP(c_c); POP(c_c)) {
      control c = CONTROL(CAR(c_c));
      
      if(c!=root && hash_get(replicated_output_controls, c)==HASH_UNDEFINED_VALUE) {
	if(exit_control_p(c, partition)) {
	  control new_c2 = control_undefined;
	  list pred_new_c2_c = list_undefined;

	  /* c cannot have been replicated earlier or it would not be in partition */
	  pips_assert("c has not yet been output replicated",
		      hash_get(replicated_output_controls, c)==HASH_UNDEFINED_VALUE);
	  new_c2 = make_control(control_statement(c),
				gen_copy_seq(control_predecessors(c)),
				gen_copy_seq(control_successors(c)));
	  add_child_parent_pair(ancestor_map, new_c2, c);
	  copy_dfn(new_c2, c);

	  pips_debug(4, "Allocate new output control node new_c2=%p as a copy of node %p\n",
		     new_c2, c);

	  /* Keep only successors out of partition for new_c2. */
	  /* A true branch successor may have to be replaced to preserve
             the position of the false branch successor. */
	  /* gen_list_and_not(&control_successors(new_c2), partition); */
	  intersect_successors_with_partition_complement(new_c2, partition);
	  
	  /* Update reverse arcs */
	  MAP(CONTROL, succ_c2, {
	    gen_list_patch(control_predecessors(succ_c2), c, new_c2);
	  }, control_successors(new_c2) );
	  
	  /* Keep only c's succcessors in partition. Wise or not? See above... */
	  /* gen_list_and(&control_successors(c), partition); */
	  intersect_successors_with_partition(c, partition);
	  /* Reverse arcs have already been correctly updated */
	
	  /* Update predecessors of new_c2 which have already been replicated */
	  for(pred_new_c2_c=control_predecessors(new_c2);
	      !ENDP(pred_new_c2_c); POP(pred_new_c2_c)) {
	    control pred_new_c2 = CONTROL(CAR(pred_new_c2_c));
	    control new_pred_new_c2 = hash_get(replicated_output_controls, pred_new_c2);
	    if(new_pred_new_c2!=HASH_UNDEFINED_VALUE) {
	      /* gen_list_patch() has no effect because new_pred's
                 successors have already been updated and c does not
                 appear there. */
	      /* gen_list_patch(control_successors(new_pred), c, new_c2); */
	      update_successor_in_copy(new_pred_new_c2, pred_new_c2, c, new_c2);
	      /*
	      CONTROL(CAR(pred_new_c2_c)) = new_pred_new_c2;
	      pips_assert("new_c2 is now consistent", check_control_statement(new_c2));
	      */
	    }
	    else{
	      /* Update reverse edges of predecessors... which may be
		 tough if you end up with more than one TRUE and/or more
		 than one FALSE successor. */
	      update_successors_of_predecessor(pred_new_c2, new_c2, c);
	    }
	  }
	  pips_assert("new_c2 is now consistent", check_control_statement(new_c2));
	  
	  stable_graph_p = FALSE;
	  number++;
	  number_out++;
	  hash_put(replicated_output_controls, c, new_c2);
	}
      }
    }

    ifdebug(9) {
      if(number>0) {
	pips_debug(3, "Embedding graph after replication"
		   " of output control nodes at iteration %d:\n", iteration);
	sprintf(msg, "Embedding graph after replication"
		   " of output control nodes at iteration %d:", iteration);
	print_embedding_graph(root, msg);
	pips_debug(3, "End of embedding graph after replication"
		   " of output control nodes\n");
      }
      else {
	pips_debug(3, "No new output control node at iteration %d\n", iteration);
      }
    }
  }

  clean_up_embedding_graph(root);
  
  ifdebug(3) {
    if(number_out>0 || number_in>0) {
      pips_assert("At least two iterations", iteration>1);
      pips_debug(3, "Final embedding graph after replication of all input and output paths (%d iterations)\n", iteration);
      sprintf(msg, "Final embedding graph after replication of all input and output paths (%d iterations)", iteration);
      print_embedding_graph(root, msg);
      pips_debug(3, "End of final embedding graph after replication of all input and output paths\n");
    }
    else {
      pips_assert("Only one iteration", iteration==1);
      pips_debug(3, "No new output paths\n");
    }
  }

  /* Remove cycle partition but its head */
  /* We do not want to free these nodes because they are part of
     partition.  We should not have duplicated partition earlier to make a
     new unstructured.  We only need a copy of the head, root. This should
     be dealt with after scc_to_dag is called and not before.

     MAP(CONTROL, c, 
     {
     if(c!=root) {
     shallow_free_control(c);
     }
     }
     , partition); */
  /* Unlink partition from its head.
     We could keep an arc from root to root... */
  /* This unlinking would better be performed at the caller level, even
     though this would make the name scc_to_dag() imprecise. */

  /* replicate root and unlink the cycles defined by partition from the
     embedding graph */
  new_root = make_control(control_statement(root),
			  gen_copy_seq(control_predecessors(root)),
			  gen_copy_seq(control_successors(root)));

  add_child_parent_pair(ancestor_map, new_root, root);
  u = make_unstructured(new_root, new_root);
  gen_list_and_not(&control_predecessors(root), partition);
  intersect_successors_with_partition_complement(root, partition);
  gen_list_and(&control_predecessors(new_root), partition);
  intersect_successors_with_partition(new_root, partition);
  MAP(CONTROL, c, 
  {
    gen_list_patch(control_successors(c), root, new_root);
    gen_list_patch(control_predecessors(c), root, new_root);
  }
      , partition);
  /* new_root does not belong to partition and must be processed too. */
  gen_list_patch(control_successors(new_root), root, new_root);
  gen_list_patch(control_predecessors(new_root), root, new_root);
  
  ifdebug(3) {
    list l_root = node_to_linked_nodes(root);
    list l_new_root = node_to_linked_nodes(new_root);
    
    pips_debug(3, "Nodes linked to root=%p\n", root);
    print_control_node(root);
    print_control_nodes(l_root);
    pips_debug(3, "Nodes linked to new_root=%p\n", new_root);
    print_control_node(new_root);
    print_control_nodes(l_new_root);
    
    gen_list_and(&l_root, l_new_root);
    
    if(!ENDP(l_root)) {
      fprintf(stderr,
	      "Bug: l_root and l_new_root share at least one node and must be equal\n");
      fprintf(stderr, "Common control nodes:\n");
      print_control_nodes(l_root);
      pips_assert("l_root and l_new_root have an empty intersection",
		  ENDP(l_root));
    }
    
    gen_free_list(l_root);
    gen_free_list(l_new_root);
  }

  clean_up_embedding_graph(root);

  clean_up_embedding_graph(new_root);

  ifdebug(3) {
    list l_root = node_to_linked_nodes(root);
    list l_new_root = node_to_linked_nodes(new_root);

    pips_debug(3, "Final embedding graph after replication and cycle removal\n");
    sprintf(msg, "Final embedding graph after replication and cycle removal");
    print_embedding_graph(root, msg);
    pips_debug(3, "End of final embedding graph after replication and cycle removal\n");
    pips_debug(3, "Final cycle graph\n");
    sprintf(msg, "Final cycle graph");
    print_embedding_graph(new_root, msg);
    pips_debug(3, "End of final cycle graph\n");

    /* All nodes are linked to an ancestor node or are an ancestor or are
       meaningless */
    pips_assert("All nodes in l_root are registered",
		registered_controls_p(ancestor_map, l_root));
    pips_assert("All nodes in l_new_root are registered",
		registered_controls_p(ancestor_map, l_new_root));
    pips_assert("Children and ancestors have an empty intersection",
		ancestor_map_consistent_p(ancestor_map));
    
    gen_free_list(l_root);
    gen_free_list(l_new_root);
  }
   
  ifdebug(3) {
    pips_debug(3, "End with %d replicated control nodes:\n\n", number_in+number_out);
    if(number_in>0){
      print_control_to_control_mapping("replicated_input_controls",
				       replicated_input_controls);
    }
    
    if(number_out>0) {
      print_control_to_control_mapping("replicated_output_controls",
				       replicated_output_controls);
    }
    
  }
  
  hash_table_free(replicated_input_controls);
  hash_table_free(replicated_output_controls);

  ifdebug(3) {
    pips_debug(3, "End for vertex %p and partition:\n", root);
    print_control_nodes(partition);
  }

  return u;
}

/*
 *
 * To deal with transformers computed in context, each call sites of a
 * cycle is attached a private copy of the cycle.
 *
 * This is not part of Bourdoncle's algorithm.
 *
 */

static void replicate_cycles(unstructured u_main, hash_table scc_map, hash_table ancestor_map)
{
  list scc_to_process = CONS(UNSTRUCTURED, u_main, NIL);
  list scc_to_process_next = NIL;
  list live_scc = NIL;
  int replication_number = 0;
  int deletion_number = 0;

  pips_debug(1, "Start with %d cycles for unstructured %p with entry %p\n",
	     hash_table_entry_count(scc_map), u_main, unstructured_control(u_main));

  while(!ENDP(scc_to_process)) {
    MAP(UNSTRUCTURED, u, {
      list nl = NIL;
      control root = unstructured_control(u);
      FORWARD_CONTROL_MAP(c, {
	/* Only the head of the main unstructured can be a pointer to a
           cycle. The head of a scc cannot point to another scc as they
           would be the same by construction: all their cycles would be
           cut off by the head of each scc. */
	if((c!=root || c==unstructured_control(u_main))
	   && !meaningless_control_p(c)
	   && cycle_head_p((control)c, ancestor_map, scc_map)) {
	  static unstructured ancestor_cycle_head_to_scc(control, hash_table);
	  control a = control_to_ancestor(c, ancestor_map);
	  unstructured scc_u = ancestor_cycle_head_to_scc((control)a, scc_map);

	  unstructured scc_u_c = unstructured_shallow_copy(scc_u, ancestor_map);
	  scc_to_process_next = CONS(UNSTRUCTURED, scc_u_c, scc_to_process_next);
	  hash_put(scc_map, c, (void *) scc_u_c);
	  replication_number++;

	  ifdebug(1) {
	    control e = unstructured_control(scc_u_c);
	    pips_debug(1, "New scc %p with entry %p for node %p copy of ancestor node %p\n",
		       scc_u_c, e, c, control_to_ancestor(c, ancestor_map));
	    pips_assert("e is a proper cycle head", proper_cycle_head_p(e, scc_map));
	    pips_assert("e is a cycle head", cycle_head_p(e, ancestor_map, scc_map));
	    pips_assert("e is a not a direct cycle head",
			!direct_cycle_head_p(e, scc_map));
	  }

	}
      }, root, nl);
      gen_free_list(nl);
    }, scc_to_process);
    live_scc = gen_nconc(live_scc, scc_to_process);
    scc_to_process = scc_to_process_next;
    scc_to_process_next = NIL;
  }

  /* Only live scc are useful */
  HASH_MAP(c, scc, {
    if(!gen_in_list_p(scc, live_scc)) {
      void * key = NULL;
      unstructured u_u = unstructured_undefined;
      u_u = (unstructured) hash_delget(scc_map, (void *) c, (void **) &key);
      if(unstructured_undefined_p(u_u)) {
	pips_internal_error("No scc for control %p\n", c);
      }
      else {
	/* we really need a shallow_free_unstructured() */
	/* free_unstructured(u_u); */
	pips_debug(1, "scc %p unlinked from node %p\n", u_u, c);
	deletion_number++;
      }
    }
  }, scc_map);

  pips_assert("The number of scc's has increased",
	      replication_number >= deletion_number);

  gen_free_list(live_scc);

  pips_debug(1, "End replication process with %d copies and %d deletions\n",
	     replication_number, deletion_number);

  pips_debug(1, "Start with %d cycles\n", hash_table_entry_count(scc_map));
}

/* 
   MAIN ENTRY: BOURDONCLE_PARTITION

   Decomposition of control flow graph u into a non-deterministic DAG CFG
   *p_ndu and two mappings. 

   Mapping scc_map maps nodes of u used to break cycles to the
   unstructured representing these cycles if cycles are not
   replicated. Else, scc_map maps nodes of ndu and of the scc's to their
   own copies of the relevant scc's.

   Mapping ancestor_map maps nodes used in DAG new_u or in unstructured
   refered to by scc_map to nodes in u. The initial control nodes in u are
   called "ancestors".

   The partition list returned should be compatible with a
   top-down partial order.

 */

list bourdoncle_partition(unstructured u,
			  unstructured * p_ndu,
			  hash_table *p_ancestor_map,
			  hash_table * p_scc_map)
{
  list partition = NIL;
  control root = control_undefined;
  /* DAG derived from u by elimination all cycles */
  unstructured new_u = unstructured_undefined;
  /* mapping from nodes in the new unstructured to the node in the input unstructured */
  hash_table ancestor_map = hash_table_make(hash_pointer, 0);
  /* mapping from nodes in u, used as cycle breakers, to the corresponding
     scc represented as an unstructured */
  hash_table scc_map = hash_table_make(hash_pointer, 0);
  
  set_davinci_count();

  ifdebug(2) {
    pips_debug(2, "Begin with unstructured:\n");
    print_unstructured(u);
    davinci_print_non_deterministic_unstructured(u, "Initial unstructured", scc_map, ancestor_map);
  }

  make_vertex_stack();
  num = 0;
  dfn = hash_table_make(hash_pointer, 0);

  new_u = unstructured_shallow_copy(u, ancestor_map);

  ifdebug(8) {
    pips_debug(3, "Copied unstructured new_u:\n");
    print_unstructured(new_u);
    davinci_print_non_deterministic_unstructured(u, "Copy of initial unstructured",
						 scc_map, ancestor_map);
  }
      

  /* Initialize dfn to 0 */
  gen_multi_recurse(new_u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, reset_dfn, NULL);

  /* Start from the entry point */
  root = unstructured_control(new_u);  
  (void) bourdoncle_visit(root, &partition, ancestor_map, scc_map);
    
  /* Should the partition be translated? */

  free_vertex_stack();
  hash_table_free(dfn);

  /* Should the cycles be replicated to refine the analyses? If yes, replicate them. */
  if(get_bool_property("SEMANTICS_COMPUTE_TRANSFORMERS_IN_CONTEXT")) {
    replicate_cycles(new_u, scc_map, ancestor_map);
  }

  ifdebug(2) {
    int number_of_fix_points = 0;

    pips_debug(2, "End with partition:");
    print_control_nodes(partition);

    pips_debug(2, "End with scc_map:\n");
    HASH_MAP(h, uc, {
      number_of_fix_points++;
      fprintf(stderr, "head=%p with unstructured=%p\n", h, uc);
    } , scc_map);
    pips_debug(2, "End with %d fix points\n", number_of_fix_points);

    /* Control nodes associated to a fix point */
    pips_debug(2, "\nControl nodes associated to a fix point via a sub-unstructured:\n");
    HASH_MAP(c_n, c_o, {
      void * su;
      if((su=hash_get(scc_map, c_o))!=HASH_UNDEFINED_VALUE) {
	fprintf(stderr, "head=%p copy of %p with unstructured=%p\n", c_n, c_o, su);
      }
    }, ancestor_map);
    
    /* The hierarchy of fix points is/seems to be lost by scc_map... */
    pips_debug(2, "End with %d sets of cycles:\n", number_of_fix_points);
    number_of_fix_points = 0;
    HASH_MAP(h, uc, {
      char msg[256];
      
      number_of_fix_points++;
      fprintf(stderr, "Cycles associated to head=%p with unstructured=%p (fix point %d)\n",
	      h, uc, number_of_fix_points);
      sprintf(msg, "Cycles associated to head=%p\\n with unstructured=%p (fix point %d)",
	      h, uc, number_of_fix_points);
      print_unstructured((unstructured) uc);
      davinci_print_non_deterministic_unstructured((unstructured) uc, msg, scc_map, ancestor_map);
      fprintf(stderr, "\n");
    } , scc_map);

    /* We also need the final embedding graph... which might be lost? Or
       hanging from the first node in partition, the entry point, and hence from new_u. */
    pips_debug(2, "Final embedding graph %p:\n", new_u);
      print_unstructured(new_u);
    davinci_print_non_deterministic_unstructured(new_u, "Final embedding graph", scc_map, ancestor_map);

    pips_debug(2, "End. \n");
  }

  *p_ndu = new_u;
  *p_ancestor_map = ancestor_map;
  *p_scc_map = scc_map;

  return partition;
}

/* FUNCTIONS FOR BOURDONCLE_COMPONENT()
 */

/* Check the existence of a path from b to e with all its element in partition */
static bool partition_successor_p(control b, control e, list partition)
{
  bool path_p = FALSE;
  list preds = CONS(CONTROL, b, NIL);
  list succs = NIL;
  list not_seen = gen_copy_seq(partition);
  int length = 0;

  pips_debug(3, "Begin with b=%p abd e=%p\n", b, e)

  ifdebug(3) {
    pips_assert("b is in partition", gen_in_list_p(b, partition));
    pips_assert("e is in partition", gen_in_list_p(e, partition));
    /* You might be interested in the existence of a cycle from b to b */
    /* pips_assert("b is not e", b!=e); */
  }

  gen_remove(&not_seen, b);

  while(!ENDP(preds)){
    length++;
    pips_debug(3, "Iteration %d\n", length);
    MAP(CONTROL, pred, {
      pips_debug(3, "\tpred=%p\n", pred);
      MAP(CONTROL, succ, {
	pips_debug(3, "\t\tsucc=%p\n", succ);
	if(succ==e) {
	  path_p = TRUE;
	  goto end;
	}
	else if(gen_in_list_p(succ, not_seen)) {
	  gen_remove(&not_seen, succ);
	  succs = CONS(CONTROL, succ, succs);
	}
      } , control_successors(pred));
    } , preds);
    gen_free_list(preds);
    preds = succs;
    succs = NIL;
  }

 end:
  gen_free_list(not_seen);
  gen_free_list(preds);
  gen_free_list(succs);

  pips_debug(3, "End: path_p=%s\n", bool_to_string(path_p));
  
  return path_p;
}

static void update_partition(control root,
			     list partition,
			     hash_table ancestor_map)
{
  /* Controls in partition may have been copied and may not appear anymore
     in the graph embedding root. The input graph is modified to
     separate clean cycles with one entry-exit node, but Bourdoncle's
     algorithm is not aware of this.  */
  list embedding_nodes = node_to_linked_nodes(root);
  int changes = 0;
  int eliminations = 0;
  list eliminated = NIL;
  
  pips_assert("The head is in the partition", gen_in_list_p(root, partition));

  ifdebug(2) {
    pips_debug(2, "Begin for partition:\n");
    print_control_nodes(partition);
  }
  
  MAPL(c_c,
  {
    control c = CONTROL(CAR(c_c));
    pips_assert("c is not a meaningless control node",
		!meaningless_control_p(c));
    
    if(!gen_in_list_p(c, embedding_nodes)) {
      /* Find a replacement for c, and hope to find only one! */
      control a = control_to_ancestor(c, ancestor_map);
      list replacement_list = NIL;

      changes++;

      MAP(CONTROL, c_new, 
      {
	if(!meaningless_control_p(c_new)) {
	  control a_new = control_to_ancestor(c_new, ancestor_map);
	  if(a==a_new) {
	    replacement_list = CONS(CONTROL, c_new, replacement_list);
	  }
	}
	
      }
	  , embedding_nodes);

      switch(gen_length(replacement_list)) {
      case 0: 
	/* pips_internal_error("No replacement for node %p\n", c); */
	/* This node must be eliminated since it is no longer part of the
           partition, probably because it has disappeared in an inner
           cycle. */
	eliminated = CONS(CONTROL, CONTROL(CAR(c_c)), eliminated);
	break;
      case 1:
	CONTROL(CAR(c_c)) = CONTROL(CAR(replacement_list));
	gen_free_list(replacement_list);
	break;
      default:
	/* Many possible replacements... Keep them all. */
	CONTROL(CAR(c_c)) = CONTROL(CAR(replacement_list));
	replacement_list = gen_nconc(replacement_list, CDR(c_c));
	CDR(c_c) = CDR(replacement_list);
	CDR(replacement_list) = NIL;
	gen_free_list(replacement_list);
	
	/*
	fprintf(stderr,
		       "Too many replacement nodes (%d) in embedding nodes for node %p\n",
		       gen_length(replacement_list), c);
	print_control_node(c);
	print_control_nodes(replacement_list);
	pips_internal_error("Too many replacement nodes\n");
	*/
	break;
      }
      
    }
  }
       , partition);

  ifdebug(2) {
    if(changes==0) {
      pips_debug(2, "No renaming\n");
    }
    else{
      pips_debug(2, "After %d renamings, new partition:\n", changes);
      print_control_nodes(partition);
    }
  }

  eliminations = gen_length(eliminated);

  ifdebug(2) {
    if(eliminations>0) {
      pips_debug(2, "Untranslatable control(s) to be eliminated from partition:\n");
      print_control_nodes(eliminated);
    }
  }

  /* Some nodes may no longer be part of the partition because they only
     belonged to an inner cycle which has already been eliminated. */
  
  MAP(CONTROL, c, {
    if(c==root) {
      pips_assert("There is a path from root to root in partition",
		  partition_successor_p(c, root, partition));
    }
    else {
      if(!partition_successor_p(c, root, partition)){
	eliminated = gen_once(c, eliminated);
      }
    }
  } , partition);

  ifdebug(2) {
    if(eliminations < gen_length(eliminated)) {
      /* Unreachable controls have been found */
      pips_debug(2, "Untranslatable and unreachable control(s) to be eliminated from partition:\n");
      print_control_nodes(eliminated);
    }
  }

  /* It does not make sense to use &partition but... */
  pips_assert("root is the fist element in partition",
	      CONTROL(CAR(partition))==root);
  pips_assert("root is not eliminated",
	      !gen_in_list_p(root, eliminated));
  gen_list_and_not(&partition, eliminated);
  
  gen_free_list(eliminated);
  
  ifdebug(2) {
    if(eliminations==0 && changes==0) {
      pips_debug(2, "End with same partition (no renaming, no elimination)\n");
    }
    else if(eliminations==0){
      pips_debug(2, "End with renamed partition (%d renamings, no elimination)\n",
		 changes);
    }
    else if(changes==0){
      pips_debug(2, "End with reduced partition (no renamings, %d eliminations)\n",
		 eliminations);
    }
    else{
      pips_debug(2, "End with new partition (%d renamings, %d eliminations):\n",
		 changes, eliminations);
      print_control_nodes(partition);
    }
  }
}


list bourdoncle_component(control vertex,
			  hash_table ancestor_map,
			  hash_table scc_map)
{
  list partition = NIL;
  list b_partition = list_undefined; /* bourdoncle partition */
  unstructured u = unstructured_undefined;
  control vertex_ancestor = control_undefined;
  
  ifdebug(2) {
    pips_debug(2, "Begin for node: \n");
    print_control_node(vertex);
  }

  MAP(CONTROL, c, 
  {
    if(get_dfn(c)==0) {
      (void) bourdoncle_visit(c, &partition, ancestor_map, scc_map);
    }
  }
      , control_successors(vertex));
  
  partition = CONS(CONTROL, vertex, partition);
  
  /* Build sub-unstructured associated to vertex and partition */
  /* Re-use copying performed in scc_to_dag() instead
  u = partition_to_unstructured(vertex, partition);
  vertex_ancestor = control_to_ancestor(vertex, ancestor_map);
  hash_put(scc_map, vertex_ancestor, u);
  */

  /* The partition may have to be refreshed because of the previous
     recursive descent and its graph restructuring action. It is assumed
     that vertex is still good because heads are not: however vertex is
     not an ancestor because the input unstructured is copied right away
     by bourdoncle_partition().

     It might be better to recompute the smallest scc including
     vertex... */
  b_partition = gen_copy_seq(partition);
  
  update_partition(vertex, partition, ancestor_map);
  
  /* Update parent unstructured containing vertex and partition, remove
     partition and return a new unstructured with the partition inside,
     except for the initial vertex node which is left in the embedding
     graph */
  u = scc_to_dag(vertex, partition, ancestor_map);

  /* Build sub-unstructured associated to vertex and partition */
  /* u = unlink_partition_and build_unstructured(vertex, partition); */
  vertex_ancestor = control_to_ancestor(vertex, ancestor_map);
  hash_put(scc_map, vertex_ancestor, u);

  ifdebug(2) {
    pips_debug(2, "End with internal partition: ");
    print_control_nodes(partition);
    pips_debug(2, "End with Bourdoncle partition: ");
    print_control_nodes(b_partition);
    pips_debug(2, "End with new nodes:\n");
    /* Do not go down into nested unstructured */
    gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, print_control_node, NULL);
    pips_debug(2, "With entry nodes\n");
    print_control_node(unstructured_control(u));
    pips_debug(2, "And exit node\n");
    print_control_node(unstructured_exit(u));
    pips_debug(2, "End.\n");
  }

  gen_free_list(partition);
  return b_partition;
}


int bourdoncle_visit(control vertex,
		     list * ppartition,
		     hash_table ancestor_map,
		     hash_table scc_map)
{
  int min = 0;
  int head = 0;
  bool loop = FALSE;
  
  vertex_push(vertex);
  num = num+1;
  head = num;
  update_dfn(vertex, num);
  
  MAP(CONTROL, succ, 
  {
    if(get_dfn(succ)==0) {
      min = bourdoncle_visit(succ, ppartition, ancestor_map, scc_map);
    }
    else {
      min = get_dfn(succ);
    }
    if(min<=head) {
      head = min;
      loop = TRUE;
    }
  }
      , control_successors(vertex));

  if (head==get_dfn(vertex)) {
    control e = vertex_pop();

    update_dfn(vertex, LONG_MAX);

    if(loop) {
      while(e!=vertex) {
	update_dfn(e, 0);
	e = vertex_pop();
      }
      *ppartition = gen_nconc(bourdoncle_component(vertex, ancestor_map, scc_map),
			      *ppartition);
    }
    else {
      *ppartition = CONS(CONTROL, vertex, *ppartition);
    }

  }
  
  return head;
}

/*
 * OBSERVERS TO USE THE DATASTRUCTURES BUILT BY BOURDONCLE_PARTITION()
 *
 *
 */

/* scc_map maps either the ancestor node to its scc if the transformers
   are computed without context, or the call site node to its scc if
   cycles are replicated to compute transformers within their context. */

/* In spite of the name, this function can be call with ANY control,
   ancestor or not. An ancestor or a call site are mapped to a defined value. */
unstructured ancestor_cycle_head_to_scc(control a, hash_table scc_map)
{
  unstructured scc_u = unstructured_undefined;

  if((scc_u = (unstructured) hash_get(scc_map, (void *) a))
     == (unstructured) (HASH_UNDEFINED_VALUE))
    scc_u = unstructured_undefined;

  return scc_u;
}

/* Retrieve a scc_u from its head */
unstructured proper_cycle_head_to_scc(control h, hash_table scc_map)
{
  unstructured r_scc_u = unstructured_undefined;

  HASH_MAP(k_c, v_scc_u, {
    unstructured scc_u = (unstructured) v_scc_u;

    if(h==unstructured_control(scc_u)) {
      r_scc_u = scc_u;
      break;
    }
  }, scc_map);

  return r_scc_u;
}

/* This node is a cycle call site. */
bool cycle_head_p(control c, hash_table ancestor_map, hash_table  scc_map)
{
  /* This is correct whether the actual scc associated to c is scc_u or
     not. If a copy of a control is associated to a scc, its ancestors and
     all the others copies also are associated to a scc. */
  bool is_cycle = FALSE;
  control a = control_to_ancestor(c, ancestor_map);
  unstructured scc_u = ancestor_cycle_head_to_scc(a, scc_map);

  if(unstructured_undefined_p(scc_u)) {
    /* we may be in the context sensitive transformer mode */
    scc_u = ancestor_cycle_head_to_scc(c, scc_map);
  }

  is_cycle = !unstructured_undefined_p(scc_u);

  return is_cycle;
}

/* This node is really the head of a cycle (red on daVinci pictures). */
bool proper_cycle_head_p(control c, hash_table  scc_map)
{
  bool is_proper_cycle_head_p = FALSE;

    HASH_MAP(n, u, 
    {
      unstructured scc_u = (unstructured) u;

      if((control)c==unstructured_control(scc_u)) {
	is_proper_cycle_head_p = TRUE;
	break;
      }
    }, scc_map);
  return is_proper_cycle_head_p;
}

/* This node is directly associated to a specific cycle. */
bool direct_cycle_head_p(control c, hash_table  scc_map)
{
  bool is_cycle = FALSE;
  unstructured scc_u = ancestor_cycle_head_to_scc(c, scc_map);

  is_cycle = !unstructured_undefined_p(scc_u);

  return is_cycle;
}

/* The ancestor of this node is associated to a specific cycle. */
unstructured cycle_head_to_scc(control c, hash_table ancestor_map, hash_table  scc_map)
{
  /* either we have a direct connection, or we need to go thru the
     ancestor node */

  unstructured scc_u = ancestor_cycle_head_to_scc(c, scc_map);

  if(unstructured_undefined_p(scc_u)) {
    control a = control_to_ancestor(c, ancestor_map);
    scc_u = ancestor_cycle_head_to_scc(a, scc_map);
  }
  return scc_u;
}

/* useful for non-deterministic control flow graph only */

/* There exists at least one real successor of the requested kind and only
   meaningless successors of the other kind. */
bool one_successor_kind_only_p(control c, bool true_p)
{
  bool one_kind_only_p = FALSE;
  bool is_true_successor_p = TRUE;
  bool real_successor_found_p = FALSE;
  list succs = control_successors(c);

  pips_assert("c is a test", control_test_p(c));

  MAP(CONTROL, s, {
    if(is_true_successor_p && true_p) {
      real_successor_found_p |= !meaningless_control_p(s);
    }
    else {
      one_kind_only_p &= meaningless_control_p(s);
    }
  }, succs);

  one_kind_only_p &= real_successor_found_p;

  return one_kind_only_p;
}

bool true_successors_only_p(control c)
{
  bool true_only_p = FALSE;
  true_only_p = one_successor_kind_only_p(c, TRUE);
  return true_only_p;
}

bool false_successors_only_p(control c)
{
  bool true_only_p = FALSE;
  true_only_p = one_successor_kind_only_p(c, TRUE);
  return true_only_p;
}

static void suppress_statement_reference(control c)
{
  /* The statements were not replicated and are still pointed to by the
     initial unstructured. */
  control_statement(c) = statement_undefined;
}

static void bourdoncle_unstructured_free(unstructured u)
{
  /* Suppress references to statements, but do not go down recursively
     into nested unstructured. */
  gen_multi_recurse(u,
		    statement_domain, gen_false, gen_null,
		    control_domain, gen_true, suppress_statement_reference, NULL);

  free_unstructured(u);
}

void bourdoncle_free(unstructured ndu,
		     hash_table ancestor_map,
		     hash_table scc_map)
{
  /* Do not free the statements pointed by the nodes in ndu or in the scc */
  bourdoncle_unstructured_free(ndu);
  HASH_MAP(k, v, {
    bourdoncle_unstructured_free((unstructured) v);
  }, scc_map);
  hash_table_free(ancestor_map);
  hash_table_free(scc_map);
}
