/*							sinl.c
 *
 *	Circular sine, long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, sinl();
 *
 * y = sinl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Range reduction is into intervals of pi/4.  The reduction
 * error is nearly eliminated by contriving an extended precision
 * modular arithmetic.
 *
 * Two polynomial approximating functions are employed.
 * Between 0 and pi/4 the sine is approximated by the Cody
 * and Waite polynomial form
 *      x + x^3 P(x^2) .
 * Between pi/4 and pi/2 the cosine is represented as
 *      1 - .5 x^2 + x^4 Q(x^2) .
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain      # trials      peak         rms
 *    IEEE     +-3.6e16      100,000    2.0e-34     5.3e-35
 * 
 * ERROR MESSAGES:
 *
 *   message           condition        value returned
 * sin total loss      x > 2^55              0.0
 *
 */
/*							cosl.c
 *
 *	Circular cosine, long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, cosl();
 *
 * y = cosl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Range reduction is into intervals of pi/4.  The reduction
 * error is nearly eliminated by contriving an extended precision
 * modular arithmetic.
 *
 * Two polynomial approximating functions are employed.
 * Between 0 and pi/4 the cosine is approximated by
 *      1 - .5 x^2 + x^4 Q(x^2) .
 * Between pi/4 and pi/2 the sine is represented by the Cody
 * and Waite polynomial form
 *      x  +  x^3 P(x^2) .
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain      # trials      peak         rms
 *    IEEE     +-3.6e16     100,000      2.0e-34     5.2e-35
 *
 * ERROR MESSAGES:
 *
 *   message           condition        value returned
 * cos total loss      x > 2^55              0.0
 */

/*							sin.c	*/

/*
Cephes Math Library Release 2.2:  December, 1990
Copyright 1985, 1990 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

#include "mconf.h"

/* sin(x) = x + x^3 P(x^2)
 * Theoretical peak relative error = 5.6e-39
 * relative peak error spread = 1.7e-9
 */

static long double sincof[12] = {
 6.410290407010279602425714995528976754871E-26L,
-3.868105354403065333804959405965295962871E-23L,
 1.957294039628045847156851410307133941611E-20L,
-8.220635246181818130416407184286068307901E-18L,
 2.811457254345322887443598804951004537784E-15L,
-7.647163731819815869711749952353081768709E-13L,
 1.605904383682161459812515654720205050216E-10L,
-2.505210838544171877505034150892770940116E-8L,
 2.755731922398589065255731765498970284004E-6L,
-1.984126984126984126984126984045294307281E-4L,
 8.333333333333333333333333333333119885283E-3L,
-1.666666666666666666666666666666666647199E-1L
};
/* cos(x) = 1 - .5 x^2 + x^2 (x^2 P(x^2))
 * Theoretical peak relative error = 2.1e-37,
 * relative peak error spread = 1.4e-8
 */
static long double coscof[11] = {
 1.601961934248327059668321782499768648351E-24L,
-8.896621117922334603659240022184527001401E-22L,
 4.110317451243694098169570731967589555498E-19L,
-1.561920696747074515985647487260202922160E-16L,
 4.779477332386900932514186378501779328195E-14L,
-1.147074559772972328629102981460088437917E-11L,
 2.087675698786809897637922200570559726116E-9L,
-2.755731922398589065255365968070684102298E-7L,
 2.480158730158730158730158440896461945271E-5L,
-1.388888888888888888888888888765724370132E-3L,
 4.166666666666666666666666666666459301466E-2L
};
/*
static long double DP1 = 7.853981554508209228515625E-1L;
static long double DP2 =  7.94662735614792836713604629039764404296875E-9L;
static long double DP3 = 3.0616169978683829430651648306875026455243736148E-17L;
static long double lossth = 5.49755813888e11L;
*/
static long double DP1 =
 7.853981633974483067550664827649598009884357452392578125E-1L;
static long double DP2 =
 2.8605943630549158983813312792950660807511260829685741796657E-18L;
static long double DP3 =
 2.1679525325309452561992610065108379921905808E-35L;

static long double lossth =  3.6028797018963968E16L; /* 2^55 */

extern long double PIO4L;


long double sinl(x)
long double x;
{
long double y, z, zz;
int j, sign;
long double polevll(), floorl(), ldexpl();

/* make argument positive but save the sign */
sign = 1;
if( x < 0 )
	{
	x = -x;
	sign = -1;
	}

if( x > lossth )
	{
	mtherr( "sinl", TLOSS );
	return(0.0L);
	}

y = floorl( x/PIO4L ); /* integer part of x/PIO4 */

/* strip high bits of integer part to prevent integer overflow */
z = ldexpl( y, -4 );
z = floorl(z);           /* integer part of y/8 */
z = y - ldexpl( z, 4 );  /* y - 16 * (y/16) */

j = z; /* convert to integer for tests on the phase angle */
/* map zeros to origin */
if( j & 1 )
	{
	j += 1;
	y += 1.0L;
	}
j = j & 07; /* octant modulo 360 degrees */
/* reflect in x axis */
if( j > 3)
	{
	sign = -sign;
	j -= 4;
	}

/* Extended precision modular arithmetic */
z = ((x - y * DP1) - y * DP2) - y * DP3;

zz = z * z;
if( (j==1) || (j==2) )
	{
	y = 1.0L - ldexpl(zz,-1) + zz * zz * polevll( zz, coscof, 10 );
	}
else
	{
	y = z  +  z * (zz * polevll( zz, sincof, 11 ));
	}

if(sign < 0)
	y = -y;

return(y);
}





long double cosl(x)
long double x;
{
long double y, z, zz;
long i;
int j, sign;
long double polevll(), floorl(), ldexpl();


/* make argument positive */
sign = 1;
if( x < 0 )
	x = -x;

if( x > lossth )
	{
	mtherr( "cosl", TLOSS );
	return(0.0L);
	}

y = floorl( x/PIO4L );
z = ldexpl( y, -4 );
z = floorl(z);		/* integer part of y/8 */
z = y - ldexpl( z, 4 );  /* y - 16 * (y/16) */

/* integer and fractional part modulo one octant */
i = z;
if( i & 1 )	/* map zeros to origin */
	{
	i += 1;
	y += 1.0L;
	}
j = i & 07;
if( j > 3)
	{
	j -=4;
	sign = -sign;
	}

if( j > 1 )
	sign = -sign;

/* Extended precision modular arithmetic */
z = ((x - y * DP1) - y * DP2) - y * DP3;

zz = z * z;
if( (j==1) || (j==2) )
	{
	y = z  +  z * (zz * polevll( zz, sincof, 11 ));
	}
else
	{
	y = 1.0L - ldexpl(zz,-1) + zz * zz * polevll( zz, coscof, 10 );
	}

if(sign < 0)
	y = -y;

return(y);
}
