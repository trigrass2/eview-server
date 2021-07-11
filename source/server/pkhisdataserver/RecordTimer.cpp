#include "RecordTimerTask.h"

#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "ace/High_Res_Timer.h"
#include <ace/Default_Constants.h>
#include <map>
#include <vector>

#include "json/json.h"

#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "pkdata/pkdata.h"	

#include "MainTask.h"
#include "RecordTimer.h"

extern CPKLog g_logger;
extern void StringReplace(string &strBase, string strSrc, string strDes);
extern string SubString(string &src, int nLen);
using namespace mongo;

CRecordTimer::CRecordTimer()
{
	m_pTimerTask = NULL;
	m_bFirstTimerCome = true;
}

CRecordTimer::~CRecordTimer()
{
}

void CRecordTimer::SetTimerTask(CRecordTimerTask *pTimerTask)
{
	m_pTimerTask = pTimerTask;
}

int  CRecordTimer::handle_timeout(const  ACE_Time_Value &current_time, const   void  *act)
{
	// �Ƿ��ǵ�һ�ζ�ʱ���������ǵ�һ�Σ���ô���ܱ������ݣ���Ϊ������������
	if (m_bFirstTimerCome)
	{
		m_bFirstTimerCome = false;
		return 0;
	}

	char szCurTime[32];
	PKTimeHelper::HighResTime2String(szCurTime, sizeof(szCurTime), current_time.sec(), current_time.msec());
	//g_logger.LogMessage(PK_LOGLEVEL_INFO, "record Timer::handle_timeout,time:%s", szCurTime);

	MAIN_TASK->m_mutexMongo.acquire(); // mongo�����ܱ�����߳�ͬʱ���ʣ���˼��������쳣
	ProcessTags(current_time);
	MAIN_TASK->m_mutexMongo.release();
	return  0;
}

int  CRecordTimer::Start()
{
	ACE_Time_Value tvInterval;
	tvInterval.msec(m_nIntervalMS);  //msec(m_nCyclePeriodMS);
	ACE_Time_Value tvStart(0);          // = ACE_OS::gettimeofday();  ��1970��1��1�� 0:00:00�뿪ʼ��ʱ��

	ACE_Timer_Queue *timer_queue = m_pTimerTask->m_pReactor->timer_queue();
	if (0 != timer_queue)
		timer_queue->schedule(this, NULL, tvStart, tvInterval);

	//m_pTimerTask->m_pReactor->schedule_timer(this, NULL, tvStart, tvInterval); ���ֶ�ʱ�����޷�������0��0��0�뿪ʼ��ʱ

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "%s, started timer, interval: %d second, tagnum:%d", "record", m_nIntervalMS/1000, m_vecTags.size());
	return  0;
}

int  CRecordTimer::Stop()
{
	m_pTimerTask->m_pReactor->cancel_timer(this);
	return  0;
}

/**
*  ���ܣ��������Tag��Ľ���������MongoDb����д�빤��.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CRecordTimer::ProcessTags(ACE_Time_Value tvTimer) // yyyy-mm-dd
{
	char szCurTime[32];
	PKTimeHelper::Time2String(szCurTime, sizeof(szCurTime), tvTimer.sec());

	int nTagNum_NoRealData = 0;
	int nTagNum_TimeFormatError = 0;
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	int nTag = 0;
	vector<string> vecTagValue;
	m_pTimerTask->m_redisRW.mget(m_vecTagName, vecTagValue);
	if (m_vecTagName.size() != vecTagValue.size())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RecordTimer::ProcessTags, interval:%d seconds, mget(%d tags), return data count:%d, NOT EQUAL", m_nIntervalMS / 1000, m_vecTagName.size(), vecTagValue.size());
		return -1;
	}

	vector<BSONObj> vecBsonObj;
	for (int iTag = 0; iTag < m_vecTagName.size(); iTag++)
	{
		string &strTagName = m_vecTagName[iTag];
		string &strTagVTQ = vecTagValue[iTag];
		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "begin processing tag:%s", pTag->strTagName.c_str());

		if (strTagVTQ.empty()) // ���ܻ��ȡ�����ַ���
		{
			nTagNum_NoRealData++;
			// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����%s������tag���ݵ�δ��ȡ��", pTag->strTagName.c_str());
			continue;
		}
		nTag++;
		//printf("tagname:%s, No.:%d, value:%s\n", pTag->strTagName.c_str(), nTag, curStr.c_str());

		string strNewRecTime = szCurTime;
		string strNewRecValue = "???";
		int nNewRecQuality = -50000;
		Json::Reader reader;
		Json::Value root;
		if(reader.parse(strTagVTQ, root, false))
		{
			//�ж����ݱ�������:int \ double \ string
			if (!root[REC_ATTR_VALUE].isNull())
				strNewRecValue = root[REC_ATTR_VALUE].asString();
			if (!root[REC_ATTR_QUALITY].isNull())
				nNewRecQuality = atoi(root[REC_ATTR_QUALITY].asString().c_str());
		} 
		RealData_TO_Table_Record(vecBsonObj, strTagName, strNewRecTime, strNewRecValue, nNewRecQuality);
	}
	vecTagValue.clear();

	// ���� strYYMMDDHHMMSS.XXXX �õ� YYMMDDHH
	string strCurTime = szCurTime;
	string strYYYYMMDD = SubString(strCurTime, TIMELEN_YYYYMMDD);
	string strTableName = MONGO_DB_TABLE_RECORD_PREFIX + strYYYYMMDD;
	StringReplace(strTableName, "-", "_");

	try {
		m_pTimerTask->m_mongoConn.insert(strTableName, vecBsonObj);
		vecBsonObj.clear();
	}
	catch (DBException& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record DBException(table:%s, count:%d): %s", strTableName.c_str(), vecBsonObj.size(), e.what());
		vecBsonObj.clear(); 
		return -1;
	}
	catch (std::exception& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record StdException(table:%s, count:%d): %s", strTableName.c_str(), vecBsonObj.size(), e.what());
		vecBsonObj.clear(); 
		return -2;
	}
	catch (...) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record AllException(table:%s, count:%d): NULL", strTableName.c_str(), vecBsonObj.size());
		vecBsonObj.clear(); 
		return -3;
	}

	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvEnd - tvBegin;
	float fSec = tvSpan.get_msec() / 1000.0f;
	
	if (nTagNum_NoRealData != 0 || nTagNum_TimeFormatError != 0)
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "==%s,interval:%d s, tagnum:%d. NO DATA Tagcount:%d, wrong time format tagcount:%d. consumed:%f second", 
		"WRITE RECORD", m_nIntervalMS/1000, m_vecTags.size(), nTagNum_NoRealData, nTagNum_TimeFormatError, fSec);
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "==%s,interval:%d s, tagnum:%d, consumed:%f second", "WRITE RECORD", m_nIntervalMS/1000, m_vecTags.size(), fSec);
	return 0;
}

// ������д�뵽mongodb�� record_yyyy-mm-dd ��
int CRecordTimer::RealData_TO_Table_Record(vector<BSONObj> &vecBsonObj, string &strTagName, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality)
{
	string strTagNameUtf8 = PKCodingConv::AnsiToUtf8(string(strTagName));
	// string strYYMMDDHHMMSS = SubString(pRawRecord->strTime, TIMELEN_YYYYMMDD_HHMMSS); // ��Ҫ����
	BSONObjBuilder builder;
	builder.append(REC_ATTR_TAGNAME, strTagNameUtf8);
	builder.append(REC_ATTR_TIME, strNewRecTime.c_str());
	builder.append(REC_ATTR_VALUE, strNewRecValue);
	builder.append(REC_ATTR_QUALITY, nNewRecQuality);
	vecBsonObj.push_back(builder.obj());
	//BSONObjBuilder whereBuilder;
	//whereBuilder.append(REC_ATTR_TAGNAME, strTagNameUtf8.c_str()).append(REC_ATTR_TIME, strNewRecTime.c_str());
	//try{
	//	m_pTimerTask->m_mongoConn.update(strTableName, whereBuilder.obj(), builder.obj(), true);
	//}
	//catch (DBException& e) {
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record DBException(tagname:%s): %s", strTagName.c_str(), e.what());
	//	return -1;
	//}
	//catch (std::exception& e) {
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record StdException(tagname:%s): %s", strTagName.c_str(), e.what());
	//	return -2;
	//}
	//catch (...){
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(RealData_TO_Table_Record)PKHisDB RealData_TO_Table_Record AllException(tagname:%s): NULL", strTagName.c_str());
	//	return -3;
	//}

	return 0;
}