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
	// �������벻�ظ������ݵ�tempTable
	if (pTag->vecTempRec.size() > 0)
	{
		CTagRawRecord *pTagRaw = pTag->vecTempRec[pTag->vecTempRec.size() - 1];
		if (pTagRaw->strTime.compare(strNewRecTime) == 0 && pTagRaw->nQuality == nNewRecQuality && pTagRaw->dbValue == dbNewRecValue)
		{
			// �ظ��ļ�¼������Ҫ��������
			return 0;
		}
	}

	CTagRawRecord *pTempRec = new CTagRawRecord();
	pTempRec->dbValue = dbNewRecValue;
	pTempRec->strTagName = pTag->strTagName;
	pTempRec->strTime = strNewRecTime;
	pTempRec->nQuality = nNewRecQuality;

	// ���µĲ��ظ����ݲ��뵽temp��
	RealData_TO_TempTable(pTag->strTagName, pTempRec);

	pTag->vecTempRec.push_back(pTempRec);
	if (pTag->vecTempRec.size() <= 1)	 // ֻ��һ�����ݣ�����Ҫ����
		return 0;

	// �������ֵ��˵���ڴ���û����õĵط���ɾ��ͷ������
	if (pTag->vecTempRec.size() > 20000)
	{
		pTag->vecTempRec.erase(pTag->vecTempRec.begin());
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����%s,temp���еļ�¼����̫����:%d������20000", pTag->strTagName.c_str(), pTag->vecTempRec.size());
	}

	CTagRawRecord * pRecNewest = pTempRec;	// ���µļ�¼
	CTagRawRecord * pRecSecondNewest = pTag->vecTempRec[pTag->vecTempRec.size() - 2]; // ���µ�һ����¼
	CTagRawRecord * pRecOldest = pTag->vecTempRec[0]; // ���ϵļ�¼

	// ����һ�����ݺ����µ����ݣ��ǲ���������1����ͬ���ڲ���ͬһ��10���ӵ������ڣ�������ڣ���ֹͣ��������������´���
	// ��ȷ��16���ӵ�����
	string strNewestRec_ymdhm = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	string strSecondNewest_ymdhm = SubString(pRecSecondNewest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	string strOldestRec_ymdhm = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15

	string strNewestRec_ymdh10m = SubString(pRecNewest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	string strOldestRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1

	if (strSecondNewest_ymdhm.compare(strNewestRec_ymdhm) == 0)   // ���µļ�¼��yymmddhhmm�����µļ�¼�ķ���ʱ����ͬ��˵��ʱ��û�иı䣬û���µķ�������
	{
		return 0;
	}

	// ��Ҫ���ϵ����ݼ�����ŵ�raw��
	int iRecord = 0;
	string strOlderRec_ymdhm = strOldestRec_ymdhm; // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	string strOlderRec_ymdh10m = SubString(pRecOldest->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:15
	// �������У�2017-06-28 10:30(20����  ��2017-06-28 10:40(20������2017-06-28 10:50(30������2017-06-28 11:10(20����
	// strOldestRec_ymdh10m == 	2017-06-28 10:30	  strNewestRec_ymdh10m == 	2017-06-28 11:10
	// strOlderRec_ymdh10m ���ε��� 2017-06-28 10:30(20���� ��2017-06-28 10:40(20������2017-06-28 10:50(30����
	for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end();)
	{
		CTagRawRecord *pRec = *itRec;
		string strCurRec_ymdhm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
		if (strCurRec_ymdhm.compare(strNewestRec_ymdhm) == 0) // ����1����ʱ��ε����ݺ͵�ǰ��¼����ͬһ���ӣ����ܱ��� 
			break;

		// ��ǰ���ݺ����µ�һ���Ӳ�ͬ����Ҫ���档���ҳ�����һ������������10���ӵ����м�¼
		vector<CTagRawRecord *> vec1MinuteTempRecToRaw;

		// 10����ʱ�����һ��strOlderRec_ymdh10m��ͬ������Ϊ��ͬһ��10���ӵ����ݣ�һ������
		vector<CTagRawRecord *>::iterator itRec2 = itRec;
		for (; itRec2 != pTag->vecTempRec.end(); itRec2++)
		{
			CTagRawRecord *pRec2 = *itRec2;
			string strTmpymdhm = SubString(pRec2->strTime, TIMELEN_YYYYMMDD_HHMM);
			if (strTmpymdhm.compare(strCurRec_ymdhm) == 0)  // �������µķ��ӣ���Ҫ����
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
		itRec = itRec2; //itRec2ָ����һ��Ҫ����ķ��ӻ������·���
		pRec = *itRec;

		if (vec1MinuteTempRecToRaw.size() > 0)
		{
			Write_1m_Rec_TO_RawTable(pTag->strTagName, strCurRec_ymdhm, vec1MinuteTempRecToRaw);
		}

		vec1MinuteTempRecToRaw.clear();
		vec1MinuteTempRecToRaw.push_back(pRec); //	  �����¸�ʮ����
	}

	// ɾ��Temp�ڴ����Ѿ� ���浽raw������ݡ���	pRecOldest��ɾ����ʱ��===strNewestRec_ymdh10m	 Ϊֹ
	// ��������10���ӵ����ݣ�
	strNewestRec_ymdh10m = SubString(strNewestRec_ymdhm, strNewestRec_ymdhm.length() - 1);
	for (vector<CTagRawRecord *>::iterator itRec = pTag->vecTempRec.begin(); itRec != pTag->vecTempRec.end(); itRec)   // �����µ�10��������ʱ�䲻ͬ����Ҫ���ϵ����ݼ�����ŵ�raw��
	{
		CTagRawRecord *pRec = *itRec;
		string strCurRec_ymdh10m = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHM); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
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
*  ���ܣ��������Tag��Ľ���������MongoDb����д�빤��.
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
		if (curStr.empty()) // ���ܻ��ȡ�����ַ���
		{
			nTagNum_NoRealData++;
			// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����%s������tag���ݵ�δ��ȡ��", pTag->strTagName.c_str());
			continue;
		}
        //PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "value string:%s", curStr.c_str());

		Json::Reader reader;
		Json::Value root;
		reader.parse(curStr, root, false);

		//�ж����ݱ�������:int \ double \ string
		string strNewRecTime = root[REC_ATTR_TIME].asString();;

		// ʱ�䳤�ȵĺϷ����ж�
		if (strNewRecTime.length() < TIMELEN_YYYYMMDD_HHMMSSXXX) // ʱ�䣬yyyy-MM-dd hh:mm:ss.xxx������==24���ֽ�
		{
			nTagNum_TimeFormatError++;
			//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����%s������tag���ݵ�ʱ�䣺%s��ʽ�Ƿ�������Ϊ24���ֽ�", pTag->strTagName.c_str(), pTempRec->strTime.c_str());
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

	string strSubString = src.substr(0, nLen); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	return strSubString;
}
// ������д�뵽mongodb��temp��
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

// ÿ10���ӵ�������Ϊһ����¼�����ȥ
// {"n" : "bus1con1.206ACC_onoff","t" : "2017-06-28 18:50","vs" : "[{"q":"0","t":"2017-06-28 18:50:12.034","v":"810"},{"q":"0","t":"2017-06-28 18:50:14.039","v":"716"},{"q":"0","t":"2017-06-28 18:50:15.034","v":"445"},{"q":"0","t":"2017-06-28 18:50:18.034","v":"661"},{"q":"0","t":"2017-06-28 18:50:20.034","v":"695"},{"q":"0","t":"2017-06-28 18:50:22.610","v":"902"},{"q":"0","t":"2017-06-28 18:50:24.035","v":"651"}]"}
int CSaveTimer::Write_10m_Rec_TO_RawTable(string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute)
{
	MAIN_TASK->EnsureTableIndex(strTableName); // Ϊ����������

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

// ��һ���ӵ�����д�뵽mongodb��raw	��raw_yymmdd. tΪ��yy-mm-dd hh:m0, 2017-06-28 15:50
// raw��ÿ10����1����¼�������ѯ��10���Ӽ�¼���ڣ�����£�������롣һ����ִ��һ��
int CSaveTimer::Write_1m_Rec_TO_RawTable(string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute)
{
	// ���� strYYMMDDHHM	�е�strYYMMDDHH
	string strYYYYMMDD = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD);
	string strTableName = MONGO_DB_RAW_PREFIX + strYYYYMMDD;
	MAIN_TASK->EnsureTableIndex(strTableName); // Ϊ����������
	string strYYYYMMDD10M = SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HHM) + "0";

	vector<CTagRawRecord *> vecOld10MinuteRec; // ��Ҫ�ֹ�ɾ������ڴ�
	int nRet = Query_Raw_Record(strTagName, strYYYYMMDD10M, vecOld10MinuteRec); // ��ѯ��10���ӵ����ݣ�ÿ��������600��ԭʼ�������

	vector<CTagRawRecord *> vecNew10MinuteRec;
	bool bMerged = MergeRawDataRecs(strTagName, vecOld10MinuteRec, vecRecOf1Minute, vecNew10MinuteRec);
	if (bMerged)
	{
		Write_10m_Rec_TO_RawTable(strTagName, strTableName, strYYYYMMDD10M, vecNew10MinuteRec);
		// DeleteTempRecordVector(vecNewRec); ��ЩCTagRawRecord�ں��汣�����ڴ����ˣ�����ɾ����
		RealData_TO_MinuteTable(strTagName, SubString(strYYMMDDHHMM, TIMELEN_YYYYMMDD_HH), vecNew10MinuteRec);
	}
	// vecRecOf1Minute���ڴ��Ǵ�pTag->vecTemp�л�ȡ�ģ�����ɾ����
	// ��Щ�ڴ���Queryʱ��ʱ���ٵģ���Ҫ�ֹ�ɾ��
	DeleteTempRecordVector(vecOld10MinuteRec);
	return 0;
}

// ��minute_yyyyMM���л�ȡ��һ����¼�е���������
int CSaveTimer::Query_60m_Records_From_MinuteTable(string & strTagName, string strYYYYMMDDHH, vector<CTagStatRecord *> &vecRecOf60M)
{
	string strYYYYMM = SubString(strYYYYMMDDHH, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_MINUTE_PREFIX + strYYYYMM;
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, strTableName, strYYYYMMDDHH, vecRecOf60M);
}

// ��hour_yyyyMM���л�ȡ��һ����¼�е���������
int CSaveTimer::Query_24h_Records_From_HourTable(string & strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vecRecOf24h)
{
	string strYYYYMM = SubString(strYYYYMMDD, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_HOUR_PREFIX + strYYYYMM;
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, strTableName, strYYYYMMDD, vecRecOf24h);
}

// ��day���л�ȡ��һ����¼�е���������
int CSaveTimer::Query_30d_Records_From_DayTable(string & strTagName, string strYYYYMM, vector<CTagStatRecord *> &vecRecOf30d)
{
	return Query_Stat_CompoundRecords_By_NameAndTime(strTagName, MONGO_DB_DAY, strYYYYMM, vecRecOf30d);
}

// ��day���л�ȡ��һ����¼�е���������
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

	// ��һ�����ֵ����VTQ
	CTagRawRecord *pRecFirst = vecRawRecOf1Minute[0];
	pMinuteRec->strFirstTime = pRecFirst->strTime;
	pMinuteRec->strTime = SubString(pMinuteRec->strFirstTime, TIMELEN_YYYYMMDD_HHMM); // ʱ��Ϊ��2017-06-29 15:21a
	pMinuteRec->nQuality = pRecFirst->nQuality;
	pMinuteRec->dbValue = pRecFirst->dbValue;
	// ����vec1MinuteRec�еĵ�һ��ֵ��ƽ��ֵ�����ֵ����Сֵ������
	double dbSum = 0;
	for (vector<CTagRawRecord *>::iterator itRec2 = vecRawRecOf1Minute.begin(); itRec2 != vecRawRecOf1Minute.end(); itRec2++)
	{
		CTagRawRecord *pRec2 = *itRec2;
		// �����VTQ
		if (pMinuteRec->dbMaxValue < pRec2->dbValue)
		{
			pMinuteRec->dbMaxValue = pRec2->dbValue;
			pMinuteRec->strMaxTime = pRec2->strTime;
			pMinuteRec->nMaxQuality = pRec2->nQuality;
		}
		// ��С���VTQ
		if (pMinuteRec->dbMinValue > pRec2->dbValue)
		{
			pMinuteRec->dbMinValue = pRec2->dbValue;
			pMinuteRec->strMinTime = pRec2->strTime;
			pMinuteRec->nMinQuality = pRec2->nQuality;
		}
		dbSum += pRec2->dbValue;
	}
	// ��ĸ�����ƽ��ֵ
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
//// �±� ��һ����¼û�� vs��������
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

// һ����ִ��һ�Ρ�������д�뵽mongodb��minute_YYYYMM��
// ���� strYYMMDDHHM	�е�strYYMMDDHH����mongo��minute_YYYYMMDD�����ü�¼�Ƿ����
// ������ڸü�¼���˳�
// �����ڣ�����mongo minute_YYYYMMDD
// һ����¼�����60���ӵ�����
int CSaveTimer::RealData_TO_MinuteTable(string &strTagName, string strYYMMDDHH, vector<CTagRawRecord *> &vec10MinuteRawRec)
{
	// ���� strYYMMDDHHM	�е�strYYMMDDHH
	string strYYYYMM = SubString(strYYMMDDHH, TIMELEN_YYYYMM);
	string strTableName = MONGO_DB_MINUTE_PREFIX + strYYYYMM;
	MAIN_TASK->EnsureTableIndex(strTableName); // Ϊ����������

	// ��10�����ڵ����е�һ���Ӽ�¼�������������������
	// 10����ʱ�����һ��strOlderRec_ymdh10m��ͬ������Ϊ��ͬһ��10���ӵ����ݣ�һ������
	vector<CTagStatRecord *> vecNewMinutesRec; // ÿ����¼��1���ӵ����ݣ�
	vector<CTagRawRecord *> vecRawRecOf1Minute;
	CTagRawRecord *pRecFirst = vec10MinuteRawRec[0];
	string strLastMinute_yyyyMMdd_hhmm = SubString(pRecFirst->strTime, TIMELEN_YYYYMMDD_HHMM);
	for (vector<CTagRawRecord *>::iterator itRec = vec10MinuteRawRec.begin(); itRec != vec10MinuteRawRec.end(); itRec++)
	{
		CTagRawRecord *pRec = *itRec;
		string strCurMinute_yyyyMMdd_hhmm = SubString(pRec->strTime, TIMELEN_YYYYMMDD_HHMM);
		if (strCurMinute_yyyyMMdd_hhmm.compare(strLastMinute_yyyyMMdd_hhmm) != 0) // �µ�һ�����ˣ�
		{
			string strYYMMDDMM_HH = SubString(strCurMinute_yyyyMMdd_hhmm, TIMELEN_YYYYMMDD_HH); // ����һ�������Ƿ��С����������Ҫ���²�����
			CTagStatRecord *pMinuteRec = Calc1MinuteStatRecFromRealDatas(strTagName, vecRawRecOf1Minute);
			vecNewMinutesRec.push_back(pMinuteRec);
			vecRawRecOf1Minute.clear();
			strLastMinute_yyyyMMdd_hhmm = strCurMinute_yyyyMMdd_hhmm;
		}
		
		vecRawRecOf1Minute.push_back(*itRec); //	strOlderRec_ymdh10m		
	}
	// ���1���ӵ�����
	if (vecRawRecOf1Minute.size() > 0)
	{
		CTagStatRecord *pMinuteRec = Calc1MinuteStatRecFromRealDatas(strTagName, vecRawRecOf1Minute);
		vecNewMinutesRec.push_back(pMinuteRec);
		vecRawRecOf1Minute.clear();
	}

	// �ϲ�10���Ӻ��¼����1���ӵ�ԭʼ��¼
	vector<CTagStatRecord *> vecOldRecOf60M;
	int nRet = Query_60m_Records_From_MinuteTable(strTagName, strYYMMDDHH, vecOldRecOf60M); // ��ѯ��60���ӵ�����

	vector<CTagStatRecord *> vecNew60mRec;
	bool bMerged = MergeStatDataRecs(strTagName, TIMELEN_YYYYMMDD_HHMM, vecOldRecOf60M, vecNewMinutesRec, vecNew60mRec);
	
	if (bMerged)
	{
		Write_60m_records_TO_MinuteTable(strTagName, strTableName, strYYMMDDHH, vecNew60mRec);

		string strYYYYMMDD = SubString(strYYMMDDHH, TIMELEN_YYYYMMDD);
		MinuteData_TO_HourTable(strTagName, strYYMMDDHH, vecNew60mRec);
	}

	// ��Щ�ڴ���Queryʱ��ʱ���ٵģ���Ҫ�ֹ�ɾ��
	DeleteStatRecordVector(vecNew60mRec);

	return 0;
}

// ��ѯ1��raw_yyyyMMdd���еļ�¼����ת��Ϊvector<CTagRawRecord *>����
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

// ����һ����ͬ���ڵ�ͳ�����ݣ��õ��������ڵ����ݡ��磺����minute--->hour  nResultRecTimeLen��hour��ʱ�䳤��
CTagStatRecord * CSaveTimer::CalcStatRowFromStatDatas(string strTagName, int nResultRecTimeLen, vector<CTagStatRecord *> &vecTempData)
{
	if (vecTempData.size() <= 0)
		return NULL;

	CTagStatRecord *pResultRec = new CTagStatRecord();
	pResultRec->strTagName = strTagName;

	// ��һ�����ֵ����VTQ
	CTagStatRecord *pRecFirst = vecTempData[0];
	pResultRec->strFirstTime = pRecFirst->strFirstTime;
	pResultRec->strTime = SubString(pRecFirst->strTime, nResultRecTimeLen);
	pResultRec->nQuality = pRecFirst->nQuality;
	pResultRec->dbValue = pRecFirst->dbValue;

	// ����vec1MinuteRec�еĵ�һ��ֵ��ƽ��ֵ�����ֵ����Сֵ������
	double dbSum = 0;
	for (vector<CTagStatRecord *>::iterator itRec2 = vecTempData.begin(); itRec2 != vecTempData.end(); itRec2++)
	{
		CTagStatRecord *pSrcRec = *itRec2;
		// �����VTQ
		if (pResultRec->dbMaxValue < pSrcRec->dbMaxValue)
		{
			pResultRec->dbMaxValue = pSrcRec->dbMaxValue;
			pResultRec->strMaxTime = pSrcRec->strMaxTime;
			pResultRec->nMaxQuality = pSrcRec->nMaxQuality;
		}
		// ��С���VTQ
		if (pResultRec->dbMinValue > pSrcRec->dbMinValue)
		{
			pResultRec->dbMinValue = pSrcRec->dbMinValue;
			pResultRec->strMinTime = pSrcRec->strMinTime;
			pResultRec->nMinQuality = pSrcRec->nMinQuality;
		}

		pResultRec->nDataCount += pSrcRec->nDataCount;
		dbSum += pSrcRec->dbAvg * pSrcRec->nDataCount; // �����ܺ�
	}
	// ��ĸ�����ƽ��ֵ
	pResultRec->dbAvg = dbSum / pResultRec->nDataCount;
	return pResultRec;
}

// ÿ���ӱ�����ʱ:
// ����YYYYMMDDHH�ҵ�minute_yyyyMM��ĸ�����¼�������Сʱ��ͳ��ֵ
// ����YYYYMMDD�ҵ�hour_yyyyMM��ĸ�����¼�������������Сʱֵ�ϲ�
// ��󽫺ϲ����ֵ���µ�hour_yyyyMM��
int CSaveTimer::MinuteData_TO_HourTable(string strTagName, string strYYYYMMDDHH, vector<CTagStatRecord *> &vec60mRec)
{
	// �����Сʱ��ͳ��ֵ
	CTagStatRecord *pHourRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMMDD_HH, vec60mRec);
	if (!pHourRec)
		return -1;

	// �õ�hour_yyyyMMСʱ���е�һ����¼��24��Сʱ��1������ݣ�
	string strYYYYMMDD = SubString(strYYYYMMDDHH, TIMELEN_YYYYMMDD);
	vector<CTagStatRecord *> vecOldRecOf24h;
	this->Query_24h_Records_From_HourTable(strTagName, strYYYYMMDD, vecOldRecOf24h);

	// �ϲ��µ�1Сʱ��¼�����е�24Сʱ�ļ�¼
	vector<CTagStatRecord *> vecNewRecOf24h;
	int nRecTimeLen = TIMELEN_YYYYMMDD_HH; // Сʱ���У�ÿ����¼��ʾ24Сʱ������vs��ÿ��Ԫ�ص�ʱ�䳤�ȵ�Сʱ
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf24h, pHourRec, vecNewRecOf24h);
	if (bMerged)
	{
		// ���ϲ���ļ�¼���µ�Сʱ����
		string strTableName = MONGO_DB_HOUR_PREFIX + SubString(strYYYYMMDDHH, TIMELEN_YYYYMM);
		Write_24h_records_TO_HourTable(strTagName, strTableName, strYYYYMMDD, vecNewRecOf24h);

		// �������µ���
		HourData_TO_DayTable(strTagName, strYYYYMMDD, vecNewRecOf24h);
	}
	// ��Щ�ڴ���Queryʱ��ʱ���ٵģ���Ҫ�ֹ�ɾ��
	DeleteStatRecordVector(vecNewRecOf24h); // �ͷ��ڴ�
	return 0;
}

// �ҵ�minute��Ĵ��¼�¼��ÿ���ӱ�����ʱ���ȵ�Minute_strYYYYMMDD���У�ȡ���µ�2����¼����2��Сʱ�ļ�¼�� ���ֻ��һ����¼���������ȡһ�����µļ�¼�� ��������¼�ҵ����µļ�¼����Ϊ�����ж��Ƿ�Ҫ����Hour�������
// �������¼�¼��YYYYHHMMHHȥ����Hour_YYHHMM���Ҳ�������� ��Сʱ���ݣ����뵽Hour���ҵ�������㲢���¸�Hour��ļ�¼
int CSaveTimer::HourData_TO_DayTable(string strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vec24hRec)
{
	// �����Сʱ��ͳ��ֵ
	CTagStatRecord *pDayRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMMDD, vec24hRec);
	if (!pDayRec)
		return -1;

	// �õ�hour_yyyyMMСʱ���е�һ����¼��24��Сʱ��1������ݣ�
	string strYYYYMM = SubString(strYYYYMMDD, TIMELEN_YYYYMM);
	vector<CTagStatRecord *> vecOldRecOf30d;
	this->Query_30d_Records_From_DayTable(strTagName, strYYYYMM, vecOldRecOf30d);

	// �ϲ��µ�1����¼�����е�30��ļ�¼
	vector<CTagStatRecord *> vecNewRecOf30d;
	int nRecTimeLen = TIMELEN_YYYYMMDD; // Сʱ���У�ÿ����¼��ʾ24Сʱ������vs��ÿ��Ԫ�ص�ʱ�䳤�ȵ�Сʱ
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf30d, pDayRec, vecNewRecOf30d);
	if (bMerged)
	{
		// ���ϲ���ļ�¼���µ�Сʱ����
		Write_30d_records_TO_DayTable(strTagName, MONGO_DB_DAY, strYYYYMM, vecNewRecOf30d);
		DayData_TO_MonthTable(strTagName, strYYYYMM, vecNewRecOf30d);
	}

	DeleteStatRecordVector(vecNewRecOf30d); // �ͷ��ڴ�

	return 0;
}

// �ҵ�minute��Ĵ��¼�¼��ÿ���ӱ�����ʱ���ȵ�Minute_strYYYYMMDD���У�ȡ���µ�2����¼����2��Сʱ�ļ�¼�� ���ֻ��һ����¼���������ȡһ�����µļ�¼�� ��������¼�ҵ����µļ�¼����Ϊ�����ж��Ƿ�Ҫ����Hour�������
// �������¼�¼��YYYYHHMMHHȥ����Hour_YYHHMM���Ҳ�������� ��Сʱ���ݣ����뵽Hour���ҵ�������㲢���¸�Hour��ļ�¼
int CSaveTimer::DayData_TO_MonthTable(string strTagName, string strYYYYMM, vector<CTagStatRecord *> &vec30dRec)
{
	// �����Сʱ��ͳ��ֵ
	CTagStatRecord *pMonthRec = CalcStatRowFromStatDatas(strTagName, TIMELEN_YYYYMM, vec30dRec);
	if (!pMonthRec)
		return -1;

	// �õ�hour_yyyyMMСʱ���е�һ����¼��24��Сʱ��1������ݣ�
	string strYYYY = SubString(strYYYYMM, TIMELEN_YYYY); // ÿ��1����¼
	vector<CTagStatRecord *> vecOldRecOf12M;
	this->Query_12Month_Records_From_MonthTable(strTagName, strYYYY, vecOldRecOf12M);

	// �ϲ��µ�1����¼�����е�30��ļ�¼
	vector<CTagStatRecord *> vecNewRecOf12M;
	int nRecTimeLen = TIMELEN_YYYYMM; // �±��У�ÿ����¼��ʾ12���£�����vs��ÿ��Ԫ�ص�ʱ�䳤�ȵ�Сʱ
	bool bMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecOldRecOf12M, pMonthRec, vecNewRecOf12M);
	if (bMerged)
	{
		// ���ϲ���ļ�¼���µ�Сʱ����
		Write_12M_records_TO_MonthTable(strTagName, MONGO_DB_MONTH, strYYYY, vecNewRecOf12M);
	}

	DeleteStatRecordVector(vecNewRecOf12M); // �ͷ��ڴ�

	// ���ϲ���ļ�¼���µ�Сʱ����
	// Write_1M_records_TO_MonthTable(strTagName, MONGO_DB_MONTH, strYYYYMM, pMonthRec);

	//delete pMonthRec;
	//pMonthRec = NULL;
	return 0;
}

// �ϲ�����vector��
// �����Ƿ�������vecBaseRec��ͬ
// vecBaseRec���ϲ��Ļ�׼�����1
// vecRecToMerge��Ҫ�ϲ�����Ľ����
// vecNewRec���ϲ����
bool CSaveTimer::MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, CTagStatRecord * pRecToMerge, vector<CTagStatRecord *> &vecNewRec)
{
	vector<CTagStatRecord *> vecToMerge;
	vecToMerge.push_back(pRecToMerge);
	bool bHasMerged = MergeStatDataRecs(strTagName, nRecTimeLen, vecBaseRec, vecToMerge, vecNewRec);
	vecToMerge.clear();
	return bHasMerged;
}

// �ϲ�����vector��
// �����Ƿ�������vecBaseRec��ͬ
// vecBaseRec���ϲ��Ļ�׼�����1
// vecRecToMerge��Ҫ�ϲ�����Ľ����
// vecNewRec���ϲ����
bool CSaveTimer::MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, vector<CTagStatRecord *> &vecRecToMerge, vector<CTagStatRecord *> &vecNewRec)
{
	bool bHasVecMerged = false; // �Ƿ���vecBaseRec������������

	// ��mongo��minute_YYYYMM�����ü�¼�Ƿ���ڲ�ȡ��
	map<string, CTagStatRecord *> mapTime2Rec; // ʹ��map���������ж��ظ�������
	for (vector<CTagStatRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
	{
		CTagStatRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nRecTimeLen); // ��Сʱ����ʱ��Ϊvs��ÿ��Ԫ�ص�t����yyyyMMdd hh
		mapTime2Rec[strRecTime] = pTmpRec;
	}

	for (vector<CTagStatRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
	{
		CTagStatRecord *pRecToMerge = *itRec;
		string strRecTime = pRecToMerge->strTime;// .substr(0, nRecTimeLen); // ��Сʱ����ʱ��Ϊvs��ÿ��Ԫ�ص�t����yyyyMMdd hh
		map<string, CTagStatRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
		if (itFound != mapTime2Rec.end()) // �ҵ��˸�����¼
		{
			// ��������ʱ�������
			CTagStatRecord *pOldRec = itFound->second;
			if (pOldRec->nDataCount != pRecToMerge->nDataCount) // ��¼����ͬ����Ϊ�޸Ĺ���
			{
				delete pOldRec;
				pOldRec = NULL;
				mapTime2Rec[strRecTime] = pRecToMerge;
				bHasVecMerged = true;
			}
			else // ��Ϊû���޸Ĺ���ɾ���µ��Ǹ�
			{
				delete pRecToMerge;
			}
		}
		else // δ�ҵ��ü�¼
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


// �ϲ�����vector��
// �����Ƿ�������vecBaseRec��ͬ
// vecBaseRec���ϲ��Ļ�׼�����1
// vecRecToMerge��Ҫ�ϲ�����Ľ����
// vecNewRec���ϲ����
bool CSaveTimer::MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec)
{
	bool bHasVecMerged = false; // �Ƿ���vecBaseRec������������
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

	// ��mongo�ģ����ü�¼�Ƿ���ڲ�ȡ��
	map<string, CTagRawRecord *> mapTime2Rec; // ʹ��map���������ж��ظ�������
	for (vector<CTagRawRecord *>::iterator itRec = vecBaseRec.begin(); itRec != vecBaseRec.end(); itRec++)
	{
		CTagRawRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); // ԭʼ����temp�ĺϲ�ȡTIMELEN_YYYYMMDD_HHMMSS.XXX
		mapTime2Rec[strRecTime] = pTmpRec;
	}

	for (vector<CTagRawRecord *>::iterator itRec = vecRecToMerge.begin(); itRec != vecRecToMerge.end(); itRec++)
	{
		CTagRawRecord *pTmpRec = *itRec;
		string strRecTime = pTmpRec->strTime;// .substr(0, nTimeLen); ԭʼ����temp�ĺϲ�ȡTIMELEN_YYYYMMDD_HHMMSS.XXX
		map<string, CTagRawRecord *>::iterator itFound = mapTime2Rec.find(strRecTime);
		if (itFound != mapTime2Rec.end())
		{
			CTagRawRecord *pOldRec = itFound->second;
			if (pTmpRec->dbValue != pOldRec->dbValue || pTmpRec->nQuality != pOldRec->nQuality)
			{
				// ��������ʱ�������.������delete����Ϊ��Ҫ�����pTag->vecTemp��
				//delete pOldRec;
				//pOldRec = NULL;
				mapTime2Rec[strRecTime] = pTmpRec;
				bHasVecMerged = true;
			}
			else
			{
				// ������delete����Ϊ��Ҫ�����pTag->vecTemp��
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
	time_t tmTenMinutesAo = tmNow - 600; // ʮ����ǰ��ʱ��
	// ɾ������Ҫ�����ݱ�;
	char szDateTime[32] = { 0 };
	PKTimeHelper::Time2String(szDateTime, sizeof(szDateTime), tmTenMinutesAo);

	BSONObjBuilder condBuilder;
	condBuilder.append("$lt", szDateTime);
	BSONObjBuilder objBuilder;
	objBuilder.append(REC_ATTR_TIME, condBuilder.obj()); // {"time":{"$gt":"2017-06-28 10:00:01","$lt":"2017-06-30 12:00:01"}}

	mongo::Query query(objBuilder.obj());
	m_pMongoConn->remove(MONGO_DB_TEMP, query);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "ɾ����ʷ��:%s %s֮ǰ������", MONGO_DB_TEMP, szDateTime);
	return 0;
}
