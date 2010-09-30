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
#include <string.h>

#include "genC.h"
#include "text.h"
#include "constants.h"

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
#include "pipsmake.h"
#include "preprocessor.h"

#define FILE_WARNING 							\
 "!\n"									\
 "!     This module was automatically generated by PIPS and should\n"	\
 "!     be updated by the user with READ and WRITE effects on\n"	\
 "!     formal parameters to be useful...\n"				\
 "!\n"

#define C_FILE_WARNING_EFFECT						\
 "//\n"									\
 "//     This module was automatically generated by PIPS and should\n"	\
 "//     be updated by the user with READ and WRITE effects on\n"	\
 "//     formal parameters to be useful...\n"				\
 "//\n"

#define C_FILE_WARNING						\
 "//\n"								\
 "//     This module was automatically generated by PIPS\n"	\
 "//\n"

/* Generate a string as "f42" for the number 42 for example. */
static string 
nth_formal_name(int n) {
  string number = i2a(n);
  string formal_name = strdup(concatenate("f", number, NULL));
  free(number);
  return formal_name;
}


/* Generate an entry for one formal parameter of a stub declaration */
static sentence stub_var_decl(parameter p, int n, bool is_fortran)
{
  sentence result;
  type t = parameter_type(p);

  if(type_variable_p(t)) {
    string name = nth_formal_name(n);

    if(basic_overloaded_p(variable_basic(type_variable(t)))) {
      string comment =
	strdup(concatenate(
			   "!     Unable to determine the type of parameter number ", name, "\n",
			   "!     ", basic_to_string(variable_basic(type_variable(t))),
			   " ", name, "\n", NULL));
      free(name);
      result = make_sentence(is_sentence_formatted, comment);
    }
    else {
      result = make_sentence(is_sentence_unformatted,
			     make_unformatted(string_undefined, 0, 0,
					      gen_make_list(string_domain,
							    strdup(basic_to_string(variable_basic(type_variable(t)))),
							    strdup(" "), name, strdup(is_fortran? "" : ";"), NULL)));
    }
  }
  else if(type_void_p(t)) {
      result = make_sentence(is_sentence_unformatted,
			     make_unformatted(string_undefined, 0, 0,
					      gen_make_list(string_domain,
							    strdup("void"), NULL)));
  }
  else if(type_varargs_p(t)) {
      result = make_sentence(is_sentence_unformatted,
			     make_unformatted(string_undefined, 0, 0,
					      gen_make_list(string_domain,
							    strdup("..."), NULL)));
  }
  else {
    pips_internal_error("Unexpected type tag %d.\n", type_tag(t));
  }
  return result;
}


/* Generate the head of a stub source for a missing source. */
static sentence stub_head(entity f, bool is_fortran)
{
  list ls = NIL;
  type t = entity_type(f);
  functional fu;
  int number, n;

  pips_assert("type is functional", type_functional_p(t));

  fu = type_functional(t);
  n = gen_length(functional_parameters(fu));

  /* is it a subroutine or a function? */
  if(!type_void_p(functional_result(fu)))
    {
      type tf = functional_result(fu);
      pips_assert("result is a variable", type_variable_p(tf));
      ls = CONS(STRING, strdup(is_fortran? " FUNCTION " : " "),
		CONS(STRING,
		     strdup(basic_to_string(variable_basic(type_variable(tf)))),
		     NIL));
    }
  else
    ls = CONS(STRING, strdup(is_fortran? "SUBROUTINE ":"void "), NIL);

  ls = CONS(STRING, strdup(module_local_name(f)), ls);

  if(is_fortran) {
    /* generate the formal parameter list. */
    for(number=1; number<=n; number++)
      ls = CONS(STRING, nth_formal_name(number),
		CONS(STRING, strdup(number==1? "(": ", "), ls));

    /* close if necessary. */
    if (number>1) ls = CONS(STRING, strdup(")"), ls);
  }
  else {
    if(n>=1) {
      type t = parameter_type(PARAMETER(CAR(functional_parameters(fu))));
      if(type_void_p(t)) {
	ls = CONS(STRING, strdup("(void)"), ls);
      }
      else {
	// Assume C and generate the formal parameter list with their types
	for(number=1; number<=n; number++) {
	  ls = CONS(STRING, nth_formal_name(number),
		    CONS(STRING, strdup(number==1? "(": ", "), ls));
	}
	/* close */
	ls = CONS(STRING, strdup(")"), ls);
      }
    }
    else
      ls = CONS(STRING, strdup("()"), ls);
  }
  return make_sentence(is_sentence_unformatted,
		       make_unformatted(string_undefined, 0, 0, gen_nreverse(ls)));
}


/* Generate the text for a missing module.
 */
static text stub_text(entity module, bool is_fortran)
{
    sentence warning, head;
    type t = entity_type(module);
    int n=1;
    list /* of sentence */ ls = NIL;
    text st = text_undefined;

    ifdebug(8) {
      if(!is_fortran) {
	/* FI: The result should be good because pdl wil be set to
	   NIL and the types used for the parameter will simply be
	   declared but not defined */
	text txt = c_text_entity_simple(entity_undefined, module, 0);
	print_text(stderr, txt);
      }
    }

    if (type_undefined_p(t))
	pips_user_error("undefined type for %s\n", entity_name(module));

    if (!type_functional_p(t))
	pips_user_error("non functional type for %s\n", entity_name(module));

    warning = make_sentence(is_sentence_formatted,
			    strdup(is_fortran? FILE_WARNING:C_FILE_WARNING_EFFECT));
    if(is_fortran) {
      head = stub_head(module, is_fortran);

      FOREACH(PARAMETER, p, functional_parameters(type_functional(t))) {
	type t = parameter_type(p);
	if(!type_void_p(t))
	  ls = CONS(SENTENCE, stub_var_decl(p, n++, is_fortran), ls);
      }

      ls = CONS(SENTENCE,
		make_sentence(is_sentence_unformatted,
			      make_unformatted(string_undefined, 0, 0,
					       CONS(STRING, strdup("END"), NIL))), ls);

      ls = CONS(SENTENCE, warning, CONS(SENTENCE, head, gen_nreverse(ls)));

      st = make_text(ls);
    }
    else { /* assume is_C */
	sentence bs = make_sentence(is_sentence_unformatted,
				    make_unformatted(string_undefined, 0, 0,
						     CONS(STRING, strdup("{}"), NIL)));
	const char* name = entity_user_name(module);
	type t = entity_type(module);
	/* FI: I do not know what to use to initialize pdl usefully */
	list pdl = NIL; // each type supporting entity is declared independently
	list pc = generic_c_words_entity(t, CHAIN_SWORD(NIL,name), FALSE, TRUE, pdl);

	// st = c_text_entity_simple(entity_undefined, module, 0);
	st = make_text(NIL);
	ADD_SENTENCE_TO_TEXT(st, make_sentence(is_sentence_unformatted,
					       make_unformatted(NULL, 0, 0, pc)));
	text_sentences(st) = gen_nconc(CONS(SENTENCE, warning, NIL),
					text_sentences(st)) ;
	text_sentences(st) = gen_nconc(text_sentences(st),
				       CONS(SENTENCE, bs, NIL));
    }

    ifdebug(8) {
      if(!is_fortran) {
	print_text(stderr, st);
      }
    }

    return st;
}


/* Generate the text of a compilation unit for a missing C module.
 */
static text compilation_unit_text(entity cu, entity module)
{
    sentence warning = sentence_undefined;
    type t = entity_type(module);
    text md = text_undefined;
    list sel = NIL; // supporting entity list
    list nsel = NIL; // supporting entity list
    list cse = list_undefined;
    // The text output of the compilation unit:
    text cut = make_text(NIL);
    //entity e = entity_undefined;
    list pdl = NIL; // Let's hope it works; else pdl should contain
		    // each type to declare except for the module

    pips_assert("We must be in a C prettyprinter environment",
		prettyprint_language_is_c_p ());

    if (type_undefined_p(t))
	pips_user_error("undefined type for %s\n", entity_name(module));

    if (!type_functional_p(t))
	pips_user_error("non functional type for %s\n", entity_name(module));

    warning = make_sentence(is_sentence_formatted, strdup(C_FILE_WARNING));
    ADD_SENTENCE_TO_TEXT(cut, warning);

    sel = functional_type_supporting_entities(sel, type_functional(t));

    ifdebug(8) {
      pips_debug(8, "Redundant list of supporting entities: ");
      print_entities(sel);
      fprintf(stderr, "\n");
    }

    /* Eliminate multiple occurences. The first one must be preserved
       to preserve the dependencies. Might be more efficient to CONS
       and then to reverse nsel, or even better to update sel. I keep
       the most intuitive version. */
    for(cse = sel; !ENDP(cse); POP(cse)) {
      entity e = ENTITY(CAR(cse));
      if(!gen_in_list_p(e,nsel))
	nsel = gen_nconc(nsel, CONS(ENTITY,e, NIL));
    }
    gen_free_list(sel);
    sel = list_undefined;

    ifdebug(8) {
      pips_debug(8, "List of supporting entities: ");
      print_entities(nsel);
      fprintf(stderr, "\n\n");
    }

    pips_assert("Each entity appears only once", gen_once_p(nsel));

    FOREACH(ENTITY, se, nsel) {
      const char* n = entity_user_name(se);

      /* Do not declare dummy structures, unions and enumerations,
	 which must be part of another declaration, either a typedef
	 or a name structure, union or enumeration. */
      if((strstr(n,DUMMY_ENUM_PREFIX)==NULL) &&
	 (strstr(n,DUMMY_STRUCT_PREFIX)==NULL) &&
	 (strstr(n,DUMMY_UNION_PREFIX)==NULL)) {
	text se_text = c_text_entity(module, se, 0, pdl);

	ifdebug(8) {
	  pips_debug(8, "Add declaration of entity \"\%s\"\n", entity_name(se));
	  print_text(stderr, se_text);
	}

	MERGE_TEXTS(cut, se_text);
      }
      //free(n);
    }

    md = c_text_entity(cu, module, 0, pdl);
    MERGE_TEXTS(cut, md);

    gen_free_list(nsel);

    return cut;
}


/* Add the new resource files associated to a module with its more-or-less
   correct code.

   The idea is to prettyprint the module to some file resources and to
   parse it later in order to have a full-fledge module with all the PIPS
   data structures up-to-date.

   Useful for code generation, out-lining, stub generation...

   Should be checked with different module with the same name... Maybe a
   conflict in WORKSPACE_TMP_SPACE ?
*/
bool
add_new_module_from_text(string module_name,
			 text code_text,
			 bool is_fortran,
					    string compilation_unit_name) {
    boolean success_p = TRUE;
    entity m = local_name_to_top_level_entity(module_name);
    string file_name, dir_name, src_name, full_name, init_name, finit_name;
    /* relative to the current directory */
    FILE * f;
    string res = is_fortran? DBR_INITIAL_FILE : DBR_C_SOURCE_FILE;

    /* For C only: compilation unit cu and compilation unit name cun */
    string cun = string_undefined;
    entity cu = entity_undefined;

    if(entity_undefined_p(m))
    {
      pips_user_error(
	"No entity defined for module %s although it might"
	" have been encountered at a call site\n", module_name);
      return FALSE;
    }

    if(is_fortran) {
      set_prettyprint_language_tag(is_language_fortran);
    } else {
      set_prettyprint_language_tag(is_language_c);
    }

    // Build the coresponding compilation unit for C code
    if(string_undefined_p(compilation_unit_name) ) {
      // Function defined in pipsmake
      cun = compilation_unit_of_module(module_name);

      if(string_undefined_p(cun)) {
          cun = strdup(concatenate(module_name, FILE_SEP_STRING, NULL));
          cu = MakeCompilationUnitEntity(cun);
      }
    }
    else
        cun=strdup(compilation_unit_name);

    /* pips' current directory is just above the workspace
     */
    {
        string cu_real = strdup(cun);
        cu_real[strlen(cu_real)-1]=0;
        asprintf(&file_name,"%s%s",cu_real,is_fortran?FORTRAN_FILE_SUFFIX:PP_C_ED);
        free(cu_real);
    }
    dir_name = db_get_current_workspace_directory();
    asprintf(&src_name,WORKSPACE_TMP_SPACE "/%s",file_name);
    asprintf(&full_name,"%s/%s",dir_name,src_name);
    init_name =
      db_build_file_resource_name(res, entity_local_name(m), is_fortran? FORTRAN_INITIAL_FILE_SUFFIX : C_FILE_SUFFIX);
    asprintf(&finit_name,"%s/%s" ,dir_name,init_name);

    /* Put the code text in the temporary source file */
    db_make_subdirectory(WORKSPACE_TMP_SPACE);
    f = safe_fopen(finit_name, "w");
    print_text(f, code_text);
    safe_fclose(f, finit_name);
    /* A PIPS database may be partly incoherent after a core dump but
       still usable (Cathare 2, FI). So delete a previously finit_name
       file. */
    if(file_exists_p(full_name))
      safe_unlink(full_name);
    /* The initial file is linked to the newly generated temporary file: */
    safe_link(full_name, finit_name);

    /* Add the new generated file as a file resource with its local
     * name...  should only put a new user file, I guess?
     */
    user_log("Registering synthesized file %s\n", file_name );
    DB_PUT_FILE_RESOURCE(res, module_name, strdup(init_name));
    /* The user file dwells in the WORKSPACE_TMP_SPACE */
    DB_PUT_FILE_RESOURCE(DBR_USER_FILE, module_name, strdup(string_undefined_p(compilation_unit_name)?src_name:full_name));

    if(!is_fortran && string_undefined_p(compilation_unit_name) ) { // C is assumed
      /* Add the compilation unit files */
      pips_assert("The compilation unit name is defined", !string_undefined_p(cun));
      user_log("Registering synthesized compilation unit %s\n", file_name);

      init_name = db_build_file_resource_name(res, cun, is_fortran? FORTRAN_INITIAL_FILE_SUFFIX : C_FILE_SUFFIX);
      finit_name = strdup(concatenate(dir_name, "/", init_name, NULL));

      /* Builds the compilation unit stub: it can be empty or include
	 module_name declaration as an extern function.
      */
      text stub = compilation_unit_text(cu, m);

      /* Put it in the source file and link the initial file.
       */
      db_make_subdirectory(WORKSPACE_TMP_SPACE);
      f = safe_fopen(finit_name, "w");
      print_text(f, stub);
      safe_fclose(f, finit_name);
      DB_PUT_FILE_RESOURCE(res, cun, init_name);
      DB_PUT_FILE_RESOURCE(DBR_USER_FILE, cun, strdup(src_name));
    }
    else if(!is_fortran)
        AddEntityToModuleCompilationUnit(m,module_name_to_entity(compilation_unit_name));

    free(file_name), free(dir_name), free(full_name), free(finit_name);
    return success_p;
}


/* Add the new resource files associated to a module with its more-or-less
   correct code.

   The idea is to prettyprint the module to some file resources and to
   parse it later in order to have a full-fledge module with all the PIPS
   structured up-to-date.

   Useful for code generation, out-lining...

   There is still some redundancy with module_name, module and stat
*/
bool
add_new_module(string module_name,
	       entity module,
	       statement stat,
	       bool is_fortran/*,
				  text heading*/) {
  /* Get a text code for the new module */
  text code_text = text_module(module, stat);
  bool ret = add_new_module_from_text(module_name,
				  code_text,
				  is_fortran,string_undefined);
  free_text(code_text);

  return ret;
}


/* Generate a source file for a module, if none available.
 */
static bool missing_file_initializer(string module_name, bool is_fortran)
{
  entity m = local_name_to_top_level_entity(module_name);
  text stub = text_undefined;

  pips_user_warning("no source file for %s: synthetic code is generated\n",
		    module_name);

  if(entity_undefined_p(m)) {
    pips_user_error("No occurence of function \"%s\" has been encountered. "
		    "Code synthesis is not possible\n", module_name);
  }

  /* Builds a stub code text for the missing module */
  if(is_fortran) {
    set_prettyprint_language_tag(is_language_fortran);
  } else {
    set_prettyprint_language_tag(is_language_c);
  }

  stub = stub_text(m, is_fortran);
  bool ret = add_new_module_from_text(module_name,
				  stub,
				  is_fortran,string_undefined);
  free_text(stub);
  return ret;
}


/* Generate a new module by asking some files to the user. Can also
   generate a stub if replied as so by the user. */
static bool
ask_a_missing_file(string module, bool is_fortran)
{
  string file;
  bool ok, cont;
  string res= is_fortran? DBR_INITIAL_FILE : DBR_C_SOURCE_FILE;

  /* Should be simplified... Maybe, but not bugged... */
  do {
    file = user_request("Please enter a file for module %s\n or \"quit\" to abort or \"generate\" to generate a stub\n", module);
    if(file && strcmp(file, "quit")==0) {
      free(file);
      break;
    }
    if (file)
      {
	if (same_string_p(file, "generate"))
	  ok = missing_file_initializer(module,
					string_fortran_filename_p(module));
	else
	  ok = process_user_file(file);
      }
    cont = file && !same_string_p(file, "quit") &&
      !db_resource_p(res, module);
    if(cont)
      pips_user_warning("Module \"%s\" not found in \"%s\".\n"
			"Please type \"quit\" or another file name.\n",
			module, file);
    if (file) free(file);
  } while (cont);
  return db_resource_p(res, module);
}


/* There is no real rule to produce source or user files when there is no
 * corresponding file; so the initializer was introduced by Remi Triolet
 * to deal with source and user files as with any other kind of resources.
 *
 * Acording to the PREPROCESSOR_MISSING_FILE_HANDLING property, a stub
 * definition for the module can be generated or list of file names can be
 * asked.
 */
bool generic_initializer(string module_name, bool is_fortran)
{
    bool success_p = FALSE;
    string missing = get_string_property("PREPROCESSOR_MISSING_FILE_HANDLING");

    if (same_string_p(missing, "error"))
	pips_user_error("no source file for %s (might be an ENTRY point)\n"
			"set PREPROCESSOR_MISSING_FILE_HANDLING"
			" to \"query\" or \"generate\"...\n", module_name);
    else if (same_string_p(missing, "generate")) 
	success_p = missing_file_initializer(module_name, is_fortran);
    else if (same_string_p(missing, "query"))
	success_p = ask_a_missing_file(module_name, is_fortran);
    else 
	pips_user_error("invalid value of property "
			"PREPROCESSOR_MISSING_FILE_HANDLING = \"%s\"",
			missing);

    return success_p;
}


/* Create a module with its related file resources when there is no
   Fortran source for it. */
bool fortran_initializer(string module_name)
{
  return generic_initializer(module_name, TRUE);
}


/* A phase that creates a module with its related file resources when
   there is no Fortran source for it. */
bool initializer(string module_name)
{
  bool res = FALSE;

  debug_on("INITIALIZER_DEBUG_LEVEL");
  res = generic_initializer(module_name, TRUE);
  debug_off();

  return res;
}


/* A phase that creates a module with its related file resources when
   there is no C source for it. */
bool c_initializer(string module_name)
{
  bool res = FALSE;

  debug_on("INITIALIZER_DEBUG_LEVEL");
  res = generic_initializer(module_name, FALSE);
  debug_off();

  return res;
}


