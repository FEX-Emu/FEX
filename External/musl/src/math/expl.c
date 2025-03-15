/* origin: OpenBSD /usr/src/lib/libm/src/ld80/e_expl.c */
/*
 * Copyright (c) 2008 Stephen L. Moshier <steve@moshier.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 *      Exponential function, long double precision
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, expl();
 *
 * y = expl( x );
 *
 *
 * DESCRIPTION:
 *
 * Returns e (2.71828...) raised to the x power.
 *
 * Range reduction is accomplished by separating the argument
 * into an integer k and fraction f such that
 *
 *     x    k  f
 *    e  = 2  e.
 *
 * A Pade' form of degree 5/6 is used to approximate exp(f) - 1
 * in the basic range [-0.5 ln 2, 0.5 ln 2].
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      +-10000     50000       1.12e-19    2.81e-20
 *
 *
 * Error amplification in the exponential function can be
 * a serious matter.  The error propagation involves
 * exp( X(1+delta) ) = exp(X) ( 1 + X*delta + ... ),
 * which shows that a 1 lsb error in representing X produces
 * a relative error of X times 1 lsb in the function.
 * While the routine gives an accurate result for arguments
 * that are exactly represented by a long double precision
 * computer number, the result contains amplified roundoff
 * error for large arguments not exactly represented.
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * exp underflow    x < MINLOG         0.0
 * exp overflow     x > MAXLOG         MAXNUM
 *
 */

#include "libm.h"

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double expl(long double x)
{
	return exp(x);
}
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384

static const long double P[3] = {
 1.2617719307481059087798E-4L,
 3.0299440770744196129956E-2L,
 9.9999999999999999991025E-1L,
};
static const long double Q[4] = {
 3.0019850513866445504159E-6L,
 2.5244834034968410419224E-3L,
 2.2726554820815502876593E-1L,
 2.0000000000000000000897E0L,
};
static const long double
LN2HI = 6.9314575195312500000000E-1L,
LN2LO = 1.4286068203094172321215E-6L,
LOG2E = 1.4426950408889634073599E0L;

long double expl(long double x)
{
	long double px, xx;
	int k;

	if (isnan(x))
		return x;
	if (x > 11356.5234062941439488L) /* x > ln(2^16384 - 0.5) */
		return x * 0x1p16383L;
	if (x < -11399.4985314888605581L) /* x < ln(2^-16446) */
		return -0x1p-16445L/x;

	/* Express e**x = e**f 2**k
	 *   = e**(f + k ln(2))
	 */
	px = floorl(LOG2E * x + 0.5);
	k = px;
	x -= px * LN2HI;
	x -= px * LN2LO;

	/* rational approximation of the fractional part:
	 * e**x =  1 + 2x P(x**2)/(Q(x**2) - x P(x**2))
	 */
	xx = x * x;
	px = x * __polevll(xx, P, 2);
	x = px/(__polevll(xx, Q, 3) - px);
	x = 1.0 + 2.0 * x;
	return scalbnl(x, k);
}
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384
/*							expl.c
 *
 *	Exponential function, 128-bit long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, expl();
 *
 * y = expl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns e (2.71828...) raised to the x power.
 *
 * Range reduction is accomplished by separating the argument
 * into an integer k and fraction f such that
 *
 *     x    k  f
 *    e  = 2  e.
 *
 * A Pade' form of degree 2/3 is used to approximate exp(f) - 1
 * in the basic range [-0.5 ln 2, 0.5 ln 2].
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      +-MAXLOG    100,000     2.6e-34     8.6e-35
 *
 *
 * Error amplification in the exponential function can be
 * a serious matter.  The error propagation involves
 * exp( X(1+delta) ) = exp(X) ( 1 + X*delta + ... ),
 * which shows that a 1 lsb error in representing X produces
 * a relative error of X times 1 lsb in the function.
 * While the routine gives an accurate result for arguments
 * that are exactly represented by a long double precision
 * computer number, the result contains amplified roundoff
 * error for large arguments not exactly represented.
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * exp underflow    x < MINLOG         0.0
 * exp overflow     x > MAXLOG         MAXNUM
 *
 */

/*
Cephes Math Library Release 2.2:  December, 1990
Copyright 1984, 1990 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/


/*	Exponential function	*/

#include "mconf.h"
static char fname[] = {"expl"};

/* Pade' coefficients for exp(x) - 1
   Theoretical peak relative error = 2.2e-37,
   relative peak error spread = 9.2e-38
 */
static long double P[5] = {
 3.279723985560247033712687707263393506266E-10L,
 6.141506007208645008909088812338454698548E-7L,
 2.708775201978218837374512615596512792224E-4L,
 3.508710990737834361215404761139478627390E-2L,
 9.999999999999999999999999999999999998502E-1L
};
static long double Q[6] = {
 2.980756652081995192255342779918052538681E-12L,
 1.771372078166251484503904874657985291164E-8L,
 1.504792651814944826817779302637284053660E-5L,
 3.611828913847589925056132680618007270344E-3L,
 2.368408864814233538909747618894558968880E-1L,
 2.000000000000000000000000000000000000150E0
};
/* C1 + C2 = ln 2 */
static long double C1 = -6.93145751953125E-1L;
static long double C2 = -1.428606820309417232121458176568075500134E-6L;

extern long double LOG2EL, MAXLOGL, MINLOGL, MAXNUML;

long double expl(long double x)
{
long double px, xx;
int n;

	if (isnan(x))
		return x;
	if (x > 11356.5234062941439488L) /* x > ln(2^16384 - 0.5) */
		return x * 0x1p16383L;
	if (x < -11399.4985314888605581L) /* x < ln(2^-16446) */
		return -0x1p-16445L/x;

/* Express e**x = e**g 2**n
 *   = e**g e**( n loge(2) )
 *   = e**( g + n loge(2) )
 */
px = floorl( LOG2EL * x + 0.5L ); /* floor() truncates toward -infinity. */
n = px;
x += px * C1;
x += px * C2;
/* rational approximation for exponential
 * of the fractional part:
 * e**x =  1 + 2x P(x**2)/( Q(x**2) - P(x**2) )
 */
xx = x * x;
px = x * __polevll( xx, P, 4 );
xx = __polevll( xx, Q, 5 );
x =  px/( xx - px );
x = 1.0L + x + x;

x = ldexpl( x, n );
return(x);
}

#endif
