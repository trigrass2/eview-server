#include "windows.h"
#include "MyDBAPI.h"
#include "pklog/pklog.h"
#include "common/eviewdefine.h"
#include "pkcomm/pkcomm.h"
#include "vector"
#include "string"

using namespace std;

#define LEN_SQL_EXEC	512
//select TERMINAL,PROGRAM,LOGON_TIME,USERNAME,SCHEMANAME,MACHINE,PORT,MODULE from v$session order by program
enum VECLINE
{
	TERMINAL, PROGRAM, LOGON_TIME, USERNAME, SCHEMANAME, MACHINE, MODULE
};
//�жϽ��� ƽ̨�����̣��û������Ӹ���
typedef struct _PROCESS
{
	int nCounts;
	string strTerminal;
	string strLoginTime;
	string strUserName;
	string strSchemaName;
	string strMachine;
	string strPort;
	string strModule;
	
}PROCESS;

int BuildCheckSql(int nDatabaseType, char* szSql)
{
	switch (nDatabaseType)
	{
	case SA_Oracle_Client:
		sprintf(szSql, "select TERMINAL, PROGRAM, LOGON_TIME, USERNAME, SCHEMANAME, MACHINE, MODULE from v$session order by program ");
		return 0;
	default:
		return -1;
	}
}


//select TERMINAL,PROGRAM,LOGON_TIME,USERNAME,SCHEMANAME,MACHINE,PORT,MODULE from v$session order by program
//PROGRAM�ֶ� ��.exe
void AdjustProc(vector<vector<string>> vecRows, map<string, PROCESS> &mapProcInDB)
{
	mapProcInDB.clear();
	vector<vector<string>>::iterator itRow = vecRows.begin();
	for (; itRow != vecRows.end(); itRow++)
	{
		if ((*itRow).size() == 7)
		{
			string strProName = (*itRow)[PROGRAM];
			//()����ʾ
			strProName = strProName.substr(0, strProName.find_first_of("("));
			//�����Ƿ��Ѿ�����Ľ��̣����ޣ������ý��̣����У�������+ 1
			map<string, PROCESS>::iterator it = mapProcInDB.find(strProName);
			if (it == mapProcInDB.end())
			{
				string strTerminal = (*itRow)[TERMINAL];
				string strLoginTime = (*itRow)[LOGON_TIME];
				string strUserName = (*itRow)[USERNAME];
				string strSchemaName = (*itRow)[SCHEMANAME];
				string strMachine = (*itRow)[MACHINE];
				string strPort = " ";
				string strModule = (*itRow)[MODULE];
				PROCESS Process = {1, strTerminal, strLoginTime, strUserName, strSchemaName, strMachine, strPort, strModule };
				mapProcInDB[strProName] = Process;
			}
			else
			{
				mapProcInDB[strProName].nCounts++;
			}
		}
		else
		{ 
			//������������
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ѯ���ص���������ȷ");
			return;
		}
	}
}

void PrintLog(map<string, PROCESS> mapProcInDB, int nTimeSpan)
{
	if (mapProcInDB.size() <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Ҫ��ӡ�Ľ�����ϢΪ0");
		return;
	}
	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "������Ϣ����������Ϊ:%d", mapProcInDB.size());
	map<string, PROCESS>::iterator itPro = mapProcInDB.begin();
	for (; itPro != mapProcInDB.end(); itPro++)
	{
		if ((*itPro).second.nCounts < nTimeSpan)
		{
			continue;
		}
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "������:%s ��������%d TERMINAL��%s LOGON_TIME:%s USERNAME:%s SCHEMANAME:%s MACHINE:%s PORT:%s MODULE:%s", \
			itPro->first, (*itPro).second.nCounts, (*itPro).second.strTerminal.c_str(), (*itPro).second.strLoginTime.c_str(), (*itPro).second.strUserName.c_str(), (*itPro).second.strSchemaName.c_str(), \
			(*itPro).second.strMachine.c_str(), (*itPro).second.strPort.c_str(), (*itPro).second.strModule.c_str());
	}
}

CPKLog PKLog;
int main(_In_ int _Argc, _In_reads_(_Argc) _Pre_z_ char ** _Argv, _In_z_ char ** _Env)
{
	PKLog.SetLogFileName("CheckConn");
	CMyDbApi DBConf("connCheck.conf");
	DBConf.Init();
	char szSQL[LEN_SQL_EXEC] = { 0 };
	int nRet = BuildCheckSql(DBConf.GetDBType(), szSQL);
	if (nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����ִ�����ʧ��:���ݿ�����:%s %d", DBConf.m_strDatabaseType.c_str(), DBConf.GetDBType());
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "����ִ�����ɹ�:���ݿ�����:%s %d", DBConf.m_strDatabaseType.c_str(), DBConf.GetDBType());
	}
	while (true)
	{
		map<string, PROCESS> mapProcInDB;
		//���ж�����
		if (DBConf.CheckConnection() != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "�������ݿ�ʧ��:%s", DBConf.m_strDatabaseType.c_str());
			Sleep(1000);
			continue;
		}
		//���ӳɹ���ִ������ѯ�������
		vector<vector<string>> vecRows;
		int nRet = DBConf.SQLExecute(szSQL, vecRows);
		if (nRet == 0)
		{
			//�жϽ��̲���ӡ
			AdjustProc(vecRows, mapProcInDB);
			PrintLog(mapProcInDB, DBConf.m_nMinLogConn);
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "\n=======�������ķֽ���========\n");
			Sleep(DBConf.m_nTimeSpan);
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ִ�в�ѯ���ʧ��:%s", szSQL);
			DBConf.SQLDisconnect();
			DBConf.UnInitialize();
			DBConf.m_bIsConnected = false;
			Sleep(3000);
			continue;
		}
	}
}