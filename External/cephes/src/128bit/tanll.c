/*							tanl.c
 *
 *	Circular tangent, 128-bit float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, tanl();
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
 *	Circular cotangent, float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, cotl();
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
static float128_t P[6] = {
 {0x09978dc7ae2a2f4bULL, 0xbffefa5d486820e2ULL}, // -9.889929415807650724957118893791829849557E-1L,
 {0x52a017b1ca7c4799ULL, 0x40093e130edd1294ULL}, // 1.272297782199996882828849455156962260810E3L,
 {0x8857161b398b3c53ULL, 0xc0119f024bdcc6c3ULL}, // -4.249691853501233575668486667664718192660E5L,
 {0xcc299261a6616b83ULL, 0x401889b0ed404622ULL}, // 5.160188250214037865511600561074819366815E7L,
 {0x37d9311de4cdbf04ULL, 0xc01e1304fe4d6331ULL}, // -2.307030822693734879744223131873392503321E9L,
 {0x6e9f0eac6b638a9aULL, 0x4021ada98af62f83ULL}, // 2.883414728874239697964612246732416606301E10L
};
static float128_t Q[6] = {
/* 1.000000000000000000000000000000000000000E0L, */
 {0xeb01d728f7d3bb04ULL, 0xc009494f98d3c1caULL}, // -1.317243702830553658702531997959756728291E3L,
 {0xcdd312b4ac46a6cdULL, 0x4011ba538d331a98ULL}, // 4.529422062441341616231663543669583527923E5L,
 {0x2a1a6372eebd73a1ULL, 0xc018b57281a9f10bULL}, // -5.733709132766856723608447733926138506824E7L,
 {0x3e9defb0e348fbe5ULL, 0x401e48d6025d9b41ULL}, // 2.758476078803232151774723646710890525496E9L,
 {0x7cd82869db5580d1ULL, 0xc022355d0fdbd24eULL}, // -4.152206921457208101480801635640958361612E10L,
 {0x92f74b01508aa7f3ULL, 0x4023423f2838a3a2ULL}, // 8.650244186622719093893836740197250197602E10L
};

static float128_t DP1 =
{0x8400000000000000ULL, 0x3ffe921fb54442d1ULL};
 //7.853981633974483067550664827649598009884357452392578125E-1L;
static float128_t DP2 =
{0xe000000000000000ULL, 0x3fc4a62633145c06ULL};
 //2.8605943630549158983813312792950660807511260829685741796657E-18L;
static float128_t DP3 =
{0xa67cc74020bbea64ULL, 0x3f8bcd129024e088ULL};
 // 2.1679525325309452561992610065108379921905808E-35L;

static const float128_t lossth =  {0x0000000000000000ULL, 0x4036000000000000ULL}; // 3.6028797018963968E16L; /* 2^55 */

static const float128_t zero = {0, 0};
static const float128_t one = {0, 0x3fff000000000000ULL};
static const float128_t neg_one = {0, 0xbfff000000000000ULL};

static const float128_t max_quad = {0x35d511e976394d7aULL, 0x3fbc79ca10c92422ULL};

static float128_t tancotl( struct softfloat_state *state, float128_t xx, int cotflg );

float128_t cephes_f128_tanl(float128_t x)
{
struct softfloat_state state = {};
return( tancotl(&state, x,0) );
}


float128_t cotl(float128_t x)
{
struct softfloat_state state = {};

if( f128_eq(&state, x, zero) )
	{
	mtherr( "cotl", SING );
	return( F128_MAXNUML );
	}
return( tancotl(&state, x,1) );
}


static float128_t tancotl( struct softfloat_state *state, float128_t xx, int cotflg )
{
float128_t x, y, z, zz;
int j, sign;

/* make argument positive but save the sign */
// if (xx < 0.0L)
if( f128_lt(state, xx, zero) )
	{
	x = f128_sub(state, zero, xx);
	sign = -1;
	}
else
	{
	x = xx;
	sign = 1;
	}

//if (x > lossth)
if (f128_lt(state, lossth, x))
	{
	if( cotflg )
		mtherr( "cotl", TLOSS );
	else
		mtherr( "tanl", TLOSS );
	return zero;
	}

/* compute x mod PIO4 */
y = cephes_f128_floorl( f128_div(state, x, F128_PIO4L));

/* strip high bits of integer part */
z = cephes_f128_ldexpl( y, -4 );
z = cephes_f128_floorl(z);		/* integer part of y/16 */
z = f128_sub(state, y, cephes_f128_ldexpl( z, 4 ));  /* y - 16 * (y/16) */

/* integer and fractional part modulo one octant */
j = f128_to_i32(state, z, softfloat_round_near_even, true);

/* map zeros and singularities to origin */
if( j & 1 )
	{
	j += 1;
	y = f128_add(state, y, one);
	}

z = f128_sub(state, f128_sub(state, f128_sub(state, x, f128_mul(state, y, DP1)), f128_mul(state, y, DP2)), f128_mul(state, y, DP3));

zz = f128_mul(state, z, z);

// if( zz > 1.0e-20L )
if (f128_lt(state, max_quad, zz))
{
	y = f128_add(state, z, f128_mul(state, z, f128_div(state, f128_mul(state, zz, cephes_f128_polevll( zz, P, 5 )), cephes_f128_p1evll(zz, Q, 6))));
}
else
{
	y = z;
}
	
if( j & 2 )
	{
	if( cotflg )
    y = f128_complement_sign(y);
	else
		y = f128_div(state, neg_one, y);
	}
else
	{
	if( cotflg )
		y = f128_div(state, one, y);
	}

if( sign < 0 )
  y = f128_complement_sign(y);

return( y );
}
