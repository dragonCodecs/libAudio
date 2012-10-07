/*
 * Code originally written by Pjotr in 1987.
 * Revamped and combined into 2 files by Rachel Mant in 2010.
 * --
 * This was originally written in the oldest style of C you can find and was written for Unix.
 */

#define __FFT_C__ 1
#include "FFT.h"
#include <math.h>
#include <malloc.h>

COMPLEX *W_factors = 0;		/* array of W-factors */
unsigned Nfactors = 0;		/* number of entries in W-factors */

#define pi 3.1415926535897932384626434

static void split(COMPLEX *in, register unsigned r, register unsigned m, COMPLEX *out);

/*
 * W gives the (already computed) Wn ^ k (= e ^ (2pi * i * k / n)).
 * Notice that the powerseries of Wn has period Nfactors.
 */
#define	W(n, k)		(W_factors [((k) * (Nfactors / (n))) % Nfactors])

/*
 * W_init puts Wn ^ k (= e ^ (2pi * i * k / n)) in W_factors [k], 0 <= k < n.
 * If n is equal to Nfactors then nothing is done, so the same W_factors
 * array can used for several transforms of the same number of samples.
 * Notice the explicit calculation of sines and cosines, an iterative approach
 * introduces substantial errors.
 */
int W_init(unsigned n)
{
	unsigned k;

	if (n == Nfactors)
		return 0;
	if (Nfactors != 0 && W_factors != 0)
		free ((char *) W_factors);
	if ((Nfactors = n) == 0)
		return 0;
	if ((W_factors = (COMPLEX *) malloc (n * sizeof (COMPLEX))) == 0)
		return -1;

	for (k = 0; k < n; k++) {
		c_re (W_factors [k]) = cos (2 * pi * k / n);
		c_im (W_factors [k]) = sin (2 * pi * k / n);
	}

	return 0;
}

/*
 * Sum the n / m parts of each m samples of in to n samples in out.
 * 		   r - 1
 * Out [j] becomes  sum  in [j % m] * W (j * k).  Here in is the k-th
 * 		   k = 0   k	       n		 k
 * part of in (indices k * m ... (k + 1) * m - 1), and r is the radix.
 * For k = 0, a complex multiplication with W (0) is avoided.
 */
static void join(COMPLEX *in, register unsigned m, register unsigned n, COMPLEX *out)
{
	register unsigned i, j, jk, s;

	for (s = 0; s < m; s++)
		for (j = s; j < n; j += m) {
			out [j] = in [s];
			for (i = s + m, jk = j; i < n; i += m, jk += j)
				c_add_mul (out [j], in [i], W (n, jk));
		}
}

/*
 * Give smallest possible radix for n samples.
 * Determines (in a rude way) the smallest primefactor of n.
 */
static unsigned radix(unsigned n)
{
	unsigned r;

	if (n < 2)
		return 1;

	for (r = 2; r < n; r++)
		if (n % r == 0)
			break;
	return r;
}

/*
 * Recursive (reverse) complex fast Fourier transform on the n
 * complex samples of array in, with the Cooley-Tukey method.
 * The result is placed in out.  The number of samples, n, is arbitrary.
 * The algorithm costs O (n * (r1 + .. + rk)), where k is the number
 * of factors in the prime-decomposition of n (also the maximum
 * depth of the recursion), and ri is the i-th primefactor.
 */
void Fourier(COMPLEX *in, unsigned n, COMPLEX *out)
{
	unsigned r;

	if ((r = radix (n)) < n)
		split (in, r, n / r, out);
	join (in, n / r, n, out);
}

/*
 * Split array in of r * m samples in r parts of each m samples,
 * such that in [i] goes to out [(i % r) * m + (i / r)].
 * Then call for each part of out Fourier, so the r recursively
 * transformed parts will go back to in.
 */
static void split(COMPLEX *in, register unsigned r, register unsigned m, COMPLEX *out)
{
	register unsigned k, s, i, j;

	for (k = 0, j = 0; k < r; k++)
		for (s = 0, i = k; s < m; s++, i += r, j++)
			out [j] = in [i];

	for (k = 0; k < r; k++, out += m, in += m)
		Fourier (out, m, in);
}

/*
 * Forward Fast Fourier Transform on the n samples of complex array in.
 * The result is placed in out.  The number of samples, n, is arbitrary.
 * The W-factors are calculated in advance.
 */
int fft(COMPLEX *in, unsigned n, COMPLEX *out)
{
	unsigned i;

	for (i = 0; i < n; i++)
		c_conj (in [i]);
	
	if (W_init (n) == -1)
		return -1;

	Fourier (in, n, out);

	for (i = 0; i < n; i++) {
		c_conj (out [i]);
		c_realdiv (out [i], n);
	}

	return 0;
}

/*
 * Reverse Fast Fourier Transform on the n complex samples of array in.
 * The result is placed in out.  The number of samples, n, is arbitrary.
 * The W-factors are calculated in advance.
 */
int rft(COMPLEX *in, unsigned n, COMPLEX *out)
{
	if (W_init (n) == -1)
		return -1;

	Fourier (in, n, out);

	return 0;
}