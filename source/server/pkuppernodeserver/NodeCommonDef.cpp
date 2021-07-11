#include "common/gettimeofday.h"
#include "NodeCommonDef.h"
#include "eviewcomm/eviewcomm.h"
#include "RedisFieldDefine.h"
#include "MainTask.h"
#include "common/eviewdefine.h"
#include "mqttImpl.h"

void inttostring(int nValue, string &strValue)
{
	char szValue[128] = { 0 };
	memset(szValue, 0, sizeof(szValue));

	if (nValue == INT_MIN)
		sprintf(szValue, "INT_MIN");
	else if (nValue == INT_MAX)
		sprintf(szValue, "INT_MAX");
	else
		sprintf(szValue, "%d", nValue);
	strValue = szValue;
}

void realtostring(double dbValue, string &strValue)
{
	char szValue[128] = { 0 };

	int nMaxVal = dbValue - DBL_MAX;
	int nMinVal = dbValue + DBL_MAX;
	//if (std::abs(nMinVal) < 100)
	//	strcpy(szValue, "DBL_MIN");
	//else if (std::abs(nMaxVal) < 100)
	//	strcpy(szValue, "DBL_MAX");
	//else
	sprintf(szValue, "%f", dbValue);
	strValue = szValue;
}
   

 

