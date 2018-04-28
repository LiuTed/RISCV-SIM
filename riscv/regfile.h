#ifndef REGFILE_H
#define REGFILE_H
#include "stats.h"
#include "utli.h"
#include <cassert>
#include <cstdio>
extern Statistics* stats;
class RegFile
{
	long long int *regs;
	int n_reg;
public:
	RegFile(int n);
	~RegFile();
	long long int get(int n);
	void set(int n, long long int val);
	void print(FILE* = stdout);
};
#endif