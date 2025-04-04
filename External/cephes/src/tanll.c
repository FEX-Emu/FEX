/*							tanl.c
 *
 *	Circular tangent, 128-bit long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, tanl();
 *
 * y = tanl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the circular tangent of the radian argument x.
 *
 * Range reduction is modulo pi/4.  A rational function
 *       x + x**3 P(x**2)/Q(x**2)
 * is employed in the basic interval [0, pi/4].
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE     +-3.6e16    100,000      3.0e-34      7.2e-35
 *
 * ERROR MESSAGES:
 *
 *   message         condition          value returned
 * tan total loss   x > 2^55                0.0
 *
 */
/*							cotl.c
 *
 *	Circular cotangent, long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, cotl();
 *
 * y = cotl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the circular cotangent of the radian argument x.
 *
 * Range reduction is modulo pi/4.  A rational function
 *       x + x**3 P(x**2)/Q(x**2)
 * is employed in the basic interval [0, pi/4].
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE     +-3.6e16    100,000      2.9e-34     7.2e-35
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition          value returned
 * cot total loss   x > 2^55                0.0
 * cot singularity  x = 0                  MAXNUM
 *
 */

/*
Cephes Math Library Release 2.2:  December, 1990
Copyright 1984, 1990 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

#include "mconf.h"

/* tan(x) = x + x^3 P(x^2)
 * 0 <= |x| <= pi/4
 * Theoretical peak relative error = 4.3e-38
 * relative peak error spread = 6.1e-11
 */
static long double P[6] = {
-9.889929415807650724957118893791829849557E-1L,
 1.272297782199996882828849455156962260810E3L,
-4.249691853501233575668486667664718192660E5L,
 5.160188250214037865511600561074819366815E7L,
-2.307030822693734879744223131873392503321E9L,
 2.883414728874239697964612246732416606301E10L
};
static long double Q[6] = {
/* 1.000000000000000000000000000000000000000E0L, */
-1.317243702830553658702531997959756728291E3L,
 4.529422062441341616231663543669583527923E5L,
-5.733709132766856723608447733926138506824E7L,
 2.758476078803232151774723646710890525496E9L,
-4.152206921457208101480801635640958361612E10L,
 8.650244186622719093893836740197250197602E10L
};

static long double DP1 =
 7.853981633974483067550664827649598009884357452392578125E-1L;
static long double DP2 =
 2.8605943630549158983813312792950660807511260829685741796657E-18L;
static long double DP3 =
 2.1679525325309452561992610065108379921905808E-35L;

static long double lossth =  3.6028797018963968E16L; /* 2^55 */

extern long double PIO4L;
extern long double MAXNUML;
static long double tancotl();

long double tanl(x)
long double x;
{

return( tancotl(x,0) );
}


long double cotl(x)
long double x;
{

if( x == 0.0L )
	{
	mtherr( "cotl", SING );
	return( MAXNUML );
	}
return( tancotl(x,1) );
}


static long double tancotl( xx, cotflg )
long double xx;
int cotflg;
{
long double x, y, z, zz;
int j, sign;
long double polevll(), p1evll(), floorl(), ldexpl();

/* make argument positive but save the sign */
if( xx < 0.0L )
	{
	x = -xx;
	sign = -1;
	}
else
	{
	x = xx;
	sign = 1;
	}

if( x > lossth )
	{
	if( cotflg )
		mtherr( "cotl", TLOSS );
	else
		mtherr( "tanl", TLOSS );
	return(0.0L);
	}

/* compute x mod PIO4 */
y = floorl( x/PIO4L );

/* strip high bits of integer part */
z = ldexpl( y, -4 );
z = floorl(z);		/* integer part of y/16 */
z = y - ldexpl( z, 4 );  /* y - 16 * (y/16) */

/* integer and fractional part modulo one octant */
j = z;

/* map zeros and singularities to origin */
if( j & 1 )
	{
	j += 1;
	y += 1.0L;
	}

z = ((x - y * DP1) - y * DP2) - y * DP3;

zz = z * z;

if( zz > 1.0e-20L )
	y = z  +  z * (zz * polevll( zz, P, 5 )/p1evll(zz, Q, 6));
else
	y = z;
	
if( j & 2 )
	{
	if( cotflg )
		y = -y;
	else
		y = -1.0L/y;
	}
else
	{
	if( cotflg )
		y = 1.0L/y;
	}

if( sign < 0 )
	y = -y;

return( y );
}
