/*							atanl.c
 *
 *	Inverse circular tangent, 128-bit float128_t precision
 *      (arctangent)
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, atanl();
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
 *	float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, z, atan2l();
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
static float128_t P[9] = {
{0xf3f0105b1dae46bfULL, 0xbff45be85838aa26ULL}, // -6.635810778635296712545011270011752799963E-4L,
{0x529a2bf25f15874bULL, 0xbffec0f17ae68a18ULL}, // -8.768423468036849091777415076702113400070E-1L,
{0x3054a2e7144e265cULL, 0xc00397b0dc1f4d10ULL}, // -2.548067867495502632615671450650071218995E1L,
{0x1e19d6b8c5cd9e65ULL, 0xc006f38d4e47779aULL}, // -2.497759878476618348858065206895055957104E2L,
{0x69dcb1e41a413bddULL, 0xc0091f0a8586c642ULL}, // -1.148164399808514330375280133523543970854E3L,
{0x501d0f5157516744ULL, 0xc00a5d08ba650145ULL}, // -2.792272753241044941703278827346430350236E3L,
{0x16f18bf3f5b4b987ULL, 0xc00ace087656cfbeULL}, // -3.696264445691821235400930243493001671932E3L,
{0x2966de608cbf9696ULL, 0xc00a3a5a8d629fc7ULL}, // -2.514829758941713674909996882101723647996E3L,
{0xeb77db69572ecd22ULL, 0xc0085807a6c98431ULL}, // -6.880597774405940432145577545328795037141E2L
};
static float128_t Q[8] = {
/* 1.000000000000000000000000000000000000000E0L, */
{0x0cc994a760137543ULL, 0x40041d4c974b22bcULL}, // 3.566239794444800849656497338030115886153E1L,
{0xa5b186c10b6a065eULL, 0x4007aed5b7e20c37ULL}, // 4.308348370818927353321556740027020068897E2L,
{0x8711ebf202296129ULL, 0x400a37d5c6fdd0cdULL}, // 2.494680540950601626662048893678584497900E3L,
{0x02d59339ee4eee21ULL, 0x400bef892855649eULL}, // 7.928572347062145288093560392463784743935E3L,
{0xd9b903b0950fefb3ULL, 0x400cc7c8d1c45b09ULL}, // 1.458510242529987155225086911411015961174E4L,
{0x174d6e0dae833752ULL, 0x400ce38f8ba0a897ULL}, // 1.547394317752562611786521896296215170819E4L,
{0xfcbdd5dddcf7c68cULL, 0x400c1277f99a3d1aULL}, // 8.782996876218210302516194604424986107121E3L,
{0x7099e48f01631a53ULL, 0x400a0205bd172325ULL}, // 2.064179332321782129643673263598686441900E3L
};

/* tan( 3*pi/8 ) */
static float128_t T3P8 = {0x6484597d89b3754bULL, 0x40003504f333f9deULL};

/* tan( pi/8 ) */
static float128_t TP8 = {0x2422cbec4d9baa56ULL, 0x3ffda827999fcef3ULL};

static const float128_t zero = {0, 0};
static const float128_t one = {0, 0x3fff000000000000ULL};
__attribute__((unused)) static const float128_t f_2_p0 = {0x0000000000000000ULL, 0x4000000000000000ULL};
__attribute__((unused)) static const float128_t f_3_p0 = {0x0000000000000000ULL, 0x4000800000000000ULL};

float128_t cephes_f128_atanl(float128_t x)
{
struct softfloat_state state = {};
float128_t y, z;
short sign;

/* make argument positive and save the sign */
sign = 1;
if( f128_lt(&state, x, zero) )
	{
	sign = -1;
	x = f128_complement_sign(x);
	}

/* range reduction */
// if( x > T3P8 )

if( f128_lt(&state, T3P8, x) )
	{
	y = F128_PIO2L;
	x = f128_complement_sign( f128_div(&state, one, x));
	}

else if( f128_lt(&state, TP8, x) )
	{
	y = F128_PIO4L;
	x = f128_div(&state, f128_sub(&state, x, one), f128_add(&state, x, one));
	}
else
	y = zero;

/* rational form in x**2 */
z = f128_mul(&state, x, x);
y = f128_add(&state, f128_add(&state, y, f128_mul(&state, f128_mul(&state, f128_div(&state, cephes_f128_polevll( z, P, 8 ), cephes_f128_p1evll( z, Q, 8 ) ), z), x)), x);

if( sign < 0 )
	y = f128_complement_sign(y);

return(y);
}

/*							atan2	*/



#if ANSIC
float128_t cephes_f128_atan2l( float128_t y, float128_t x )
#else
float128_t cephes_f128_atan2l( float128_t x, float128_t y )
#endif
{
struct softfloat_state state = {};
float128_t z, w;
short code;


code = 0;
w = zero;

if( f128_lt(&state, x, zero) )
	code = 2;
if( f128_lt(&state, y, zero) )
	code |= 1;

if( f128_eq(&state, x, zero) )
	{
	if( code & 1 )
		{
#if ANSIC
		return( f128_complement_sign(F128_PIO2L) );
#else
		return( f128_mul(&state, f_3_p0, F128_PIO2L) );
#endif
		}
	if( f128_eq(&state, y, zero) )
		return zero;
	return( F128_PIO2L );
	}

if( f128_eq(&state, y, zero) )
	{
	if( code & 2 )
		return( F128_PIL );
	return zero;
	}


switch( code )
	{
#if ANSIC
	case 0:
	case 1: w = zero; break;
	case 2: w = F128_PIL; break;
	case 3: w = f128_complement_sign(F128_PIL); break;
#else
	case 0: w = zero; break;
	case 1: w = f128_mul(&state, f_2_p0, F128_PIL); break;
	case 2:
	case 3: w = F128_PIL; break;
#endif
	}

z = cephes_f128_atanl( f128_div(&state, y, x) );

return f128_add(&state, w, z );
}
