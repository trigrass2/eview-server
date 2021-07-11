#include "drvframework.h"
#include "pkcomm/pkcomm.h"
#include "pkdriver/pkdrvcmn.h"
#include "pklog/pklog.h"
#include "shmqueue/ProcessQueue.h"
extern CPKLog g_logger;

PFN_InitDriver			g_pfnInitDriver = NULL;
PFN_InitDevice			g_pfnInitDevice = NULL;
PFN_UnInitDevice		g_pfnUnInitDevice = NULL;
PFN_OnDeviceConnStateChanged			g_pfnOnDeviceConnStateChanged;

PFN_OnTimer				g_pfnOnTimer = NULL;
PFN_OnControl			g_pfnOnControl = NULL;

PFN_UnInitDriver		g_pfnUnInitDriver = NULL;
PFN_GetVersion			g_pfnGetVersion = NULL;
CProcessQueue *			g_pQueCtrlFromNodeServer = NULL;			// 和进程管理器进行通信的共享内存队列

CProcessQueue *			g_pQueData2NodeServer = NULL;			// 和进程管理器进行通信的共享内存队列
// 如果质量为bad，szTagVal可能为空
// 如果无时间，时间nSec为0
int PublishKVSimple2NodeServer(int nActionType, const char *szKey, char *szValue, int nValueLen)
{
    if (nActionType != ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV && nActionType != ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("Driver[%s} PublishKVSimple2NodeServer, nActionType:%d, key:%s, value:%s, ret:%d"),
            g_strDrvName.c_str(), nActionType, szKey, szValue, -1001);
        return -1001;
    }

	int nDataBufLen = nValueLen + strlen(szKey) + 40; // 多+40个字节，总是够了
	char *szData = new char[nDataBufLen]; // 使用C++分配器
    char *pData = szData;

    // 放入时间戳
    unsigned int nPackTimestampSec = 0;
    unsigned int nPackTimestampMSec = 0;
    nPackTimestampSec = PKTimeHelper::GetHighResTime(&nPackTimestampMSec);
    memcpy(pData, &nPackTimestampSec, sizeof(unsigned int));
    pData += sizeof(unsigned int);
    memcpy(pData, &nPackTimestampMSec, sizeof(unsigned int));
    pData += sizeof(unsigned int);

    // ActionType
    memcpy(pData, &nActionType, sizeof(int));
    pData += sizeof(int);

    // tag名
    int nKeyLen = strlen(szKey);
    memcpy(pData, &nKeyLen, sizeof(int));
    pData += sizeof(int);
    memcpy(pData, szKey, nKeyLen);
    pData += nKeyLen;

    // tag值的长度和tag值内容
    memcpy(pData, &nValueLen, sizeof(int));
    pData += sizeof(int);
    if (nValueLen > 0)
    {
        memcpy(pData, szValue, nValueLen);
        pData += nValueLen;
    }
    CProcessQueue *pQue = g_pQueData2NodeServer;
    if (pQue)
    {
        int nRet = pQue->EnQueue(szData, pData - szData);
        if (nRet == PK_SUCCESS)
        {
            g_logger.LogMessage(PK_LOGLEVEL_DEBUG, _("Driver[%s] send tagdata to NodeServer success, type:%d, len:%d"), g_strDrvName.c_str(), nActionType, nValueLen);
        }
        else
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("Driver[%s] send tagdata to NodeServer failed, type:%d, len:%d, ret:%d"), g_strDrvName.c_str(), nActionType, nValueLen, nRet);
		delete[] szData;
        return nRet;
    }
    else
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PublishKVSimple2NodeServer.Get Que(data2NodeServer) failed");

	delete[] szData;
    return -100;
}
