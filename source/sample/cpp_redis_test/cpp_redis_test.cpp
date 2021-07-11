//#include "ace/Time_Value.h"
//#include <ace/Task.h>
#include <cpp_redis/cpp_redis>
#include <iostream>
#include <string>
#include <map>
//#include <utility>
using namespace std;

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

cpp_redis::redis_client client;
cpp_redis::redis_subscriber sub;
void testSet_ASync();
void testSet_ASync_SyncCommit();

int main(void) {
	//! Enable logging
	//cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	client.auth("1qazXSW23edc");
	//sub.auth("1qazXSW23edc");
	try {
		client.connect("127.0.0.1", 7379, [](cpp_redis::redis_client& client) {
			std::cout << "client disconnected (disconnection handler)" << std::endl;
			});

		sub.connect("127.0.0.1", 7379, [](cpp_redis::redis_subscriber&) {
			std::cout << "sub disconnected (disconnection handler)" << std::endl;
		});
		client.sync_commit();
		client.select(0);
		client.sync_commit();

		client.flushdb();
		client.sync_commit();
		testSet_ASync();
	}
	catch(std::exception e){
		std::cout << "aaa" << e.what();
	}

	string strKey = "hello";
	// same as client.send({ "SET", "hello", "42" }, ...)
	client.set(strKey, "42", [](cpp_redis::reply& reply) {
		std::cout << "set hello 42: " << reply << std::endl;
	});

	//client.keys("pm:*", [](cpp_redis::reply& reply) {
	//	std::cout << "decrby hello 12: " << reply << std::endl;
	//	std::vector<cpp_redis::reply> vecReplies = reply.as_array();
	//	for (std::vector<cpp_redis::reply>::iterator itReply = vecReplies.begin(); itReply != vecReplies.end(); itReply++)
	//	{
	//		std::cout << "keys:" << itReply->as_string() << endl;
	//	}
	//});

	// same as client.send({ "GET", "hello" }, ...)
	client.get("hello", [](cpp_redis::reply& reply) {
		string s = reply.as_string();
		std::cout << "get hello: " << s << std::endl;
		// if (reply.is_string())
		//   do_something_with_string(reply.as_string())
	});

	// commands are pipelined and only sent when client.commit() is called
	// client.commit();

	// synchronous commit, no timeout
	try {
		client.sync_commit();
	}
	catch (std::exception e)
	{
		std::cout << e.what();
	}
	printf("begin\n");

	vector<string> vecParams;
	vecParams.push_back("aa");
	vecParams.push_back("bb");
	client.publish("control", "ccddd");
	client.commit();

	sub.subscribe("control", [](const std::string& chan, const std::string& msg) {
		std::cout << "MESSAGE " << chan << ": " << msg << std::endl;
	});
	sub.psubscribe("*", [](const std::string& chan, const std::string& msg) {
		std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
	});
	sub.commit();

	//while (1)
	//{
	//	cpp_redis::network::Sleep(50);
	//}
	
	printf("end\n");
	getchar();
	return 0;
}


char szSetValue[128] = "{\"value\":\"1234\",\"time\":\"2016-12-20 11:11:11\",\"quality\":\"0\",\"msg\":\"\",\"enablealarm\":\"1\"}";
time_t tmBegin, tmEnd;
//ACE_Time_Value tvBegin, tvEnd;
char szName[32];
int nTagNum = 100000;
int nCycle = 5;
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
			//char szValue[128];
			//sprintf(szValue, "%d%s", tmNow, szSetValue);
			client.set(szName, szSetValue);
			if (i % 1000 == 0)
			{
				client.sync_commit();
				nCommitNum++;
			}
		}
		printf("commit %d value, %d times\n", nTagNum, nCommitNum);
		client.sync_commit();
		nCommitNum++;
		printf("commit %d value, %d times\n", 100, nCommitNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nSec = tmEnd - tmBegin;
	client.sync_commit();
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
		for (int i = 0; i < nTagNum; i++)
		{
			sprintf(szName, "bus1con%d.communication%d", iCycle,i + 1);
			strName = szName;
			char szValue[128];
			sprintf(szValue, "%d%s", tmNow, szSetValue);
			client.set(szName, szSetValue);
			if (i % 1000 == 0)
			{
				client.commit();
				nCommitNum++;
			}
		}
		printf("commit %d value, %d times\n", nTagNum, nCommitNum);
		client.commit();
		nCommitNum++;
		printf("commit %d value, %d times\n", 100, nCommitNum);
	}

	time(&tmEnd);
	int nTimes = nTagNum * nCycle;
	int nSec = tmEnd - tmBegin;
	client.commit();
	printf("testSet_ASync, %d writes, time:%d S, %d次/秒\n", nTimes, nSec, nTimes / nSec);
}
//
//void testGet_ASync()
//{
//	printf("begin testGet\n");
//	time(&tmBegin);
//	for (int iCycle = 0; iCycle < nCycle; iCycle++)
//	{
//		time_t tmNow;
//		time(&tmNow);
//		for (int i = 0; i < nTagNum; i++)
//		{
//			sprintf(szName, "bus1con1.communication%d", i + 1);
//			strName = szName;
//			string strValue = client.get(strName.c_str());
//			//printf("%d,%d,%s\n", iCycle, i, strValue.c_str());
//		}
//	}
//
//	time(&tmEnd);
//	int nTimes = nTagNum * nCycle;
//	int nMSec = tmEnd - tmBegin;
//	client.sync_commit();
//	printf("testGet, %d writes, time:%d S, XX次每秒\n", nTimes, nMSec);
//}


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