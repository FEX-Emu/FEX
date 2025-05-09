/*                                                      ceill()
 *                                                      floorl()
 *                                                      frexpl()
 *                                                      ldexpl()
 *                                                      fabsl()
 *							signbitl()
 *							isnanl()
 *							isfinitel()
 *
 *      Floating point numeric utilities
 *
 *
 *
 * SYNOPSIS:
 *
 * float128_t x, y;
 * float128_t ceill(), floorl(), frexpl(), ldexpl(), fabsl();
 * int signbitl(), isnanl(), isfinitel();
 * int expnt, n;
 *
 * y = floorl(x);
 * y = ceill(x);
 * y = frexpl( x, &expnt );
 * y = ldexpl( x, n );
 * y = fabsl( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * All four routines return a float128_t precision floating point
 * result.
 *
 * floorl() returns the largest integer less than or equal to x.
 * It truncates toward minus infinity.
 *
 * ceill() returns the smallest integer greater than or equal
 * to x.  It truncates toward plus infinity.
 *
 * frexpl() extracts the exponent from x.  It returns an integer
 * power of two to expnt and the significand between 0.5 and 1
 * to y.  Thus  x = y * 2**expn.
 *
 * ldexpl() multiplies x by 2**n.
 *
 * fabsl() returns the absolute value of its argument.
 *
 * signbitl(x) returns 1 if the sign bit of x is 1, else 0.
 *
 * These functions are part of the standard C run time library
 * for some but not all C compilers.  The ones supplied are
 * written in C for IEEE arithmetic.  They should
 * be used only if your compiler library does not already have
 * them.
 *
 * The IEEE versions assume that denormal numbers are implemented
 * in the arithmetic.  Some modifications will be required if
 * the arithmetic has abrupt rather than gradual underflow.
 */


/*
Cephes Math Library Release 2.2:  July, 1992
Copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

#include "mconf.h"
#define DENORMAL 1

#ifdef UNK
char *unkmsg = "ceill(), floorl(), frexpl(), ldexpl() must be rewritten!\n";
#undef UNK
#define MIEEE 1
#define EXPOFS 0
#endif

#ifdef IBMPC
#define NBITS 113
#define EXPOFS 7
#endif

#ifdef MIEEE
#define NBITS 113
#define EXPOFS 0
#endif

extern float128_t F128_MAXNUML;


static const float128_t zero = {0, 0};
static const float128_t f_0_p5 = {0, 0x3ffe000000000000ULL};
static const float128_t one = {0, 0x3fff000000000000ULL};
static const float128_t neg_one = {0, 0xbfff000000000000ULL};
static const float128_t f_2_p0 = {0, 0x4000000000000000ULL};

float128_t cephes_f128_fabsl(float128_t x)
{
struct softfloat_state state = {};

if( f128_lt(&state, x, zero) )
        return f128_sub(&state, zero, x );
else
        return( x );
}



float128_t cephes_f128_ceill(float128_t x)
{
float128_t y;

#ifdef UNK
mtherr( "ceill", DOMAIN );
return(0.0L);
#endif

struct softfloat_state state = {};
y = cephes_f128_floorl(x);
if( f128_lt(&state, y, x) )
        y = f128_add(&state, y, one);
return(y);
}




/* Bit clearing masks: */

static unsigned short bmask[] = {
0xffff,
0xfffe,
0xfffc,
0xfff8,
0xfff0,
0xffe0,
0xffc0,
0xff80,
0xff00,
0xfe00,
0xfc00,
0xf800,
0xf000,
0xe000,
0xc000,
0x8000,
0x0000,
};



float128_t cephes_f128_floorl(float128_t x)
{
union
  {
    float128_t y;
    unsigned short sh[8];
  } u;
int e, j;

#ifdef UNK
mtherr( "floor", DOMAIN );
return(0.0L);
#endif

struct softfloat_state state = {};
u.y = x;
/* find the exponent (power of 2) */
e = (u.sh[EXPOFS] & 0x7fff) - 0x3fff;

if( e < 0 )
        {
        if( f128_lt(&state, u.y, zero) )
                return neg_one;
        else
                return zero;
        }

#ifdef IBMPC
j = 0;
#endif

#ifdef MIEEE
j = 7;
#endif

e = (NBITS - 1) - e;
/* clean out 16 bits at a time */
while( e >= 16 )
        {
#ifdef IBMPC
        u.sh[j++] = 0;
#endif

#ifdef MIEEE
        u.sh[j--] = 0;
#endif
        e -= 16;
        }

/* clear the remaining bits */
if( e > 0 )
        u.sh[j] &= bmask[e];

if( f128_lt(&state, x, zero) && !f128_eq(&state, u.y, x) )
        u.y = f128_sub(&state, u.y, one);;

return(u.y);
}



float128_t cephes_f128_frexpl( float128_t x, int *pw2 )
{
union
  {
    float128_t y;
    unsigned short sh[8];
  } u;
int i, k;

struct softfloat_state state = {};
u.y = x;

#ifdef UNK
mtherr( "frexp", DOMAIN );
return(0.0L);
#endif

/* find the exponent (power of 2) */
i  = u.sh[EXPOFS] & 0x7fff;

if( i == 0 )
        {
        if( f128_eq(&state, u.y, zero))
                {
                *pw2 = 0;
                return zero;
                }
/* Number is denormal or zero */
#if DENORMAL
/* Handle denormal number. */
do
        {
        u.y = f128_mul(&state, u.y, f_2_p0);
        i -= 1;
        k  = u.sh[EXPOFS] & 0x7fff;
        }
while( (k == 0) && (i > -115) );
i = i + k;
#else
        *pw2 = 0;
        return(0.0L);
#endif /* DENORMAL */
        }

*pw2 = i - 0x3ffe;
u.sh[EXPOFS] = 0x3ffe;
return( u.y );
}


float128_t cephes_f128_ldexpl( float128_t x, int pw2 )
{
union
  {
    float128_t y;
    unsigned short sh[8];
  } u;
long e;

#ifdef UNK
mtherr( "ldexp", DOMAIN );
return zero;
#endif

struct softfloat_state state = {};
u.y = x;
while( (e = (u.sh[EXPOFS] & 0x7fffL)) == 0 )
        {
#if DENORMAL
        if( f128_eq(&state, u.y, zero))
                {
                return zero;
                }
/* Input is denormal. */
        if( pw2 > 0 )
                {
                u.y = f128_mul(&state, u.y, f_2_p0);
                pw2 -= 1;
                }
        if( pw2 < 0 )
                {
                if( pw2 < -113 )
                        return zero;
                u.y = f128_sub(&state, u.y, f_0_p5);
                pw2 += 1;
                }
        if( pw2 == 0 )
                return(u.y);
#else
        return zero;
#endif
        }

e = e + pw2;

/* Handle overflow */
if( e > 0x7ffeL )
        {
          e = u.sh[EXPOFS];
          u.y = zero;
          u.sh[EXPOFS] = e | 0x7fff;
          return( u.y );
        }
u.sh[EXPOFS] &= 0x8000;
/* Handle denormalized results */
if( e < 1 )
        {
#if DENORMAL
        if( e < -113 )
                return zero;
        u.sh[EXPOFS] |= 1;
        while( e < 1 )
                {
                u.y = f128_sub(&state, u.y, f_0_p5);
                e += 1;
                }
        e = 0;
#else
        return zero;
#endif
        }

u.sh[EXPOFS] |= e & 0x7fff;
return(u.y);
}

/* Return 1 if x is a number that is Not a Number, else return 0.  */

int cephes_f128_isnanl(float128_t x)
{
#ifdef NANS
union
	{
	float128_t d;
	unsigned short s[8];
	unsigned int i[4];
	} u;

u.d = x;

if( sizeof(int) == 4 )
	{
#ifdef IBMPC	    
	if( ((u.s[7] & 0x7fff) == 0x7fff)
	    && ((u.i[3] & 0x7fff) | u.i[2] | u.i[1] | u.i[0]))
		return 1;
#endif
#ifdef MIEEE
	if( ((u.i[0] & 0x7fff0000) == 0x7fff0000)
	    && ((u.i[0] & 0x7fff) | u.i[1] | u.i[2] | u.i[3]))
		return 1;
#endif
	return(0);
	}
else
	{ /* size int not 4 */
#ifdef IBMPC
	if( (u.s[7] & 0x7fff) == 0x7fff)
		{
		if((u.s[6] & 0x7fff) | u.s[5] | u.s[4] | u.s[3] | u.s[2] | u.s[1] | u.s[0])
			return(1);
		}
#endif
#ifdef MIEEE
	if( (u.s[0] & 0x7fff) == 0x7fff)
		{
		if((u.s[1] & 0x7fff) | (u.s[2] & 0x7fff) | u.s[3] | u.s[4] | u.s[5] | u.s[6] | u.s[7])
			return(1);
		}
#endif
	return(0);
	} /* size int not 4 */

#else
/* No NANS.  */
return(0);
#endif
}


/* Return 1 if x is not infinite and is not a NaN.  */

int cephes_f128_isfinitel(float128_t x)
{
#ifdef INFINITIES
union
	{
	float128_t d;
	unsigned short s[8];
	unsigned int i[4];
	} u;

u.d = x;

if( sizeof(int) == 4 )
	{
#ifdef IBMPC
	if( (u.s[7] & 0x7fff) != 0x7fff)
		return 1;
#endif
#ifdef MIEEE
	if( (u.i[0] & 0x7fff0000) != 0x7fff0000)
		return 1;
#endif
	return(0);
	}
else
	{
#ifdef IBMPC
	if( (u.s[7] & 0x7fff) != 0x7fff)
		return 1;
#endif
#ifdef MIEEE
	if( (u.s[0] & 0x7fff) != 0x7fff)
		return 1;
#endif
	return(0);
	}
#else
/* No INFINITY.  */
return(1);
#endif
}


/* Return 1 if the sign bit of x is 1, else 0.  */

int cephes_f128_signbitl(float128_t x)
{
union
	{
	float128_t d;
	short s[8];
	int i[4];
	} u;

u.d = x;

if( sizeof(int) == 4 )
	{
#ifdef IBMPC
	return( u.s[7] < 0 );
#endif
#ifdef DEC
error no such DEC format
#endif
#ifdef MIEEE
	return( u.i[0] < 0 );
#endif
	}
else
	{
#ifdef IBMPC
	return( u.s[7] < 0 );
#endif
#ifdef DEC
error no such DEC format
#endif
#ifdef MIEEE
	return( u.s[0] < 0 );
#endif
	}
}
