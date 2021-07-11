/************************************************************************
*	Filename:		CVDRIVERCOMMON.CPP
*	Copyright:		Shanghai Peak InfoTech Co., Ltd.
*
*	Description:	$(Desp) .
*
*	@author:		shijunpu
*	@version		shijunpu	Initial Version
*  @version	9/17/2012  shijunpu  ��������Ԫ�ش�С�ӿ� .
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

PFN_OnControl	g_pfnOnControl = NULL;	// pkservermgr�����̵ĺ���

// �����IP��ָҪ���ӵ��ڴ����ݿ�IP��szUserName����ʼ��ģ���¼���û����ơ�szPassword����ʼ��ģ���¼���û����룬�����ڴ����ݿ������
SERMGRCFG_GEN_EXPORTS int BeginFuncAfterProcLoad()
{
	g_logger.SetLogFileName("proc2memdb");
	int nRet = MEMDB_TASK->StartTask();
	return nRet;
}

SERMGRCFG_GEN_EXPORTS int EndFunc()// ��������ʱ��Ҫ����
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "EndFunc: proc2memdb!");
	MEMDB_TASK->StopTask();
	return 0;
}

// ����ÿ������״̬���б�
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