#ifndef __FFT_H__
#define __FFT_H__ 1

/*
 * Code originally written by Pjotr in 1987.
 * Revamped and combined into 2 files by Rachel Mant in 2010.
 * --
 * This was originally written in the oldest style of C you can find and was written for Unix.
 */

typedef struct {
		double re, im;
	} COMPLEX;

// Only define __FFT_C__ if you really need these macro'ed operations.
#ifdef __FFT_C__
#define		c_re(c)		((c).re)
#define		c_im(c)		((c).im)

/*
 * C_add_mul adds product of c1 and c2 to c.
 */
#define	c_add_mul(c, c1, c2)	{ COMPLEX C1, C2; C1 = (c1); C2 = (c2); \
				  c_re (c) += C1.re * C2.re - C1.im * C2.im; \
				  c_im (c) += C1.re * C2.im + C1.im * C2.re; }

/*
 * C_conj substitutes c by its complex conjugate.
 */
#define c_conj(c)		{ c_im (c) = -c_im (c); }

/*
 * C_realdiv divides complex c by real.
 */
#define	c_realdiv(c, real)	{ c_re (c) /= (real); c_im (c) /= (real); }
#endif

#ifdef __cplusplus
extern "C" {
#endif

int fft(COMPLEX *in, unsigned n, COMPLEX *out);
int rft(COMPLEX *in, unsigned n, COMPLEX *out);

#ifdef __cplusplus
}
#endif

#endif /* __FFT_H__ */
