#include "TrendTimerTask.h"

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
#include "TrendTimer.h"

using namespace mongo;

extern CPKLog g_logger;

extern void StringReplace(string &strBase, string strSrc, string strDes);
extern string SubString(string &src, int nLen);

CTrendTimer::CTrendTimer()
{
	m_pTimerTask = NULL;
	m_bFirstTimerCome = true;
}

CTrendTimer::~CTrendTimer()
{
}

void CTrendTimer::SetTimerTask(CTrendTimerTask *pTimerTask)
{
	m_pTimerTask = pTimerTask;
}

int  CTrendTimer::handle_timeout(const  ACE_Time_Value &current_time, const   void  *act)
{
	// �Ƿ��ǵ�һ�ζ�ʱ���������ǵ�һ�Σ���ô���ܱ������ݣ���Ϊ������������
	if (m_bFirstTimerCome)
	{
		m_bFirstTimerCome = false;
		return 0;
	}

	MAIN_TASK->m_mutexMongo.acquire(); // mongo�����ܱ�����߳�ͬʱ���ʣ���˼��������쳣

	ProcessTags(current_time);
	MAIN_TASK->m_mutexMongo.release(); // mongo�����ܱ�����߳�ͬʱ���ʣ���˼��������쳣

	return  0;
}

int  CTrendTimer::Start()
{
	ACE_Time_Value tvInterval;
	tvInterval.msec(m_nIntervalMS);  //msec(m_nCyclePeriodMS);
	ACE_Time_Value tvStart(0);          // = ACE_OS::gettimeofday();  ��1970��1��1�� 0:00:00�뿪ʼ��ʱ��

	ACE_Timer_Queue *timer_queue = m_pTimerTask->m_pReactor->timer_queue();
	if (0 != timer_queue)
		timer_queue->schedule(this, NULL, tvStart, tvInterval);

	//m_pTimerTask->m_pReactor->schedule_timer(this, NULL, tvStart, tvInterval); ���ֶ�ʱ�����޷�������0��0��0�뿪ʼ��ʱ

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "%s, started timer, interval: %d second, tagnum:%d", "trend", m_nIntervalMS/1000, m_vecTags.size());
	return  0;
}

int  CTrendTimer::Stop()
{
	m_pTimerTask->m_pReactor->cancel_timer(this);
	return  0;
}

int CTrendTimer::ProcessTagForTrend(vector<BSONObj> & vecBsonObjNewest, vector<BSONObj> & vecBsonObj10Minute, CTagInfo *pTag, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality)
{
	//double dbNewRecValue = ::atof(strNewRecValue.c_str());
	//// �������벻�ظ������ݵ����Ʊ�
	//if (pTag->vecTempRec.size() > 0)
	//{
	//	CTagRawRecord *pTagRaw = pTag->vecTempRec[pTag->vecTempRec.size() - 1];
	//	if (pTagRaw->strTime.compare(strNewRecTime) == 0 && pTagRaw->nQuality == nNewRecQuality && pTagRaw->dbValue == dbNewRecValue)
	//	{
	//		// �ظ��ļ�¼������Ҫ��������
	//		return 0;
	//	}
	//}

	//CTagRawRecord *pTempRec = new CTagRawRecord();
	//pTempRec->dbValue = dbNewRecValue;
	//pTempRec->strTagName = pTag->strTagName;
	//pTempRec->strTime = strNewRecTime;
	//pTempRec->nQuality = nNewRecQuality;

	// ���µĲ��ظ����ݲ��뵽temp��
	RealData_TO_Table_TrendNewest(vecBsonObjNewest, pTag->strTagName, strNewRecValue, strNewRecTime, nNewRecQuality);

	//pTag->vecTempRec.push_back(pTempRec);
	//if (pTag->vecTempRec.size() <= 1)	 // ֻ��һ�����ݣ�����Ҫ����
	//	return 0;

	//// �������ֵ��˵���ڴ���û����õĵط���ɾ��ͷ������
	//if (pTag->vecTempRec.size() > 20000)
	//{
	//	pTag->vecTempRec.erase(pTag->vecTempRec.begin());
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����%s,temp���еļ�¼����̫����:%d������20000", pTag->strTagName.c_str(), pTag->vecTempRec.size());
	//}

	//CTagRawRecord * pRecNewest = pTempRec;	// ���µļ�¼
	//CTagRawRecord * pRecSecondNewest = pTag->vecTempRec[pTag->vecTempRec.size() - 2]; // ���µ�һ����¼
	//CTagRawRecord * pRecOldest = pTag->vecTempRec[0]; // ���ϵļ�¼

	//// ����һ�����ݺ����µ����ݣ��ǲ���������1����ͬ���ڲ���ͬһ��10���ӵ������ڣ�������ڣ���ֹͣ��������������´���
	//// ��ȷ��16���ӵ�����
	//string strNewestRec_ymdhm = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	//string strSecondNewest_ymdhm = SubString(pRecSecondNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	//string strOldestRec_ymdhm = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15

	//string strNewestRec_ymdh10m = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	//string strOldestRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1

	//if (strSecondNewest_ymdhm.compare(strNewestRec_ymdhm) == 0)   // ���µļ�¼��yymmddhhmm�����µļ�¼�ķ���ʱ����ͬ��˵��ʱ��û�иı䣬û���µķ�������
	//{
	//	return 0;
	//}

	//// ��Ҫ���ϵ����ݼ�����ŵ�raw��
	//int iRecord = 0;
	//string strOlderRec_ymdhm = strOldestRec_ymdhm; // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	//string strOlderRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	//// �������У�2017-06-28 10:30(20����  ��2017-06-28 10:40(20������2017-06-28 10:50(30������2017-06-28 11:10(20����
	//// strOldestRec_ymdh10m == 	2017-06-28 10:30	  strNewestRec_ymdh10m == 	2017-06-28 11:10
	//// strOlderRec_ymdh10m ���ε��� 2017-06-28 10:30(20���� ��2017-06-28 10:40(20������2017-06-28 10:50(30����
	//for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end();)
	//{
	//	CTagRawRecord *pRec = *itRec;
	//	string strCurRec_ymdhm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	//	if (strCurRec_ymdhm.compare(strNewestRec_ymdhm) == 0) // ����1����ʱ��ε����ݺ͵�ǰ��¼����ͬһ���ӣ����ܱ��� 
	//		break;

	//	// ��ǰ���ݺ����µ�һ���Ӳ�ͬ����Ҫ���档���ҳ�����һ������������10���ӵ����м�¼
	//	vector<CTagRawRecord *> vec1MinuteTempRecToRaw;

	//	// 10����ʱ�����һ��strOlderRec_ymdh10m��ͬ������Ϊ��ͬһ��10���ӵ����ݣ�һ������
	//	vector<CTagRawRecord *>::iterator itRec2 = itRec;
	//	for (; itRec2 != pTag->vecTempRec.end(); itRec2++)
	//	{
	//		CTagRawRecord *pRec2 = *itRec2;
	//		string strTmpymdhm = SubString(pRec2->strTime, TIMELEN_YYYYMMDD_HHMM);
	//		if (strTmpymdhm.compare(strCurRec_ymdhm) == 0)  // �������µķ��ӣ���Ҫ����
	//		{
	//			vec1MinuteTempRecToRaw.push_back(*itRec2); //	  strOlderRec_ymdh10m
	//			if (itRec2 == pTag->vecTempRec.end())
	//				break;
	//		}
	//		else
	//		{
	//			break;
	//		}
	//	}
	//	itRec = itRec2; //itRec2ָ����һ��Ҫ����ķ��ӻ������·���
	//	pRec = *itRec;

	//	if (vec1MinuteTempRecToRaw.size() > 0)
	//	{
	//		Write_1m_Rec_TO_Table_Trend(vecBsonObj10Minute, pTag->strTagName, strCurRec_ymdhm, vec1MinuteTempRecToRaw);
	//	}

	//	vec1MinuteTempRecToRaw.clear();
	//	vec1MinuteTempRecToRaw.push_back(pRec); //	  �����¸�ʮ����
	//}

	//// ɾ��Temp�ڴ����Ѿ� ���浽raw������ݡ���	pRecOldest��ɾ����ʱ��===strNewestRec_ymdh10m	 Ϊֹ
	//// ��������10���ӵ����ݣ�
	//strNewestRec_ymdh10m = SubString(strNewestRec_ymdhm, strNewestRec_ymdhm.length() - 1);
	//for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end(); itRec)   // �����µ�10��������ʱ�䲻ͬ����Ҫ���ϵ����ݼ�����ŵ�raw��
	//{
	//	CTagRawRecord *pRec = *itRec;
	//	string strCurRec_ymdh10m = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	//	vector<CTagRawRecord *>::iterator itCur = itRec;
	//	itRec++;
	//	if (strCurRec_ymdh10m.compare(strNewestRec_ymdh10m) != 0)
	//	{
	//		delete pRec;
	//		pRec = NULL;
	//		pTag->vecTempRec.erase(itCur);
	//	}
	//}
	return 0;
}
/**
*  ���ܣ��������Tag��Ľ���������MongoDb����д�빤��.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CTrendTimer::ProcessTags(ACE_Time_Value tvTimer) // yyyy-mm-dd
{
	char szCurTime[32];
	PKTimeHelper::Time2String(szCurTime, sizeof(szCurTime), tvTimer.sec());

	int nTagNum_NoRealData = 0;
	int nTagNum_TimeFormatError = 0;
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();

	vector<string> vecTagValue;
	m_pTimerTask->m_redisRW.mget(m_vecTagName, vecTagValue);
	if (m_vecTagName.size() != vecTagValue.size())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TrendTimer::ProcessTags, interval:%d seconds, mget(%d tags), return data count:%d, NOT EQUAL", m_nIntervalMS / 1000, m_vecTagName.size(), vecTagValue.size());
		return -1;
	}
	ACE_Time_Value tvEndMGetData = ACE_OS::gettimeofday();
	vector<BSONObj> vecBsonObjNewest, vecBsonObj10Minute;
	for (int iTag = 0; iTag < m_vecTagName.size(); iTag++)
	{
		string &strTagName = m_vecTagName[iTag];
		string &strTagVTQ = vecTagValue[iTag];
		CTagInfo *pTag = m_vecTags[iTag];
		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "begin processing tag:%s", pTag->strTagName.c_str());

		if (strTagVTQ.empty()) // ���ܻ��ȡ�����ַ���
		{
			nTagNum_NoRealData++;
			// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����%s������tag���ݵ�δ��ȡ��", pTag->strTagName.c_str());
			continue;
		}
		//nTag++;
		//printf("tagname:%s, No.:%d, value:%s\n", pTag->strTagName.c_str(), nTag, curStr.c_str());

		string strNewRecTime = szCurTime;
		string strNewRecValue = "???";
		int nNewRecQuality = -50000;
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(strTagVTQ, root, false))
		{
			//�ж����ݱ�������:int \ double \ string
			if (!root[REC_ATTR_VALUE].isNull())
				strNewRecValue = root[REC_ATTR_VALUE].asString();
			if (!root[REC_ATTR_QUALITY].isNull())
				nNewRecQuality = atoi(root[REC_ATTR_QUALITY].asString().c_str());
		}
		ProcessTagForTrend(vecBsonObjNewest, vecBsonObj10Minute, pTag, strNewRecTime, strNewRecValue, nNewRecQuality);
	}
	vecTagValue.clear();

	// ���� strYYMMDDHHMMSS.XXXX �õ� YYMMDDHH
	string strTableNewest = MONGO_DB_TABLE_TREND_NEWEST;
	// ���� strYYMMDDHHM	�е�strYYMMDDHH
	string strCurTime = szCurTime;
	string strYYYYMMDD = SubString(strCurTime, TIMELEN_YYYYMMDD);
	// ������ trend_yyyy_mm_dd
	string strTable10Minute = MONGO_DB_TABLE_TREND_PREFIX + strYYYYMMDD;
	StringReplace(strTable10Minute, "-", "_"); 

	ACE_Time_Value tvBeginWriteHisData = ACE_OS::gettimeofday();

	try 
	{
		if (vecBsonObjNewest.size() > 0)
		{
			m_pTimerTask->m_mongoConn.insert(strTableNewest, vecBsonObjNewest);
			vecBsonObjNewest.clear();
		}

		if (vecBsonObj10Minute.size() > 0)
		{
			m_pTimerTask->m_mongoConn.insert(strTable10Minute, vecBsonObj10Minute);
			vecBsonObj10Minute.clear();
		}

		ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
		ACE_Time_Value tvSpan = tvEnd - tvBegin;
		float fTotalSec = tvSpan.get_msec() / 1000.0f;
		float fMGetDataSec = (tvEndMGetData - tvBegin).get_msec() / 1000.0f;
		float fWriteHisDataSec = (tvEnd - tvBeginWriteHisData).get_msec() / 1000.0f;

		if (nTagNum_NoRealData != 0 || nTagNum_TimeFormatError != 0)
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "==WRITE TREND, interval:%ds, tagnum:%d. no data:%d, wrong time format:%d. total:%.3fsec, read:%.3f, write:%.3f",
				m_nIntervalMS / 1000, m_vecTags.size(), nTagNum_NoRealData, nTagNum_TimeFormatError, fTotalSec, fMGetDataSec, fWriteHisDataSec);
		else
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "==WRITE TREND, interval:%ds, tagnum:%d, total:%.3fsec, read:%.3f, write:%.3f",  
				m_nIntervalMS / 1000, m_vecTags.size(), fTotalSec, fMGetDataSec, fWriteHisDataSec);
	}
	catch (DBException& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "==WRITE TREND PKHisDB (table:%s, count:%d, table:%s, count:%d): %s", strTable10Minute.c_str(), vecBsonObj10Minute.size(), strTableNewest.c_str(), vecBsonObjNewest.size(), e.what());
		vecBsonObjNewest.clear();
		vecBsonObj10Minute.clear();
		return -1;
	}
	catch (std::exception& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "==WRITE TREND PKHisDB StdException(table:%s, count:%d, table:%s, count:%d): %s", strTable10Minute.c_str(), vecBsonObj10Minute.size(), strTableNewest.c_str(), vecBsonObjNewest.size(), e.what());
		vecBsonObjNewest.clear();
		vecBsonObj10Minute.clear();
		return -2;
	}
	catch (...) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "==WRITE TREND PKHisDB AllException(table:%s, count:%d, table:%s, count:%d): NULL", strTable10Minute.c_str(), vecBsonObj10Minute.size(), strTableNewest.c_str(), vecBsonObjNewest.size());
		vecBsonObjNewest.clear();
		vecBsonObj10Minute.clear();
		return -3;
	}

	return 0;
}

// ������д�뵽mongodb��temp��
int CTrendTimer::RealData_TO_Table_TrendNewest(vector<BSONObj> & vecBsonObj, string &strTagName, string &strValue, string &strTimeStamp, int &nQuality)
{
	string strTagNameUtf8 = PKCodingConv::AnsiToUtf8(string(strTagName));
	BSONObjBuilder builder;
	builder.append(REC_ATTR_TAGNAME, strTagNameUtf8);
	builder.append(REC_ATTR_TIME, strTimeStamp);
	builder.append(REC_ATTR_VALUE, strValue);
	builder.append(REC_ATTR_QUALITY, nQuality);
	vecBsonObj.push_back(builder.obj());

	//BSONObjBuilder whereBuilder;
	//whereBuilder.append(REC_ATTR_TAGNAME, strTagNameUtf8.c_str()).append(REC_ATTR_TIME, pRawRecord->strTime.c_str());
	//try{
	//	m_pTimerTask->m_mongoConn.update(MONGO_DB_TABLE_TREND_NEWEST, whereBuilder.obj(), builder.obj(), true); // �����¼����������룬�������
	//}
	//catch (DBException& e) {
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
	//	return -1;
	//}
	//catch (std::exception& e) {
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
	//	return -2;
	//}
	//catch (...){
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
	//	return -3;
	//}
	
	return 0;
}

// ÿ10���ӵ�������Ϊһ����¼�����ȥ
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
//int CTrendTimer::Write_10m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute)
//{
//	m_pTimerTask->EnsureTableIndex(m_pTimerTask->m_mongoConn, strTableName); // Ϊ����������
//
//	BSONArrayBuilder vsBuilder;
//	for (vector<CTagRawRecord *>::iterator itRec = vec10Minute.begin(); itRec != vec10Minute.end(); itRec++)
//	{
//		CTagRawRecord *pRec = *itRec;
//		BSONObjBuilder objOne;
//		objOne.append(REC_ATTR_TIME, pRec->strTime.c_str()).append(REC_ATTR_VALUE, pRec->dbValue).append(REC_ATTR_QUALITY, pRec->nQuality);
//		vsBuilder.append(objOne.obj());
//	}
//
//	string strTagNameUtf8 = PKCodingConv::AnsiToUtf8(string(strTagName));
//
//	BSONObjBuilder rowBuilder;
//	rowBuilder.append(REC_ATTR_TAGNAME, strTagNameUtf8.c_str()).append(REC_ATTR_TIME, strYYMMDDHH10M.c_str()).append(REC_ATTR_VALUESET, vsBuilder.obj());
//	vecBsonObj10Minute.push_back(rowBuilder.obj());
//	//BSONObjBuilder whereBuilder;
//	//whereBuilder.append(REC_ATTR_TAGNAME, strTagNameUtf8.c_str()).append(REC_ATTR_TIME, strYYMMDDHH10M.c_str());
//	//try{
//	//	m_pTimerTask->m_mongoConn.update(strTableName, whereBuilder.obj(), rowBuilder.obj(), true);
//	//}
//	//catch (DBException& e) {
//	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//	//	return -1;
//	//}
//	//catch (std::exception& e) {
//	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//	//	return -2;
//	//}
//	//catch (...){
//	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
//	//	return -3;
//	//}
//	return 0;
//}

// ��һ���ӵ�����д�뵽mongodb��raw	��raw_yymmdd. tΪ��yy-mm-dd hh:m0, 2017-06-28 15:50
// raw��ÿ10����1����¼�������ѯ��10���Ӽ�¼���ڣ�����£�������롣һ����ִ��һ��
//int CTrendTimer::Write_1m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute)
//{
//	// ���� strYYMMDDHHM	�е�strYYMMDDHH
//	string strYYYYMMDD = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD);
//	
//	// ������ trend_yyyy_mm_dd
//	string strTableName = MONGO_DB_TABLE_TREND_PREFIX + strYYYYMMDD;
//	StringReplace(strTableName, "-", "_");
//
//	//m_pTimerTask->EnsureTableIndex(m_pTimerTask->m_mongoConn, strTableName); // Ϊ����������
//	string strYYYYMMDD10M = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HHM) + "0";
//
//	vector<CTagRawRecord *> vecOld10MinuteRec; // ��Ҫ�ֹ�ɾ������ڴ�
//	int nRet = Query_Trend_Table_Common(strTagName, strYYYYMMDD10M, vecOld10MinuteRec); // ��ѯ��10���ӵ����ݣ�ÿ��������600��ԭʼ�������
//
//	vector<CTagRawRecord *> vecNew10MinuteRec;
//	bool bMerged = MergeRawDataRecs(strTagName, vecOld10MinuteRec, vecRecOf1Minute, vecNew10MinuteRec);
//	if (bMerged)
//	{
//		Write_10m_Rec_TO_Table_Trend(vecBsonObj10Minute, strTagName, strTableName, strYYYYMMDD10M, vecNew10MinuteRec);
//	}
//	// vecRecOf1Minute���ڴ��Ǵ�pTag->vecTemp�л�ȡ�ģ�����ɾ����
//	// ��Щ�ڴ���Queryʱ��ʱ���ٵģ���Ҫ�ֹ�ɾ��
//	DeleteTempRecordVector(vecOld10MinuteRec);
//	return 0;
//}

//void CTrendTimer::DeleteTempRecordMap(map<string, CTagRawRecord *> &mapRec)
//{
//	for (map<string, CTagRawRecord *>::iterator itMap = mapRec.begin(); itMap != mapRec.end(); itMap++)
//	{
//		delete itMap->second;
//	}
//	mapRec.clear();
//}

//void CTrendTimer::DeleteTempRecordVector(vector<CTagRawRecord *> &vecRec)
//{
//	for (vector<CTagRawRecord *>::iterator it = vecRec.begin(); it != vecRec.end(); it++)
//	{
//		delete *it;
//	}
//	vecRec.clear();
//}


// ��ѯ1��raw_yyyyMMdd���еļ�¼����ת��Ϊvector<CTagRawRecord *>����
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
//int CTrendTimer::Query_Trend_Table_Common(string & strTagName, string strYYYYMMDD10M, vector<CTagRawRecord *> &vecRecOf10M)
//{
//	string strYYYYMMDD = SubString(strYYYYMMDD10M, TIMELEN_YYYYMMDD);
//
//	string strTableName = MONGO_DB_TABLE_TREND_PREFIX + strYYYYMMDD;
//	StringReplace(strTableName, "-", "_");
//
//	BSONObjBuilder objBuilder;
//	objBuilder.append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, strYYYYMMDD10M.c_str());
//	mongo::Query query(objBuilder.obj());
//	try{
//
//		BSONObj bsonObj = m_pTimerTask->m_mongoConn.findOne(strTableName, query);
//		if (bsonObj.isEmpty())
//			return 0;
//
//		BSONObj objVS = bsonObj.getField(REC_ATTR_VALUESET).Obj();
//		if (!objVS.couldBeArray())
//			return -2;
//
//		vector<BSONElement> vecRec;
//		objVS.elems(vecRec);
//
//		//vector<BSONElement> vecRec = bsonObj.getField(REC_ATTR_VALUESET).Array();
//		for (vector<mongo::BSONElement>::iterator it = vecRec.begin(); it != vecRec.end(); ++it)
//		{
//			//cout << *it << endl;
//			BSONElement &elem = *it;
//            const BSONObj &obj = elem.Obj();
//			CTagRawRecord *pRawRec = new CTagRawRecord();
//			pRawRec->strTagName = strTagName;
//			pRawRec->strTime = obj.getStringField(REC_ATTR_TIME);
//			pRawRec->nQuality = obj.getIntField(REC_ATTR_QUALITY);
//			pRawRec->dbValue = obj.getIntField(REC_ATTR_VALUE);
//			vecRecOf10M.push_back(pRawRec);
//		}
//		return 0;
//	}
//	catch (DBException& e) {
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//		return -1;
//	}
//	catch (std::exception& e) {
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//		return -2;
//	}
//	catch (...){
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
//		return -3;
//	}
//	return 0;
//}

// �ϲ�����vector��
// �����Ƿ�������vecBaseRec��ͬ
// vecBaseRec���ϲ��Ļ�׼�����1
// vecRecToMerge��Ҫ�ϲ�����Ľ����
// vecNewRec���ϲ����
//bool CTrendTimer::MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec)
//{
//	bool bHasVecMerged = false; // �Ƿ���vecBaseRec������������
//	if (vecBaseRec.size() == 0 && vecRecToMerge.size() == 0)
//		return false;
//
//	if (vecBaseRec.size() == 0)
//	{
//		vecNewRec.swap(vecRecToMerge);
//		return true;
//	}
//	
//	if (vecRecToMerge.size() == 0)
//	{
//		vecNewRec.swap(vecBaseRec);
//		return false;
//	}
//
//	// ��mongo�ģ����ü�¼�Ƿ���ڲ�ȡ��
//	map<string, CTagRawRecord *> mapTime2Rec; // ʹ��map���������ж��ظ�������
//	for (vector<CTagRawRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
//	{
//		CTagRawRecord *pTmpRec = *itRec;
//		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); // ԭʼ����temp�ĺϲ�ȡTIMELEN_YYYYMMDD_HHMMSS.XXX
//		mapTime2Rec[strRecTime] = pTmpRec;
//	}
//
//	for (vector<CTagRawRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
//	{
//		CTagRawRecord *pTmpRec = *itRec;
//		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); ԭʼ����temp�ĺϲ�ȡTIMELEN_YYYYMMDD_HHMMSS.XXX
//		map<string, CTagRawRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
//		if (itFound != mapTime2Rec.end())
//		{
//			CTagRawRecord *pOldRec = itFound->second;
//			if (pTmpRec->dbValue != pOldRec->dbValue || pTmpRec->nQuality != pOldRec->nQuality)
//			{
//				// ��������ʱ�������.������delete����Ϊ��Ҫ�����pTag->vecTemp��
//				//delete pOldRec;
//				//pOldRec = NULL;
//				mapTime2Rec[strRecTime] = pTmpRec;
//				bHasVecMerged = true;
//			}
//			else
//			{
//				// ������delete����Ϊ��Ҫ�����pTag->vecTemp��
//				//delete pTmpRec;
//				//pTmpRec = NULL;
//			}
//		}
//		else
//		{
//			mapTime2Rec[strRecTime] = pTmpRec;
//			bHasVecMerged = true;
//		}
//	}
//	
//	for (map<string, CTagRawRecord *>::iterator itMap = mapTime2Rec.begin(); itMap != mapTime2Rec.end(); itMap++)
//	{
//		vecNewRec.push_back(itMap->second);
//	}
//
//	return bHasVecMerged;
//}
