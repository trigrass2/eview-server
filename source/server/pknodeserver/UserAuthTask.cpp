/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  确认和删除报警也放在控制里面进行执行。这里也是二道贩子，将其转发给数据处理就完事
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
 *  构造函数.
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
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CUserAuthTask::~CUserAuthTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
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

	time_t tmLastLoadDB = 0; // 上一次加载数据库中权限信息的时间
	while(!m_bStop)
	{
		time_t tmNow;
		time(&tmNow);
		unsigned int nTimeSpan = labs(tmNow - tmLastLoadDB);
		if (nTimeSpan < 3 * 60) // 每3分钟重新加载一次权限信息
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

// 本任务停止时
void CUserAuthTask::OnStop()
{
}

int CUserAuthTask::LoadAuthFromDB()
{
	CPKEviewDbApi PKEviewDbApi;
	int nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "UserAuthTask, 初始化数据库失败!");
		PKEviewDbApi.Exit();
		return -1;
	}

	// 如果有子系统时进行子系统的权限验证
	// 根据tag点的子系统属性，得到有无权限
	vector<vector<string> > vecRows;

	// 查询LoginName,RoleId&RoleName
	int nLabelNum = 0;
	// 查询roleID
	char szSql[2048] = { 0 };
	sprintf(szSql, "SELECT account.loginName,role.id,role.name	FROM t_base_account account,t_base_role role where account.isValid = 1 and account.roleId=role.id and role.isValid=1");
	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询权限时,查找用户和群组关系失败,SQL:%s, ret:%d, error:%s", szSql, nRet, strError.c_str());
		PKEviewDbApi.Exit();
		return false;
	}

	if (vecRows.size() <= 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询权限时,查找用户和群组关系, 得到记录个数为0, 请检查有效用户和有效群组是不是未配置! %s", szSql);
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

	// 继续查找群组---->资源（子系统）
	sprintf(szSql, "SELECT	rr.role_id as role_id,role.name as role_name,ss.name as sys_name FROM	t_base_role_resources rr, t_subsys_list ss,t_base_role role \
				   		WHERE rr.resources_id = ss.id and rr.role_id = role.id and rr.type = 4");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询权限时,查找群组和资源关系失败,SQL:%s, ret:%d, error:%s", szSql, nRet, strError.c_str());
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
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "加载了%d条有效用户信息, %d条群组和权限记录, 分属于:%d条群组", mapLoginName2RoleName.size(), vecRows.size(), mapRoleName2SysMap.size());
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

// 对象模式，判断用户登录名所在的群组，对这个对象的名称是否具有权限
// 设备模式，判断用户登录名所在的群组，对这个tag子系统名称是否具有权限
bool CUserAuthTask::HasAuthByTagAndLoginName(string &strLoginName, CDataTag *pTag)
{
	if (strLoginName.compare("_super_user_") == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, 通过 TAGMonitor发来的命令! 直接认为有权限!", strLoginName.c_str(), pTag->strName.c_str());
		return true;
	}

	string strSubssyName = "";
	if (pTag->pObject)
		strSubssyName = pTag->pObject->strSubsysName;//if (m_bObjMode)
	else
		strSubssyName = pTag->strSubsysName;

	if(strSubssyName.empty()) // 对象模式下子系统名称如果为空，则取设备模式下
		strSubssyName = pTag->strSubsysName;

	if (strSubssyName.length() == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, subsysname is empty, 认为有权限!", strLoginName.c_str(), pTag->strName.c_str());
		return true;
	}

	// 没有登陆用户名认为没有权限
	if (strLoginName.empty())
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "HasAuth, loginName:%s, tag:%s, subsysname:%s, 登录名为空, 无权限!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str());
		return false;
	}

	bool bHaveAuth = false;
	m_mutex.acquire();
	map<string, string>::iterator itLogin = m_mapLoginName2RoleName.find(strLoginName);
	if (itLogin == m_mapLoginName2RoleName.end())
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "HasAuth, loginName:%s, tag:%s, subsysname:%s, 不是有效用户名, 无权限!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str());
		m_mutex.release();
		return false;
	}
	string strRoleName = itLogin->second;

    map<string, map<string, bool> >::iterator itRole = m_mapRoleName2SysNameMap.find(strRoleName);
	if (itRole == m_mapRoleName2SysNameMap.end())
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "HasAuth, loginName:%s, tag:%s, subsysname:%s, rolename:%s, 不是有效群组名, 无权限!", strLoginName.c_str(), pTag->strName.c_str(), strSubssyName.c_str(), strRoleName.c_str());
		m_mutex.release();
		return false;
	}

	map<string, bool> &mapSysName = itRole->second;
	if (mapSysName.find(strSubssyName) == mapSysName.end())
	{
		m_mutex.release();
		return false;
	}

	// 找到了子系统名称，认为有权限
	m_mutex.release();
	return true;
}

