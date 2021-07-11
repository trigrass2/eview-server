/**************************************************************
 *  Filename:    CRService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description:  Defines the entry point for the console application.
 *
 *  @author:     shijunpu
 *  @version     05/23/2008  shijunpu  Initial Version
**************************************************************/

#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "PKHisDataServer.h"
#include "pkcrashdump/pkcrashdump.h"
//#ifdef _WIN32
//#include "common/RegularDllExceptionAttacher.h"
//CRegularDllExceptionAttacher g_ExceptionAttacher;
//
//#pragma comment(lib,"dbghelp.lib")
//#pragma comment(lib,"ExceptionReport.lib")
//#endif

CPKLog g_logger;
#define _x(x)	x
#define		SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit

CPKHisDataServer::CPKHisDataServer() : CPKServerBase(true)
{
}

int CPKHisDataServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf("========== PKHisDataServer Init =========\n");

	int nRet = MAIN_TASK->StartTask();
	return nRet;
}

int CPKHisDataServer::OnStop()
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "PKHisDataServer::OnStop()");
	MAIN_TASK->StopTask();
	return 0;
}

void CPKHisDataServer::PrintStartUpScreen()
{
	printf("\n");
	printf("------------Welcome to PKHisDataServer-----------\n");
	printf("\tsupport the following commands	             \n");
	printf("\tq/Q:Quit                                       \n");
	printf("\tOthers:Print tips                              \n");
	printf("-------------------------------------------------\n");
}
void CPKHisDataServer::PrintHelpScreen()
{
	PrintStartUpScreen();
}

int main(int argc, char* args[])
{
	g_logger.SetLogFileName("pkhisdataserver");
	CPKHisDataServer *pServer =new CPKHisDataServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "start PKHisDataServer::main(argc(%d)).", argc);
	int nRet = pServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKHisDataServer::main stopped return(%d)", nRet);
	if (NULL != pServer)
	{
		delete pServer;
		pServer = NULL;
	}
	return nRet;
}

#include <fstream>  
#include <streambuf>  
#include <iostream>  
#include "common/eviewdefine.h"
// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
}
