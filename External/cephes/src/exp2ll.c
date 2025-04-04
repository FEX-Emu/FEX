/*							exp2l.c
 *
 *	Base 2 exponential function, 128-bit long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, exp2l();
 *
 * y = exp2l( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns 2 raised to the x power.
 *
 * Range reduction is accomplished by separating the argument
 * into an integer k and fraction f such that
 *     x    k  f
 *    2  = 2  2.
 *
 * A Pade' form
 *
 *   1 + 2x P(x**2) / (Q(x**2) - x P(x**2) )
 *
 * approximates 2**x in the basic range [-0.5, 0.5].
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      +-16300    100,000      2.0e-34     4.8e-35
 *
 *
 * See exp.c for comments on error amplification.
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * exp2l underflow   x < -16382        0.0
 * exp2l overflow    x >= 16384       MAXNUM
 *
 */


/*
Cephes Math Library Release 2.2:  January, 1991
Copyright 1984, 1991 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/



#include "mconf.h"
static char fname[] = {"exp2l"};

/* Pade' coefficients for 2^x - 1
   Theoretical peak relative error = 1.4e-40,
   relative peak error spread = 6.8e-14
 */
static long double P[5] = {
 1.587171580015525194694938306936721666031E2L,
 6.185032670011643762127954396427045467506E5L,
 5.677513871931844661829755443994214173883E8L,
 1.530625323728429161131811299626419117557E11L,
 9.079594442980146270952372234833529694788E12L
};
static long double Q[5] = {
/* 1.000000000000000000000000000000000000000E0L, */
 1.236602014442099053716561665053645270207E4L,
 2.186249607051644894762167991800811827835E7L,
 1.092141473886177435056423606755843616331E10L,
 1.490560994263653042761789432690793026977E12L,
 2.619817175234089411411070339065679229869E13L
};


#define MAXL2 16384.0L
#define MINL2 -16382.0L


extern long double MAXNUML;

long double exp2l(x)
long double x;
{
long double px, xx;
int n;
long double polevll(), p1evll(), floorl(), ldexpl();

if( x >= MAXL2)
	{
	mtherr( fname, OVERFLOW );
	return( MAXNUML );
	}

if( x < MINL2 )
	{
	mtherr( fname, UNDERFLOW );
	return(0.0L);
	}

xx = x;	/* save x */
/* separate into integer and fractional parts */
px = floorl(x+0.5L);
n = px;
x = x - px;

/* rational approximation
 * exp2(x) = 1.0 +  2xP(xx)/(Q(xx) - P(xx))
 * where xx = x**2
 */
xx = x * x;
px = x * polevll( xx, P, 4 );
x =  px / ( p1evll( xx, Q, 5 ) - px );
x = 1.0L + ldexpl( x, 1 );

/* scale by power of 2 */
x = ldexpl( x, n );
return(x);
}
