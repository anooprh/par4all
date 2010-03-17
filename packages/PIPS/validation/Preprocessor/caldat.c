/* Declarations for math functions.
   Copyright (C) 1991-1993,1995-1999,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*
 *	ISO C99 Standard: 7.12 Mathematics	<math.h>
 */





/* Copyright (C) 1991-1993,1995-2002, 2003   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */




/* These are defined by the user (or the compiler)
   to specify the desired environment:

   __STRICT_ANSI__	ISO Standard C.
   _ISOC99_SOURCE	Extensions to ISO C89 from ISO C99.
   _POSIX_SOURCE	IEEE Std 1003.1.
   _POSIX_C_SOURCE	If ==1, like _POSIX_SOURCE; if >=2 add IEEE Std 1003.2;
			if >=199309L, add IEEE Std 1003.1b-1993;
			if >=199506L, add IEEE Std 1003.1c-1995
   _XOPEN_SOURCE	Includes POSIX and XPG things.  Set to 500 if
			Single Unix conformance is wanted, to 600 for the
			upcoming sixth revision.
   _XOPEN_SOURCE_EXTENDED XPG things and X/Open Unix extensions.
   _LARGEFILE_SOURCE	Some more functions for correct standard I/O.
   _LARGEFILE64_SOURCE	Additional functionality from LFS for large files.
   _FILE_OFFSET_BITS=N	Select default filesystem interface.
   _BSD_SOURCE		ISO C, POSIX, and 4.3BSD things.
   _SVID_SOURCE		ISO C, POSIX, and SVID things.
   _GNU_SOURCE		All of the above, plus GNU extensions.
   _REENTRANT		Select additionally reentrant object.
   _THREAD_SAFE		Same as _REENTRANT, often used by other systems.

   The `-ansi' switch to the GNU C compiler defines __STRICT_ANSI__.
   If none of these are defined, the default is to have _SVID_SOURCE,
   _BSD_SOURCE, and _POSIX_SOURCE set to one and _POSIX_C_SOURCE set to
   199506L.  If more than one of these are defined, they accumulate.
   For example __STRICT_ANSI__, _POSIX_SOURCE and _POSIX_C_SOURCE
   together give you ISO C, 1003.1, and 1003.2, but nothing else.

   These are defined by this file and are used by the
   header files to decide what to declare or define:

   __USE_ISOC99		Define ISO C99 things.
   __USE_POSIX		Define IEEE Std 1003.1 things.
   __USE_POSIX2		Define IEEE Std 1003.2 things.
   __USE_POSIX199309	Define IEEE Std 1003.1, and .1b things.
   __USE_POSIX199506	Define IEEE Std 1003.1, .1b, .1c and .1i things.
   __USE_XOPEN		Define XPG things.
   __USE_XOPEN_EXTENDED	Define X/Open Unix things.
   __USE_UNIX98		Define Single Unix V2 things.
   __USE_XOPEN2K        Define XPG6 things.
   __USE_LARGEFILE	Define correct standard I/O things.
   __USE_LARGEFILE64	Define LFS things with separate names.
   __USE_FILE_OFFSET64	Define 64bit interface as default.
   __USE_BSD		Define 4.3BSD things.
   __USE_SVID		Define SVID things.
   __USE_MISC		Define things common to BSD and System V Unix.
   __USE_GNU		Define GNU extensions.
   __USE_REENTRANT	Define reentrant/thread-safe *_r functions.
   __FAVOR_BSD		Favor 4.3BSD things in cases of conflict.

   The macros `__GNU_LIBRARY__', `__GLIBC__', and `__GLIBC_MINOR__' are
   defined by this file unconditionally.  `__GNU_LIBRARY__' is provided
   only for compatibility.  All new code should use the other symbols
   to test for features.

   All macros listed above as possibly being defined by this file are
   explicitly undefined if they are not explicitly defined.
   Feature-test macros that are not defined by the user or compiler
   but are implied by the other feature-test macros defined (or by the
   lack of any definitions) are defined by the file.  */


/* Undefine everything, so we get a clean slate.  */
/* Suppress kernel-name space pollution unless user expressedly asks
   for it.  */




/* Always use ISO C things.  */



/* If _BSD_SOURCE was defined by the user, favor BSD over POSIX.  */







/* If _GNU_SOURCE was defined by the user, turn on all the other features.  */
/* If nothing (other than _GNU_SOURCE) is defined,
   define _BSD_SOURCE and _SVID_SOURCE.  */
/* This is to enable the ISO C99 extension.  Also recognize the old macro
   which was used prior to the standard acceptance.  This macro will
   eventually go away and the features enabled by default once the ISO C99
   standard is widely adopted.  */





/* If none of the ANSI/POSIX macros are defined, use POSIX.1 and POSIX.2
   (and IEEE Std 1003.1b-1993 unless _XOPEN_SOURCE is defined).  */
/* We do support the IEC 559 math functionality, real and complex.  */



/* wchar_t uses ISO 10646-1 (2nd ed., published 2000-09-15) / Unicode 3.1.  */


/* This macro indicates that the installed library is the GNU C Library.
   For historic reasons the value now is 6 and this will stay from now
   on.  The use of this variable is deprecated.  Use __GLIBC__ and
   __GLIBC_MINOR__ now (see below) when you want to test for a specific
   GNU C library version and use the values in <gnu/lib-names.h> to get
   the sonames of the shared libraries.  */



/* Major and minor version number of the GNU C library package.  Use
   these macros to test for features in specific releases.  */



/* Convenience macros to test the versions of glibc and gcc.
   Use them like this:
   #if __GNUC_PREREQ (2,8)
   ... code requiring gcc 2.8 or later ...
   #endif
   Note - they won't work for gcc1 or glibc1, since the _MINOR macros
   were not defined then.  */
/* Decide whether a compiler supports the long long datatypes.  */







/* This is here only because every header file already includes this one.  */



/* Copyright (C) 1992-2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */




/* We are almost always included from features.h. */




/* The GNU libc does not support any K&R compilers or the traditional mode
   of ISO C compilers anymore.  Check for some of the combinations not
   anymore supported.  */




/* Some user header file might have defined this before.  */
/* For these things, GCC behaves the ANSI way normally,
   and the non-ANSI way under -traditional.  */




/* This is not a typedef so `const __ptr_t' does the right thing.  */




/* C++ needs to know that types and declarations are C, not C++.  */
/* The standard library needs the functions from the ISO C90 standard
   in the std namespace.  At the same time we want to be safe for
   future changes and we include the ISO C99 code in the non-standard
   namespace __c99.  The C++ wrapper header take case of adding the
   definitions to the global namespace.  */
/* For compatibility we do not add the declarations into any
   namespace.  They will end up in the global namespace which is what
   old code expects.  */
/* Support for bounded pointers.  */







/* Support for flexible arrays.  */
/* Some other non-C99 compiler.  Approximate with [1].  */






/* __asm__ ("xyz") is used throughout the headers to rename functions
   at the assembly language level.  This is wrapped by the __REDIRECT
   macro, in order to support compilers that can do this some other
   way.  When compilers don't support asm-names at all, we have to do
   preprocessor tricks instead (which don't have exactly the right
   semantics, but it's the best we can do).

   Example:
   int __REDIRECT(setpgrp, (__pid_t pid, __pid_t pgrp), setpgid); */
/* GCC has various useful declarations that can be made with the
   `__attribute__' syntax.  All of the ways we use this do fine if
   they are omitted for compilers that don't understand it. */




/* At some point during the gcc 2.96 development the `malloc' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */






/* At some point during the gcc 2.96 development the `pure' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */






/* At some point during the gcc 3.1 development the `used' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
/* gcc allows marking deprecated functions.  */






/* At some point during the gcc 2.8 development the `format_arg' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.
   If several `format_arg' attributes are given for the same function, in
   gcc-3.0 and older, all but the last one are ignored.  In newer gccs,
   all designated arguments are considered.  */






/* At some point during the gcc 2.97 development the `strfmon' format
   attribute for functions was introduced.  We don't want to use it
   unconditionally (although this would be possible) since it
   generates warnings.  */







/* It is possible to compile containing GCC extensions even if GCC is
   run in pedantic mode if the uses are carefully marked using the
   `__extension__' keyword.  But this is not generally available before
   version 2.8.  */




/* __restrict is known in EGCS 1.2 and above. */




/* ISO C99 also allows to declare arrays as non-overlapping.  The syntax is
     array_name[restrict]
   GCC 3.1 supports this.  */
/* Some other non-C99 compiler.  */
/* If we don't have __REDIRECT, prototypes will be missing if
   __USE_FILE_OFFSET64 but not __USE_LARGEFILE[64]. */







/* Decide whether we can define 'extern inline' functions in headers.  */






/* This is here only because every header file already includes this one.
   Get the definitions of all the appropriate `__stub_FUNCTION' symbols.
   <gnu/stubs.h> contains `#define __stub_FUNCTION' when FUNCTION is a stub
   that will always return failure (and set errno to ENOSYS).  */

/* This file is automatically generated.
   It defines a symbol `__stub_FUNCTION' for each function
   in the C library which is a stub, meaning it will fail
   every time called, usually setting errno to ENOSYS.  */


/* Get machine-dependent HUGE_VAL value (returned on overflow).
   On all IEEE754 machines, this is +Infinity.  */

/* `HUGE_VAL' constants for ix86 (where it is infinity).
   Used by <stdlib.h> and <math.h> functions for overflow.
   Copyright (C) 1992, 1995, 1996, 1997, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */






/* Copyright (C) 1991-1993,1995-2002, 2003   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
/* IEEE positive infinity (-HUGE_VAL is negative infinity).  */
static union { unsigned char __c[8]; double __d; } __huge_val = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };





/* ISO C99 extensions: (float) HUGE_VALF and (long double) HUGE_VALL.  */
/* Get machine-dependent NAN value (returned for some domain errors).  */



/* Get general and ISO C99 specific information.  */

/* Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
/* The file <bits/mathcalls.h> contains the prototypes for all the
   actual math functions.  These macros are used for those prototypes,
   so we can easily declare each function as both `name' and `__name',
   and can declare the float versions `namef' and `__namef'.  */
/* Prototype declarations for math functions; helper file for <math.h>.
   Copyright (C) 1996-2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* NOTE: Because of the special way this file is used by <math.h>, this
   file must NOT be protected from multiple inclusion as header files
   usually are.

   This file provides prototype declarations for the math functions.
   Most functions are declared using the macro:

   __MATHCALL (NAME,[_r], (ARGS...));

   This means there is a function `NAME' returning `double' and a function
   `NAMEf' returning `float'.  Each place `_Mdouble_' appears in the
   prototype, that is actually `double' in the prototype for `NAME' and
   `float' in the prototype for `NAMEf'.  Reentrant variant functions are
   called `NAME_r' and `NAMEf_r'.

   Functions returning other types like `int' are declared using the macro:

   __MATHDECL (TYPE, NAME,[_r], (ARGS...));

   This is just like __MATHCALL but for a function returning `TYPE'
   instead of `_Mdouble_'.  In all of these cases, there is still
   both a `NAME' and a `NAMEf' that takes `float' arguments.

   Note that there must be no whitespace before the argument passed for
   NAME, to make token pasting work with -traditional.  */






/* Trigonometric functions.  */


/* Arc cosine of X.  */
extern double acos (double __x) ; extern double __acos (double __x) ;
/* Arc sine of X.  */
extern double asin (double __x) ; extern double __asin (double __x) ;
/* Arc tangent of X.  */
extern double atan (double __x) ; extern double __atan (double __x) ;
/* Arc tangent of Y/X.  */
extern double atan2 (double __y, double __x) ; extern double __atan2 (double __y, double __x) ;

/* Cosine of X.  */
extern double cos (double __x) ; extern double __cos (double __x) ;
/* Sine of X.  */
extern double sin (double __x) ; extern double __sin (double __x) ;
/* Tangent of X.  */
extern double tan (double __x) ; extern double __tan (double __x) ;

/* Hyperbolic functions.  */

/* Hyperbolic cosine of X.  */
extern double cosh (double __x) ; extern double __cosh (double __x) ;
/* Hyperbolic sine of X.  */
extern double sinh (double __x) ; extern double __sinh (double __x) ;
/* Hyperbolic tangent of X.  */
extern double tanh (double __x) ; extern double __tanh (double __x) ;


/* Hyperbolic arc cosine of X.  */
extern double acosh (double __x) ; extern double __acosh (double __x) ;
/* Hyperbolic arc sine of X.  */
extern double asinh (double __x) ; extern double __asinh (double __x) ;
/* Hyperbolic arc tangent of X.  */
extern double atanh (double __x) ; extern double __atanh (double __x) ;



/* Exponential and logarithmic functions.  */


/* Exponential function of X.  */
extern double exp (double __x) ; extern double __exp (double __x) ;

/* Break VALUE into a normalized fraction and an integral power of 2.  */
extern double frexp (double __x, int *__exponent) ; extern double __frexp (double __x, int *__exponent) ;

/* X times (two to the EXP power).  */
extern double ldexp (double __x, int __exponent) ; extern double __ldexp (double __x, int __exponent) ;

/* Natural logarithm of X.  */
extern double log (double __x) ; extern double __log (double __x) ;

/* Base-ten logarithm of X.  */
extern double log10 (double __x) ; extern double __log10 (double __x) ;

/* Break VALUE into integral and fractional parts.  */
extern double modf (double __x, double *__iptr) ; extern double __modf (double __x, double *__iptr) ;


/* Return exp(X) - 1.  */
extern double expm1 (double __x) ; extern double __expm1 (double __x) ;

/* Return log(1 + X).  */
extern double log1p (double __x) ; extern double __log1p (double __x) ;

/* Return the base 2 signed integral exponent of X.  */
extern double logb (double __x) ; extern double __logb (double __x) ;

/* Power functions.  */


/* Return X to the Y power.  */
extern double pow (double __x, double __y) ; extern double __pow (double __x, double __y) ;

/* Return the square root of X.  */
extern double sqrt (double __x) ; extern double __sqrt (double __x) ;




/* Return `sqrt(X*X + Y*Y)'.  */
extern double hypot (double __x, double __y) ; extern double __hypot (double __x, double __y) ;





/* Return the cube root of X.  */
extern double cbrt (double __x) ; extern double __cbrt (double __x) ;




/* Nearest integer, absolute value, and remainder functions.  */


/* Smallest integral value not less than X.  */
extern double ceil (double __x) ; extern double __ceil (double __x) ;

/* Absolute value of X.  */
extern double fabs (double __x) ; extern double __fabs (double __x) ;

/* Largest integer not greater than X.  */
extern double floor (double __x) ; extern double __floor (double __x) ;

/* Floating-point modulo remainder of X/Y.  */
extern double fmod (double __x, double __y) ; extern double __fmod (double __x, double __y) ;


/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int __isinf (double __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int __finite (double __value) ;



/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int isinf (double __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int finite (double __value) ;

/* Return the remainder of X/Y.  */
extern double drem (double __x, double __y) ; extern double __drem (double __x, double __y) ;


/* Return the fractional part of X after dividing out `ilogb (X)'.  */
extern double significand (double __x) ; extern double __significand (double __x) ;




/* Return X with its signed changed to Y's.  */
extern double copysign (double __x, double __y) ; extern double __copysign (double __x, double __y) ;

/* Return nonzero if VALUE is not a number.  */
extern int __isnan (double __value) ;


/* Return nonzero if VALUE is not a number.  */
extern int isnan (double __value) ;

/* Bessel functions.  */
extern double j0 (double) ; extern double __j0 (double) ;
extern double j1 (double) ; extern double __j1 (double) ;
extern double jn (int, double) ; extern double __jn (int, double) ;
extern double y0 (double) ; extern double __y0 (double) ;
extern double y1 (double) ; extern double __y1 (double) ;
extern double yn (int, double) ; extern double __yn (int, double) ;





/* Error and gamma functions.  */
extern double erf (double) ; extern double __erf (double) ;
extern double erfc (double) ; extern double __erfc (double) ;
extern double lgamma (double) ; extern double __lgamma (double) ;

/* Obsolete alias for `lgamma'.  */
extern double gamma (double) ; extern double __gamma (double) ;



/* Reentrant version of lgamma.  This function uses the global variable
   `signgam'.  The reentrant version instead takes a pointer and stores
   the value through it.  */
extern double lgamma_r (double, int *__signgamp) ; extern double __lgamma_r (double, int *__signgamp) ;





/* Return the integer nearest X in the direction of the
   prevailing rounding mode.  */
extern double rint (double __x) ; extern double __rint (double __x) ;

/* Return X + epsilon if X < Y, X - epsilon if X > Y.  */
extern double nextafter (double __x, double __y) ; extern double __nextafter (double __x, double __y) ;




/* Return the remainder of integer divison X / Y with infinite precision.  */
extern double remainder (double __x, double __y) ; extern double __remainder (double __x, double __y) ;


/* Return X times (2 to the Nth power).  */
extern double scalbn (double __x, int __n) ; extern double __scalbn (double __x, int __n) ;


/* Return the binary exponent of X, which must be nonzero.  */
extern int ilogb (double __x) ; extern int __ilogb (double __x) ;
/* Include the file of declarations again, this time using `float'
   instead of `double' and appending f to each function name.  */
/* Prototype declarations for math functions; helper file for <math.h>.
   Copyright (C) 1996-2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* NOTE: Because of the special way this file is used by <math.h>, this
   file must NOT be protected from multiple inclusion as header files
   usually are.

   This file provides prototype declarations for the math functions.
   Most functions are declared using the macro:

   __MATHCALL (NAME,[_r], (ARGS...));

   This means there is a function `NAME' returning `double' and a function
   `NAMEf' returning `float'.  Each place `_Mdouble_' appears in the
   prototype, that is actually `double' in the prototype for `NAME' and
   `float' in the prototype for `NAMEf'.  Reentrant variant functions are
   called `NAME_r' and `NAMEf_r'.

   Functions returning other types like `int' are declared using the macro:

   __MATHDECL (TYPE, NAME,[_r], (ARGS...));

   This is just like __MATHCALL but for a function returning `TYPE'
   instead of `_Mdouble_'.  In all of these cases, there is still
   both a `NAME' and a `NAMEf' that takes `float' arguments.

   Note that there must be no whitespace before the argument passed for
   NAME, to make token pasting work with -traditional.  */






/* Trigonometric functions.  */


/* Arc cosine of X.  */
extern float acosf (float __x) ; extern float __acosf (float __x) ;
/* Arc sine of X.  */
extern float asinf (float __x) ; extern float __asinf (float __x) ;
/* Arc tangent of X.  */
extern float atanf (float __x) ; extern float __atanf (float __x) ;
/* Arc tangent of Y/X.  */
extern float atan2f (float __y, float __x) ; extern float __atan2f (float __y, float __x) ;

/* Cosine of X.  */
extern float cosf (float __x) ; extern float __cosf (float __x) ;
/* Sine of X.  */
extern float sinf (float __x) ; extern float __sinf (float __x) ;
/* Tangent of X.  */
extern float tanf (float __x) ; extern float __tanf (float __x) ;

/* Hyperbolic functions.  */

/* Hyperbolic cosine of X.  */
extern float coshf (float __x) ; extern float __coshf (float __x) ;
/* Hyperbolic sine of X.  */
extern float sinhf (float __x) ; extern float __sinhf (float __x) ;
/* Hyperbolic tangent of X.  */
extern float tanhf (float __x) ; extern float __tanhf (float __x) ;


/* Hyperbolic arc cosine of X.  */
extern float acoshf (float __x) ; extern float __acoshf (float __x) ;
/* Hyperbolic arc sine of X.  */
extern float asinhf (float __x) ; extern float __asinhf (float __x) ;
/* Hyperbolic arc tangent of X.  */
extern float atanhf (float __x) ; extern float __atanhf (float __x) ;



/* Exponential and logarithmic functions.  */


/* Exponential function of X.  */
extern float expf (float __x) ; extern float __expf (float __x) ;

/* Break VALUE into a normalized fraction and an integral power of 2.  */
extern float frexpf (float __x, int *__exponent) ; extern float __frexpf (float __x, int *__exponent) ;

/* X times (two to the EXP power).  */
extern float ldexpf (float __x, int __exponent) ; extern float __ldexpf (float __x, int __exponent) ;

/* Natural logarithm of X.  */
extern float logf (float __x) ; extern float __logf (float __x) ;

/* Base-ten logarithm of X.  */
extern float log10f (float __x) ; extern float __log10f (float __x) ;

/* Break VALUE into integral and fractional parts.  */
extern float modff (float __x, float *__iptr) ; extern float __modff (float __x, float *__iptr) ;


/* Return exp(X) - 1.  */
extern float expm1f (float __x) ; extern float __expm1f (float __x) ;

/* Return log(1 + X).  */
extern float log1pf (float __x) ; extern float __log1pf (float __x) ;

/* Return the base 2 signed integral exponent of X.  */
extern float logbf (float __x) ; extern float __logbf (float __x) ;

/* Power functions.  */


/* Return X to the Y power.  */
extern float powf (float __x, float __y) ; extern float __powf (float __x, float __y) ;

/* Return the square root of X.  */
extern float sqrtf (float __x) ; extern float __sqrtf (float __x) ;




/* Return `sqrt(X*X + Y*Y)'.  */
extern float hypotf (float __x, float __y) ; extern float __hypotf (float __x, float __y) ;





/* Return the cube root of X.  */
extern float cbrtf (float __x) ; extern float __cbrtf (float __x) ;




/* Nearest integer, absolute value, and remainder functions.  */


/* Smallest integral value not less than X.  */
extern float ceilf (float __x) ; extern float __ceilf (float __x) ;

/* Absolute value of X.  */
extern float fabsf (float __x) ; extern float __fabsf (float __x) ;

/* Largest integer not greater than X.  */
extern float floorf (float __x) ; extern float __floorf (float __x) ;

/* Floating-point modulo remainder of X/Y.  */
extern float fmodf (float __x, float __y) ; extern float __fmodf (float __x, float __y) ;


/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int __isinff (float __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int __finitef (float __value) ;



/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int isinff (float __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int finitef (float __value) ;

/* Return the remainder of X/Y.  */
extern float dremf (float __x, float __y) ; extern float __dremf (float __x, float __y) ;


/* Return the fractional part of X after dividing out `ilogb (X)'.  */
extern float significandf (float __x) ; extern float __significandf (float __x) ;




/* Return X with its signed changed to Y's.  */
extern float copysignf (float __x, float __y) ; extern float __copysignf (float __x, float __y) ;

/* Return nonzero if VALUE is not a number.  */
extern int __isnanf (float __value) ;


/* Return nonzero if VALUE is not a number.  */
extern int isnanf (float __value) ;

/* Bessel functions.  */
extern float j0f (float) ; extern float __j0f (float) ;
extern float j1f (float) ; extern float __j1f (float) ;
extern float jnf (int, float) ; extern float __jnf (int, float) ;
extern float y0f (float) ; extern float __y0f (float) ;
extern float y1f (float) ; extern float __y1f (float) ;
extern float ynf (int, float) ; extern float __ynf (int, float) ;





/* Error and gamma functions.  */
extern float erff (float) ; extern float __erff (float) ;
extern float erfcf (float) ; extern float __erfcf (float) ;
extern float lgammaf (float) ; extern float __lgammaf (float) ;

/* Obsolete alias for `lgamma'.  */
extern float gammaf (float) ; extern float __gammaf (float) ;



/* Reentrant version of lgamma.  This function uses the global variable
   `signgam'.  The reentrant version instead takes a pointer and stores
   the value through it.  */
extern float lgammaf_r (float, int *__signgamp) ; extern float __lgammaf_r (float, int *__signgamp) ;





/* Return the integer nearest X in the direction of the
   prevailing rounding mode.  */
extern float rintf (float __x) ; extern float __rintf (float __x) ;

/* Return X + epsilon if X < Y, X - epsilon if X > Y.  */
extern float nextafterf (float __x, float __y) ; extern float __nextafterf (float __x, float __y) ;




/* Return the remainder of integer divison X / Y with infinite precision.  */
extern float remainderf (float __x, float __y) ; extern float __remainderf (float __x, float __y) ;


/* Return X times (2 to the Nth power).  */
extern float scalbnf (float __x, int __n) ; extern float __scalbnf (float __x, int __n) ;


/* Return the binary exponent of X, which must be nonzero.  */
extern int ilogbf (float __x) ; extern int __ilogbf (float __x) ;
/* Include the file of declarations again, this time using `long double'
   instead of `double' and appending l to each function name.  */
/* Prototype declarations for math functions; helper file for <math.h>.
   Copyright (C) 1996-2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* NOTE: Because of the special way this file is used by <math.h>, this
   file must NOT be protected from multiple inclusion as header files
   usually are.

   This file provides prototype declarations for the math functions.
   Most functions are declared using the macro:

   __MATHCALL (NAME,[_r], (ARGS...));

   This means there is a function `NAME' returning `double' and a function
   `NAMEf' returning `float'.  Each place `_Mdouble_' appears in the
   prototype, that is actually `double' in the prototype for `NAME' and
   `float' in the prototype for `NAMEf'.  Reentrant variant functions are
   called `NAME_r' and `NAMEf_r'.

   Functions returning other types like `int' are declared using the macro:

   __MATHDECL (TYPE, NAME,[_r], (ARGS...));

   This is just like __MATHCALL but for a function returning `TYPE'
   instead of `_Mdouble_'.  In all of these cases, there is still
   both a `NAME' and a `NAMEf' that takes `float' arguments.

   Note that there must be no whitespace before the argument passed for
   NAME, to make token pasting work with -traditional.  */






/* Trigonometric functions.  */


/* Arc cosine of X.  */
extern long double acosl (long double __x) ; extern long double __acosl (long double __x) ;
/* Arc sine of X.  */
extern long double asinl (long double __x) ; extern long double __asinl (long double __x) ;
/* Arc tangent of X.  */
extern long double atanl (long double __x) ; extern long double __atanl (long double __x) ;
/* Arc tangent of Y/X.  */
extern long double atan2l (long double __y, long double __x) ; extern long double __atan2l (long double __y, long double __x) ;

/* Cosine of X.  */
extern long double cosl (long double __x) ; extern long double __cosl (long double __x) ;
/* Sine of X.  */
extern long double sinl (long double __x) ; extern long double __sinl (long double __x) ;
/* Tangent of X.  */
extern long double tanl (long double __x) ; extern long double __tanl (long double __x) ;

/* Hyperbolic functions.  */

/* Hyperbolic cosine of X.  */
extern long double coshl (long double __x) ; extern long double __coshl (long double __x) ;
/* Hyperbolic sine of X.  */
extern long double sinhl (long double __x) ; extern long double __sinhl (long double __x) ;
/* Hyperbolic tangent of X.  */
extern long double tanhl (long double __x) ; extern long double __tanhl (long double __x) ;


/* Hyperbolic arc cosine of X.  */
extern long double acoshl (long double __x) ; extern long double __acoshl (long double __x) ;
/* Hyperbolic arc sine of X.  */
extern long double asinhl (long double __x) ; extern long double __asinhl (long double __x) ;
/* Hyperbolic arc tangent of X.  */
extern long double atanhl (long double __x) ; extern long double __atanhl (long double __x) ;



/* Exponential and logarithmic functions.  */


/* Exponential function of X.  */
extern long double expl (long double __x) ; extern long double __expl (long double __x) ;

/* Break VALUE into a normalized fraction and an integral power of 2.  */
extern long double frexpl (long double __x, int *__exponent) ; extern long double __frexpl (long double __x, int *__exponent) ;

/* X times (two to the EXP power).  */
extern long double ldexpl (long double __x, int __exponent) ; extern long double __ldexpl (long double __x, int __exponent) ;

/* Natural logarithm of X.  */
extern long double logl (long double __x) ; extern long double __logl (long double __x) ;

/* Base-ten logarithm of X.  */
extern long double log10l (long double __x) ; extern long double __log10l (long double __x) ;

/* Break VALUE into integral and fractional parts.  */
extern long double modfl (long double __x, long double *__iptr) ; extern long double __modfl (long double __x, long double *__iptr) ;


/* Return exp(X) - 1.  */
extern long double expm1l (long double __x) ; extern long double __expm1l (long double __x) ;

/* Return log(1 + X).  */
extern long double log1pl (long double __x) ; extern long double __log1pl (long double __x) ;

/* Return the base 2 signed integral exponent of X.  */
extern long double logbl (long double __x) ; extern long double __logbl (long double __x) ;

/* Power functions.  */


/* Return X to the Y power.  */
extern long double powl (long double __x, long double __y) ; extern long double __powl (long double __x, long double __y) ;

/* Return the square root of X.  */
extern long double sqrtl (long double __x) ; extern long double __sqrtl (long double __x) ;




/* Return `sqrt(X*X + Y*Y)'.  */
extern long double hypotl (long double __x, long double __y) ; extern long double __hypotl (long double __x, long double __y) ;





/* Return the cube root of X.  */
extern long double cbrtl (long double __x) ; extern long double __cbrtl (long double __x) ;




/* Nearest integer, absolute value, and remainder functions.  */


/* Smallest integral value not less than X.  */
extern long double ceill (long double __x) ; extern long double __ceill (long double __x) ;

/* Absolute value of X.  */
extern long double fabsl (long double __x) ; extern long double __fabsl (long double __x) ;

/* Largest integer not greater than X.  */
extern long double floorl (long double __x) ; extern long double __floorl (long double __x) ;

/* Floating-point modulo remainder of X/Y.  */
extern long double fmodl (long double __x, long double __y) ; extern long double __fmodl (long double __x, long double __y) ;


/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int __isinfl (long double __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int __finitel (long double __value) ;



/* Return 0 if VALUE is finite or NaN, +1 if it
   is +Infinity, -1 if it is -Infinity.  */
extern int isinfl (long double __value) ;

/* Return nonzero if VALUE is finite and not NaN.  */
extern int finitel (long double __value) ;

/* Return the remainder of X/Y.  */
extern long double dreml (long double __x, long double __y) ; extern long double __dreml (long double __x, long double __y) ;


/* Return the fractional part of X after dividing out `ilogb (X)'.  */
extern long double significandl (long double __x) ; extern long double __significandl (long double __x) ;




/* Return X with its signed changed to Y's.  */
extern long double copysignl (long double __x, long double __y) ; extern long double __copysignl (long double __x, long double __y) ;

/* Return nonzero if VALUE is not a number.  */
extern int __isnanl (long double __value) ;


/* Return nonzero if VALUE is not a number.  */
extern int isnanl (long double __value) ;

/* Bessel functions.  */
extern long double j0l (long double) ; extern long double __j0l (long double) ;
extern long double j1l (long double) ; extern long double __j1l (long double) ;
extern long double jnl (int, long double) ; extern long double __jnl (int, long double) ;
extern long double y0l (long double) ; extern long double __y0l (long double) ;
extern long double y1l (long double) ; extern long double __y1l (long double) ;
extern long double ynl (int, long double) ; extern long double __ynl (int, long double) ;





/* Error and gamma functions.  */
extern long double erfl (long double) ; extern long double __erfl (long double) ;
extern long double erfcl (long double) ; extern long double __erfcl (long double) ;
extern long double lgammal (long double) ; extern long double __lgammal (long double) ;

/* Obsolete alias for `lgamma'.  */
extern long double gammal (long double) ; extern long double __gammal (long double) ;



/* Reentrant version of lgamma.  This function uses the global variable
   `signgam'.  The reentrant version instead takes a pointer and stores
   the value through it.  */
extern long double lgammal_r (long double, int *__signgamp) ; extern long double __lgammal_r (long double, int *__signgamp) ;





/* Return the integer nearest X in the direction of the
   prevailing rounding mode.  */
extern long double rintl (long double __x) ; extern long double __rintl (long double __x) ;

/* Return X + epsilon if X < Y, X - epsilon if X > Y.  */
extern long double nextafterl (long double __x, long double __y) ; extern long double __nextafterl (long double __x, long double __y) ;




/* Return the remainder of integer divison X / Y with infinite precision.  */
extern long double remainderl (long double __x, long double __y) ; extern long double __remainderl (long double __x, long double __y) ;


/* Return X times (2 to the Nth power).  */
extern long double scalbnl (long double __x, int __n) ; extern long double __scalbnl (long double __x, int __n) ;


/* Return the binary exponent of X, which must be nonzero.  */
extern int ilogbl (long double __x) ; extern int __ilogbl (long double __x) ;
/* This variable is used by `gamma' and `lgamma'.  */
extern int signgam;



/* ISO C99 defines some generic macros which work on any data type.  */
/* Support for various different standard error handling behaviors.  */
typedef enum
{
  _IEEE_ = -1, /* According to IEEE 754/IEEE 854.  */
  _SVID_, /* According to System V, release 4.  */
  _XOPEN_, /* Nowadays also Unix98.  */
  _POSIX_,
  _ISOC_ /* Actually this is ISO C99.  */
} _LIB_VERSION_TYPE;

/* This variable can be changed at run-time to any of the values above to
   affect floating point error handling behavior (it may also be necessary
   to change the hardware FPU exception settings).  */
extern _LIB_VERSION_TYPE _LIB_VERSION;




/* In SVID error handling, `matherr' is called with this description
   of the exceptional condition.

   We have a problem when using C++ since `exception' is a reserved
   name in C++.  */



struct exception

  {
    int type;
    char *name;
    double arg1;
    double arg2;
    double retval;
  };




extern int matherr (struct exception *__exc);




/* Types of exceptions in the `type' field.  */







/* SVID mode specifies returning this large value instead of infinity.  */
/* Some useful constants.  */
/* The above constants are not adequate for computation using `long double's.
   Therefore we provide as an extension constants with similar names as a
   GNU extension.  Provide enough digits for the 128-bit IEEE quad.  */
/* When compiling in strict ISO C compatible mode we must not use the
   inline functions since they, among other things, do not set the
   `errno' variable correctly.  */




/* Get machine-dependent inline versions (if there are any).  */







void caldat(julian,mm,id,iyyy)
long julian;
int *mm,*id,*iyyy;
{
        long ja,jalpha,jb,jc,jd,je;

        if (julian >= 2299161) {
                jalpha=((float) (julian-1867216)-0.25)/36524.25;
                ja=julian+1+jalpha-(long) (0.25*jalpha);
        } else
                ja=julian;
        jb=ja+1524;
        jc=6680.0+((float) (jb-2439870)-122.1)/365.25;
        jd=365*jc+(0.25*jc);
        je=(jb-jd)/30.6001;
        *id=jb-jd-(int) (30.6001*je);
        *mm=je-1;
        if (*mm > 12) *mm -= 12;
        *iyyy=jc-4715;
        if (*mm > 2) --(*iyyy);
        if (*iyyy <= 0) --(*iyyy);
}
