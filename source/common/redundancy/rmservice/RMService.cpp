/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "RMService.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "common/pkProcessController.h"

#include "common/cvRedisAccess.h"
#include "common/cvRedisDefine.h"
#include "common/pkGlobalHelper.h"

#include <string>
using namespace std;

// 进程启停控制器
//CCVProcessController g_cvProcController("RMService");

CServiceBase* g_pServiceHandler = new CRMService();
STATERWINSTANCE g_pStateRW;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRMService::CRMService() : CServiceBase("RMService", true, NULL)
{
	g_pStateRW = NULL;
}

CRMService::~CRMService()
{
	if (g_pStateRW)
	{
		STATERW_HostUnInitialize(g_pStateRW);
		g_pStateRW = NULL;
	}
}

/**
 *  Run Service.
 *
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
/*void CRMService::RunService()
{
	LH_LOG((LOG_LEVEL_INFO, "RMService Started!"));
	m_RMSCtrl.StartThread();
	//InteractiveLoop();
	m_RMSCtrl.StopThread();	
	LH_LOG((LOG_LEVEL_INFO, "RMService Stopped!"));

}//*/

/**
 *  Do Some Finalize Stuffs.
 *
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
/*void CRMService::FiniService()
{
	
}//*/

/**
 *  Main Thread Processing.
 *
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
/*void CRMService::InteractiveLoop()
{
	std::cout << ">>>" << std::flush;
	
	char c;
	bool bFlag = true;
	do 
	{
		if(g_cvProcController.CV_KbHit())
		{
			c = getchar();
			switch(c)
			{
			case 'Q':
			case 'q':
				std::cout << "Exiting..."<< std::endl;
				bFlag = false;
				break;
			case 'V':
			case 'v':
				std::cout << "RM Status:";
				std::cout << m_RMSCtrl.GetStatus() << std::endl;
 				break;
			case 0x0a: // "return"
				std::cout << ">>>" << std::flush;
				continue;
			case 'H':
			case 'h':
			default:
				std::cout << "H/h: Help\n";
				std::cout << "Q/q: Exit\n";
				std::cout << "V/v: Print Status" << std::endl;
				break;
			}
		}

		// 判断是否退出信号量被触发
		if(g_cvProcController.IsProcessQuitSignaled())
		{
			bFlag = false;
			break;
		}

		ACE_Time_Value tvTimeOut;
		tvTimeOut.msec(200);
		ACE_OS::sleep(tvTimeOut);

	} while( bFlag );
}//*/

//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
void CRMService::SetLogFileName()
{
	LH_SET_LOG_FILE( DEFAULT_RM_SVC_FILE_NAME );
}


/**
 *  Initialize Service.
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
long CRMService::Init( int argc, ACE_TCHAR* args[] )
{
	printf("========== RMService Init =========\n");
// 	if (CVComm.GetCVEnv())
// 	{
// 		return EC_ICV_COMM_GETENVFAIL;
// 	}

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	STATERW_HostInitialize("",0,timeout,&g_pStateRW);

	ACE_Time_Value timeValue = ACE_OS::gettimeofday();
	HighResTime timeStamp;
	timeStamp.tv_sec = (uint32_t)timeValue.sec();
	timeStamp.tv_usec = (uint32_t)timeValue.usec();
	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	CVTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	cvcommon::CastTimeToASCII(szTime, ICV_HOSTNAMESTRING_MAXLEN, timeStamp);

	// 初始记为正常启动
	char szStatus[ICV_HOSTNAMESTRING_MAXLEN];
	memset(szStatus, 0x00, ICV_HOSTNAMESTRING_MAXLEN);
	ACE_OS::snprintf(szStatus, sizeof(szStatus), "0;%s", szTime);
	STATERW_SetStringCommand(&g_pStateRW, CV_STATUS_RMSVR_STATUS, szStatus);
	// 启动时刻
	STATERW_SetStringCommand(&g_pStateRW, CV_STATUS_RMSVR_STARTTIME, szTime);

	m_strHome = CVComm.GetCVRunTimeDataPath();
	ACE_OS::mkdir(m_strHome.c_str());
	m_strCfgFile = CVComm.GetCVProjCfgPath();
	m_strCfgFile += ACE_DIRECTORY_SEPARATOR_STR DEFAULT_RM_SVC_FILE_NAME ACE_TEXT(".xml");

	//Initialization Process:
	//1. Parse Command Line Options
	long nErr = ParseOptions(argc, args);
	if ( nErr != ICV_SUCCESS)
		return nErr;

	timeValue = ACE_OS::gettimeofday();
	timeStamp.tv_sec = (uint32_t)timeValue.sec();
	timeStamp.tv_usec = (uint32_t)timeValue.usec();
	memset(szTime, 0x00, ICV_HOSTNAMESTRING_MAXLEN);
	cvcommon::CastTimeToASCII(szTime, ICV_HOSTNAMESTRING_MAXLEN, timeStamp);

	//2. Load Configuration files
	nErr = m_cfgLoader.LoadConfig(m_strCfgFile,&m_RMSCtrl);
	if (nErr != ICV_SUCCESS)
	{
		// 记录异常状态
		char szStatus[ICV_HOSTNAMESTRING_MAXLEN];
		memset(szStatus, 0x00, ICV_HOSTNAMESTRING_MAXLEN);
		ACE_OS::snprintf(szStatus, sizeof(szStatus), "%d;%s", nErr, szTime);
		STATERW_SetStringCommand(&g_pStateRW, CV_STATUS_RMSVR_STATUS, szStatus);
		return nErr;
	}

	//4. Initialize Shared Memery.
	nErr = m_RMSCtrl.Initialize(m_strHome);
	if (nErr != ICV_SUCCESS)
	{
		// 记录异常状态
		char szStatus[ICV_HOSTNAMESTRING_MAXLEN];
		memset(szStatus, 0x00, ICV_HOSTNAMESTRING_MAXLEN);
		ACE_OS::snprintf(szStatus, sizeof(szStatus), "%d;%s", nErr, szTime);
		STATERW_SetStringCommand(&g_pStateRW, CV_STATUS_RMSVR_STATUS, szStatus);
	}
	return nErr;
}

/**
 *  Parse Command Line Options.
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
long CRMService::ParseOptions(int argc, ACE_TCHAR* args[])
{
// 	ACE_Get_Opt cmd_opts(argc, args);
// 	cmd_opts.long_option(LONG_OPTION_SIM_MODE, SHORT_OPTION_SIM_MODE, ACE_Get_Opt::NO_ARG);
// 
// 	long nOption;
// 
// 	while ((nOption = cmd_opts()) != EOF)
// 	{
// 		switch (nOption)
// 		{
// 		case SHORT_OPTION_SIM_MODE:
// 			ACE_OS::printf("CVCollector running in the Simulation Mode!\n");
// 			DATA_COLLECTOR::instance()->m_bSimMode = true;
// 			break;
// 		default:
// 			break;
// 		}
// 	}
	return ICV_SUCCESS;
}

bool CRMService::ProcessCmd( char c )
{
	switch(c)
	{
	case 'V':
	case 'v':
		std::cout << "RM Status:";
		std::cout << m_RMSCtrl.GetStatus() << std::endl;
		return true;
	default:
		return false;
	}
}

void CRMService::PrintHelpScreen()
{
	std::cout << "H/h: Help\n";
	std::cout << "Q/q: Exit\n";
	std::cout << "V/v: Print Status" << std::endl;
}

void CRMService::Refresh()
{
	CRMStatusCtrl	newRMStatus;

	long nErr = m_cfgLoader.LoadConfig(m_strCfgFile,&newRMStatus);
	if (nErr != ICV_SUCCESS)
	{
		LH_LOG((LOG_LEVEL_ERROR, ACE_TEXT("[EC:%d] Refresh(): Load Config Failed!"), nErr));
		return;
	}

	if (m_RMSCtrl != newRMStatus)
	{
		LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Refresh(): Config Change Reboot!"), nErr));
		CServiceBase::Refresh();
	}
}
