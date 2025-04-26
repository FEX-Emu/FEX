/*							sinl.c
 *
 *	Circular sine, float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, sinl();
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
 *	Circular cosine, float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, cosl();
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

static float128_t sincof[12] = {
{0x07424c0cc240ddd5ULL, 0x3fab3d6c15b6d187ULL}, // 6.410290407010279602425714995528976754871E-26L,
{0x0f48760e659301d0ULL, 0xbfb47619a65f0be7ULL}, // -3.868105354403065333804959405965295962871E-23L,
{0xcb791f8ea7c13184ULL, 0x3fbd71b8ee9a64e1ULL}, // 1.957294039628045847156851410307133941611E-20L,
{0x0b420eabbeb9d9bcULL, 0xbfc62f49b467cdf7ULL}, // -8.220635246181818130416407184286068307901E-18L,
{0x4be70cee4054eef9ULL, 0x3fce952c77030ab5ULL}, // 2.811457254345322887443598804951004537784E-15L,
{0xe782874b38cbd281ULL, 0xbfd6ae7f3e733b81ULL}, // -7.647163731819815869711749952353081768709E-13L,
{0x97c83627668fe57cULL, 0x3fde6124613a86d0ULL}, // 1.605904383682161459812515654720205050216E-10L,
{0x38fe73eef2ec94cdULL, 0xbfe5ae64567f544eULL}, // -2.505210838544171877505034150892770940116E-8L,
{0x38faac1c6f6fa52aULL, 0x3fec71de3a556c73ULL}, // 2.755731922398589065255731765498970284004E-6L,
{0xa01a01a019fc52ccULL, 0xbff2a01a01a01a01ULL}, // -1.984126984126984126984126984045294307281E-4L,
{0x1111111111111083ULL, 0x3ff8111111111111ULL}, // 8.333333333333333333333333333333119885283E-3L,
{0x5555555555555555ULL, 0xbffc555555555555ULL}, // -1.666666666666666666666666666666666647199E-1L
};
/* cos(x) = 1 - .5 x^2 + x^2 (x^2 P(x^2))
 * Theoretical peak relative error = 2.1e-37,
 * relative peak error spread = 1.4e-8
 */
static float128_t coscof[11] = {
{0x86919a6fdf15a4b3ULL, 0x3fafefc8801eb0a1ULL}, // 1.601961934248327059668321782499768648351E-24L,
{0x902367b3281c9510ULL, 0xbfb90ce245980e11ULL}, // -8.896621117922334603659240022184527001401E-22L,
{0xcf5102d043ad399aULL, 0x3fc1e542b8eb4f0dULL}, // 4.110317451243694098169570731967589555498E-19L,
{0xa8272970c73ab5ffULL, 0xbfca6827863b2960ULL}, // -1.561920696747074515985647487260202922160E-16L,
{0xf9016edb75d1fb52ULL, 0x3fd2ae7f3e733b51ULL}, // 4.779477332386900932514186378501779328195E-14L,
{0xc3e862188c1c1f15ULL, 0xbfda93974a8c07c9ULL}, // -1.147074559772972328629102981460088437917E-11L,
{0x7b517ff3abf58399ULL, 0x3fe21eed8eff8d89ULL}, // 2.087675698786809897637922200570559726116E-9L,
{0xc72eef5d4453f45cULL, 0xbfe927e4fb7789f5ULL}, // -2.755731922398589065255365968070684102298E-7L,
{0xa01a019fdf56450dULL, 0x3fefa01a01a01a01ULL}, // 2.480158730158730158730158440896461945271E-5L,
{0x6c16c16c16b76e10ULL, 0xbff56c16c16c16c1ULL}, // -1.388888888888888888888888888765724370132E-3L,
{0x55555555555553fdULL, 0x3ffa555555555555ULL}, // 4.166666666666666666666666666666459301466E-2L
};
/*
static float128_t DP1 = 7.853981554508209228515625E-1L;
static float128_t DP2 =  7.94662735614792836713604629039764404296875E-9L;
static float128_t DP3 = 3.0616169978683829430651648306875026455243736148E-17L;
static float128_t lossth = 5.49755813888e11L;
*/
static float128_t DP1 =
{0x8400000000000000ULL, 0x3ffe921fb54442d1ULL};
 //7.853981633974483067550664827649598009884357452392578125E-1L;
static float128_t DP2 =
{0xe000000000000000ULL, 0x3fc4a62633145c06ULL};
 //2.8605943630549158983813312792950660807511260829685741796657E-18L;
static float128_t DP3 =
{0xa67cc74020bbea64ULL, 0x3f8bcd129024e088ULL};
 //2.1679525325309452561992610065108379921905808E-35L;

static const float128_t lossth =  {0x0000000000000000ULL, 0x4036000000000000ULL}; // 3.6028797018963968E16L; /* 2^55 */
static const float128_t zero = {0, 0};
static const float128_t one = {0, 0x3fff000000000000ULL};

float128_t cephes_f128_sinl(float128_t x)
{
struct softfloat_state state = {};
float128_t y, z, zz;
int j, sign;

/* make argument positive but save the sign */
sign = 1;
if( f128_lt(&state, x, zero) )
	{
	x = f128_complement_sign(x);
	sign = -1;
	}

if( f128_lt(&state, lossth, x))
	{
	mtherr( "sinl", TLOSS );
	return zero;
	}

y = cephes_f128_floorl( f128_div(&state, x, F128_PIO4L) ); /* integer part of x/PIO4 */

/* strip high bits of integer part to prevent integer overflow */
z = cephes_f128_ldexpl( y, -4 );
z = cephes_f128_floorl(z);           /* integer part of y/8 */
z = f128_sub(&state, y, cephes_f128_ldexpl( z, 4 ));  /* y - 16 * (y/16) */

j = f128_to_i32(&state, z, softfloat_round_near_even, true); /* convert to integer for tests on the phase angle */
/* map zeros to origin */
if( j & 1 )
	{
	j += 1;
	y = f128_add(&state, y, one);
	}
j = j & 07; /* octant modulo 360 degrees */
/* reflect in x axis */
if( j > 3)
	{
	sign = -sign;
	j -= 4;
	}

/* Extended precision modular arithmetic */
// z = ((x - y * DP1) - y * DP2) - y * DP3;
 {
   float128_t tmp1 = f128_mul(&state, y, DP1);
   float128_t tmp2 = f128_mul(&state, y, DP2);
   float128_t tmp3 = f128_mul(&state, y, DP3);
   float128_t tmp4 = f128_sub(&state, x, tmp1);
   float128_t tmp5 = f128_sub(&state, tmp4, tmp2);
   z = f128_sub(&state, tmp5, tmp3);
 }

z = f128_sub(&state, f128_sub(&state, f128_sub(&state, x, f128_mul(&state, y, DP1)), f128_mul(&state, y, DP2)), f128_mul(&state, y, DP3));

zz = f128_mul(&state, z, z);
if( (j==1) || (j==2) )
	{
  // y = 1.0L - ldexpl(zz,-1) + zz * zz * polevll( zz, coscof, 10 );
  float128_t tmp1 = f128_mul(&state, zz, zz);
  float128_t tmp2 = f128_mul(&state, tmp1, cephes_f128_polevll( zz, coscof, 10 ));
  float128_t tmp3 = f128_sub(&state, one, cephes_f128_ldexpl(zz,-1));
  y = f128_add(&state, tmp3, tmp2);
	}
else
	{
  // y = z  +  z * (zz * polevll( zz, sincof, 11 ));
  float128_t tmp1 = f128_mul(&state, zz, cephes_f128_polevll( zz, sincof, 11 ));
  float128_t tmp2 = f128_mul(&state, z, tmp1);
  y = f128_add(&state, z, tmp2);
	}

if(sign < 0)
	y = f128_complement_sign(y);

return(y);
}





float128_t cephes_f128_cosl(float128_t x)
{
struct softfloat_state state = {};
float128_t y, z, zz;
long i;
int j, sign;

/* make argument positive */
sign = 1;
if( f128_lt(&state, x, zero) )
	x = f128_complement_sign(x);


if( f128_lt(&state, lossth, x))
	{
	mtherr( "cosl", TLOSS );
	return zero;
	}

y = cephes_f128_floorl( f128_div(&state, x, F128_PIO4L));
z = cephes_f128_ldexpl( y, -4 );
z = cephes_f128_floorl(z);		/* integer part of y/8 */
z = f128_sub(&state, y, cephes_f128_ldexpl( z, 4 ));  /* y - 16 * (y/16) */

/* integer and fractional part modulo one octant */
i = f128_to_i32(&state, z, softfloat_round_near_even, true);
if( i & 1 )	/* map zeros to origin */
	{
	i += 1;
	y = f128_add(&state, y, one);
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
// z = ((x - y * DP1) - y * DP2) - y * DP3;
 {
   float128_t tmp1 = f128_mul(&state, y, DP1);
   float128_t tmp2 = f128_mul(&state, y, DP2);
   float128_t tmp3 = f128_mul(&state, y, DP3);
   float128_t tmp4 = f128_sub(&state, x, tmp1);
   float128_t tmp5 = f128_sub(&state, tmp4, tmp2);
   z = f128_sub(&state, tmp5, tmp3);
 }

zz = f128_mul(&state, z, z);
if( (j==1) || (j==2) )
	{
    // y = z  +  z * (zz * polevll( zz, sincof, 11 ));
    float128_t tmp1 = f128_mul(&state, zz, cephes_f128_polevll( zz, sincof, 11 ));
    float128_t tmp2 = f128_mul(&state, z, tmp1);
    y = f128_add(&state, z, tmp2);
	}
else
	{
    // y = 1.0L - ldexpl(zz,-1) + zz * zz * polevll( zz, coscof, 10 );
    float128_t tmp1 = f128_mul(&state, zz, zz);
    float128_t tmp2 = f128_mul(&state, tmp1, cephes_f128_polevll( zz, coscof, 10 ));
    float128_t tmp3 = f128_sub(&state, one, cephes_f128_ldexpl(zz,-1));
    y = f128_add(&state, tmp3, tmp2);
	}

if(sign < 0)
	y = f128_complement_sign(y);

return(y);
}
