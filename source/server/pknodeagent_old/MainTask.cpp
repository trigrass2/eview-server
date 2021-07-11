/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    lijingjing
 *  @version    05/28/2008  lijingjing  Initial Version
 *  @version	9/17/2012  baoyuansong  数据块类型修改为字符串类型，并且修改增加数据块时的接口
 *  @version	3/1/2013   baoyuansong  增加由点生成块的逻辑，当设备下没有配置数据块时，直接由点生成数据块 .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"

#include "MainTask.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/pkdefine.h"
#include "pklog/pklog.h"
#include "pkNodeAgent.h"
#include "CtrlRouteTask.h"
#include "RTDataRouteTask.h"
#include "AlarmRouteTask.h"
#include "DownloadTask.h"

extern CNodeAgent* g_pServer;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CMainTask::CMainTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CMainTask::~CMainTask()
{
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CMainTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = OnStart();
	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"PbServer 正常退出！");
	OnStop();

	m_bStopped = true;
	return nRet;	
}

int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	
	nStatus = RMAPI_Init();
	if (nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"初始化RMAPI失败!");
	}

	nStatus = LoadConfig();
	if (nStatus != PK_SUCCESS)
	{
		//记录日志，读取配置文件失败
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"load config failed, file error or doesnot exist,ret:%d!", nStatus);
		g_pServer->UpdateServerStatus(-100,"configfile error");
		return -1;
	}
	//PKLog.LogMessage(PK_LOGLEVEL_NOTICE," load config success, tagnum:%d!", );
	InitRedis(); // 清空驱动中redis的实时数据和配置

	CTRLROUTE_TASK->Start();
	RTDATAROUTE_TASK->Start();
	ALARMROUTE_TASK->Start();
	DOWNLOAD_TASK->Start();
	return PK_SUCCESS;
}

// 本任务停止时
void CMainTask::OnStop()
{
	// 停止其他线程
	CTRLROUTE_TASK->Stop();
	RTDATAROUTE_TASK->Stop();
	ALARMROUTE_TASK->Stop();
	DOWNLOAD_TASK->Stop();
	m_redisRW.Finalize();
}

#define NULLASSTRING(x) x==NULL?"":x
int CMainTask::LoadConfig()
{
	int nRet = PK_SUCCESS;
	
	return nRet;
}


int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

/* 将变量的配置信息写入到redis中，以便在进行控制时可以快速根据tag点信息找到对应的设备信息
db0:
删除所有的tag点
db1:
 要写入的数据配置数据包括：tag点对应的配置（tagname为key，value为hash表：drivername、devicename、tag点信息等)
*/
int CMainTask::InitRedis()
{
	int nRet = 0;
	nRet = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"m_redisSub.Initialize failed,ret:%d",nRet);
		return nRet;
	}

	return nRet;
}
