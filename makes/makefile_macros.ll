#
# $Id$
#
# macros related to use of 64 bits arithmetic.
#

CPPFLAGS += \
	-DLINEAR_VALUE_IS_LONGLONG \
	-DLINEAR_VALUE_PROTECT_MULTIPLY \
	-DLINEAR_VALUE_ASSUME_SOFTWARE_IDIV 
