#define ACE_BUILD_SVC_DLL

#include "ace/svc_export.h"
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "MainTask.h"

#define PKLINKEVT_ALARM_LOGFILENAME		"pklinkevt_alarm"
PFN_LinkEventCallback	g_pfnLinkEvent = NULL;

CPKLog g_logger;

// 用于各个触发源传递是否已经触发消息给联动服务的函数。由联动服务传入
extern "C" ACE_Svc_Export long InitEvent(PFN_LinkEventCallback pfnLinkEvent)
{
	g_logger.SetLogFileName(PKLINKEVT_ALARM_LOGFILENAME);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"pklinkevt_alarm.dll begin");
	g_pfnLinkEvent = pfnLinkEvent;
	MAIN_TASK->StartTask();
	return 0;
}

extern "C" ACE_Svc_Export long ExitEvent()
{
	MAIN_TASK->StopTask();
	g_pfnLinkEvent = NULL;
	return 0;
}


int PushInt32ToBuffer(char *&pBufer, int &nLeftBufLen, int nValue)
{
	if(nLeftBufLen < 4)
		return -1;

	memcpy(pBufer, &nValue, 4);
	pBufer +=4;
	nLeftBufLen -= 4;

	return 0;
}

// 将字符串推到缓冲区中去
int PushStringToBuffer(char *&pSendBuf, int &nLeftBufLen, const char *szStr, int nStrLen)
{
	if(nLeftBufLen < nStrLen + 4)
		return -1;

	memcpy(pSendBuf, &nStrLen, 4);
	pSendBuf +=4;
	nLeftBufLen -= 4;

	if(nStrLen > 0)
	{
		memcpy(pSendBuf, (char *)szStr, nStrLen);
		pSendBuf += nStrLen;
		nLeftBufLen -= nStrLen;
	}

	return 0;
}

// 从缓冲区字符串出，字符串先存放4个字节长度，再存放实际内容（不包含0结束符）
// nLeftBufLen 长度也会被改变
int PopStringFromBuffer(char *&pBufer, int &nLeftBufLen, string &strValue)
{
	strValue = "";
	if(nLeftBufLen < 4)
		return -1;

	int nStrLen = 0;
	memcpy(&nStrLen, pBufer, 4);
	nLeftBufLen -= 4;
	pBufer += 4;

	if(nStrLen <= 0) // 字符串长度为空
		return 0;

	if(nStrLen > nLeftBufLen) // 剩余的缓冲区长度不够
		return -2;

	char *pszValue = new char[nStrLen + 1];
	memcpy(pszValue, pBufer, nStrLen);
	nLeftBufLen -= nStrLen;
	pBufer += nStrLen;

	pszValue[nStrLen] = '\0';
	strValue = pszValue;
	return 0;
}

// 从缓冲区字符串出，字符串先存放4个字节长度，再存放实际内容（不包含0结束符）
// nLeftBufLen 长度也会被改变
int PopInt32FromBuffer(char *&pBufer, int &nLeftBufLen, int &nValue)
{
	nValue = 0;
	if(nLeftBufLen < 4)
		return -1;

	memcpy(&nValue, pBufer, 4);
	nLeftBufLen -= 4;
	pBufer += 4;

	return 0;
}