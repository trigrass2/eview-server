/**************************************************************
 *  Filename:    ATSAnalyzeServer.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: ATS数据解析服务源文件.
 *
 *  @author:     wanghongwei
 *  @version     10/10/2016  wanghongwei  Initial Version
**************************************************************/

#include "PKSockCliSvr.h"
#include "SockClientTask.h"
#include "SockServerTask.h"

#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
CPKLog PKLog;
using namespace std;


class CPKSockCliSvr : public CPKServerBase
{
public:
	bool Initializing;
	
public:
	CPKSockCliSvr();
	virtual ~CPKSockCliSvr();

public:
	int OnStart(int argc, char* args[]);
	int OnStop();
};

//ctor
CPKSockCliSvr::CPKSockCliSvr() : CPKServerBase("PKSockCliSvr", false)
{

}
//dctor
CPKSockCliSvr::~CPKSockCliSvr()
{	
}


int CPKSockCliSvr::OnStart( int argc, char* args[] )
{
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKSockCliSvr::OnStart");
	SOCKCLIENT_TASK->StartTask();
	SOCKSERVER_TASK->StartTask();
	return 0;
}

int CPKSockCliSvr::OnStop()
{
	SOCKCLIENT_TASK->StopTask();
	SOCKSERVER_TASK->StopTask();
	return 0;
}

int main(int argc, char* args[])
{
	PKLog.SetLogFileName("pksockclisvr");
	CPKSockCliSvr *pServer = new CPKSockCliSvr();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKSockCliSvr::DoService(), argc(%d).", argc);
	int nRet = pServer->Main(argc, args);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKSockCliSvr::DoService Return(%d)", nRet);
	if (NULL != pServer)
	{
		delete pServer;
		pServer = NULL;
	}
	return nRet;
}
