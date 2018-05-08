void mat_mul(const double *a, const double *b, double* c, int m, int n, int p)
{
	for(int i = 0; i < m; i++)
		for(int j = 0; j < p; j++)
		{
			double sum = 0;
			for(int k = 0; k < n; k++)
				sum += a[i*n+k] * b[k*p+j];
			c[i*p+j] = sum;
		}
	return;
}

double a[100*100], b[100*100], c[100*100];


int main()
{
	for(int i = 0; i < 100; i++)
		for(int j = 0; j < 100; j++)
		{
			a[i*100+j] = i * 2.0 / (j + 1);
			b[i*100+j] = i + 1.0 / (j + 1);
		}
	mat_mul(a, b, c, 100, 100, 100);
}
