/* Try to simplify the control graph.

   Ronan Keryell, 1995.
   */

/* 	%A% ($Date: 1995/09/20 14:48:33 $, ) version $Revision$, got on %D%, %T% [%P%].\n Copyright (c) �cole des Mines de Paris Proprietary.	 */

#ifndef lint
 char vcid_unspaghettify[] = "%A% ($Date: 1995/09/20 14:48:33 $, ) version $Revision$, got on %D%, %T% [%P%].\n Copyright (c) �cole des Mines de Paris Proprietary.";
#endif /* lint */

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 

#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "text-util.h"
#include "database.h"
#include "misc.h"
#include "pipsdbm.h"
#include "resources.h"
#include "control.h"

                                                       
/* Remove a useless continue in a sequence of control since it is
   typically the kind of useless things generated by the
   controlizer... */
static void
remove_useless_continue_or_empty_code_in_unstructured(unstructured u)
{
   list blocs = NIL;
   list remove_continue_list = NIL;
   
   /* The entry point of the unstructured: */
   control entry_node = unstructured_control(u);
   /* and its exit point: */
   control exit_node = unstructured_exit(u);

   CONTROL_MAP(c,
               {
                  if (get_debug_level() > 0)
                     pips_assert("control inconsistants...",
                                 gen_consistent_p(c));

                  /* Do not remove the exit nor the entry node node
                     since it is boring to relink the entry and exit
                     node... That is not important since there is
                     another pass that fuse the sequences. Dead code
                     elimination should remove these structured
                     CONTINUE afterward... */
                  if (c != exit_node && c != entry_node)
                     if (gen_length(control_successors(c)) == 1) {
                        /* Deal any number of predecessor.

                           There may be also some unreachable
                           continues, that are without predecessors
                           (0)... We want to remove them also. */
                     
                        statement st = control_statement(c);

                        if (empty_statement_or_continue_p(st)) {
                           /* It is some useless code, so put it in
                              the remove list: */
                           remove_continue_list = CONS(CONTROL,
                                                       c,
                                                       remove_continue_list);
                        }
                     }
               },
                  entry_node,
                  blocs);
   gen_free_list(blocs);

   /* Now we have the list of the control node to discard. Do the
      cleaning: */
   MAPL(a_control_list,
        {
           control c = CONTROL(CAR(a_control_list));

           debug(3, "remove_useless_continue_or_empty_code_in_unstructured",
                 "Remove control 0x%x\n", c);           
           remove_a_control_from_an_unstructured(c);
        },
           remove_continue_list);

   gen_free_list(remove_continue_list);
}


/* The controlizer of PIPS seems to have a bug:
   it put an empty successor node to the so-called exit node.
   Correct this fact: */
static void
clean_up_exit_node(unstructured u)
{
   control exit_node = unstructured_exit(u);
   list l = control_successors(exit_node);
   
   if (gen_length(l) == 1) {
      control c = CONTROL(CAR(l));
      pips_assert("clean_up_exit_node",
                  gen_length(control_successors(c)) == 0
                  && empty_statement_or_continue_p(control_statement(c)));

      /* Remove the useless node: */
      control_predecessors(c) = NIL;
      gen_free(c);
      
      /* Now the exit node has no longer a successor: */
      control_successors(exit_node) = NIL;
   }

   pips_assert("clean_up_exit_node",
               gen_length(control_successors(exit_node)) == 0);
}


/* Try to transform each sequence in a single statement instead of a
   list of control nodes: */
static void
fuse_sequences_in_unstructured(unstructured u)
{
   control the_successor;
   list blocs = NIL;

   /* The entry point of the unstructured: */
   control entry_node = unstructured_control(u);
   /* and its exit point: */
   control exit_node = unstructured_exit(u);

   /* To store the list of the controls to fuse we use a mapping since
      we need to keep track of eventual previous fuse on a control: */
   hash_table controls_to_fuse_with_their_successors =
      hash_table_make(hash_pointer, 0);
   
   CONTROL_MAP(c,
               {
                  int number_of_successors_of_the_successor;
                  int number_of_predecessors_of_the_successor;
   
                  if (get_debug_level() > 0)
                     pips_assert("control inconsistants...",
                                 gen_consistent_p(c));
               
                  /* Select a node with only one successor: */      
                  if (gen_length(control_successors(c)) == 1) {
                     the_successor = CONTROL(CAR(control_successors(c)));

                     number_of_successors_of_the_successor = gen_length(control_successors(the_successor));
                     number_of_predecessors_of_the_successor = gen_length(control_predecessors(the_successor));
                     debug(3, "dead_rewrite_unstructured",
                           "(gen_length(control_successors(c)) == 1), number_of_successors_of_the_successor = %d, number_of_predecessors_of_the_successor = %d\n",
                           number_of_successors_of_the_successor,
                           number_of_predecessors_of_the_successor);
                     if (number_of_successors_of_the_successor == 1 &&
                         number_of_predecessors_of_the_successor == 1) {
                        /* Ok, we have found a node in a sequence. */
                        
                        /* Put the control in the fuse list. Since no
                           fusing occurs yet, the address of a control
                           node is itself: */
                        hash_put(controls_to_fuse_with_their_successors,
                                 (char *) c,
                                 (char *) c);
                     }
                  }
                  else {
                     debug(3, "dead_rewrite_unstructured",
                           "(gen_length(control_successors(c)) == %d)\n",
                           gen_length(control_successors(c)));
                  }
               },
                  entry_node,
                  blocs);
   gen_free_list(blocs);

   /* Now we have the list of the control nodes to fuse with their
      successors, do the fusion: */
   HASH_MAP(the_original_control,
            its_adress_now,
            {
               /* Just for fun, the following line gets CPP lost
                  macro `HASH_MAP' used with too many (6) args
               control a_control_to_fuse, its_successor, the_third_successor; */
               control a_control_to_fuse;
               control its_successor;
               control the_third_successor;
               statement st;
               
               /* Find the adress of a control node to fuse even if it
                  has already been fused with a predecessor: */
               a_control_to_fuse = (control) its_adress_now;
               if (get_debug_level() > 0)
                  pips_assert("control inconsistants...",
                              gen_consistent_p(a_control_to_fuse));
               
               its_successor = CONTROL(CAR(control_successors(a_control_to_fuse)));
               if (get_debug_level() > 0)
                  pips_assert("control inconsistants...",
                              gen_consistent_p(its_successor));
               
               the_third_successor = CONTROL(CAR(control_successors(its_successor)));
               if (get_debug_level() > 0)
                  pips_assert("control inconsistants...",
                              gen_consistent_p(the_third_successor));
           
               /* make st with the statements of 2 following nodes: */
               st = make_empty_statement();
               statement_instruction(st) =
                  make_instruction_block(CONS(STATEMENT,
                                              control_statement(a_control_to_fuse),
                                              CONS(STATEMENT,
                                                   control_statement(its_successor), NIL)));
               /* Reconnect the new statement to the node to fuse: */
               control_statement(a_control_to_fuse) = st;

               /* Link the first node with the third one in the forward
                  direction: */
               CONTROL(CAR(control_successors(a_control_to_fuse))) =
                  the_third_successor;
               /* Since the third successor may have more than 1
                  predecessor, we need to update only the link to
                  "its_successor": */
               MAPL(another_control_list,
                    {
                       if (CONTROL(CAR(another_control_list)) == its_successor) {
                          CONTROL(CAR(another_control_list)) =
                             a_control_to_fuse;
                          break;
                       }
                    },
                       control_predecessors(the_third_successor));
               /* If the node "its_successor" is in the fuse list, we
                  want to keep track of its new place, that is in fact
                  fused in "a_control_to_fuse", so that an eventual
                  fusion will use "a_control_to_fuse" instead of
                  "its_successor": */
               if (hash_defined_p(controls_to_fuse_with_their_successors,
                                  (char *) its_successor)) {
                  /* Ok, "its_successor" is a node to fuse or that has
                     been already fused (in this case the following is
                     useless, but anyway...). Thus we keep track of
                     its new location: */
                  hash_update(controls_to_fuse_with_their_successors,
                              (char *) its_successor,
                              (char *) a_control_to_fuse);
               }

               /* If the successor is the exit node, then update
                  exit_node to point to the new one: */
               if (its_successor == exit_node)
                  /* Actually I think it cannot happend according to
                     some previous conditions... */
                  exit_node = a_control_to_fuse;
                  
               /* Now we remove the useless intermediate node
                  "its_successor": */
               control_successors(its_successor) = NIL;
               control_predecessors(its_successor) = NIL;
               control_statement(its_successor) = make_empty_statement();
               gen_free(its_successor);
            },
               controls_to_fuse_with_their_successors);

   /* Update the potentially modified exit node: */
                  /* Actually I think it cannot happend according to
                     some previous conditions... */
   unstructured_exit(u)= exit_node;

   hash_table_free(controls_to_fuse_with_their_successors);
}


/* Take the entry node out the unstructured if it is not useful, such
   as not an IF or a node without predecessor.

   Return TRUE if there is still un unstructured, FALSE if the
   unstructured has been replaced by a structured statement.

   If the first node is taked out, then * new_unstructured_statement
   returns the new statement that own the unstructured, else s. */
bool static
take_out_the_entry_node_of_the_unstructured(statement s,
                                            instruction i,
                                            unstructured u,
                                            statement * new_unstructured_statement)
{
   control entry_node = unstructured_control(u);
   list entry_node_successors = control_successors(entry_node);
   int entry_node_successors_length = gen_length(entry_node_successors);
   *new_unstructured_statement = s;
   
   if (entry_node_successors_length == 2
       || gen_length(control_predecessors(entry_node)) > 0)
      /* Well, this node is useful here since it is an unstructured IF
         or there is a GOTO on it. */
      return TRUE;

   if (entry_node_successors_length == 0) {
      /* In fact the unstructured has only one control node! Transform
         it in a structured statement: */
      statement_instruction(s) =
         make_instruction_block(CONS(STATEMENT,
                                     control_statement(entry_node),
                                     NIL));
      /* Remove the unstructured: */
      control_statement(entry_node) = statement_undefined;
      gen_free(i);
      /* No longer unstructured: */
      return FALSE;
   }
   else {
      /* Take out the entry node: */
      *new_unstructured_statement = make_stmt_of_instr(i);
      statement_instruction(s) =
         make_instruction_block(CONS(STATEMENT,
                                     control_statement(entry_node),
                                     CONS(STATEMENT,
                                          *new_unstructured_statement,
                                          NIL)));
      /* The entry node is now the second node: */
      pips_assert("take_out_the_entry_node_of_the_unstructured",
                  entry_node_successors_length == 1);
      unstructured_control(u) = CONTROL(CAR(entry_node_successors));
      
      discard_a_control_sequence_without_its_statements(entry_node,
                                                        entry_node);
      return TRUE;
   }
}


/* If the unstructured begin and/or end with a sequence, move the
   sequence(s) out of the unstructured and return a new statement with
   an equivalent but more structured code. It returns TRUE if the
   unstructured has disappeared and else FALSE.

   If the unstructured is still here, the statement directly owning it
   is returned in new_unstructured_statement: */

/* Still buggy. No longer used. */
static bool
try_to_structure_the_unstructured(statement s,
                                  instruction i,
                                  unstructured u,
                                  statement * new_unstructured_statement)
{
   control end_of_first_sequence, c;
   list the_successors, the_predecessors;
   list begin_statement_list = NIL;
   list end_statement_list = NIL;
   control begin_of_last_sequence = control_undefined /* no gcc warning */;
   
   /* The entry point of the unstructured: */
   control entry_node = unstructured_control(u);
   /* and its exit point: */
   control exit_node = unstructured_exit(u);

   /* Find the biggest sequence from the begin: */
   if (entry_node == exit_node)
      /* Well, we have an unstructured with one control node, it is
         still a sequence: */
      end_of_first_sequence = entry_node;
   else {
      end_of_first_sequence = control_undefined;
      for(c = entry_node;
          gen_length(the_successors = control_successors(c)) <= 1
             && gen_length(control_predecessors(c)) <= 1;
          c = CONTROL(CAR(the_successors))) {
         end_of_first_sequence = c;
         if (the_successors == NIL)
            /* It is in fact the exit_node: */
            break;
      }
   }
   
   if (end_of_first_sequence != control_undefined)
      /* OK, there is a sequence at the beginning of the unstructured: */
      begin_statement_list =
         generate_a_statement_list_from_a_control_sequence(entry_node,
                                                           end_of_first_sequence);
   
   /* If there is still something in the sequence: */
   if (end_of_first_sequence != exit_node) {
      /* Find the biggest sequence from the end: */
      begin_of_last_sequence = control_undefined;
      for(c = exit_node;
          gen_length(the_predecessors = control_predecessors(c)) == 1
             && gen_length(control_successors(c)) <= 1;
          c = CONTROL(CAR(the_predecessors)))
         begin_of_last_sequence = c;

      /* In fact, an unstructured IF is noted as a control node with 2
         successors, that means that we cannot take the biggest
         sequence since it may move one successor of such an IF and
         PIPS would be out. */
      if (begin_of_last_sequence != control_undefined
          && gen_length(control_successors(c)) == 2) {
         /* OK, it is actually an IF before the end sequence. Try to
            keep a spare node from the sequence. Since the
            controllizer seems to put always a continue as an IF
            successor, it is *seems* sensible. */
         the_successors = control_successors(begin_of_last_sequence);
         
         if (the_successors != NIL)
            /* There is one successor, that is the sequence has at
               least 2 control node. Keep the first node as part as IF
               unstructured: */
            begin_of_last_sequence = CONTROL(CAR(the_successors));
         else
            /* There is only one node in the sequence. It remains in
               the unstructured: */
            begin_of_last_sequence = control_undefined;
      }
      
      if (begin_of_last_sequence != control_undefined)
         /* Then there is a sequence at the end of the unstructured: */
         end_statement_list =
            generate_a_statement_list_from_a_control_sequence(begin_of_last_sequence,
                                                              exit_node);
   }

   /* Now, restructure all the stuff only if there is something to do: */
   if (begin_statement_list != NIL || end_statement_list != NIL) {
      if (end_of_first_sequence == exit_node) {
         /* All the unstructured is a sequence, replace it with the
            equivalent block of statement statement: */
         statement_instruction(s) =
            make_instruction_block(begin_statement_list);

         /* Discard the unstructured instruction: */
         discard_an_unstructured_without_its_statements(u);

         /* Warn that the unstructured no longer exist: */
         *new_unstructured_statement = statement_undefined;
         return TRUE;
      }
      else {
         /* There is still an unstructured, but with one pre- or
            post-sequence: */
         list list_of_the_new_statements;
         /* Put the unstructured in the new statement list: */
         *new_unstructured_statement = make_stmt_of_instr(i);
         
         list_of_the_new_statements = CONS(STATEMENT,
                                           *new_unstructured_statement,
                                           NIL);
         
         if (begin_statement_list != NIL) {
            /* There is a pre-sequence before the unstructured: */

            /* Put the sequence before the unstructured: */
            list_of_the_new_statements = gen_nconc(begin_statement_list,
                                                   list_of_the_new_statements);
            
            /* Update the entry node of the unstructured: */
            unstructured_control(u) =
               CONTROL(CAR(control_successors(end_of_first_sequence)));
            
            /* Clean up the equivalent control sequence: */
            discard_a_control_sequence_without_its_statements(entry_node,
                                                              end_of_first_sequence);
         }

         if (end_statement_list != NIL) {
            /* There is a post-sequence after the unstructured: */
            list_of_the_new_statements = gen_nconc(list_of_the_new_statements,
                                                   end_statement_list);
            
            /* Update the exit node of the unstructured: */
            unstructured_exit(u) =
               CONTROL(CAR(control_predecessors(begin_of_last_sequence)));

            /* Clean up the equivalent control sequence: */
            discard_a_control_sequence_without_its_statements(begin_of_last_sequence, exit_node);
         }

         /* Make the instruction of the old statement with the
            sequence(s) and the stripped-down unstructured: */
         statement_instruction(s) =
            make_instruction_block(list_of_the_new_statements);
         
         return FALSE;
      }
   }
   /* By default the unstructured is not changed, thus return the
      statement owning it: */
   *new_unstructured_statement = s;
   
   return FALSE;
}


/* The exit node is the landing pad of the control graph. But if it is
   not a continue, that means that its statement is a sequence at the
   end of the unstructured and we can take it out of the
   unstructured. */
/* The s, i, u heritage may not be respected at the output. */
void
take_out_the_exit_node_if_not_a_continue(statement s,
                                         instruction i,
                                         unstructured u)
{   
   control exit_node = unstructured_exit(u);
   statement the_control_statement = control_statement(exit_node);
   instruction the_control_instruction =
      statement_instruction(the_control_statement);

   pips_assert("take_out_the_exit_node_if_not_a_continue :"
               "i != statement_instruction(s) || u != instruction_unstructured(i) !",
               i == statement_instruction(s)
               && instruction_unstructured_p(i)
               && u == instruction_unstructured(i));
   
   if (! empty_statement_or_continue_p(the_control_statement)) {
      /* Put an empty exit node and keep the statement for the label : */
      statement_instruction(the_control_statement) =
         make_continue_instruction();
      if (get_debug_level() > 0)
         pips_assert("Statements inconsistants...", gen_consistent_p(the_control_statement));
      /* Replace the unstructured by an unstructured followed by the
         out-keeped instruction: */
      statement_instruction(s) =
         make_instruction_block(CONS(STATEMENT,
                                     make_stmt_of_instr(i),
                                     CONS(STATEMENT,
                                          make_stmt_of_instr(the_control_instruction),
                                          NIL)));            
      if (get_debug_level() > 0)
         pips_assert("Statements inconsistants...", gen_consistent_p(s));
   }
}


/* All the optimization to the unstructured during the bottom-up phase: */
void
unspaghettify_rewrite_unstructured(statement s, instruction i, unstructured u)
{
   statement new_unstructured_statement;
   
   clean_up_exit_node(u);
   
   remove_the_unreachable_controls_of_an_unstructured(u);

   if (get_debug_level() > 0) {
      fprintf(stderr, "dead_rewrite_unstructured after remove_the_unreachable_controls_of_an_unstructured\n");
      print_text(stderr, text_statement(get_current_module_entity(), 0, s));
   }

   remove_useless_continue_or_empty_code_in_unstructured(u);
   
   if (get_debug_level() > 0) {
      fprintf(stderr, "dead_rewrite_unstructured after remove_useless_continue_or_empty_code_in_unstructured\n");
      print_text(stderr, text_statement(get_current_module_entity(), 0, s));
   }
   
   fuse_sequences_in_unstructured(u);

   if (get_debug_level() > 0) {
      fprintf(stderr, "dead_rewrite_unstructured after fuse_sequences_in_unstructured\n");
      print_text(stderr, text_statement(get_current_module_entity(), 0, s));
   }

   if (take_out_the_entry_node_of_the_unstructured(s, i, u, &new_unstructured_statement)) {
      /* If take_out_the_entry_node_of_the_unstructured() has not been
         able to discard the unstructured, go on with some other
         optimizations: */
      if (get_debug_level() > 0) {
         fprintf(stderr, "dead_rewrite_unstructured after take_out_the_entry_node_of_the_unstructured\n");
         print_text(stderr, text_statement(get_current_module_entity(), 0, s));
      }

      take_out_the_exit_node_if_not_a_continue(new_unstructured_statement, i, u);
   }

   if (get_debug_level() > 0) {
      fprintf(stderr, "dead_rewrite_unstructured exit:\n");
      print_text(stderr, text_statement(get_current_module_entity(), 0, s));
   }
}


/* This is the function that is applied on each statement of the code
   in a bottom-up way. Even if we deal with the control graph, that is
   the "unstructured" instruction, we need to deal with the statement
   over the unstructured since the restructuration can move some code
   outside of the unstructured in the statement. */
static void
unspaghettify_rewrite(statement s)
{
   instruction i = statement_instruction(s);

   debug(2, "unspaghettify_rewrite enter", "\n");
   if (get_debug_level() >= 3) {
      fprintf(stderr, "[ The current statement : ]\n");
      print_text(stderr, text_statement(get_current_module_entity(), 0, s));
   }

   if (instruction_unstructured_p(i)) {
      unspaghettify_rewrite_unstructured(s, i, instruction_unstructured(i));
      if (get_debug_level() >= 9) {
         fprintf(stderr, "After dead_rewrite_unstructured:\n");
         print_text(stderr, text_statement(get_current_module_entity(), 0, s));
         fprintf(stderr, "-----\n");
      }
   }
   debug(2, "unspaghettify_rewrite exit", "\n");
}


/* Always go down in the module before doing something: */
static bool
unspaghettify_filter(statement s)
{

   return TRUE;
}


/* The real entry point on unspaghettify: */
void
unspaghettify_statement(statement mod_stmt)
{
   gen_recurse(mod_stmt, statement_domain,
               unspaghettify_filter, unspaghettify_rewrite);
}


/* Try to simplify the control graph of a module by removing useless
   CONTINUEs, unreachable code. Try to structure a little bit more and
   so on. */

void
unspaghettify(char * mod_name)  
{
   statement mod_stmt;

   debug_on("UNSPAGHETTIFY_DEBUG_LEVEL");

   /* Get the true ressource, not a copy. */
   mod_stmt = (statement) db_get_memory_resource(DBR_CODE, mod_name, TRUE);
   set_current_module_statement(mod_stmt);

   set_current_module_entity(local_name_to_top_level_entity(mod_name));
  
   if (get_debug_level() > 0)
      pips_assert("Statements inconsistants...", gen_consistent_p(mod_stmt));

   unspaghettify_statement(mod_stmt);

   if (get_debug_level() > 0)
      pips_assert("Statements inconsistants...", gen_consistent_p(mod_stmt));

   /* Reorder the module, because new statements have been generated. */
   module_body_reorder(mod_stmt);

   if (get_debug_level() > 0)
      pips_assert("Statements inconsistants...", gen_consistent_p(mod_stmt));

   DB_PUT_MEMORY_RESOURCE(DBR_CODE, strdup(mod_name), mod_stmt);

   reset_current_module_statement();
   reset_current_module_entity();

   debug(2,"unspaghettify", "done for %s\n", mod_name);
   debug_off();
}
