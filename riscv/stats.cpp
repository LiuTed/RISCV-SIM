#include "stats.h"
using namespace std;
Statistics::Statistics():cnt(){}

void Statistics::summary(string key, double val)
{
	cnt[key] += val;
}

void Statistics::print(FILE* out)
{
	fprintf(out, "\n");
	for(const auto& i : cnt)
	{
		fprintf(out, "%s: %lf\n", i.first.c_str(), i.second);
	}
}

double Statistics::get(string key)
{
	auto res = cnt.find(key);
	if(res != cnt.end())
		return res->second;
	else
		return 0;
}

void Statistics::propagate()
{
	summary("BranchPredict: fault prediction rate",
		get("BranchPredict: fault predict") / (get("BranchPredict: fault predict") + get("BranchPredict: correct predict")));

	summary("Cache: L1D cache read miss rate",
		get("Cache: L1D cache read miss") / get("Cache: L1D cache fine-grained read"));
	summary("Cache: L1D cache write miss rate",
		get("Cache: L1D cache write miss") / get("Cache: L1D cache fine-grained write"));

	summary("Cache: L1I cache read miss rate",
		get("Cache: L1I cache read miss") / get("Cache: L1I cache fine-grained read"));

	summary("Cache: L2 cache read miss rate",
		get("Cache: L2 cache read miss") / get("Cache: L2 cache fine-grained read"));
	summary("Cache: L2 cache write miss rate",
		get("Cache: L2 cache write miss") / get("Cache: L2 cache fine-grained write"));

	summary("Cache: L3 cache read miss rate",
		get("Cache: L3 cache read miss") / get("Cache: L3 cache fine-grained read"));
	summary("Cache: L3 cache write miss rate",
		get("Cache: L3 cache write miss") / get("Cache: L3 cache fine-grained write"));

	summary("Memory: TLB miss rate",
		get("Memory: TLB miss") / get("Memory: TLB access"));

	double latency = //+ n * (latency - 1)
		  get("OP: division") * 14//3-30, 15
		+ get("OP: multiplication") * 2//3

		+ get("OP: double precision floating add/sub") * 2//3
		+ get("OP: double precision floating mul") * 4//5
		+ get("OP: double precision floating div") * 8//3-15, 9
		+ get("OP: double precision floating mul-add") * 4//5

		+ get("OP: single precision floating add/sub") * 2//3
		+ get("OP: single precision floating mul") * 4//5
		+ get("OP: single precision floating div") * 8//3-15, 9
		+ get("OP: single precision floating mul-add") * 4//5

		+ get("OP: integer-floating convert")//2
		+ get("OP: floating-integer convert")//2
		+ get("OP: floating-floating convert")//2

		+ get("Machine: syscall") * 9//11

		+ get("Cache: L1D cache fine-grained read") * 2//3
		+ get("Cache: L1D cache fine-grained write") * 2//3
		+ get("Cache: L1I cache fine-grained read") * 2//3
		+ get("Cache: L2 cache fine-grained read") * 11//12
		+ get("Cache: L2 cache fine-grained write") * 11//12
		+ get("Cache: L3 cache fine-grained read") * 39//40
		+ get("Cache: L3 cache fine-grained write") * 39//40
		+ get("Cache: L3 cahce read miss") * 249//250
		+ get("Cache: L3 cache write miss") * 249//250

		+ get("Memory: TLB hit") * 1
		+ get("Memory: TLB miss") * 500
		;

	summary("Machine: fine cycle",
		  get("Machine: coarse cycle") + latency);

	summary("Machine: throughput",
		get("Machine: total instr execute") / get("Machine: fine cycle"));
	summary("Machine: CPI",
		get("Machine: fine cycle") / get("Machine: Nnop instr execute"));
	summary("Machine: speedup ratio",
		(5 * get("Machine: total instr execute") + latency)
		/ get("Machine: fine cycle"));
}
