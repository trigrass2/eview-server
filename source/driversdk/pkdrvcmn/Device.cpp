/**************************************************************
*  Filename:    Device.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �豸��Ϣʵ����.
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
// szStrData��szBinData����ָ��ͬһƬ�ڴ�����
extern int TagValFromString2Bin(PKTAG *pTag, int nTagDataType, const char *szStrData, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBits);
extern bool IsActiveHost();
extern CProcessQueue *g_pQueData2NodeServer;

extern void PKDEVICE_Reset(PKDEVICE *pDevice);
extern void PKDEVICE_Free(PKDEVICE *pDevice);

/**
*  ���캯��.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CDevice::CDevice(CTaskGroup *pTaskGrp, const char *pszDeviceName) :m_pTaskGroup(pTaskGrp)
{
    // �豸�Ƿ�֧�ֶ�����
    m_bMultiLink = false;
    m_pTimers = NULL;
    m_nTimerNum = 0;

    m_strName = "";
    m_strName = pszDeviceName;
    //memset(m_szTmpTagValue, 0, sizeof(m_szTmpTagValue));
    memset(m_szTmpTagTimeWithMS, 0, sizeof(m_szTmpTagTimeWithMS));

    m_strDesc = "";
	m_bConfigChanged = false; // ��һ�������ӣ������Ϊtrue�����ӡ��رա�������
    m_tvTimerBegin = CVLibHead::gettimeofday_tickcnt();

	m_hPKCommunicate = NULL;
    m_strTaskID = "";
    m_pOutDeviceInfo = new PKDEVICE();
	PKDEVICE_Reset(m_pOutDeviceInfo);
    m_pOutDeviceInfo->_pInternalRef = (void *)this;
    m_bDeviceBigEndian = false; // ȱʡΪС�������⼴Ϊ����

    m_nConnCount = 0; // ���Ӵ���
    memset(m_szLastConnTime, 0, sizeof(m_szLastConnTime)); // �ϴ�����ʱ��

    m_bLastConnStatus = false;
    m_tmLastDisconnect = 0; // 0��ʾδ�Ͽ�
    m_nDisconnectConfirmSeconds = 10; // ȱʡΪ10��
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
*  ��������.
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

    // ɾ���豸���ӣ�
	if (m_hPKCommunicate)
	{
		PKCommunicate_UnInitConn(m_hPKCommunicate); // �ر�����
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

// ��ʱ��ѯ�����ڵ���
int CDevice::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{
    // ȷ������������ȴ������ƴ���������̲��������TaskGroup��
    if (m_pTaskGroup != NULL)
        m_pTaskGroup->HandleWriteCmd();

    // ����Ƿ���Ҫ�������ӡ������Ҫ��ֱ����������
    int nRet = CheckAndConnect(100);
    return PK_SUCCESS;
}


CDevice & CDevice::operator=(CDevice &theDevice)
{
    CDrvObjectBase::operator=(theDevice);
    m_strConnType = theDevice.m_strConnType;	// ��������
    m_strConnParam = theDevice.m_strConnParam;	// ���Ӳ���
    m_nConnTimeOut = theDevice.m_nConnTimeOut;	// ����Ϊ��λ
    m_nRecvTimeOut = theDevice.m_nRecvTimeOut;	// ����Ϊ��λ
    m_bMultiLink = theDevice.m_bMultiLink;	// �豸�Ƿ�֧�ֶ�����
    m_strTaskID = theDevice.m_strTaskID;
    return *this;
}

// ����������һ�����Զ�������λ֮��
// �����ʱ���Ƕ�ʱ�������״̬�Ķ�ʱ��
void  CDevice::StartCheckConnectTimer()
{
    m_pTaskGroup->m_pReactor->cancel_timer((ACE_Event_Handler *)(CDevice *)this);

    // �趨�豸�ϵĶ�ʱ�����������Լ���������InitDevice�У��Ƿ�����ʱ��

    // ����ʱ����Ϊ������ӵĶ�ʱ��
    int nCheckConnTimeout = m_nConnTimeOut * 2; // 2�������ӳ�ʱ����Ϊ��ʱ��������Ƿ�Ͽ�������

    if(nCheckConnTimeout <= 1000) // С��1��
        nCheckConnTimeout = 5000; // ȱʡΪ5��

    ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
    tvCheckConn.msec(nCheckConnTimeout);
    ACE_Time_Value tvStart = ACE_Time_Value::zero;
    m_pTaskGroup->m_pReactor->schedule_timer((ACE_Event_Handler *)(CDevice *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "start device(%s)'timer to check connect, cycle:%d ms",this->m_strName.c_str(), nCheckConnTimeout);
}


void CDevice::CallInitDeviceFunc()
{
    if (g_nDrvLang == PK_LANG_CPP) // C++����
    {
        // ���ó�ʼ�����������ܻ�ȥͨ��socket�򴮿������豸�����������Ҫ�����豸����֮�����
        if (g_pfnInitDevice)
            g_pfnInitDevice(this->m_pOutDeviceInfo);
    }
    else
    {
		Py_InitDevice(this);
    }

    // ȡ�����ж�ʱ������Сʱ��
    int nMinTimerPeriodMS = 0;
    for (vector<CUserTimer*>::iterator itTimer = m_vecTimers.begin(); itTimer != m_vecTimers.end(); itTimer++)
    {
        CUserTimer *pTimer = *itTimer;
        if (pTimer->m_timerInfo.nPeriodMS > nMinTimerPeriodMS)
            nMinTimerPeriodMS = pTimer->m_timerInfo.nPeriodMS;
    }
    // ������ʱ���������������С�Ķ�ʱ����5�������
    int nTimerDiscConfirmSeconds = ceil(nMinTimerPeriodMS * 5 / 1000.0f); // 5������С��ʱ�������
    if (nTimerDiscConfirmSeconds > m_nDisconnectConfirmSeconds) // �Զ�����ʱ�䣬����ʱ�����С
        m_nDisconnectConfirmSeconds = nTimerDiscConfirmSeconds;
}

int OnConnChanged(PKCONNECTHANDLE hConnect, int bConnected, char *szConnExtra, int nCallbackParam, void *pCallbackParam)
{
	CDevice *pDevice = (CDevice *)pCallbackParam;
	if (bConnected)
	{
		pDevice->SetConnectOK(); // ����������socket������Ϊ����״̬OK
		pDevice->SetDeviceConnected(1);
	}
	else
		pDevice->SetDeviceConnected(0);

	if (g_pfnOnDeviceConnStateChanged) // ����������������
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
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "���������������ӷ�ʽ��ͨ�����ͣ�%s,���� %s", m_strConnType.c_str(), m_strConnParam.c_str());
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
	InitConnectToDevice(); // ������û�н�������

	// ���ó�ʼ�����������ܻ�ȥͨ��socket�򴮿������豸�����������Ҫ�����豸����֮�����
	StartCheckConnectTimer();

	CallInitDeviceFunc();

	// ���ó�ʼ����Ϻ�, ����Ӧ�ó��Կ�ʼ�������ӻ��ߴ������˿�
	if (m_hPKCommunicate)
	{
		PKCommunicate_Connect(m_hPKCommunicate, 100);
	}
}

void CDevice::Stop()
{
    m_pTaskGroup->m_pReactor->cancel_timer(this);

    // ֹͣ�������ݿ�Ķ�ʱ��
    std::vector<CUserTimer*>::iterator iterBlk = m_vecTimers.begin();
    for (; iterBlk != m_vecTimers.end(); iterBlk++)
    {
        CUserTimer *pTimer = *iterBlk;
        pTimer->StopTimer();
    }

    g_logger.LogMessage(PK_LOGLEVEL_INFO, "%s will call UnitInitDevice!", m_strName.c_str());
    // �����ڶϿ�����ǰ��UnInitDevice�����⻹��Ҫʹ������!
    if(g_pfnUnInitDevice)
        g_pfnUnInitDevice(this->m_pOutDeviceInfo);
	Py_UnInitDevice(this); // python�汾�ĸú���ʵ��

	if (m_hPKCommunicate)
	{
		PKCommunicate_UnInitConn(m_hPKCommunicate);
		m_hPKCommunicate = NULL;
	}
}

// �����豸��ϵͳ��byteorder��һ�µ����
void CDevice::processByteOrder(PKTAG *pTag, char *szBinData, int nBinLen)
{
	if (g_bSysBigEndian == this->m_bDeviceBigEndian)
	{
		// Ӧ�ü����ֽ�����
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

	// Ӧ�ü����ֽ�����
	CByteOrder::SwapBytes(pTag->nByteOrder, (unsigned char *)szBinData, nBinLen, pTag->nDataType);
}

bool CDevice::GetEnableConnect()	// �����Ƿ���������
{
	return m_pTagEnableConnect->szData[0] != 0 ?true : false;
}

void CDevice::SetEnableConnect(bool bEnableConnect)	// �����Ƿ���������
{
	m_pTagEnableConnect->szData[0] = bEnableConnect ? 1 : 0;
	PKCommunicate_EnableConnect(m_hPKCommunicate, bEnableConnect);

	m_pOutDeviceInfo->bEnableConnect = bEnableConnect; // ������������PKDEVICE��������������
	if (!bEnableConnect)
	{
		// ���е����Ϊ**��
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

		// ��ֵ���͵�nodeserver
		short nVal = bEnableConnect ? 1 : 0;
		char szValue[32];
		sprintf(szValue, "%d", nVal);
		Drv_SetTagData_Text(pTag, szValue, 0, 0, 0);
		vector<PKTAG *> vecTags;
		vecTags.push_back(pTag);
		UpdateTagsData(vecTags.data(), vecTags.size());
		vecTags.clear();

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "���յ�enableconnect�����device:%s, tagname:%s, addr[%s], value:%d", m_strName.c_str(), pTag->szName, pTag->szAddress, bEnableConnect);
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Device::OnWriteCommand, cannot find OnControl function, Fail to control��device:%s, tag:%s, value:%s",
			m_strName.c_str(), pTag->szName, strValue.c_str());
		return;
	} 

    int lStatus = g_pfnOnControl(this->m_pOutDeviceInfo, pTag, strValue.c_str(), lCmdId);
    if (lStatus == PK_SUCCESS)
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "success to control��device:%s, tagname:%s, addr:%s, value:%s",
        m_strName.c_str(), pTag->szName, pTag->szAddress, strValue.c_str());
    else
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "fail to control��device:%s, tag:%s, addr:%s, value:%s, ret:%d",
        m_strName.c_str(), pTag->szName, pTag->szAddress, strValue.c_str(), lStatus);
}

int CDevice::SendToDevice(const char *szBuffer, int nBufLen, int nTimeOutMS)
{
	//printf("SendToDevice 7111, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());
    // ���߲�������豸��Ϣ�����ı䣬��Ҫ���������豸
    if (!m_bMultiLink && !IsActiveHost())
    {
        g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���豸��������ʧ��(���� %d ���ֽڣ���ʱ %d ), ���豸�ǵ������豸���ұ���Ϊ����",
            nBufLen, nTimeOutMS);
        return -1;
    }
	//printf("SendToDevice 7112, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());

	// TODO ���豸���÷����˸ı䡣�����е����ˣ�Ӧ�����յ�֪ͨʱ�ͽ��������л�
	if (m_bConfigChanged) // ���豸���÷����˸ı䡣�����е����ˣ�
	{
		//printf("SendToDevice 7113, buffer:0x%x len:%d, %s\n", (int)szBuffer, nBufLen, m_strName.c_str());
		m_bConfigChanged = false;

		// ���½������ӣ�������ҪInitDevice
		InitConnectToDevice();
		StartCheckConnectTimer(); // ���������쳵���ӵĶ�ʱ��

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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��������ʧ��(Ҫ���� %d ���ֽڣ���ʱ %d ), m_hPKCommunicate == NULL �����Ƿ����ӻ����õ����Ӵ��Ƿ���ȷ",
			m_strName.c_str(), nBufLen, nTimeOutMS);
		return -35;
	}
}

int CDevice::RecvFromDevice(char *szBuffer, int nBufLen, int nTimeOutMS)
{
    if (!m_hPKCommunicate)
    {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��������ʧ��(���ջ��������� %d ���ֽڣ���ʱ %d ), m_hPKCommunicate == NULL�����Ƿ������ӻ����õ����Ӵ��Ƿ���ȷ",
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


// ���Ӳ����������tcp�����:IP,Port,��ѡ�����ӳ�ʱ���Ժ��ǿ����д���
// ����һ����tcpclient���ӷ�ʽ����ʽ: IP:192.168.1.5;Port:502;ConnTimeOut:3000
int CDevice::GetMultiLink(char *szConnParam)
{
    m_bMultiLink = true;
    string strConnParam = szConnParam;
    //g_logger.LogMessage(PK_LOGLEVEL_INFO, "�����豸���Ӳ��� %s", szConnParam);
    strConnParam += ";"; // ����һ���ֺ�
    int nPos = strConnParam.find(';');
    while (nPos != string::npos)
    {
        string strOneParam = strConnParam.substr(0, nPos);
        strConnParam = strConnParam.substr(nPos + 1);// ��ȥ��һ���Ѿ��������Ĳ���
        nPos = strConnParam.find(';');

        if (strOneParam.empty())
            continue;

        int nPosPart = strOneParam.find('=');	// IP:168.2.8.75
        if (nPosPart == string::npos)
            continue;

        // ��ȡ��ĳ���������ƺ�ֵ
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
		memcpy(szValue, pTagData->szData, nValueBufLen); // ���Ƶ���ԭʼ���ݣ���Ҫת��Ϊ�ַ���
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

    // ֹͣ�������ݿ�Ķ�ʱ��
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

// Ϊ�ձ�ʾtag��ʱ���
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

// �õ�tag�����Ϣ���������Ա��������NodeServer
// ���ػ���������>=0��Ч
int CDevice::GetTagDataBuffer(PKTAG *pTag, char *szTagBuffer, int nBufLen, unsigned int nCurSec, unsigned int nCurMSec)
{
	int nTagDataLen = pTag->nDataLen; // ���нضϣ�������
	if (nBufLen < sizeof(int) * 5 + sizeof(short) + pTag->nDataLen)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "GetTagDataBuffer(%s), taglen:%d > bufferlen:%d + sizeof(int) * 5 + sizeof(short) + pTag->nDataLen", pTag->szName, pTag->nDataLen, nBufLen);
		nTagDataLen = sizeof(int) * 5 + sizeof(short) + pTag->nDataLen - nBufLen;
	}

    char *szTagName = pTag->szName;
    int nDataType = pTag->nDataType;
    int nSec = nCurSec;
    int nMSec = nCurMSec;
    // ���ʱ��Ϊ���򷵻ص�ǰʱ��
    if (pTag->nTimeSec > 0)
    {
        nSec = pTag->nTimeSec;
        nMSec = pTag->nTimeMilSec;
    }

    char *pData = szTagBuffer;

    // tag��
    int nLen = strlen(szTagName);
    memcpy(pData, &nLen, sizeof(int));
    pData += sizeof(int);
    memcpy(pData, szTagName, nLen);
    pData += nLen;

    // tag��������
    memcpy(pData, &nDataType, sizeof(int));
    pData += sizeof(int);

    // tagֵ�ĳ��Ⱥ�tagֵ����
    memcpy(pData, &nTagDataLen, sizeof(int));
    pData += sizeof(int);
    if (nTagDataLen > 0)
    {
        memcpy(pData, pTag->szData, nTagDataLen);
		CByteOrder::SwapBytes(pTag->nByteOrder, (unsigned char *)pData, nTagDataLen, pTag->nDataType); // Ӧ�ü����ֽ�����
        pData += nTagDataLen;
    }

    // tag��ͺ�����
    memcpy(pData, &nSec, sizeof(unsigned int));
    pData += sizeof(int);
    memcpy(pData, &nMSec, sizeof(unsigned short));
	pData += sizeof(short);

    // tag����
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

    // ȡ��ʱ����Ϣ
    unsigned int nCurSec = 0;
    unsigned int nCurMSec = 0;
    nCurSec = PKTimeHelper::GetHighResTime(&nCurMSec);

    int nRet = 0;
    int nFailCount = 0;
	char *szQueRecBuffer = m_pTaskGroup->m_pQueRecBuffer; /// [PROCESSQUEUE_DEFAULT_RECORD_LENGHT] = { 0 }; //ÿ��Que��һ����¼�Ĵ�С
    char *pQueBufP = szQueRecBuffer;
    char *pQueBufEnd = szQueRecBuffer + m_pTaskGroup->m_nQueRecBufferLen; // ָ��β��

    // ����ʱ���
    memcpy(pQueBufP, &nCurSec, sizeof(unsigned int));
    pQueBufP += sizeof(unsigned int);
    memcpy(pQueBufP, &nCurMSec, sizeof(unsigned short));
	pQueBufP += sizeof(unsigned short);

    // ��������
    int nActionType = ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ;
    memcpy(pQueBufP, &nActionType, sizeof(nActionType));
    pQueBufP += sizeof(int); // ����acitontype

	char *pTagCount = pQueBufP;
    pQueBufP += sizeof(int); // ����tag����,֮�ٸ�ֵ��
    int nTagNumThisPack = 0;
    for (int iTag = 0; iTag < nTagNum; iTag++)
    {
		PKTAG *pTag = ppTags[iTag];
        if (pTag->nTimeSec == 0 && pTag->nTimeMilSec == 0)
        {
            pTag->nTimeSec = nCurSec;
            pTag->nTimeMilSec = nCurMSec;
        }

        if (pTag->nQuality == PK_SUCCESS && pTag->nDataLen > 0) // ����ֵ
        {
            // �������Ƶ�tagdataת��Ϊ�ַ�������
            TagValBuf_ConvertEndian(this->m_bDeviceBigEndian, pTag->szName, pTag->nDataType, pTag->szData, pTag->nDataLen, 0);
        }

        // ���µ����صĻ����У��Ա㽫��������Ҫʱ���Կ��ٻ�ȡ
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
        char *szOneTagBuf = m_pTaskGroup->m_pTagVTQBuffer; //ÿ��Tag��һ����¼�Ĵ�С,�������4K,���Ǽ���һЩʱ�������Ϣ
        int nOneTagBufLen = GetTagDataBuffer(pTag, szOneTagBuf, m_pTaskGroup->m_nTagVTQBufferLen, nCurSec, nCurMSec);
        if (nOneTagBufLen + pQueBufP > pQueBufEnd) // һ�ηŲ����ˣ���Ҫ�ȷ�����
        {
			memcpy(pTagCount, &nTagNumThisPack, sizeof(int)); // ����tag����
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
            // ���¿�ʼ
            nTagNumThisPack = 0;
            pQueBufP = szQueRecBuffer;
            pQueBufP += sizeof(unsigned int); // ����ʱ�����
            pQueBufP += sizeof(unsigned short); // ����ʱ�������
            pQueBufP += sizeof(int); // ����actiontype

            // ��ǰ���һ������δ���ͣ���Ҫ�ȷŽ�ȥ
            nTagNumThisPack++;
			pQueBufP += sizeof(int); // ����tag����
			memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);

            pQueBufP += nOneTagBufLen;
        }
        else // ��������û���꣬�ɷ��ڰ���һ����
        {
            nTagNumThisPack++;
            memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);
            pQueBufP += nOneTagBufLen;
        }
    }

    // �������һ����������еĻ�
    if (pQueBufP - szQueRecBuffer > sizeof(int) * 2)
    {
		memcpy(pTagCount, &nTagNumThisPack, sizeof(int)); // ����tag����,ǰ����ʱ�����ActionType
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

// Ϊÿ���豸�������е�tag�����Ӧ��״̬λ��
int CDevice::AddTags2InitDataShm()
{
    int nRet = 0;
    char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    std::map<std::string, PKTAG *>::iterator itTag = m_mapName2Tags.begin();
    for (; itTag != m_mapName2Tags.end(); itTag++)
    {
        PKTAG *pTag = (PKTAG *)itTag->second;
        char szTagVTQ[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH]; // ����ַ�������ܴ�
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

	nRet = this->SetDeviceConnected(0); // ȱʡ��Ϊ��δ�����ϡ������moxa������ô���������أ�
    nRet = AddTags2InitDataShm();

    // �����豸��task��shm���̣߳�
    char szKey[PK_NAME_MAXLEN * 6] = { 0 };
    PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:%s:tasks", g_strDrvName.c_str(), szDeviceName);
    nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV, szKey, szTaskName, this->m_pTaskGroup->m_drvObjBase.m_strName.length());

    return nRet;
}

int CDevice::ReConnectDevice(int nTimeOutMS)
{
	// ����������õ�֧��ͨѶ��ʽ�������������Ϊ�豸��������
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
	if (m_nConnectType != PK_COMMUNICATE_TYPE_NOTSUPPORT) // ���õ�ͨѶ��ʽ������TcpClient��TcpServer��Serial��Udp
	{
		int nConnected = PKCommunicate_IsConnected(m_hPKCommunicate);
		if (nConnected == 0) // ���δ������������ֱ�ӷ���
		{
			int nRet = PKCommunicate_Connect(m_hPKCommunicate, nTimeOutMS);
			nConnected = PKCommunicate_IsConnected(m_hPKCommunicate);
		}

		if (nConnected == 0) // ���δ�����������϶�δBAD������Ҫ����
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CheckAndConnect failed to connect to device:%s,conntype:%s,connparam:%s!", this->m_strName.c_str(), this->m_strConnType.c_str(), this->m_strConnParam.c_str());
			SetDeviceConnected(0); // �����豸δ������
			return 0;
		}
	}

	// �������Ϊ���������ˣ�1������ͨѶ�����ϣ�����serial��tcpserver��tcpclient��udp������2��������ͨѶ��ʽ��DB��pkmqtt���ļ��ȣ�
	// ����ϴ��Ƿ��յ����ݣ�����յ���������ΪͨѶ��OK��
	ACE_Time_Value tvNow = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvNow - m_tvLastConnOK;
	ACE_UINT64 nMSec = tvSpan.get_msec();
	// string strDeviceConnStatus = "1";
	if (m_nConnStatusTimeoutMS > 0 && nMSec > m_nConnStatusTimeoutMS) // ������ͨѶ��ʱ���ҳ�ʱδ�յ���������Ϊ�豸δ����
	{
		// strDeviceConnStatus = "0";
		nConnected = 0;
	}
	else
		nConnected = 1;

	SetDeviceConnected(nConnected); // �����豸��������

	// Drv_SetTagData_Text(m_pTagConnStatus, strDeviceConnStatus.c_str(), 0, 0, 0);

	return 0; 
}

// ���豸��Ϣ�������������õ����ݽṹ��ȥ
void CDevice::CopyDeviceInfo2OutDevice()
{
    if (m_pTaskGroup)
        m_pOutDeviceInfo->pDriver = m_pTaskGroup->m_pMainTask->m_driverInfo.m_pOutDriverInfo;
    else
        m_pOutDeviceInfo->pDriver = NULL;

    // ����C�����������õ����ݽṹ.������ָ���ڴ��׵�ַ
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
        if (m_bLastConnStatus)// ��ǰ������,����û����,1--->0
        {
            if (m_tmLastDisconnect == 0) // ��ǰû�жϿ�
                m_tmLastDisconnect = tmNow; // ���η��ֶϿ�ʱ��
            else // ��ǰ��¼��ʱ�䣬�Ѿ����˶Ͽ���ʶ
            {
                int nDiscTimeSpan = tmNow - m_tmLastDisconnect; // ���ϴ�Ϊ�����ϵĲ�ֵ
                if (nDiscTimeSpan > m_nDisconnectConfirmSeconds)
                {
                    bChangedStatus = true;
                    //bDisconnConfirmed = true; // ȷ�϶Ͽ�������
                    m_bLastConnStatus = false; // ȷ��֮ǰ״̬Ϊδ����
                }
            }
        }
    }
    else // ����������
    {
        if (!m_bLastConnStatus)   // ��ǰû���ϣ�����������
        {
            bChangedStatus = true;
            m_bLastConnStatus = true; // �ϴ�����״̬
        }
        m_tmLastDisconnect = 0; // ��ʶ֮ǰһֱ��������״̬
    }

    // ����tag�㣬��������״̬��Щ���⣬��Ӧ��Ϊ*
    if (bChangedStatus || (!m_bLastConnStatus && !bDevConnected)) // ״̬�ı䣬����֮ǰ�����ڶ�δ�����ϣ�����ˢ��
	{
		vector<PKTAG*> vecTags;
        // ���ӷ�����״̬�ĵ㵽���¼�����
        for (int i = 0; i < m_vecAllTag.size(); i++)
        {
			PKTAG *pTag = m_vecAllTag[i];
            if (!bDevConnected) // �Ͽ��������ã������򲻴���
            {
				Drv_SetTagData_Text(pTag, TAG_QUALITY_COMMUNICATE_FAILURE_STRING, 0, 0, TAG_QUALITY_COMMUNICATE_FAILURE);
				vecTags.push_back(pTag);
			}
        }

        // ��������״̬�ĵ㵽���¼�����
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
        // ��������£�������������״̬�ĵ�
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

// ppTags��PKTAG *���飬�Ǵ���ǰӦ��Ԥ�ȷ���õ�
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

// �Զ������ж�ͨѶ�Ͽ��ĳ�ʱ
int CDevice::SetConnectOKTimeout(int nConnBadTimeooutMS)
{
	m_nConnStatusTimeoutMS = nConnBadTimeooutMS;
	m_tvLastConnOK = ACE_Time_Value::zero;
	return 0;
}

// �յ�����ʱ����1�θýӿ�
void  CDevice::SetConnectOK()
{
	m_tvLastConnOK = ACE_OS::gettimeofday();
}
