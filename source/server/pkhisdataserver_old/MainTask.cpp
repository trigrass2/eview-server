/**************************************************************
 *  Filename:    ExecutionTak.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ִ��Ԥ�����̣߳����յ���Ϣ��������Դ����������ִ����ϵ�.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
 *  @version     08/09/2010  chenzhiquan  ����¼���ϸ������ִ�н��Ϊִ�С��ɹ���or��ʧ�ܡ�
 *  @version     08/10/2010  chenzhiquan  �޸��¼������Ԥ������ɹ���ʧ��
 *  @version	 07/12/2013	 zhangqiang 
**************************************************************/

#include "MainTask.h"
#include "ace/High_Res_Timer.h"
#include "SaveTimer.h"
#include "common/gettimeofday.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "common/eviewdefine.h"
#include "global.h"
#include "pkdata/pkdata.h"	

#include <fstream>  
#include <streambuf>  
#include <iostream>  

using namespace std;
using namespace mongo;

#ifndef __sun
using std::map;
using std::string;
#endif

#define NULLASSTRING(x) x==NULL?"":x
extern CPKLog PKLog;


CMainTask::CMainTask() :m_mongoConn(true)
{
	m_bStop = false;
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, true);
	this->reactor(m_pReactor);
	m_strMongoConnString = "";
}

CMainTask::~CMainTask()
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
long CMainTask::StartTask()
{
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, 1);
	if(lRet != PK_SUCCESS)
	{
		lRet = -2;
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
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
long CMainTask::StopTask()
{
	m_bStop = true;
	this->reactor()->end_reactor_event_loop();

	int nWaitResult = wait();

	return PK_SUCCESS;	
}


//�������յ���event-alarm  �����ı�����Ϣmsg  ��ִ�ж����ķ���,�������� cmdcode
//int CMainTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
//{
//	return 0;
//}


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
int CMainTask::svc()
{
	PKLog.LogMessage(PK_LOGLEVEL_INFO, _("MainTask::svc() begin"));
	
	ACE_High_Res_Timer::global_scale_factor();
	this->reactor()->owner(ACE_OS::thr_self ());

	int nRet = PK_SUCCESS;
	nRet = OnStart();

	while(!m_bStop)
	{
		this->reactor()->reset_reactor_event_loop();
		this->reactor()->run_reactor_event_loop();
	}

	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CMainTask stopped");
	OnStop();

	return PK_SUCCESS;	
}

int CMainTask::OnStart()
{
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero; 
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

	// ��ʼ���ڴ����ݿ�.������ڳ�ʼ��mongodb֮ǰ
	int nRet = InitRedis();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "connect pkmemdb failed! please check pkmemdb started and ip&port is correct");
		//return nRet; ����˳�������޷�ִ���ˣ�
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "init pkmemdb successfully!");

	//��ȡ���������Ϣ;
	nRet = LoadConfig();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "LoadConfig failed,ret:%d", nRet);
		return nRet;
	}

	// ���ɶ�ʱ��
	nRet = GenerateTimersByTagInfo();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "generate %d timers failed,retcode��%d", m_mapInterval2TagGroupTimer.size(), nRet);
		// return nRet; ����˳�������޷�ִ���ˣ�
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "generate %d timers successfully", m_mapInterval2TagGroupTimer.size());

	//����Mongodb���ݿ�;
	nRet = InitMongdb();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "failed to connect to PKHisDB��check pkhisdb started first");
		//return nRet; ����˳�������޷�ִ���ˣ�
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "init PKHisDB successful(OnStart)");

	// ��ȡ����δ������ϵ�tempdata��20����

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "begin ReadLastUnProcessedTempData(tagnum:%d) ......",m_vecTags.size());
	int nQueryCount = 0;
	int nRecordCount = 0;
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	nRet = ReadLastUnProcessedTempData(nQueryCount, nRecordCount);
	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvEnd - tvBegin;
	float fSec = tvSpan.msec() / 1000.0f;
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Read Last UnProcessed TempData failed, retcode��%d,time consumed:%f seconds, queryNum:%d, queryRecNum:%d", nRet, fSec, nQueryCount, nRecordCount);
		// return nRet; ����˳�������޷�ִ���ˣ�
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Read Last UnProcessed TempData successfully,time consumed:%f seconds, queryNum:%d, queryRecNum:%d, now start timer!", fSec, nQueryCount, nRecordCount);

	StartProcessTimers();

	return 0;
}

int CMainTask::OnStop()
{
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop begin.");
	m_pReactor->close();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop Reactor->close().");
	// ֹͣ�����߳�;
	m_redisRW.finalize();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop end.");
	return 0;
}

// ��ʱ��ѯ�����ڵ���.json��ʽ����־��Ϣ��������level,time�����ֶ�
int CMainTask::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{

	return PK_SUCCESS;
}

int CMainTask::StopTimers()
{
	// ��ȡ����ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�
	std::map<int, CSaveTimer *>::iterator iterTimer = m_mapInterval2TagGroupTimer.begin();
	for (; iterTimer != m_mapInterval2TagGroupTimer.end(); iterTimer++)
	{
		CSaveTimer *pTimer = iterTimer->second;
		pTimer->Stop();
	}
	return 0;
}


int CMainTask::LoadConfig()
{
	CPKEviewDbApi PKEviewDbApi;
	int nRet = PKEviewDbApi.Init();
	if (nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Init DataBase failed, ret:%d", nRet);
		PKEviewDbApi.Exit();
		return nRet;
	}

	//����sqlite���ݿⲢ�ҽ����ݽ��б���;
	m_vecTags.clear();
    vector<vector<string> > rows;
	vector<string> fields;
	vector<int > lis;
	long lQueryId = 0;
	//�˴����ִ��ʧ�ܿ�����Ҫ�ٴ��������ݿ�;
	// select name,enable_history,saveperiod from t_device_tag where enable_history=1 and saveperiod is not null and saveperiod > 0
	//nRet = PKEviewDbApi.SQLExecute("select name,enable_history,saveperiod from t_device_tag", rows);
	nRet = PKEviewDbApi.SQLExecute("select name,enable_history from t_device_tag", rows);
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "execute query tags to save history data failed");
		PKEviewDbApi.Exit();
		return nRet;
	}
	//������������tag�㱣������;
	int nHistoryTagNum = 0;
	for (int iRow = 0; iRow < rows.size(); iRow++)
	{
		vector<string> &row = rows[iRow];
		string strTagName = row[0];
		int nEnableHistory = ::atoi(row[1].c_str());
		//int nSavePeriod = ::atoi(row[2].c_str());
		int nSavePeriod = 10000;
		if (strTagName.empty())
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "tagname:%s is null", strTagName.c_str());
			continue;
		}

		if (nEnableHistory == 0)
		{
			continue;
		}

		if (nSavePeriod == 0) // ���δ����saveperiod����ôÿ10���Ӷ�ȡ�ͱ���һ�α���
			nSavePeriod = 10000;

		if (nSavePeriod < 0 || nSavePeriod > 24 * 60 * 60 * 1000)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "tagname:%s, saveperiod <= 0", strTagName.c_str());
			continue;
		}

		nHistoryTagNum++;
		
		CTagInfo *pTag = new CTagInfo();
		pTag->strTagName = strTagName;
		pTag->nSavePeriod = nSavePeriod;
		m_vecTags.push_back(pTag);
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Load Config over. tag count:%d, tag to save history data:%d ", m_vecTags.size(), nHistoryTagNum);
	PKEviewDbApi.Exit();
	return 0;
}

int CMainTask::GenerateTimersByTagInfo()
{
	//��ȡTAG�㣬�����鵽CSaveTimer;
	vector<CTagInfo *>::iterator itTag;
	for (itTag = m_vecTags.begin(); itTag != m_vecTags.end(); itTag++)
	{
		CTagInfo *pTag = *itTag;
		CSaveTimer *pTimer = NULL;
		map<int, CSaveTimer *>::iterator itTimer = m_mapInterval2TagGroupTimer.find(pTag->nSavePeriod);
		if (itTimer == m_mapInterval2TagGroupTimer.end())
		{
			pTimer = new CSaveTimer();
			pTimer->SetMainTask(this);
			pTimer->m_nCyclePeriodMS = pTag->nSavePeriod;
			m_mapInterval2TagGroupTimer[pTag->nSavePeriod] = pTimer;
		}
		else
			pTimer = itTimer->second;

		pTimer->m_vecTags.push_back(pTag);
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "GenerateTimersByTagInfo %d timers", m_mapInterval2TagGroupTimer.size());

	return 0;
}


int CMainTask::StartProcessTimers()
{
	// ��ȡ����ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�;
	std::map<int, CSaveTimer *>::iterator iterTimer = m_mapInterval2TagGroupTimer.begin();
	for (; iterTimer != m_mapInterval2TagGroupTimer.end(); iterTimer++)
	{
		CSaveTimer *pTimer = iterTimer->second;
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "generate timer, interval :%d ms, tag count:%d", pTimer->m_nCyclePeriodMS, pTimer->m_vecTags.size());
		int nRet = pTimer->Start();
	}
	return 0;
}

inline string& ltrim(string &str) {
	string::iterator p = find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace)));
	str.erase(str.begin(), p);
	return str;
}

inline string& rtrim(string &str) {
	string::reverse_iterator p = find_if(str.rbegin(), str.rend(), not1(ptr_fun<int, int>(isspace)));
	str.erase(p.base(), str.end());
	return str;
}

inline string& trim(string &str) {
	ltrim(rtrim(str));
	return str;
}
/*
hisdataserver=127.0.0.1:27017
*/
int CMainTask::ReadHisDatabaseUrl()
{
	char szMonConn[256];
	sprintf(szMonConn, "%s:%d", PK_LOCALHOST_IP, PKHISDB_LISTEN_PORT);
	m_strMongoConnString = szMonConn; // ȱʡ��IP�Ͷ˿�
	// �õ������ļ���·��
	const char *szConfigPath = PKComm::GetConfigPath();
	string strConfigPath = szConfigPath;
	strConfigPath = strConfigPath + PK_DIRECTORY_SEPARATOR_STR + "db.conf";

	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // �����ڸ��ļ�������Ϊ��
	{
		// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return 0;
	}

	// �����ļ���С  
textFile.seekg(0, std::ios::end);
std::streampos len = textFile.tellg();
textFile.seekg(0, std::ios::beg);

// ����1  
std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());

textFile.close();

string strConnStr = "";
string strUserName = "";
string strPassword = "";
string strDbType = "";
string strTimeout = "";
string strCodingSet = "";
while (strConfigData.length() > 0)
{
	string strLine = "";
	int nPosLine = strConfigData.find('\n');
	if (nPosLine > 0)
	{
		strLine = strConfigData.substr(0, nPosLine);
		strConfigData = strConfigData.substr(nPosLine + 1);
	}
	else
	{
		strLine = strConfigData;
		strConfigData = "";
	}

	strLine = trim(strLine);
	if (strLine.empty())
		continue;

	// x=y
	int nPosSection = strLine.find('=');
	if (nPosSection <= 0) // ��=�Ż�=�ſ�ͷ
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, line: %s invalid, need =!", strConfigPath.c_str(), strLine.c_str());
		continue;
	}

	string strSecName = strLine.substr(0, nPosSection);
	string strSecValue = strLine.substr(nPosSection + 1);
	strSecName = trim(strSecName);
	strSecValue = trim(strSecValue);
	std::transform(strSecName.begin(), strSecName.end(), strSecName.begin(), ::tolower);
	if (strSecName.compare("hisdb") == 0)
	{
		if (!strSecValue.empty())
			m_strMongoConnString = strSecValue;
	}
}

if (m_strMongoConnString.empty())
m_strMongoConnString = szMonConn;
int nFound = m_strMongoConnString.find(":");
if (nFound <= 0)
m_strMongoConnString = szMonConn;
return 0;
}


/**
*  ���ܣ���ʼ��MongoDb���ݿ�;
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CMainTask::InitMongdb()
{
#ifdef _WIN32
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		return -1;
	}
#endif 

	ReadHisDatabaseUrl(); // ��ȡmongo�ĵ�ַ
	try
	{
		mongo::client::initialize();
		m_mongoConn.connect(m_strMongoConnString.c_str());
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "connect to pkhisdb successfully");

		// ���������������
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "create index of temp,data,month,year,raw_YYYYMMDD,hour_YYYYMMDD,minute_YYYYHHDD");

		list<string> lCollections = m_mongoConn.getCollectionNames(MONGO_DB_NAME);
		for (list<string>::iterator itCol = lCollections.begin(); itCol != lCollections.end(); itCol++)
		{
			string strTableName = *itCol;
			if (PKStringHelper::StriCmp(strTableName.c_str(), "temp") == 0 ||
				PKStringHelper::StriCmp(strTableName.c_str(), "day") == 0 ||
				PKStringHelper::StriCmp(strTableName.c_str(), "month") == 0 ||
				PKStringHelper::StriCmp(strTableName.c_str(), "year") == 0 ||
				strTableName.find("raw_") == 0 ||
				strTableName.find("hour_") == 0 ||
				strTableName.find("minute_") == 0)
				{
					string strDBTable = MONGO_DB_NAME;
					strDBTable = strDBTable + "." + strTableName;
					EnsureTableIndex(strDBTable);
				}
		}
		//EnsureTableIndex(MONGO_DB_TEMP); // Ϊtemp����������
		//EnsureTableIndex(MONGO_DB_DAY); // Ϊtemp����������
		//EnsureTableIndex(MONGO_DB_MONTH); // Ϊtemp����������
		//EnsureTableIndex(MONGO_DB_YEAR); // Ϊtemp����������
	}
	catch (const mongo::DBException &e)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "connect to pkhisdb except:%s", e.what());
		return -2;
	} 
	return PK_SUCCESS;
}
/**
*  ���ܣ���ʼ��Redis.
*
*  @version     04/10/2017  xingxing  Initial Version.
*/
int CMainTask::InitRedis()
{
	int nRet = 0;

	nRet = m_redisRW.initialize(PK_LOCALHOST_IP, PKMEMDB_LISTEN_PORT, PKMEMDB_ACCESS_PASSWORD, 0);
	if (nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "MainTask m_redisRW.Initialize failed,ret:%d", nRet);
		return nRet;
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Init PKMemDB success");
	return nRet;
}

// ��ȡ20���ӵ�temp���ݣ����浽ÿ��tag��
int CMainTask::ReadLastUnProcessedTempData(int &nQueryCount, int &nRecordCount)
{
	int nReadedNum = 0;
	for (vector<CTagInfo *>::iterator itTag = m_vecTags.begin(); itTag != m_vecTags.end(); itTag++)
	{
		CTagInfo *pTag = *itTag;		 
		nReadedNum++;
		/*if (nReadedNum % 100 == 0){
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "ReadLastUnProcessedTempData, end to read tags from No.%d --- No.%d tags, total queryNum:%d, total recNum:%d", 
				nReadedNum - 100, nReadedNum, nQueryCount, nRecordCount);
		}*/

		// ��mongodb��ȡ��tag����tine�ֶε����ֵ�ļ�¼�������µ�һ����¼
		nQueryCount++;
		CTagRawRecord *pMaxTimeRec = QueryMaxTimeRecord_From_TempTable(pTag->strTagName);
		if (pMaxTimeRec == NULL)   // δ��ȡ��
			 continue;
		nRecordCount += 1;

		 // ����pRecNewest�е�ʱ�䣬��ǰ����20����
		 string strEndTime = pMaxTimeRec->strTime;
		 time_t tmBeginTime = PKTimeHelper::String2Time((char *)strEndTime.c_str());
		 tmBeginTime = tmBeginTime - 20 * 60; // 20 ����
		 char szBeginTime[32];
		 string strBeginTime = PKTimeHelper::Time2String(szBeginTime, sizeof(szBeginTime), tmBeginTime);

		// ��ȡ> strRecentBeginTime �� <= pRecNewestʱ������м�¼����Լ20���ӵ����ݣ�����ŵ�vecTempRec
		 vector<CTagRawRecord *> vecRecords;
		 QueryRecords_From_TempTable(pTag->strTagName, strBeginTime, strEndTime, vecRecords);
		 for (vector<CTagRawRecord *>::iterator itRec = vecRecords.begin(); itRec != vecRecords.end(); itRec++)
		 {
			 pTag->vecTempRec.push_back(*itRec);
		 }
		 nQueryCount++;
		 nRecordCount += vecRecords.size();
		 // �ڴ治��delete
		 vecRecords.clear();
	}
	return 0;
}

// {"n" : "bus1con1.206ACC_onoff","id" : 1,"v" : "418","t" : "2017-06-28 18:50:09.034"}
CTagRawRecord * CMainTask::QueryMaxTimeRecord_From_TempTable(string & strTagName)
{
	// select * from t order by t limit 1
	mongo::Query query(BSONObjBuilder().append(REC_ATTR_TAGNAME, strTagName).obj());
	query.sort(BSONObjBuilder().append(REC_ATTR_TIME, -1).obj());
	try{
		BSONObj bsonObj = m_mongoConn.findOne(MONGO_DB_TEMP, query);
		if (bsonObj.isEmpty())
			return NULL;

		CTagRawRecord *pRawRec = new CTagRawRecord();
		pRawRec->strTagName = strTagName;
		pRawRec->strTime = bsonObj.getStringField(REC_ATTR_TIME);
		pRawRec->nQuality = bsonObj.getIntField(REC_ATTR_QUALITY);
		pRawRec->dbValue = bsonObj.getIntField(REC_ATTR_VALUE);

		return pRawRec;
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return NULL;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): %s", strTagName.c_str(), e.what());
		return NULL;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKHisDB Exception(%s): NULL", strTagName.c_str());
		return NULL;
	}
	
	return NULL;
}

int CMainTask::QueryRecords_From_TempTable(string strTagName, string strBeginTime, string strEndTime, vector<CTagRawRecord *> &vecRecords)
{
	BSONObjBuilder condBuilder;
	condBuilder.append("$gte", strBeginTime).append("$lt", strEndTime);
	BSONObjBuilder objBuilder;
	objBuilder.append(REC_ATTR_TAGNAME, strTagName).append(REC_ATTR_TIME, condBuilder.obj()); // {"time":{"$gt":"2017-06-28 10:00:01","$lt":"2017-06-30 12:00:01"}}
	mongo::Query query(objBuilder.obj());
	query.sort(BSONObjBuilder().append(REC_ATTR_TIME, 1).obj());
	try{
		std::auto_ptr<DBClientCursor> cursor = m_mongoConn.query(MONGO_DB_TEMP, query);
		while (cursor->more())
		{
			mongo::BSONObj bsonObj = cursor->next();
			CTagRawRecord *pRawRec = new CTagRawRecord();
			pRawRec->strTagName = strTagName;
			pRawRec->strTime = bsonObj.getStringField(REC_ATTR_TIME);
			pRawRec->nQuality = bsonObj.getIntField(REC_ATTR_QUALITY);
			pRawRec->dbValue = bsonObj.getIntField(REC_ATTR_VALUE);
			vecRecords.push_back(pRawRec);
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



// �Ƿ��mongo�ı��Ѿ�����������
void CMainTask::EnsureTableIndex(string strTableName)
{
	map<string, bool>::iterator itTable = m_mapTableName2Index.find(strTableName);
	if (itTable == m_mapTableName2Index.end()){
		int nRet = CreateTableIndex(strTableName);
		//if (nRet == 0)
		m_mapTableName2Index[strTableName] = true;
	}
}

int CMainTask::CreateTableIndex(string &strTableName)
{
	try{
		mongo::IndexSpec oIds;
		mongo::IndexSpec::KeyVector keyVec;
		std::pair<std::string, mongo::IndexSpec::IndexType> index1_1("n", mongo::IndexSpec::kIndexTypeAscending);
		std::pair<std::string, mongo::IndexSpec::IndexType> index1_2("t", mongo::IndexSpec::kIndexTypeDescending);
		keyVec.push_back(index1_1);
		keyVec.push_back(index1_2);
		oIds.addKeys(keyVec);
		oIds.unique(false);
		//oIds.addKey("v");
		m_mongoConn.createIndex(strTableName, oIds);
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "create index({'n':1,'t':-1}) of table %s success!", strTableName.c_str());
	}
	catch (DBException& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): %s", strTableName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): %s", strTableName.c_str(), e.what());
		return -2;
	}
	catch (...){
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): NULL", strTableName.c_str());
		return -3;
	}
	return 0;
}
