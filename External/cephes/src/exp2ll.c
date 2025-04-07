/*							exp2l.c
 *
 *	Base 2 exponential function, 128-bit float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, exp2l();
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
static float128_t P[5] = {
 {0x3008ca100ca13471ULL, 0x40063d6f2f556577ULL}, // 1.587171580015525194694938306936721666031E2L,
 {0x9fac10fe43d72769ULL, 0x40122e00e88b4606ULL}, // 6.185032670011643762127954396427045467506E5L,
 {0x4c22cf0c6c7a8fc7ULL, 0x401c0eb996d98ba4ULL}, // 5.677513871931844661829755443994214173883E8L,
 {0x4acd9b1339dda08aULL, 0x40241d19e728a6beULL}, // 1.530625323728429161131811299626419117557E11L,
 {0xae406b996488ba7aULL, 0x402a0840400c1c84ULL}, // 9.079594442980146270952372234833529694788E12L
};
static float128_t Q[5] = {
/* 1.000000000000000000000000000000000000000E0L, */
 {0xcf48c9db239c2189ULL, 0x400c827029417a6aULL}, // 1.236602014442099053716561665053645270207E4L,
 {0xb20f61f9a3c778b9ULL, 0x40174d9860120d5dULL}, // 2.186249607051644894762167991800811827835E7L,
 {0x9f361a3e85f209ceULL, 0x4020457bc8296e4eULL}, // 1.092141473886177435056423606755843616331E10L,
 {0x2dcf78c66f0a65ddULL, 0x40275b0c5bcbd7a7ULL}, // 1.490560994263653042761789432690793026977E12L,
 {0x4e4a9905cf9c9235ULL, 0x402b7d3bcb89794eULL}, // 2.619817175234089411411070339065679229869E13L
};

static const float128_t MAXL2 = {0x0000000000000000ULL, 0x400d000000000000ULL};
static const float128_t MINL2 = {0x0000000000000000ULL, 0xc00cfff000000000ULL};
static const float128_t zero = {0, 0};
static const float128_t f_0_p5 = {0, 0x3ffe000000000000ULL};
static const float128_t one = {0, 0x3fff000000000000ULL};

extern float128_t MAXNUML;

float128_t cephes_f128_exp2l(float128_t x) {
struct softfloat_state state = {};
float128_t px, xx;
int n;

if( f128_le(&state, MAXL2, x))
	{
	mtherr( fname, OVERFLOW );
	return( MAXNUML );
	}

if(f128_lt(&state, x, MINL2) )
	{
	mtherr( fname, UNDERFLOW );
	return zero;
	}

xx = x;	/* save x */
/* separate into integer and fractional parts */
px = cephes_f128_floorl(f128_add(&state, x, f_0_p5));
n = f128_to_i32(&state, px, softfloat_round_near_even, true);
x = f128_sub(&state, x, px);

/* rational approximation
 * exp2(x) = 1.0 +  2xP(xx)/(Q(xx) - P(xx))
 * where xx = x**2
 */
xx = f128_mul(&state, x, x);
px = f128_mul(&state, x, cephes_f128_polevll( xx, P, 4 ));
x = f128_div(&state, px, f128_sub(&state, cephes_f128_p1evll( xx, Q, 5 ), px));
x = f128_add(&state, one, cephes_f128_ldexpl( x, 1 ));

/* scale by power of 2 */
x = cephes_f128_ldexpl( x, n );
return(x);
}
