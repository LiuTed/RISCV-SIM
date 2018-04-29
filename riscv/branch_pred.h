#ifndef BP_H
#define BP_H
#include "utli.h"
#include "stats.h"
extern Statistics *stats;
class BranchPredict
{
	int status;
public:
	BranchPredict();
	bool predict(ulli pc, lli offset);
	void feedback(ulli pc, lli offset, bool pred, bool succ);
};
#endif
