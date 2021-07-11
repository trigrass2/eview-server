/**************************************************************
 *  Filename:    pkservermgr_plugin.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
**************************************************************/
#ifndef _PK_SERVERMGR_PLUGIN_H_
#define _PK_SERVERMGR_PLUGIN_H_

// PKServerMgr����Ч�Ľ���������Ϣ
class CPKServerMgrProcessInfo
{
public:
	string m_strDisplayName; // Name,��ʾ����, t_device_driver��name�ֶ�
	string m_strExeName;	// ExeName, t_device_driver��modulename�ֶ�
	string m_strExeDir;
	string m_strStartCmd;	// StartCmd
	string m_strStopCmd;	// StopCmd

	string m_strPlatform;	// Platform, win/linux
	string m_strRestartAt;	// ��ʱ�������á�RestartAt:Day=1/1;Hour=*;Minute=*
	string m_strDesc;		// ����
	int	   m_nDelaySec;		// DelaySec
	bool    m_bShowConsole; // �Ƿ���ʾ
	bool	m_bEnable;		// �Ƿ����
	bool	m_bAutoRestart; // �Ƿ��Զ����������StartCmd��Ϊ����0������1
	bool   m_bAutoGenerate; // ������Զ����ɵĽ��̣������´����¼���ʱ����������
	int    m_nSortIndex;	// ���ڵڼ�����ȱʡΪ-1��ʾ����ν��0��ʾ���ڵ�һ��λ��. �������ֵ��ʾ��������λ��

public:
	CPKServerMgrProcessInfo()
	{
		m_strExeName = m_strPlatform = m_strDisplayName = m_strStartCmd = m_strStopCmd = m_strRestartAt = m_strExeDir = "";
		m_nDelaySec = 0;
		m_bShowConsole = false;
		m_bAutoRestart = true;
		m_bEnable = true;
		m_bAutoGenerate = false;
		m_nSortIndex = 0;
	}

	CPKServerMgrProcessInfo(const CPKServerMgrProcessInfo &x)
	{
		*this = x;
	}
	CPKServerMgrProcessInfo & operator=(const CPKServerMgrProcessInfo &x)
	{
		m_strExeName = x.m_strExeName;
		m_strPlatform = x.m_strPlatform;
		m_strDisplayName = x.m_strDisplayName;
		m_strStartCmd = x.m_strStartCmd;
		m_strStopCmd = x.m_strStopCmd;
		m_strRestartAt = x.m_strRestartAt;
		m_strExeDir = x.m_strExeDir;
		m_nDelaySec = x.m_nDelaySec;
		m_bShowConsole = x.m_bShowConsole;
		m_bAutoRestart = x.m_bAutoRestart;
		m_bEnable = x.m_bEnable;
		m_bAutoGenerate = x.m_bAutoGenerate;
		m_nSortIndex = x.m_nSortIndex;

		return *this;
	}
};

#endif // _PK_SERVERMGR_PLUGIN_H_
