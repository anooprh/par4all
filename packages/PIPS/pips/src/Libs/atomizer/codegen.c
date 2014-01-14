/*

  $Id$

  Copyright 1989-2014 MINES ParisTech

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
/* -- codegen.c
 *
 * package atomizer :  Alexis Platonoff, juillet 91
 * --
 *
 * Functions that generates the new instructions.
 */

#include "local.h"

/* A hash table to map temporary variables (entities)
 * to memory variables (entities).
 */
extern hash_table MemToTmp;



/*============================================================================*/
/* expression find_tmp_of_exp(expression exp): Returns the expression
 * of the temporary in which the variable of "exp" was loaded.
 * If "exp" is not atomic, or, if "exp" was not yet loaded, this function
 * returns "expression_undefined".
 *
 * This function uses the hash table MemToTmp. For a given memory variable
 * you may have an associated temporary. This association is built when such
 * a variable is loaded for the first time.
 *
 * Called functions :
 *       _ entity_scalar_p() : ri-util/entity.c
 *       _ make_entity_expression() : ri-util/util.c
 */
static expression find_tmp_of_exp(exp)
expression exp;
{
expression ret_exp = expression_undefined;

if(syntax_tag(expression_syntax(exp)) == is_syntax_reference)
  {
  entity exp_ent, tmp_ent;
  exp_ent = reference_variable(syntax_reference(expression_syntax(exp)));
  if(entity_scalar_p(exp_ent))
    {
    tmp_ent = (entity) hash_get(MemToTmp, (char *) exp_ent);
    if(tmp_ent != entity_undefined)
      ret_exp = make_entity_expression(tmp_ent, NIL);
    }
  }
return(ret_exp);
}



/*============================================================================*/
/* void put_stmt_in_Block(statement new_stmt, Block *cb): Puts "new_stmt"
 * at the end of the list "cb->first".
 *
 * First, if "new_stmt" is the first statement generated by the statement
 * being translated (the current statement), then the caracteritics of
 * "new_stmt" are taken from those of the current statement.
 *
 * Second, "new_stmt" is added to the list of statements of "cb".
 */
void put_stmt_in_Block(new_stmt, cb)
statement new_stmt;
Block *cb;
{
statement current_stmt;

/* The current statement is the first element of the list "cb->last". */
current_stmt= STATEMENT(CAR(cb->last));

/* "stmt_generated" is false if the current statement has not produced
 * any statement yet, in which case the new statement gets the "label",
 * "number", "ordering" and "comments" (cf. RI) of the current statement.
 */
if (! cb->stmt_generated)
  {
  entity new_label;
  /* The new statement gets the caracteristic of the current statement,
   * while the latter gets the caracteristics of the new statement.
   */
  new_label = statement_label(new_stmt);
  statement_label(new_stmt) = statement_label(current_stmt);
  statement_number(new_stmt) = statement_number(current_stmt);
  statement_ordering(new_stmt) = statement_ordering(current_stmt);
  statement_comments(new_stmt) = statement_comments(current_stmt);

  statement_label(current_stmt) = new_label;
  statement_number(current_stmt) = STATEMENT_NUMBER_UNDEFINED;
  statement_ordering(current_stmt) = STATEMENT_ORDERING_UNDEFINED;
  statement_comments(current_stmt) = string_undefined;

  /* The current statement has, now, generated at least one statement. */
  cb->stmt_generated = true;
  }

/* The new statement is put just before the current statement in the block.
 * Thus, it is put in last position in the list "cb->first".
 */
cb->first = gen_nconc(cb->first, CONS(STATEMENT, new_stmt, NIL));
}



/*============================================================================*/
/* expression assign_tmp_to_exp(expression exp, Block *cb): Produces an assign
 * statement of "exp" in a temporary (this is a load), puts it in "cb" and
 * returns the expression of the temporary.
 *
 * There are two cases:
 *    1. "exp" is found to be a scalar variable already loaded in a
 *       temporary, then the expression of the temporary is returned.
 *
 *    2. otherwise, we do the following manipulations:
 *        _ Firstly, we compute a new basic of the same kind as "exp".
 *        _ Secondly, we create a new temporary with the basic found above.
 *        _ Thirdly, the new assign statement is created.
 *        _ Fourthly, this new statement is put in the current block ("cb").
 *        _ Finaly, we return the expression of the new temporary.
 *
 * The variable "cb" memorises the information about the block of statements
 * that contains the expressions.
 *
 * Called functions :
 *       _ entity_scalar_p() : ri-util/entity.c
 *       _ make_entity_expression() : ri-util/util.c
 *       _ make_assign_statement() : ri-util/statement.c
 */
expression assign_tmp_to_exp(exp, cb)
expression exp;
Block *cb;
{
expression tmp;
entity tmp_ent, exp_ent = entity_undefined;
basic tmp_basic;
statement new_stmt;

/* If this expression is a scalar variable already loaded in a
 * temporary, then it is unnecessary to loaded it again. The only
 * thing to do is to find the temporary that was used and return it.
 */
if(syntax_tag(expression_syntax(exp)) == is_syntax_reference)
  {
  exp_ent = reference_variable(syntax_reference(expression_syntax(exp)));
  if(entity_scalar_p(exp_ent))
    {
    tmp = find_tmp_of_exp(exp);
    if(tmp != expression_undefined)
      return(tmp);
    }
  }

/* We compute the basic of "exp". */
tmp_basic = basic_of_expression(exp);

/* We create a new temporary with the basic "tmp_basic". */
tmp_ent = make_new_entity(tmp_basic, TMP_ENT);

/* If "exp_ent" is a scalar variable, this temporary is placed in
 * MemToTmp at the key "exp_ent".
 */
if(syntax_tag(expression_syntax(exp)) == is_syntax_reference)
  if(entity_scalar_p(exp_ent))
    hash_put(MemToTmp, (char *) exp_ent, (char *) tmp_ent);

/* We create a new expression with this temporary. */
tmp = make_entity_expression(tmp_ent, NIL);

/* The new assign statement is created ... */
new_stmt = make_assign_statement(tmp, exp);

/* ... and put in the current Block. */
put_stmt_in_Block(new_stmt, cb);

/* The new temporary is returned. */
return(tmp);
}



/* We define a set of constant in order to a more generic function for the
 * insert of the declarations of the new created variables.
 */
#define INTEGER_DECL   "      INTEGER"
#define REAL_DECL      "      REAL"
#define COMPLEX_DECL   "      COMPLEX"
#define LOGICAL_DECL   "      LOGICAL"
#define DOUBLE_DECL    "      DOUBLE PRECISION"
#define CHARACTER_DECL "      CHARACTER"
#define SPACE " "
#define COMA  ","
#define NEWLINE "\n     *"
#define ENDLINE "\nC\n"
#define LINE_LENGHT 68
#define MARGIN_LENGHT 7 

/*============================================================================*/
/* void insert_one_type_declaration(entity mod_entity, list var_to_decl,
 *                                  string type_decl):
 * Inserts in the module "mod_entity" the declarations of the entities in the
 * list "var_to_decl" with the type "type_decl".
 */
void insert_one_type_declaration(mod_entity, var_to_decl, type_decl)
entity mod_entity;
list var_to_decl;
string type_decl;
{
string new_decls;
code mod_code;
list l;
int counter = strlen(type_decl);

if(var_to_decl != NIL)
  {
  /* Type declaration. */
  new_decls = strdup(concatenate(type_decl, (char *) NULL));
  for(l = var_to_decl; l != NIL;)
    {
    entity e = ENTITY(CAR(l));

    /* Lenght of the declaration to insert. */
    int decl_len = strlen(SPACE) + strlen(COMA) + strlen(entity_local_name(e));

    /* If the line is to long ..., we begin a new line!! */
    if( (counter + decl_len) > LINE_LENGHT)
      {
      counter = MARGIN_LENGHT;
      new_decls = strdup(concatenate(new_decls, NEWLINE, (char *) NULL));
      }
    new_decls = strdup(concatenate(new_decls, SPACE,
                                   entity_local_name(e), (char *) NULL));
    l = CDR(l);

    /* We separate the variables between comas. */
    if(l != NIL)
      new_decls = strdup(concatenate(new_decls, COMA, (char *) NULL));

    counter += decl_len;
    }

  new_decls = strdup(concatenate(new_decls, ENDLINE, (char *) NULL));

  mod_code = entity_code(mod_entity);
  code_decls_text(mod_code) = strdup(concatenate(code_decls_text(mod_code),
			                         new_decls, (char *) NULL));
  }
}



/*============================================================================*/
/* void insert_new_declarations(char *mod_name): puts in the declaration
 * text of the module "mod_name" the declarations for the temporary, auxiliary
 * and NLC variables.
 *
 * Called functions :
 *       _ local_name_to_top_level_entity() : ri-util/entity.c
 */
void insert_new_declarations(mod_name)
char *mod_name;
{
extern list integer_entities, real_entities, complex_entities, logical_entities,
	    double_entities, char_entities;

entity mod_entity;

mod_entity = local_name_to_top_level_entity(mod_name);

/* We declare only variables that are not INTEGER or
 * REAL simple precision.
 */
/*
integer_entities = gen_nreverse(integer_entities);
real_entities = gen_nreverse(real_entities);
*/
logical_entities = gen_nreverse(logical_entities);
double_entities = gen_nreverse(double_entities);
complex_entities = gen_nreverse(complex_entities);
char_entities = gen_nreverse(char_entities);

/*
insert_one_type_declaration(mod_entity, integer_entities, INTEGER_DECL);
insert_one_type_declaration(mod_entity, real_entities, REAL_DECL);
*/
insert_one_type_declaration(mod_entity, logical_entities, LOGICAL_DECL);
insert_one_type_declaration(mod_entity, double_entities, DOUBLE_DECL);
insert_one_type_declaration(mod_entity, complex_entities, COMPLEX_DECL);
insert_one_type_declaration(mod_entity, char_entities, CHARACTER_DECL);

gen_free_list(integer_entities);
gen_free_list(real_entities);
gen_free_list(logical_entities);
gen_free_list(double_entities);
gen_free_list(complex_entities);
gen_free_list(char_entities);
}



/*============================================================================*/
/* void store_expression(expression exp, Block *cb): Removes the key of the
 * hash table MemToTmp corresponding to the variable contained in "exp". It
 * means that the value of this variable is no longer equal to the value of
 * the temporary stored in  MemToTmp.
 *
 * Note: "exp" must have a "syntax_tag" equal to "reference".
 *
 * Called functions :
 *       _ entity_scalar_p() : ri-util/entity.c
 *       _ make_entity_expression() : ri-util/util.c
 *       _ make_assign_statement() : ri-util/statement.c
 */
void store_expression(exp, cb)
expression exp;
Block *cb;
{
entity ent;

if(syntax_tag(expression_syntax(exp)) != is_syntax_reference)
  user_error("store_expression", "Expression is not a reference");

ent = reference_variable(syntax_reference(expression_syntax(exp)));
if(entity_scalar_p(ent))
  {
/* entity tmp_ent; */
    
  (void) hash_del(MemToTmp, (char *) ent);

/*
  tmp_ent = (entity) hash_get(MemToTmp, (char *) ent);
  if(tmp_ent != entity_undefined)
    {
    expression tmp;
    tmp = make_entity_expression(tmp_ent, NIL);
    put_stmt_in_Block(make_assign_statement(exp, tmp), cb);
    (void) hash_del(MemToTmp, (char *) ent);
    }
*/
  }
}

