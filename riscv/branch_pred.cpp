#include "branch_pred.h"
#include "utli.h"
BranchPredict::BranchPredict():status(3){}
bool BranchPredict::predict(ulli pc, lli offset)
{
	return status > 0;
}
void BranchPredict::feedback(ulli pc, lli offset, bool pred, bool succ)
{
	if(succ)
	{
		stats->summary("BranchPredict: correct predict");
	}
	else
	{
		stats->summary("BranchPredict: fault predict");
	}
	if(pred ^ succ)
		status --;//did not jump
	else
		status ++;//did jump
	status = (status > 3 ? 3 : status);
	status = (status < 0 ? 0 : status);
}
