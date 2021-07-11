#ifndef _CRS_SERVICE_HXX_DEF_
#define	_CRS_SERVICE_HXX_DEF_

#include <ace/ACE.h>
#include <ace/SOCK_Stream.h>
#include <ace/Event_Handler.h>
#include <ace/Null_Mutex.h>
#include <ace/Synch.h>
#include <ace/Message_Queue.h>
#include <ace/Singleton.h>
#include <ace/Task.h>
#include "common/OS.h"
#include <ace/Get_Opt.h>
#include <ace/Containers.h>

#include "pkcomm/pkcomm.h"
#include "common/RMAPI.h"
#include "pklog/pklog.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include "pkserver/PKServerBase.h"
extern CPKLog g_logger;

class CMainTask;
class CPKHisDataServer :public CPKServerBase
{
public:
	CPKHisDataServer();
	virtual ~CPKHisDataServer(){};

	virtual int	OnStart(int argc, ACE_TCHAR* args[]);

	virtual int OnStop();

	virtual void PrintStartUpScreen();

	virtual void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		g_logger.SetLogFileName("PKHisDataServer");
	}

	void StartAllTasks();
public:
	CMainTask *m_pTrendTimerTask;
	CMainTask *m_pRecordTimerTask;
};


#endif //_CRS_SERVICE_HXX_DEF_
