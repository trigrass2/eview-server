/**************************************************************
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/

#include "TrendTimerTask.h" // �������������
#include "ace/High_Res_Timer.h"

#include <fstream>  
#include <streambuf>  
#include <iostream>  

#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "global.h"
#include "pkdata/pkdata.h"	
#include "common/eviewdefine.h"
#include "common/gettimeofday.h"

#include "MainTask.h"
#include "TrendTimer.h"

#ifndef __sun
using std::map;
using std::string;
#endif

extern CPKLog g_logger;
int GetPKMemDBPort(int &nListenPort, string &strPassword);

CTrendTimerTask::CTrendTimerTask() :m_mongoConn(true)
{
	m_bStop = false;
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, true);
	this->reactor(m_pReactor);
}

CTrendTimerTask::~CTrendTimerTask()
{
	if(m_pReactor)	
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
}


/**
 *  Start the Task and threads in the task are activated.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CTrendTimerTask::StartTask()
{
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, 1);
	if(lRet != PK_SUCCESS)
	{
		lRet = -2;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
	}

	return PK_SUCCESS;	
}

/**
 *  stop the request handler task and threads in it.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CTrendTimerTask::StopTask()
{
	m_bStop = true;
	StopTimers(); // ������ֹͣ��ʱ��
	this->reactor()->end_reactor_event_loop();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask()...");
	int nWaitResult = wait(); // ����ȴ��������أ�����������
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask() return");

	return PK_SUCCESS;	
}

/*�Ӵ����¼����������¼���Ϣ
 *  main procedure of the task.
 *	get a msgblk and 
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.

 //���� EventTypeTaskManager ActionTypeTaskManagers
 */
int CTrendTimerTask::svc()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, _("MainTask::svc() begin"));
	
	ACE_High_Res_Timer::global_scale_factor();
	this->reactor()->owner(ACE_OS::thr_self ());

	int nRet = PK_SUCCESS;
	nRet = OnStart();

	while(!m_bStop)
	{
		this->reactor()->reset_reactor_event_loop();
		this->reactor()->run_reactor_event_loop();
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CTrendTimerTask stopped");
	OnStop();

	return PK_SUCCESS;	

}

int CTrendTimerTask::OnStart()
{
	// �����ʱ�����Ƕ�ʱ����Ƿ�Ҫɾ���ļ��������Ƿ�ɹ��Ķ�ʱ��
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero + ACE_Time_Value(5); // �ӵ�0:0:5�뿪ʼ��飬�Ա���0:0:0������ݴ����� 

	ACE_Timer_Queue *timer_queue = m_pReactor->timer_queue();
	if (0 != timer_queue)
		timer_queue->schedule((ACE_Event_Handler *)this, NULL, tvStart, tvCheckConn);

	// m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvStart, tvCheckConn);

	// ��ʼ���ڴ����ݿ�.������ڳ�ʼ��mongodb֮ǰ
	int nRet = InitRedis();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TrendTimerTask::OnStart, connect pkmemdb failed! please check pkmemdb started and ip&port is correct");
		//return nRet; ����˳�������޷�ִ���ˣ�
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "TrendTimerTask::OnStart, init pkmemdb successfully!");

	// ���ɶ�ʱ��
	nRet = GenerateTimersByTagInfo();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TrendTimerTask::OnStart, generate %d trend timers failed,retcode��%d", 
			m_mapInterval2TagGroupTimer.size(), nRet);
		// return nRet; ����˳�������޷�ִ���ˣ�
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "TrendTimerTask::OnStart, generate %d trend timers successfully", m_mapInterval2TagGroupTimer.size());

	//����Mongodb���ݿ�;
	nRet = InitMongdb();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TrendTimerTask::OnStart,failed to connect to PKHisDB��check pkhisdb started first");
		//return nRet; ����˳�������޷�ִ���ˣ�
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "TrendTimerTask::OnStart, init PKHisDB successful(OnStart)");

	// ��ȡ����δ������ϵ�tempdata��20����

	//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "begin ReadLastUnProcessedTrendData(tagnum:%d) ......", MAIN_TASK->m_vecTags.size());
	//int nQueryCount = 0;
	//int nRecordCount = 0;
	//ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	//nRet = ReadLastUnProcessedTrendData(nQueryCount, nRecordCount);
	//ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	//ACE_Time_Value tvSpan = tvEnd - tvBegin;
	//float fSec = tvSpan.get_msec() / 1000.0f;
	//if (nRet != 0)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Read Last UnProcessed TempData failed, retcode��%d,time consumed:%f seconds, queryNum:%d, queryRecNum:%d", nRet, fSec, nQueryCount, nRecordCount);
	//	// return nRet; ����˳�������޷�ִ���ˣ�
	//}
	//else
	//	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Read Last UnProcessed TempData successfully,time consumed:%f seconds, queryNum:%d, queryRecNum:%d, now start timer!", fSec, nQueryCount, nRecordCount);

	StartTimers();

	return 0;
}

int CTrendTimerTask::OnStop()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop begin.");
	m_pReactor->close();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop Reactor->close().");
	// ֹͣ�����߳�;
	m_redisRW.finalize();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop end.");
	return 0;
}

int CTrendTimerTask::StopTimers()
{
	// ��ȡ����ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�
	std::map<int, CTrendTimer *>::iterator itTrendTimer = m_mapInterval2TagGroupTimer.begin();
	for (; itTrendTimer != m_mapInterval2TagGroupTimer.end(); itTrendTimer++)
	{
		CTrendTimer *pTimer = itTrendTimer->second;
		pTimer->Stop();
	}

	return 0;
}

int CTrendTimerTask::GenerateTimersByTagInfo()
{
	//��ȡTAG�㣬�����鵽CTrendTimer;
	vector<CTagInfo *>::iterator itTag;
	for (itTag = MAIN_TASK->m_vecTags.begin(); itTag != MAIN_TASK->m_vecTags.end(); itTag++)
	{
		CTagInfo *pTag = *itTag;
		if (pTag->nTrendIntervalSec <= 0)
			continue;

		// �жϲ��������ƶ�ʱ����
		CTrendTimer *pTrendTimer = NULL;
		map<int, CTrendTimer *>::iterator itTrendTimer = m_mapInterval2TagGroupTimer.find(pTag->nTrendIntervalSec);
		if (itTrendTimer == m_mapInterval2TagGroupTimer.end())
		{
			pTrendTimer = new CTrendTimer();
			pTrendTimer->SetTimerTask(this);
			pTrendTimer->m_nIntervalMS = pTag->nTrendIntervalSec * 1000; // ��λ�Ǻ���
			m_mapInterval2TagGroupTimer[pTag->nTrendIntervalSec] = pTrendTimer;
		}
		else
			pTrendTimer = itTrendTimer->second;

		pTrendTimer->m_vecTags.push_back(pTag);
		pTrendTimer->m_vecTagName.push_back(pTag->strTagName);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "GenerateTimersByTagInfo, trend %d timers", m_mapInterval2TagGroupTimer.size());

	return 0;
}


int CTrendTimerTask::StartTimers()
{
	// ��ȡ�����ƶ�ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�;
	std::map<int, CTrendTimer *>::iterator itTrendTimer = m_mapInterval2TagGroupTimer.begin();
	for (; itTrendTimer != m_mapInterval2TagGroupTimer.end(); itTrendTimer++)
	{
		CTrendTimer *pTimer = itTrendTimer->second;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "generate trend timer, interval :%d second, tag count:%d", pTimer->m_nIntervalMS/1000, pTimer->m_vecTags.size());
		int nRet = pTimer->Start();
	}

	return 0;
}

/**
*  ���ܣ���ʼ��MongoDb���ݿ�;
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CTrendTimerTask::InitMongdb()
{
	try
	{ 
		MAIN_TASK->m_mutexMongo.acquire();
		m_mongoConn.connect(MAIN_TASK->m_strMongoConnString.c_str());
		MAIN_TASK->m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "connect to pkhisdb successfully");
		 
	}
	catch (const mongo::DBException &e)
	{
		MAIN_TASK->m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "connect to pkhisdb except:%s", e.what());
		return -2;
	} 
	return PK_SUCCESS;
}
/**
*  ���ܣ���ʼ��Redis.
*
*  @version     04/10/2017  xingxing  Initial Version.
*/
int CTrendTimerTask::InitRedis()
{
	int nRet = 0;

	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nRet = m_redisRW.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MainTask m_redisRW.Initialize failed,ret:%d", nRet);
		return nRet;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "trend, Init PKMemDB success");
	return nRet;
}

// ��ȡ20���ӵ�temp���ݣ����浽ÿ��tag�㣬���������������֤����������һ���֡����ǲ���Ҫ��
//int CTrendTimerTask::ReadLastUnProcessedTrendData(int &nQueryCount, int &nRecordCount)
//{
//	int nReadedNum = 0;
//	for (vector<CTagInfo *>::iterator itTag = MAIN_TASK->m_vecTags.begin(); itTag != MAIN_TASK->m_vecTags.end(); itTag++)
//	{
//		CTagInfo *pTag = *itTag;		 
//		nReadedNum++;
//		/*if (nReadedNum % 100 == 0){
//			g_logger.LogMessage(PK_LOGLEVEL_INFO, "ReadLastUnProcessedTrendData, end to read tags from No.%d --- No.%d tags, total queryNum:%d, total recNum:%d", 
//				nReadedNum - 100, nReadedNum, nQueryCount, nRecordCount);
//		}*/
//
//		// ��mongodb��ȡ��tag����tine�ֶε����ֵ�ļ�¼�������µ�һ����¼
//		nQueryCount++;
//		CTagRawRecord *pMaxTimeRec = QueryMaxTimeRecord_From_NewestTrendTable(pTag->strTagName);
//		if (pMaxTimeRec == NULL)   // δ��ȡ��
//			 continue;
//		nRecordCount += 1;
//
//		 // ����pRecNewest�е�ʱ�䣬��ǰ����20����
//		 string strEndTime = pMaxTimeRec->strTime;
//		 time_t tmBeginTime = PKTimeHelper::String2Time((char *)strEndTime.c_str());
//		 tmBeginTime = tmBeginTime - 20 * 60; // 20 ����
//		 char szBeginTime[32];
//		 string strBeginTime = PKTimeHelper::Time2String(szBeginTime, sizeof(szBeginTime), tmBeginTime);
//
//		// ��ȡ> strRecentBeginTime �� <= pRecNewestʱ������м�¼����Լ20���ӵ����ݣ�����ŵ�vecTempRec
//		 vector<CTagRawRecord *> vecRecords;
//		 QueryRecords_From_Table_TrendNewest(pTag->strTagName, strBeginTime, strEndTime, vecRecords);
//		 for (vector<CTagRawRecord *>::iterator itRec = vecRecords.begin(); itRec != vecRecords.end(); itRec++)
//		 {
//			 pTag->vecTempRec.push_back(*itRec);
//		 }
//		 nQueryCount++;
//		 nRecordCount += vecRecords.size();
//		 // �ڴ治��delete
//		 vecRecords.clear();
//	}
//	return 0;
//}

// {"n" : "bus1con1.206ACC_onoff","id" : 1,"v" : "418","t" : "2017-06-28 18:50:09.034"}
//CTagRawRecord * CTrendTimerTask::QueryMaxTimeRecord_From_NewestTrendTable(string & strTagName)
//{
//	// select * from t order by t limit 1
//	mongo::Query query(mongo::BSONObjBuilder().append(REC_ATTR_TAGNAME, strTagName).obj());
//	query.sort(mongo::BSONObjBuilder().append(REC_ATTR_TIME, -1).obj());
//	try{
//		mongo::BSONObj bsonObj = m_mongoConn.findOne(MONGO_DB_TABLE_TREND_NEWEST, query); // ���µ���ʷ��¼��
//		if (bsonObj.isEmpty())
//			return NULL;
//
//		CTagRawRecord *pRawRec = new CTagRawRecord();
//		pRawRec->strTagName = strTagName;
//		pRawRec->strTime = bsonObj.getStringField(REC_ATTR_TIME);
//		pRawRec->nQuality = bsonObj.getIntField(REC_ATTR_QUALITY);
//		pRawRec->dbValue = bsonObj.getIntField(REC_ATTR_VALUE);
//
//		return pRawRec;
//	}
//	catch (mongo::DBException& e) {
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//		return NULL;
//	}
//	catch (std::exception& e) {
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
//		return NULL;
//	}
//	catch (...){
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
//		return NULL;
//	}
//	
//	return NULL;
//}

//int CTrendTimerTask::QueryRecords_From_Table_TrendNewest(string strTagName, string strBeginTime, string strEndTime, vector<CTagRawRecord *> &vecRecords)
//{
//	mongo::BSONObjBuilder condBuilder;
//	condBuilder.append("$gte", strBeginTime).append("$lt", strEndTime);
//	mongo::BSONObjBuilder objBuilder;
//	objBuilder.append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, condBuilder.obj()); // {"time":{"$gt":"2017-06-28 10:00:01","$lt":"2017-06-30 12:00:01"}}
//	mongo::Query query(objBuilder.obj());
//	query.sort(mongo::BSONObjBuilder().append(REC_ATTR_TIME, 1).obj());
//	try{
//		std::auto_ptr<mongo::DBClientCursor> cursor = m_mongoConn.query(MONGO_DB_TABLE_TREND_NEWEST, query);
//		while (cursor->more())
//		{
//			mongo::BSONObj bsonObj = cursor->next();
//			CTagRawRecord *pRawRec = new CTagRawRecord();
//			pRawRec->strTagName = strTagName;
//			pRawRec->strTime = bsonObj.getStringField(REC_ATTR_TIME);
//			pRawRec->nQuality = bsonObj.getIntField(REC_ATTR_QUALITY);
//			pRawRec->dbValue = bsonObj.getIntField(REC_ATTR_VALUE);
//			vecRecords.push_back(pRawRec);
//		}
//		return 0;
//	}
//	catch (mongo::DBException& e) {
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


