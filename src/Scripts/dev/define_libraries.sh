#
# $Id$
#
# $Log: define_libraries.sh,v $
# Revision 1.28  1998/04/16 12:03:48  coelho
# pip is part of paf*
#
# Revision 1.27  1998/04/16 09:07:18  coelho
# paf-related libs moved out.
#
# Revision 1.26  1998/04/14 16:08:18  coelho
# newgen added.
#
# Revision 1.25  1997/12/05 11:37:30  coelho
# preprocessor added.
#
# Revision 1.24  1997/11/21 10:23:13  coelho
# polylib added as an independent lib.
#
# Revision 1.23  1997/11/12 13:18:02  coelho
# semantics and control order reversed.
#
# Revision 1.22  1997/08/22 07:11:23  coelho
# forgotten '
#
# Revision 1.21  1997/08/19 14:58:33  coelho
# FPIPS_... variables added.
#
# Revision 1.20  1997/08/18 15:10:43  coelho
# fpips directory added for construction.
#
# Revision 1.19  1997/08/18 12:29:02  coelho
# X11 added explicitely to wpips for linux.
#
# Revision 1.17  1997/08/18 07:43:58  coelho
# *_MAIN added.
#
# Revision 1.16  1997/08/05 14:27:17  coelho
# switched to new generic effects.
#
# Revision 1.15  1997/07/28 17:32:30  coelho
# new effects-* libraries added.
#
# Revision 1.14  1997/06/20 08:45:32  creusil
# added library alias-classes. Nicky.
#
# Revision 1.11  1997/03/27 13:51:20  coelho
# *_DOCS added.
#
# Revision 1.10  1997/03/27 13:39:00  coelho
# fiew more comments.
#
#
# this file define newgen, linear and pips libraries.
# it is to be included by some shell-scripts and makefiles (after a sed).

#
# NEWGEN

NEWGEN_DOCS='doc'
NEWGEN_ORDERED_LIBS='scripts genC'

NEWGEN_LIBS='-lgenC'

#
# C3/LINEAR

LINEAR_DOCS=''

# plint
LINEAR_ORDERED_LIBS='arithmetique vecteur contrainte sc matrice matrix ray_dte sommet sg polynome polylib polyedre sparse_sc union'

# removed because not used by pips: plint

LINEAR_LIBS='-lmatrice -lunion -lpolyedre -lpolylib -lsparse_sc -lsc -lcontrainte -lsg -lsommet -lray_dte -lpolynome -lmatrix -lvecteur -larithmetique'

#
# PIPS

# static ordering for bootstraping PIPS...

# many doc dirs are skipped (would be long?)
PIPS_DOCS='Documentation constants help manl newgen'

PIPS_ORDERED_LIBS='xpomp text-util properties misc ri-util newgen conversion movements pipsdbm comp_sections transformer bootstrap control hyperplane flint syntax prettyprint static_controlize effects effects-generic effects-simple semantics complexity continuation reductions regions effects-convex alias-classes callgraph icfg paf-util pip ricedg array_dfg prgm_mapping scheduling reindexing chains rice transformations hpfc atomizer wp65 preprocessor pipsmake top-level pips tpips wpips fpips'

paf_libs='-lprgm_mapping -lscheduling -lreindexing -larray_dfg -lpaf-util -lstatic_controlize -lpip'

# all libraires for pips
PIPS_LIBS='-ltop-level -lpipsmake -lwp65 -lhpfc -ltransformations -lmovements -lbootstrap -lcallgraph -licfg -lchains -lcomplexity -lconversion -lprettyprint -latomizer -lsyntax -leffects-simple -leffects-convex -leffects-generic -lalias-classes -lcomp_sections -lsemantics -lcontrol -lcontinuation -lrice -lricedg -lpipsdbm -ltransformer -lpreprocessor -lri-util -lproperties -ltext-util -lmisc -lproperties -lreductions -lflint -lnewgen $(NEWGEN_LIBS) $(LINEAR_LIBS) -lm -lrx'


#
# X11

PIPS_X11_ADDED_CPPFLAGS='-I$(X11_ROOT)/include'
PIPS_X11_ADDED_LDFLAGS='-L$(X11_ROOT)/lib'
PIPS_X11_ADDED_LIBS='-lX11'


#
# PIPS

PIPS_MAIN='main_pips.o'


#
# TPIPS

# is this so portable?

TPIPS_ADDED_LIBS='-lreadline -ltermcap'

TPIPS_MAIN='main_tpips.o'


#
# WPIPS

WPIPS_ADDED_CPPFLAGS='-I$(OPENWINHOME)/include -I$(X11_ROOT)/include -Iicons'
WPIPS_ADDED_LDFLAGS='-L$(OPENWINHOME)/lib -L$(X11_ROOT)/lib'
WPIPS_ADDED_LIBS='-lxview -lolgx -lX11'

WPIPS_MAIN='main_wpips.o'


#
# FPIPS

FPIPS_ADDED_CPPFLAGS='$(WPIPS_ADDED_CPPFLAGS)'
FPIPS_ADDED_LDFLAGS='$(WPIPS_ADDED_LDFLAGS)'
FPIPS_ADDED_LIBS='-lpips -ltpips -lwpips $(TPIPS_ADDED_LIBS) $(WPIPS_ADDED_LIBS)'

FPIPS_MAIN='main_fpips.o'

# end of it!
#
