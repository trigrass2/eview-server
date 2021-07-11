#include "MainTask.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "ace/High_Res_Timer.h"
#include "SaveTimer.h"
#include "global.h"
#include <ace/Default_Constants.h>
#include <map>
#include <vector>
#include "common/OS.h"
#include "MainTask.h"
#include "json/json.h"
#include "math.h"
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "pkdata/pkdata.h"	
extern CPKLog PKLog;

CSaveTimer::CSaveTimer()
{
	m_pMainTask = NULL;
	m_tmLastMainData = 0;
}


CSaveTimer::~CSaveTimer()
{

}

void CSaveTimer::SetMainTask(CMainTask *pMainTask)
{
	m_pMainTask = pMainTask;
	m_pMemDBClient = &pMainTask->m_redisRW;
	m_pMongoConn = &pMainTask->m_mongoConn;
}

int  CSaveTimer::handle_timeout(const  ACE_Time_Value &current_time, const   void  *act)
{
	ProcessTags();

	MaintainAllTables();
	return  0;
}

int  CSaveTimer::Start()
{
	ACE_Time_Value tvInterval;
	tvInterval.msec(m_nCyclePeriodMS);  //msec(m_nCyclePeriodMS);
	ACE_Time_Value tvStart(0);          // = ACE_OS::gettimeofday();
	m_pMainTask->m_pReactor->schedule_timer(this, NULL, tvStart, tvInterval);

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "started timer, cycle(ms): %d, tagnum:%d", m_nCyclePeriodMS, m_vecTags.size());
	return  0;
}

int  CSaveTimer::Stop()
{
	m_pMainTask->m_pReactor->cancel_timer(this);
	return  0;
}

CTagRawRecord *CSaveTimer::GetMaxTimeRecord_From_TempTable(string &strTagName)
{
	return m_pMainTask->QueryMaxTimeRecord_From_TempTable(strTagName);
}

int CSaveTimer::ProcessTag(CTagInfo *pTag, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality)
{
	double dbNewRecValue = ::atof(strNewRecValue.c_str());
	// 仅仅插入不重复的数据到tempTable
	if (pTag->vecTempRec.size() > 0)
	{
		CTagRawRecord *pTagRaw = pTag->vecTempRec[pTag->vecTempRec.size() - 1];
		if (pTagRaw->strTime.compare(strNewRecTime) == 0 && pTagRaw->nQuality == nNewRecQuality && pTagRaw->dbValue == dbNewRecValue)
		{
			// 重复的记录，不需要处理！！！
			return 0;
		}
	}

	CTagRawRecord *pTempRec = new CTagRawRecord();
	pTempRec->dbValue = dbNewRecValue;
	pTempRec->strTagName = pTag->strTagName;
	pTempRec->strTime = strNewRecTime;
	pTempRec->nQuality = nNewRecQuality;

	// 最新的不重复数据插入到temp表
	RealData_TO_TempTable(pTag->strTagName, pTempRec);

	pTag->vecTempRec.push_back(pTempRec);
	if (pTag->vecTempRec.size() <= 1)	 // 只有一个数据，不需要处理
		return 0;

	// 超过最大值，说明内存有没处理好的地方，删除头部数据
	if (pTag->vecTempRec.size() > 20000)
	{
		pTag->vecTempRec.erase(pTag->vecTempRec.begin());
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "变量%s,temp表中的记录个数太多了:%d，超过20000", pTag->strTagName.c_str(), pTag->vecTempRec.size());
	}

	CTagRawRecord * pRecNewest = pTempRec;	// 最新的记录
	CTagRawRecord * pRecSecondNewest = pTag->vecTempRec[pTag->vecTempRec.size() - 2]; // 次新的一条记录
	CTagRawRecord * pRecOldest = pTag->vecTempRec[0]; // 最老的记录

	// 检查第一个数据和最新的数据，是不是年月日1分相同（在不在同一个10分钟的周期内）。如果在，则停止处理，否则继续向下处理
	// 精确到16分钟的数据
	string strNewestRec_ymdhm = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	string strSecondNewest_ymdhm = SubString(pRecSecondNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	string strOldestRec_ymdhm = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15

	string strNewestRec_ymdh10m = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
	string strOldestRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1

	if (strSecondNewest_ymdhm.compare(strNewestRec_ymdhm) == 0)   // 次新的记录的yymmddhhmm，最新的记录的分钟时间相同，说明时间没有改变，没有新的分钟数据
	{
		return 0;
	}

	// 需要将老的数据计算后存放到raw表
	int iRecord = 0;
	string strOlderRec_ymdhm = strOldestRec_ymdhm; // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	string strOlderRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:15
	// 数据序列：2017-06-28 10:30(20个）  、2017-06-28 10:40(20个）、2017-06-28 10:50(30个）、2017-06-28 11:10(20个）
	// strOldestRec_ymdh10m == 	2017-06-28 10:30	  strNewestRec_ymdh10m == 	2017-06-28 11:10
	// strOlderRec_ymdh10m 依次等于 2017-06-28 10:30(20个） 、2017-06-28 10:40(20个）、2017-06-28 10:50(30个）
	for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end();)
	{
		CTagRawRecord *pRec = *itRec;
		string strCurRec_ymdhm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
		if (strCurRec_ymdhm.compare(strNewestRec_ymdhm) == 0) // 最新1分钟时间段的数据和当前记录都是同一分钟，不能保存 
			break;

		// 当前数据和最新的一分钟不同，需要保存。先找出最新一分钟数据所在10分钟的所有记录
		vector<CTagRawRecord *> vec1MinuteTempRecToRaw;

		// 10分钟时间和上一个strOlderRec_ymdh10m相同，则认为是同一个10分钟的数据，一道更新
		vector<CTagRawRecord *>::iterator itRec2 = itRec;
		for (; itRec2 != pTag->vecTempRec.end(); itRec2++)
		{
			CTagRawRecord *pRec2 = *itRec2;
			string strTmpymdhm = SubString(pRec2->strTime, TIMELEN_YYYYMMDD_HHMM);
			if (strTmpymdhm.compare(strCurRec_ymdhm) == 0)  // 不是最新的分钟，需要保存
			{
				vec1MinuteTempRecToRaw.push_back(*itRec2); //	  strOlderRec_ymdh10m
				if (itRec2 == pTag->vecTempRec.end())
					break;
			}
			else
			{
				break;
			}
		}
		itRec = itRec2; //itRec2指向下一个要保存的分钟或者最新分钟
		pRec = *itRec;

		if (vec1MinuteTempRecToRaw.size() > 0)
		{
			Write_1m_Rec_TO_RawTable(pTag->strTagName, strCurRec_ymdhm, vec1MinuteTempRecToRaw);
		}

		vec1MinuteTempRecToRaw.clear();
		vec1MinuteTempRecToRaw.push_back(pRec); //	  保存下个十分钟
	}

	// 删除Temp内存中已经 保存到raw表的数据。从	pRecOldest，删除到时间===strNewestRec_ymdh10m	 为止
	// 保留至少10分钟的数据！
	strNewestRec_ymdh10m = SubString(strNewestRec_ymdhm, strNewestRec_ymdhm.length() - 1);
	for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end(); itRec)   // 和最新的10分钟数据时间不同，需要将老的数据计算后存放到raw表
	{
		CTagRawRecord *pRec = *itRec;
		string strCurRec_ymdh10m = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
		vector<CTagRawRecord *>::iterator itCur = itRec;
		itRec++;
		if (strCurRec_ymdh10m.compare(strNewestRec_ymdh10m) != 0)
		{
			delete pRec;
			pRec = NULL;
			pTag->vecTempRec.erase(itCur);
		}
	}
	return 0;
}
/**
*  功能：处理接受Tag点的解析工作及MongoDb数据写入工作.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CSaveTimer::ProcessTags()
{
	int nTagNum_NoRealData = 0;
	int nTagNum_TimeFormatError = 0;
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	for (vector<CTagInfo *>::iterator itTag = m_vecTags.begin(); itTag != m_vecTags.end(); itTag++)
	{
		CTagInfo *pTag = *itTag;
		//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "begin processing tag:%s", pTag->strTagName.c_str());

		string curStr = m_pMainTask->m_redisRW.get(pTag->strTagName.c_str());
		if (curStr.empty()) // 可能会读取到空字符串
		{
			nTagNum_NoRealData++;
			// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "变量%s的最新tag数据的未读取到", pTag->strTagName.c_str());
			continue;
		}
        //PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "value string:%s", curStr.c_str());

		Json::Reader reader;
		Json::Value root;
		reader.parse(curStr, root, false);

		//判断数据保存类型:int \ double \ string
		string strNewRecTime = root[REC_ATTR_TIME].asString();;

		// 时间长度的合法性判断
		if (strNewRecTime.length() < TIMELEN_YYYYMMDD_HHMMSSXXX) // 时间，yyyy-MM-dd hh:mm:ss.xxx，必须==24个字节
		{
			nTagNum_TimeFormatError++;
			//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "变量%s的最新tag数据的时间：%s格式非法，必须为24个字节", pTag->strTagName.c_str(), pTempRec->strTime.c_str());
			continue;
		}

		string strNewRecValue = root[REC_ATTR_VALUE].asString();
		int nNewRecQuality = atoi(root[REC_ATTR_QUALITY].asString().c_str());
        ProcessTag(pTag, strNewRecTime, strNewRecValue, nNewRecQuality);
	}

	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvEnd - tvBegin;
	float fSec = tvSpan.msec() / 1000.0f;
	
	if (nTagNum_NoRealData != 0 || nTagNum_TimeFormatError != 0)
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "timer cycle:%d ms, total tagnum:%d. tagnum with no data:%d, with wrong time format:%d. consumed:%f second", 
		m_nCyclePeriodMS, m_vecTags.size(), nTagNum_NoRealData, nTagNum_TimeFormatError, fSec);
	else
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "timer cycle:%d ms, tagnum:%d, time consumed:%f second", m_nCyclePeriodMS, m_vecTags.size(), fSec);
	return 0;
}

string CSaveTimer::SubString(string &src, int nLen)
{
	if (src.length() < nLen)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "when SubStr(%s,%d), srcLen:%d < %d", src.c_str(), nLen, src.c_str(), nLen);
		return "";
	}

	string strSubString = src.substr(0, nLen); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
	return strSubString;
}
// 将数据写入到mongodb的temp表
int CSaveTimer::RealData_TO_TempTable(string &strTagName, CTagRawRecord *pRawRecord)
{
	//m_pMongoConn->createIndex()
	BSONObjBuilder builder;
	builder.append(REC_ATTR_TAGNAME, pRawRecord->strTagName);
	builder.append(REC_ATTR_TIME, pRawRecord->strTime.c_str());
	builder.append(REC_ATTR_VALUE, pRawRecord->dbValue);
	builder.append(REC_ATTR_QUALITY, pRawRecord->nQuality);
	BSONObjBuilder whereBuilder;
	whereBuilder.append(REC_ATTR_TAGNAME, pRawRecord->strTagName.c_str()).append(REC_ATTR_TIME, pRawRecord->strTime.c_str());
	try{
		m_pMongoConn->update(MONGO_DB_TEMP, whereBuilder.obj(), builder.obj(), true);
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -2;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return -3;
	}
	
	return 0;
}

// 每10分钟的数据作为一条记录插入进去
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
int CSaveTimer::Write_10m_Rec_TO_RawTable(string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute)
{
	MAIN_TASK->EnsureTableIndex(strTableName); // 为表建立索引！

	BSONArrayBuilder vsBuilder;
	for (vector<CTagRawRecord *>::iterator itRec = vec10Minute.begin(); itRec != vec10Minute.end(); itRec++)
	{
		CTagRawRecord *pRec = *itRec;
		BSONObjBuilder objOne;
		objOne.append(REC_ATTR_TIME, pRec->strTime.c_str()).append(REC_ATTR_VALUE, pRec->dbValue).append(REC_ATTR_QUALITY, pRec->nQuality);
		vsBuilder.append(objOne.obj());
	}
	BSONObjBuilder rowBuilder;
	rowBuilder.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, strYYMMDDHH10M.c_str()).append(REC_ATTR_VALUESET, vsBuilder.obj());

	BSONObjBuilder whereBuilder;
	whereBuilder.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, strYYMMDDHH10M.c_str());
	try{
		m_pMongoConn->update(strTableName, whereBuilder.obj(), rowBuilder.obj(), true);
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -2;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return -3;
	}
	return 0;
}

// 将一分钟的数据写入到mongodb的raw	表raw_yymmdd. t为：yy-mm-dd hh:m0, 2017-06-28 15:50
// raw表每10分钟1条记录。如果查询该10分钟记录存在，则更新，否则插入。一分钟执行一次
int CSaveTimer::Write_1m_Rec_TO_RawTable(string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute)
{
	// 根据 strYYMMDDHHM	中的strYYMMDDHH
	string strYYYYMMDD = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD);
	string strTableName = MONGO_DB_RAW_PREFIX + strYYYYMMDD;
	MAIN_TASK->EnsureTableIndex(strTableName); // 为表建立索引！
	string strYYYYMMDD10M = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HHM) + "0";

	vector<CTagRawRecord *> vecOld10MinuteRec; // 需要手工删除这块内存
	int nRet = Query_Raw_Record(strTagName, strYYYYMMDD10M, vecOld10MinuteRec); // 查询到10分钟的数据，每个数据由600个原始数据组成

	vector<CTagRawRecord *> vecNew10MinuteRec;
	bool bMerged = MergeRawDataRecs(strTagName, vecOld10MinuteRec, vecRecOf1Minute, vecNew10MinuteRec);
	if (bMerged)
	{
		Write_10m_Rec_TO_RawTable(strTagName, strTableName, strYYYYMMDD10M, vecNew10MinuteRec);
		// DeleteTempRecordVector(vecNewRec); 这些CTagRawRecord在后面保留在内存中了，不能删除！
		RealData_TO_MinuteTable(strTagName, SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HH), vecNew10MinuteRec);
	}
	// vecRecOf1Minute的内存是从pTag->vecTemp中获取的，不能删除！
	// 这些内存是Query时临时开辟的，需要手工删除
	DeleteTempRecordVector(vecOld10MinuteRec);
	return 0;
}

// 从minute_yyyyMM表中获取到一条记录中的所有数据
int CSaveTimer::Query_60m_Records_From_MinuteTable(string & strTagName, string strYYYYMMDDHH, vector<CTagStatRecord *> &vecRecOf60M)
{
	string strYYYYMM = SubString(strYYYYMMDDHH, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_MINUTE_PREFIX + strYYYYMM;
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, strTableName, strYYYYMMDDHH, vecRecOf60M);
}

// 从hour_yyyyMM表中获取到一条记录中的所有数据
int CSaveTimer::Query_24h_Records_From_HourTable(string & strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vecRecOf24h)
{
	string strYYYYMM = SubString(strYYYYMMDD, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_HOUR_PREFIX + strYYYYMM;
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, strTableName, strYYYYMMDD, vecRecOf24h);
}

// 从day表中获取到一条记录中的所有数据
int CSaveTimer::Query_30d_Records_From_DayTable(string & strTagName, string strYYYYMM, vector<CTagStatRecord *> &vecRecOf30d)
{
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, MONGO_DB_DAY, strYYYYMM, vecRecOf30d);
}

// 从day表中获取到一条记录中的所有数据
int CSaveTimer::Query_12Month_Records_From_MonthTable(string & strTagName, string strYYYY, vector<CTagStatRecord *> &vecOldRecOf12M)
{
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, MONGO_DB_MONTH, strYYYY, vecOldRecOf12M);
	//return Query_Stat_SimpleRecord_By_NameAndTime(strTagName, MONGO_DB_MONTH, strYYYY);
}

int CSaveTimer::Query_Stat_CompoundRecords_By_NameAndTime(string & strTagName, string strTableName, string strTime, vector<CTagStatRecord *> &vecRec)
{
	try
	{
		// select * from t where t=? and n=?
		mongo::Query query(BSONObjBuilder().append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, strTime.c_str()).obj());
		BSONObj bsonObj = m_pMongoConn->findOne(strTableName.c_str(), query);
		if (bsonObj.isEmpty())
			return -1;

		// "n":"bus1con1.208_alwayson","t" : "2017-06-28 18:50",
		// "vs":"[{"maxt":"2017-06-28 18:50:20.034","maxv":"998","mint":"2017-06-28 18:50:24.035","minv":"83","q":"0","t":"2017-06-28 18:51:00.035","v":"722"},{"maxt":"2017-06-28 18:51:15.035","maxv":"905","mint":"2017-06-28 18:51:12.034","minv":"20","q":"0","t":"2017-06-28 18:52:00.034","v":"667"},{"maxt":"2017-06-28 18:52:10.602","maxv":"904","mint":"2017-06-28 18:52:12.034","minv":"4","q":"0","t":"2017-06-28 18:53:00.035","v":"8"},{"maxt":"2017-06-28 18:53:26.041","maxv":"844","mint":"2017-06-28 18:53:00.035","minv":"8","q":"0","t":"2017-06-28 18:54:00.034","v":"22"}]"}
		BSONObj objVS = bsonObj.getField(REC_ATTR_VALUESET).Obj();
		if (!objVS.couldBeArray())
			return -2;

		vector<BSONElement> vecObj;
		objVS.elems(vecObj);

		//vector<BSONElement> vecObj = bsonObj.getField(REC_ATTR_VALUESET).Obj().elems(vecObj);
		for (vector<mongo::BSONElement>::iterator it = vecObj.begin(); it != vecObj.end(); ++it)
		{
			//cout << *it << endl;
			BSONElement &elem = *it;
            const BSONObj &obj = elem.Obj();

			CTagStatRecord *pStatRec = new CTagStatRecord();
			pStatRec->strTagName = strTagName;
			pStatRec->strTime = obj.getStringField(REC_ATTR_TIME);
			pStatRec->strFirstTime = obj.getStringField(REC_ATTR_FIRST_TIME);
			pStatRec->dbValue = ::atof(obj.getStringField(REC_ATTR_VALUE));
			pStatRec->nQuality = obj.getIntField(REC_ATTR_QUALITY);

			pStatRec->strMaxTime = obj.getStringField(REC_ATTR_MAX_TIME);
			pStatRec->dbMaxValue = ::atof(obj.getStringField(REC_ATTR_MAX_VALUE));
			pStatRec->nMaxQuality = obj.getIntField(REC_ATTR_MAX_QUALITY);

			pStatRec->strMinTime = obj.getStringField(REC_ATTR_MIN_TIME);
			pStatRec->dbMinValue = ::atof(obj.getStringField(REC_ATTR_MIN_VALUE));
			pStatRec->nMinQuality = obj.getIntField(REC_ATTR_MIN_QUALITY);

			pStatRec->dbAvg = ::atof(obj.getStringField(REC_ATTR_AVERAGE_VALUE));
			pStatRec->nDataCount = ::atof(obj.getStringField(REC_ATTR_DATA_COUNT));
			vecRec.push_back(pStatRec);
		}
		vecObj.clear();
		return 0;
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -2;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return -3;
	}
	return 0;
}
//
//CTagStatRecord * CSaveTimer::Query_Stat_SimpleRecord_By_NameAndTime(string & strTagName, string strTableName, string strTime)
//{
//	try
//	{
//		// select * from t where t=? and n=?
//		mongo::Query query(BSONObjBuilder().append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, strTime.c_str()).obj());
//		BSONObj bsonObj = m_pMongoConn->findOne(strTableName.c_str(), query);
//		if (bsonObj.isEmpty())
//			return NULL;
//
//		// "n":"bus1con1.208_alwayson","t" : "2017-06-28 18:50",
//		// "maxt":"2017-06-28 18:50:20.034","maxv":"998","mint":"2017-06-28 18:50:24.035","minv":"83","q":"0","t":"2017-06-28 18:51:00.035","v":"722"},{"maxt":"2017-06-28 18:51:15.035","maxv":"905","mint":"2017-06-28 18:51:12.034","minv":"20","q":"0","t":"2017-06-28 18:52:00.034","v":"667"},{"maxt":"2017-06-28 18:52:10.602","maxv":"904","mint":"2017-06-28 18:52:12.034","minv":"4","q":"0","t":"2017-06-28 18:53:00.035","v":"8"},{"maxt":"2017-06-28 18:53:26.041","maxv":"844","mint":"2017-06-28 18:53:00.035","minv":"8","q":"0","t":"2017-06-28 18:54:00.034","v":"22"}]"}
//		BSONObj &obj = bsonObj;
//
//		CTagStatRecord *pStatRec = new CTagStatRecord();
//		pStatRec->strTagName = strTagName;
//		pStatRec->strTime = obj.getStringField(REC_ATTR_TIME);
//		pStatRec->strFirstTime = obj.getStringField(REC_ATTR_FIRST_TIME);
//		pStatRec->dbValue = ::atof(obj.getStringField(REC_ATTR_VALUE));
//		pStatRec->nQuality = obj.getIntField(REC_ATTR_QUALITY);
//
//		pStatRec->strMaxTime = obj.getStringField(REC_ATTR_MAX_TIME);
//		pStatRec->dbMaxValue = ::atof(obj.getStringField(REC_ATTR_MAX_VALUE));
//		pStatRec->nMaxQuality = obj.getIntField(REC_ATTR_MAX_QUALITY);
//
//		pStatRec->strMinTime = obj.getStringField(REC_ATTR_MIN_TIME);
//		pStatRec->dbMinValue = ::atof(obj.getStringField(REC_ATTR_MIN_VALUE));
//		pStatRec->nMinQuality = obj.getIntField(REC_ATTR_MIN_QUALITY);
//
//		pStatRec->dbAvg = ::atof(obj.getStringField(REC_ATTR_AVERAGE_VALUE));
//		pStatRec->nDataCount = ::atof(obj.getStringField(REC_ATTR_DATA_COUNT));
//
//		return pStatRec;
//	}
//	catch (DBException& e) {
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): %s", e.what());
//		return NULL;
//	}
//	catch (std::exception& e) {
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): %s", e.what());
//		return NULL;
//	}
//	catch (...){
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): NULL");
//		return NULL;
//	}
//	return NULL;
//}

CTagStatRecord * CSaveTimer::Calc1MinuteStatRecFromRealDatas(string strTagName, vector<CTagRawRecord *> &vecRawRecOf1Minute)
{
	if (vecRawRecOf1Minute.size() <= 0)
		return NULL;

	CTagStatRecord *pMinuteRec = new CTagStatRecord();
	pMinuteRec->strTagName = strTagName;

	// 第一个点的值就是VTQ
	CTagRawRecord *pRecFirst = vecRawRecOf1Minute[0];
	pMinuteRec->strFirstTime = pRecFirst->strTime;
	pMinuteRec->strTime = SubString(pMinuteRec->strFirstTime, TIMELEN_YYYYMMDD_HHMM); // 时间为：2017-06-29 15:21a
	pMinuteRec->nQuality = pRecFirst->nQuality;
	pMinuteRec->dbValue = pRecFirst->dbValue;
	// 计算vec1MinuteRec中的第一个值、平均值、最大值、最小值、个数
	double dbSum = 0;
	for (vector<CTagRawRecord *>::iterator itRec2 = vecRawRecOf1Minute.begin(); itRec2 != vecRawRecOf1Minute.end(); itRec2++)
	{
		CTagRawRecord *pRec2 = *itRec2;
		// 最大点的VTQ
		if (pMinuteRec->dbMaxValue < pRec2->dbValue)
		{
			pMinuteRec->dbMaxValue = pRec2->dbValue;
			pMinuteRec->strMaxTime = pRec2->strTime;
			pMinuteRec->nMaxQuality = pRec2->nQuality;
		}
		// 最小点的VTQ
		if (pMinuteRec->dbMinValue > pRec2->dbValue)
		{
			pMinuteRec->dbMinValue = pRec2->dbValue;
			pMinuteRec->strMinTime = pRec2->strTime;
			pMinuteRec->nMinQuality = pRec2->nQuality;
		}
		dbSum += pRec2->dbValue;
	}
	// 点的个数和平均值
	pMinuteRec->nDataCount = vecRawRecOf1Minute.size();
	pMinuteRec->dbAvg = dbSum / pMinuteRec->nDataCount;
	return pMinuteRec;
}

// "n":"bus1con1.208_alwayson","t" : "2017-06-28 18:50",
// "vs":"[{"maxt":"2017-06-28 18:50:20.034","maxv":"998","mint":"2017-06-28 18:50:24.035","minv":"83","q":"0","t":"2017-06-28 18:51:00.035","v":"722"},{"maxt":"2017-06-28 18:51:15.035","maxv":"905","mint":"2017-06-28 18:51:12.034","minv":"20","q":"0","t":"2017-06-28 18:52:00.034","v":"667"},{"maxt":"2017-06-28 18:52:10.602","maxv":"904","mint":"2017-06-28 18:52:12.034","minv":"4","q":"0","t":"2017-06-28 18:53:00.035","v":"8"},{"maxt":"2017-06-28 18:53:26.041","maxv":"844","mint":"2017-06-28 18:53:00.035","minv":"8","q":"0","t":"2017-06-28 18:54:00.034","v":"22"}]"}
int CSaveTimer::Write_60m_records_TO_MinuteTable(string & strTagName, string strTableName, string strYYMMDDMM_HH, vector<CTagStatRecord *> &vec60MRec)
{
	return Write_Stat_records_TO_Table(strTagName, strTableName, strYYMMDDMM_HH, TIMELEN_YYYYMMDD_HHMM, vec60MRec);
}

int CSaveTimer::Write_24h_records_TO_HourTable(string & strTagName, string strTableName, string strYYYYMMDD, vector<CTagStatRecord *> &vec24hRec)
{
	return Write_Stat_records_TO_Table(strTagName, strTableName, strYYYYMMDD, TIMELEN_YYYYMMDD_HH, vec24hRec);
}

int CSaveTimer::Write_30d_records_TO_DayTable(string & strTagName, string strTableName, string strYYYYMM, vector<CTagStatRecord *> &vecNewRecOf30d)
{
	return Write_Stat_records_TO_Table(strTagName, strTableName, strYYYYMM, TIMELEN_YYYYMMDD,  vecNewRecOf30d);
}

int CSaveTimer::Write_12M_records_TO_MonthTable(string & strTagName, string strTableName, string strYYYY, vector<CTagStatRecord *> &vecNewRecOf12M)
{
	return Write_Stat_records_TO_Table(strTagName, strTableName, strYYYY, TIMELEN_YYYYMM, vecNewRecOf12M);
}

//int CSaveTimer::Write_1M_records_TO_MonthTable(string & strTagName, string strTableName, string strYYYYMM, CTagStatRecord * pMonthRec)
//{
//	return Write_Stat_OneRecord_TO_Table(strTagName, strTableName, strYYYYMM, pMonthRec);
//}
//
//// 月表 是一条记录没有 vs集合属性
//int CSaveTimer::Write_Stat_OneRecord_TO_Table(string & strTagName, string strTableName, string strRowTime, CTagStatRecord * pRec)
//{
//	char szDataCount[32];
//	sPKLog.LogMessage(PK_LOGLEVEL_ERROR, szDataCount, "%d", pRec->nDataCount);
//	BSONObjBuilder objOne;
//	objOne.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, pRec->strTime.c_str()).append(REC_ATTR_VALUE, pRec->dbValue).append(REC_ATTR_QUALITY, pRec->nQuality)
//		.append(REC_ATTR_FIRST_TIME, pRec->strMaxTime.c_str()).append(REC_ATTR_FIRST_VALUE, pRec->dbMaxValue).append(REC_ATTR_FIRST_QUALITY, pRec->nMaxQuality)
//		.append(REC_ATTR_MAX_TIME, pRec->strMaxTime.c_str()).append(REC_ATTR_MAX_VALUE, pRec->dbMaxValue).append(REC_ATTR_MAX_QUALITY, pRec->nMaxQuality)
//		.append(REC_ATTR_MIN_TIME, pRec->strMinTime.c_str()).append(REC_ATTR_MIN_VALUE, pRec->dbMinValue).append(REC_ATTR_MIN_QUALITY, pRec->nMinQuality)
//		.append(REC_ATTR_AVERAGE_VALUE, pRec->dbAvg).append(REC_ATTR_DATA_COUNT, szDataCount);
//
//	BSONObjBuilder whereBuilder;
//	whereBuilder.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, pRec->strTime.c_str());
//
//	try{
//		m_pMongoConn->update(strTableName, whereBuilder.obj(), objOne.obj(), true);
//	}
//	catch (DBException& e) {
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): %s", e.what());
//		return -1;
//	}
//	catch (std::exception& e) {
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): %s", e.what());
//		return -1;
//	}
//	catch (...){
//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(set): NULL");
//		return -1;
//	}
//	return 0;
//}

int CSaveTimer::Write_Stat_records_TO_Table(string & strTagName, string strTableName, string strRowTime, int nTimeLenInVS, vector<CTagStatRecord *> &vecStatRec)
{
	BSONArrayBuilder vsBuilder;
	if (vecStatRec.size() <= 0)
		return 0;

	//CTagStatRecord *pFirstRec = vecStatRec[0];
	//string strTimeOfNewRec = SubString(pFirstRec->strTime, nTimeLenInVS);
	for (vector<CTagStatRecord *>::iterator itRec = vecStatRec.begin(); itRec != vecStatRec.end(); itRec++)
	{
		CTagStatRecord *pRec = *itRec;
		char szDataCount[32];
		sprintf(szDataCount, "%d", pRec->nDataCount);
		BSONObjBuilder objOne;
		objOne.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_FIRST_TIME, pRec->strFirstTime.c_str())
			.append(REC_ATTR_TIME, pRec->strTime.c_str()).append(REC_ATTR_VALUE, pRec->dbValue).append(REC_ATTR_QUALITY, pRec->nQuality)
			.append(REC_ATTR_MAX_TIME, pRec->strMaxTime.c_str()).append(REC_ATTR_MAX_VALUE, pRec->dbMaxValue).append(REC_ATTR_MAX_QUALITY, pRec->nMaxQuality)
			.append(REC_ATTR_MIN_TIME, pRec->strMinTime.c_str()).append(REC_ATTR_MIN_VALUE, pRec->dbMinValue).append(REC_ATTR_MIN_QUALITY, pRec->nMinQuality)
			.append(REC_ATTR_AVERAGE_VALUE, pRec->dbAvg).append(REC_ATTR_DATA_COUNT, szDataCount);
		vsBuilder.append(objOne.obj());
	}

	BSONObjBuilder rowBuilder;
	rowBuilder.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, strRowTime.c_str()).append(REC_ATTR_VALUESET, vsBuilder.obj());

	BSONObjBuilder whereBuilder;
	whereBuilder.append(REC_ATTR_TAGNAME, strTagName.c_str()).append(REC_ATTR_TIME, strRowTime.c_str());

	try{
		m_pMongoConn->update(strTableName, whereBuilder.obj(), rowBuilder.obj(), true);
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return -1;
	}
	return 0;
}

void CSaveTimer::DeleteTempRecordMap(map<string, CTagRawRecord *> &mapRec)
{
	for (map<string, CTagRawRecord *>::iterator itMap = mapRec.begin(); itMap != mapRec.end(); itMap++)
	{
		delete itMap->second;
	}
	mapRec.clear();
}

void CSaveTimer::DeleteTempRecordVector(vector<CTagRawRecord *> &vecRec)
{
	for (vector<CTagRawRecord *>::iterator it = vecRec.begin(); it != vecRec.end(); it++)
	{
		delete *it;
	}
	vecRec.clear();
}

void CSaveTimer::DeleteStatRecordMap(map<string, CTagStatRecord *> &mapRec)
{
	for (map<string, CTagStatRecord *>::iterator itMap = mapRec.begin(); itMap != mapRec.end(); itMap++)
	{
		delete itMap->second;
	}
	mapRec.clear();
}

void CSaveTimer::DeleteStatRecordVector(vector<CTagStatRecord *> &vecRec)
{
	for (vector<CTagStatRecord *>::iterator it = vecRec.begin(); it != vecRec.end(); it++)
	{
		delete *it;
	}
	vecRec.clear();
}

// 一分钟执行一次。将数据写入到mongodb的minute_YYYYMM表
// 根据 strYYMMDDHHM	中的strYYMMDDHH，到mongo的minute_YYYYMMDD，看该记录是否存在
// 如果存在该记录，退出
// 不存在，插入mongo minute_YYYYMMDD
// 一条记录，存放60分钟的数据
int CSaveTimer::RealData_TO_MinuteTable(string &strTagName, string strYYMMDDHH, vector<CTagRawRecord *> &vec10MinuteRawRec)
{
	// 根据 strYYMMDDHHM	中的strYYMMDDHH
	string strYYYYMM = SubString(strYYMMDDHH, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_MINUTE_PREFIX + strYYYYMM;
	MAIN_TASK->EnsureTableIndex(strTableName); // 为表建立索引！

	// 将10分钟内的所有的一分钟记录计算出来，逐个计算出来
	// 10分钟时间和上一个strOlderRec_ymdh10m相同，则认为是同一个10分钟的数据，一道更新
	vector<CTagStatRecord *> vecNewMinutesRec; // 每条记录是1分钟的数据！
	vector<CTagRawRecord *> vecRawRecOf1Minute;
	CTagRawRecord *pRecFirst = vec10MinuteRawRec[0];
	string strLastMinute_yyyyMMdd_hhmm = SubString(pRecFirst->strTime, TIMELEN_YYYYMMDD_HHMM);
	for (vector<CTagRawRecord *>::iterator itRec = vec10MinuteRawRec.begin(); itRec != vec10MinuteRawRec.end(); itRec++)
	{
		CTagRawRecord *pRec = *itRec;
		string strCurMinute_yyyyMMdd_hhmm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM);
		if (strCurMinute_yyyyMMdd_hhmm.compare(strLastMinute_yyyyMMdd_hhmm) != 0) // 新的一分钟了！
		{
			string strYYMMDDMM_HH = SubString(strCurMinute_yyyyMMdd_hhmm, TIMELEN_YYYYMMDD_HH); // 根据一分钟内是否有。如果有则不需要重新插入了
			CTagStatRecord *pMinuteRec = Calc1MinuteStatRecFromRealDatas(strTagName, vecRawRecOf1Minute);
			vecNewMinutesRec.push_back(pMinuteRec);
			vecRawRecOf1Minute.clear();
			strLastMinute_yyyyMMdd_hhmm = strCurMinute_yyyyMMdd_hhmm;
		}
		
		vecRawRecOf1Minute.push_back(*itRec); //	strOlderRec_ymdh10m		
	}
	// 最后1分钟的数据
	if (vecRawRecOf1Minute.size() > 0)
	{
		CTagStatRecord *pMinuteRec = Calc1MinuteStatRecFromRealDatas(strTagName, vecRawRecOf1Minute);
		vecNewMinutesRec.push_back(pMinuteRec);
		vecRawRecOf1Minute.clear();
	}

	// 合并10分钟和新计算的1分钟的原始记录
	vector<CTagStatRecord *> vecOldRecOf60M;
	int nRet = Query_60m_Records_From_MinuteTable(strTagName, strYYMMDDHH, vecOldRecOf60M); // 查询到60分钟的数据

	vector<CTagStatRecord *> vecNew60mRec;
	bool bMerged = MergeStatDataRecs(strTagName, TIMELEN_YYYYMMDD_HHMM, vecOldRecOf60M, vecNewMinutesRec, vecNew60mRec);
	
	if (bMerged)
	{
		Write_60m_records_TO_MinuteTable(strTagName, strTableName, strYYMMDDHH, vecNew60mRec);

		string strYYYYMMDD = SubString(strYYMMDDHH, TIMELEN_YYYYMMDD);
		MinuteData_TO_HourTable(strTagName, strYYMMDDHH, vecNew60mRec);
	}

	// 这些内存是Query时临时开辟的，需要手工删除
	DeleteStatRecordVector(vecNew60mRec);

	return 0;
}

// 查询1条raw_yyyyMMdd表中的记录，并转换为vector<CTagRawRecord *>返回
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
int CSaveTimer::Query_Raw_Record(string & strTagName, string strYYYYMMDD10M, vector<CTagRawRecord *> &vecRecOf10M)
{
	string strYYYYMMDD = SubString(strYYYYMMDD10M, TIMELEN_YYYYMMDD);
	string strTableName = MONGO_DB_RAW_PREFIX + strYYYYMMDD;

	BSONObjBuilder objBuilder;
	objBuilder.append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, strYYYYMMDD10M.c_str());
	mongo::Query query(objBuilder.obj());
	try{

		BSONObj bsonObj = m_pMongoConn->findOne(strTableName, query);
		if (bsonObj.isEmpty())
			return 0;

		BSONObj objVS = bsonObj.getField(REC_ATTR_VALUESET).Obj();
		if (!objVS.couldBeArray())
			return -2;

		vector<BSONElement> vecRec;
		objVS.elems(vecRec);

		//vector<BSONElement> vecRec = bsonObj.getField(REC_ATTR_VALUESET).Array();
		for (vector<mongo::BSONElement>::iterator it = vecRec.begin(); it != vecRec.end(); ++it)
		{
			//cout << *it << endl;
			BSONElement &elem = *it;
            const BSONObj &obj = elem.Obj();
			CTagRawRecord *pRawRec = new CTagRawRecord();
			pRawRec->strTagName = strTagName;
			pRawRec->strTime = obj.getStringField(REC_ATTR_TIME);
			pRawRec->nQuality = obj.getIntField(REC_ATTR_QUALITY);
			pRawRec->dbValue = obj.getIntField(REC_ATTR_VALUE);
			vecRecOf10M.push_back(pRawRec);
		}
		return 0;
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return -2;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return -3;
	}
	return 0;
}

// 根据一组相同周期的统计数据，得到更长周期的数据。如：根据minute--->hour  nResultRecTimeLen是hour的时间长度
CTagStatRecord * CSaveTimer::CalcStatRowFromStatDatas(string strTagName, int nResultRecTimeLen, vector<CTagStatRecord *> &vecTempData)
{
	if (vecTempData.size() <= 0)
		return NULL;

	CTagStatRecord *pResultRec = new CTagStatRecord();
	pResultRec->strTagName = strTagName;

	// 第一个点的值就是VTQ
	CTagStatRecord *pRecFirst = vecTempData[0];
	pResultRec->strFirstTime = pRecFirst->strFirstTime;
	pResultRec->strTime = SubString(pRecFirst->strTime, nResultRecTimeLen);
	pResultRec->nQuality = pRecFirst->nQuality;
	pResultRec->dbValue = pRecFirst->dbValue;

	// 计算vec1MinuteRec中的第一个值、平均值、最大值、最小值、个数
	double dbSum = 0;
	for (vector<CTagStatRecord *>::iterator itRec2 = vecTempData.begin(); itRec2 != vecTempData.end(); itRec2++)
	{
		CTagStatRecord *pSrcRec = *itRec2;
		// 最大点的VTQ
		if (pResultRec->dbMaxValue < pSrcRec->dbMaxValue)
		{
			pResultRec->dbMaxValue = pSrcRec->dbMaxValue;
			pResultRec->strMaxTime = pSrcRec->strMaxTime;
			pResultRec->nMaxQuality = pSrcRec->nMaxQuality;
		}
		// 最小点的VTQ
		if (pResultRec->dbMinValue > pSrcRec->dbMinValue)
		{
			pResultRec->dbMinValue = pSrcRec->dbMinValue;
			pResultRec->strMinTime = pSrcRec->strMinTime;
			pResultRec->nMinQuality = pSrcRec->nMinQuality;
		}

		pResultRec->nDataCount += pSrcRec->nDataCount;
		dbSum += pSrcRec->dbAvg * pSrcRec->nDataCount; // 换算总和
	}
	// 点的个数和平均值
	pResultRec->dbAvg = dbSum / pResultRec->nDataCount;
	return pResultRec;
}

// 每分钟被触发时:
// 根据YYYYMMDDHH找到minute_yyyyMM表的该条记录，计算该小时的统计值
// 根据YYYYMMDD找到hour_yyyyMM表的该条记录，和上面算出到小时值合并
// 最后将合并后的值更新到hour_yyyyMM中
int CSaveTimer::MinuteData_TO_HourTable(string strTagName, string strYYYYMMDDHH, vector<CTagStatRecord *> &vec60mRec)
{
	// 计算该小时的统计值
	CTagStatRecord *pHourRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMMDD_HH, vec60mRec);
	if (!pHourRec)
		return -1;

	// 得到hour_yyyyMM小时表中的一条记录（24个小时，1天的数据）
	string strYYYYMMDD = SubString(strYYYYMMDDHH, TIMELEN_YYYYMMDD);
	vector<CTagStatRecord *> vecOldRecOf24h;
	this->Query_24h_Records_From_HourTable(strTagName, strYYYYMMDD, vecOldRecOf24h);

	// 合并新的1小时记录和已有的24小时的记录
	vector<CTagStatRecord *> vecNewRecOf24h;
	int nRecTimeLen = TIMELEN_YYYYMMDD_HH; // 小时表中，每条记录表示24小时，其中vs中每个元素的时间长度到小时
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf24h, pHourRec, vecNewRecOf24h);
	if (bMerged)
	{
		// 将合并后的记录更新到小时表中
		string strTableName = MONGO_DB_HOUR_PREFIX + SubString(strYYYYMMDDHH, TIMELEN_YYYYMM);
		Write_24h_records_TO_HourTable(strTagName, strTableName, strYYYYMMDD, vecNewRecOf24h);

		// 继续向下调用
		HourData_TO_DayTable(strTagName, strYYYYMMDD, vecNewRecOf24h);
	}
	// 这些内存是Query时临时开辟的，需要手工删除
	DeleteStatRecordVector(vecNewRecOf24h); // 释放内存
	return 0;
}

// 找到minute表的次新记录：每分钟被触发时，先到Minute_strYYYYMMDD表中，取最新的2条记录，即2个小时的记录。 如果只有一个记录，则从昨天取一个最新的记录。 从两条记录找到次新的记录，作为后续判断是否要插入Hour表的依据
// 这条次新纪录的YYYYHHMMHH去查找Hour_YYHHMM表，找不到则计算 该小时数据，插入到Hour表；找到了则计算并更新该Hour表的记录
int CSaveTimer::HourData_TO_DayTable(string strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vec24hRec)
{
	// 计算该小时的统计值
	CTagStatRecord *pDayRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMMDD, vec24hRec);
	if (!pDayRec)
		return -1;

	// 得到hour_yyyyMM小时表中的一条记录（24个小时，1天的数据）
	string strYYYYMM = SubString(strYYYYMMDD, TIMELEN_YYYYMM);
	vector<CTagStatRecord *> vecOldRecOf30d;
	this->Query_30d_Records_From_DayTable(strTagName, strYYYYMM, vecOldRecOf30d);

	// 合并新的1条记录和已有的30天的记录
	vector<CTagStatRecord *> vecNewRecOf30d;
	int nRecTimeLen = TIMELEN_YYYYMMDD; // 小时表中，每条记录表示24小时，其中vs中每个元素的时间长度到小时
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf30d, pDayRec, vecNewRecOf30d);
	if (bMerged)
	{
		// 将合并后的记录更新到小时表中
		Write_30d_records_TO_DayTable(strTagName, MONGO_DB_DAY, strYYYYMM, vecNewRecOf30d);
		DayData_TO_MonthTable(strTagName, strYYYYMM, vecNewRecOf30d);
	}

	DeleteStatRecordVector(vecNewRecOf30d); // 释放内存

	return 0;
}

// 找到minute表的次新记录：每分钟被触发时，先到Minute_strYYYYMMDD表中，取最新的2条记录，即2个小时的记录。 如果只有一个记录，则从昨天取一个最新的记录。 从两条记录找到次新的记录，作为后续判断是否要插入Hour表的依据
// 这条次新纪录的YYYYHHMMHH去查找Hour_YYHHMM表，找不到则计算 该小时数据，插入到Hour表；找到了则计算并更新该Hour表的记录
int CSaveTimer::DayData_TO_MonthTable(string strTagName, string strYYYYMM, vector<CTagStatRecord *> &vec30dRec)
{
	// 计算该小时的统计值
	CTagStatRecord *pMonthRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMM, vec30dRec);
	if (!pMonthRec)
		return -1;

	// 得到hour_yyyyMM小时表中的一条记录（24个小时，1天的数据）
	string strYYYY = SubString(strYYYYMM, TIMELEN_YYYY); // 每年1条记录
	vector<CTagStatRecord *> vecOldRecOf12M;
	this->Query_12Month_Records_From_MonthTable(strTagName, strYYYY, vecOldRecOf12M);

	// 合并新的1条记录和已有的30天的记录
	vector<CTagStatRecord *> vecNewRecOf12M;
	int nRecTimeLen = TIMELEN_YYYYMM; // 月表中，每条记录表示12个月，其中vs中每个元素的时间长度到小时
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf12M, pMonthRec, vecNewRecOf12M);
	if (bMerged)
	{
		// 将合并后的记录更新到小时表中
		Write_12M_records_TO_MonthTable(strTagName, MONGO_DB_MONTH, strYYYY, vecNewRecOf12M);
	}

	DeleteStatRecordVector(vecNewRecOf12M); // 释放内存

	// 将合并后的记录更新到小时表中
	// Write_1M_records_TO_MonthTable(strTagName, MONGO_DB_MONTH, strYYYYMM, pMonthRec);

	//delete pMonthRec;
	//pMonthRec = NULL;
	return 0;
}

// 合并两个vector。
// 返回是否结果集和vecBaseRec不同
// vecBaseRec，合并的基准结果集1
// vecRecToMerge，要合并进入的结果集
// vecNewRec，合并结果
bool CSaveTimer::MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, CTagStatRecord * pRecToMerge, vector<CTagStatRecord *> &vecNewRec)
{
	vector<CTagStatRecord *> vecToMerge;
	vecToMerge.push_back(pRecToMerge);
	bool bHasMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecBaseRec, vecToMerge, vecNewRec);
	vecToMerge.clear();
	return bHasMerged;
}

// 合并两个vector。
// 返回是否结果集和vecBaseRec不同
// vecBaseRec，合并的基准结果集1
// vecRecToMerge，要合并进入的结果集
// vecNewRec，合并结果
bool CSaveTimer::MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, vector<CTagStatRecord *> &vecRecToMerge, vector<CTagStatRecord *> &vecNewRec)
{
	bool bHasVecMerged = false; // 是否再vecBaseRec基础上增加了

	// 到mongo的minute_YYYYMM，看该记录是否存在并取出
	map<string, CTagStatRecord *> mapTime2Rec; // 使用map进行排序、判断重复并更新
	for (vector<CTagStatRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
	{
		CTagStatRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nRecTimeLen); // 如小时表，则时间为vs中每个元素的t，即yyyyMMdd hh
		mapTime2Rec[strRecTime] = pTmpRec;
	}

	for (vector<CTagStatRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
	{
		CTagStatRecord *pRecToMerge = *itRec;
		string strRecTime = pRecToMerge->strTime;// .substr(0, nRecTimeLen); // 如小时表，则时间为vs中每个元素的t，即yyyyMMdd hh
		map<string, CTagStatRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
		if (itFound != mapTime2Rec.end()) // 找到了该条记录
		{
			// 更新已有时间的数据
			CTagStatRecord *pOldRec = itFound->second;
			if (pOldRec->nDataCount != pRecToMerge->nDataCount) // 记录数不同则认为修改过了
			{
				delete pOldRec;
				pOldRec = NULL;
				mapTime2Rec[strRecTime] = pRecToMerge;
				bHasVecMerged = true;
			}
			else // 认为没有修改过，删除新的那个
			{
				delete pRecToMerge;
			}
		}
		else // 未找到该记录
		{
			mapTime2Rec[strRecTime] = pRecToMerge;
			bHasVecMerged = true;
		}
	}

	for (map<string, CTagStatRecord *>::iterator itMap = mapTime2Rec.begin(); itMap != mapTime2Rec.end(); itMap++)
	{
		vecNewRec.push_back(itMap->second);
	}

	return bHasVecMerged;
}


// 合并两个vector。
// 返回是否结果集和vecBaseRec不同
// vecBaseRec，合并的基准结果集1
// vecRecToMerge，要合并进入的结果集
// vecNewRec，合并结果
bool CSaveTimer::MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec)
{
	bool bHasVecMerged = false; // 是否再vecBaseRec基础上增加了
	if (vecBaseRec.size() == 0 && vecRecToMerge.size() == 0)
		return false;

	if (vecBaseRec.size() == 0)
	{
		vecNewRec.swap(vecRecToMerge);
		return true;
	}
	
	if (vecRecToMerge.size() == 0)
	{
		vecNewRec.swap(vecBaseRec);
		return false;
	}

	// 到mongo的，看该记录是否存在并取出
	map<string, CTagRawRecord *> mapTime2Rec; // 使用map进行排序、判断重复并更新
	for (vector<CTagRawRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
	{
		CTagRawRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); // 原始数据temp的合并取TIMELEN_YYYYMMDD_HHMMSS.XXX
		mapTime2Rec[strRecTime] = pTmpRec;
	}

	for (vector<CTagRawRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
	{
		CTagRawRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); 原始数据temp的合并取TIMELEN_YYYYMMDD_HHMMSS.XXX
		map<string, CTagRawRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
		if (itFound != mapTime2Rec.end())
		{
			CTagRawRecord *pOldRec = itFound->second;
			if (pTmpRec->dbValue != pOldRec->dbValue || pTmpRec->nQuality != pOldRec->nQuality)
			{
				// 更新已有时间的数据.但不能delete，因为都要存放在pTag->vecTemp中
				//delete pOldRec;
				//pOldRec = NULL;
				mapTime2Rec[strRecTime] = pTmpRec;
				bHasVecMerged = true;
			}
			else
			{
				// 但不能delete，因为都要存放在pTag->vecTemp中
				//delete pTmpRec;
				//pTmpRec = NULL;
			}
		}
		else
		{
			mapTime2Rec[strRecTime] = pTmpRec;
			bHasVecMerged = true;
		}
	}
	
	for (map<string, CTagRawRecord *>::iterator itMap = mapTime2Rec.begin(); itMap != mapTime2Rec.end(); itMap++)
	{
		vecNewRec.push_back(itMap->second);
	}

	return bHasVecMerged;
}

int CSaveTimer::MaintainAllTables()
{
	time_t tmNow;
	time(&tmNow);

	if (labs(tmNow - m_tmLastMainData) < 60) // second
	{
		return 0;
	}

	m_tmLastMainData = tmNow;
	time_t tmTenMinutesAo = tmNow - 600; // 十分钟前的时间
	// 删除不需要的数据表;
	char szDateTime[32] = { 0 };
	PKTimeHelper::Time2String(szDateTime, sizeof(szDateTime), tmTenMinutesAo);

	BSONObjBuilder condBuilder;
	condBuilder.append("$lt", szDateTime);
	BSONObjBuilder objBuilder;
	objBuilder.append(REC_ATTR_TIME, condBuilder.obj()); // {"time":{"$gt":"2017-06-28 10:00:01","$lt":"2017-06-30 12:00:01"}}

	mongo::Query query(objBuilder.obj());
	m_pMongoConn->remove(MONGO_DB_TEMP, query);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "删除历史表:%s %s之前的数据", MONGO_DB_TEMP, szDateTime);
	return 0;
}
