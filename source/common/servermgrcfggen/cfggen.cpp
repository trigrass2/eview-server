/************************************************************************
 *	Filename:		CVDRIVERCOMMON.CPP
 *	Copyright:		Shanghai Peak InfoTech Co., Ltd.
 *
 *	Description:	$(Desp) .
 *
 *	@author:		shijunpu
 *	@version		??????	shijunpu	Initial Version
 *  @version	9/17/2012  shijunpu  ��������Ԫ�ش�С�ӿ� .
*************************************************************************/
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "common/eviewcomm_internal.h"
#include "pkservermgr/pkservermgr_plugin.h"

#define BITS_PER_BYTE 8

#ifdef WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze 
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher;
#endif//#ifndef NO_EXCEPTION_ATTACHER
#endif

#include <sstream>
#include <string>
#include <fstream>  
#include <streambuf>  
#include <iostream>
#include "pkeviewdbapi/pkeviewdbapi.h"
#include <stdlib.h>
using namespace std;

#ifdef _WIN32
#include "windows.h"
#define  SERMGRCFG_GEN_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  SERMGRCFG_GEN_EXPORTS extern "C" __attribute__ ((visibility ("default")))
#endif //_WIN32

#define NULLASSTRING(x) x==NULL?"":x

int ReadDriversInfoFromDB(CPKEviewDbApi &PKEviewDbApi, vector<CPKServerMgrProcessInfo> &vecServer);
int ReadServersInfoFromDB(CPKEviewDbApi &PKEviewDbApi, vector<CPKServerMgrProcessInfo> &vecServer);

CPKLog g_logger;

// �����IP��ָҪ���ӵ��ڴ����ݿ�IP��szUserName����ʼ��ģ���¼���û����ơ�szPassword����ʼ��ģ���¼���û����룬�����ڴ����ݿ������
SERMGRCFG_GEN_EXPORTS int BeginFuncBeforeProcLoad(vector<CPKServerMgrProcessInfo> &vecProcConfig)
{
	string strRuntimePath = PKComm::GetRunTimeDataPath();

	// ��ɾ�����е�����ʱ�����ڴ�Ŀ¼
	if (PKFileHelper::IsDirectoryExist(strRuntimePath.c_str()))
		PKFileHelper::DeleteDir(strRuntimePath.c_str());

	CPKEviewDbApi PKEviewDbApi;
 	char szSql[2048] = { 0 };
	// ��ȡ���е��������б�
	vector<vector<string> > vecDriver;
	int nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ʼ�����ݿ�ʧ��!");
		PKEviewDbApi.Exit();
		return -1;
	}

	// �Զ���ȡ����������������
	ReadServersInfoFromDB(PKEviewDbApi, vecProcConfig);

	// �������е���������
 	ReadDriversInfoFromDB(PKEviewDbApi, vecProcConfig); 

	PKEviewDbApi.Exit(); 
	return 0;
}

SERMGRCFG_GEN_EXPORTS int EndFunc()// ��������ʱ��Ҫ����
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "EndFunc: Generate eview pkservermgr xml configfile!");

	return 0;
}

// �Ƿ���������ʷ���ݿ���񣬲���t_device_tag��t_object_list��ϵͳ������t_sys_param����̫��ȷ����˵������
bool IsConfigHisData(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;

	string strError;
	// �鿴�豸ģʽ��t_device_tag��ʾ�Ƿ������˱�����ʷ����̫��ȷ������˵������
	sprintf(szSql, "select count(1) from t_device_tag \
		where name is not null and name <>''\
		and hisdata_interval is not null and hisdata_interval <>'' and hisdata_interval <> 0\
		and trenddata_interval is not null and trenddata_interval <>'' and trenddata_interval <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsConfigHisData, query hisdata or trend tagnus from t_device_tag to failed, sql:%s, error:%s, continue check...", szSql, strError.c_str());
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}
	else
	{ 
		int nCount = ::atoi(vecRow[0][0].c_str());
		vecRow[0].clear();
		vecRow.clear();
		if (nCount > 0) // ����������豸ģʽ�µ���ʷ���򷵻�true
		{
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsConfigHisData, query t_device_tag, get:%d>0 tag to save history, return true", nCount);
			return true;
		}
	}

	int nVersion = PKEviewComm::GetVersion(PKEviewDbApi, "pkhisdataserver");
	if (nVersion > 0) // �汾�ϵͣ�δ��������������
	{
		// �鿴����ģʽ��t_object_list��ʾ�Ƿ������˱�����ʷ��
		sprintf(szSql, "select count(1) from t_object_list \
			where name is not null and name <>''\
			and hisdata_interval is not null and hisdata_interval <>'' and hisdata_interval <> 0\
			and trenddata_interval is not null and trenddata_interval <>'' and trenddata_interval <> 0");
		nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
		if (nRet != 0 || vecRow.size() <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsConfigHisData, query hisdata or trend tagnus from t_device_tag to failed, sql:%s, error:%s, continue check...", szSql, strError.c_str());
			// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
		}
		else
		{
			int nCount = ::atoi(vecRow[0][0].c_str());
			vecRow[0].clear();
			vecRow.clear();
			if (nCount > 0) // ����������豸ģʽ�µ���ʷ���򷵻�true
			{
				g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsConfigHisData, query t_object_list, get:%d object>0 to save history, return true", nCount);
				return true;
			}
		}
	}

	string strTrendIntervalSec, strTrendSaveDay, strRecordIntervalMinute, strRecordSaveDay;
	PKEviewComm::LoadSysParam(PKEviewDbApi, "trenddata_interval_second", "0", strTrendIntervalSec);
	PKEviewComm::LoadSysParam(PKEviewDbApi, "hisdata_interval_minute", "0", strRecordIntervalMinute);
	int nTrendIntervalSec = ::atoi(strTrendIntervalSec.c_str());
	int nRecordIntervalSec = ::atoi(strRecordIntervalMinute.c_str()) * 60; // ��Ϊ��λ
 
	if (nTrendIntervalSec <= 0) // ���Ƽ���������1����
		nTrendIntervalSec = 0;

	if (nRecordIntervalSec > 60) // ���Ʊ���С��1����
		nRecordIntervalSec = 60;

	if (nTrendIntervalSec > 0 || nRecordIntervalSec > 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsConfigHisData, query t_sys_param, default to need to save history, return true");
		return true;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsConfigHisData, query t_device_tag/t_object_list/t_sys_param, all do not config trenddata_interval and hisdata_interval, return FALSE");

	return false;
}

// �Ƿ���ͨѶת������
bool IsHavePKCommunicateForward(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	// �鿴�豸ģʽ��t_device_tag��ʾ�Ƿ������˱�����ʷ����̫��ȷ������˵������
	sprintf(szSql, "select count(1) from t_commforward_connpair where enable is null or enable <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsHavePKCommunicateForward, query node count from t_commforward_connpair to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ����������豸ģʽ�µ���ʷ���򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHavePKCommunicateForward, query t_commforward_connpair, get:%d>0 connectionpair, return TRUE", nCount);
		return true;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHavePKCommunicateForward, query t_commforward_connpair, get:%d>0 connectionpair, return FALSE", nCount);
	return false;
}

bool IsHaveLowerNode(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow; 
	string strError;
	// �鿴�豸ģʽ��t_device_tag��ʾ�Ƿ������˱�����ʷ����̫��ȷ������˵������
	sprintf(szSql, "select count(1) from t_node_lower_list where enable is null or enable <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsHaveLowerNode, query node count from t_node_lower_list to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}
	 
	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ����������豸ģʽ�µ���ʷ���򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveLowerNode, query t_node_lower_list, get:%d>0 lower node to sync data to LOCAL NODE, return TRUE", nCount);
		return true;
	} 
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveLowerNode, query t_node_lower_list, get:%d>0 lower node to sync data to LOCAL NODE, return FALSE", nCount);
	return false;
}

bool IsHaveUpperNode(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	// �鿴�豸ģʽ��t_device_tag��ʾ�Ƿ������˱�����ʷ����̫��ȷ������˵������
	sprintf(szSql, "select count(1) from t_node_upper_list where enable is null or enable <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsHaveUpperNode, query uppernode count from t_node_upper_list to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ����������豸ģʽ�µ���ʷ���򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveUpperNode, query t_node_upper_list, get:%d>0 upper node to sync data to UPPER NODE, return TRUE", nCount);
		return true;
	}
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveUpperNode, query t_node_upper_list, get:%d>0 upper node to sync data to UPPER NODE, return FALSE", nCount);
	return false; 
}

bool IsHaveLinkAction(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
 	sprintf(szSql, "select count(1) from t_link_action where enable is null or enable <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsHaveLinkAction, query linkaction count from t_link_action to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveLinkAction, query t_link_action, get:%d>0 linkaction, return TRUE", nCount);
		return true;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveLinkAction, query t_link_action, get:%d>0 linkaction, return FALSE", nCount);
	return false;
}

bool IsHaveVideoConfiged(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	sprintf(szSql, "select count(1) from t_video_camera CAM, t_video_device DEV,t_video_product PROD \
		where(CAM.realvideo_deviceid = DEV.id OR CAM.hisvideo_deviceid = DEV.id) and DEV.product_id = PROD.id");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsHaveVideoConfiged, query camera count from t_video_camera to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveVideoConfiged, query t_video_camera, get:%d>0 cameras, return TRUE", nCount);
		return true;
	}
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsHaveVideoConfiged, query t_video_camera, get:%d>0 cameras, return FALSE", nCount);
	return false;
}

bool IsDBTransferConfiged(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	sprintf(szSql, "select count(1) from t_db_connection CONN, t_db_transfer_rule RULE where(CONN.id = RULE.dbconn_id)");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsDBTransferConfiged, query transfer rule count from t_db_transfer_rule to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsDBTransferConfiged, query t_db_transfer_rule, get:%d>0 transfer rule, return TRUE", nCount);
		return true;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsDBTransferConfiged, query t_db_transfer_rule, get:%d<=0 transfer rule, return FALSE", nCount);
	return false;
}

// �Ƿ�������MQTTת��
bool IsMqttForwardConfiged(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	sprintf(szSql, "select count(1) from t_forward_list where (enable is NULL or enable<>0 or enable='') and protocol='mqtt'");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsMqttForwardConfiged, query t_forward_list failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsMqttForwardConfiged, query t_forward_list, get:%d>0 forward config, return TRUE", nCount);
		return true;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsMqttForwardConfiged, query t_forward_list, get:%d<=0 forward config, return FALSE", nCount);
	return false;
}

bool IsModbusServerConfiged(CPKEviewDbApi &PKEviewDbApi)
{
	// �豸ģʽ�µı���
	int nRet = 0;
	char szSql[2408] = { 0 };
	vector<vector<string> > vecRow;
	string strError;
	sprintf(szSql, "select count(1) from t_modserver_info where enable is null or enable <> 0");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsModbusServerConfiged, query modserver info from t_modserver_info to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	int nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount <= 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsModbusServerConfiged, query t_modserver_info, get:%d>0 modbus server info, return FALSE", nCount);
		return false;
	}

	sprintf(szSql, "select count(1) from t_modserver_tag");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0 || vecRow.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "IsModbusServerConfiged, query mod tagcount info from t_modserver_tag to failed, sql:%s, error:%s, return FALSE", szSql, strError.c_str());
		return false;
		// ���ʧ������ΪӦ������������Ϊ�˺��ϰ汾����
	}

	nCount = ::atoi(vecRow[0][0].c_str());
	vecRow[0].clear();
	vecRow.clear();
	if (nCount > 0) // ��������� �򷵻�true
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsModbusServerConfiged, query t_modserver_tag, get:%d>0 modbusserver tagcount info, return TRUE", nCount);
		return true;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "IsModbusServerConfiged, query t_modserver_tag, get:%d>0 modbusserver tagcount info, return FALSE", nCount);
	return false;
}

bool AddToServerArray(vector<CPKServerMgrProcessInfo> &vecServer, CPKServerMgrProcessInfo &curServer)
{
	bool bFound = false;
	for (int i = 0; i < vecServer.size(); i++)
	{
		CPKServerMgrProcessInfo &proc = vecServer[i];
		if (proc.m_strExeName.compare(curServer.m_strExeName) == 0 && proc.m_strStartCmd.compare(curServer.m_strStartCmd) == 0)
		{
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "AddToServerArray, server dumplicated! ExeName:%s, StartCmd:%s, DisplayName:%s", curServer.m_strExeName.c_str(),
			curServer.m_strStartCmd.c_str(), curServer.m_strDisplayName.c_str());
		return false;
	}

	vecServer.push_back(curServer);
	return true;
}

// ����eview��ȡ��������.��������ѡ��pkmemdb��pknodeserver��web��ѡ��pkhisdb��pkupppernodeserver��pksync2uppernodeserver��pkvideoserver��pkhisdataserver��pkdbtransfer��pkmodserver
int ReadServersInfoFromDB(CPKEviewDbApi &PKEviewDbApi, vector<CPKServerMgrProcessInfo> &vecServer)
{
	// ��һ�����ڴ����ݿ����
	CPKServerMgrProcessInfo procMemDB;
	procMemDB.m_bAutoGenerate = true;
	procMemDB.m_nSortIndex = 0;  // �ȽϿ�ǰ
	procMemDB.m_strExeName = "pkmemdb";
#ifdef _WIN32
	procMemDB.m_strDisplayName = "�ڴ����ݿ����";
	procMemDB.m_strStartCmd = "start_pkmemdb.bat";
#else
	procMemDB.m_strDisplayName = "peak memdb server";
	procMemDB.m_strStartCmd = "start_pkmemdb.sh";
#endif
	AddToServerArray(vecServer, procMemDB);

	// ��2�������ؽڵ㴦����� 
	CPKServerMgrProcessInfo procNodeServer;
	procNodeServer.m_bAutoGenerate = true;
	procNodeServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
	procNodeServer.m_strExeName = "pknodeserver";
	procNodeServer.m_nDelaySec = 2; // ��Ҫ�ӳ�2�룬�����ʱpkmemdb��û����������
#ifdef _WIN32
	procNodeServer.m_strDisplayName = "���ؽڵ㴦�����";
#else
	procNodeServer.m_strDisplayName = "Local Node Server";
#endif
	AddToServerArray(vecServer, procNodeServer);

	bool bHaveHisData = IsConfigHisData(PKEviewDbApi); // �Ƿ�����ʷ���ݷ���
	// ��3�������ؽڵ㴦�����
	if (bHaveHisData)
	{
		CPKServerMgrProcessInfo procHisDB;
		procHisDB.m_bAutoGenerate = true;
		procHisDB.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procHisDB.m_strExeName = "pkhisdb";
#ifdef _WIN32
		procHisDB.m_strDisplayName = "��ʷ���ݿ����";
		procHisDB.m_strStartCmd = "start_pkhisdb.bat";
#else
		procHisDB.m_strDisplayName = "peak hisdatabase server";
		procHisDB.m_strStartCmd = "start_pkhisdb.sh";
#endif
		AddToServerArray(vecServer, procHisDB);
	}

	bool bHaveMqttForward = IsMqttForwardConfiged(PKEviewDbApi);// �Ƿ�������MQTTת��
	// ��4�����Ƿ�Ҫ�����ϼ��ڵ㴦�����������¼��ڵ�����Ҫ����
	bool bHaveLowerNode = IsHaveLowerNode(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bHaveLowerNode || bHaveMqttForward)
	{
		CPKServerMgrProcessInfo procMQTTServer;
		procMQTTServer.m_bAutoGenerate = true;
		procMQTTServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procMQTTServer.m_strExeName = "pkmqtt";
#ifdef _WIN32
 		procMQTTServer.m_strDisplayName = "MQTT����";
		procMQTTServer.m_strStartCmd = "start_pkmqtt.bat";
#else
 		procMQTTServer.m_strDisplayName = "MQTT Server";
		procMQTTServer.m_strStartCmd = "start_pkmqtt.sh";
#endif
		AddToServerArray(vecServer, procMQTTServer);
 	}

	if (bHaveLowerNode)
	{
		CPKServerMgrProcessInfo procUpperNodeServer;
		procUpperNodeServer.m_bAutoGenerate = true;
		procUpperNodeServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procUpperNodeServer.m_strExeName = "pkuppernodeserver";
 #ifdef _WIN32
		procUpperNodeServer.m_strDisplayName = "�ϼ��ڵ����";
#else
		procUpperNodeServer.m_strDisplayName = "Upper Node Server";
#endif
		AddToServerArray(vecServer, procUpperNodeServer);
	}

	if (bHaveMqttForward)
	{
		CPKServerMgrProcessInfo procMqttForwardServer;
		procMqttForwardServer.m_bAutoGenerate = true;
		procMqttForwardServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procMqttForwardServer.m_strExeName = "pkmqttforwardserver";
#ifdef _WIN32
		procMqttForwardServer.m_strDisplayName = "MQTTת������";
#else
		procMqttForwardServer.m_strDisplayName = "MQTT Forward Server";
#endif
		AddToServerArray(vecServer, procMqttForwardServer);
	}

	// ��4�����Ƿ�Ҫ����ͬ�����ϼ��ڵ㴦�����������ϼ��ڵ�����Ҫ����
	bool bHaveUpperNode = IsHaveUpperNode(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bHaveUpperNode)
	{
		CPKServerMgrProcessInfo procSync2UpperNodeServer;
		procSync2UpperNodeServer.m_bAutoGenerate = true;
		procSync2UpperNodeServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procSync2UpperNodeServer.m_strExeName = "pksync2uppernodeserver";
#ifdef _WIN32
		procSync2UpperNodeServer.m_strDisplayName = "ͬ�����ϼ��ڵ����";
#else
		procSync2UpperNodeServer.m_strDisplayName = "Sync To Upper Node Server";
#endif
		AddToServerArray(vecServer, procSync2UpperNodeServer);
	}

	// ��5�����Ƿ�Ҫ��������
	bool bHaveLinkAction = IsHaveLinkAction(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bHaveLinkAction)
	{
		CPKServerMgrProcessInfo procLinkServer;
		procLinkServer.m_bAutoGenerate = true;
		procLinkServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procLinkServer.m_strExeName = "pklinkserver";
#ifdef _WIN32
		procLinkServer.m_strDisplayName = "��������";
#else
		procLinkServer.m_strDisplayName = "Link Server";
#endif
		AddToServerArray(vecServer, procLinkServer);
	}

	// �Ƿ�Ҫ������Ƶ����
	bool bHaveVideoServer = IsHaveVideoConfiged(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bHaveVideoServer)
	{
		CPKServerMgrProcessInfo procVideoServer;
		procVideoServer.m_bAutoGenerate = true;
		procVideoServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procVideoServer.m_strExeName = "pkvideoserver";
#ifdef _WIN32
		procVideoServer.m_strDisplayName = "��Ƶ����";
#else
		procVideoServer.m_strDisplayName = "Video Server";
#endif
		AddToServerArray(vecServer, procVideoServer);
	}

	// �Ƿ�������ʷ�洢����
	if (bHaveHisData)
	{
		CPKServerMgrProcessInfo procHisDataServer;
		procHisDataServer.m_bAutoGenerate = true;
		procHisDataServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procHisDataServer.m_strExeName = "pkhisdataserver";
#ifdef _WIN32
		procHisDataServer.m_strDisplayName = "��ʷ�洢����";
#else
		procHisDataServer.m_strDisplayName = "HisData Store Server";
#endif
		AddToServerArray(vecServer, procHisDataServer);
	}

	// �Ƿ�Ҫ������Ƶ����
	bool bDBTransfer = IsDBTransferConfiged(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bDBTransfer)
	{
		CPKServerMgrProcessInfo procDBTransferServer;
		procDBTransferServer.m_bAutoGenerate = true;
		procDBTransferServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procDBTransferServer.m_strExeName = "pkdbtransferserver";
#ifdef _WIN32
		procDBTransferServer.m_strDisplayName = "��ϵ���ݿ�ת������";
#else
		procDBTransferServer.m_strDisplayName = "DB Transfer Server";
#endif
		AddToServerArray(vecServer, procDBTransferServer);
	}

	// �Ƿ�Ҫ������Ƶ����
	bool bHaveCommunicateForward = IsHavePKCommunicateForward(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bHaveCommunicateForward)
	{
		CPKServerMgrProcessInfo procCommForward;
		procCommForward.m_bAutoGenerate = true;
		procCommForward.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procCommForward.m_strExeName = "pkcommforwardserver";
#ifdef _WIN32
		procCommForward.m_strDisplayName = "ͨѶ͸������";
#else
		procCommForward.m_strDisplayName = "Communicate Forward Server";
#endif
		AddToServerArray(vecServer, procCommForward);
	}

	// �Ƿ�Ҫ����Modbusת������
	bool bModbusServer = IsModbusServerConfiged(PKEviewDbApi); // �Ƿ�Ҫ�����ϼ��ڵ㴦�����
	if (bDBTransfer)
	{
		CPKServerMgrProcessInfo procModbusServer;
		procModbusServer.m_bAutoGenerate = true;
		procModbusServer.m_nSortIndex = 0;  // �ȽϿ�ǰ
		procModbusServer.m_strExeName = "pkmodserver";
#ifdef _WIN32
		procModbusServer.m_strDisplayName = "Modbus��������";
#else
		procModbusServer.m_strDisplayName = "Modbus Server";
#endif
		AddToServerArray(vecServer, procModbusServer);
	}

	CPKServerMgrProcessInfo procWeb;
	procWeb.m_bAutoGenerate = true;
	procWeb.m_nSortIndex = 0;  // �ȽϿ�ǰ
	procWeb.m_strExeName = "";
	procWeb.m_bAutoRestart = 0; // �ر�֮���Զ�������
#ifdef _WIN32
	procWeb.m_strDisplayName = "Web����";
	procWeb.m_strStartCmd = "..\\web\\startWeb.bat";
	procWeb.m_strStopCmd = "..\\web\\stopWeb.bat";
#else
	procWeb.m_strDisplayName = "Web Server";
	procWeb.m_strStartCmd = "../web/startWeb.sh";
	procWeb.m_strStopCmd = "../web/stopWeb.sh"; 
#endif
	AddToServerArray(vecServer, procWeb);

	return 0;
}

int ReadDriversInfoFromDB(CPKEviewDbApi &PKEviewDbApi, vector<CPKServerMgrProcessInfo> &vecServer)
{
	int nRet = 0;
	char szSql[2048] = { 0 };
	string strBinPath = PKComm::GetBinPath();
	// ��ȡ���е��������б�
	vector<vector<string> > vecDriver;
	int nVersion = PKEviewComm::GetVersion(PKEviewDbApi, "pknodeserver");
	if (nVersion > 0)
	{
		PKStringHelper::Snprintf(szSql, sizeof(szSql), "SELECT B.id as drv_id,B.name as drv_name,B.modulename as drv_modulename,B.description as drv_description,B.platform as drv_platform, B.enable as drv_enable \
		FROM t_device_driver B \
		WHERE (B.enable is null or B.enable<>0) \
		AND (select count(*) from t_device_list A where A.driver_id=B.id and (A.enable IS NULL OR A.enable <> 0) ) > 0");
	}
	else
	{
		PKStringHelper::Snprintf(szSql, sizeof(szSql), "SELECT B.id as drv_id,B.name as drv_name,B.modulename as drv_modulename,B.description as drv_description,B.platform as drv_platform, B.enable as drv_enable \
		FROM t_device_driver B \
		WHERE (B.enable is null or B.enable<>0) \
		AND (select count(*) from t_device_list A where A.driver_id=B.id) > 0");
	}
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecDriver, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Fail to qeury drivers from database, error code:%d, SQL:%s, error:%s", nRet, szSql, strError.c_str());
		return -2;
	}

	if (vecDriver.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Query database and get NO drivers configed, SQL:%s!", szSql);
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Query database and get %d Drivers Configed", vecDriver.size());

	for (int iDrv = 0; iDrv < vecDriver.size(); iDrv++)
	{
		vector<string> &row = vecDriver[iDrv];
		int iCol = 0;
		// ��������Ϣ
		string strDriverId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDisplayName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverExeName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverSupportPlatform = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverEnable = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (strDriverExeName.length() <= 0 || strDriverExeName.length() <= 0){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver: %s, modulename:%s, cannot empty, skip...", strDisplayName.c_str(), strDriverExeName.c_str());
			continue;
		}

		CPKServerMgrProcessInfo drvInfo;
		drvInfo.m_strExeName = strDriverExeName;
		drvInfo.m_strDisplayName = strDisplayName;
		drvInfo.m_strPlatform = strDriverSupportPlatform;
		drvInfo.m_bAutoGenerate = true;
		drvInfo.m_nSortIndex = 0;  // �ȽϿ�ǰ

		// ���û���ظ����������룬����������
		bool bFound = false;
		for (vector<CPKServerMgrProcessInfo>::iterator itDrv = vecServer.begin(); itDrv != vecServer.end(); itDrv++)
		{
			if (itDrv->m_strExeName.compare(drvInfo.m_strExeName) == 0) // �����Ѿ�����������Ҫ����
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver: %s, modulename:%s, HAS in List(count:%d), skip...", strDisplayName.c_str(), strDriverExeName.c_str(), vecServer.size());
			continue;
		}

		// pkdriver.exe��·��,�������ڵ�����.exe�ļ�������ȥ
		string strAbsDrvFilePath = strBinPath + PK_OS_DIR_SEPARATOR + "drivers" + PK_OS_DIR_SEPARATOR + drvInfo.m_strExeName +
			PK_OS_DIR_SEPARATOR + drvInfo.m_strExeName;
		string strTemplDriverPath = strBinPath + PK_OS_DIR_SEPARATOR + "pkdriver";
#ifdef _WIN32 // windows����Ҫ����.exe ��׺, linux�²���Ҫ����
		strTemplDriverPath = strTemplDriverPath + ".exe";
		strAbsDrvFilePath = strAbsDrvFilePath + ".exe";
#else
		strTemplDriverPath = strTemplDriverPath;
		strAbsDrvFilePath = strAbsDrvFilePath;
#endif
		if (PKFileHelper::IsFileExist(strTemplDriverPath.c_str()))
		{
			nRet = PKFileHelper::CopyAFile(strTemplDriverPath.c_str(), strAbsDrvFilePath.c_str());
			if (nRet == PK_SUCCESS)
				g_logger.LogMessage(PK_LOGLEVEL_INFO, "Success to Copy DriverExe Template:%s To Driver: %s", strTemplDriverPath.c_str(), strAbsDrvFilePath.c_str());
			else
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Fail to Copy DriverExe Template:%s To Driver: %s��return code:%d!", strTemplDriverPath.c_str(), strAbsDrvFilePath.c_str(), nRet);

		}
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CAN NOT FIND DriverExe template file: %s, Please confirm ALL drivers Executable have been copied manually!", strTemplDriverPath.c_str());
		string strRelDrvDir = "drivers";
		drvInfo.m_strExeDir = strRelDrvDir + PK_OS_DIR_SEPARATOR + drvInfo.m_strExeName;
		AddToServerArray(vecServer, drvInfo);
	}

	return 0;
}
