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

#include <stdint.h>
#include <stdlib.h>

#include "genC.h"
#include "misc.h"
#include "freia_spoc.h"

#include "linear.h"
#include "pipsdbm.h"

#include "ri.h"
#include "ri-util.h"
#include "properties.h"

// help reduce code size:
#define T true
#define F false
#define cat concatenate

#define NO_POC { { spoc_poc_unused, 0 }, { spoc_poc_unused, 0 } }
#define NO_MES { measure_none, measure_none }

// types used by AIPO parameters
#define TY_INT "int32_t"
#define TY_PIN "int32_t *"
#define TY_UIN "uint32_t"
#define TY_PUI "uint32_t *"

#define NO_PARAM { NULL, NULL, NULL }
// this may be moved into some "hwac_freia.[hc]"
/* !!! there are some underlying assumptions when using this structure:
 * only one of ALU/POC/MES is used per AIPO.
 * only the first of two POC or MES is used per AIPO.
 * ??? conversions between 8/16 bits images are definitely not handled here.
 */
static const freia_api_t FREIA_AIPO_API[] = {
  { "undefined", "?", 0, 0, 0, 0, NO_PARAM, NO_PARAM,
    { 0, NO_POC, alu_unused, NO_MES }
  },
  {
    // ARITHMETIC
    // binary
    AIPO "add", "+", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    // spoc_hw_t
    {
      // parts used...
      spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      // poc[2]
      NO_POC,
      // alu
      alu_add,
      // global measures
      NO_MES
    }
  },
  { AIPO "sub", "-", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_sub_01, NO_MES }
  },
  { AIPO "mul", "*",  1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_mul, NO_MES }
  },
  { AIPO "div", "/", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_div_01, NO_MES }
  },
  { AIPO "addsat", "+s", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_addsat, NO_MES }
  },
  { AIPO "subsat", "-s", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_subsat_01, NO_MES }
  },
  { AIPO "absdiff", "-|", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_abssub_01, NO_MES }
  },
  { AIPO "inf", "<", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_inf_01, NO_MES }
  },
  { AIPO "sup", ">", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_sup_01, NO_MES }
  },
  { AIPO "and", "&", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_and, NO_MES }
  },
  { AIPO "or", "|", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_or, NO_MES }
  },
  { AIPO "xor", "^", 1, 2, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_input_1|spoc_output_0|spoc_alu,
      NO_POC, alu_xor, NO_MES }
  },
  // unary
  { AIPO "not", "!", 1, 1, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_not_0, NO_MES }
  },
  { AIPO "add_const", "+.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_add_0cst, NO_MES }
  },
  { AIPO "inf_const", "<.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_inf_0cst, NO_MES }
  },
  { AIPO "sup_const", ">.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_sup_0cst, NO_MES }
  },
  { AIPO "sub_const", "-.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_sub_0cst, NO_MES }
  },
  { AIPO "and_const", "&.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_and_0cst, NO_MES }
  },
  { AIPO "or_const", "|.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_or_0cst, NO_MES }
  },
  { AIPO "xor_const", "^.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_xor_0cst, NO_MES }
  },
  { AIPO "addsat_const", "+s.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_addsat_0cst, NO_MES }
  },
  { AIPO "subsat_const", "-s.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_subsat_0cst, NO_MES }
  },
  { AIPO "absdiff_const", "-|.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_abssub_0cst, NO_MES }
  },
  { AIPO "mul_const", "*.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_mul_0cst, NO_MES }
  },
  { AIPO "div_const", "/.", 1, 1, 0, 1, NO_PARAM, { TY_INT, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_alu, NO_POC, alu_div_0cst, NO_MES }
  },
  // nullary
  { AIPO "set_constant", "C", 1, 0, 0, 1, NO_PARAM, { TY_INT, NULL, NULL},
    { spoc_output_0|spoc_alu, NO_POC, alu_copy_cst, NO_MES }
  },
  // MISC
  // this one may be ignored?!
  { AIPO "copy", "=", 1, 1, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_input_0|spoc_output_0, NO_POC, alu_unused, NO_MES }
  },
  { // not implemented by SPOC!
    AIPO "cast", "=()", 1, 1, 0, 0, NO_PARAM, NO_PARAM,
    { spoc_nothing, NO_POC, alu_unused, NO_MES }
  },
  { AIPO "threshold", "thr", 1, 1, 0, 3, NO_PARAM, { TY_INT, TY_INT, TY_INT },
    { spoc_input_0|spoc_output_0|spoc_th_0, NO_POC, alu_unused, NO_MES }
  },
  // MORPHO
  { AIPO "erode_6c", "E6", 1, 1, 0, 1, NO_PARAM, { TY_PIN, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_poc_0,
      { { spoc_poc_erode, 6 }, { spoc_poc_unused, 0 } }, alu_unused, NO_MES
    }
  },
  { AIPO "dilate_6c", "D6", 1, 1, 0, 1, NO_PARAM, { TY_PIN, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_poc_0,
      { { spoc_poc_dilate, 6 }, { spoc_poc_unused, 0 } }, alu_unused, NO_MES
    }
  },
  { AIPO "erode_8c", "E8", 1, 1, 0, 1, NO_PARAM, { TY_PIN, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_poc_0,
      { { spoc_poc_erode, 8 }, { spoc_poc_unused, 0 } }, alu_unused, NO_MES
    }
  },
  { AIPO "dilate_8c", "D8", 1, 1, 0, 1,  NO_PARAM, { TY_PIN, NULL, NULL },
    { spoc_input_0|spoc_output_0|spoc_poc_0,
      { { spoc_poc_dilate, 8 }, { spoc_poc_unused, 0 } }, alu_unused, NO_MES
    }
  },
  // MEASURES
  { AIPO "global_min", "min", 0, 1, 1, 0, { TY_PIN, NULL, NULL }, NO_PARAM,
    { spoc_input_0 | spoc_measure_0,
      NO_POC, alu_unused, { measure_min, measure_none }
    }
  },
  { AIPO "global_max", "max", 0, 1, 1, 0, { TY_PIN, NULL, NULL }, NO_PARAM,
    { spoc_input_0 | spoc_measure_0,
      NO_POC, alu_unused, { measure_max, measure_none }
    }
  },
  { AIPO "global_min_coord", "min!", 0, 1, 3, 0,
    { TY_PIN, TY_PIN, TY_PIN }, NO_PARAM,
    { spoc_input_0 | spoc_measure_0,
      NO_POC, alu_unused, { measure_min_coord, measure_none }
    }
  },
  { AIPO "global_max_coord", "max!", 0, 1, 3, 0,
    { TY_PIN, TY_PIN, TY_PIN }, NO_PARAM,
    { spoc_input_0 | spoc_measure_0,
      NO_POC, alu_unused, { measure_max_coord, measure_none }
    }
  },
  { AIPO "global_vol", "vol", 0, 1, 1, 0,
    { TY_PIN, NULL, NULL }, NO_PARAM,
    { spoc_input_0 | spoc_measure_0,
      NO_POC, alu_unused, { measure_vol, measure_none }
    }
  },
  // LINEAR
  // not implemented by SPOC!
  { AIPO "convolution", "conv", 1, 1, 0, 3,
    NO_PARAM, { TY_PIN, TY_UIN, TY_UIN },
    { spoc_nothing, NO_POC, alu_unused, NO_MES }
  },
  // not implemented by SPOC!
  { AIPO "fast_correlation", "corr", 1, 2, 0, 1,
    NO_PARAM, { TY_UIN, NULL, NULL },
    { spoc_nothing, NO_POC, alu_unused, NO_MES }
  },
  // last entry
  { NULL, NULL, 0, 0, 0, 0, NO_PARAM, NO_PARAM,
    { 0, NO_POC, alu_unused, NO_MES } }
};

#define FREIA_AIPO_API_SIZE (sizeof(*FREIA_AIPO_API)/sizeof(freia_api_t))

/* returns the index of the description of an AIPO function
 */
static int hwac_freia_api_index(string function)
{
  const freia_api_t * api;
  for (api = FREIA_AIPO_API; api->function_name; api++)
    if (same_string_p(function, api->function_name))
      return api - FREIA_AIPO_API;
  return -1;
}

/* @returns the description of a FREIA AIPO API function.
 * may be moved elswhere. raise an error if not found.
 */
const freia_api_t * hwac_freia_api(string function)
{
  int index = hwac_freia_api_index(function);
  return index>=0? &FREIA_AIPO_API[index]: NULL;
}

const freia_api_t * get_freia_api(int index)
{
  // pips_assert("index exists", index>=0 && index<(int) FREIA_AIPO_API_SIZE);
  return &FREIA_AIPO_API[index];
}

/********************************************************* ALU CONFIGURATION */

// value to set operation, in same order as enum declaration
static const spoc_alu_op_t ALU_OP[] = {
  { alu_unused, alu_unused, NULL, F, F, F },
  // ADD
  { alu_add, alu_add, "SPOC_ALU_ADD_IN0_IN1", F, T, T },
  { alu_add_0cst, alu_add_1cst, "SPOC_ALU_ADD_IN0_CONST", T, T, F },
  { alu_add_1cst, alu_add_0cst, "SPOC_ALU_ADD_IN1_CONST", T, F, T },
  // ADDSAT
  { alu_addsat, alu_addsat, "SPOC_ALU_ADDSAT_IN0_IN1", F, T, T },
  { alu_addsat_0cst, alu_addsat_1cst, "SPOC_ALU_ADDSAT_IN0_CONST", T, T, F },
  { alu_addsat_1cst, alu_addsat_0cst, "SPOC_ALU_ADDSAT_IN1_CONST", T, F, T },
  // SUB
  { alu_sub_01, alu_sub_10, "SPOC_ALU_SUB_IN0_IN1", F, T, T },
  { alu_sub_10, alu_sub_01, "SPOC_ALU_SUB_IN1_IN0", F, T, T },
  { alu_sub_0cst, alu_sub_1cst, "SPOC_ALU_SUB_IN0_CONST", T, T, F },
  { alu_sub_1cst, alu_sub_0cst, "SPOC_ALU_SUB_IN1_CONST", T, F, T },
  // SUBSAT
  { alu_subsat_01, alu_subsat_10, "SPOC_ALU_SUBSAT_IN0_IN1", F, T, T },
  { alu_subsat_10, alu_subsat_01, "SPOC_ALU_SUBSAT_IN1_IN0", F, T, T },
  { alu_subsat_0cst, alu_subsat_1cst, "SPOC_ALU_SUBSAT_IN0_CONST", T, T, F },
  { alu_subsat_1cst, alu_subsat_0cst, "SPOC_ALU_SUBSAT_IN1_CONST", T, F, T },
  // ABSSUB
  { alu_abssub_01, alu_abssub_10, "SPOC_ALU_ABSSUB_IN0_IN1", F, T, T },
  { alu_abssub_10, alu_abssub_01, "SPOC_ALU_ABSSUB_IN1_IN0", F, T, T },
  { alu_abssub_0cst, alu_abssub_1cst, "SPOC_ALU_ABSSUB_IN0_CONST", T, T, F },
  { alu_abssub_1cst, alu_abssub_0cst, "SPOC_ALU_ABSSUB_IN1_CONST", T, F, T },
  // MUL
  { alu_mul, alu_mul, "SPOC_ALU_MUL_IN0_IN1", F, T, T },
  { alu_mul_0cst, alu_mul_1cst, "SPOC_ALU_MUL_IN0_CONST", T, T, F },
  { alu_mul_1cst, alu_mul_0cst, "SPOC_ALU_MUL_IN1_CONST", T, F, T },
  // DIV
  { alu_div_01, alu_div_10, "SPOC_ALU_DIV_IN0_IN1", F, T, T },
  { alu_div_10, alu_div_01, "SPOC_ALU_DIV_IN1_IN0", F, T, T },
  { alu_div_0cst, alu_div_1cst, "SPOC_ALU_DIV_IN0_CONST", T, T, F },
  { alu_div_1cst, alu_div_0cst, "SPOC_ALU_DIV_IN1_CONST", T, F, T },
  // INF
  { alu_inf_01, alu_inf_01, "SPOC_ALU_INF_IN0_IN1", F, T, T },
  { alu_inf_0cst, alu_inf_1cst, "SPOC_ALU_INF_IN0_CONST", T, T, F },
  { alu_inf_1cst, alu_inf_0cst, "SPOC_ALU_INF_IN1_CONST", T, F, T },
  // SUP
  { alu_sup_01, alu_sup_01, "SPOC_ALU_SUP_IN0_IN1", F, T, T },
  { alu_sup_0cst, alu_sup_1cst, "SPOC_ALU_SUP_IN0_CONST", T, T, F },
  { alu_sup_1cst, alu_sup_0cst, "SPOC_ALU_SUP_IN1_CONST", T, F, T },
  // AND
  { alu_and, alu_and, "SPOC_ALU_AND_IN0_IN1", F, T, T },
  { alu_and_0cst, alu_and_1cst, "SPOC_ALU_AND_IN0_CONST", T, T, F },
  { alu_and_1cst, alu_and_0cst, "SPOC_ALU_AND_IN1_CONST", T, F, T },
  // OR
  { alu_or, alu_or, "SPOC_ALU_OR_IN0_IN1", F, T, T },
  { alu_or_0cst, alu_or_1cst, "SPOC_ALU_OR_IN0_CONST", T, T, F },
  { alu_or_1cst, alu_or_0cst, "SPOC_ALU_OR_IN1_CONST", T, F, T },
  // XOR
  { alu_xor, alu_xor, "SPOC_ALU_XOR_IN0_IN1", F, T, T },
  { alu_xor_0cst, alu_xor_1cst, "SPOC_ALU_XOR_IN0_CONST", T, T, F },
  { alu_xor_1cst, alu_xor_0cst, "SPOC_ALU_XOR_IN1_CONST", T, F, T },
  // NOT
  { alu_not_0, alu_not_1, "SPOC_ALU_NOT_IN0", F, T, F },
  { alu_not_1, alu_not_0, "SPOC_ALU_NOT_IN1", T, F, T },
  // MISC
  { alu_copy_cst, alu_copy_cst, "SPOC_ALU_COPY_CONST", T, F, F }
  // and so on?
};

const spoc_alu_op_t *get_spoc_alu_conf(spoc_alu_t alu)
{
  return &ALU_OP[alu];
}

/* @return a string which describes the type of operation
 * (i.e. the hardware used by the function).
 */
string what_operation(_int type)
{
  switch (type)
  {
  case spoc_type_oth: return "other";
  case spoc_type_nop: return "none";
  case spoc_type_inp: return "input";
  case spoc_type_poc: return "poc";
  case spoc_type_alu: return "alu";
  case spoc_type_mes: return "measure";
  case spoc_type_thr: return "threshold";
  case spoc_type_out: return "output";
  default: return "unexpected value...";
  }
}

/* SPoC: set shape depending on hardware component used by vertex
 */
string what_operation_shape(_int type)
{
  string shape;
  switch (type)
  {
  case spoc_type_oth:
    shape = "shape=none"; break;
  case spoc_type_poc:
    shape = "shape=box"; break;
  case spoc_type_alu:
    shape = "shape=trapezium,orientation=270"; break;
  case spoc_type_thr:
    shape = "shape=parallelogram"; break;
  case spoc_type_mes:
    shape = "shape=diamond"; break;
    // these should not happen
  case spoc_type_out:
  case spoc_type_inp:
  case spoc_type_nop:
  default:
    shape = "shape=circle";
  }
  return shape;
}

/* ??? beurk: I keep the operation as two ints for code regeneration.
 */
void set_operation(const freia_api_t * api, _int * type, _int * id)
{
  pips_assert("no type set", *type == spoc_type_nop);

  // set type which is enough for staging?
  if (api->spoc.used & spoc_alu)
    *type = spoc_type_alu;
  else if (api->spoc.used & (spoc_poc_0|spoc_poc_1))
    *type = spoc_type_poc;
  else if (api->spoc.used & (spoc_th_0|spoc_th_1))
    *type = spoc_type_thr;
  else if (api->spoc.used & (spoc_measure_0|spoc_measure_1))
    *type = spoc_type_mes;
  else
    *type = spoc_type_nop; // e.g. for copy? (cast? convol? ...)

  // set details
  *id = hwac_freia_api_index(api->function_name);
}

/* all is well
 */
static call freia_ok()
{
  // how to build the "FREIA_OK" constant?
  return make_call(local_name_to_top_level_entity("0"), NIL);
}

/* is it an assignment to ignore
 */
bool freia_assignment_p(entity e)
{
  return ENTITY_ASSIGN_P(e) || ENTITY_BITWISE_OR_UPDATE_P(e);
}

/* replace statement contents with call to c, or continue if kill
 */
void hwac_replace_statement(statement s, call newc, bool kill)
{
  instruction old = statement_instruction(s);
  pips_assert("must kill a call", instruction_call_p(old));
  call c = instruction_call(old);
  entity called = call_function(c);
  if (ENTITY_C_RETURN_P(called))
  {
    // replace argument list
    gen_full_free_list(call_arguments(c));
    call_arguments(c) = CONS(expression, call_to_expression(newc), NIL);
  }
  else if (freia_assignment_p(called))
  {
    // new second argument
    list largs = call_arguments(c);
    pips_assert("two arguments to assignment", gen_length(largs)==2);
    expression first = EXPRESSION(CAR(largs));
    free_expression(EXPRESSION(CAR(CDR(largs))));
    gen_free_list(largs), largs = NIL;
    call_arguments(c) =
      CONS(expression, first, CONS(expression, call_to_expression(newc), NIL));
  }
  else
  {
    free_instruction(old);
    if (kill)
    {
      // ??? in C, it is not a do nothing instruction!
      statement_instruction(s) = make_continue_instruction();
      free_call(newc);
    }
    else
    {
      statement_instruction(s) = make_instruction_call(newc);
    }
  }
}

void hwac_kill_statement(statement s)
{
  hwac_replace_statement(s, freia_ok(), true);
}

/* rather approximative?
 */
bool freia_image_variable_p(entity var)
{
  bool is_image = false;
  if (entity_variable_p(var) && entity_scalar_p(var))
  {
    type t = ultimate_type(entity_type(var));
    basic b = variable_basic(type_variable(t));
    if (basic_pointer_p(b))
    {
      t = basic_pointer(b);
      b = variable_basic(type_variable(t));
      is_image = basic_typedef_p(b) &&
	same_string_p(entity_local_name(basic_typedef(b)),
		      "$" FREIA_IMAGE_TYPE);
    }
  }

  pips_debug(8, "%s is%s an image\n", entity_name(var), is_image? "": " not");
  return is_image;
}

/* for "ret = freia_aipo_*()": return the 'ret' variable...
 * return NULL if not an assignment
 */
static entity get_assigned_variable(statement s)
{
  entity assigned = NULL;
  pips_assert("statement is a call", statement_call_p(s));
  call c = instruction_call(statement_instruction(s));
  entity called = call_function(c);
  if (freia_assignment_p(called))
    assigned = expression_to_entity(EXPRESSION(CAR(call_arguments(c))));
  return assigned;
}

/* returns whether the entity is a freia API (AIPO) function.
 */
bool entity_freia_api_p(entity f)
{
  // very partial...
  return strncmp(entity_local_name(f), AIPO, strlen(AIPO))==0;
}

/* @return whether to optimize AIPO call to function for SPoC.
*/
static bool freia_spoc_optimise(entity called)
{
  string fname = entity_local_name(called);
  return !same_string_p(fname, "freia_aipo_convolution") &&
    !same_string_p(fname, "freia_aipo_cast") &&
    !same_string_p(fname, "freia_aipo_fast_correlation");
}

/* returns whether the statement is a FREIA call.
 */
bool freia_statement_aipo_call_p(statement s)
{
  // very partial as well
  instruction i = statement_instruction(s);
  if (instruction_call_p(i)) {
    call c = instruction_call(i);
    entity called = call_function(c);
    if (entity_freia_api_p(called) &&
	// ??? should be take care later?
	freia_spoc_optimise(called))
      return true;
    else if (freia_assignment_p(called))
    {
      list la = call_arguments(c);
      pips_assert("2 arguments to assign", gen_length(la));
      syntax op2 = expression_syntax(EXPRESSION(CAR(CDR(la))));
      if (syntax_call_p(op2))
	return entity_freia_api_p(call_function(syntax_call(op2)))
	  // ??? later?
	  && freia_spoc_optimise(call_function(syntax_call(op2)));
    }
    else if (ENTITY_C_RETURN_P(called))
    {
      list la = call_arguments(c);
      if (gen_length(la)==1) {
	syntax op = expression_syntax(EXPRESSION(CAR(la)));
	if (syntax_call_p(op))
	  return entity_freia_api_p(call_function(syntax_call(op)))
	    // ??? later?
	    && freia_spoc_optimise(call_function(syntax_call(op)));
      }
    }
  }
  return false;
}

#include "effects-generic.h"

/* append simple scalar entities with written/read effects to s
 * scalars assigned to are ignored (return status).
 * no attempt at managing aliasing or the like...
 */
static void set_add_scalars(set s, statement stat, bool written)
{
  effects efs = load_cumulated_rw_effects(stat);
  entity skip = NULL;

  // skip assigned variable of AIPO calls
  if (freia_statement_aipo_call_p(stat))
    skip = get_assigned_variable(stat);

  FOREACH(effect, e, effects_effects(efs))
  {
    if ((written && effect_write_p(e)) || (!written && effect_read_p(e)))
    {
      entity var = reference_variable(effect_any_reference(e));
      if (entity_variable_p(var) && entity_scalar_p(var) &&
	  !freia_image_variable_p(var) && var!=skip)
	set_add_element(s, s, var);
    }
  }
}

/* is there a simple scalar (no image) rw dependency from s to t?
 * when the return value is assigned, I have a problem.
 * WW deps are ignored.
 */
bool freia_simple_scalar_rw_dependency(statement s, statement t)
{
  // pips_assert("distinct statements", s!=t);
  if (s==t || !s || !t) return false;
  // I should really use entity_conflict_p...
  set reads = set_make(set_pointer), writes = set_make(set_pointer);
  set_add_scalars(writes, s, true);
  set_add_scalars(reads, t, false);
  bool rw_dep = set_intersection_p(writes, reads);
  set_free(reads);
  set_free(writes);
  pips_debug(8, "%" _intFMT " %sdependent from %" _intFMT "\n",
	     statement_number(t), rw_dep? "": "in", statement_number(s));
  return rw_dep;
}
