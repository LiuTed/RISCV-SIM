#include "branch_pred.h"
#include "utli.h"
BranchPredict::BranchPredict():status(3){}
bool BranchPredict::predict(ulli pc, lli offset)
{
	return status > 0;
}
void BranchPredict::feedback(ulli pc, lli offset, bool succ)
{
	if(succ)
	{
		stats->summary("BranchPredict: correct predict");
		status++;
	}
	else
	{
		stats->summary("BranchPredict: fault predict");
		status--;
	}
	status = (status > 3 ? 3 : status);
	status = (status < 0 ? 0 : status);
}
