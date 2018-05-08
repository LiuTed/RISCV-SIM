long long int Ackermann(int m, long long int n)
{
	if(m == 0)
		return n + 1;
	if(m > 0 && n == 0)
		return Ackermann(m - 1, 1);
	if(m > 0 && n > 0)
		return Ackermann(m - 1, Ackermann(m, n - 1));
}

int main()
{
	long long int n1 = Ackermann(3,5);
	long long int n2 = Ackermann(3,7);
}
