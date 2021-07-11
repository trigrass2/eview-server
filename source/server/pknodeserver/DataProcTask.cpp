/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  shijunpu  Initial Version
 *  @version	9/17/2012  shijunpu  ���ݿ������޸�Ϊ�ַ������ͣ������޸��������ݿ�ʱ�Ľӿ�
 *  @version	3/1/2013   shijunpu  �����ɵ����ɿ���߼������豸��û���������ݿ�ʱ��ֱ���ɵ��������ݿ� .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"
#include "pkcomm/pkcomm.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include <sstream>
#include "pkcomm/pkcomm.h" 
#include "RedisFieldDefine.h"
#include "TagExprCalc.h"
#include "DataProcTask.h"
#include "MainTask.h"

#ifdef _WIN32
#include "EnumProc.h"
static CFindKillProcess g_findKillProcess;
#endif

extern CPKLog g_logger;
extern CProcessQueue * g_pQueData2NodeSrv;

int TagValFromBin2String(CDataTag *pTag, char *szTagName, int nTagDataType,const char *szInBinData, int nBinDataBufLen, string &strTagValue);

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CDataProcTask::CDataProcTask()
{
	m_bStop = false;
	m_bStopped = false;

	m_nPackNumRecvRecent = 0;
	m_nPackNumRecvFromStart = 0;

	m_tvRecentRecvPack = m_tvFirstRecvPack = ACE_OS::gettimeofday();	// �ϴδ�ӡ
	m_nTimeoutPackNumFromStart = 0;
	m_nTimeoutPackNumRecent = 0;
	m_nTimeoutMaxSecFromStart = 0;
	m_nTimeoutMaxSecRecent = 0;

	m_nRecvTagCountRecent = 0;	// һ��ʱ�����յ�����������������tag��
	m_nValidTagCountRecent = 0;	// һ��ʱ�����յ�����������������Чtag��
	m_strMinTagTimestampRecent = "";	// ���һ��ʱ�����Сʱ���
	m_strMaxTagTimestampRecent = ""; // ���һ��ʱ������ʱ���
	m_nCommitTagNumRecent = 0;		// ���һ��ʱ���ύ��tag����
	m_nCommitTimesRecent = 0;		// ���һ��ʱ���ύ�Ĵ���
	m_strCommitDetail = "";			// ÿ���ύ������

	m_nQueRecBufferLen = MAIN_TASK->m_nNodeServerQueSize / 2;
	if (m_nQueRecBufferLen < 1 * 1024 * 1024) // 1M
		m_nQueRecBufferLen = 1 * 1024 * 1024;
	if (m_nQueRecBufferLen > 10 * 1024 * 1024)
		m_nQueRecBufferLen = 10 * 1024 * 1024;
	m_pQueRecBuffer = new char[m_nQueRecBufferLen];

	m_nQueTagVTQBufferLen = m_nQueRecBufferLen; // һ�η��Ͷ��TAG����ڴ�ָ��
	m_pQueTagVTQBuffer = new char[m_nQueTagVTQBufferLen]; // һ�η��Ͷ��TAG��ļ�¼�ڴ��С
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CDataProcTask::~CDataProcTask()
{
	
}

void CDataProcTask::PrintDataPackStatInfo(ACE_Time_Value &tvNow, ACE_Time_Value &tvPack)
{
	m_nPackNumRecvFromStart ++;
	m_nPackNumRecvRecent ++;

	// ������û�г�ʱ
	ACE_Time_Value tvPackSpan = tvNow - tvPack;
	if(labs(tvPackSpan.sec()) > 2) // ���ͺͽ���ʱ�������2�룡
	{
		m_nTimeoutPackNumFromStart ++;
		m_nTimeoutPackNumRecent ++;
		if (labs(tvPackSpan.sec()) > m_nTimeoutMaxSecRecent)
			m_nTimeoutMaxSecRecent = labs(tvPackSpan.sec());
		if (labs(tvPackSpan.sec()) > m_nTimeoutMaxSecFromStart)
			m_nTimeoutMaxSecFromStart = labs(tvPackSpan.sec());
	}

	ACE_Time_Value tvSpanRecentRecv = tvNow - m_tvRecentRecvPack;
	int nSecSpan = labs(tvSpanRecentRecv.sec());
	if(nSecSpan > 30) // ��ʱ����Ҫ��ӡ��Ϣ��
	{
#ifdef _WIN32
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�����������յ�:%d��, ����tag��:%d��, ��Чtag��:%d��",
			m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// ÿ���ύ������);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "���:%d��: ���� %d ����,ƽ�� %f����/��. �ύ����:%d, tag����:%d��,ƽ��ÿ��%.f��, tag��Сʱ��:%s, ���ʱ��:%s. ",
			nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
			 m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());	
#else
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recv:%d packets from start, total tagnum:%d, invalid tagnum:%d",
            m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// ÿ���ύ������);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recent:%d seconds: recv %d packets,avg %f packets/second. commit count:%d, tagnum:%d,avg %.f tagnum/second, tag mintime:%s, maxtime:%s. ",
            nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
             m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());
#endif
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "���:%d��: �ύ��ϸ:%s", nSecSpan, m_strCommitDetail.c_str());	

		if(m_nTimeoutPackNumRecent > 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "!!����(performance)����: ���%d���ڣ����պͷ��ͼ������>2��(�������%d�룩��%d����. ������������, ����2��(�������%d�룩�� %d ����",
				nSecSpan, m_nTimeoutMaxSecRecent, m_nTimeoutPackNumRecent, m_nTimeoutMaxSecFromStart, m_nTimeoutPackNumFromStart);
        }
		m_nPackNumRecvRecent = 0;
		m_nTimeoutPackNumRecent = 0;
		m_nTimeoutMaxSecRecent = 0;
		m_tvRecentRecvPack = tvNow;

		m_nRecvTagCountRecent = 0;
		m_nValidTagCountRecent = 0;

		m_nCommitTagNumRecent = 0;
		m_strMinTagTimestampRecent = "";
		m_strMaxTagTimestampRecent = "";

		m_strCommitDetail = "";
		m_nCommitTimesRecent = 0;	
	}
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CDataProcTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();
	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	char *szRecBuffer = m_pQueRecBuffer;
	unsigned int nRecLen = 0;

	// ��������������е���������ʷ����
	//g_logger.LogMessage(PK_LOGLEVEL_INFO, "������������������ݶ���");
	while(true)
	{
		int nResult = g_pQueData2NodeSrv->DeQueue(szRecBuffer, m_nQueRecBufferLen, &nRecLen);
		if(nResult != PK_SUCCESS)
			break;
	}

	while(!m_bStop)
	{
		bool bFoundRecord = false;
		// ��ȡ�����һ���������ݣ�ǰ��ļ�¼ȫ�����׵�
        int nResult = g_pQueData2NodeSrv->DeQueue(szRecBuffer, m_nQueRecBufferLen, &nRecLen); // cpu is low: this step
		if(nResult != PK_SUCCESS)
		{
			ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // �յ�����ʱ��
			DoCalcTags(tvNow); // ��������ã���ô����ֹͣ�󣬶��α������޷����£�
			CheckAndCommitTagData(tvNow);

			ACE_Time_Value tvCycle;
			tvCycle.msec(100);
			ACE_OS::sleep (tvCycle);
			continue;
		}

		if(nRecLen >= m_nQueRecBufferLen)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[DeQueue]recv an error buffer, len:%d > %d",nRecLen, m_nQueRecBufferLen);
			continue;
		}

		ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // �յ�����ʱ��
		string strrecbuffer = szRecBuffer;
		szRecBuffer[nRecLen] = '\0';
		char *pCurBuf = szRecBuffer;

		// ��������ʱ���
		int nPackTimeStampSec = 0;
		int nPackTimeStampMSec = 0;
		// �������͵�ʱ�䣨��ͺ��룩
		memcpy(&nPackTimeStampSec, pCurBuf, sizeof(unsigned int));
		pCurBuf += sizeof(unsigned int);
		memcpy(&nPackTimeStampMSec, pCurBuf, sizeof(unsigned short));
		pCurBuf += sizeof(unsigned short);
		ACE_Time_Value tvPack;
		tvPack.sec(nPackTimeStampSec);
		tvPack.usec(nPackTimeStampMSec * 1000);
		PrintDataPackStatInfo(tvNow, tvPack);

		int nActionType;
		memcpy(&nActionType, pCurBuf, sizeof(int)); // ��һ���ֽ��Ƕ�������
		pCurBuf += sizeof(int);

		int uTagNameLen = 0;
		int uTagDataLen = 0;
		char szTagName[PK_NAME_MAXLEN * 2];
		char *szTagData = m_pQueTagVTQBuffer;
		int nTagQualityLen = 0;

		if(nActionType == ACTIONTYPE_DRV2SERVER_CLEAR_DRIVER)
		{
			int nDriverNameLen = 0;
			memcpy(&nDriverNameLen, pCurBuf, sizeof(int));
			pCurBuf += sizeof(int);
			char szDriverName[PK_NAME_MAXLEN] = {0};
			PKStringHelper::Safe_StrNCpy(szDriverName, pCurBuf, sizeof(szDriverName));
			ClearMDBDataOfDriver(szDriverName);
		}
		else if(nActionType == ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ) // push������GroupTaskʱ�õ����б�ʽ
		{
			ACE_Time_Value tvTimeStamp;
			unsigned int	nSec = 0;
			unsigned int	mSec = 0;
			int				nCurQuality = 0;
			int	nDataType = 0;

			// ����tag����
			int nTagNum = 0;
			memcpy(&nTagNum, pCurBuf, sizeof(int));
			pCurBuf += sizeof(int);

			for(int iTag = 0; iTag < nTagNum; iTag ++)
			{
				m_nRecvTagCountRecent ++;

				// ����tagname�ĳ���
				memcpy(&uTagNameLen, pCurBuf, sizeof(int));
				if(pCurBuf > szRecBuffer + nRecLen)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pbserver,process tagnum:%d, no.%d buffer not enough(quelen:%d)", nTagNum, iTag, nRecLen);
					break;
				}
				// ����tagname
				if(uTagNameLen > sizeof(szTagName))
					uTagNameLen = sizeof(szTagName) - 1;
				pCurBuf += sizeof(int);
				memcpy(szTagName, pCurBuf, uTagNameLen);
				szTagName[uTagNameLen] = '\0';
				pCurBuf += uTagNameLen;

				//if (strcmp(szTagName, "һ�ܵ�.״̬") == 0)
				//{
				//	printf("TagName:%s,data recv\n", szTagName);
				//}
				// ����tag����������
				memcpy(&nDataType, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);

				// ����tagdata��length
				memcpy(&uTagDataLen, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);
				if(uTagDataLen > m_nQueTagVTQBufferLen) // ����10M����Ϊ���ݷǷ�
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pbserver, dataproc error, byes:%d, tagnum:%d, cur:%d", nRecLen, nTagNum, iTag);
					break;
				}
				
				// ����tagdata
				memcpy(szTagData, pCurBuf, uTagDataLen);
				szTagData[uTagDataLen] = '\0';
				pCurBuf += uTagDataLen;

				// ����tag��timestamp
				memcpy(&nSec, pCurBuf, sizeof(unsigned int));
				pCurBuf += sizeof(unsigned int);
				memcpy(&mSec, pCurBuf, sizeof(unsigned short));
				pCurBuf += sizeof(unsigned short);
				tvTimeStamp.sec(nSec);
				tvTimeStamp.usec(mSec * 1000);

				// ����tag������
				memcpy(&nCurQuality, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);

				//----һ��tag�������

				// ��������Ҫ�������㴦���
				map<string, CDataTag *> &mapName2Tag = MAIN_TASK->m_mapName2Tag;
				map<string, CDataTag *>::iterator itMap = mapName2Tag.find(szTagName);
				if (itMap == mapName2Tag.end())
				{
					if (strstr(szTagName, ".connstatus") == NULL) // ״̬�����õģ�����Ҫ����Ҳ����Ҫ��ʾ
						g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKNodeServer::DataProcTask, recv tagname %s, not existing", szTagName);
					continue;
				}
				CDataTag *pTag = itMap->second;

				string strTagValue;
				if(nCurQuality == 0)
				{
					if(uTagDataLen > 0)
					{
						TagValFromBin2String(pTag, szTagName, pTag->nDataType, szTagData, uTagDataLen, strTagValue);
					}
					else
						strTagValue = "";
				}

				// ���ϴε�ֵ���浽lastXXX�У���ǰ������curVal��
				pTag->lastVal.AssignTagValue(pTag->nDataTypeClass, &pTag->curVal);
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, strTagValue, nCurQuality);
				if(pTag->nDataTypeClass != DATATYPE_CLASS_OTHER && pTag->bEnableAlarm)
				{
					// ����ʹ�����
					if (!pTag->vecAllAlarmTags.empty())
					{
						bool bAlarmChanged = false;
						m_eaProcessor.CheckAlarmsOfTag(pTag, bAlarmChanged);
						//if (bAlarmChanged)
						//	m_eaProcessor.ReCalcTagAlarmStatus(pTag); //  // ��Ϊδ֪ԭ���ڴ���tag��nMaxLevel��vecAlarm�ṹ��һ�£����������������
					}
				}

				if (pTag->bEnableRange)
				{

				}
				// ת����pkcserver TRDataProcTask���д��������ڴ����ݿ⣩,����ֻ����tag��ֵ��ʱ��Ȼ�������,����������Ե������д���
				ConvertTagData2JsonAndCache(pTag, nSec, mSec, nCurQuality); 

				m_nValidTagCountRecent ++;
			} 
		}
		else if(nActionType == ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE || nActionType == ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV) // ���������ݳ��ȶ�����
		{ // ��ʼ��ʱ��Ҫ���ó�ʼtag�㣬��������list��Աʱ�����
			int nKeyLen = 0;
			int nValueLen = 0;
			char szKey[PK_NAME_MAXLEN * 2];
			char szValue[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH];  // ���������ݳ��ȶ�������4K�͹���

			// ����tagname
			memcpy(&nKeyLen, pCurBuf, sizeof(int));
			if(nKeyLen >= sizeof(szKey))
				nKeyLen = sizeof(szKey) - 1;
			pCurBuf += sizeof(int);
			memcpy(szKey, pCurBuf, nKeyLen);
			szKey[nKeyLen] = '\0';
			pCurBuf += nKeyLen;

			// ����tagdata��length
			memcpy(&nValueLen, pCurBuf, sizeof(int));
			pCurBuf += sizeof(int);

			// ����tagdata
			memcpy(szValue, pCurBuf, nValueLen);
			szValue[nValueLen] = '\0';
			pCurBuf += nValueLen;
		}

		DoCalcTags(tvNow);
		CheckAndCommitTagData(tvNow);
	} // while(!m_bStop)

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"nodeserver DataProcTask exit normally!");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

vector<string> CDataProcTask::split(string str,string pattern)
{
	string::size_type pos;  
	vector<string> result;  
	str+=pattern;     //��չ�ַ����Է������  
	int size=str.size();  

	for(int i=0; i<size; i++)  
	{  
		pos=str.find(pattern,i);  
		if(pos<size)  
		{  
			std::string s=str.substr(i,pos-i);  
			result.push_back(s);  
			i=pos+pattern.size()-1;  
		}  
	}  
	return result;  
}

int FromBlobToBase64Str(const char *szBlob, int nBlobBufLen, string &strBase64){
	return 0;
}

#define SAFE_COVERT(T)	T _temp;  memcpy(&_temp, szBinData, sizeof(T));
int TagValFromBin2String(CDataTag *pTag, char *szTagName, int nTagDataType,const char *szInBinData, int nBinDataBufLen, string &strTagValue)
{
	if(nBinDataBufLen <= 0)
	{
		strTagValue = "";
		return -1;
	}

	if (nTagDataType == TAG_DT_TEXT)
	{
		strTagValue = szInBinData;
		return 0;
	}
	if (nTagDataType == TAG_DT_BLOB)
	{
		FromBlobToBase64Str(szInBinData, nBinDataBufLen, strTagValue);
		return 0;
	}

	const char *szBinData = szInBinData;
	char szStrValue[PK_NAME_MAXLEN + 1] = { 0 }; // ֻ����ֵ���ͣ�����ռ�úܶ�ռ�

	// ������ֵ���ַ���ת��Ϊ����������
	stringstream ssValue;
	switch(nTagDataType){
	case TAG_DT_BOOL:		// 1 bit value
		{
			unsigned char ucValue = (*(unsigned char *)(szBinData));
			ucValue = pTag->CalcRangeValue((int)ucValue);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", (ucValue & 0x01) != 0 ); // ȡnTagOffsetOfOneByteλ

			strTagValue = szStrValue;
			break;
		}
	case TAG_DT_CHAR:		// 8 bit signed integer value
		{
			SAFE_COVERT(char)
			_temp = pTag->CalcRangeValue((int)_temp);

			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp);
			strTagValue = szStrValue;
			break;
		}
	case TAG_DT_INT16:		// 16 bit Signed Integer value
		{
			if(nBinDataBufLen < sizeof(short))
				return -2;
			SAFE_COVERT(short)
			_temp = pTag->CalcRangeValue((int)_temp);

			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp );
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
		{
			if(nBinDataBufLen < sizeof(unsigned short))
				return -2;
			SAFE_COVERT(unsigned short)
			_temp = pTag->CalcRangeValue((int)_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_FLOAT:		// 32 bit IEEE float
		{
			if(nBinDataBufLen < sizeof(float))
				return -2;
			SAFE_COVERT(float)
			_temp = pTag->CalcRangeValue(_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_UCHAR:		// 8 bit unsigned integer value
		SAFE_COVERT(unsigned char)
		_temp = pTag->CalcRangeValue((int)_temp);
		PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
		strTagValue = szStrValue;
		break;
	case TAG_DT_UINT32:		// 32 bit integer value
		{
			if(nBinDataBufLen < sizeof(unsigned int))
				return -2;
			SAFE_COVERT(unsigned int)
			_temp = pTag->CalcRangeValue((int)_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_INT32:		// 32 bit signed integer value
		{
			if(nBinDataBufLen < sizeof(int))
				return -2;
			SAFE_COVERT(int)
			_temp = pTag->CalcRangeValue((int)_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_DOUBLE:		// 64 bit double
		{
			if(nBinDataBufLen < sizeof(double))
				return -2;
			SAFE_COVERT(double)
			_temp = pTag->CalcRangeValue(_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_INT64:		// 64 bit signed integer value
		{
			if(nBinDataBufLen < sizeof(ACE_INT64))
				return -2;
			SAFE_COVERT(ACE_INT64)
			_temp = pTag->CalcRangeValue(_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%lld", _temp);
			strTagValue = szStrValue;
		}
		break;

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
		{
			if(nBinDataBufLen < sizeof(ACE_UINT64))
				return -2;
			SAFE_COVERT(ACE_UINT64)
				_temp = pTag->CalcRangeValue(_temp);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%llu", _temp);
			strTagValue = szStrValue;
		}
		break;
	default:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, unknown datatype of %d when convert value from bin to string", szTagName, nTagDataType);
		return -1;
	}
	return 0;
}

int TagValFromBin2Int(char *szTagName, int nTagDataType, const char *szBinData, int nBinDataBufLen, int &nValue)
{
	if (nBinDataBufLen <= 0)
	{
		nValue = -1;
		return -1;
	}
	// ������ֵ���ַ���ת��Ϊ����������
	switch (nTagDataType){
	case TAG_DT_BOOL:		// 1 bit value
	{
		unsigned char ucValue = (*(unsigned char *)(szBinData));
		nValue = ucValue & 0x01; // ȡnTagOffsetOfOneByteλ
		break;
	}
	case TAG_DT_CHAR:		// 8 bit signed integer value
	{
		if (nBinDataBufLen < sizeof(char))
			return -2;
		SAFE_COVERT(unsigned int)
		nValue = _temp;
		break;
	}
	case TAG_DT_INT16:		// 16 bit Signed Integer value
	{
		if (nBinDataBufLen < sizeof(short))
			return -2;
		SAFE_COVERT(short)
		nValue = _temp;
	}
	break;

	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
	{
		if (nBinDataBufLen < sizeof(unsigned short))
			return -2;
		SAFE_COVERT(unsigned short)
		nValue = _temp;
	}
	break;
	case TAG_DT_UCHAR:		// 8 bit unsigned integer value
	{
		SAFE_COVERT(unsigned char)
		nValue = _temp;
		break;
	}
	case TAG_DT_UINT32:		// 32 bit integer value
	{
		if (nBinDataBufLen < sizeof(unsigned int))
			return -2;
		SAFE_COVERT(unsigned int)
			nValue = _temp;
	}
	break;

	case TAG_DT_INT32:		// 32 bit signed integer value
	{
		if (nBinDataBufLen < sizeof(int))
			return -2;
		SAFE_COVERT(int)
			nValue = _temp;
	}
	break;

	case TAG_DT_INT64:		// 64 bit signed integer value
	{
		if (nBinDataBufLen < sizeof(ACE_INT64))
			return -2;
		SAFE_COVERT(ACE_INT64)
			nValue = _temp;
	}
	break;

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
	{
		if (nBinDataBufLen < sizeof(ACE_UINT64))
			return -2;
		SAFE_COVERT(ACE_UINT64)
			nValue = _temp;
	}
	break;
	default:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, unknown datatype of %d when convert value from bin to string", szTagName, nTagDataType);
		return -1;
	}
	return 0;
}

int TagValFromBin2Real(char *szTagName, int nTagDataType, const char *szBinData, int nBinDataBufLen, double &dbValue)
{
	if (nBinDataBufLen <= 0)
	{
		dbValue = -1.f;
		return -1;
	}
	// ������ֵ���ַ���ת��Ϊ����������
	switch (nTagDataType)
	{
	case TAG_DT_FLOAT:		// 1 bit value
	{
		if (nBinDataBufLen < sizeof(float))
			return -2;
		SAFE_COVERT(float)
			dbValue = _temp;
		break;
	}
	case TAG_DT_DOUBLE:		// 8 bit signed integer value
	{
		if (nBinDataBufLen < sizeof(double))
			return -2;
		SAFE_COVERT(double)
			dbValue = _temp;
		break;
	}
	
	default:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, unknown datatype of %d when convert value from bin to string", szTagName, nTagDataType);
		return -1;
	}
	return 0;
}

int TagValFromBin2Text(char *szTagName, int nTagDataType, const char *szBinData, int nBinDataBufLen, string &strValue)
{
	if (nBinDataBufLen <= 0)
	{
		strValue = "";
		return -1;
	}
	switch (nTagDataType)
	{
	case TAG_DT_TEXT:
		// strncpy(szStrValue, szBinData, sizeof(szStrValue) - 1);
		strValue = szBinData;
		break;
	case TAG_DT_BLOB:
		FromBlobToBase64Str(szBinData, nBinDataBufLen, strValue);
		break;

	default:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, unknown datatype of %d when convert value from bin to string", szTagName, nTagDataType);
		return -1;
	}
	return 0;
}


// ��tag��ת��Ϊjson�ַ���
int CDataProcTask::ConvertTagData2JsonAndCache(CDataTag *pTag, unsigned int nSec, unsigned short mSec, int nQuality)
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::HighResTime2String(szTime, sizeof(szTime), nSec, mSec);
	string strTagValue;
	pTag->curVal.GetStrValue(pTag->nDataTypeClass, strTagValue);

	Json::Value jsonOneTag;
	if (nQuality == TAG_QUALITY_GOOD) // 
		jsonOneTag[JSONFIELD_VALUE] = strTagValue.c_str();
	else if (nQuality == TAG_QUALITY_INIT_STATE) // ?
	{
		jsonOneTag[JSONFIELD_VALUE] = TAG_QUALITY_INIT_STATE_STRING;
		jsonOneTag[JSONFIELD_MSG] = TAG_QUALITY_INIT_STATE_TIP;
	}
	else if (nQuality == TAG_QUALITY_COMMUNICATE_FAILURE) // *
	{
		jsonOneTag[JSONFIELD_VALUE] = TAG_QUALITY_COMMUNICATE_FAILURE_STRING;
		jsonOneTag[JSONFIELD_MSG] = TAG_QUALITY_COMMUNICATE_FAILURE_TIP;
	}
	else if (nQuality == TAG_QUALITY_DRIVER_NOT_ALIVE) // **
	{
		jsonOneTag[JSONFIELD_VALUE] = TAG_QUALITY_DRIVER_NOT_ALIVE_STRING;
		jsonOneTag[JSONFIELD_MSG] = TAG_QUALITY_DRIVER_NOT_ALIVE_TIP;
	}	
	else // ��������ΪBAD����� ***
	{
		jsonOneTag[JSONFIELD_VALUE] = TAG_QUALITY_UNKNOWN_REASON_STRING;
		jsonOneTag[JSONFIELD_MSG] = TAG_QUALITY_UNKNOWN_REASON_TIP;
	}
	jsonOneTag[JSONFIELD_TIME] = szTime;
	jsonOneTag[JSONFIELD_QUALITY] = nQuality;
	jsonOneTag[JSONFIELD_MAXALARM_LEVEL] = pTag->m_curAlarmInfo.m_nMaxAlarmLevel;

	//TAGDATA_JSONSTR tagDataJson;
	//tagDataJson.strName = pTag->strName;
	//tagDataJson.strVTQ = m_jsonWriter.write(jsonOneTag); // ���ܸ���
	//tagDataJson.strTime4Debug = szTime;	
	//m_vecTagDataCached.push_back(tagDataJson);

	string &strTagVTQ = m_jsonWriter.write(jsonOneTag);
	m_vecTagNameCached.push_back(pTag->strName);
	m_vecTagVTQCached.push_back(strTagVTQ);
	//m_vecTagTime4DebugCached.push_back(tagDataJson.strTime4Debug);




	return 0;
}

int CDataProcTask::OnStart()
{
	//m_vecTagDataCached.reserve(MAIN_TASK->m_mapName2Tag.size());
	m_vecTagNameCached.reserve(MAIN_TASK->m_mapName2Tag.size());
	m_vecTagVTQCached.reserve(MAIN_TASK->m_mapName2Tag.size());
	//m_vecTagTime4DebugCached.reserve(MAIN_TASK->m_mapName2Tag.size());

	return 0;
}

// ������ֹͣʱ
void CDataProcTask::OnStop()
{
}

int CDataProcTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CDataProcTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}


int CDataProcTask::ClearMDBDataOfDriver(char *szDriverName)
{
	int nRet = 0;

	// ��ո������µ����е�tag�㡣��ô�죿ÿ�������汣�����������ԣ�v��driver��device��block
	// ������Ϣ: driver:drivernameX
	/*
	nRet = m_redisRW.flushdb();
	if(nRet!= 0){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"memdb flushdb failed, ret:%d",nRet);
	}*/
	
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"���memdbδʵ��:%s", szDriverName); 
	return nRet;
}

// ������������tag������Ϊ����ֹͣ��BAD�������ر��ˣ�
int CDataProcTask::UpdateTagsOfDriverToBad(CMyDriver *pDriver)
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "UpdateTagsOfDriverToBad,driver:%s, has devicenum:%d", pDriver->m_strName.c_str(), pDriver->m_mapDevice.size());

	for (map<string, CMyDevice *>::iterator itDevice = pDriver->m_mapDevice.begin(); itDevice != pDriver->m_mapDevice.end(); itDevice++)
	{
		CMyDevice *pDevice = itDevice->second;
		for (vector<CDeviceTag *>::iterator itTag = pDevice->m_vecDeviceTag.begin(); itTag != pDevice->m_vecDeviceTag.end(); itTag++)
		{
			CDeviceTag *pTag = *itTag;
			// ��ַΪconnstatus�ı�����ʾ������״̬������������ڲ����������ܹ�����Ϊ***�ţ�
			// �����ֵ��Ϊ�գ�Ҳ���ܸı�Ϊ****
			pTag->lastVal.AssignTagValue(pTag->nDataTypeClass, &pTag->curVal);

			if (pTag->strAddress.compare("connstatus") == 0) // ����tag��
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, "0", 0); // δ����״̬
			}
			else if (pTag->strAddress.compare("enableconnect") == 0) // ����tag�㣬����
			{
			}
			else if (!pTag->strInitValue.empty())
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, pTag->strInitValue.c_str(), TAG_QUALITY_DRIVER_NOT_ALIVE);
			}
			else
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, TAG_QUALITY_DRIVER_NOT_ALIVE_STRING, TAG_QUALITY_DRIVER_NOT_ALIVE); // δ����״̬
			}

			string strJsonValue;
			pTag->GetJsonStringToDataMemDB(m_jsonWriter, szTime, strJsonValue);

			//TAGDATA_JSONSTR tagDataJson;
			//tagDataJson.strName = pTag->strName;
			//tagDataJson.strVTQ = strJsonValue;
			//m_vecTagDataCached.push_back(tagDataJson); // ���뵽Ҫ����tag���ݻ����������Ա����ͳһˢ��

			// Ҫ���µ�tag��
			m_vecTagNameCached.push_back(pTag->strName); // ����дredis��
			m_vecTagVTQCached.push_back(strJsonValue); //����дredis
			//m_vecTagTime4DebugCached.push_back(tagDataJson.strTime4Debug);

			if (pTag->nDataTypeClass != DATATYPE_CLASS_OTHER && pTag->bEnableAlarm)
			{
				// ����ʹ�����
				if (!pTag->vecAllAlarmTags.empty())
				{
					bool bAlarmChanged = false;
					m_eaProcessor.CheckAlarmsOfTag(pTag, bAlarmChanged); // ���ǵ�����״̬��������ЧҲ��Ҫ�жϱ����������������˱������жϡ��˴���DataProcTask�߳���ͬһ���̣߳�����Ҫ������
				}
			}
		}
	}
	return 0;
}

#ifndef _WIN32
#  ifdef __hpux
#include <sys/pstat.h>
#include <sys/types.h>
int FindPidByName(const char* szProcName, std::list<pid_t> &listPid)
{
    if (NULL == szProcName)
        return -1;

    static const size_t BURST = 10;
    struct pst_status pst[BURST];
    int i, count;
    int idx = 0; /* pstat index within the Process pstat context */
    while ((count = pstat_getproc(pst, sizeof(pst[0]),BURST,idx))>0)
    {
        /* got count this time.   process them */
        for (i = 0; i < count; i++)
        {
            if (ACE_OS::strstr(pst[i].pst_cmd, szProcName) != NULL)
            {
                listPid.push_back(pst[i].pst_pid);
            }
        }

        /*
            * now go back and do it again, using the next index after
            * the current 'burst'
            */
        idx = pst[count-1].pst_idx + 1;
        (void)memset(pst,0,BURST*sizeof(struct pst_status));
    }
    if (count == -1)
        return -1;
    return 0;
}
#  else  // (__hpux)
/**
 *  $(Desp)��δ�����ժ��linux��pidof���� .
 *  $(Detail) .
 *
 *  @param		-[in]  char * szProcName : ��������
 *  @param		-[out] std::list<int> &listPid : ����pid�б�
 *  @return		int.
 *
 *  @version	8/29/2013  shijunpu  Initial Version.
 */
int FindPidByName( const char* szProcName, std::list<pid_t> &listPid)
{
    if (NULL == szProcName)
        return -1;

    /* Open the /proc directory. */
    ACE_DIR *dir = ACE_OS::opendir("/proc");
    if (!dir)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "cannot open /proc");
        return -1;
    }

    struct ACE_DIRENT *d = NULL;
    /* Walk through the directory. */
    while ((d = ACE_OS::readdir(dir)) != NULL)
    {
        char *s = NULL;
        char *q = NULL;
        char path [PATH_MAX+1] = {0};
        char buf[PATH_MAX+1] = {0};
        int len = 0;
        int namelen = 0;
        int pid = 0;
        FILE *fp = NULL;

        /* See if this is a process */
        if ((pid = ACE_OS::atoi(d->d_name)) == 0)
            continue;
#ifdef __sun
        ACE_OS::snprintf(path, sizeof(path), "/proc/%s/psinfo", d->d_name);
        if ((fp = ACE_OS::fopen(path, "r")) != NULL)
        {
            psinfo_t psinfo;
            memset(&psinfo, 0x00, sizeof(psinfo));
            ACE_OS::fgets((char*)&psinfo, sizeof(psinfo), fp);
            ACE_OS::fclose(fp);
            if (ACE_OS::strstr(psinfo.pr_fname, szProcName) != NULL)
            {
                listPid.push_back(pid);
            }
        }
#else
        ACE_OS::snprintf(path, sizeof(path), "/proc/%s/stat", d->d_name);
          /* Read SID & statname from it. */
        if ((fp = ACE_OS::fopen(path, "r")) != NULL)
        {
            buf[0] = 0;
            ACE_OS::fgets(buf, sizeof(buf), fp);
            ACE_OS::fclose(fp);
            /* See if name starts with '(' */
            s = buf;
            while (*s != ' ') s++;
            s++;
            if (*s == '(')
            {
                /* Read program name. */
                q = ACE_OS::strrchr(buf, ')');
                if (q == NULL)
                    continue;
                s++;
            }
            else
            {
                q = s;
                while (*q != ' ') q++;
            }
            *q = 0;

            // ARM-LINUX��LINUX�½��������15���ֽڣ������Ľضϡ���pkmqttforwardserver��ض�Ϊ��pkmqttforwardse
            char szTrimProcName[16] = { 0 };
            strncpy(szTrimProcName, szProcName, sizeof(szTrimProcName) - 1); // Ҫ��ѯ״̬�Ľ��̣� ��ౣ��15���ֽ�
            char szTrimAliveProcName[16] = { 0 };
            strncpy(szTrimAliveProcName, s, sizeof(szTrimAliveProcName) - 1); //��ǰ�������еĽ��̣� ��ౣ��15���ֽ�

            if (!PKStringHelper::StriCmp(szTrimProcName, szTrimAliveProcName)) // ���ǰ15���ֽ���ȣ������ִ�Сд������Ϊ��ȣ�
            {
                listPid.push_back(pid);
            }
        }
#endif
    }
    ACE_OS::closedir(dir);

    return  0;
}
#  endif // (__hpux)
#endif

bool CDataProcTask::IsDriverAlive(string strDrvName)
{
	pid_t tempPid = 0;
#ifdef _WIN32
	tempPid = g_findKillProcess.GetProcessID(strDrvName.c_str(), TRUE);
#else
	std::list<pid_t> listPid;
    FindPidByName(strDrvName.c_str(), listPid);
	//�������ǵĽ���ֻ��������Ψһ��һ�������ȡ��һ������
	if (!listPid.empty())
		tempPid = *listPid.begin();
#endif
	if(tempPid > 0) // ���̴���
		return true;
	return false;
}

// ��������Ƿ��Ѿ����������δ��������������tag�������ΪBAD
void CDataProcTask::CheckDriverAlive(ACE_Time_Value tvNow)
{
	ACE_Time_Value tvSpan = tvNow - m_tvLastCheckDriverAlive;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if(nSpanAbsMS < 60000) // 60����1��
		return;

	m_tvLastCheckDriverAlive = tvNow;

#ifdef _WIN32
	// �����ѯ������Ϣ�ٶ��Ǻ����ģ�Ϊ�˼ӿ��ٶ���Ҫһ�β�����еĽ���ͳһ����
	map<string, DWORD> mapProcessName2Id;
	g_findKillProcess.GetProcessInfo(mapProcessName2Id); // �õ����еĽ�����Ϣ, �����Ѿ���ΪСд

	map<string, CMyDriver *>::iterator itDrv = MAIN_TASK->m_mapName2Driver.begin();
	//bool bNeedCommit = false;
	for (; itDrv != MAIN_TASK->m_mapName2Driver.end(); itDrv++)
	{
		string strDrvName = itDrv->first;
		if (strDrvName.compare(DRIVER_NAME_SIMULATE) == 0 || strDrvName.compare(DRIVER_NAME_MEMORY) == 0)
			continue;

		CMyDriver *pDriver = itDrv->second;
		string strDrvWithExtName = strDrvName + ".exe";
		transform(strDrvWithExtName.begin(), strDrvWithExtName.end(), strDrvWithExtName.begin(), ::tolower); // �����Ѿ���ΪСд
		if (mapProcessName2Id.find(strDrvWithExtName) == mapProcessName2Id.end())
		{
			UpdateTagsOfDriverToBad(pDriver);
			//bNeedCommit = true;
		}
	}
	mapProcessName2Id.clear();
#else
	map<string, CMyDriver *>::iterator itDrv = MAIN_TASK->m_mapName2Driver.begin();
	bool bNeedCommit = false;
	for (; itDrv != MAIN_TASK->m_mapName2Driver.end(); itDrv++)
	{
		string strDrvName = itDrv->first;
		CMyDriver *pDriver = itDrv->second;

		if (!IsDriverAlive(strDrvName))
		{
			UpdateTagsOfDriverToBad(pDriver);
			//bNeedCommit = true;
		}
	}
#endif
	//if (bNeedCommit)
	//	MAIN_TASK->m_redisRW0.commit();
}

void CDataProcTask::CheckAndCommitTagData(ACE_Time_Value tvNow)
{
	CheckDriverAlive(tvNow); // ��������Ƿ���������

	ACE_Time_Value tvSpan = tvNow - m_tvLastCommitTagData;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if (nSpanAbsMS < 50)
		return;

	m_tvLastCommitTagData = tvNow;

	if(m_vecTagNameCached.size() <= 0)
		return;

	m_nCommitTimesRecent ++;
	char szTmp[32] = {0};
	sprintf(szTmp, "%d", m_vecTagNameCached.size());
	if(m_strCommitDetail.length() == 0)
		m_strCommitDetail = szTmp;
	else
		m_strCommitDetail = m_strCommitDetail + "," + szTmp;

    int nCommitSuccessNum = 0;
    int nCommitFailNum = 0;
	//vector<string> vecKeys;
	//vector<string> vecValues;
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
    int nRet = 0;
    nRet = MAIN_TASK->m_redisRW0.mset(m_vecTagNameCached, m_vecTagVTQCached);
	if (nRet == 0)
		nCommitSuccessNum += m_vecTagNameCached.size();
	else
		nCommitFailNum += m_vecTagNameCached.size();
	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_UINT64 nConsumeMS = (tvEnd - tvBegin).get_msec();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "commit %d tags, success:%d, fail:%d, consume:%d MS", nCommitSuccessNum + nCommitFailNum, nCommitSuccessNum, nCommitFailNum, nConsumeMS);

	//m_vecTagDataCached.clear();
	m_vecTagNameCached.clear();
	m_vecTagVTQCached.clear();
	//m_vecTagTime4DebugCached.clear();
}

void CDataProcTask::DoCalcTags(ACE_Time_Value tvNow)
{
	ACE_Time_Value tvSpan = tvNow - m_tvLastCaclTags;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if(nSpanAbsMS < 1000) // ���ÿ���м���һ��!
		return;

	m_tvLastCaclTags = tvNow;
	DoCalcTags(tvNow, MAIN_TASK->m_mapName2CalcTagLevel2);	// �����������
	DoCalcTags(tvNow, MAIN_TASK->m_mapName2CalcTagLevel3);	// ������������
}

// �����������,�����������ۼ�ʱ�䣬�����ۼƴ���
void CDataProcTask::DoCalcTags(ACE_Time_Value tvNow, map<string, CCalcTag*> &mapName2CalcTag)
{
	// ����/��������
	map<string, CCalcTag *>::iterator itCalcTag = mapName2CalcTag.begin();
	for (itCalcTag; itCalcTag != mapName2CalcTag.end(); itCalcTag ++) // ��������
	{
		const string &strTagName = itCalcTag->first;
		CCalcTag *pCalcTag = itCalcTag->second;
		CTagExprCalc &exprCalc = pCalcTag->exprCalc;
		ExprNode *pExprNode = exprCalc.GetRootNode();
		
		int nQuality = 0;
		double dbValue = 0.0;
		try{
			dbValue = exprCalc.DoCalc(pExprNode);
			nQuality = 0;
		}
		catch (TagException e)
		{
			pCalcTag->nCalcErrorNumLog++;
			ACE_Time_Value tvSpan = pCalcTag->tvLastLogCalcError - tvNow;
			if (tvSpan.sec() > 60)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, calc expr(%s), ���60�����������:%d, calc except:%s,errcode:%d", pCalcTag->strName.c_str(), exprCalc.m_strExpression.c_str(), pCalcTag->nCalcErrorNumLog, e.what(), e.errcode());
				pCalcTag->tvLastLogCalcError = tvNow;
				pCalcTag->nCalcErrorNumLog = 0;
			}
			nQuality = -20000;// e.errcode();
		}
		catch(std::exception e) // ����Ϊbad�����߳�0������Ҫ������ֵ�ʹ�������
		{
			pCalcTag->nCalcErrorNumLog++;
			ACE_Time_Value tvSpan = pCalcTag->tvLastLogCalcError - tvNow;
			if (tvSpan.sec() > 60)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, calc expr(%s), ���60�����������:%d, calc except:%s", pCalcTag->strName.c_str(), exprCalc.m_strExpression.c_str(), pCalcTag->nCalcErrorNumLog, e.what());
				pCalcTag->tvLastLogCalcError = tvNow;
				pCalcTag->nCalcErrorNumLog = 0;
			}
			nQuality = -10000;
		}

		char szTagValue[256];	
		if (pCalcTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME || pCalcTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
		{
			if (nQuality == 0)
			{
				CTagAccumulate *pTagAccu = (CTagAccumulate *)pCalcTag;
				bool bSatisfied = false;
				if (fabs(dbValue) < EPSINON)
				{
					bSatisfied = true;
				}

				if (pCalcTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME)
				{
					int nTagValue = 0;
					if (pTagAccu->bLastSatified) // ֻҪ�ϴ�������������ʱ���ۼ�
					{
						ACE_Time_Value tvCurTagTime = ACE_OS::gettimeofday(); // to do ??????????????????????
						ACE_Time_Value tvTimeSpan = tvCurTagTime - pTagAccu->tvLastValue;
						if (pTagAccu->nDataTypeClass == DATATYPE_CLASS_INTEGER)
							pTagAccu->curVal.nValue = pTagAccu->curVal.nValue + tvTimeSpan.sec();
						else 
							pTagAccu->curVal.dbValue = pTagAccu->curVal.dbValue + tvTimeSpan.sec();
					}
				}
				else if (pCalcTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
				{
					if (!pTagAccu->bLastSatified && bSatisfied) // ֻҪ�ϴ��������������ۼ�
					{
						if (pTagAccu->nDataTypeClass == DATATYPE_CLASS_INTEGER)
							pTagAccu->curVal.nValue += 1;
						else
							pTagAccu->curVal.dbValue += 1;
					}
				}
			}

			pCalcTag->lastVal.nQuality = pCalcTag->curVal.nQuality;
			pCalcTag->curVal.nQuality = nQuality;
		}
		else
		{

			if (pCalcTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
			{
				pCalcTag->curVal.nValue = (int)dbValue;
				sprintf(szTagValue, "%ld", pCalcTag->curVal.nValue);
			}
			else if (pCalcTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			{
				pCalcTag->curVal.dbValue = dbValue;
				sprintf(szTagValue, "%f", dbValue);
			}
			else // TEXT
			{
				sprintf(szTagValue, "%f", dbValue);
			}
			pCalcTag->lastVal.AssignTagValue(pCalcTag->nDataTypeClass, &pCalcTag->curVal);
			pCalcTag->curVal.AssignStrValueAndQuality(pCalcTag->nDataTypeClass, szTagValue, nQuality);
		}

		// ����ʹ�����
		if ((!pCalcTag->vecAllAlarmTags.empty()) && pCalcTag->bEnableAlarm)
		{
			bool bAlarmChanged = false;
			m_eaProcessor.CheckAlarmsOfTag(pCalcTag, bAlarmChanged);
			//if (bAlarmChanged)
			//	m_eaProcessor.ReCalcTagAlarmStatus(pCalcTag); //  // ��Ϊδ֪ԭ���ڴ���tag��nMaxLevel��vecAlarm�ṹ��һ�£����������������
		}
		// ת����pkcserver TRDataProcTask���д��������ڴ����ݿ⣩,����ֻ����tag��ֵ��ʱ��Ȼ�������,����������Ե������д���

		unsigned int nMSec = 0;
		int nSec = PKTimeHelper::GetHighResTime(&nMSec);
		ConvertTagData2JsonAndCache(pCalcTag, nSec, nMSec, nQuality);
		m_nValidTagCountRecent ++;
	}
}
