// CPKHisTrendServer.cpp : �������̨Ӧ�ó������ڵ㡣
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <iostream>
#include "MainTask.h"
#include "LoadDateBase.h"

using namespace std;
#define	SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit
#define PK_SUCCESS	0								// iCVϵͳ�����гɹ���Ϣ�ķ�����
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
	//������־�ļ����ƣ���֤��ӡServiceBase�����־���������ļ���
	virtual void SetLogFileName()
	{
		//PKLog.SetLogFileName("CPKHisTrendServer");
	}
	void StartAllTasks();

public:

};
CPKHisTrendServer PKHisTrend;
CPKHisTrendServer *g_pServer = NULL;