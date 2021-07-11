/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  ȷ�Ϻ�ɾ������Ҳ���ڿ����������ִ�С�����Ҳ�Ƕ������ӣ�����ת�������ݴ��������
**************************************************************/
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h""
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "NodeCommonDef.h"
#include "UserAuthTask.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "MainTask.h"

extern CPKLog g_logger;
#define	DEFAULT_SLEEP_TIME		500	


/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CUserAuthTask::CUserAuthTask()
{
	m_bStop = false;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CUserAuthTask::~CUserAuthTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CUserAuthTask::svc()
{	
	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	time_t tmLastLoadDB = 0; // ��һ�μ������ݿ���Ȩ����Ϣ��ʱ��
	while(!m_bStop)
	{
		time_t tmNow;
		time(&tmNow);
		unsigned int nTimeSpan = labs(tmNow - tmLastLoadDB);
		if (nTimeSpan < 3 * 60) // ÿ3�������¼���һ��Ȩ����Ϣ
		{
			ACE_Time_Value tvSleep;
			tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
			ACE_OS::sleep(tvSleep);
			continue;
		}
		tmLastLoadDB = tmNow;
		LoadAuthFromDB();
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"nodeserver UserAuthTask exit normally!");
	OnStop();

	return PK_SUCCESS;	
}


int CUserAuthTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CUserAuthTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}


int CUserAuthTask::OnStart()
{
	return 0;
}

// ������ֹͣʱ
void CUserAuthTask::OnStop()
{
}

int CUserAuthTask::LoadAuthFromDB()
{
	CPKEviewDbApi PKEviewDbApi;
	int nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "UserAuthTask, ��ʼ�����ݿ�ʧ��!");
		PKEviewDbApi.Exit();
		return -1;
	}

	// �������ϵͳʱ������ϵͳ��Ȩ����֤
	// ����tag�����ϵͳ���ԣ��õ�����Ȩ��
	vector<vector<string> > vecRows;

	// ��ѯLoginName,RoleId&RoleName
	int nLabelNum = 0;
	// ��ѯroleID
	char szSql[2048] = { 0 };
	sprintf(szSql, "SELECT account.loginName,role.id,role.name	FROM t_base_account account,t_base_role role where account.isValid = 1 and account.roleId=role.id and role.isValid=1");
	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯȨ��ʱ,�����û���Ⱥ���ϵʧ��,SQL:%s, ret:%d, error:%s", szSql, nRet, strError.c_str());
		PKEviewDbApi.Exit();
		return false;
	}

	if (vecRows.size() <= 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯȨ��ʱ,�����û���Ⱥ���ϵ, �õ���¼����Ϊ0, ������Ч�û�����ЧȺ���ǲ���δ����! %s", szSql);
		return false;
	}

	map<string, string> mapLoginName2RoleName;
	for (unsigned int i = 0; i < vecRows.size(); i++)
	{
		vector<string> &row = vecRows[i];
		string strLoginName = row[0];
		string strRoleId = row[1];
		string strRoleName = row[2];
		mapLoginName2RoleName[strLoginName] = strRoleName;
		row.clear();
	}
	vecRows.clear();

	map<string, map<string, bool> > mapRoleName2SysMap;

	// ��������Ⱥ��---->��Դ����ϵͳ��
	sprintf(szSql, "SELECT	rr.role_id as role_id,role.name as role_name,ss.name as sys_name FROM	t_base_role_resources rr, t_subsys_list ss,t_base_role role \
				   		WHERE rr.resources_id = ss.id and rr.role_id = role.id and rr.type = 4");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯȨ��ʱ,����Ⱥ�����Դ��ϵʧ��,SQL:%s, ret:%d, error:%s", szSql, nRet, strError.c_str());
		//mapLoginName2RoleName.clear();
	}
	else
	{
		for (unsigned int i = 0; i < vecRows.size(); i++)
		{
			vector<string> &row = vecRows[i];
			string strRoleId = row[0];
			string strRoleName = row[1];
			string strSysName = row[2];
			map<string, map<string, bool> >::iterator itMap = mapRoleName2SysMap.find(strRoleName);
			if (itMap == mapRoleName2SysMap.end())
			{
				map<string, bool> mapSysName;
				mapSysName[strSysName] = true;
				mapRoleName2SysMap[strRoleName] = mapSysName;
			}
			else
			{
				map<string, bool> &mapSysName = itMap->second;
				mapSysName[strSysName] = true;
			}
			row.clear();
		}
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "������%d����Ч�û���Ϣ, %d��Ⱥ���Ȩ�޼�¼, ������:%d��Ⱥ��", mapLoginName2RoleName.size(), vecRows.size(), mapRoleName2SysMap.size());
		vecRows.clear();
	}

	PKEviewDbApi.Exit();

	m_mutex.acquire();
	m_mapLoginName2RoleName.swap(mapLoginName2RoleName);
	m_mapRoleName2SysNameMap.swap(mapRoleName2SysMap);
	m_mutex.release();

	mapLoginName2RoleName.clear();
    for (map<string, map<string, bool> >::iterator itMap = mapRoleName2SysMap.begin(); itMap != mapRoleName2SysMap.end(); itMap++)
	{
		itMap->second.clear();
	}
	mapRoleName2SysMap.clear();
	return 0;
}

// ����ģʽ���ж��û���¼�����ڵ�Ⱥ�飬���������������Ƿ����Ȩ��
// �豸ģʽ���ж��û���¼�����ڵ�Ⱥ�飬�����tag��ϵͳ�����Ƿ����Ȩ��
bool CUserAuthTask::HasAuthByTagAndLoginName(string &strLoginName, CDataTag *pTag)
{
	if (strLoginName.compare("_super_user_") == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, ͨ�� TAGMonitor����������! ֱ����Ϊ��Ȩ��!", strLoginName.c_str(), pTag->strName.c_str());
		return true;
	}

	string strSubssyName = "";
	if (pTag->pObject)
		strSubssyName = pTag->pObject->strSubsysName;//if (m_bObjMode)
	else
		strSubssyName = pTag->strSubsysName;

	if(strSubssyName.empty()) // ����ģʽ����ϵͳ�������Ϊ�գ���ȡ�豸ģʽ��
		strSubssyName = pTag->strSubsysName;

	if (strSubssyName.length() == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, subsysname is empty, ��Ϊ��Ȩ��!", strLoginName.c_str(), pTag->strName.c_str());
		return true;
	}

	// û�е�½�û�����Ϊû��Ȩ��
	if (strLoginName.empty())
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, subsysname:%s, ��¼��Ϊ��, ��Ȩ��!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str());
		return false;
	}

	bool bHaveAuth = false;
	m_mutex.acquire();
	map<string, string>::iterator itLogin = m_mapLoginName2RoleName.find(strLoginName);
	if (itLogin == m_mapLoginName2RoleName.end())
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "HasAuth, loginName:%s, tag:%s, subsysname:%s, ������Ч�û���, ��Ȩ��!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str());
		m_mutex.release();
		return false;
	}
	string strRoleName = itLogin->second;

    map<string, map<string, bool> >::iterator itRole = m_mapRoleName2SysNameMap.find(strRoleName);
	if (itRole == m_mapRoleName2SysNameMap.end())
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "HasAuth, loginName:%s, tag:%s, subsysname:%s, rolename:%s, ������ЧȺ����, ��Ȩ��!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str(), strRoleName.c_str());
		m_mutex.release();
		return false;
	}

	map<string, bool> &mapSysName = itRole->second;
	if (mapSysName.find(strSubssyName) == mapSysName.end())
	{
		m_mutex.release();
		return false;
	}

	// �ҵ�����ϵͳ���ƣ���Ϊ��Ȩ��
	m_mutex.release();
	return true;
}

