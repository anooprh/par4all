#
# $RCSfile: config.makefile,v $ for dev
#

SCRIPTS = 	pips-makemake \
		install_pips_sources \
		make-tags \
		pips_install_file \
		analyze_libraries \
		clean-pips \
		grep_libraries \
		search-pips \
		checked-out \
		order_libraries \
		pips-experiment \
		pips_at_night \
		forward_gnu_makefile \
		remove_from_sccs_file

COPY=	cp -f

forward: 
	$(COPY) forward_gnu_makefile ${PIPS_DEVEDIR}/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_DEVEDIR}/Lib/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_DEVEDIR}/Passes/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_DEVEDIR}/Scripts/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_DEVEDIR}/Runtime/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_PRODDIR}/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_SRCDIR}/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_SRCDIR}/Lib/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_SRCDIR}/Passes/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_SRCDIR}/Scripts/Makefile
	$(COPY) forward_gnu_makefile ${PIPS_SRCDIR}/Runtime/Makefile

# that is all
#
