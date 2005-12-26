# $Id$

# expected macros:
# ROOT
ifndef ROOT
$(error "expected ROOT macro not found!")
endif

# ARCH (or default provided)

# INC_TARGET: header file for directory (on INC_CFILES or LIB_CFILES)

# LIB_CFILES: C files that are included in the library
# LIB_TARGET: generated library file
# BIN_TARGET: generated binary files

# files to be installed in subdirectories:
# INSTALL_INC: headers to be installed (INC_TARGET not included)
# INSTALL_LIB: libraries to be added (LIB_TARGET not included)
# INSTALL_BIN: added binaries (BIN_TARGET not included)
# INSTALL_EXE: added executable (shell scripts or the like)
# INSTALL_ETC: configuration files
# INSTALL_SHR: shared (cross platform) files
# INSTALL_UTL: script utilities
# INSTALL_RTM: runtime-related stuff
# INSTALL_MAN DOC HTM: various documentations

all: recompile

recompile: phase0 phase1 phase2 phase3 phase4 phase5 phase6

########################################################################## ROOT

ifndef INSTALL_DIR
INSTALL_DIR	= $(ROOT)
endif # INSTALL_DIR

ifndef PIPS_ROOT
PIPS_ROOT	= $(ROOT)/../pips
endif # PIPS_ROOT

ifndef NEWGEN_ROOT
NEWGEN_ROOT	= $(ROOT)/../newgen
endif # NEWGEN_ROOT

ifndef LINEAR_ROOT
LINEAR_ROOT	= $(ROOT)/../linear
endif # LINEAR_ROOT

ifndef EXTERN_ROOT
EXTERN_ROOT	= $(ROOT)/../extern
endif # EXTERN_ROOT

# where are make files
MAKE.d	= $(ROOT)/makes

########################################################################## ARCH

ifndef ARCH
ifdef PIPS_ARCH
ARCH	= $(PIPS_ARCH)
else
ifdef NEWGEN_ARCH
ARCH	= $(NEWGEN_ARCH)
else
ifdef LINEAR_ARCH
ARCH	= $(LINEAR_ARCH)
else
ARCH	:= $(shell $(MAKE.d)/arch.sh)
endif # LINEAR_ARCH
endif # NEWGEN_ARCH
endif # PIPS_ARCH
endif # ARCH

ifndef ARCH
$(error "ARCH macro is not defined")
endif

# where to install stuff
BIN.d	= $(INSTALL_DIR)/bin/$(ARCH)
EXE.d	= $(INSTALL_DIR)/bin
LIB.d	= $(INSTALL_DIR)/lib
INC.d	= $(INSTALL_DIR)/include
ETC.d	= $(INSTALL_DIR)/etc
DOC.d	= $(INSTALL_DIR)/doc
MAN.d	= $(INSTALL_DIR)/man
HTM.d	= $(INSTALL_DIR)/html
UTL.d	= $(INSTALL_DIR)/utils
SHR.d	= $(INSTALL_DIR)/share
RTM.d	= $(INSTALL_DIR)/runtime

clean: NO_INCLUDES=1

# ??? do not include for some targets such as "clean"
ifndef NO_INCLUDES
include $(MAKE.d)/$(ARCH).mk
include $(MAKE.d)/svn.mk
ifdef PROJECT
include $(MAKE.d)/$(PROJECT).mk
endif # PROJECT
endif # NO_INCLUDES

# ??? fix path...
PATH	:= $(PATH):$(NEWGEN_ROOT)/bin:$(NEWGEN_ROOT)/bin/$(ARCH)

###################################################################### DO STUFF

UTC_DATE := $(shell date -u | tr ' ' '_')
CPPFLAGS += -DSOFT_ARCH='"$(ARCH)"' -I$(ROOT)/include

# {C,CPP,LD,L,Y}OPT macros allow to *add* things from the command line
# as gmake CPPOPT="-DFOO=bar" ... that will be added to the defaults
# a typical interesting example is to put -pg in {C,LD}OPT
#
PREPROC	= $(CC) -E $(CANSI) $(CPPOPT) $(CPPFLAGS)
COMPILE	= $(CC) $(CANSI) $(CFLAGS) $(COPT) $(CPPOPT) $(CPPFLAGS) -c
F77CMP	= $(FC) $(FFLAGS) $(FOPT) -c
LINK	= $(LD) $(LDFLAGS) $(LDOPT) -o
SCAN	= $(LEX) $(LFLAGS) $(LOPT) -t
TYPECK	= $(LINT) $(LINTFLAGS) $(CPPFLAGS) $(LINT_LIBS)
PARSE	= $(YACC) $(YFLAGS) $(YOPT)
ARCHIVE = $(AR) $(ARFLAGS)
PROTOIZE= $(PROTO) $(PRFLAGS) -E "$(PREPROC) -DCPROTO_IS_PROTOTYPING"
M4FLT	= $(M4) $(M4OPT) $(M4FLAGS)
MAKEDEP	= $(CC) $(CMKDEP) $(CANSI) $(CFLAGS) $(COPT) $(CPPOPT) $(CPPFLAGS)
NODIR	= --no-print-directory
COPY	= cp
MOVE	= mv
JAVAC	= javac
JNI	= javah -jni
MKDIR	= mkdir -p -m 755
RMDIR	= rmdir
INSTALL	= install
CMP	= cmp -s

# misc filters
FLTVERB	= sed  '/^\\begin{verbatim}/,/^\\end{verbatim}/!d;\
		/^\\begin{verbatim}/d;/^\\end{verbatim}/s,.*,,'
UPPER	= tr '[a-z]' '[A-Z]'

# for easy debugging... e.g. gmake ECHO='something' echo
echo:; @echo $(ECHO)

%: %.c
%: %.o
$(ARCH)/%.o: %.c; $(COMPILE) $< -o $@
$(ARCH)/%.o: %.f; $(F77CMP) $< -o $@

%.class: %.java; $(JAVAC) $<
%.h: %.class; $(JNI) $*

%.f: %.m4f;	$(M4FLT) $(M4FOPT) $< > $@
%.c: %.m4c;	$(M4FLT) $(M4COPT) $< > $@
%.h: %.m4h;	$(M4FLT) $(M4HOPT) $< > $@

# latex
%.dvi: %.tex
	-grep '\\makeindex' $*.tex && touch $*.ind
	$(LATEX) $<
	-grep '\\bibdata{' \*.aux && { $(BIBTEX) $* ; $(LATEX) $< ;}
	test ! -f $*.idx || { $(MAKEIDX) $*.idx ; $(LATEX) $< ;}
	$(LATEX) $<
	touch $@

################################################################## DEPENDENCIES

ifdef LIB_CFILES
need_depend	= 1
endif # LIB_CFILES

ifdef OTHER_CFILES
need_depend	= 1
endif # OTHER_CFILES

ifdef need_depend

DEPEND	= .depend.$(ARCH)

phase0: $(DEPEND)

ifndef NO_INCLUDES
-include $(DEPEND)
endif

# generation by make recursion
$(DEPEND): $(LIB_CFILES) $(OTHER_CFILES) $(DERIVED_CFILES)
	touch $@
	test -s $(DEPEND) || $(MAKE) depend

# actual generation is done on demand only
depend: $(DERIVED_HEADERS) $(INC_TARGET)
	$(MAKEDEP) $(LIB_CFILES) $(OTHER_CFILES) $(DERIVED_CFILES) | \
	sed    's,^\(.*\.o:\),$(ARCH)/\1,;\
		s,$(PIPS_ROOT),$$(PIPS_ROOT),g;\
		s,$(LINEAR_ROOT),$$(LINEAR_ROOT),g;\
		s,$(NEWGEN_ROOT),$$(NEWGEN_ROOT),g;\
		s,$(ROOT),$$(ROOT),g' > $(DEPEND)

clean: depend-clean

depend-clean:; $(RM) .depend.*

endif # need_depend

########################################################### CONFIGURATION FILES

ifdef ETC_TARGET

INSTALL_ETC	+= $(ETC_TARGET)

endif # ETC_TARGET

ifdef INSTALL_ETC

$(ETC.d):
	$(MKDIR) $@

.build_etc: $(INSTALL_ETC) $(ETC.d)
	for f in $(INSTALL_ETC) ; do \
	  $(CMP) $$f $(ETC.d)/$$f || \
	    $(INSTALL) -m 644 $$f $(ETC.d) ; \
	done
	touch $@

clean: etc-clean

etc-clean:
	$(RM) .build_etc

phase1: .build_etc

endif # INSTALL_ETC

####################################################################### HEADERS

ifdef INC_TARGET
# generate directory header file with cproto

ifndef INC_CFILES
INC_CFILES	= $(LIB_CFILES)
endif # INC_CFILES

name	= $(subst -,_, $(notdir $(CURDIR)))

build-header-file:
	$(COPY) $(TARGET)-local.h $(INC_TARGET); \
	{ \
	  echo "/* header file built by $(PROTO) */"; \
	  echo "#ifndef $(name)_header_included";\
	  echo "#define $(name)_header_included";\
	  cat $(TARGET)-local.h;\
	  $(PROTOIZE) $(INC_CFILES) | \
	  sed 's/struct _iobuf/FILE/g;\
	       s/__const/const/g;\
	       /_BUFFER_STATE/d;\
	       /__inline__/d;' ; \
	  echo "#endif /* $(name)_header_included */";\
	} > $(INC_TARGET).tmp
	$(MOVE) $(INC_TARGET).tmp $(INC_TARGET)

header:	.header $(INC_TARGET)

# .header carrie all dependencies for INC_TARGET:
.header: $(TARGET)-local.h $(DERIVED_HEADERS) $(LIB_CFILES) 
	$(MAKE) $(GMKNODIR) build-header-file
	touch .header

$(INC_TARGET): $(TARGET)-local.h
	$(RM) .header; $(MAKE) $(GMKNODIR) .header

phase2:	header

clean: header-clean

header-clean:
	$(RM) $(INC_TARGET) .header

INSTALL_INC	+=   $(INC_TARGET)

endif # INC_TARGET

ifdef INSTALL_INC

phase2: .build_inc

$(INC.d):; $(MKDIR) $(INC.d)

.build_inc: $(INSTALL_INC) $(INC.d)
	for f in $(INSTALL_INC) ; do \
	  $(CMP) $$f $(INC.d)/$$f || \
	    $(INSTALL) -m 644 $$f $(INC.d) ; \
	done
	touch $@

clean: inc-clean

inc-clean:
	$(RM) .build_inc

endif # INSTALL_INC

####################################################################### LIBRARY

# ARCH subdirectory
$(ARCH):
	test -d $(ARCH) || $(MKDIR) $(ARCH)

clean: arch-clean

arch-clean:
	-$(RM) $(ARCH)/*.o $(ARCH)/lib*.a
	-$(RMDIR) $(ARCH)

ifdef LIB_CFILES
ifndef LIB_OBJECTS
LIB_OBJECTS = $(addprefix $(ARCH)/,$(LIB_CFILES:.c=.o))
endif # LIB_OBJECTS
endif # LIB_CFILES

ifdef LIB_TARGET

$(ARCH)/$(LIB_TARGET): $(LIB_OBJECTS)
	$(ARCHIVE) $(ARCH)/$(LIB_TARGET) $(LIB_OBJECTS)
	ranlib $@

INSTALL_LIB	+=   $(addprefix $(ARCH)/,$(LIB_TARGET))

endif # LIB_TARGET


ifdef INSTALL_LIB

phase2: $(ARCH)

phase4:	.build_lib.$(ARCH)

$(INSTALL_LIB): $(ARCH)

$(LIB.d):
	$(MKDIR) $@

$(LIB.d)/$(ARCH): $(LIB.d)
	$(MKDIR) $@

.build_lib.$(ARCH): $(INSTALL_LIB) $(LIB.d)/$(ARCH)
	for l in $(INSTALL_LIB) ; do \
	  $(CMP) $$l $(LIB.d)/$$l || \
	    $(INSTALL) -m 644 $$l $(LIB.d)/$(ARCH) ; \
	done
	touch $@

clean: lib-clean

lib-clean:; $(RM) $(ARCH)/$(LIB_TARGET) .build_lib.*

endif # INSTALL_LIB

######################################################################## PHASES

# multiphase compilation?

compile:
	$(MAKE) phase0
	$(MAKE) phase1
	$(MAKE) phase2
	$(MAKE) phase3
	$(MAKE) phase4
	$(MAKE) phase5
	$(MAKE) phase6

install: recompile

# empty dependencies to please compile targets
phase0:
phase1:
phase2:
phase3:
phase4:
phase5:
phase6:

ifdef INSTALL_EXE

phase2: .build_exe

$(EXE.d):
	$(MKDIR) $@

.build_exe: $(INSTALL_EXE) $(EXE.d)
	$(INSTALL) -m 755 $(INSTALL_EXE) $(EXE.d)
	touch $@

clean: exe-clean

exe-clean:
	$(RM) .build_exe

endif # INSTALL_EXE


# binaries
ifdef BIN_TARGET
INSTALL_BIN	+=   $(addprefix $(ARCH)/,$(BIN_TARGET))
endif # BIN_TARGET

ifdef INSTALL_BIN

phase5: .build_bin.$(ARCH)

$(INSTALL_BIN): $(ARCH)

$(BIN.d):
	$(MKDIR) $@

.build_bin.$(ARCH): $(INSTALL_BIN) $(BIN.d)
	$(INSTALL) -m 755 $(INSTALL_BIN) $(BIN.d)
	touch $@

clean: bin-clean

bin-clean:
	$(RM) .build_bin.*

endif # INSTALL_BIN

# documentation
ifdef INSTALL_DOC

phase6: .build_doc

$(DOC.d):; $(MKDIR) $(DOC.d)

.build_doc: $(INSTALL_DOC) $(DOC.d)
	$(INSTALL) -m 644 $(INSTALL_DOC) $(DOC.d)
	touch $@

clean: doc-clean

doc-clean:
	$(RM) .build_doc

endif # INSTALL_DOC

# manuel
ifdef INSTALL_MAN

phase6: .build_man

$(MAN.d):; $(MKDIR) $(MAN.d)

.build_man: $(INSTALL_MAN) $(MAN.d)
	$(INSTALL) -m 644 $(INSTALL_MAN) $(MAN.d)
	touch $@

clean: man-clean

man-clean:
	$(RM) .build_man

endif # INSTALL_MAN

# documentations html
ifdef INSTALL_HTM

phase6: .build_htm

$(HTM.d):; $(MKDIR) $(HTM.d)

.build_htm: $(INSTALL_HTM) $(HTM.d)
	$(INSTALL) -m 644 $(INSTALL_HTM) $(HTM.d)
	touch $@

clean: htm-clean

htm-clean:
	$(RM) .build_htm

endif # INSTALL_HTM

# shared
ifdef INSTALL_SHR

phase2: .build_shr

$(SHR.d):; $(MKDIR) $(SHR.d)

.build_shr: $(INSTALL_SHR) $(SHR.d)
	$(INSTALL) -m 644 $(INSTALL_SHR) $(SHR.d)
	touch $@

clean: shr-clean

shr-clean:
	$(RM) .build_shr

endif # INSTALL_SHR

# utils
ifdef INSTALL_UTL

phase2: .build_utl

$(UTL.d):; $(MKDIR) $(UTL.d)

.build_utl: $(INSTALL_UTL) $(UTL.d)
	$(INSTALL) -m 755 $(INSTALL_UTL) $(UTL.d)
	touch $@

clean: utl-clean

utl-clean:
	$(RM) .build_utl

endif # INSTALL_UTL

# other targets

clean: main-clean

main-clean:
	$(RM) *~ *.tmp
