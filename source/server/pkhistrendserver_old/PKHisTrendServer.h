// CPKHisTrendServer.cpp : 定义控制台应用程序的入口点。
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <iostream>
#include "MainTask.h"
#include "LoadDateBase.h"

using namespace std;
#define	SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit
#define PK_SUCCESS	0								// iCV系统中所有成功消息的返回码
#define PK_FAILURE	-1


class CPKHisTrendServer :public CPKServerBase
{
public:
	CPKHisTrendServer();
	virtual ~CPKHisTrendServer(){};

	virtual int	OnStart(int argc, ACE_TCHAR* args[]);

	virtual int OnStop();

	virtual void PrintStartUpScreen();

	virtual void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		//PKLog.SetLogFileName("CPKHisTrendServer");
	}
	void StartAllTasks();

public:

};
CPKHisTrendServer PKHisTrend;
CPKHisTrendServer *g_pServer = NULL;