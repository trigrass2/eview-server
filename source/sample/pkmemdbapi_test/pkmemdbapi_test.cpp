#include "pkmemdbapi/pkmemdbapi.h"
#include <iostream>
#include <string>
#include <map>
#include "time.h"
#include "string.h"
#ifdef _WIN32  // Sleep declaration
#include <windows.h>
#else
#include "unistd.h"
#endif
#include "pkcomm/pkcomm.h"
using namespace std;

//#ifdef _WIN32
#pragma comment(lib,"ws2_32")
#pragma comment(lib,"pkmemdbapi")
#pragma comment(lib,"pkcomm")
//#endif

void testSet_Sync();
void testGet_Sync();
void testMSet();
void testHSet();
void testHMSet();
void testMGet_Sync();
void testSet_ASync();
void testSet_ASync_SyncCommit();

void OnChannelMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam) // 发送的回调函数。当需要分批发送时（比如中间需要返回进度，需要设定每个包的长度
{
	cout << szChannelName << ":" << szMessage;
	CMemDBSubscriberAsync *pMemDBSubscriber = (CMemDBSubscriberAsync *)pCallbackParam;
}

CMemDBClientAsync g_memDBClient;
CMemDBClientSync g_memDBClientSync;

int main_SubscribeSync(int argc, char **argv);
int main_publish_async(int argc, char **argv);

const char * g_channelName = "pkvideoctrl";
const char *g_szHostIp = "192.168.26.128";
const int g_nPort = 30001;
const char *g_szPassword = "1qazXSW@3edc";


int main(int argc, char **argv)
{
	if (argc <= 3) // ip is 
	{
		printf("usage:\n");
		printf("1.pkmemdbapi_test ip sub channel,channel2,channel3\n");
		printf("2.pkmemdbapi_test ip pub channelname message\n");

		//main_SubscribeSync();
		return 0;
	}
	
	char *szOpmode = argv[2];
	if (strstr(szOpmode, "pub") != NULL) // publish
	{
		main_publish_async(argc, argv);
		return 0;
	}
	else if (strstr(szOpmode, "sub") != NULL) // subscribe
	{
		main_SubscribeSync(argc, argv);
		return 0;
	}
	else
	{
		printf("usage:\n");
		printf("1.pkmemdbapi_test ip sub channelname\n");
		printf("2.pkmemdbapi_test ip pub channelname message\n");
	}

	return 0;
}

int main_publish_async(int argc, char **argv)
{
	if (argc < 5)
	{
		printf("usage:pkmemdbapi_test ip publish channelname message\n");
		return -1;
	}


	const char *szIp = argv[1];
	const char *channelName = argv[3];
	const char *message = argv[4];
	CMemDBClientAsync memDBClient;
	memDBClient.initialize(szIp, g_nPort, g_szPassword, 0, NULL, NULL);

	while (true)
	{
		memDBClient.publish(channelName, message);// "pkvieoctrl1,pkvieoctrl,2pkvieoctrl,3pkvieoctrl,4pkvieoctrl");
		memDBClient.commit();
		printf("publish,channel:%s, message:%s\n", channelName, message);
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
	}

	//getchar();
	//return 0;

	g_memDBClient.flushdb();
	g_memDBClient.commit();
}

int main_SubscribeSync(int argc, char **argv)
{
	if (argc < 4)
	{
		printf("usage:pkmemdbapi_test ip subribe channel1,channel2,channel3...\n");
		return -1;
	}

	const char *szIp = argv[1];
	const char *channels = argv[3];
	char szChannel[1024];

	vector<string> vecChannel = PKStringHelper::StriSplit(channels, ",");

	CMemDBSubscriberSync memDBSubscriberSync;
	memDBSubscriberSync.initialize(szIp, g_nPort, g_szPassword);
	for (int i = 0; i < vecChannel.size(); i++)
	{
		string strChannel = vecChannel[i];
		memDBSubscriberSync.subscribe(strChannel.c_str());
		printf("subscribed channel No.%d:%s\n", i, strChannel.c_str());
	}
	
	while (true)
	{
		string strChannel;
		string strMessage;
		int nRet = memDBSubscriberSync.receivemessage(strChannel, strMessage, 1000);
		printf("ret:%d, channel:%s, message:%s\n", nRet, strChannel.c_str(), strMessage.c_str());
	}
	return 0;
}

int main_subscribe_async()
{
	CMemDBSubscriberAsync memDBSubscriberAsync;
	cout << "subscriber test" << endl;
	memDBSubscriberAsync.initialize(g_szHostIp, g_nPort, g_szPassword, NULL, NULL);
	memDBSubscriberAsync.subscribe("control", OnChannelMsg, &memDBSubscriberAsync);

}
int main_readwrite_async() {
	g_memDBClient.initialize(g_szHostIp, 7379, "1qazXSW23edc", 0, NULL, NULL);
    //g_memDBClientSync.initialize(g_szHostIp, 7379, "1qazXSW23edc", 0);

    //g_memDBClient.set("hello", "42");
	char szValue[1024];
	//g_memDBClient.get("aa", szValue, sizeof(szValue), NULL);
	//std::vector<std::pair<std::string, std::string>> mVals;
	//mVals.push_back(std::make_pair<string, string>("aa1", "aa"));
	//mVals.push_back(std::make_pair<string, string>("bb1", "bb"));
	//mVals.push_back(std::make_pair<string, string>("cc1", "cc"));
	//g_memDBClient.mset(mVals);

    printf("begin g_memDBClient.get();\n");
	string strV = g_memDBClient.get("hello");
    printf("end g_memDBClient.get();val:%s\n",strV.c_str());

    //getchar();
    //return 0;

	g_memDBClient.flushdb();
	g_memDBClient.commit();

	//testMGet_Sync();
	//testSet_Sync();
	//testGet_Sync();
	testSet_ASync();


	//testSet();
	getchar();
	return 0;
}

char szSetValue[128] = "{\"value\":\"1234\",\"time\":\"2016-12-20 11:11:11\",\"quality\":\"0\",\"msg\":\"\",\"enablealarm\":\"1\"}";
time_t tmBegin, tmEnd;
//ACE_Time_Value tvBegin, tvEnd;
char szName[32];
int nTagNum = 100000;
int nCycle = 10;
string strName = szName;
string strEnableAlarm = "1";
string strQuality = "0";
string strMsg = "";
string strValue = "1234";
string strTime = "2016-12-20 11:11:11.111";

std::string strSetValue = szSetValue;
//ACE_Time_Value tvBegin, tvEnd;
// mac pro 笔记本，2014版
// 每秒可以进行10857次。用全新的tag点和 用老的tag点差不多]

void testSet_ASync_SyncCommit()
{
	printf("begin testSet\n");
	time(&tmBegin);
	int nCommitNum = 0;
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con%d.communication%d", iCycle, i + 1);
			strName = szName;
			char szValue[128];
			sprintf(szValue, "%d%s", tmNow, szSetValue);
			g_memDBClient.set(szName, szSetValue);
			if (i % 1000 == 0)
			{
				g_memDBClient.sync_commit();
				nCommitNum++;
			}
		}

		g_memDBClient.sync_commit();
		nCommitNum++;
		printf("commit %d tag this cycle, total commit %d times\n", nTagNum, nCommitNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nSec = tmEnd - tmBegin;
	g_memDBClient.sync_commit();
	printf("testSet_ASync_SyncCommit, %d writes, time:%d S, %d次/秒\n", nTimes, nSec, nTimes / nSec);
}

void testSet_ASync()
{
	printf("begin testSet\n");
	time(&tmBegin);
	int nCommitNum = 0;
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
        int nThisCmdCnt=0;
		for (int i = 0; i < nTagNum; i++)
		{
            nThisCmdCnt ++;
			sprintf(szName, "bus1con%d.communication%d", iCycle, i + 1);
			strName = szName;
			char szValue[128];
			sprintf(szValue, "%d%s", tmNow, szSetValue);
			g_memDBClient.set(szName, szSetValue);
			if (i % 1000 == 0)
			{
                g_memDBClient.sync_commit();
				nCommitNum++;
                nThisCmdCnt = 0;
			}
		}
		printf("commit %d tag this cycle, total commit %d times\n", nTagNum, nCommitNum);
        g_memDBClient.sync_commit();
		nCommitNum++;
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nSec = tmEnd - tmBegin;
    printf("beore sync_commit,testSet_ASync, %d writes, time:%d S, %d次/秒\n", nTimes, nSec, nTimes / nSec);
	g_memDBClient.sync_commit();
	printf("testSet_ASync, %d writes, time:%d S, %d次/秒\n", nTimes, nSec, nTimes / nSec);
}

void testGet_ASync()
{
	printf("begin testGet\n");
	time(&tmBegin);
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con1.communication%d", i + 1);
			strName = szName;
			string strValue = g_memDBClient.get(strName.c_str());
			//printf("%d,%d,%s\n", iCycle, i, strValue.c_str());
		}
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nMSec = tmEnd - tmBegin;
	g_memDBClient.sync_commit();
	printf("testGet, %d writes, time:%d S, XX次每秒\n", nTimes, nMSec);
}


void testSet_Sync()
{
	printf("begin testSet_Sync\n");
	time(&tmBegin);
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con1.communication%d", i + 1);
			strName = szName;
			char szValue[128];
			sprintf(szValue, "%d%s", tmNow, szSetValue);
			g_memDBClientSync.set(szName, szSetValue);
		}
		printf("cycle:%d,tag:%d\n", iCycle, nTagNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nSec = tmEnd - tmBegin;
	printf("testSet_Sync, %d writes, time:%d S, %d次/秒\n", nTimes, nSec, nTimes / nSec);
}

void testGet_Sync()
{
	printf("begin testGet_Sync\n");
	time(&tmBegin);
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con1.communication%d", i + 1);
			strName = szName;
			string strValue = g_memDBClientSync.get(strName.c_str());
			//printf("%d,%d,%s\n", iCycle, i, strValue.c_str());
		}
		printf("cycle:%d,tag:%d\n", iCycle, nTagNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nMSec = tmEnd - tmBegin;
	printf("testGet_Sync, %d writes, time:%d S, XX次每秒\n", nTimes, nMSec);
}


void testMGet_Sync()
{
	printf("begin testMGet_Sync\n");
	time(&tmBegin);
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		time_t tmNow;
		time(&tmNow);
		vector<string> vecKeys;
		for (int i = 0; i < nTagNum; i++)
		{
			if ((i+1) % 50 == 0)
			{
				string strKeys;
				CVecHelper::Vector2String(vecKeys, strKeys);
				string strValue = g_memDBClientSync.mget(strKeys.c_str());
				vector<string> vecValues;
				CVecHelper::String2Vector(strValue, vecValues);
				vecKeys.clear();
				vecValues.clear();
			}
			else
			{
				sprintf(szName, "bus1con1.communication%d", i + 1);
				strName = szName;
				vecKeys.push_back(strName);
			}
			//printf("%d,%d,%s\n", iCycle, i, strValue.c_str());
		}
		printf("cycle:%d,tag:%d\n", iCycle, nTagNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nMSec = tmEnd - tmBegin;
	printf("testGet_Sync, %d writes, time:%d S, XX次每秒\n", nTimes, nMSec);
}


/*
// mac pro 笔记本，2014版
// 每批次写入50个tag点，每秒均可以写入100000个点！
void testMSet()
{
	//tvBegin = ACE_OS::gettimeofday();
	time(&tmBegin);
	std::vector<std::pair<std::string, std::string>> vecKeyVal;
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		// 每批次50个
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con1.communication%d", i + 1);
			vecKeyVal.push_back(std::make_pair(szName, szSetValue));
			if (vecKeyVal.size() >= 50)
			{
				client.mset(vecKeyVal);
				vecKeyVal.clear();
				client.sync_commit();
			}
		}
	}

	client.sync_commit();
	time(&tmEnd);
	//tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	//ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	//int nMSec = tvTimeSpan.msec();
	int nMSec = tmEnd - tmBegin;

	printf("testMSet, %d writes, time:%d MS, XXX次每秒\n", nTimes, nMSec);
}


// mac pro 笔记本，2014版
// 每次写入50个点，每秒均可以写入250次共，12500个点！
void testHMSet()
{
	time(&tmBegin);

	std::vector<std::pair<std::string, std::string>> vecKeyVal;
	for (int iCycle = 0; iCycle < nCycle; iCycle++)
	{
		// 每批次50个
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con1.communication%d", i + 1);

			if (nTagNum % 50 == 0)
			{
				client.sync_commit();
			}
			vecKeyVal.push_back(std::make_pair("value", strValue));
			vecKeyVal.push_back(std::make_pair("time", strTime));
			vecKeyVal.push_back(std::make_pair("quality", strQuality));
			vecKeyVal.push_back(std::make_pair("msg", strMsg));
			vecKeyVal.push_back(std::make_pair("enablealarm", strEnableAlarm));
			client.hmset(strName, vecKeyVal);
			vecKeyVal.clear();
		}
	}

	time(&tmEnd);
	//tvEnd = ACE_OS::gettimeofday();
	int nTimes = nTagNum * nCycle;
	//ACE_Time_Value tvTimeSpan = tvEnd - tvBegin;
	//int nMSec = tvTimeSpan.msec();
	int nMSec = tmEnd - tmBegin;
	printf("testHMSet, %d writes, time:%d S,xxx次每秒\n", nTimes, nMSec);
}
*/
