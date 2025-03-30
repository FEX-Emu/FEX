/*							atanl.c
 *
 *	Inverse circular tangent, 128-bit long double precision
 *      (arctangent)
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, atanl();
 *
 * y = atanl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns radian angle between -pi/2 and +pi/2 whose tangent
 * is x.
 *
 * Range reduction is from four intervals into the interval
 * from zero to  tan( pi/8 ).  The approximant uses a rational
 * function of degree 3/4 of the form x + x**3 P(x)/Q(x).
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      -10, 10    100,000      2.6e-34     6.5e-35
 *
 */
/*							atan2l()
 *
 *	Quadrant correct inverse circular tangent,
 *	long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, z, atan2l();
 *
 * z = atan2l( y, x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns radian angle whose tangent is y/x.
 * Define compile time symbol ANSIC = 1 for ANSI standard,
 * range -PI < z <= +PI, args (y,x); else ANSIC = 0 for range
 * 0 to 2PI, args (x,y).
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      -10, 10    100,000      3.2e-34      5.9e-35
 * See atan.c.
 *
 */

/*							atan.c */


/*
Cephes Math Library Release 2.2:  December, 1990
Copyright 1984, 1990 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/


#include "mconf.h"

/* arctan(x) = x + x^3 P(x^2)
 * Theoretical peak relative error = 3.0e-36
 * relative peak error spread = 6.6e-8
 */
static long double P[9] = {
-6.635810778635296712545011270011752799963E-4L,
-8.768423468036849091777415076702113400070E-1L,
-2.548067867495502632615671450650071218995E1L,
-2.497759878476618348858065206895055957104E2L,
-1.148164399808514330375280133523543970854E3L,
-2.792272753241044941703278827346430350236E3L,
-3.696264445691821235400930243493001671932E3L,
-2.514829758941713674909996882101723647996E3L,
-6.880597774405940432145577545328795037141E2L
};
static long double Q[8] = {
/* 1.000000000000000000000000000000000000000E0L, */
 3.566239794444800849656497338030115886153E1L,
 4.308348370818927353321556740027020068897E2L,
 2.494680540950601626662048893678584497900E3L,
 7.928572347062145288093560392463784743935E3L,
 1.458510242529987155225086911411015961174E4L,
 1.547394317752562611786521896296215170819E4L,
 8.782996876218210302516194604424986107121E3L,
 2.064179332321782129643673263598686441900E3L
};

/* tan( 3*pi/8 ) */
static long double T3P8 = 2.414213562373095048801688724209698078569672L;

/* tan( pi/8 ) */
static long double TP8 = 0.414213562373095048801688724209698078569672L;




long double atanl(x)
long double x;
{
extern long double PIO2L, PIO4L;
long double y, z;
long double polevll(), p1evll();
short sign;

/* make argument positive and save the sign */
sign = 1;
if( x < 0.0L )
	{
	sign = -1;
	x = -x;
	}

/* range reduction */
if( x > T3P8 )
	{
	y = PIO2L;
	x = -( 1.0L/x );
	}

else if( x > TP8 )
	{
	y = PIO4L;
	x = (x-1.0L)/(x+1.0L);
	}
else
	y = 0.0L;

/* rational form in x**2 */
z = x * x;
y = y + ( polevll( z, P, 8 ) / p1evll( z, Q, 8 ) ) * z * x + x;

if( sign < 0 )
	y = -y;

return(y);
}

/*							atan2	*/


extern long double PIL, PIO2L;

#if ANSIC
long double atan2l( y, x )
#else
long double atan2l( x, y )
#endif
long double x, y;
{
long double z, w;
short code;
long double atanl();


code = 0;
w = 0.0L;

if( x < 0.0L )
	code = 2;
if( y < 0.0L )
	code |= 1;

if( x == 0.0L )
	{
	if( code & 1 )
		{
#if ANSIC
		return( -PIO2L );
#else
		return( 3.0L*PIO2L );
#endif
		}
	if( y == 0.0L )
		return( 0.0L );
	return( PIO2L );
	}

if( y == 0.0L )
	{
	if( code & 2 )
		return( PIL );
	return( 0.0L );
	}


switch( code )
	{
#if ANSIC
	case 0:
	case 1: w = 0.0L; break;
	case 2: w = PIL; break;
	case 3: w = -PIL; break;
#else
	case 0: w = 0.0L; break;
	case 1: w = 2.0L * PIL; break;
	case 2:
	case 3: w = PIL; break;
#endif
	}

z = atanl( y/x );

return( w + z );
}
