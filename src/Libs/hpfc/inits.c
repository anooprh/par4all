/* Fabien Coelho, June 1993
 *
 * in this file there are functions to generate the 
 * run-time resolution parameters.
 *
 * $RCSfile: inits.c,v $ ($Date: 1996/02/16 12:02:19 $, )
 * version $Revision$,
 */

#include "defines-local.h"

void create_common_parameters_h(file)
FILE* file;
{
    fprintf(file,
	    "      integer\n"
	    "     $     REALNBOFARRAYS,\n"
	    "     $     REALNBOFTEMPLATES,\n"
	    "     $     REALNBOFPROCESSORS,\n"
	    "     $     REALMAXSIZEOFPROCS,\n"
	    "     $     REALMAXSIZEOFBUFFER\n\n"
	    "c\nc parameters\nc\n"
	    "      parameter(REALNBOFARRAYS = %d)\n"
	    "      parameter(REALNBOFTEMPLATES = %d)\n"
	    "      parameter(REALNBOFPROCESSORS = %d)\n"
	    "      parameter(REALMAXSIZEOFPROCS = %d)\n"
	    "      parameter(REALMAXSIZEOFBUFFER = %d)\n",
	    number_of_distributed_arrays(),
	    number_of_templates(),
	    number_of_processors(),
	    max_size_of_processors(),
	    get_int_property("HPFC_BUFFER_SIZE"));
}

/* create_parameters_h
 *
 * to be called after the declaration_with_overlaps() call.
 * it generates the parameters for module.
 */
void create_parameters_h(file, module)
FILE* file;
entity module;
{
    entity newarray = entity_undefined;
    dimension d = dimension_undefined;
    int i;
    list l = list_of_distributed_arrays_for_module(module);

    fprintf(file, "c\nc parameters generated for %s\nc\n",
	    module_local_name(module));

    MAP(ENTITY, array,
    {
	int andim;

	newarray = load_new_node(array);
	andim  = NumberOfDimension(newarray);

	pips_debug(8, "considering array %s (new is %s)\n",
	      entity_name(array), entity_name(newarray));
	
	for (i=1 ; i<=andim ; i++)
	{
	    d = FindIthDimension(newarray, i);
	    
	    /* ??? memory leak from bound_parameter_name
	     */
	    fprintf(file,
		    "      integer \n"
		    "     $    %s,\n"
		    "     $    %s\n"
		    "      parameter(%s = %d)\n"
		    "      parameter(%s = %d)\n",
		    bound_parameter_name(newarray, LOWER, i),
		    bound_parameter_name(newarray, UPPER, i),
		    bound_parameter_name(newarray, LOWER, i),
		    HpfcExpressionToInt(dimension_lower(d)),
		    bound_parameter_name(newarray, UPPER, i),
		    HpfcExpressionToInt(dimension_upper(d)));
	}
    },
	l);

    gen_free_list(l);
}

int max_size_of_processors()
{
    int current_max = 1;

    MAP(ENTITY, e, 
    {
	variable a;
	
	pips_assert("variable", type_variable_p(entity_type(e)));
	a = type_variable(entity_type(e));
	
	current_max = max(current_max,
			  NumberOfElements(variable_dimensions(a)));
    }, 
	list_of_processors());

    return current_max;
}

/* translation from an hpf_newdecl tag to the runtime code expected
 */
static int code_number(t)
tag t;
{
    switch(t)
    {
    case is_hpf_newdecl_none: return 0;
    case is_hpf_newdecl_alpha: return 1;
    case is_hpf_newdecl_beta: return 2;
    case is_hpf_newdecl_gamma: return 3;
    case is_hpf_newdecl_delta: return 4;
    default: 
	pips_internal_error("unexpected hpf_newdecl tag %d\n", t);
    }
    
    return -1; /* just to avoid a gcc warning */
}

void create_init_common_param_for_arrays(file, module)
FILE* file;
entity module;
{
    list l = list_of_distributed_arrays_for_module(module);
    
    fprintf(file, "c\nc Arrays Initializations for %s\nc\n",
	    module_local_name(module));

    MAP(ENTITY, array,
    {
	int an = load_hpf_number(array);
	int nd = NumberOfDimension(array);
	align al = load_hpf_alignment(array);
	entity template = align_template(al);
	int tn = load_hpf_number(template);
	distribute di = load_hpf_distribution(template);
	int i;
	
	/* NODIMA: Number Of  DIMensions of an Array
	 * ATOT: Array TO Template
	 */
	fprintf(file, 
		"c\nc initializing array %s, number %d\nc\n"
		"      NODIMA(%d) = %d\n"
		"      ATOT(%d) = %d\n",
		entity_local_name(array), an, an, nd, an, tn);

	if (dynamic_entity_p(array) && array==load_primary_entity(array))
	{
	    /* The primary entity is the initial mapping ???
	     * not sure... some decision should be involved...
	     */
	    fprintf(file,
		    "      MSTATUS(%d) = %d\n"
		    "      LIVEMAPPING(%d) = .TRUE.", an, an, an);
	}
	
	/* RANGEA: lower, upper, size and declaration, aso
	 */
	i = 1;
	for (i=1; i<=nd; i++)
	{
	    dimension d = entity_ith_dimension(array, i);
	    int lb = HpfcExpressionToInt(dimension_lower(d));
	    int ub = HpfcExpressionToInt(dimension_upper(d));
	    int sz = (ub-lb+1);
	    tag decl = new_declaration_tag(array, i);
	    alignment a = FindAlignmentOfDim(align_alignment(al), i);
	    
	    /* RANGEA contents:
	     *
	     * 1: lower bound
	     * 2: upper bound
	     * 3: size, (2)-(1)+1
	     * 4: new declaration flag
	     */
	    fprintf(file, "\n"
		    "      RANGEA(%d, %d, 1) = %d\n"
		    "      RANGEA(%d, %d, 2) = %d\n"
		    "      RANGEA(%d, %d, 3) = %d\nc\n"
		    "      RANGEA(%d, %d, 4) = %d\n", 
		    an, i, lb, an, i, ub, an, i, sz,
		    an, i, code_number(decl));
	    
	    switch(decl)
	    {
	     case is_hpf_newdecl_none:
		 break;
	     case is_hpf_newdecl_alpha:
		 /*
		  * 5: 1 - lower bound
		  */
		 fprintf(file, "      RANGEA(%d, %d, 5) = %d\n", 
			 an, i, (1-lb));
		 break;
	     case is_hpf_newdecl_beta:
	     {
		 int tdim = alignment_templatedim(a);
		 int procdim = 0;
		 distribution
		     d = FindDistributionOfDim(distribute_distribution(di), 
					       tdim,
					       &procdim);
		 int param = HpfcExpressionToInt(distribution_parameter(d));
		 int rate;
		 int shift;
		 dimension 
		     dim = FindIthDimension(template, tdim);
		 
		 pips_assert("block distribution",
			     style_block_p(distribution_style(d)));
		 
		 rate = HpfcExpressionToInt(alignment_rate(a));
		 shift = (HpfcExpressionToInt(alignment_constant(a)) -
			  HpfcExpressionToInt(dimension_lower(dim)));

		 /* 5: distribution parameter n, 
		  * 6: alignment rate a,
		  * 7: alignment shift, b-t_{m}
		  */
		 fprintf(file, 
			 "      RANGEA(%d, %d, 5) = %d\n"
			 "      RANGEA(%d, %d, 6) = %d\n"
			 "      RANGEA(%d, %d, 7) = %d\n",
			 an, i, param, an, i, rate, an, i, shift);
		 
		 break;
	     }
	     case is_hpf_newdecl_gamma:
	     {
		 int tdim = alignment_templatedim(a);
		 int procdim = 0;
		 distribution
		     d = FindDistributionOfDim(distribute_distribution(di), 
					       tdim,
					       &procdim);
		 int param = HpfcExpressionToInt(distribution_parameter(d));
		 int sc;
		 int no;
		 int shift;
		 dimension 
		     dim = FindIthDimension(template, tdim);
		 entity
		     proc = distribute_processors(di);
		 
		 pips_assert("cyclic distribution",
			     style_cyclic_p(distribution_style(d)));
		 
		 sc = param*SizeOfIthDimension(proc, procdim);
		 shift = (HpfcExpressionToInt(alignment_constant(a)) -
			  HpfcExpressionToInt(dimension_lower(dim)));
		 no = (lb + shift) / sc ;
		 
		 /* 5: distribution parameter n,
		  * 6: cycle length n*p,
		  * 7: initial cycle number,
		  * 8: alignment shift, b-t_{m}
		  */
		 fprintf(file,
			 "      RANGEA(%d, %d, 5) = %d\n"
			 "      RANGEA(%d, %d, 6) = %d\n"
			 "      RANGEA(%d, %d, 7) = %d\n"
			 "      RANGEA(%d, %d, 8) = %d\n", 
			 an, i, param, an, i, sc, an, i, no, an, i, shift);
		 
		 break;
	     }
	     case is_hpf_newdecl_delta:
	     {
		 int tdim = alignment_templatedim(a);
		 int rate = HpfcExpressionToInt(alignment_rate(a));
		 int cst  = HpfcExpressionToInt(alignment_constant(a));
		 int param = HpfcExpressionToInt(distribution_parameter(d));
		 int procdim = 0;
		 int sc;
		 int no;
		 int shift;
		 int chck;		      
		 distribution
		     d = FindDistributionOfDim(distribute_distribution(di), 
					       tdim,
					       &procdim);
		 dimension 
		     templdim = FindIthDimension(template, tdim);
		 entity
		     proc = distribute_processors(di);
		 
		 pips_assert("cyclic distribution",
			     style_cyclic_p(distribution_style(d)));
		 
		 sc = param*SizeOfIthDimension(proc, procdim);
		 shift = (cst - HpfcExpressionToInt(dimension_lower(templdim)));
		 no = (rate*lb + shift) / sc ;
		 chck = iceil(param, abs(rate));
		 
		 /*  5: distribution parameter n
		  *  6: cycle length n*p,
		  *  7: initial cycle number,
		  *  8: alignment shift, b-t_{m}
		  *  9: alignment rate a,
		  * 10: chunck size ceil(n,|a|)
		  */
		 fprintf(file,
			 "      RANGEA(%d, %d, 5) = %d\n"
			 "      RANGEA(%d, %d, 6) = %d\n"
			 "      RANGEA(%d, %d, 7) = %d\n"
			 "      RANGEA(%d, %d, 8) = %d\n"
			 "      RANGEA(%d, %d, 9) = %d\n"
			 "      RANGEA(%d, %d, 10) = %d\n",
			 an, i, param, an, i, sc, an, i, no,
			 an, i, shift, an, i, rate, an, i, chck);
		 break;
	     }
	     default:
		 pips_internal_error("unexpected decl. tag (%d)\n", decl);
	     }
	 }

	 fprintf(file, "\n");

	 /* ALIGN
	  */

	 for(i=1 ; i<=NumberOfDimension(template) ; i++)
	 {
	     alignment
		 a = FindAlignmentOfTemplateDim(align_alignment(al), i);

	      if (a==alignment_undefined)
	      {
		  fprintf(file, "      ALIGN(%d, %d, 1) = INTFLAG\n", an, i);
	      }
	     else
	     {
		 int adim = alignment_arraydim(a);

		 fprintf(file, 
			 "      ALIGN(%d, %d, 1) = %d\n"
			 "      ALIGN(%d, %d, 2) = %d\n"
			 "      ALIGN(%d, %d, 3) = %d\n", 
			 an, i, adim, an, i,
			 adim ? HpfcExpressionToInt(alignment_rate(a)) : 0,
			 an, i, HpfcExpressionToInt(alignment_constant(a)));

	     }
	  }
     },
	 l);

    gen_free_list(l);
}

void create_init_common_param_for_templates(file)
FILE* file;
{
    fprintf(file, "c\nc Templates Initializations\nc\n");

    MAP(ENTITY, template,
    {
	 int tn = load_hpf_number(template);
	 int nd  = NumberOfDimension(template);
	 distribute di = load_hpf_distribution(template);
	 entity proc = distribute_processors(di);
	 int pn = load_hpf_number(proc);
	 int procdim = 1;
	 int tempdim = 1;
	 
	 /* NODIMT: Number Of  DIMensions of a Template
	  * TTOP: Template TO Processors arrangement
	  */
	 fprintf(file,
		 "c\nc initializing template %s, number %d\nc\n"
		 "      NODIMT(%d) = %d\n"
		 "      TTOP(%d) = %d\n", 
		 entity_local_name(template), tn, tn, nd, tn, pn);
	 
	 /* RANGET: lower, upper, size 
	  */
	 MAP(DIMENSION, d,
	 {
	     int lb = HpfcExpressionToInt(dimension_lower(d));
	     int ub = HpfcExpressionToInt(dimension_upper(d));
	     int sz = (ub-lb+1);
	     
	      fprintf(file, "\n"
		      "      RANGET(%d, %d, 1) = %d\n"
		      "      RANGET(%d, %d, 2) = %d\n"
		      "      RANGET(%d, %d, 3) = %d\n",
		      tn, tempdim, lb, tn, tempdim, ub, tn, tempdim, sz);

	      tempdim++;
	  },
	      variable_dimensions(type_variable(entity_type(template))));

	 /* DIST
	  */
	 tempdim = 1;
	 fprintf(file, "\n");
	 MAP(DISTRIBUTION, d,
	 {
	      int param;
	      bool block_case = FALSE;

	      switch(style_tag(distribution_style(d)))
	      {
	      case is_style_none:
		  break;
	      case is_style_block:
		  block_case = TRUE;
	      case is_style_cyclic:
		  param = HpfcExpressionToInt(distribution_parameter(d));
		  if (!block_case) param = -param;
		  fprintf(file, 
			  "      DIST(%d, %d, 1) = %d\n"
			  "      DIST(%d, %d, 2) = %d\n", 
			  tn, procdim, tempdim, tn, procdim, param);
		  procdim++;
		  break;
	      default:
		  pips_internal_error("unexpected style tag\n");
		  break;
	      }

	      tempdim++;
	  },
	      distribute_distribution(di));
     },
	 list_of_templates());
}

void create_init_common_param_for_processors(file)
FILE* file;
{
    fprintf(file, "c\nc Processors Initializations\nc\n");

    MAP(ENTITY, proc,
    {
	int pn = load_hpf_number(proc);
	int nd  = NumberOfDimension(proc);
	int procdim = 1;
	 
	 fprintf(file, "c\nc initializing processors %s, number %d\nc\n",
		 entity_local_name(proc), pn);

	 /* NODIMP: Number Of  DIMensions of a Processors arrangement
	  */
	 fprintf(file, "      NODIMP(%d) = %d\n", pn, nd);
	 
	 /* RANGEP: lower, upper, size 
	  */
	 MAP(DIMENSION, d,
	 {
	      int lb = HpfcExpressionToInt(dimension_lower(d));
	      int ub = HpfcExpressionToInt(dimension_upper(d));
	      int sz = (ub-lb+1);

	      fprintf(file, "\n"
		      "      RANGEP(%d, %d, 1) = %d\n"
		      "      RANGEP(%d, %d, 2) = %d\n"
		      "      RANGEP(%d, %d, 3) = %d\n",
		      pn, procdim, lb, pn, procdim, ub, pn, procdim, sz);
	      
	      procdim++;
	  },
	      variable_dimensions(type_variable(entity_type(proc))));
     },
	 list_of_processors());
}

/*   create_init_common_param (templates and modules)
 */
void create_init_common_param(file)
FILE* file;
{
    create_init_common_param_for_templates(file);
    create_init_common_param_for_processors(file);
}

