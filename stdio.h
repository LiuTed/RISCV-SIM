void printchar(char c)
{
	__asm__(
		"li a0, 0\n\t"
		"mv a1, %0\n\t"
		"ecall"
		:
		:"r"(c)
		:"a0","a1"
	);
}
void printint(long long int i)
{
	__asm__(
		"li a0, 1\n\t"
		"mv a1, %0\n\t"
		"ecall"
		:
		:"r"(i)
		:"a0","a1"
	);
}
void printuint(unsigned long long int i)
{
	__asm__(
		"li a0, 2\n\t"
		"mv a1, %0\n\t"
		"ecall"
		:
		:"r"(i)
		:"a0","a1"
	);
}
void printdouble(double i)
{
	__asm__(
		"li a0, 4\n\t"
		"mv a1, %0\n\t"
		"ecall"
		:
		:"r"(i)
		:"a0","fa1"
	);
}
void printstring(const char* c)
{
	__asm__(
		"li a0, 3\n\t"
		"mv a1, %0\n\t"
		"ecall"
		:
		:"r"(c)
		:"a0","a1"
	);
}
void halt()
{
	__asm__(
		"li a0, 10\n\t"
		"ecall"
		:
		:
		:"a0"
	);
}
void ebreak()
{
	__asm__(
		"ebreak":::
	);
}

