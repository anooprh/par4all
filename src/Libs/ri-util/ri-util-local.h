 /* RI-UTIL Library: Functions dealing with and constants related to
    PIPS intermediate representation ri.newgen,
    that are NOT generated by NewGen */

 /* Pvecteur is an external type for NewGen and the ri data structure
  * and is not included in ri.h
  *
  * It is included here to avoid changes in many PIPS modules which
  * use ri-util
  *
  * Idem for type Ppolynome (PB 25/07/90); FI: polynomials are not
  * used in ri.newgen; they should not be mentionned here
  *
  * Idem for type Psysteme (FI 3 November 1990)
  */

#include "boolean.h"
#include "vecteur.h"
#include "contrainte.h"
#include "sc.h"
#include "polynome.h"
#include "word_attachment.h"

#include "text.h"

/* mapping.h inclusion
 *
 * I do that because this file was previously included in genC.h,
 * but the macros defined here use ri types (statement, entity...).
 * three typedef are also included here. ri.h is a prerequisit for 
 * mapping.h.
 *
 * FC, Feb 21, 1994
 */

/* these macros are obsolete! newgen functions (->) should be used
 * instead
 */

#ifndef STATEMENT_MAPPING_INCLUDED
#define STATEMENT_MAPPING_INCLUDED
typedef hash_table statement_mapping;
#define MAKE_STATEMENT_MAPPING() \
     (statement_mapping) hash_table_make(hash_pointer, 0)
#define FREE_STATEMENT_MAPPING(map) \
    (hash_table_free((hash_table) (map)))
#define SET_STATEMENT_MAPPING(map, stat, val) \
    hash_put((hash_table) (map), (char *)(stat), (char *)(val))
#define GET_STATEMENT_MAPPING(map, stat) \
    hash_get((hash_table) (map), (char *) (stat))
#define STATEMENT_MAPPING_COUNT(map) \
    hash_table_entry_count((hash_table) map)
#define STATEMENT_MAPPING_MAP(s, v, code, h) \
        HASH_MAP(s, v, code, h)
/*
 * this warrant BA Macro Compatibility:
 */
#define DEFINE_CURRENT_MAPPING(name, type) \
        GENERIC_CURRENT_MAPPING(name, type, statement)
#endif

#ifndef ENTITY_MAPPING_INCLUDED
#define ENTITY_MAPPING_INCLUDED
typedef hash_table entity_mapping;
#define MAKE_ENTITY_MAPPING() \
    ((entity_mapping) hash_table_make(hash_pointer, 0))
#define FREE_ENTITY_MAPPING(map) \
    (hash_table_free((hash_table) (map)))
#define SET_ENTITY_MAPPING(map, ent, val) \
    hash_put((hash_table) (map), (char *)(ent), (char *)(val))
#define GET_ENTITY_MAPPING(map, ent) \
    hash_get((hash_table) (map), (char *)(ent))
#define ENTITY_MAPPING_COUNT(map) \
    hash_table_entry_count((hash_table) map)
#define ENTITY_MAPPING_MAP(s, v, code, h) \
        HASH_MAP(s, v, code, h)
#endif

#ifndef CONTROL_MAPPING_INCLUDED
#define CONTROL_MAPPING_INCLUDED
typedef hash_table control_mapping;
#define MAKE_CONTROL_MAPPING() \
    ((control_mapping) hash_table_make(hash_pointer, 0))
#define FREE_CONTROL_MAPPING(map) \
    (hash_table_free((hash_table) (map)))
#define SET_CONTROL_MAPPING(map, cont, val) \
    hash_put((hash_table) (map), (char *)(cont), (char *)(val))
#define GET_CONTROL_MAPPING(map, cont) \
    hash_get((hash_table) (map), (char *)(cont))
#define CONTROL_MAPPING_COUNT(map) \
    hash_table_entry_count((hash_table) map)
#define CONTROL_MAPPING_MAP(s, v, code, h) \
        HASH_MAP(s, v, code, h)
#endif

/*  special characters
 */
#define MODULE_SEP ':'
#define MODULE_SEP_STRING ":"
#define LABEL_PREFIX "@"
#define MAIN_PREFIX "%"
#define COMMON_PREFIX "~"
#define BLOCKDATA_PREFIX "&"

/*  constant names
 */
#define BLANK_COMMON_LOCAL_NAME "*BLANK*"
#define DYNAMIC_AREA_LOCAL_NAME "*DYNAMIC*"
#define EMPTY_LABEL_NAME LABEL_PREFIX
#define LIST_DIRECTED_FORMAT_NAME "LIST-DIRECTED"
#define STATIC_AREA_LOCAL_NAME "*STATIC*"


#define TOP_LEVEL_MODULE_NAME "TOP-LEVEL"
#define UNBOUNDED_DIMENSION_NAME "UNBOUNDED-DIMENSION"
#define IMPLIED_DO_NAME "IMPLIED-DO"
#define RETURN_LABEL_NAME "00000"

#define ASSIGN_OPERATOR_NAME "="
#define PLUS_OPERATOR_NAME "+"
#define MINUS_OPERATOR_NAME "-"
#define UNARY_MINUS_OPERATOR_NAME "--"
#define MULTIPLY_OPERATOR_NAME "*"
#define DIVIDE_OPERATOR_NAME "/"
#define POWER_OPERATOR_NAME "**"
#define MODULO_OPERATOR_NAME "MOD"
#define MIN_OPERATOR_NAME "MIN"
#define MIN0_OPERATOR_NAME "MIN0"
#define MAX_OPERATOR_NAME "MAX"
#define MAX0_OPERATOR_NAME "MAX0"


/* FI: intrinsics are defined at a third place after bootstrap and effects!
 *     I guess the name should be defined here and used in table(s) there
 */

#define AND_OPERATOR_NAME ".AND."
#define OR_OPERATOR_NAME ".OR."
#define NOT_OPERATOR_NAME ".NOT."
#define NON_EQUAL_OPERATOR_NAME ".NE."
#define EQUIV_OPERATOR_NAME ".EQV."
#define NON_EQUIV_OPERATOR_NAME ".NEQV."

#define TRUE_OPERATOR_NAME ".TRUE."
#define FALSE_OPERATOR_NAME ".FALSE."

#define GREATER_OR_EQUAL_OPERATOR_NAME ".GE."
#define GREATER_THAN_OPERATOR_NAME ".GT."
#define LESS_OR_EQUAL_OPERATOR_NAME ".LE."
#define LESS_THAN_OPERATOR_NAME ".LT."
#define EQUAL_OPERATOR_NAME ".EQ."

#define CONTINUE_FUNCTION_NAME "CONTINUE"
#define RETURN_FUNCTION_NAME "RETURN"
#define STOP_FUNCTION_NAME "STOP"
#define FORMAT_FUNCTION_NAME "FORMAT"
#define SUBSTRING_FUNCTION_NAME "_SUBSTR"
#define ASSIGN_SUBSTRING_FUNCTION_NAME "_ASSIGN_SUBSTR"

#define MOD_INTRINSIC_NAME "MOD"

/* moved from ricedg-local.h */
#define LOOP_COUNTER_MODULE_NAME "LOOP-COUNTER"
#define DI_VAR_MODULE_NAME "DI-VAR"

/* macros */
#define entity_an_operator_p(e,name) \
  (strcmp(entity_local_name(e), name##_OPERATOR_NAME)==0)
#define entity_a_function_p(e,name) \
  (strcmp(entity_local_name(e), name##_FUNCTION_NAME)==0)

#define ENTITY_CONTINUE_P(e) entity_a_function_p(e, CONTINUE)
#define ENTITY_STOP_P(e) entity_a_function_p(e, STOP)
#define ENTITY_RETURN_P(e) entity_a_function_p(e, RETURN)

#define ENTITY_ASSIGN_P(e) entity_an_operator_p(e, ASSIGN)
#define ENTITY_PLUS_P(e) entity_an_operator_p(e, PLUS)
#define ENTITY_MINUS_P(e) entity_an_operator_p(e, MINUS)
#define ENTITY_UNARY_MINUS_P(e) entity_an_operator_p(e, UNARY_MINUS)
#define ENTITY_MULTIPLY_P(e) entity_an_operator_p(e, MULTIPLY)
#define ENTITY_MODULO_P(e) entity_an_operator_p(e, MODULO)
#define ENTITY_POWER_P(e) entity_an_operator_p(e, POWER)
#define ENTITY_DIVIDE_P(e) entity_an_operator_p(e, DIVIDE)
#define ENTITY_MIN_P(e) entity_an_operator_p(e, MIN)
#define ENTITY_MAX_P(e) entity_an_operator_p(e, MAX)
#define ENTITY_MIN0_P(e) entity_an_operator_p(e, MIN0)
#define ENTITY_MAX0_P(e) entity_an_operator_p(e, MAX0)
#define ENTITY_MIN_OR_MAX_P(e) (ENTITY_MIN_P(e) || ENTITY_MAX_P(e) )

#define ENTITY_AND_P(e) entity_an_operator_p(e, AND)

#define ENTITY_OR_P(e) entity_an_operator_p(e, OR)
#define ENTITY_NOT_P(e) entity_an_operator_p(e, NOT)
#define ENTITY_NON_EQUAL_P(e) entity_an_operator_p(e, NON_EQUAL)
#define ENTITY_EQUIV_P(e) entity_an_operator_p(e, EQUIV)
#define ENTITY_NON_EQUIV_P(e) entity_an_operator_p(e, NON_EQUIV)

#define ENTITY_LOGICAL_OPERATOR_P(e) ( ENTITY_RELATIONAL_OPERATOR_P(e) || \
                                       ENTITY_AND_P(e) || \
                                       ENTITY_OR_P(e) || \
                                       ENTITY_NOT_P(e) || \
                                       ENTITY_NON_EQUAL_P(e) || \
                                       ENTITY_EQUIV_P(e) || \
                                       ENTITY_NON_EQUIV_P(e) )

#define ENTITY_TRUE_P(e) entity_an_operator_p(e, TRUE)
#define ENTITY_FALSE_P(e) entity_an_operator_p(e, FALSE)

#define ENTITY_GREATER_OR_EQUAL_P(e) entity_an_operator_p(e, GREATER_OR_EQUAL)
#define ENTITY_GREATER_THAN_P(e)  entity_an_operator_p(e, GREATER_THAN)
#define ENTITY_LESS_OR_EQUAL_P(e) entity_an_operator_p(e, LESS_OR_EQUAL)
#define ENTITY_LESS_THAN_P(e) entity_an_operator_p(e, LESS_THAN)

#define ENTITY_EQUAL_P(e) entity_an_operator_p(e, EQUAL)

#define ENTITY_RELATIONAL_OPERATOR_P(e) ( \
					 ENTITY_GREATER_OR_EQUAL_P(e) || \
					 ENTITY_GREATER_THAN_P(e) || \
					 ENTITY_LESS_OR_EQUAL_P(e) || \
					 ENTITY_LESS_THAN_P(e) || \
					 ENTITY_NON_EQUAL_P(e) || \
					 ENTITY_EQUAL_P(e) )

#define SPECIAL_COMMON_P(e) \
    ((same_string_p(entity_local_name(e), DYNAMIC_AREA_LOCAL_NAME)) || \
     (same_string_p(entity_local_name(e), STATIC_AREA_LOCAL_NAME)))

/* IO Management
 */
#define ENTITY_NAME_P(e, name)(same_string_p(entity_local_name(e),name))
#define ENTITY_WRITE_P(e) ENTITY_NAME_P(e, "WRITE")
#define ENTITY_REWIND_P(e) ENTITY_NAME_P(e, "REWIND")
#define ENTITY_OPEN_P(e) ENTITY_NAME_P(e, "OPEN")
#define ENTITY_CLOSE_P(e) ENTITY_NAME_P(e, "CLOSE")
#define ENTITY_READ_P(e) ENTITY_NAME_P(e, "READ")
#define ENTITY_BUFFERIN_P(e) ENTITY_NAME_P(e, "BUFFERIN")
#define ENTITY_BUFFEROUT_P(e) ENTITY_NAME_P(e, "BUFFEROUT")
#define ENTITY_ENDFILE_P(e) ENTITY_NAME_P(e, "ENDFILE")
#define ENTITY_IMPLIEDDO_P(e) ENTITY_NAME_P(e, "IMPLIED-DO")
#define ENTITY_FORMAT_P(e) ENTITY_NAME_P(e, "FORMAT")

#define ENTITY_FOUR_OPERATION_P(e) ( ENTITY_PLUS_P(e) || \
                                     ENTITY_MINUS_P(e) || \
				     ENTITY_UNARY_MINUS_P(e) || \
                                     ENTITY_MULTIPLY_P(e) || \
                                     ENTITY_DIVIDE_P(e) )

#define IO_CALL_P(call) io_intrinsic_p(call_function(call))

/*  constant sizes
 */
#define LABEL_SIZE 5
#define INDENTATION 3
#define MAXIMAL_MODULE_NAME_SIZE 36

/*   default values
 */
#define STATEMENT_NUMBER_UNDEFINED (-1)
#define STATEMENT_ORDERING_UNDEFINED (-1)

/* On devrait utiliser Newgen pour cela, mais comme on ne doit pas
   les utiliser directement (mais via statement_less_p), cela devrait
   decourager les temeraires */

#define MAKE_ORDERING(u,s) (u<<16|s)
#define ORDERING_NUMBER(o) (o>>16)
#define ORDERING_STATEMENT(o) (o & 0xffff)

#define NORMALIZE_EXPRESSION(e) \
    ((expression_normalized(e) == normalized_undefined) ? \
         (expression_normalized(e) = NormalizeExpression(e)) : \
         (expression_normalized(e)))

/*   MISC: newgen shorthands
 */
#define entity_declarations(e) (code_declarations(entity_code(e)))

#define effect_system(e) \
	(descriptor_convex_p(effect_descriptor(e))? \
	 descriptor_convex(effect_descriptor(e)) : SC_UNDEFINED)

#define effect_reference(e) \
	 preference_reference(cell_preference(effect_cell(e)))

#define entity_variable_p(e) (type_variable_p(entity_type(e)))

#define make_simple_effect(reference,action,approximation)\
    make_effect(make_cell(is_cell_preference, make_preference(reference)),\
    (action),(approximation), \
    make_descriptor(is_descriptor_none,UU))  

#define make_convex_effect(reference,action,approximation,system)\
    make_effect(make_cell(is_cell_preference, make_preference(reference)),\
    (action),(approximation), \
    make_descriptor(is_descriptor_convex,system))  


#define statement_block_p(stat) \
	(instruction_block_p(statement_instruction(stat)))

#define entity_constant_p(e) (type_functional_p(entity_type(e)) && \
  storage_rom_p(entity_storage(e)) && value_constant_p(entity_initial(e)))

/* building instruction and statements...
 */
#define instruction_to_statement(i) \
   make_statement(entity_empty_label(),\
		  STATEMENT_NUMBER_UNDEFINED, STATEMENT_ORDERING_UNDEFINED,\
		  empty_comments, i)

#define loop_to_instruction(l) make_instruction(is_instruction_loop, l)
#define test_to_instruction(t) make_instruction(is_instruction_test, t)
#define call_to_instruction(c) make_instruction(is_instruction_call, c)

#define loop_to_statement(l) instruction_to_statement(loop_to_instruction(l))
#define test_to_statement(t) instruction_to_statement(test_to_instruction(t))
#define call_to_statement(c) instruction_to_statement(call_to_instruction(c))


/***************************************************** BOOLEAN EXPRESSIONS */
/* Building quickly boolean expressions, FC.
 */

#define unary_intrinsic_expression(name, e)\
 call_to_expression(make_call(entity_intrinsic(name),CONS(EXPRESSION,e,NIL)))

#define binary_intrinsic_expression(name, e1, e2)\
 call_to_expression(make_call(entity_intrinsic(name),\
 CONS(EXPRESSION, e1, CONS(EXPRESSION, e2, NIL))))
  
#define not_expression(e) \
    unary_intrinsic_expression(NOT_OPERATOR_NAME, e)
#define or_expression(e1, e2) \
    binary_intrinsic_expression(OR_OPERATOR_NAME, e1, e2)
#define and_expression(e1, e2) \
    binary_intrinsic_expression(AND_OPERATOR_NAME, e1, e2)
#define ne_expression(e1, e2) \
    binary_intrinsic_expression(NON_EQUAL_OPERATOR_NAME, e1, e2)
#define eq_expression(e1, e2) \
    binary_intrinsic_expression(EQUAL_OPERATOR_NAME, e1, e2)
#define gt_expression(e1, e2) \
    binary_intrinsic_expression(GREATER_THAN_OPERATOR_NAME, e1, e2)
#define ge_expression(e1, e2) \
    binary_intrinsic_expression(GREATER_OR_EQUAL_OPERATOR_NAME, e1, e2)
#define lt_expression(e1, e2) \
    binary_intrinsic_expression(LESS_THAN_OPERATOR_NAME, e1, e2)
#define le_expression(e1, e2) \
    binary_intrinsic_expression(LESS_OR_EQUAL_OPERATOR_NAME, e1, e2)


/******************************************************** CONTROL GRAPH... */
/* For the control graph modifiers: */

/* To specify the way that remove_a_control_from_a_list_and_relink
   acts: */
enum remove_a_control_from_a_list_and_relink_direction 
{
   /* Put some strange number to avoid random clash as much as
      possible... */
   source_is_predecessor_and_dest_is_successor = 119,
      source_is_successor_and_dest_is_predecessor = -123
      };
typedef enum remove_a_control_from_a_list_and_relink_direction
remove_a_control_from_a_list_and_relink_direction;


/********************************************************* DUMMY VARIABLES */
#define PRIME_LETTER_FOR_VARIABLES	"p"

/* define to build the _dummy and _prime of a variable.
 */
#define GET_DUMMY_VARIABLE_ENTITY(MODULE, NAME, lname)\
entity get_ith_##lname##_dummy(int i)\
    {return(get_ith_dummy(MODULE, NAME, i));}\
entity get_ith_##lname##_prime(int i)\
    {return(get_ith_dummy(MODULE, NAME PRIME_LETTER_FOR_VARIABLES, i));}

/* Constants for some ex-atomizer variable generation */
#define TMP_ENT 1
#define AUX_ENT 2
#define DOUBLE_PRECISION_SIZE 8


/***************************************************** HPF/HPFC DIRECTIVES */

/* moved here because needed by syntax:-(
 */
/* Directive names encoding: HPF_PREFIX + one character.
 * This encoding is achieved thru a sed script that transforms directives 
 * into calls that can be parsed by the PIPS F77 parser. It's a hack but 
 * it greatly reduced the number of lines for directive analysis, and 
 * it allowed quite simply to figure out where the executable directives
 * are in the code.
 * However the syntax allowed in mapping directives is restricted to F77.
 */

/* prefix for spacial directive calls
 */
#define HPF_PREFIX		"HPFC"

/* suffixies for encoded hpf keywords
 */
#define BLOCK_SUFFIX		"K"
#define CYCLIC_SUFFIX		"C"
#define STAR_SUFFIX		"S"

/* suffixes for HPF directives managed by HPFC
 */
#define ALIGN_SUFFIX		"A"
#define REALIGN_SUFFIX		"B"
#define DISTRIBUTE_SUFFIX	"D"
#define REDISTRIBUTE_SUFFIX	"E"
#define INDEPENDENT_SUFFIX	"I"
#define NEW_SUFFIX		"N"
#define REDUCTION_SUFFIX	"R"
#define PROCESSORS_SUFFIX	"P"
#define TEMPLATE_SUFFIX		"T"
#define PURE_SUFFIX		"U"
#define DYNAMIC_SUFFIX		"Y"

/* suffixes for my own (FCD:-) directives.
 * these directives are used to instrument the code. 
 * must be used carefully. may be ignore with some properties.
 */
#define TELL_SUFFIX		"0"
#define SYNCHRO_SUFFIX		"1"
#define TIMEON_SUFFIX		"2"
#define TIMEOFF_SUFFIX		"3"
#define SETBOOL_SUFFIX		"4"
#define SETINT_SUFFIX		"5"
#define HPFCIO_SUFFIX		"6"
#define HOSTSECTION_SUFFIX	"7"
#define DEAD_SUFFIX		"8"
#define FAKE_SUFFIX		"9"

/* property prefix for ignoring FCD directives
 * TIME, SYNCHRO and SET exists.
 */
#define FCD_IGNORE_PREFIX	"HPFC_IGNORE_FCD_"

/* soft block->sequence transition
 */
#ifdef is_instruction_sequence
#define is_instruction_block is_instruction_sequence
#define instruction_block_p(i) instruction_sequence_p(i)
#define instruction_block(i) sequence_statements(instruction_sequence(i))
#endif

/* Default type sizes */

#define DEFAULT_INTEGER_TYPE_SIZE (4)
#define DEFAULT_REAL_TYPE_SIZE (4)
#define DEFAULT_DOUBLEPRECISION_TYPE_SIZE (8)
#define DEFAULT_COMPLEX_TYPE_SIZE (8)
#define DEFAULT_DOUBLECOMPLEX_TYPE_SIZE (16)
#define DEFAULT_LOGICAL_TYPE_SIZE (4)
#define DEFAULT_CHARACTER_TYPE_SIZE (1)

/* Implicit variables to handle IO effetcs */

/* package name for io routines */
#define IO_EFFECTS_PACKAGE_NAME "__IO_EFFECTS"
/* array of Logical UNits; it is more or less handled as the current file pointer */
#define IO_EFFECTS_ARRAY_NAME "LUNS"
/* array of end of file codes */
#define IO_EOF_ARRAY_NAME "END_LUNS"
/* array of error codes for LUNs */
#define IO_ERROR_ARRAY_NAME "ERR_LUNS"
/* size of the unit specifier */
#define IO_EFFECTS_UNIT_SPECIFIER_LENGTH 4

/* Empty comments (i.e. default comments) */

#define empty_comments string_undefined

/* Macro to walk through controls reachable from the entry node of an
 * unstructured. Reachability is defined by successors and predecessors
 * (i.e. the control flow graph is seen as non-directed.
 *
 * A list of control is built. Most of the time, it must be freed
 * on exit from CONTROL_MAP().
 */

/*
#define CONTROL_MAP( ctl, code, c, list ) \
{ \
    cons *_cm_list_init = (list) ; \
    cons *_cm_list = _cm_list_init ; \
    if( _cm_list == NIL ) {\
	 get_blocs( c, &_cm_list ) ; \
         _cm_list = gen_nreverse( _cm_list ) ; \
    }\
    MAPL( _cm_ctls, {control ctl = CONTROL( CAR( _cm_ctls )) ; \
 \
		 code ;}, \
	  _cm_list ) ; \
   if( _cm_list_init == NIL ) \
        list = _cm_list ; \
}
*/

#define CONTROL_MAP( ctl, code, c, list ) \
    GENERIC_CONTROL_MAP( control_map_get_blocs, ctl, code, c, list ) 

#define FORWARD_CONTROL_MAP( ctl, code, c, list ) \
    GENERIC_CONTROL_MAP( forward_control_map_get_blocs, ctl, code, c, list ) 

#define GENERIC_CONTROL_MAP( get_controls, ctl, code, c, list ) \
{ \
    cons *_cm_list_init = (list) ; \
    cons *_cm_list = _cm_list_init ; \
    if( _cm_list == NIL ) {\
	 get_controls( c, &_cm_list ) ; \
         _cm_list = gen_nreverse( _cm_list ) ; \
    }\
    MAPL( _cm_ctls, {control ctl = CONTROL( CAR( _cm_ctls )) ; \
 \
		 code ;}, \
	  _cm_list ) ; \
   if( _cm_list_init == NIL ) \
        list = _cm_list ; \
}

/* that is all for $RCSfile: ri-util-local.h,v $
 */
