#
# The following macros define the value of commands that are used to
# compile source code.
#
# you can add your own options behind pips default values.
# 
# example: CFLAGS= $(PIPS_CFLAGS) -DSYSTEM=BSD4.2
#
AR=		$(PIPS_AR)
ARFLAGS=	$(PIPS_ARFLAGS)
CC=		$(PIPS_CC)
CFLAGS=		$(PIPS_CFLAGS)
CPPFLAGS=	$(PIPS_CPPFLAGS)
LD=		$(PIPS_LD)
LDFLAGS=	$(PIPS_LDFLAGS)
LEX=		$(PIPS_LEX)
LFLAGS=		$(PIPS_LFLAGS)
LINT=		$(PIPS_LINT)
LINTFLAGS=	$(PIPS_LINTFLAGS)
YACC=		$(PIPS_YACC)
YFLAGS=		$(PIPS_YFLAGS) -v -d
#
# The following macros define your library.
#
# Name of the library without the .a suffix.
TARGET= 	ricedg
#
# Name of the main program to test or use the library
MAIN=		main
#
#
# Source, header and object files used to build the library.
# Do not include the main program source file.
LIB_CFILES=	util.c contexts.c testdep_util.c ricedg.c prettyprint.c quick_privatize.c
LIB_HEADERS=	ricedg-local.h
LIB_OBJECTS=	util.o contexts.o testdep_util.o ricedg.o prettyprint.o quick_privatize.o
