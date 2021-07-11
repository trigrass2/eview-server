#include <ace/Task.h>
#include <string>
#include <map>
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "pklog/pklog.h"
#include "common/pkcomm.h"

CRedisProxy		g_redisRW; // 订阅

//void testHMSet_Pipe();
//void testHSet_Pipe();
//void testHMSet();
//void testHSet();
void testSet();
void testMSet();
int main(int argc, char* argv[])
{
	// 初始记为正常启动
	int nRet = PK_SUCCESS;
	nRet = g_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize RW failed, ret:%d", nRet);
	}

	//testHMSet_Pipe();
	//testHMSet();
	//testHSet_Pipe();
	//testHSet();

	testSet();
	testMSet();
	getchar();

	return 0;
}

char szSetValue[128] = "{\"value\":\"1234\",\"time\":\"2016-12-20 11:11:11\",\"quality\":\"0\",\"msg\":\"\",\"enablealarm\":\"1\"}";
time_t tmBegin,tmEnd;
ACE_Time_Value tvBegin, tvEnd;
char szName[32];
int nTagNum = 50000;
int nCycle = 4;
string strEnableAlarm = "1";
string strQuality = "0";
string strMsg = "";
string strValue = "1234";
string strTime = "2016-12-20 11:11:11.111";

// mac pro 笔记本，2014版
// 每秒可以进行10857次。用全新的tag点和 用老的tag点差不多
void testSet()
{
	tvBegin = ACE_OS::gettimeofday();
	//time(&tmBegin);
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);
			g_redisRW.set(szName, szSetValue);
		}
	}

	//time(&tmEnd);
	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testSet, %d writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}

// mac pro 笔记本，2014版
// 每批次50、100、200个点，每秒均可以写入64164个点！
void testMSet()
{
	tvBegin = ACE_OS::gettimeofday();

	vector<string> vecKeys, vecValues;
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		// 每批次50个
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);

			vecKeys.push_back(szName);
			vecValues.push_back(szSetValue);
			if(vecKeys.size() >= 200)
			{
				g_redisRW.mset(vecKeys, vecValues);
				vecKeys.clear();
				vecValues.clear();
			}
		}
	}

	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testMSet, %d writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}


// mac pro 笔记本，2014版
// 每批次50、100、200个点，每秒均可以写入66666个点！
void testHSet()
{
	tvBegin = ACE_OS::gettimeofday();

	vector<string> vecKeys, vecValues;
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		// 每批次50个
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);

			g_redisRW.hset(szName, "value", "1234");
			g_redisRW.hset(szName, "time", "2016-12-20 11:11:11.111");
			g_redisRW.hset(szName, "quality", "0");
			g_redisRW.hset(szName, "msg", "");
			g_redisRW.hset(szName, "enablealarm", "");
		}
	}

	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testHSet, %d * 5 writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}

// 每批次50、100、200个点，每秒均可以写入66666个点！
void testHSet_Pipe()
{
	tvBegin = ACE_OS::gettimeofday();
	int nPipeNum = 0;
	//vector<string> vecCmds;
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		// 每批次50个
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);
			//char szTmp[1024];
			//sprintf(szTmp, "hset \"%s\" %s %s", szName, "value", "1234");
			//vecCmds.push_back(szTmp);
			//sprintf(szTmp, "hset \"%s\" %s \"%s\"", szName, "time", "2016-12-20 11:11:11.111");
			//vecCmds.push_back(szTmp);
			//sprintf(szTmp, "hset \"%s\" %s %s", szName, "quality", "0");
			//vecCmds.push_back(szTmp);
			//sprintf(szTmp, "hset \"%s\" %s %s", szName, "msg", "\"\"");
			//vecCmds.push_back(szTmp);
			//sprintf(szTmp, "hset \"%s\" %s %s", szName, "enablealarm", "1");
			//vecCmds.push_back(szTmp);

			g_redisRW.pipe_append("hset %s %s %s", szName, strlen(szName), "value", strlen("value"), "1234", strlen("1234"));
			g_redisRW.pipe_append("hset %s %s %s", szName, strlen(szName), "time", strlen("time"), "2016-12-20 11:11:11.111", strlen("2016-12-20 11:11:11.111"));
			g_redisRW.pipe_append("hset %s %s %s", szName, strlen(szName), "quality", strlen("quality"), "0", strlen("0"));
			g_redisRW.pipe_append("hset %s %s %s", szName, strlen(szName), "msg", strlen("msg"), "", strlen(""));
			g_redisRW.pipe_append("hset %s %s %s", szName, strlen(szName), "enablealarm",strlen("enablealarm"),  "1", strlen("1"));
			nPipeNum += 5;
		}

		if(nPipeNum >= 250)
		{
			vector<string> vecResult;
			g_redisRW.pipe_getReply(nPipeNum, vecResult);
		}
		/*if(vecCmds.size() >= 100)
		{
			g_redisRW.redisCommon(vecCmds);
			vecCmds.clear();
		}*/
	}

	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testHSet_Pipe, %d * 5 writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}

// mac pro 笔记本，2014版
// 每批次50、100、200个点，每秒均可以写入66666个点！
void testHMSet()
{
	tvBegin = ACE_OS::gettimeofday();
	vector<string> vecKeys, vecValues;
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		// 每批次50个
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);
			g_redisRW.redisCommon("hmset %s value %s time %s quality %s msg %s enablealarm %s", 
				szName, strlen(szName), strValue.c_str(), strValue.length(), strTime.c_str(), strTime.length(), 
				strQuality.c_str(), strQuality.length(), strMsg.c_str(), strMsg.length(), strEnableAlarm.c_str(), strEnableAlarm.length());
		}
	}

	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testHMSet, %d * 5 writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}

// mac pro 笔记本，2014版
// 每批次50、100、200个点，每秒均可以写入66666个点！
void testHMSet_Pipe()
{
	tvBegin = ACE_OS::gettimeofday();
	int nBatchNum = 0;
	vector<string> vecKeys, vecValues;
	for(int iCycle = 0; iCycle < nCycle; iCycle ++)
	{
		// 每批次50个
		for(int i = 0; i < nTagNum; i ++)
		{
			sprintf(szName, "bus1con1.communication%d", i+1);
			g_redisRW.pipe_append("hmset %s value %s time %s quality %s msg %s enablealarm %s", 
				szName, strlen(szName), strValue.c_str(), strValue.length(), strTime.c_str(), strTime.length(), 
				strQuality.c_str(), strQuality.length(), strMsg.c_str(), strMsg.length(), strEnableAlarm.c_str(), strEnableAlarm.length());
			nBatchNum += 1;
		}

		if(nBatchNum >= 500)
		{
			vector<string> vecResult;
			g_redisRW.pipe_getReply(nBatchNum, vecResult);
		}
	}

	tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	int nMSec = tvTimeSpan.msec();
	printf("testHMSet_Pipe, %d * 5 writes, time:%d MS, %d次每秒\n", nTimes, nMSec, nTimes * 1000/nMSec);
}