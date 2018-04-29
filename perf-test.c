#include "stdio.h"

void qs(int* arr, int s, int e)
{
	if(s >= e-1) return;
	int key = arr[s];
	int i = s, j = e-1;
	while(i < j)
	{
		while(i < j && arr[j] >= key)
			j--;
		arr[i] = arr[j];
		while(i < j && arr[i] <= key)
			i++;
		arr[j] = arr[i];
	}
	arr[i] = key;
	qs(arr, s, i);
	qs(arr, i+1, e);
	return;
}

void mat_mul(const double *a, const double *b, double* c, int m, int n, int p)
{
	//ebreak();
	for(int i = 0; i < m; i++)
		for(int j = 0; j < p; j++)
		{
			double sum = 0;
			for(int k = 0; k < n; k++)
				sum += a[i*n+k] * b[k*p+j];
			c[i*p+j] = sum;
		}
	//ebreak();
	return;
}

long long int Ackermann(int m, long long int n)
{
	if(m == 0)
		return n + 1;
	if(m > 0 && n == 0)
		return Ackermann(m - 1, 1);
	if(m > 0 && n > 0)
		return Ackermann(m - 1, Ackermann(m, n - 1));
}

int arr[1000];
double a[100*100], b[100*100], c[100*100];

int main()
{
	for(int i = 0; i < 1000; i++)
		arr[i] = (541 * i) % 1000;//6133 is the 800th prime number
	for(int i = 0; i < 100; i++)
		for(int j = 0; j < 100; j++)
		{
			a[i*100+j] = i * 2.0 / (j + 1);
			b[i*100+j] = i + 1.0 / (j + 1);
		}
	printstring("test start\n");
	qs(arr, 0, 10000);
	printstring("quick sort done\n");
	mat_mul(a, b, c, 100, 100, 100);
	printstring("matrix multiplication done\n");
	//ebreak();
	long long int n1 = Ackermann(3,5);
	long long int n2 = Ackermann(3,7);
	printstring("Ackermann done\n");
	//ebreak();
	for(int i = 0; i < 1000; i++)
	{
		printint(arr[i]);
		printchar(' ');
	}
	printchar('\n');
	for(int i = 0; i < 100; i++)
		for(int j = 0; j < 100; j++)
		{
			printdouble(c[i*100+j]);
			printchar(' ');
		}
	printchar('\n');
	printint(n1);
	printchar(' ');
	printint(n2);
	printchar('\n');
}