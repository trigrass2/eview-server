/************************************************************************
*	Filename:		CVDRIVERCOMMON.CPP
*	Copyright:		Shanghai Peak InfoTech Co., Ltd.
*
*	Description:	$(Desp) .
*
*	@author:		shijunpu
*	@version		shijunpu	Initial Version
*  @version	9/17/2012  shijunpu  增加设置元素大小接口 .
*************************************************************************/
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"

#define BITS_PER_BYTE 8

#ifdef WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze 
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher;
#endif//#ifndef NO_EXCEPTION_ATTACHER
#endif

#include <sstream>
#include <string>
#include <fstream>  
#include <streambuf>  
#include <iostream>
#include "json/json.h"
#include "MemDBTask.h"
#include "pkservermgr/pkservermgr_plugin.h"

using namespace std;

#ifdef _WIN32
#include "windows.h"
#define  SERMGRCFG_GEN_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  SERMGRCFG_GEN_EXPORTS extern "C" __attribute__ ((visibility ("default")))
#endif //_WIN32

#define NULLASSTRING(x) x==NULL?"":x

CPKLog g_logger;

PFN_OnControl	g_pfnOnControl = NULL;	// pkservermgr主进程的函数

// 这里的IP是指要连接的内存数据库IP。szUserName：初始化模拟登录的用户名称。szPassword：初始化模拟登录的用户密码，不是内存数据库的密码
SERMGRCFG_GEN_EXPORTS int BeginFuncAfterProcLoad()
{
	g_logger.SetLogFileName("proc2memdb");
	int nRet = MEMDB_TASK->StartTask();
	return nRet;
}

SERMGRCFG_GEN_EXPORTS int EndFunc()// 发生错误时需要重新
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "EndFunc: proc2memdb!");
	MEMDB_TASK->StopTask();
	return 0;
}

// 设置每个进程状态的列表
SERMGRCFG_GEN_EXPORTS int OnProcessStatusChange(char *szJsonProcessInfo)
{
	// g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnProcessStatusChange:%s", szJsonProcessInfo);
	MEMDB_TASK->m_memDBClientAsync.set("pm.processes", szJsonProcessInfo);
	MEMDB_TASK->m_memDBClientAsync.commit();
	return 0;
}

SERMGRCFG_GEN_EXPORTS int SetOnControlCallbackFunc(PFN_OnControl pfnOnControl)
{
	g_pfnOnControl = pfnOnControl;
	return 0;
}