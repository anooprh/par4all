#!/usr/bin/env python

import sys, os, shutil

from pyps import workspace

with workspace(["effects.c"], name="effects") as ws:
	ws.props.ABORT_ON_USER_ERROR = True

	print "cumulated effects on only one function"
	fct = ws.fun.add_comp
	fct.print_code_cumulated_effects()
	fct.display ()
