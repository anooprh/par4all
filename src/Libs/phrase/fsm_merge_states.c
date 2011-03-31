/*

  $Id$

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
#include <stdio.h>
#include <ctype.h>

#include "genC.h"
#include "linear.h"
#include "ri.h"
#include "effects.h"

#include "resources.h"

#include "misc.h"
#include "ri-util.h"
#include "effects-util.h"
#include "pipsdbm.h"

#include "text-util.h"


#include "control.h"
#include "callgraph.h"

#include "fsm_generation.h"

/*********************************************************
 * Phase main
 *********************************************************/

bool fsm_merge_states(string module_name)
{
  entity module;
  //entity state_variable;

   /* get the resources */
  statement stat = (statement) db_get_memory_resource(DBR_CODE, 
						      module_name, 
						      TRUE);

  module = module_name_to_entity(module_name);
  
  set_current_module_statement(stat);
  set_current_module_entity(module_name_to_entity(module_name)); // FI: redundant
  
  debug_on("FSM_MERGE_STATES_DEBUG_LEVEL");
  /* Now do the job */
  
  pips_assert("Statement is consistent after FSM_MERGE_STATES", 
	       statement_consistent_p(stat));
  
  /* Reorder the module, because new statements have been added */  
  module_reorder(stat);
  DB_PUT_MEMORY_RESOURCE(DBR_CODE, module_name, stat);
  DB_PUT_MEMORY_RESOURCE(DBR_CALLEES, module_name, 
			 compute_callees(stat));
  
  /* update/release resources */
  reset_current_module_statement();
  reset_current_module_entity();
  
  debug_off();
  
  return TRUE;
}