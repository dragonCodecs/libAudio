// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
/*
 * Direct/Discrete fourier transform
 */
int DFT(int dir, int m, double *x1, double *y1)
{
	long i, k;
	double arg;
	double cosarg, sinarg;
	double *x2 = NULL, *y2 = NULL;

	x2 = (double *)malloc(m * sizeof(double));
	y2 = (double *)malloc(m * sizeof(double));
	if (x2 == NULL || y2 == NULL)
		return FALSE;

	for (i = 0; i < m; i++)
	{
		x2[i] = 0;
		y2[i] = 0;
		arg = - dir * 2.0 * 3.141592654 * (double)i / (double)m;
		for (k = 0; k < m; k++)
		{
			cosarg = cos(k * arg);
			sinarg = sin(k * arg);
			x2[i] += (x1[k] * cosarg - y1[k] * sinarg);
			y2[i] += (x1[k] * sinarg + y1[k] * cosarg);
		}
	}

	/* Copy the data back */
	if (dir == 1)
	{
		for (i = 0; i < m; i++)
		{
			x1[i] = x2[i] / (double)m;
			y1[i] = y2[i] / (double)m;
		}
	}
	else
	{
		for (i = 0; i < m; i++)
		{
			x1[i] = x2[i];
			y1[i] = y2[i];
		}
	}

	free(x2);
	free(y2);
	return TRUE;
}

/*
 * This computes an in-place complex-to-complex FFT 
 * x and y are the real and imaginary arrays of 2^m points.
 * dir = 1 gives forward transform
 * dir = -1 gives reverse transform 
 */
short FFT(short int dir, long m, double *x, double *y)
{
	long n, i, i1, j, k, i2, l, l1, l2;
	double c1, c2, tx, ty, t1, t2, u1, u2, z;

	/* Calculate the number of points */
	n = 1;
	for (i = 0; i < m; i++)
		n *= 2;

	/* Do the bit reversal */
	i2 = n >> 1;
	j = 0;
	for (i = 0; i < (n - 1); i++)
	{
		if (i < j)
		{
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}

		k = i2;
		while (k <= j)
		{
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	/* Compute the FFT */
	c1 = -1.0; 
	c2 = 0.0;
	l2 = 1;
	for (l = 0; l < m; l++)
	{
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0; 
		u2 = 0.0;

		for (j = 0; j < l1; j++)
		{
			for (i = j; i < n; i += l2)
			{
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1; 
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}

			z =	u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}

		c2 = sqrt((1.0 - c1) / 2.0);
		if (dir == 1)
		{
			c2 = -c2;
		}
		c1 = sqrt((1.0 + c1) / 2.0);
	}

	/* Scaling for forward transform */
	if (dir == 1)
	{
		for (i = 0; i < n; i++)
		{
			x[i] /= n;
			y[i] /= n;
		}
	}

	return TRUE;
}

double WFIR_Kaiser4Tap(int j, int n)
{
	double a = (2.0 * M_PI) / (n - 1);
	double w = 0.40243 - (0.49804 * cos(a * j)) + (0.09831 * cos(2 * a * j)) - (0.00122 * cos(3 * a * j));

	return w;
}
