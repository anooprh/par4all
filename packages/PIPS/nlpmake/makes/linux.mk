#
# $Id$
#
# updates to GNU for LINUX.

# CFLAGS	+=	-march=pentium -mwide-multiply -malign-double

CPPFLAGS += \
	-D_POSIX_SOURCE \
	-D_POSIX_C_SOURCE=2 \
	-D_BSD_SOURCE \
	-D_SVID_SOURCE

AR	=	ar
RANLIB	=	ranlib
DIFF	=	diff
M4	=	m4

# end of it
#
