#
# $Id$
#

AR	= ar
ARFLAGS	= rv
CC	= cc
CFLAGS	= -O -g
CMKDEP	= -M
LD	= $(CC)
RANLIB	= ranlib
LEX	= flex
LFLAGS	=
FC	= f77
FFLAGS	= -O -g
LINT	= lint
LINTFLAGS= -habxu

# The parser can no longer be compiled with yacc...
YACC	= bison
YFLAGS	= -y

PROTO   = cproto
PRFLAGS    = -evcf2

# end of it!
#
