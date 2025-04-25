/*							cephes_f128_log2l.c
 *
 *	Base 2 logarithm, float128_t precision
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y, cephes_f128_log2l();
 *
 * y = cephes_f128_log2l( x );
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

#include "mconf.h"
static char fname[] = {"cephes_f128_log2l"};

/* Coefficients for ln(1+x) = x - x**2/2 + x**3 P(x)/Q(x)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 5.3e-37,
 * relative peak error spread = 2.3e-14
 */
static float128_t P[13] = {
	{0x95434922008560fcULL, 0x3feb9d04a0d6ed82ULL}, // 1.538612243596254322971797716843006400388E-6L
	{0x2e9cb5e91a8c2fa0ULL, 0x3ffdffd7e21347ccULL}, // 4.998469661968096229986658302195402690910E-1L
	{0x674c43ea62a592e7ULL, 0x400373615178fe96ULL}, // 2.321125933898420063925789532045674660756E1L
	{0xfa539715d5fd0560ULL, 0x40079b73a8639c28ULL}, // 4.114517881637811823002128927449878962058E2L
	{0x5ec5c60d38b7fa2aULL, 0x400ade1e79b3ae12ULL}, // 3.824952356185897735160588078446136783779E3L
	{0x6369f0cada64eeecULL, 0x400d4ca24f0550cfULL}, // 2.128857716871515081352991964243375186031E4L
	{0x115104b644c1f464ULL, 0x400f28a791822d40ULL}, // 7.594356839258970405033155585486712125861E4L
	{0x95ec43488121aff8ULL, 0x40105f196a49f171ULL}, // 1.797628303815655343403735250238293741397E5L
	{0xa2484b7171ab5034ULL, 0x401116caba9f2757ULL}, // 2.854829159639697837788887080758954924001E5L
	{0xe49b2bf8646a8a1eULL, 0x401125a72eb05ba7ULL}, // 3.007007295140399532324943111654767187848E5L
	{0x17ac5c737d1b8ad4ULL, 0x4010897ca319418dULL}, // 2.014652742082537582487669938141683759923E5L
	{0x9ff15925da76d408ULL, 0x400f2f8f8bfbf9a1ULL}, // 7.771154681358524243729929227226708890930E4L
	{0xe740b8544d79077cULL, 0x400c9a7dcad5d0efULL}, // 1.313572404063446165910279910527789794488E4L
};
static float128_t Q[12] = {
/* 1.000000000000000000000000000000000000000E0L, */
{0x4a2113daac8d7fa5ULL,0x40048322fbda4d3fULL}, // 4.839208193348159620282142911143429644326E1L,
{0x9efb2fe2c778f56fULL,0x4008c73f14777e56ULL}, // 9.104928120962988414618126155557301584078E2L,
{0xf23a98d434d3a705ULL,0x400c1dd933ea5565ULL}, // 9.147150349299596453976674231612674085381E3L,
{0x4b44059a3b76f461ULL,0x400eb5f4d77aed02ULL}, // 5.605842085972455027590989944010492125825E4L,
{0x2962234d48fff0bcULL,0x4010b71bb67f5effULL}, // 2.248234257620569139969141618556349415120E5L,
{0xe673c713bcf24ee3ULL,0x40122b6c5ddac3b8ULL}, // 6.132189329546557743179177159925690841200E5L,
{0x34d8d36e8de37c71ULL,0x40131ab83fa3b03bULL}, // 1.158019977462989115839826904108208787040E6L,
{0x061338bb0e95b314ULL,0x401371d8273f762aULL}, // 1.514882452993549494932585972882995548426E6L,
{0xe379b5d8e7071d74ULL,0x401348fbe89d38e2ULL}, // 1.347518538384329112529391120390701166528E6L,
{0x412eafafea233277ULL,0x40127bc5211688c1ULL}, // 7.777690340007566932935753241556479363645E5L,
{0x16378fd2514ba129ULL,0x40110088814003eaULL}, // 2.626900195321832660448791748036714883242E5L,
{0xed708a3f3a1ac5caULL,0x400e33de58205cb3ULL}, // 3.940717212190338497730839731583397586124E4L
};

/* Coefficients for log(x) = z + z^3 P(z^2)/Q(z^2),
 * where z = 2(x-1)/(x+1)
 * 1/sqrt(2) <= x < sqrt(2)
 * Theoretical peak relative error = 1.1e-35,
 * relative peak error spread 1.1e-9
 */
static float128_t R[6] = {
	{0x68479d54e4ced708ULL, 0xbffec40a1c874f5aULL}, // -8.828896441624934385266096344596648080902E-1L,
	{0x565b5611a30df628ULL, 0x40054247b533971eULL}, // 8.057002716646055371965756206836056074715E1L,
	{0xb690eddd457e03b0ULL, 0xc009fa1350a9210eULL}, // -2.024301798136027039250415126250455056397E3L,
	{0xea1230d4dc2a41c8ULL, 0x400d4020cbb3c4edULL}, // 2.048819892795278657810231591630928516206E4L,
	{0x388e5d3ae806c32aULL, 0xc00f5eac94780e23ULL}, // -8.977257995689735303686582344659576526998E4L,
	{0x6802a6fb3250b4fdULL, 0x401014fab5e2e8c1ULL}, // 1.418134209872192732479751274970992665513E5L
};
static float128_t S[6] = {
/* 1.000000000000000000000000000000000000000E0L, */
 {0x2575cd7cadd52c63ULL, 0xc005da8b34108b63ULL}, // -1.186359407982897997337150403816839480438E2L,
 {0x9022bf51e9d20aecULL, 0x400af3d0db24df08ULL}, // 3.998526750980007367835804959888064681098E3L,
 {0xeb27fc1032bb267dULL, 0xc00ec11ad77cc51cULL}, // -5.748542087379434595104154610899551484314E4L,
 {0xaeec5bd6a5211cbdULL, 0x401186c6f13df72eULL}, // 4.001557694070773974936904547424676279307E5L,
 {0xee9e91e4b3020178ULL, 0xc013455371e04bc5ULL}, // -1.332535117259762928288745111081235577029E6L,
 {0x1c03fa78cb791730ULL, 0x40139f7810d45d22ULL}, // 1.701761051846631278975701529965589676574E6L
};
/* log2(e) - 1 */
static const float128_t LOG2EA = {0x85ddf43ff68348eaULL, 0x3ffdc551d94ae0bfULL};

static const float128_t SQRTH = {0xc908b2fb1366ea95ULL,  0x3ffe6a09e667f3bcULL};
static const float128_t zero = {0, 0};
static const float128_t f_0_p5 = {0, 0x3ffe000000000000ULL};
static const float128_t one = {0, 0x3fff000000000000ULL};

static const float128_t indeterminate = {0x0000000000000000ULL, 0xc00d000000000000ULL};

float128_t cephes_f128_log2l(float128_t x) {
VOLATILE float128_t z;
float128_t y;
int e;

struct softfloat_state state = {};

/* Test for domain */
if( f128_le(&state, x, zero) )
	{
	if( f128_eq(&state, x, zero) )
		mtherr( fname, SING );
	else
		mtherr( fname, DOMAIN );
	return indeterminate;
	}

/* separate mantissa from exponent */

/* Note, frexp is used so that denormal numbers
 * will be handled properly.
 */
x = cephes_f128_frexpl( x, &e );


/* logarithm using log(x) = z + z**3 P(z)/Q(z),
 * where z = 2(x-1)/x+1)
 */
if( (e > 2) || (e < -2) )
{
if( f128_lt(&state, x, SQRTH) )
	{ /* 2( 2x-1 )/( 2x+1 ) */
	e -= 1;
	z = f128_sub(&state, x, f_0_p5);
	y = f128_add(&state, f128_mul(&state, f_0_p5, z), f_0_p5);
	}	
else
	{ /*  2 (x-1)/(x+1)   */
	z = f128_sub(&state, x, f_0_p5);
	z = f128_sub(&state, z, f_0_p5);
	y = f128_add(&state, f128_mul(&state, f_0_p5, x), f_0_p5);
	}
x = f128_div(&state, z, y);
z = f128_mul(&state, x, x);
y = f128_mul(&state, x,
    f128_div(&state, f128_mul(&state, z, cephes_f128_polevll( z, R, 5 )), cephes_f128_p1evll( z, S, 6 ) ));
goto done;
}


/* logarithm using log(1+x) = x - .5x**2 + x**3 P(x)/Q(x) */

if( f128_lt(&state, x, SQRTH) )
	{
	e -= 1;
	x = f128_sub(&state, cephes_f128_ldexpl( x, 1 ), one); /*  2x - 1  */
	}
else
	{
	x = f128_sub(&state, x, one);
	}
z = f128_mul(&state, x, x);
y = f128_mul(&state, x,
    f128_div(&state, f128_mul(&state, z, cephes_f128_polevll( x, P, 12 )), cephes_f128_p1evll( x, Q, 12 )));
y = f128_sub(&state, y, cephes_f128_ldexpl( z, -1 ));   /* -0.5x^2 + ... */

done:

/* Multiply log of fraction by log2(e)
 * and base 2 exponent by 1
 *
 * ***CAUTION***
 *
 * This sequence of operations is critical and it may
 * be horribly defeated by some compiler optimizers.
 */
z = f128_mul(&state, y, LOG2EA);
z = f128_add(&state, z, f128_mul(&state, x, LOG2EA));
z = f128_add(&state, z, y);
z = f128_add(&state, z, x);
z = f128_add(&state, z, i32_to_f128(e));
return( z );
}

