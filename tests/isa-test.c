#include "stdio.h"
typedef long long int lli;
int main()
{
	lli x = 0x123;
	lli b = 0 - x;
	x = b + x;
	if(x == 0)
		b = -b;
	x = 1 << 1;
	x *= b;
	x = x & b;
	lli div = b / x;
	lli rem = b % x;
	int i = 3;
	while(i > rem)
		i--;
	lli large1 = 1ll << 54, large2 = 1ll << 54;
	lli hi, lo;
	__asm__(
		"mul %0, %2, %3\n\t"
		"mulh %1, %2, %3"
		:"=r&"(lo),"=r"(hi)
		:"r"(large1),"r"(large2)
	);
	double da = 2.0, db = 1.2;
	printint((lli)(db*da));
	printchar('\n');
	printdouble(db * da);
	printchar('\n');
	printint(x);
	printchar('\n');
	printint(b);
	printchar('\n');
	printint(div);
	printchar('\n');
	printint(rem);
	printchar('\n');
	printint(large1);
	printchar('\n');
	printint(hi);
	printchar('\n');
	printint(lo);
	printchar('\n');
	printstring("hello world\n");
	halt();
}