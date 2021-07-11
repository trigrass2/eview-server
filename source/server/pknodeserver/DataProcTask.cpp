/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  shijunpu  Initial Version
 *  @version	9/17/2012  shijunpu  数据块类型修改为字符串类型，并且修改增加数据块时的接口
 *  @version	3/1/2013   shijunpu  增加由点生成块的逻辑，当设备下没有配置数据块时，直接由点生成数据块 .
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
 *  构造函数.
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

	m_tvRecentRecvPack = m_tvFirstRecvPack = ACE_OS::gettimeofday();	// 上次打印
	m_nTimeoutPackNumFromStart = 0;
	m_nTimeoutPackNumRecent = 0;
	m_nTimeoutMaxSecFromStart = 0;
	m_nTimeoutMaxSecRecent = 0;

	m_nRecvTagCountRecent = 0;	// 一段时间内收到的驱动来的数据总tag数
	m_nValidTagCountRecent = 0;	// 一段时间内收到的驱动来的数据有效tag数
	m_strMinTagTimestampRecent = "";	// 最近一段时间的最小时间戳
	m_strMaxTagTimestampRecent = ""; // 最近一段时间的最大时间戳
	m_nCommitTagNumRecent = 0;		// 最近一段时间提交的tag数量
	m_nCommitTimesRecent = 0;		// 最近一段时间提交的次数
	m_strCommitDetail = "";			// 每次提交的数量

	m_nQueRecBufferLen = MAIN_TASK->m_nNodeServerQueSize / 2;
	if (m_nQueRecBufferLen < 1 * 1024 * 1024) // 1M
		m_nQueRecBufferLen = 1 * 1024 * 1024;
	if (m_nQueRecBufferLen > 10 * 1024 * 1024)
		m_nQueRecBufferLen = 10 * 1024 * 1024;
	m_pQueRecBuffer = new char[m_nQueRecBufferLen];

	m_nQueTagVTQBufferLen = m_nQueRecBufferLen; // 一次发送多个TAG点的内存指针
	m_pQueTagVTQBuffer = new char[m_nQueTagVTQBufferLen]; // 一次发送多个TAG点的记录内存大小
}

/**
 *  析构函数.
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

	// 检查包有没有超时
	ACE_Time_Value tvPackSpan = tvNow - tvPack;
	if(labs(tvPackSpan.sec()) > 2) // 发送和接收时间戳大于2秒！
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
	if(nSecSpan > 30) // 超时，需要打印消息了
	{
#ifdef _WIN32
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "从启动至今共收到:%d包, 共计tag点:%d个, 无效tag点:%d个",
			m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// 每次提交的数量);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "最近:%d秒: 接收 %d 个包,平均 %f个包/秒. 提交次数:%d, tag数量:%d个,平均每秒%.f个, tag最小时间:%s, 最大时间:%s. ",
			nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
			 m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());	
#else
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recv:%d packets from start, total tagnum:%d, invalid tagnum:%d",
            m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// 每次提交的数量);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recent:%d seconds: recv %d packets,avg %f packets/second. commit count:%d, tagnum:%d,avg %.f tagnum/second, tag mintime:%s, maxtime:%s. ",
            nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
             m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());
#endif
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "最近:%d秒: 提交详细:%s", nSecSpan, m_strCommitDetail.c_str());	

		if(m_nTimeoutPackNumRecent > 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "!!性能(performance)问题: 最近%d秒内，接收和发送间隔大于>2秒(最大间隔：%d秒）有%d个包. 程序启动以来, 超过2秒(最大间隔：%d秒）共 %d 个包",
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
 *  虚函数，继承自ACE_Task.
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

	// 启动后，先清空所有的遗留的历史数据
	//g_logger.LogMessage(PK_LOGLEVEL_INFO, "清空驱动发送来的数据队列");
	while(true)
	{
		int nResult = g_pQueData2NodeSrv->DeQueue(szRecBuffer, m_nQueRecBufferLen, &nRecLen);
		if(nResult != PK_SUCCESS)
			break;
	}

	while(!m_bStop)
	{
		bool bFoundRecord = false;
		// 获取到最后一个队列内容，前面的记录全部都抛掉
        int nResult = g_pQueData2NodeSrv->DeQueue(szRecBuffer, m_nQueRecBufferLen, &nRecLen); // cpu is low: this step
		if(nResult != PK_SUCCESS)
		{
			ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // 收到包的时间
			DoCalcTags(tvNow); // 如果不调用，那么驱动停止后，二次变量将无法更新！
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

		ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // 收到包的时间
		string strrecbuffer = szRecBuffer;
		szRecBuffer[nRecLen] = '\0';
		char *pCurBuf = szRecBuffer;

		// 解析包的时间戳
		int nPackTimeStampSec = 0;
		int nPackTimeStampMSec = 0;
		// 解析发送的时间（秒和毫秒）
		memcpy(&nPackTimeStampSec, pCurBuf, sizeof(unsigned int));
		pCurBuf += sizeof(unsigned int);
		memcpy(&nPackTimeStampMSec, pCurBuf, sizeof(unsigned short));
		pCurBuf += sizeof(unsigned short);
		ACE_Time_Value tvPack;
		tvPack.sec(nPackTimeStampSec);
		tvPack.usec(nPackTimeStampMSec * 1000);
		PrintDataPackStatInfo(tvNow, tvPack);

		int nActionType;
		memcpy(&nActionType, pCurBuf, sizeof(int)); // 第一个字节是动作类型
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
		else if(nActionType == ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ) // push在增加GroupTask时用的是列表方式
		{
			ACE_Time_Value tvTimeStamp;
			unsigned int	nSec = 0;
			unsigned int	mSec = 0;
			int				nCurQuality = 0;
			int	nDataType = 0;

			// 解析tag个数
			int nTagNum = 0;
			memcpy(&nTagNum, pCurBuf, sizeof(int));
			pCurBuf += sizeof(int);

			for(int iTag = 0; iTag < nTagNum; iTag ++)
			{
				m_nRecvTagCountRecent ++;

				// 解析tagname的长度
				memcpy(&uTagNameLen, pCurBuf, sizeof(int));
				if(pCurBuf > szRecBuffer + nRecLen)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pbserver,process tagnum:%d, no.%d buffer not enough(quelen:%d)", nTagNum, iTag, nRecLen);
					break;
				}
				// 解析tagname
				if(uTagNameLen > sizeof(szTagName))
					uTagNameLen = sizeof(szTagName) - 1;
				pCurBuf += sizeof(int);
				memcpy(szTagName, pCurBuf, uTagNameLen);
				szTagName[uTagNameLen] = '\0';
				pCurBuf += uTagNameLen;

				//if (strcmp(szTagName, "一跑道.状态") == 0)
				//{
				//	printf("TagName:%s,data recv\n", szTagName);
				//}
				// 解析tag的数据类型
				memcpy(&nDataType, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);

				// 解析tagdata的length
				memcpy(&uTagDataLen, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);
				if(uTagDataLen > m_nQueTagVTQBufferLen) // 大于10M，认为数据非法
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pbserver, dataproc error, byes:%d, tagnum:%d, cur:%d", nRecLen, nTagNum, iTag);
					break;
				}
				
				// 解析tagdata
				memcpy(szTagData, pCurBuf, uTagDataLen);
				szTagData[uTagDataLen] = '\0';
				pCurBuf += uTagDataLen;

				// 解析tag的timestamp
				memcpy(&nSec, pCurBuf, sizeof(unsigned int));
				pCurBuf += sizeof(unsigned int);
				memcpy(&mSec, pCurBuf, sizeof(unsigned short));
				pCurBuf += sizeof(unsigned short);
				tvTimeStamp.sec(nSec);
				tvTimeStamp.usec(mSec * 1000);

				// 解析tag的质量
				memcpy(&nCurQuality, pCurBuf, sizeof(int));
				pCurBuf += sizeof(int);

				//----一个tag解析完毕

				// 下面是需要后续计算处理的
				map<string, CDataTag *> &mapName2Tag = MAIN_TASK->m_mapName2Tag;
				map<string, CDataTag *>::iterator itMap = mapName2Tag.find(szTagName);
				if (itMap == mapName2Tag.end())
				{
					if (strstr(szTagName, ".connstatus") == NULL) // 状态点内置的，不需要配置也不需要提示
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

				// 将上次的值保存到lastXXX中，当前保存在curVal中
				pTag->lastVal.AssignTagValue(pTag->nDataTypeClass, &pTag->curVal);
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, strTagValue, nCurQuality);
				if(pTag->nDataTypeClass != DATATYPE_CLASS_OTHER && pTag->bEnableAlarm)
				{
					// 计算和处理报警
					if (!pTag->vecAllAlarmTags.empty())
					{
						bool bAlarmChanged = false;
						m_eaProcessor.CheckAlarmsOfTag(pTag, bAlarmChanged);
						//if (bAlarmChanged)
						//	m_eaProcessor.ReCalcTagAlarmStatus(pTag); //  // 因为未知原因，内存中tag的nMaxLevel和vecAlarm结构不一致，因此这里索性算算
					}
				}

				if (pTag->bEnableRange)
				{

				}
				// 转发到pkcserver TRDataProcTask进行处理（更新内存数据库）,这里只更新tag的值和时间等基本属性,报警相关属性单独进行处理
				ConvertTagData2JsonAndCache(pTag, nSec, mSec, nCurQuality); 

				m_nValidTagCountRecent ++;
			} 
		}
		else if(nActionType == ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE || nActionType == ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV) // 这两类数据长度都不大
		{ // 初始化时需要设置初始tag点，或者增加list成员时会调用
			int nKeyLen = 0;
			int nValueLen = 0;
			char szKey[PK_NAME_MAXLEN * 2];
			char szValue[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH];  // 这两类数据长度都不大，用4K就够了

			// 解析tagname
			memcpy(&nKeyLen, pCurBuf, sizeof(int));
			if(nKeyLen >= sizeof(szKey))
				nKeyLen = sizeof(szKey) - 1;
			pCurBuf += sizeof(int);
			memcpy(szKey, pCurBuf, nKeyLen);
			szKey[nKeyLen] = '\0';
			pCurBuf += nKeyLen;

			// 解析tagdata的length
			memcpy(&nValueLen, pCurBuf, sizeof(int));
			pCurBuf += sizeof(int);

			// 解析tagdata
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
	str+=pattern;     //扩展字符串以方便操作  
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
	char szStrValue[PK_NAME_MAXLEN + 1] = { 0 }; // 只有数值类型，不会占用很多空间

	// 将数据值由字符串转换为二进制类型
	stringstream ssValue;
	switch(nTagDataType){
	case TAG_DT_BOOL:		// 1 bit value
		{
			unsigned char ucValue = (*(unsigned char *)(szBinData));
			ucValue = pTag->CalcRangeValue((int)ucValue);
			PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", (ucValue & 0x01) != 0 ); // 取nTagOffsetOfOneByte位

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
	// 将数据值由字符串转换为二进制类型
	switch (nTagDataType){
	case TAG_DT_BOOL:		// 1 bit value
	{
		unsigned char ucValue = (*(unsigned char *)(szBinData));
		nValue = ucValue & 0x01; // 取nTagOffsetOfOneByte位
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
	// 将数据值由字符串转换为二进制类型
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


// 将tag点转换为json字符串
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
	else // 其他质量为BAD的情况 ***
	{
		jsonOneTag[JSONFIELD_VALUE] = TAG_QUALITY_UNKNOWN_REASON_STRING;
		jsonOneTag[JSONFIELD_MSG] = TAG_QUALITY_UNKNOWN_REASON_TIP;
	}
	jsonOneTag[JSONFIELD_TIME] = szTime;
	jsonOneTag[JSONFIELD_QUALITY] = nQuality;
	jsonOneTag[JSONFIELD_MAXALARM_LEVEL] = pTag->m_curAlarmInfo.m_nMaxAlarmLevel;

	//TAGDATA_JSONSTR tagDataJson;
	//tagDataJson.strName = pTag->strName;
	//tagDataJson.strVTQ = m_jsonWriter.write(jsonOneTag); // 性能更高
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

// 本任务停止时
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

	// 清空该驱动下的所有的tag点。怎么办？每个点下面保存了三个属性：v、driver和device、block
	// 驱动信息: driver:drivernameX
	/*
	nRet = m_redisRW.flushdb();
	if(nRet!= 0){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"memdb flushdb failed, ret:%d",nRet);
	}*/
	
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"清空memdb未实现:%s", szDriverName); 
	return nRet;
}

// 更新驱动所有tag点质量为驱动停止了BAD（驱动关闭了）
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
			// 地址为connstatus的变量表示是连接状态，这个变量是内部变量，不能够被改为***号！
			// 如果初值不为空，也不能改变为****
			pTag->lastVal.AssignTagValue(pTag->nDataTypeClass, &pTag->curVal);

			if (pTag->strAddress.compare("connstatus") == 0) // 内置tag点
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, "0", 0); // 未连接状态
			}
			else if (pTag->strAddress.compare("enableconnect") == 0) // 内置tag点，不动
			{
			}
			else if (!pTag->strInitValue.empty())
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, pTag->strInitValue.c_str(), TAG_QUALITY_DRIVER_NOT_ALIVE);
			}
			else
			{
				pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, TAG_QUALITY_DRIVER_NOT_ALIVE_STRING, TAG_QUALITY_DRIVER_NOT_ALIVE); // 未连接状态
			}

			string strJsonValue;
			pTag->GetJsonStringToDataMemDB(m_jsonWriter, szTime, strJsonValue);

			//TAGDATA_JSONSTR tagDataJson;
			//tagDataJson.strName = pTag->strName;
			//tagDataJson.strVTQ = strJsonValue;
			//m_vecTagDataCached.push_back(tagDataJson); // 加入到要更新tag数据缓存起来，以便后续统一刷新

			// 要更新的tag点
			m_vecTagNameCached.push_back(pTag->strName); // 批量写redis用
			m_vecTagVTQCached.push_back(strJsonValue); //批量写redis
			//m_vecTagTime4DebugCached.push_back(tagDataJson.strTime4Debug);

			if (pTag->nDataTypeClass != DATATYPE_CLASS_OTHER && pTag->bEnableAlarm)
			{
				// 计算和处理报警
				if (!pTag->vecAllAlarmTags.empty())
				{
					bool bAlarmChanged = false;
					m_eaProcessor.CheckAlarmsOfTag(pTag, bAlarmChanged); // 考虑到连接状态和质量无效也需要判断报警，因此这里进行了报警的判断。此处和DataProcTask线程在同一个线程，不需要加锁！
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
 *  $(Desp)这段代码是摘自linux的pidof函数 .
 *  $(Detail) .
 *
 *  @param		-[in]  char * szProcName : 进程名称
 *  @param		-[out] std::list<int> &listPid : 返回pid列表
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

            // ARM-LINUX，LINUX下进程名最多15个字节，超过的截断。如pkmqttforwardserver则截断为：pkmqttforwardse
            char szTrimProcName[16] = { 0 };
            strncpy(szTrimProcName, szProcName, sizeof(szTrimProcName) - 1); // 要查询状态的进程， 最多保留15个字节
            char szTrimAliveProcName[16] = { 0 };
            strncpy(szTrimAliveProcName, s, sizeof(szTrimAliveProcName) - 1); //当前正在运行的进程， 最多保留15个字节

            if (!PKStringHelper::StriCmp(szTrimProcName, szTrimAliveProcName)) // 最多前15个字节相等，不区分大小写，则认为相等！
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
	//对于我们的进程只可能启动唯一的一个，因此取第一个即可
	if (!listPid.empty())
		tempPid = *listPid.begin();
#endif
	if(tempPid > 0) // 进程存在
		return true;
	return false;
}

// 检查驱动是否已经启动。如果未启动则设置所有tag点的质量为BAD
void CDataProcTask::CheckDriverAlive(ACE_Time_Value tvNow)
{
	ACE_Time_Value tvSpan = tvNow - m_tvLastCheckDriverAlive;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if(nSpanAbsMS < 60000) // 60秒检查1次
		return;

	m_tvLastCheckDriverAlive = tvNow;

#ifdef _WIN32
	// 这里查询进程信息速度是很慢的，为了加快速度需要一次查出所有的进程统一处理
	map<string, DWORD> mapProcessName2Id;
	g_findKillProcess.GetProcessInfo(mapProcessName2Id); // 得到所有的进程信息, 名称已经变为小写

	map<string, CMyDriver *>::iterator itDrv = MAIN_TASK->m_mapName2Driver.begin();
	//bool bNeedCommit = false;
	for (; itDrv != MAIN_TASK->m_mapName2Driver.end(); itDrv++)
	{
		string strDrvName = itDrv->first;
		if (strDrvName.compare(DRIVER_NAME_SIMULATE) == 0 || strDrvName.compare(DRIVER_NAME_MEMORY) == 0)
			continue;

		CMyDriver *pDriver = itDrv->second;
		string strDrvWithExtName = strDrvName + ".exe";
		transform(strDrvWithExtName.begin(), strDrvWithExtName.end(), strDrvWithExtName.begin(), ::tolower); // 名称已经变为小写
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
	CheckDriverAlive(tvNow); // 检查驱动是否还在启动中

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
	if(nSpanAbsMS < 1000) // 最多每秒中计算一次!
		return;

	m_tvLastCaclTags = tvNow;
	DoCalcTags(tvNow, MAIN_TASK->m_mapName2CalcTagLevel2);	// 计算二级变量
	DoCalcTags(tvNow, MAIN_TASK->m_mapName2CalcTagLevel3);	// 计算三级变量
}

// 计算二级变量,包括：计算累计时间，计算累计次数
void CDataProcTask::DoCalcTags(ACE_Time_Value tvNow, map<string, CCalcTag*> &mapName2CalcTag)
{
	// 二级/三级变量
	map<string, CCalcTag *>::iterator itCalcTag = mapName2CalcTag.begin();
	for (itCalcTag; itCalcTag != mapName2CalcTag.end(); itCalcTag ++) // 二级变量
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
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, calc expr(%s), 最近60秒计算错误次数:%d, calc except:%s,errcode:%d", pCalcTag->strName.c_str(), exprCalc.m_strExpression.c_str(), pCalcTag->nCalcErrorNumLog, e.what(), e.errcode());
				pCalcTag->tvLastLogCalcError = tvNow;
				pCalcTag->nCalcErrorNumLog = 0;
			}
			nQuality = -20000;// e.errcode();
		}
		catch(std::exception e) // 质量为bad，或者除0错。还是要继续赋值和处理报警的
		{
			pCalcTag->nCalcErrorNumLog++;
			ACE_Time_Value tvSpan = pCalcTag->tvLastLogCalcError - tvNow;
			if (tvSpan.sec() > 60)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, calc expr(%s), 最近60秒计算错误次数:%d, calc except:%s", pCalcTag->strName.c_str(), exprCalc.m_strExpression.c_str(), pCalcTag->nCalcErrorNumLog, e.what());
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
					if (pTagAccu->bLastSatified) // 只要上次满足条件，则时间累计
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
					if (!pTagAccu->bLastSatified && bSatisfied) // 只要上次满足条件，则累计
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

		// 计算和处理报警
		if ((!pCalcTag->vecAllAlarmTags.empty()) && pCalcTag->bEnableAlarm)
		{
			bool bAlarmChanged = false;
			m_eaProcessor.CheckAlarmsOfTag(pCalcTag, bAlarmChanged);
			//if (bAlarmChanged)
			//	m_eaProcessor.ReCalcTagAlarmStatus(pCalcTag); //  // 因为未知原因，内存中tag的nMaxLevel和vecAlarm结构不一致，因此这里索性算算
		}
		// 转发到pkcserver TRDataProcTask进行处理（更新内存数据库）,这里只更新tag的值和时间等基本属性,报警相关属性单独进行处理

		unsigned int nMSec = 0;
		int nSec = PKTimeHelper::GetHighResTime(&nMSec);
		ConvertTagData2JsonAndCache(pCalcTag, nSec, nMSec, nQuality);
		m_nValidTagCountRecent ++;
	}
}
