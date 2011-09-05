/*

  $Id: dowhile_to_while.c 14477 2009-07-07 06:21:50Z guelton $

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
#include "genC.h"
#include "linear.h"
#include "ri.h"
#include "ri-util.h"
#include "misc.h"
#include "control.h"
#include "pipsdbm.h"
#include "resources.h"

static
bool dowhile_to_while_walker(statement stmt)
{
	instruction instr = statement_instruction(stmt);
	if( instruction_whileloop_p(instr) )
	{
        statement_instruction(stmt)=instruction_undefined;
		whileloop wl = instruction_whileloop(instr);
		/* it's a do -while loop */
		if( evaluation_after_p(whileloop_evaluation(wl) ) )
		{
			/* do-while -> while-do */
			free_evaluation(whileloop_evaluation(wl));
			whileloop_evaluation(wl)=make_evaluation_before();
			/* push while-do instruction */
			statement duplicated_statement = make_empty_statement();
			/* duplicate while-do statements and push it */
			clone_context cc = make_clone_context(
					get_current_module_entity(),
					get_current_module_entity(),
                                        NIL,
					duplicated_statement
			);
            statement dup = clone_statement( whileloop_body(wl),cc);
			free_clone_context(cc);
            insert_statement(duplicated_statement,dup,false);
            insert_statement(duplicated_statement,instruction_to_statement(instr),false);
			/* see how elegant is the patching ? */
            update_statement_instruction(stmt,
                    make_instruction_block(CONS(STATEMENT,duplicated_statement,NIL))
                    );
		}
	}
    return true;
}

bool
dowhile_to_while(const char* module_name)
{
	/* prelude */
	set_current_module_entity( module_name_to_entity(module_name) );
	set_current_module_statement(
			(statement) db_get_memory_resource(DBR_CODE, module_name, true)
			);

	/* transformation */
  	gen_recurse(get_current_module_statement(), statement_domain, gen_true, &dowhile_to_while_walker);

	/* postlude */
	module_reorder(get_current_module_statement());
	DB_PUT_MEMORY_RESOURCE(DBR_CODE, module_name, get_current_module_statement());

	reset_current_module_statement();
	reset_current_module_entity();

	return true;
}

/* converts a doloop to a while loop, in place */
void do_loop_to_while_loop(statement sl) {
    pips_assert("statement is a loop",statement_loop_p(sl));
    loop l =statement_loop(sl);
    range r = loop_range(l);

    /* convert the loop to a while loop :
     * fst the body
     */
    list statements = make_statement_list(
            copy_statement(loop_body(l)),
            make_assign_statement(
                entity_to_expression(loop_index(l)),
                MakeBinaryCall(
                    entity_intrinsic(PLUS_OPERATOR_NAME),
                    entity_to_expression(loop_index(l)),
                    range_increment(r)
                    )
                )
            );
    /* then the whileloop */
    whileloop wl = make_whileloop(
            MakeBinaryCall(
                entity_intrinsic(LESS_OR_EQUAL_OPERATOR_NAME),
                entity_to_expression(loop_index(l)),
                range_upper(r)
                ),
            make_block_statement(statements),
            entity_empty_label(),
            make_evaluation_before());

    /* and the prelude */
    sequence seq = make_sequence(
            make_statement_list(
                make_assign_statement(entity_to_expression(loop_index(l)),
                    range_lower(r)),
                instruction_to_statement(make_instruction_whileloop(wl))
                )
            );
    range_upper(r)=expression_undefined;
    range_lower(r)=expression_undefined;
    range_increment(r)=expression_undefined;

    update_statement_instruction(sl,make_instruction_sequence(seq));
}

/* converts a doloop to a for loop, in place*/
void do_loop_to_for_loop(statement sl)
{
    pips_assert("statement is a loop",statement_loop_p(sl));
    loop l =statement_loop(sl);
    range r = loop_range(l);

    forloop fl = make_forloop(
		    make_assign_expression(entity_to_expression(loop_index(l)), copy_expression(range_lower(r))),
		    MakeBinaryCall(entity_intrinsic(LESS_OR_EQUAL_OPERATOR_NAME), entity_to_expression(loop_index(l)), copy_expression(range_upper(r))),
		    MakeBinaryCall(entity_intrinsic(PLUS_UPDATE_OPERATOR_NAME), entity_to_expression(loop_index(l)), copy_expression(range_increment(r))),
	            copy_statement(loop_body(l)));
    update_statement_instruction(sl, make_instruction_forloop(fl));
}

