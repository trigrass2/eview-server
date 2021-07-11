/**************************************************************
 *  Filename:    MqttForwardServer.cpp
 *  Copyright:   XinHong Software Co., Ltd.
 *
 *  Description: Mqtt Upload Server
 *
 *  @author:     xingxing
 *  @version     2021/07/09 Initial Version
**************************************************************/
#include "MqttForwardServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;
CMqttForwardServer *g_mqttForwardServer = NULL;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CMqttForwardServer::CMqttForwardServer() : CPKServerBase(true)
{
}

CMqttForwardServer::~CMqttForwardServer()
{
}

/**
 *  Initialize Service,Access of Program;
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2018  xingxing  Initial Version.;
 第二个参数为客户端名！！！
 */

int CMqttForwardServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(("========== MqttForwaredServer Init =========\n"));

	//获取运行时数据目录;
	string strRunTimePath = PKComm::GetRunTimeDataPath();
    // 创建接受驱动数据的共享队列;

	// 初始记为正常启动;
	//UpdateServerStatus(0, "started");
	//UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CMqttForwardServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

//#ifdef YCD
int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
    g_mqttForwardServer = new CMqttForwardServer();
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKMqttSubServer::Main(), argc(%d) start...===============", argc);

    int nRet = g_mqttForwardServer->Main(argc, args);
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKMqttSubServer::Main Return(%d)", nRet);
    if (NULL != g_mqttForwardServer)
	{
        delete g_mqttForwardServer;
        g_mqttForwardServer = NULL;
	}
	return nRet;
}
//#endif