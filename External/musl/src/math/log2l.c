/* origin: OpenBSD /usr/src/lib/libm/src/ld80/e_log2l.c */
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
 *      Base 2 logarithm, long double precision
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, log2l();
 *
 * y = log2l( x );
 *
 *
 * DESCRIPTION:
 *
 * Returns the base 2 logarithm of x.
 *
 * The argument is separated into its exponent and fractional
 * parts.  If the exponent is between -1 and +1, the (natural)
 * logarithm of the fraction is approximated by
 *
 *     log(1+x) = x - 0.5 x**2 + x**3 P(x)/Q(x).
 *
 * Otherwise, setting  z = 2(x-1)/x+1),
 *
 *     log(x) = z + z**3 P(z)/Q(z).
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      0.5, 2.0     30000      9.8e-20     2.7e-20
 *    IEEE     exp(+-10000)  70000      5.4e-20     2.3e-20
 *
 * In the tests over the interval exp(+-10000), the logarithms
 * of the random arguments were uniformly distributed over
 * [-10000, +10000].
 */

#include "libm.h"

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
long double log2l(long double x)
{
	return log2(x);
}
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
/* Coefficients for ln(1+x) = x - x**2/2 + x**3 P(x)/Q(x)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 6.2e-22
 */
static const long double P[] = {
 4.9962495940332550844739E-1L,
 1.0767376367209449010438E1L,
 7.7671073698359539859595E1L,
 2.5620629828144409632571E2L,
 4.2401812743503691187826E2L,
 3.4258224542413922935104E2L,
 1.0747524399916215149070E2L,
};
static const long double Q[] = {
/* 1.0000000000000000000000E0,*/
 2.3479774160285863271658E1L,
 1.9444210022760132894510E2L,
 7.7952888181207260646090E2L,
 1.6911722418503949084863E3L,
 2.0307734695595183428202E3L,
 1.2695660352705325274404E3L,
 3.2242573199748645407652E2L,
};

/* Coefficients for log(x) = z + z^3 P(z^2)/Q(z^2),
 * where z = 2(x-1)/(x+1)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 6.16e-22
 */
static const long double R[4] = {
 1.9757429581415468984296E-3L,
-7.1990767473014147232598E-1L,
 1.0777257190312272158094E1L,
-3.5717684488096787370998E1L,
};
static const long double S[4] = {
/* 1.00000000000000000000E0L,*/
-2.6201045551331104417768E1L,
 1.9361891836232102174846E2L,
-4.2861221385716144629696E2L,
};
/* log2(e) - 1 */
#define LOG2EA 4.4269504088896340735992e-1L

#define SQRTH 0.70710678118654752440L

long double log2l(long double x)
{
	long double y, z;
	int e;

	if (isnan(x))
		return x;
	if (x == INFINITY)
		return x;
	if (x <= 0.0) {
		if (x == 0.0)
			return -1/(x*x); /* -inf with divbyzero */
		return 0/0.0f; /* nan with invalid */
	}

	/* separate mantissa from exponent */
	/* Note, frexp is used so that denormal numbers
	 * will be handled properly.
	 */
	x = frexpl(x, &e);

	/* logarithm using log(x) = z + z**3 P(z)/Q(z),
	 * where z = 2(x-1)/x+1)
	 */
	if (e > 2 || e < -2) {
		if (x < SQRTH) {  /* 2(2x-1)/(2x+1) */
			e -= 1;
			z = x - 0.5;
			y = 0.5 * z + 0.5;
		} else {  /*  2 (x-1)/(x+1)   */
			z = x - 0.5;
			z -= 0.5;
			y = 0.5 * x + 0.5;
		}
		x = z / y;
		z = x*x;
		y = x * (z * __polevll(z, R, 3) / __p1evll(z, S, 3));
		goto done;
	}

	/* logarithm using log(1+x) = x - .5x**2 + x**3 P(x)/Q(x) */
	if (x < SQRTH) {
		e -= 1;
		x = 2.0*x - 1.0;
	} else {
		x = x - 1.0;
	}
	z = x*x;
	y = x * (z * __polevll(x, P, 6) / __p1evll(x, Q, 7));
	y = y - 0.5*z;

done:
	/* Multiply log of fraction by log2(e)
	 * and base 2 exponent by 1
	 *
	 * ***CAUTION***
	 *
	 * This sequence of operations is critical and it may
	 * be horribly defeated by some compiler optimizers.
	 */
	z = y * LOG2EA;
	z += x * LOG2EA;
	z += y;
	z += x;
	z += e;
	return z;
}
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384
/*							log2l.c
 *
 *	Base 2 logarithm, long double precision
 *
 *
 *
 * SYNOPSIS:
 *
 * long double x, y, log2l();
 *
 * y = log2l( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the base 2 logarithm of x.
 *
 * The argument is separated into its exponent and fractional
 * parts.  If the exponent is between -1 and +1, the (natural)
 * logarithm of the fraction is approximated by
 *
 *     log(1+x) = x - 0.5 x**2 + x**3 P(x)/Q(x).
 *
 * Otherwise, setting  z = 2(x-1)/x+1),
 * 
 *     log(x) = z + z**3 P(z)/Q(z).
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      0.5, 2.0     100,000    1.3e-34     4.5e-35
 *    IEEE     exp(+-10000)  100,000    9.6e-35     4.0e-35
 *
 * In the tests over the interval exp(+-10000), the logarithms
 * of the random arguments were uniformly distributed over
 * [-10000, +10000].
 *
 * ERROR MESSAGES:
 *
 * log singularity:  x = 0; returns MINLOG
 * log domain:       x < 0; returns MINLOG
 */

/*
Cephes Math Library Release 2.2:  January, 1991
Copyright 1984, 1991 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

static char fname[] = {"log2l"};

/* Coefficients for ln(1+x) = x - x**2/2 + x**3 P(x)/Q(x)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 5.3e-37,
 * relative peak error spread = 2.3e-14
 */
static long double P[13] = {
 1.538612243596254322971797716843006400388E-6L,
 4.998469661968096229986658302195402690910E-1L,
 2.321125933898420063925789532045674660756E1L,
 4.114517881637811823002128927449878962058E2L,
 3.824952356185897735160588078446136783779E3L,
 2.128857716871515081352991964243375186031E4L,
 7.594356839258970405033155585486712125861E4L,
 1.797628303815655343403735250238293741397E5L,
 2.854829159639697837788887080758954924001E5L,
 3.007007295140399532324943111654767187848E5L,
 2.014652742082537582487669938141683759923E5L,
 7.771154681358524243729929227226708890930E4L,
 1.313572404063446165910279910527789794488E4L
};
static long double Q[12] = {
/* 1.000000000000000000000000000000000000000E0L, */
 4.839208193348159620282142911143429644326E1L,
 9.104928120962988414618126155557301584078E2L,
 9.147150349299596453976674231612674085381E3L,
 5.605842085972455027590989944010492125825E4L,
 2.248234257620569139969141618556349415120E5L,
 6.132189329546557743179177159925690841200E5L,
 1.158019977462989115839826904108208787040E6L,
 1.514882452993549494932585972882995548426E6L,
 1.347518538384329112529391120390701166528E6L,
 7.777690340007566932935753241556479363645E5L,
 2.626900195321832660448791748036714883242E5L,
 3.940717212190338497730839731583397586124E4L
};

/* Coefficients for log(x) = z + z^3 P(z^2)/Q(z^2),
 * where z = 2(x-1)/(x+1)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 1.1e-35,
 * relative peak error spread 1.1e-9
 */
static long double R[6] = {
-8.828896441624934385266096344596648080902E-1L,
 8.057002716646055371965756206836056074715E1L,
-2.024301798136027039250415126250455056397E3L,
 2.048819892795278657810231591630928516206E4L,
-8.977257995689735303686582344659576526998E4L,
 1.418134209872192732479751274970992665513E5L
};
static long double S[6] = {
/* 1.000000000000000000000000000000000000000E0L, */
-1.186359407982897997337150403816839480438E2L,
 3.998526750980007367835804959888064681098E3L,
-5.748542087379434595104154610899551484314E4L,
 4.001557694070773974936904547424676279307E5L,
-1.332535117259762928288745111081235577029E6L,
 1.701761051846631278975701529965589676574E6L
};
/* log2(e) - 1 */
#define LOG2EA 4.4269504088896340735992468100189213742664595E-1L

#define SQRTH 7.071067811865475244008443621048490392848359E-1L

extern long double MINLOGL;

long double log2l(long double x)
{
long double y, z;
int e;

	if (isnan(x))
		return x;
	if (x == INFINITY)
		return x;
	if (x <= 0.0) {
		if (x == 0.0)
			return -1/(x*x); /* -inf with divbyzero */
		return 0/0.0f; /* nan with invalid */
	}


/* separate mantissa from exponent */

/* Note, frexp is used so that denormal numbers
 * will be handled properly.
 */
x = frexpl( x, &e );


/* logarithm using log(x) = z + z**3 P(z)/Q(z),
 * where z = 2(x-1)/x+1)
 */
if( (e > 2) || (e < -2) )
{
if( x < SQRTH )
	{ /* 2( 2x-1 )/( 2x+1 ) */
	e -= 1;
	z = x - 0.5L;
	y = 0.5L * z + 0.5L;
	}	
else
	{ /*  2 (x-1)/(x+1)   */
	z = x - 0.5L;
	z -= 0.5L;
	y = 0.5L * x  + 0.5L;
	}
x = z / y;
z = x*x;
y = x * ( z * __polevll( z, R, 5 ) / __p1evll( z, S, 6 ) );
goto done;
}


/* logarithm using log(1+x) = x - .5x**2 + x**3 P(x)/Q(x) */

if( x < SQRTH )
	{
	e -= 1;
	x = ldexpl( x, 1 ) - 1.0L; /*  2x - 1  */
	}	
else
	{
	x = x - 1.0L;
	}
z = x*x;
y = x * ( z * __polevll( x, P, 12 ) / __p1evll( x, Q, 12 ) );
y = y - ldexpl( z, -1 );   /* -0.5x^2 + ... */

done:

/* Multiply log of fraction by log2(e)
 * and base 2 exponent by 1
 *
 * ***CAUTION***
 *
 * This sequence of operations is critical and it may
 * be horribly defeated by some compiler optimizers.
 */
z = y * LOG2EA;
z += x * LOG2EA;
z += y;
z += x;
z += e;
return( z );
}
#endif
