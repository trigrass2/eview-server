/**************************************************************
*  Filename:    Device.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 设备信息实体类.
*
**************************************************************/

#include <ace/ACE.h>
#include <ace/Default_Constants.h>
#include <ace/OS.h>
#include "ace/OS_NS_stdio.h"
#include "Device.h"
#include "TaskGroup.h"
#include "common/CommHelper.h"
#include "pkcomm/pkcomm.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "common/RMAPI.h"
#include "string.h"
#include "common/gettimeofday.h"
#include "UserTimer.h"
#include "MainTask.h"
#include "shmqueue/ProcessQueue.h"
#include "../../server/pknodeserver/RedisFieldDefine.h"
#include "pklog/pklog.h"
#include "ByteOrder.h"

extern CPKLog g_logger;
extern bool g_bSysBigEndian;
extern int TagValBuf_ConvertEndian(bool bDeviceIsBigEndian, char *szTagName, int nTagDataType, const char *szInBinData, int nBinDataBufLen, int nTagOffsetBits);
extern int TagValFromBin2String(bool bDeviceIsBigEndian, int nTagDataType, const char *szInBinData, int nBinDataBufLen, int nTagOffsetBits, string &strTagValue);
// szStrData和szBinData可能指向同一片内存区域！
extern int TagValFromString2Bin(PKTAG *pTag, int nTagDataType, const char *szStrData, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBits);
extern bool IsActiveHost();
extern CProcessQueue *g_pQueData2NodeServer;

extern void PKDEVICE_Reset(PKDEVICE *pDevice);
extern void PKDEVICE_Free(PKDEVICE *pDevice);

/**
*  构造函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CDevice::CDevice(CTaskGroup *pTaskGrp, const char *pszDeviceName) :m_pTaskGroup(pTaskGrp)
{
    // 设备是否支持多连接
    m_bMultiLink = false;
    m_pTimers = NULL;
    m_nTimerNum = 0;

    m_strName = "";
    m_strName = pszDeviceName;
    //memset(m_szTmpTagValue, 0, sizeof(m_szTmpTagValue));
    memset(m_szTmpTagTimeWithMS, 0, sizeof(m_szTmpTagTimeWithMS));

    m_strDesc = "";
	m_bConfigChanged = false; // 第一次有连接，如果设为true则连接、关闭、再连接
    m_tvTimerBegin = CVLibHead::gettimeofday_tickcnt();

	m_hPKCommunicate = NULL;
    m_strTaskID = "";
    m_pOutDeviceInfo = new PKDEVICE();
	PKDEVICE_Reset(m_pOutDeviceInfo);
    m_pOutDeviceInfo->_pInternalRef = (void *)this;
    m_bDeviceBigEndian = false; // 缺省为小端序，三菱即为这种

    m_nConnCount = 0; // 连接次数
    memset(m_szLastConnTime, 0, sizeof(m_szLastConnTime)); // 上次连接时间

    m_bLastConnStatus = false;
    m_tmLastDisconnect = 0; // 0表示未断开
    m_nDisconnectConfirmSeconds = 10; // 缺省为10秒
    m_nPollRate = 1000;
    m_nRecvTimeOut = 2000;
    m_nConnTimeOut = 2000;

	m_pTagEnableConnect = NULL;
	m_pTagConnStatus = NULL;

	Py_CDeviceConstructor(this);
	m_nConnStatusTimeoutMS = 0;
	m_tvLastConnOK = ACE_OS::gettimeofday();
}


/**
*  析构函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CDevice::~CDevice()
{
	std::vector< CUserTimer*>::iterator iterBlks = m_vecTimers.begin();
	for (; iterBlks != m_vecTimers.end(); iterBlks++)
	{
		CUserTimer * pDataBlock = (CUserTimer *)*iterBlks;
		SAFE_DELETE(pDataBlock);
	}
	m_vecTimers.clear();

    std::map<std::string, PKTAG*>::iterator iterTag = m_mapName2Tags.begin();
    for (; iterTag != m_mapName2Tags.end(); iterTag++)
    {
        PKTAG * pTag = (PKTAG *)iterTag->second;
        SAFE_DELETE(pTag);
    }
    m_mapName2Tags.clear();

    std::map<std::string, PKTAGDATA*>::iterator iterTagData = m_mapName2CachedTagsData.begin();
    for (; iterTagData != m_mapName2CachedTagsData.end(); iterTagData++)
    {
        PKTAGDATA * pTagData = (PKTAGDATA *)iterTagData->second;
        SAFE_DELETE(pTagData);
    }
    m_mapName2CachedTagsData.clear();

    std::map<std::string, vector<PKTAG *>* >::iterator iterTagVec = m_mapAddr2TagVec.begin();
    for (; iterTagVec != m_mapAddr2TagVec.end(); iterTagVec++)
    {
		vector<PKTAG *> * pTagVec = (vector<PKTAG *> *)iterTagVec->second;
        pTagVec->clear();
        SAFE_DELETE(pTagVec);
    }
    m_mapAddr2TagVec.clear();

    // 删除设备连接！
	if (m_hPKCommunicate)
	{
		PKCommunicate_UnInitConn(m_hPKCommunicate); // 关闭连接
		m_hPKCommunicate = NULL;
	}

    for(int i =0; i < m_vecAllTag.size(); i ++)
    {
        delete m_vecAllTag[i];
        m_vecAllTag[i] = NULL;
    }


	PKDEVICE_Free(m_pOutDeviceInfo);
    //delete m_pOutDeviceInfo;
    m_pOutDeviceInfo = NULL;
}

// 定时轮询的周期到了
int CDevice::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{
    // 确保控制命令被优先处理，控制处理的主流程不在这里，在TaskGroup中
    if (m_pTaskGroup != NULL)
        m_pTaskGroup->HandleWriteCmd();

    // 检查是否需要重新连接。如果需要则直接重新连接
    int nRet = CheckAndConnect(100);
    return PK_SUCCESS;
}


CDevice & CDevice::operator=(CDevice &theDevice)
{
    CDrvObjectBase::operator=(theDevice);
    m_strConnType = theDevice.m_strConnType;	// 连接类型
    m_strConnParam = theDevice.m_strConnParam;	// 连接参数
    m_nConnTimeOut = theDevice.m_nConnTimeOut;	// 毫秒为单位
    m_nRecvTimeOut = theDevice.m_nRecvTimeOut;	// 毫秒为单位
    m_bMultiLink = theDevice.m_bMultiLink;	// 设备是否支持多连接
    m_strTaskID = theDevice.m_strTaskID;
    return *this;
}

// 本函数调用一定在自动计算相位之后
// 这个定时器是定时检查连接状态的定时器
void  CDevice::StartCheckConnectTimer()
{
    m_pTaskGroup->m_pReactor->cancel_timer((ACE_Event_Handler *)(CDevice *)this);

    // 设定设备上的定时器。由驱动自己决定（在InitDevice中）是否开启定时器

    // 将定时器作为检查连接的定时器
    int nCheckConnTimeout = m_nConnTimeOut * 2; // 2倍的连接超时，作为定时检查连接是否断开的依据

    if(nCheckConnTimeout <= 1000) // 小于1秒
        nCheckConnTimeout = 5000; // 缺省为5秒

    ACE_Time_Value	tvCheckConn;		// 扫描周期，单位ms
    tvCheckConn.msec(nCheckConnTimeout);
    ACE_Time_Value tvStart = ACE_Time_Value::zero;
    m_pTaskGroup->m_pReactor->schedule_timer((ACE_Event_Handler *)(CDevice *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "start device(%s)'timer to check connect, cycle:%d ms",this->m_strName.c_str(), nCheckConnTimeout);
}


void CDevice::CallInitDeviceFunc()
{
    if (g_nDrvLang == PK_LANG_CPP) // C++语言
    {
        // 调用初始化，驱动可能会去通过socket或串口连接设备，所以这个需要放在设备连接之后进行
        if (g_pfnInitDevice)
            g_pfnInitDevice(this->m_pOutDeviceInfo);
    }
    else
    {
		Py_InitDevice(this);
    }

    // 取得所有定时器的最小时间
    int nMinTimerPeriodMS = 0;
    for (vector<CUserTimer*>::iterator itTimer = m_vecTimers.begin(); itTimer != m_vecTimers.end(); itTimer++)
    {
        CUserTimer *pTimer = *itTimer;
        if (pTimer->m_timerInfo.nPeriodMS > nMinTimerPeriodMS)
            nMinTimerPeriodMS = pTimer->m_timerInfo.nPeriodMS;
    }
    // 调整定时间间隔，必须大于最小的定时器的5倍间隔！
    int nTimerDiscConfirmSeconds = ceil(nMinTimerPeriodMS * 5 / 1000.0f); // 5倍的最小定时器间隔！
    if (nTimerDiscConfirmSeconds > m_nDisconnectConfirmSeconds) // 自动计算时间，避免时间过于小
        m_nDisconnectConfirmSeconds = nTimerDiscConfirmSeconds;
}

int OnConnChanged(PKCONNECTHANDLE hConnect, int bConnected, char *szConnExtra, int nCallbackParam, void *pCallbackParam)
{
	CDevice *pDevice = (CDevice *)pCallbackParam;
	if (bConnected)
	{
		pDevice->SetConnectOK(); // 重新连上了socket，则认为连接状态OK
		pDevice->SetDeviceConnected(1);
	}
	else
		pDevice->SetDeviceConnected(0);

	if (g_pfnOnDeviceConnStateChanged) // 这是驱动导出函数
		g_pfnOnDeviceConnStateChanged(pDevice->m_pOutDeviceInfo, bConnected);
	return 0;
}

int CDevice::InitConnectToDevice()
{
	if (m_hPKCommunicate)
	{
		PKCommunicate_UnInitConn(m_hPKCommunicate);
		m_hPKCommunicate = NULL;
	}

	string strConnParam = m_strConnParam + ";";
	// ip=127.0.0.1;ip2=192.168.1.1;port=102;multiLink=1
	char *szConnType = (char *)m_strConnType.c_str();
	if (ACE_OS::strcasecmp(szConnType, "tcpclient") == 0)
	{
		m_nConnectType = PK_COMMUNICATE_TYPE_TCPCLIENT;
	}
	else if (ACE_OS::strcasecmp(szConnType, "serial") == 0)
	{
		m_nConnectType = PK_COMMUNICATE_TYPE_SERIAL;
	}
	else if (ACE_OS::strcasecmp(szConnType, "udpserver") == 0 || ACE_OS::strcasecmp(szConnType, "udp") == 0)
	{
		m_nConnectType = PK_COMMUNICATE_TYPE_UDP;
	}
	else if (ACE_OS::strcasecmp(szConnType, "tcpserver") == 0) // ip=127.0.0.1;ip=127.0.0.1/192.168.1.1;port=102;
	{
		m_nConnectType = PK_COMMUNICATE_TYPE_TCPSERVER;
	}
	else
	{
		m_nConnectType = PK_COMMUNICATE_TYPE_NOTSUPPORT;
		m_hPKCommunicate = NULL;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "您采用了其他连接方式，通信类型：%s,参数 %s", m_strConnType.c_str(), m_strConnParam.c_str());
	}

	if (m_nConnectType != PK_COMMUNICATE_TYPE_NOTSUPPORT)
	{
		m_hPKCommunicate = PKCommunicate_InitConn(m_nConnectType, strConnParam.c_str(), m_strName.c_str(), OnConnChanged, 0, this);
		PKCommunicate_EnableConnect(m_hPKCommunicate, 1);
		GetMultiLink((char *)strConnParam.c_str());
	}

	return 0;
}

void CDevice::Start()
{
	InitConnectToDevice(); // 这里面没有建立连接

	// 调用初始化，驱动可能会去通过socket或串口连接设备，所以这个需要放在设备连接之后进行
	StartCheckConnectTimer();

	CallInitDeviceFunc();

	// 调用初始化完毕后, 这里应该尝试开始建立连接或者打开侦听端口
	if (m_hPKCommunicate)
	{
		PKCommunicate_Connect(m_hPKCommunicate, 100);
	}
}

void CDevice::Stop()
{
    m_pTaskGroup->m_pReactor->cancel_timer(this);

    // 停止各个数据块的定时器
    std::vector<CUserTimer*>::iterator iterBlk = m_vecTimers.begin();
    for (; iterBlk != m_vecTimers.end(); iterBlk++)
    {
        CUserTimer *pTimer = *iterBlk;
        pTimer->StopTimer();
    }

    g_logger.LogMessage(PK_LOGLEVEL_INFO, "%s will call UnitInitDevice!", m_strName.c_str());
    // 必须在断开连接前先UnInitDevice，避免还需要使用连接!
    if(g_pfnUnInitDevice)
        g_pfnUnInitDevice(this->m_pOutDeviceInfo);
	Py_UnInitDevice(this); // python版本的该函数实现

	if (m_hPKCommunicate)
	{
		PKCommunicate_UnInitConn(m_hPKCommunicate);
		m_hPKCommunicate = NULL;
	}
}

// 处理设备和系统的byteorder不一致的情况
void CDevice::processByteOrder(PKTAG *pTag, char *szBinData, int nBinLen)
{
	if (g_bSysBigEndian == this->m_bDeviceBigEndian)
	{
		// 应用级的字节序处理
		CByteOrder::SwapBytes(pTag->nByteOrder, (unsigned char *)szBinData, nBinLen, pTag->nDataType);
		return;
	}

    if (pTag->nLenBits > 8 && pTag->nDataType != TAG_DT_BLOB && pTag->nDataType != TAG_DT_TEXT)
    {
        int nTypeLen = pTag->nLenBits / 8;
        if (nTypeLen > nBinLen)
        {
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, when converting byteorder, given data len(%d) < datatype len(%d)!", pTag->szName, nBinLen, nTypeLen);
            return;
        }
        for (int II = 0; II < nTypeLen / 2; ++II)
        {
            char chTmp = szBinData[nTypeLen - II - 1];
            szBinData[nTypeLen - II - 1] = szBinData[II];
            szBinData[II] = chTmp;
        }
    }

	// 应用级的字节序处理
	CByteOrder::SwapBytes(pTag->nByteOrder, (unsigned char *)szBinData, nBinLen, pTag->nDataType);
}

bool CDevice::GetEnableConnect()	// 返回是否允许连接
{
	return m_pTagEnableConnect->szData[0] != 0 ?true : false;
}

void CDevice::SetEnableConnect(bool bEnableConnect)	// 设置是否允许连接
{
	m_pTagEnableConnect->szData[0] = bEnableConnect ? 1 : 0;
	PKCommunicate_EnableConnect(m_hPKCommunicate, bEnableConnect);

	m_pOutDeviceInfo->bEnableConnect = bEnableConnect; // 设置驱动里面PKDEVICE的允许连接属性
	if (!bEnableConnect)
	{
		// 所有点更新为**号
		for (int i = 0; i < this->m_vecAllTag.size(); i++)
		{
			PKTAG *pTag = m_vecAllTag[i];
			Drv_SetTagData_Text(pTag, TAG_QUALITY_COMMUNICATE_FAILURE_STRING, 0, 0, TAG_QUALITY_COMMUNICATE_FAILURE);
		}
		UpdateTagsData(m_vecAllTag.data(), m_vecAllTag.size());
	}
}

void CDevice::OnWriteCommand(PKTAG *pTag, PKTAGDATA *pTagData)
{
    processByteOrder(pTag, pTagData->szData, pTag->nLenBits / 8);
	if (pTag == this->m_pTagEnableConnect)
	{
		bool bEnableConnect = pTagData->szData[0];
		this->SetEnableConnect(bEnableConnect);

		// 将值发送到nodeserver
		short nVal = bEnableConnect ? 1 : 0;
		char szValue[32];
		sprintf(szValue, "%d", nVal);
		Drv_SetTagData_Text(pTag, szValue, 0, 0, 0);
		vector<PKTAG *> vecTags;
		vecTags.push_back(pTag);
		UpdateTagsData(vecTags.data(), vecTags.size());
		vecTags.clear();

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "接收到enableconnect的命令：device:%s, tagname:%s, addr[%s], value:%d", m_strName.c_str(), pTag->szName, pTag->szAddress, bEnableConnect);
		return;
	}

	string strValue;
	TagValFromBin2String(g_bSysBigEndian, pTag->nDataType, pTagData->szData, pTagData->nDataLen, 0, strValue);
	if (g_nDrvLang == PK_LANG_PYTHON)
	{
		int nRet = Py_OnControl(this, pTag, strValue); // PK_LANG_PYTHON
		return;
	}

	if (g_nDrvLang != PK_LANG_CPP)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Device::OnWriteCommand, NO DRIVER!!! device:%s, tagname:%s, addr[%s],value:%s",
			m_strName.c_str(), pTag->szName, pTag->szAddress, strValue.c_str());
		return;
	}

    int lCmdId = 0;
	if (!g_pfnOnControl)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Device::OnWriteCommand, cannot find OnControl function, Fail to control：device:%s, tag:%s, value:%s",
			m_strName.c_str(), pTag->szName, strValue.c_str());
		return;
	} 

    int lStatus = g_pfnOnControl(this->m_pOutDeviceInfo, pTag, strValue.c_str(), lCmdId);
    if (lStatus == PK_SUCCESS)
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "success to control：device:%s, tagname:%s, addr:%s, value:%s",
        m_strName.c_str(), pTag->szName, pTag->szAddress, strValue.c_str());
    else
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "fail to control：device:%s, tag:%s, addr:%s, value:%s, ret:%d",
        m_strName.c_str(), pTag->szName, pTag->szAddress, strValue.c_str(), lStatus);
}

int CDevice::SendToDevice(const char *szBuffer, int nBufLen, int nTimeOutMS)
{
	//printf("SendToDevice 7111, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());
    // 在线部署后发现设备信息发生改变，需要重新连接设备
    if (!m_bMultiLink && !IsActiveHost())
    {
        g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "向设备发送数据失败(发送 %d 个字节，超时 %d ), 本设备是单连接设备，且本机为备机",
            nBufLen, nTimeOutMS);
        return -1;
    }
	//printf("SendToDevice 7112, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());

	// TODO 该设备配置发生了改变。连的有点晚了？应该在收到通知时就进行连接切换
	if (m_bConfigChanged) // 该设备配置发生了改变。连的有点晚了？
	{
		//printf("SendToDevice 7113, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());
		m_bConfigChanged = false;

		// 重新建立连接！但不需要InitDevice
		InitConnectToDevice();
		StartCheckConnectTimer(); // 重新启动检车连接的定时器

	} // m_bConfigChanged

	//printf("SendToDevice 7114, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());
	if (m_hPKCommunicate)
	{
		//printf("SendToDevice 7115, buffer:0x%x len:%d, %s,conn:0x%x\n", szBuffer, nBufLen, m_strName.c_str(), (int)m_hPKCommunicate);
		int nRet = PKCommunicate_Send(m_hPKCommunicate, szBuffer, nBufLen, nTimeOutMS);
		//printf("SendToDevice 7116, buffer:0x%x len:%d, %s,conn:0x%x\n", szBuffer, nBufLen, m_strName.c_str(), (int)m_hPKCommunicate);
		return nRet;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "向设备:%s 发送数据失败(要发送 %d 个字节，超时 %d ), m_hPKCommunicate == NULL 请检查是否连接或配置的连接串是否正确",
			m_strName.c_str(), nBufLen, nTimeOutMS);
		return -35;
	}
}

int CDevice::RecvFromDevice(char *szBuffer, int nBufLen, int nTimeOutMS)
{
    if (!m_hPKCommunicate)
    {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "从设备:%s 接收数据失败(接收缓冲区长度 %d 个字节，超时 %d ), m_hPKCommunicate == NULL请检查是否已连接或配置的连接串是否正确",
            m_strName.c_str(), nBufLen, nTimeOutMS);
        return -1;
    }

	int nRecv = PKCommunicate_Recv(m_hPKCommunicate, szBuffer, nBufLen, nTimeOutMS);
	if (nRecv > 0)
		SetConnectOK();
	return nRecv;
}

void CDevice::ClearRecvBuffer()
{
	PKCommunicate_ClearRecvBuffer(m_hPKCommunicate);
}


// 连接参数，如果是tcp则包括:IP,Port,可选的连接超时。以后考虑可以有串口
// 这里一定是tcpclient连接方式。形式: IP:192.168.1.5;Port:502;ConnTimeOut:3000
int CDevice::GetMultiLink(char *szConnParam)
{
    m_bMultiLink = true;
    string strConnParam = szConnParam;
    //g_logger.LogMessage(PK_LOGLEVEL_INFO, "设置设备连接参数 %s", szConnParam);
    strConnParam += ";"; // 补上一个分号
    int nPos = strConnParam.find(';');
    while (nPos != string::npos)
    {
        string strOneParam = strConnParam.substr(0, nPos);
        strConnParam = strConnParam.substr(nPos + 1);// 除去这一个已经解析过的参数
        nPos = strConnParam.find(';');

        if (strOneParam.empty())
            continue;

        int nPosPart = strOneParam.find('=');	// IP:168.2.8.75
        if (nPosPart == string::npos)
            continue;

        // 获取到某个参数名称和值
        string strParamName = strOneParam.substr(0, nPosPart); // e.g. IP
        string strParamValue = strOneParam.substr(nPosPart + 1); // e.g. 168.2.8.75

        if (ACE_OS::strcasecmp("multilink", strParamName.c_str()) == 0)
        {
            m_bMultiLink = ::atoi(strParamValue.c_str());
            break;
        }
    }

    return PK_SUCCESS;
}

int CDevice::GetCachedTagData(PKTAG *pTag, char *szValue, int nValueBufLen, int *pnRetValueLen, unsigned int *pnTimeSec, unsigned short *pnTimeMilSec, int *pnQuality)
{
	std::map<string, PKTAGDATA *>::iterator itTagMap = m_mapName2CachedTagsData.find(pTag->szName);
    if (itTagMap == m_mapName2CachedTagsData.end())
    {
		if (pnRetValueLen)
			*pnRetValueLen = 0;
		if (pnTimeSec)
			*pnTimeSec = 0;
		if (pnTimeMilSec)
			*pnTimeMilSec = 0;
		if (pnQuality)
			*pnQuality = -111;
        return -111;
    }

	PKTAGDATA *pTagData = itTagMap->second;

	if (pnTimeSec)
		*pnTimeSec = pTagData->nTimeSec;
	if (pnTimeMilSec)
		*pnTimeMilSec = pTagData->nTimeMilSec;
	if (pnQuality) 
		*pnQuality = pTagData->nQuality;

	if (pTag->nDataType == TAG_DT_BLOB)
	{
		if (nValueBufLen > pTagData->nDataLen)
			nValueBufLen = pTagData->nDataLen;

		if (pnRetValueLen)
			*pnRetValueLen = nValueBufLen;
		memcpy(szValue, pTagData->szData, nValueBufLen); // 复制的是原始数据！需要转换为字符串
	}
	else
	{
		string strTagValue;
		TagValFromBin2String(false, pTag->nDataType, pTagData->szData, pTagData->nDataLen, 0, strTagValue);
		PKStringHelper::Safe_StrNCpy(szValue, strTagValue.c_str(), nValueBufLen);
		if (pnRetValueLen)
			*pnRetValueLen = strlen(szValue);
	}
    return 0;
}

CUserTimer * CDevice::CreateAndStartTimer(PKTIMER * timerInfo)
{
    CUserTimer *pBlockTimer = NULL;
    pBlockTimer = new CUserTimer(this);
    memcpy(&pBlockTimer->m_timerInfo, timerInfo, sizeof(PKTIMER));
    pBlockTimer->m_timerInfo._pInternalRef = pBlockTimer;

    m_vecTimers.push_back(pBlockTimer);
    pBlockTimer->StartTimer();
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "(CreateAndStartTimer)timer started: device:%s, start driver timer cycle=%d MS", this->m_strName.c_str(), timerInfo->nPeriodMS);
    return pBlockTimer;
}

PKTIMER * CDevice::GetTimers(int * pnTimerNum)
{
    if (m_nTimerNum != m_vecTimers.size() || m_nTimerNum == 0)
    {
        if (m_pTimers)
            SAFE_DELETE_ARRAY(m_pTimers);

        m_nTimerNum = m_vecTimers.size();
        m_pTimers = new PKTIMER[m_nTimerNum];
        for (int i = 0; i < m_vecTimers.size(); i++)
        {
            CUserTimer* pBlockTimer = m_vecTimers[i];
            memcpy(&m_pTimers[i], &(pBlockTimer->m_timerInfo), sizeof(PKTIMER));
        }
    }
    *pnTimerNum = m_nTimerNum;
    return m_pTimers;
}

void CDevice::DestroyTimers()
{
    m_pTaskGroup->m_pReactor->cancel_timer(this);

    // 停止各个数据块的定时器
    std::vector<CUserTimer*>::iterator iterBlk = m_vecTimers.begin();
    for (; iterBlk != m_vecTimers.end(); iterBlk++)
    {
        CUserTimer *pTimer = *iterBlk;
        pTimer->StopTimer();
    }
}
int CDevice::StopAndDestroyTimer(CUserTimer * pBlockTimer)
{
    pBlockTimer->StopTimer();
    delete pBlockTimer;
    return 0;
}

// 为空表示tag无时间戳
char * CDevice::GetTagTimeString(PKTAG *pTag)
{
    if (pTag->nTimeSec == 0)
        return NULL;

    int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;
    time_t tmTmp = pTag->nTimeSec;
    struct tm *m_tm = gmtime(&tmTmp);
    year = m_tm->tm_year + 1900;
    mon = m_tm->tm_mon + 1;
    day = m_tm->tm_mday;
    hour = m_tm->tm_hour;
    min = m_tm->tm_min;
    sec = m_tm->tm_sec;
    sprintf(m_szTmpTagTimeWithMS, "%04d-%02d-%02d %d:%d:%d.%03d", year, mon, day, hour, min, sec, pTag->nTimeMilSec);
    return m_szTmpTagTimeWithMS;
}

// 得到tag点的信息缓冲区，以便后续发给NodeServer
// 返回缓冲区长度>=0有效
int CDevice::GetTagDataBuffer(PKTAG *pTag, char *szTagBuffer, int nBufLen, unsigned int nCurSec, unsigned int nCurMSec)
{
	int nTagDataLen = pTag->nDataLen; // 进行截断！！！！
	if (nBufLen < sizeof(int) * 5 + sizeof(short) + pTag->nDataLen)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "GetTagDataBuffer(%s), taglen:%d > bufferlen:%d + sizeof(int) * 5 + sizeof(short) + pTag->nDataLen", pTag->szName, pTag->nDataLen, nBufLen);
		nTagDataLen = sizeof(int) * 5 + sizeof(short) + pTag->nDataLen - nBufLen;
	}

    char *szTagName = pTag->szName;
    int nDataType = pTag->nDataType;
    int nSec = nCurSec;
    int nMSec = nCurMSec;
    // 如果时间为空则返回当前时间
    if (pTag->nTimeSec > 0)
    {
        nSec = pTag->nTimeSec;
        nMSec = pTag->nTimeMilSec;
    }

    char *pData = szTagBuffer;

    // tag名
    int nLen = strlen(szTagName);
    memcpy(pData, &nLen, sizeof(int));
    pData += sizeof(int);
    memcpy(pData, szTagName, nLen);
    pData += nLen;

    // tag数据类型
    memcpy(pData, &nDataType, sizeof(int));
    pData += sizeof(int);

    // tag值的长度和tag值内容
    memcpy(pData, &nTagDataLen, sizeof(int));
    pData += sizeof(int);
    if (nTagDataLen > 0)
    {
        memcpy(pData, pTag->szData, nTagDataLen);
		CByteOrder::SwapBytes(pTag->nByteOrder, (unsigned char *)pData, nTagDataLen, pTag->nDataType); // 应用级的字节序处理
        pData += nTagDataLen;
    }

    // tag秒和毫秒数
    memcpy(pData, &nSec, sizeof(unsigned int));
    pData += sizeof(int);
    memcpy(pData, &nMSec, sizeof(unsigned short));
	pData += sizeof(short);

    // tag质量
    memcpy(pData, &pTag->nQuality, sizeof(int));
    pData += sizeof(int);

    return pData - szTagBuffer;
}

int CDevice::UpdateTagsData(PKTAG **ppTags, int nTagNum)
{
	if (nTagNum <= 0)
        return 0;

    CProcessQueue *pQue = g_pQueData2NodeServer;
    if (!pQue)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver:%s,device:%s, On updateTagsData, cannot get queue:data2NodeServer, tag count:%d,discard...",
			g_strDrvName.c_str(), m_strName.c_str(), nTagNum);
        return -1;
    }

    // 取得时间信息
    unsigned int nCurSec = 0;
    unsigned int nCurMSec = 0;
    nCurSec = PKTimeHelper::GetHighResTime(&nCurMSec);

    int nRet = 0;
    int nFailCount = 0;
	char *szQueRecBuffer = m_pTaskGroup->m_pQueRecBuffer; /// [PROCESSQUEUE_DEFAULT_RECORD_LENGHT] = { 0 }; //每个Que的一个记录的大小
    char *pQueBufP = szQueRecBuffer;
    char *pQueBufEnd = szQueRecBuffer + m_pTaskGroup->m_nQueRecBufferLen; // 指针尾部

    // 放入时间戳
    memcpy(pQueBufP, &nCurSec, sizeof(unsigned int));
    pQueBufP += sizeof(unsigned int);
    memcpy(pQueBufP, &nCurMSec, sizeof(unsigned short));
	pQueBufP += sizeof(unsigned short);

    // 动作类型
    int nActionType = ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ;
    memcpy(pQueBufP, &nActionType, sizeof(nActionType));
    pQueBufP += sizeof(int); // 跳过acitontype

	char *pTagCount = pQueBufP;
    pQueBufP += sizeof(int); // 跳过tag个数,之再赋值后
    int nTagNumThisPack = 0;
    for (int iTag = 0; iTag < nTagNum; iTag++)
    {
		PKTAG *pTag = ppTags[iTag];
        if (pTag->nTimeSec == 0 && pTag->nTimeMilSec == 0)
        {
            pTag->nTimeSec = nCurSec;
            pTag->nTimeMilSec = nCurMSec;
        }

        if (pTag->nQuality == PK_SUCCESS && pTag->nDataLen > 0) // 更新值
        {
            // 将二进制的tagdata转换为字符串类型
            TagValBuf_ConvertEndian(this->m_bDeviceBigEndian, pTag->szName, pTag->nDataType, pTag->szData, pTag->nDataLen, 0);
        }

        // 更新到本地的缓存中，以便将来驱动需要时可以快速获取
        PKTAGDATA *pNewTagData = NULL;
        std::map<string, PKTAGDATA *>::iterator itTagMap = m_mapName2CachedTagsData.find(pTag->szName);
        if (itTagMap == m_mapName2CachedTagsData.end())
        {
            pNewTagData = new PKTAGDATA();
            strcpy(pNewTagData->szName, pTag->szName);
            pNewTagData->nDataType = pTag->nDataType;
			pNewTagData->nDataLen = pTag->nDataLen;
			pNewTagData->szData = new char[pNewTagData->nDataLen + 1];
			memset(pNewTagData->szData, 0, pNewTagData->nDataLen + 1);

            m_mapName2CachedTagsData.insert(std::make_pair(pTag->szName, pNewTagData));
        }
        else
        {
            pNewTagData = itTagMap->second;
        }
        pNewTagData->nDataLen = pTag->nDataLen;
        pNewTagData->nQuality = pTag->nQuality;
		pNewTagData->nTimeSec = pTag->nTimeSec;
		pNewTagData->nTimeMilSec = pTag->nTimeMilSec;
		memcpy(pNewTagData->szData, pTag->szData, pNewTagData->nDataLen);
        char *szOneTagBuf = m_pTaskGroup->m_pTagVTQBuffer; //每个Tag的一个记录的大小,内容最大4K,还是加上一些时间戳等信息
        int nOneTagBufLen = GetTagDataBuffer(pTag, szOneTagBuf, m_pTaskGroup->m_nTagVTQBufferLen, nCurSec, nCurMSec);
        if (nOneTagBufLen + pQueBufP > pQueBufEnd) // 一次放不下了，需要先发送走
        {
			memcpy(pTagCount, &nTagNumThisPack, sizeof(int)); // 更新tag个数
            int nRet = pQue->EnQueue(szQueRecBuffer, pQueBufP - szQueRecBuffer);
            if (nRet == PK_SUCCESS)
            {
                g_logger.LogMessage(PK_LOGLEVEL_DEBUG, ("Driver[%s] send tagdata to NodeServer success, tagcount:%d, len:%d"), g_strDrvName.c_str(), nTagNumThisPack, pQueBufP - szQueRecBuffer);
            }
            else
            {
                g_logger.LogMessage(PK_LOGLEVEL_ERROR, ("Driver[%s] send tagdata to NodeServer failed, type:%d, len:%d, ret:%d"), g_strDrvName.c_str(), nActionType, pQueBufP - szQueRecBuffer, nRet);
                nFailCount += nTagNumThisPack;
            }
            // 重新开始
            nTagNumThisPack = 0;
            pQueBufP = szQueRecBuffer;
            pQueBufP += sizeof(unsigned int); // 跳过时间戳秒
            pQueBufP += sizeof(unsigned short); // 跳过时间戳毫秒
            pQueBufP += sizeof(int); // 跳过actiontype

            // 当前最后一个点尚未发送，需要先放进去
            nTagNumThisPack++;
			pQueBufP += sizeof(int); // 跳过tag个数
			memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);

            pQueBufP += nOneTagBufLen;
        }
        else // 缓冲区还没放完，可放在包中一起发送
        {
            nTagNumThisPack++;
            memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);
            pQueBufP += nOneTagBufLen;
        }
    }

    // 发送最后一个包，如果有的话
    if (pQueBufP - szQueRecBuffer > sizeof(int) * 2)
    {
		memcpy(pTagCount, &nTagNumThisPack, sizeof(int)); // 更新tag个数,前面有时间戳和ActionType
        int nRet = pQue->EnQueue(szQueRecBuffer, pQueBufP - szQueRecBuffer);
        if (nRet == PK_SUCCESS)
        {
            g_logger.LogMessage(PK_LOGLEVEL_DEBUG, ("Driver[%s] send tagdata to NodeServer success, tagcount:%d, len:%d"), g_strDrvName.c_str(), nTagNumThisPack, pQueBufP - szQueRecBuffer);
        }
        else
        {
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, ("Driver[%s] send tagdata to NodeServer failed, type:%d, len:%d, ret:%d"), g_strDrvName.c_str(), nActionType, pQueBufP - szQueRecBuffer, nRet);
            nFailCount += nTagNumThisPack;
        }
    }

    if (nFailCount > 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "dev:%s, task:%s, uptagcount:%d, fail:%d", this->m_strName.c_str(), this->m_strTaskID.c_str(), nTagNumThisPack, nFailCount);
    else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "dev:%s, task:%s, uptagcount:%d, success:%d", this->m_strName.c_str(), this->m_strTaskID.c_str(), nTagNumThisPack, nTagNumThisPack - nFailCount);
    return 0;
}

// 为每个设备增加所有的tag点击对应的状态位点
int CDevice::AddTags2InitDataShm()
{
    int nRet = 0;
    char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    std::map<std::string, PKTAG *>::iterator itTag = m_mapName2Tags.begin();
    for (; itTag != m_mapName2Tags.end(); itTag++)
    {
        PKTAG *pTag = (PKTAG *)itTag->second;
        char szTagVTQ[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH]; // 这个字符串不会很大
        sprintf(szTagVTQ, "{\"v\":\"\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"}", szTime, CONN_STATUS_INIT_CODE, STATUS_INIT_TEXT);
        //int nRetOne = PublishTag2NodeServer2(ACTIONTYPE_DRV2NodeServer_SET_KV_VTQ, pTag, 0, NULL, 0, 0, STATUS_INIT_CODE, STATUS_INIT_TEXT);
        int nRetOne = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, pTag->szName, szTagVTQ, strlen(szTagVTQ));
        if (nRetOne != 0)
        {
            nRet = nRetOne;
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pdb:add tag(name:%s,value:%s) to device(name:%s) failed, ret:%d", pTag->szName, szTagVTQ, this->m_strName.c_str(), nRet);
        }
    }

    return nRet;
}

int CDevice::InitShmDeviceInfo()
{
    char *szTaskName = (char *)this->m_pTaskGroup->m_drvObjBase.m_strName.c_str();
    char *szDeviceName = (char *)m_strName.c_str();
    char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    int nRet = PK_SUCCESS;

	nRet = this->SetDeviceConnected(0); // 缺省认为是未连接上。如果是moxa卡，怎么算连上了呢？
    nRet = AddTags2InitDataShm();

    // 更新设备的task到shm（线程）
    char szKey[PK_NAME_MAXLEN * 6] = { 0 };
    PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:%s:tasks", g_strDrvName.c_str(), szDeviceName);
    nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV, szKey, szTaskName, this->m_pTaskGroup->m_drvObjBase.m_strName.length());

    return nRet;
}

int CDevice::ReConnectDevice(int nTimeOutMS)
{
	// 如果不是内置的支持通讯方式，进入这里就认为设备连接上了
	DisconnectFromDevice();
	int nRet = CheckAndConnect(nTimeOutMS);
	return nRet;
}

int CDevice::DisconnectFromDevice()
{
	if (m_nConnectType == PK_COMMUNICATE_TYPE_NOTSUPPORT)
	{
		this->SetDeviceConnected(false);
		return 0;
	}
	else
	{
		int nRet = PKCommunicate_Disconnect(m_hPKCommunicate);
		return nRet;
	}
}

int CDevice::CheckAndConnect(int nTimeOutMS)
{
	int nConnected = 1; // 
	if (m_nConnectType != PK_COMMUNICATE_TYPE_NOTSUPPORT) // 内置的通讯方式，包括TcpClient、TcpServer、Serial、Udp
	{
		int nConnected = PKCommunicate_IsConnected(m_hPKCommunicate);
		if (nConnected == 0) // 如果未连接则尝试连接直接返回
		{
			int nRet = PKCommunicate_Connect(m_hPKCommunicate, nTimeOutMS);
			nConnected = PKCommunicate_IsConnected(m_hPKCommunicate);
		}

		if (nConnected == 0) // 如果未连接则质量肯定未BAD，不需要考虑
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CheckAndConnect failed to connect to device:%s,conntype:%s,connparam:%s!", this->m_strName.c_str(), this->m_strConnType.c_str(), this->m_strConnParam.c_str());
			SetDeviceConnected(0); // 设置设备未连接上
			return 0;
		}
	}

	// 下面就认为是连接上了（1，内置通讯连接上，包括serial、tcpserver、tcpclient、udp，或者2，非内置通讯方式如DB、pkmqtt、文件等）
	// 检查上次是否收到数据，如果收到数据则认为通讯是OK的
	ACE_Time_Value tvNow = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvNow - m_tvLastConnOK;
	ACE_UINT64 nMSec = tvSpan.get_msec();
	// string strDeviceConnStatus = "1";
	if (m_nConnStatusTimeoutMS > 0 && nMSec > m_nConnStatusTimeoutMS) // 配置了通讯超时，且超时未收到数据则认为设备未连接
	{
		// strDeviceConnStatus = "0";
		nConnected = 0;
	}
	else
		nConnected = 1;

	SetDeviceConnected(nConnected); // 设置设备连接上了

	// Drv_SetTagData_Text(m_pTagConnStatus, strDeviceConnStatus.c_str(), 0, 0, 0);

	return 0; 
}

// 将设备信息拷贝到驱动所用的数据结构上去
void CDevice::CopyDeviceInfo2OutDevice()
{
    if (m_pTaskGroup)
        m_pOutDeviceInfo->pDriver = m_pTaskGroup->m_pMainTask->m_driverInfo.m_pOutDriverInfo;
    else
        m_pOutDeviceInfo->pDriver = NULL;

    // 拷贝C语言驱动所用的数据结构.仅仅是指向内存首地址
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szName, m_strName.c_str(), PK_NAME_MAXLEN);
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szDesc, m_strDesc.c_str(), PK_DESC_MAXLEN);
    m_pOutDeviceInfo->nRecvTimeout = m_nRecvTimeOut;
    m_pOutDeviceInfo->nConnTimeout = m_nConnTimeOut;
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szConnType, m_strConnType.c_str(), PK_NAME_MAXLEN);
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szConnParam, m_strConnParam.c_str(), PK_PARAM_MAXLEN);

	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szParam1, m_strParam1.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szParam2, m_strParam2.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szParam3, m_strParam3.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy(m_pOutDeviceInfo->szParam4, m_strParam4.c_str(), PK_PARAM_MAXLEN);

	if (m_pOutDeviceInfo->ppTags)
		delete[] m_pOutDeviceInfo->ppTags;

	m_pOutDeviceInfo->nTagNum = m_vecAllTag.size();
	if (m_vecAllTag.size() > 0)
	{
		m_pOutDeviceInfo->ppTags = new PKTAG*[m_vecAllTag.size()];
		for (int i = 0; i < m_vecAllTag.size(); i++)
			m_pOutDeviceInfo->ppTags[i] = m_vecAllTag[i]; //m_pTags;
	}
	Py_CopyDeviceInfo2OutDevice(this);
}

int CDevice::SetDeviceConnected(bool bDevConnected)
{
    bool bChangedStatus = false;
    //bool bDisconnConfirmed = false;
    if (!bDevConnected)
    {
        time_t tmNow;
        time(&tmNow);
        if (m_bLastConnStatus)// 以前连上了,现在没连上,1--->0
        {
            if (m_tmLastDisconnect == 0) // 以前没有断开
                m_tmLastDisconnect = tmNow; // 初次发现断开时间
            else // 以前记录有时间，已经做了断开标识
            {
                int nDiscTimeSpan = tmNow - m_tmLastDisconnect; // 和上次为连接上的差值
                if (nDiscTimeSpan > m_nDisconnectConfirmSeconds)
                {
                    bChangedStatus = true;
                    //bDisconnConfirmed = true; // 确认断开连接了
                    m_bLastConnStatus = false; // 确认之前状态为未连接
                }
            }
        }
    }
    else // 现在连上了
    {
        if (!m_bLastConnStatus)   // 以前没连上，现在连上了
        {
            bChangedStatus = true;
            m_bLastConnStatus = true; // 上次连接状态
        }
        m_tmLastDisconnect = 0; // 标识之前一直处于连接状态
    }

    // 所有tag点，除了连接状态这些点外，都应该为*
    if (bChangedStatus || (!m_bLastConnStatus && !bDevConnected)) // 状态改变，或者之前和现在都未连接上，进行刷新
	{
		vector<PKTAG*> vecTags;
        // 增加非连接状态的点到更新集合中
        for (int i = 0; i < m_vecAllTag.size(); i++)
        {
			PKTAG *pTag = m_vecAllTag[i];
            if (!bDevConnected) // 断开连接重置，连接则不处理
            {
				Drv_SetTagData_Text(pTag, TAG_QUALITY_COMMUNICATE_FAILURE_STRING, 0, 0, TAG_QUALITY_COMMUNICATE_FAILURE);
				vecTags.push_back(pTag);
			}
        }

        // 增加连接状态的点到更新集合中
        if (bDevConnected)
        {
            Drv_SetTagData_Text(m_pTagConnStatus, "1", 0, 0, 0);
        }
        else
        {
			Drv_SetTagData_Text(m_pTagConnStatus, "0", 0, 0, 0);
        }
		vecTags.push_back(m_pTagConnStatus);

		UpdateTagsData(vecTags.data(), vecTags.size());
        vecTags.clear();
    }
    else
    {
        // 正常情况下，仅仅更新链接状态的点
        vector<PKTAG *> vecTags;
        if (bDevConnected)
        {
			Drv_SetTagData_Text(m_pTagConnStatus, "1", 0, 0, 0);
        }
        else
        {
			Drv_SetTagData_Text(m_pTagConnStatus, "0", 0, 0, 0);
        }
		vecTags.push_back(m_pTagConnStatus);

        UpdateTagsData(vecTags.data(), vecTags.size());
        vecTags.clear();
    }

    return 0;
}

// ppTags是PKTAG *数组，是传入前应该预先分配好的
int CDevice::GetTagsByAddr(const char * szTagAddress, PKTAG **ppTags, int nMaxTagNum)
{ 
    vector<PKTAG *> *pVecTagOfAddr = NULL;
    map<string, vector<PKTAG *> *>::iterator itMapT = m_mapAddr2TagVec.find(szTagAddress);
    if (itMapT == m_mapAddr2TagVec.end())
    {
        return 0;
    }

    pVecTagOfAddr = itMapT->second;
	int nTagNum = 0;
    for (int i = 0; i < pVecTagOfAddr->size() && i < nMaxTagNum; i++)
    {
        PKTAG *pTag = (*pVecTagOfAddr)[i];
		ppTags[i] = pTag;
		nTagNum++;
    }
	return nTagNum;
}

// 自动计算判断通讯断开的超时
int CDevice::SetConnectOKTimeout(int nConnBadTimeooutMS)
{
	m_nConnStatusTimeoutMS = nConnBadTimeooutMS;
	m_tvLastConnOK = ACE_Time_Value::zero;
	return 0;
}

// 收到数据时调用1次该接口
void  CDevice::SetConnectOK()
{
	m_tvLastConnOK = ACE_OS::gettimeofday();
}
