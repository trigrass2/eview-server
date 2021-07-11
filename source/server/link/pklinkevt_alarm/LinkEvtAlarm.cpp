#define ACE_BUILD_SVC_DLL

#include "ace/svc_export.h"
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "MainTask.h"

#define PKLINKEVT_ALARM_LOGFILENAME		"pklinkevt_alarm"
PFN_LinkEventCallback	g_pfnLinkEvent = NULL;

CPKLog g_logger;

// ���ڸ�������Դ�����Ƿ��Ѿ�������Ϣ����������ĺ�����������������
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

// ���ַ����Ƶ���������ȥ
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

// �ӻ������ַ��������ַ����ȴ��4���ֽڳ��ȣ��ٴ��ʵ�����ݣ�������0��������
// nLeftBufLen ����Ҳ�ᱻ�ı�
int PopStringFromBuffer(char *&pBufer, int &nLeftBufLen, string &strValue)
{
	strValue = "";
	if(nLeftBufLen < 4)
		return -1;

	int nStrLen = 0;
	memcpy(&nStrLen, pBufer, 4);
	nLeftBufLen -= 4;
	pBufer += 4;

	if(nStrLen <= 0) // �ַ�������Ϊ��
		return 0;

	if(nStrLen > nLeftBufLen) // ʣ��Ļ��������Ȳ���
		return -2;

	char *pszValue = new char[nStrLen + 1];
	memcpy(pszValue, pBufer, nStrLen);
	nLeftBufLen -= nStrLen;
	pBufer += nStrLen;

	pszValue[nStrLen] = '\0';
	strValue = pszValue;
	return 0;
}

// �ӻ������ַ��������ַ����ȴ��4���ֽڳ��ȣ��ٴ��ʵ�����ݣ�������0��������
// nLeftBufLen ����Ҳ�ᱻ�ı�
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