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
	// 是否是第一次定时器到达。如果是第一次，那么不能保存数据，因为不是整分整秒
	if (m_bFirstTimerCome)
	{
		m_bFirstTimerCome = false;
		return 0;
	}

	MAIN_TASK->m_mutexMongo.acquire(); // mongo对象不能被多个线程同时访问，因此加锁避免异常

	ProcessTags(current_time);
	MAIN_TASK->m_mutexMongo.release(); // mongo对象不能被多个线程同时访问，因此加锁避免异常

	return  0;
}

int  CTrendTimer::Start()
{
	ACE_Time_Value tvInterval;
	tvInterval.msec(m_nIntervalMS);  //msec(m_nCyclePeriodMS);
	ACE_Time_Value tvStart(0);          // = ACE_OS::gettimeofday();  从1970年1月1日 0:00:00秒开始定时器

	ACE_Timer_Queue *timer_queue = m_pTimerTask->m_pReactor->timer_queue();
	if (0 != timer_queue)
		timer_queue->schedule(this, NULL, tvStart, tvInterval);

	//m_pTimerTask->m_pReactor->schedule_timer(this, NULL, tvStart, tvInterval); 这种定时方法无法做到从0点0分0秒开始定时

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
	//// 仅仅插入不重复的数据到趋势表。
	//if (pTag->vecTempRec.size() > 0)
	//{
	//	CTagRawRecord *pTagRaw = pTag->vecTempRec[pTag->vecTempRec.size() - 1];
	//	if (pTagRaw->strTime.compare(strNewRecTime) == 0 && pTagRaw->nQuality == nNewRecQuality && pTagRaw->dbValue == dbNewRecValue)
	//	{
	//		// 重复的记录，不需要处理！！！
	//		return 0;
	//	}
	//}

	//CTagRawRecord *pTempRec = new CTagRawRecord();
	//pTempRec->dbValue = dbNewRecValue;
	//pTempRec->strTagName = pTag->strTagName;
	//pTempRec->strTime = strNewRecTime;
	//pTempRec->nQuality = nNewRecQuality;

	// 最新的不重复数据插入到temp表
	RealData_TO_Table_TrendNewest(vecBsonObjNewest, pTag->strTagName, strNewRecValue, strNewRecTime, nNewRecQuality);

	//pTag->vecTempRec.push_back(pTempRec);
	//if (pTag->vecTempRec.size() <= 1)	 // 只有一个数据，不需要处理
	//	return 0;

	//// 超过最大值，说明内存有没处理好的地方，删除头部数据
	//if (pTag->vecTempRec.size() > 20000)
	//{
	//	pTag->vecTempRec.erase(pTag->vecTempRec.begin());
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量%s,temp表中的记录个数太多了:%d，超过20000", pTag->strTagName.c_str(), pTag->vecTempRec.size());
	//}

	//CTagRawRecord * pRecNewest = pTempRec;	// 最新的记录
	//CTagRawRecord * pRecSecondNewest = pTag->vecTempRec[pTag->vecTempRec.size() - 2]; // 次新的一条记录
	//CTagRawRecord * pRecOldest = pTag->vecTempRec[0]; // 最老的记录

	//// 检查第一个数据和最新的数据，是不是年月日1分相同（在不在同一个10分钟的周期内）。如果在，则停止处理，否则继续向下处理
	//// 精确到16分钟的数据
	//string strNewestRec_ymdhm = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	//string strSecondNewest_ymdhm = SubString(pRecSecondNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	//string strOldestRec_ymdhm = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15

	//string strNewestRec_ymdh10m = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
	//string strOldestRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1

	//if (strSecondNewest_ymdhm.compare(strNewestRec_ymdhm) == 0)   // 次新的记录的yymmddhhmm，最新的记录的分钟时间相同，说明时间没有改变，没有新的分钟数据
	//{
	//	return 0;
	//}

	//// 需要将老的数据计算后存放到raw表
	//int iRecord = 0;
	//string strOlderRec_ymdhm = strOldestRec_ymdhm; // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	//string strOlderRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	//// 数据序列：2017-06-28 10:30(20个）  、2017-06-28 10:40(20个）、2017-06-28 10:50(30个）、2017-06-28 11:10(20个）
	//// strOldestRec_ymdh10m == 	2017-06-28 10:30	  strNewestRec_ymdh10m == 	2017-06-28 11:10
	//// strOlderRec_ymdh10m 依次等于 2017-06-28 10:30(20个） 、2017-06-28 10:40(20个）、2017-06-28 10:50(30个）
	//for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end();)
	//{
	//	CTagRawRecord *pRec = *itRec;
	//	string strCurRec_ymdhm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
	//	if (strCurRec_ymdhm.compare(strNewestRec_ymdhm) == 0) // 最新1分钟时间段的数据和当前记录都是同一分钟，不能保存 
	//		break;

	//	// 当前数据和最新的一分钟不同，需要保存。先找出最新一分钟数据所在10分钟的所有记录
	//	vector<CTagRawRecord *> vec1MinuteTempRecToRaw;

	//	// 10分钟时间和上一个strOlderRec_ymdh10m相同，则认为是同一个10分钟的数据，一道更新
	//	vector<CTagRawRecord *>::iterator itRec2 = itRec;
	//	for (; itRec2 != pTag->vecTempRec.end(); itRec2++)
	//	{
	//		CTagRawRecord *pRec2 = *itRec2;
	//		string strTmpymdhm = SubString(pRec2->strTime, TIMELEN_YYYYMMDD_HHMM);
	//		if (strTmpymdhm.compare(strCurRec_ymdhm) == 0)  // 不是最新的分钟，需要保存
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
	//	itRec = itRec2; //itRec2指向下一个要保存的分钟或者最新分钟
	//	pRec = *itRec;

	//	if (vec1MinuteTempRecToRaw.size() > 0)
	//	{
	//		Write_1m_Rec_TO_Table_Trend(vecBsonObj10Minute, pTag->strTagName, strCurRec_ymdhm, vec1MinuteTempRecToRaw);
	//	}

	//	vec1MinuteTempRecToRaw.clear();
	//	vec1MinuteTempRecToRaw.push_back(pRec); //	  保存下个十分钟
	//}

	//// 删除Temp内存中已经 保存到raw表的数据。从	pRecOldest，删除到时间===strNewestRec_ymdh10m	 为止
	//// 保留至少10分钟的数据！
	//strNewestRec_ymdh10m = SubString(strNewestRec_ymdhm, strNewestRec_ymdhm.length() - 1);
	//for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end(); itRec)   // 和最新的10分钟数据时间不同，需要将老的数据计算后存放到raw表
	//{
	//	CTagRawRecord *pRec = *itRec;
	//	string strCurRec_ymdh10m = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
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
*  功能：处理接受Tag点的解析工作及MongoDb数据写入工作.
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

		if (strTagVTQ.empty()) // 可能会读取到空字符串
		{
			nTagNum_NoRealData++;
			// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量%s的最新tag数据的未读取到", pTag->strTagName.c_str());
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
			//判断数据保存类型:int \ double \ string
			if (!root[REC_ATTR_VALUE].isNull())
				strNewRecValue = root[REC_ATTR_VALUE].asString();
			if (!root[REC_ATTR_QUALITY].isNull())
				nNewRecQuality = atoi(root[REC_ATTR_QUALITY].asString().c_str());
		}
		ProcessTagForTrend(vecBsonObjNewest, vecBsonObj10Minute, pTag, strNewRecTime, strNewRecValue, nNewRecQuality);
	}
	vecTagValue.clear();

	// 根据 strYYMMDDHHMMSS.XXXX 得到 YYMMDDHH
	string strTableNewest = MONGO_DB_TABLE_TREND_NEWEST;
	// 根据 strYYMMDDHHM	中的strYYMMDDHH
	string strCurTime = szCurTime;
	string strYYYYMMDD = SubString(strCurTime, TIMELEN_YYYYMMDD);
	// 表名： trend_yyyy_mm_dd
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

// 将数据写入到mongodb的temp表
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
	//	m_pTimerTask->m_mongoConn.update(MONGO_DB_TABLE_TREND_NEWEST, whereBuilder.obj(), builder.obj(), true); // 如果记录不存在则插入，否则更新
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

// 每10分钟的数据作为一条记录插入进去
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
//int CTrendTimer::Write_10m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute)
//{
//	m_pTimerTask->EnsureTableIndex(m_pTimerTask->m_mongoConn, strTableName); // 为表建立索引！
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

// 将一分钟的数据写入到mongodb的raw	表raw_yymmdd. t为：yy-mm-dd hh:m0, 2017-06-28 15:50
// raw表每10分钟1条记录。如果查询该10分钟记录存在，则更新，否则插入。一分钟执行一次
//int CTrendTimer::Write_1m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute)
//{
//	// 根据 strYYMMDDHHM	中的strYYMMDDHH
//	string strYYYYMMDD = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD);
//	
//	// 表名： trend_yyyy_mm_dd
//	string strTableName = MONGO_DB_TABLE_TREND_PREFIX + strYYYYMMDD;
//	StringReplace(strTableName, "-", "_");
//
//	//m_pTimerTask->EnsureTableIndex(m_pTimerTask->m_mongoConn, strTableName); // 为表建立索引！
//	string strYYYYMMDD10M = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HHM) + "0";
//
//	vector<CTagRawRecord *> vecOld10MinuteRec; // 需要手工删除这块内存
//	int nRet = Query_Trend_Table_Common(strTagName, strYYYYMMDD10M, vecOld10MinuteRec); // 查询到10分钟的数据，每个数据由600个原始数据组成
//
//	vector<CTagRawRecord *> vecNew10MinuteRec;
//	bool bMerged = MergeRawDataRecs(strTagName, vecOld10MinuteRec, vecRecOf1Minute, vecNew10MinuteRec);
//	if (bMerged)
//	{
//		Write_10m_Rec_TO_Table_Trend(vecBsonObj10Minute, strTagName, strTableName, strYYYYMMDD10M, vecNew10MinuteRec);
//	}
//	// vecRecOf1Minute的内存是从pTag->vecTemp中获取的，不能删除！
//	// 这些内存是Query时临时开辟的，需要手工删除
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


// 查询1条raw_yyyyMMdd表中的记录，并转换为vector<CTagRawRecord *>返回
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

// 合并两个vector。
// 返回是否结果集和vecBaseRec不同
// vecBaseRec，合并的基准结果集1
// vecRecToMerge，要合并进入的结果集
// vecNewRec，合并结果
//bool CTrendTimer::MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec)
//{
//	bool bHasVecMerged = false; // 是否再vecBaseRec基础上增加了
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
//	// 到mongo的，看该记录是否存在并取出
//	map<string, CTagRawRecord *> mapTime2Rec; // 使用map进行排序、判断重复并更新
//	for (vector<CTagRawRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
//	{
//		CTagRawRecord *pTmpRec = *itRec;
//		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); // 原始数据temp的合并取TIMELEN_YYYYMMDD_HHMMSS.XXX
//		mapTime2Rec[strRecTime] = pTmpRec;
//	}
//
//	for (vector<CTagRawRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
//	{
//		CTagRawRecord *pTmpRec = *itRec;
//		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); 原始数据temp的合并取TIMELEN_YYYYMMDD_HHMMSS.XXX
//		map<string, CTagRawRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
//		if (itFound != mapTime2Rec.end())
//		{
//			CTagRawRecord *pOldRec = itFound->second;
//			if (pTmpRec->dbValue != pOldRec->dbValue || pTmpRec->nQuality != pOldRec->nQuality)
//			{
//				// 更新已有时间的数据.但不能delete，因为都要存放在pTag->vecTemp中
//				//delete pOldRec;
//				//pOldRec = NULL;
//				mapTime2Rec[strRecTime] = pTmpRec;
//				bHasVecMerged = true;
//			}
//			else
//			{
//				// 但不能delete，因为都要存放在pTag->vecTemp中
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
