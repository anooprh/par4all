INSTALL_DOC = p4a_script.pdf
INSTALL_HTM = p4a_script.htdoc

# Where to install the documentation:
INSTALL_MACHINE_DIR=$(INSTALL_MACHINE_DOC_DIR)/simple_tools/p4a

TEXINPUTS:=$(TEXINPUTS):$(shell pwd)/../../figures//::$(shell pwd)/../../../examples/Python::

# Hide the PDF into the HTML documentation directory
DOC.subd = $(INSTALL_HTM)

use_pdflatex = 1
# Force HTML generation:
_HAS_HTLATEX_ = 1

# common stuff
ROOT    = $(P4A_ROOT)/packages/PIPS/pips
PROJECT	= pips

# Some global config information:
include $(P4A_ROOT)/doc/libs/par4all.mk

default: full-compile phase7

include $(ROOT)/makes/main.mk


publish:
	ssh $(INSTALL_MACHINE) mkdir -p $(INSTALL_MACHINE_DIR)
	rsync -av $(INSTALL_DOC) $(INSTALL_HTM) $(INSTALL_MACHINE):$(INSTALL_MACHINE_DIR)

# local stuff
clean: local-clean
	$(RM) -rf ../../figures/*.eps ../../figures/*.pdf

local-clean:
	$(RM) -rf $(INSTALL_DOC) $(INSTALL_HTM) \
		*.aux *.bbl *.blg *.rel *.toc *.idx *.ilg *.ind *.log *.dvi *.ps *.out *.help *-help.tex parsetab.py

# Often, PDF output of tools (inkscape...) are bad because it is a whole
# page without cropping.  So rely on .eps output and apply translation and
# cropping:
%.pdf: %.eps
	ps2pdf $< $@
	pdfcrop $@
	mv $*-crop.pdf $@

# Add the dependence on some automatically generated help:
# The .pdf figure is for pdfLaTeX but the .eps is for LaTeX4HT.
p4a_script.tex: p4a-help.tex ../../figures/p4a_work_flow.pdf  ../../figures/p4a_work_flow.eps

# generate .eps from svg
%.eps: %.svg
	inkscape $< --export-eps=$@ --export-text-to-path

# How to generate the script documentation:
%-help.tex: %.help Makefile
	../../../src/dev/optparse_help_to_tex --article --article-section-level 3 $< > $@

%.help: ../../../src/simple_tools/%.py Makefile
	# Generate raw help message for the given command:
	$< -h > $@

### Local Variables:
### mode: flyspell
### ispell-local-dictionary: "american"
### End:
