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

// strndup are GNU extensions...
#include <stdio.h>
#include <string.h>

#include "genC.h"
#include "text.h"

#include "text-util.h"
#include "misc.h"
#include "properties.h"
#include "linear.h"
#include "ri.h"
#include "effects.h"
#include "ri-util.h"
#include "effects-util.h"
#include "pipsdbm.h"

#include "resources.h"
#include "phases.h"
#include "control.h"
#include "preprocessor.h"


/* High-level functions about modules, using pipsdbm and ri-util and
   some global variables assumed properly set
 */

/* Retrieve all declarations linked to a module, but the local
   variables private to loops. Allocate and build a new list which
   will have to be freed by the caller.

   This function has been implemented twice.

   It may be useless because code_declarations() is supposed to
   contain all module declarations, regardless of where the
   declarations happens.
 */
list module_declarations(entity m)
{
  statement s = get_current_module_statement();
  list dl = gen_copy_seq(code_declarations(value_code(entity_initial(m))));
  dl = gen_nconc(dl, statement_to_declarations(s));

  /* FI: maybe we should also look up the declarations in the compilation unit... */

  ifdebug(9) {
    pips_debug(8, "Current module declarations:\n");
    print_entities(dl);
    fprintf(stderr, "\n");
  }

  return dl;
}

list current_module_declarations()
{
  entity m = get_current_module_entity();
  return module_declarations(m);
}

/* Retrieve the compilation unit containing a module definition.

   The implementation is clumsy.

   It would be nice to memoize the informtion as with
   get_current_module_entity().
*/
entity module_entity_to_compilation_unit_entity(entity m)
{
  entity cu = entity_undefined;

  if(compilation_unit_entity_p(m))
    cu = m;
  else {
    // string aufn = db_get_memory_resource(DBR_USER_FILE, entity_user_name(m), TRUE);
    string aufn = db_get_memory_resource(DBR_USER_FILE, module_local_name(m), TRUE);
    string lufn = strrchr(aufn, '/')+1;

    if(lufn!=NULL) {
      string n = strstr(lufn, PP_C_ED);
      int l = n-lufn;
      string cun = strndup(lufn, l);

      if(static_module_name_p(cun)) {
	string end = strrchr(cun, FILE_SEP_CHAR);
	*(end+1) = '\0';
	cu = local_name_to_top_level_entity(cun);
      }
      else {
	string ncun = strdup(concatenate(cun, FILE_SEP_STRING, NULL));
	cu = local_name_to_top_level_entity(ncun);
	free(ncun);
      }
      free(cun);
    }
    else
      pips_internal_error("Not implemented yet\n");
  }
  pips_assert("cu is a compilation unit", compilation_unit_entity_p(cu));
  return cu;
}

bool language_module_p(entity m, string lid)
{
  bool c_p = FALSE;

  if(entity_module_p(m)) {
    /* FI: does not work with static functions */
    //string aufn = db_get_memory_resource(DBR_USER_FILE, entity_user_name(m), TRUE);
    /* SG: must check if the ressource exist (not always the case) */
    string lname= module_local_name(m);
    if( db_resource_p(DBR_USER_FILE,lname) )
    {
        string aufn = db_get_memory_resource(DBR_USER_FILE, module_local_name(m), TRUE);
        string n = strstr(aufn, lid);

        c_p = (n!=NULL);
    }
    else
        c_p = TRUE; /* SG: be positive ! (needed for Hpfc)*/
  }
  return c_p;
}


/** Test if a module is in C */
bool c_module_p(entity m)
{
  bool c_p = FALSE;
  value v = entity_initial(m);

  if(!value_undefined_p(v)) {
    language l = code_language(value_code(v));
    c_p = language_c_p(l);
    /* Temporary fix for the too many make_unknown_language()... */
    if(language_unknown_p(l))
      c_p = language_module_p(m, PP_C_ED);
  }
  else
    c_p = language_module_p(m, PP_C_ED);

  return c_p;
}


/** Test if a module is in Fortran */
/* Could be better factored in with C case */
bool fortran_module_p(entity m)
{
  /* These two first lines should be replaced by what follows. */
  bool fortran_p =  language_module_p(m, FORTRAN_FILE_SUFFIX);
  return fortran_p;

  /* FI->FC: the code that follows breaks the validation of Hpfc?!? */

  /*bool*/ fortran_p = FALSE;
  value v = entity_initial(m);
  if(!value_undefined_p(v)) {
    fortran_p = language_fortran_p(code_language(value_code(v)));
  }
  else {
    /* If this alternative did not exist, the source code should be
       moved to ri-util*/
    fortran_p =  language_module_p(m, FORTRAN_FILE_SUFFIX);
  }
  return fortran_p;
}

/* Return a list of all variables and functions accessible somewhere in a module. */
list module_entities(entity m)
{
  entity cu = module_entity_to_compilation_unit_entity(m);
  list cudl = gen_copy_seq(code_declarations(value_code(entity_initial(cu))));
  list mdl = module_declarations(m);

  cudl = gen_nconc(cudl, mdl);
  return cudl;
}

/* Based on entity_user_name, but preserves scoping information when
   needed to avoid ambiguity between two or more variables with the
   same name.

   A variable or function name can be used for an external or static
   entity in the compilation unit, it can be used as function
   parameter, and then declared within the function at any scope
   level.

   This function cannot directly be used in an interprocedural setting
   as it is only related to one module.

   It is assumed that no new string is allocated, but a pointer to an
   existing string is returned.

   The current implementation is extremely ineffective, especially for
   a very unlikely, although mystifying, problem.
*/
const char* entity_module_unambiguous_user_name(entity e, entity m)
{
  const char* uan = entity_user_name(e);

  /* No problem in Fortran */
  if(c_module_p(m)) {
    list conflicts = module_entities(m);

    FOREACH(ENTITY, v, conflicts){
	if(v!=e) {
	  const char* vn = entity_user_name(v);
	  if(same_string_p(uan,vn)) {
	    uan = entity_local_name(e);
	    break;
	  }
	}
      }
    gen_free_list(conflicts);
  }

  return (uan);
}

const char* entity_unambiguous_user_name(entity e)
{
  entity m = get_current_module_entity();

  return entity_module_unambiguous_user_name(e, m);
}

/* In interprocedural context, returns the shortest non-ambiguous name
   for a variable. If it is local to the current module, use the user
   name. If not return entity_name(), which is not fully satisfying
   for C variables because it includes scope information.

   Note also that this function assumes the existence of a current module.

   FI: why is this function in preprocessor and not in ri-util?
*/
/* For REGIONS_MODULE_NAME, which could/should be defined in
   ri-util-local.h as it is used to create entities? */
#include "effects-convex.h"
static const char* entity_more_or_less_minimal_name(entity e, bool strict_p)
{
  entity m = get_current_module_entity();
  string mln = module_local_name(m);
  const char* emn = string_undefined;
  string cun = string_undefined; // compilation unit name
  entity cu = entity_undefined; // compilation unit
  list cudl = list_undefined; // compilation unit declaration list
  list mdl = code_declarations(value_code(entity_initial(m)));

  if(c_module_p(m) && !compilation_unit_p(mln)) {
    /* in pipsmake library...*/
    string compilation_unit_of_module(string);
    cun = compilation_unit_of_module(mln);
    cu = global_name_to_entity(TOP_LEVEL_MODULE_NAME, cun);
    cudl = code_declarations(value_code(entity_initial(cu)));
  }

  pips_assert("some current entity", !entity_undefined_p(m));

  /* gen_in_list_p() would/should be sufficient... */
  if (strcmp(mln, entity_module_name(e)) == 0
      || gen_in_list_p(e, mdl)) {
    /* The variable is declared in the current module */
    //return global_name_to_user_name(entity_name(e));
    if(strict_p)
      emn = entity_unambiguous_user_name(e);
    else
      emn = entity_user_name(e);
  }
  else if (!list_undefined_p(cudl) && gen_in_list_p(e, cudl)) {
    /* The variable is declared in the compilation unit of the current
       module */
    //return global_name_to_user_name(entity_name(e));
    if(strict_p)
      emn = entity_unambiguous_user_name(e);
    else
      emn = entity_user_name(e);
  }
  else if (entity_field_p(e)) {
    /* The variable is a union or struct field. No need to
       disambiguate. */
    if(strict_p)
      emn = entity_unambiguous_user_name(e);
    else
      emn = entity_user_name(e);
  }
  else if (entity_formal_p(e)) {
    /* Formal parameters should always be unambiguous? */
    if(strict_p)
      emn = entity_unambiguous_user_name(e);
    else
      emn = entity_user_name(e);
  }
  else if (strstr(entity_module_name(e), DUMMY_PARAMETER_PREFIX)) {
    /* The variable is a dummy parameter entity, used in a function
       declaration */
    if(strict_p)
      emn = entity_local_name(e);
    else {
      /* In analysis results, let's know when dummy parameters are
	 used... */
      //emn = strdup(entity_local_name(e));
      emn = strdup(entity_name(e));
    }
  }
  else if (strcmp(TOP_LEVEL_MODULE_NAME, entity_module_name(e)) == 0) {
    /* The variable is a ??? */
    if(strict_p)
      emn = entity_local_name(e);
    else
      emn = strdup(entity_local_name(e));
  }
  else if (strcmp(REGIONS_MODULE_NAME, entity_module_name(e)) == 0) {
    /* The variable is a PHI entity */
    if(strict_p)
      emn = entity_local_name(e);
    else
      emn = strdup(entity_local_name(e));
  }
  else if (strcmp(POINTS_TO_MODULE_NAME, entity_module_name(e)) == 0) {
    /* The variable is a stub entity for formal and global pointers */
    if(strict_p)
      emn = entity_local_name(e);
    else
      emn = strdup(entity_local_name(e));
  }
  else {
    /* must be used to prettyprint interprocedural information... */
    //entity om = local_name_to_top_level_entity(mln);
    //emn = entity_interprocedural_unambiguous_user_name(e, om);
    if(strict_p)
      emn = entity_name(e);
    else
      emn = strdup(entity_name(e));
  }

  return emn;
}

/* Do preserve scope informations

   This function does not allocate a new string, which implies to keep
   the scope information in the name of extraprocedural variables and
   functions.
 */
const char* entity_minimal_name(entity e)
{
  return entity_more_or_less_minimal_name(e, TRUE);
}

/* Do not preserve scope information

   A new string is allocated.
 */
const char* entity_minimal_user_name(entity e)
{
  return entity_more_or_less_minimal_name(e, FALSE);
}

/* Add an entity to the current's module compilation unit declarations
 * we have to generate its statement if none cerated before
 * due to limitation of pipsmake, it is not always possible to make sure from pipsmake
 * that this ressource is created
 * for example in INLINING (!) we would like to tell pipsmake
 * we need the CODE resource from all module callers
 */
void
AddEntityToCompilationUnit(entity e, entity cu)
{
    statement s = statement_undefined;
    string cum = module_local_name(cu);
    if( c_module_p(cu) ) {
        if(!db_resource_required_or_available_p(DBR_CODE,cum))
        {
	  bool controlizer(string);
            entity tmp = get_current_module_entity();
            statement stmt = get_current_module_statement();
            reset_current_module_entity();
            reset_current_module_statement();
            controlizer(cum);
            if(!entity_undefined_p(tmp))
                set_current_module_entity(tmp);
            if(!statement_undefined_p(stmt))
                set_current_module_statement(stmt);
        }
        s=(statement)db_get_memory_resource(DBR_CODE,cum,TRUE);
    }
    AddLocalEntityToDeclarations(e,cu,s);
    if( c_module_p(cu) ) {
        module_reorder(s);
        db_put_or_update_memory_resource(DBR_CODE,cum,s,TRUE);
        db_touch_resource(DBR_CODE,cum);
    }
}
void
AddEntityToModuleCompilationUnit(entity e, entity module)
{
    entity cu = module_entity_to_compilation_unit_entity(module);
    AddEntityToCompilationUnit(e,cu);
}

static bool recompile_module_removable_entity_p(gen_chunkp obj)
{
    entity e = (entity)obj;
    return same_string_p(entity_module_name(e),get_current_module_name()) && !entity_area_p(e) && !entity_label_p(e) ;
}

/* build a textual representation of the modified module and update db
 */
void
recompile_module(char* module)
{
    entity modified_module = module_name_to_entity(module);
    statement modified_module_statement =
        (statement) db_get_memory_resource(DBR_CODE, module, TRUE);
    bool controlizer(string);

    set_current_module_entity( modified_module );
    set_current_module_statement( modified_module_statement );

    /* build and register textual representation */
    text t = text_module(get_current_module_entity(), modified_module_statement);
    //add_new_module_from_text(module,t,fortran_module_p(modified_module),compilation_unit_of_module(module));
    string dirname = db_get_current_workspace_directory();
    string res = fortran_module_p(modified_module)? DBR_INITIAL_FILE : DBR_C_SOURCE_FILE;
    string filename = db_get_file_resource(res,module,TRUE);
    string fullname = strdup(concatenate(dirname, "/",filename, NULL));
    FILE* f = safe_fopen(fullname,"w");
    print_text(f,t);
    fclose(f);
    DB_PUT_FILE_RESOURCE(res,module,filename);

    /* the module will be reparsed, so fix(=delete) all its previous entites */
    {
        list p = NIL;
        FOREACH(ENTITY, e, entity_declarations(modified_module))
        {
            if( recompile_module_removable_entity_p((gen_chunkp)e))
                gen_clear_tabulated_element((gen_chunk*)e);
            else
                p = CONS(ENTITY,e,p);
        }
        entity_declarations(modified_module) = gen_nreverse(p);
        code_initializations(value_code(entity_initial(modified_module)))=make_sequence(NIL);
    }

    reset_current_module_entity();
    reset_current_module_statement();

    /* the ugliest glue ever produced by SG
     * needed to get all the declarations etc right
     * unfortunetly we do it without calling pipsmake, so handle it by hand
     * signed: SG
     */
    //string compilation_unit_of_module(string);
    //string cu = compilation_unit_of_module(module);
    {
        gen_array_t modules = db_get_module_list_initial_order();
        int n = gen_array_nitems(modules), i;
        for (i=0; i<n; i++)
        {
            string m = gen_array_item(modules, i);
	    if(!db_resource_required_or_available_p(DBR_CODE,m)) {
	      (void) controlizer(m);
	    }
//            if(!db_resource_required_or_available_p(DBR_PRINTED_FILE,m))
//                print_code(m);
        }
    }

    bool parser(string); // Fortran parser
    bool c_parser(string); // C parser
    bool parsing_ok =(fortran_module_p(modified_module)) ? parser(module) : c_parser(module);
    if(!parsing_ok)
        pips_user_error("failed to recompile module");
    if(!controlizer(module))
        pips_user_warning("failed to apply controlizer after recompilation");
}
