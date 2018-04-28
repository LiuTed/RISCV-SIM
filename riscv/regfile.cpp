#include "regfile.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static const char* reg_name[num_registers] = {
	"zero", "ra", "sp", "gp",
	"tp", "t0", "t1", "t2",
	"s0/fp", "s1", "a0", "a1",
	"a2", "a3", "a4", "a5",
	"a6", "a7", "s2", "s3",
	"s4", "s5", "s6", "s7",
	"s8", "s9", "s10", "s11",
	"t3", "t4", "t5", "t6",

	"ft0", "ft1", "ft2", "ft3",
	"ft4", "ft5", "ft6", "ft7",
	"fs0", "fs1", "fa0", "fa1",
	"fa2", "fa3", "fa4", "fa5",
	"fa6", "fa7", "fs2", "fs3",
	"fs4", "fs5", "fs6", "fs7",
	"fs8", "fs9", "fs10", "fs11",
	"ft8", "ft9", "ft10", "ft11",

	"pc", "end pc", "NA", "NA",
	"NA", "NA", "NA", "NA"
};

RegFile::RegFile(int n)
{
	regs = new long long int[n];
	n_reg = n;
	memset(regs, 0, sizeof(long long int) * n);
}
RegFile::~RegFile()
{
	delete[] regs;
}
long long int RegFile::get(int n)
{
	assert(n < n_reg);
	if(n < num_usr_regs)
		stats->summary("RegFile: reg get");
	return regs[n];
}
void RegFile::set(int n, long long int val)
{
	assert(n < n_reg);
	if(n < num_usr_regs)
		stats->summary("RegFile: reg set");
	regs[n] = val;
}
void RegFile::print(FILE* out)
{
	for(int i = 0; i < num_int_regs; i++)
	{
		fprintf(out, "%7s(%-2d) %16llx %lld\n", reg_name[i], i, regs[i], regs[i]);
	}
	for(int i = num_int_regs; i < num_usr_regs; i++)
	{
		fprintf(out, "%7s(%-2d) %16llx %lf %f\n", reg_name[i], i, regs[i], treat_as<double>(regs[i]), treat_as<float>((int)regs[i]));
	}
	for(int i = num_usr_regs; i < n_reg; i++)
	{
		fprintf(out, "%7s(%-2d) %16llx %lld\n", reg_name[i], i, regs[i], regs[i]);
	}
}