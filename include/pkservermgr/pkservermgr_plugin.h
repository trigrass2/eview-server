/**************************************************************
 *  Filename:    pkservermgr_plugin.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
**************************************************************/
#ifndef _PK_SERVERMGR_PLUGIN_H_
#define _PK_SERVERMGR_PLUGIN_H_

// PKServerMgr的有效的进程配置信息
class CPKServerMgrProcessInfo
{
public:
	string m_strDisplayName; // Name,显示名称, t_device_driver的name字段
	string m_strExeName;	// ExeName, t_device_driver的modulename字段
	string m_strExeDir;
	string m_strStartCmd;	// StartCmd
	string m_strStopCmd;	// StopCmd

	string m_strPlatform;	// Platform, win/linux
	string m_strRestartAt;	// 定时重启设置。RestartAt:Day=1/1;Hour=*;Minute=*
	string m_strDesc;		// 描述
	int	   m_nDelaySec;		// DelaySec
	bool    m_bShowConsole; // 是否显示
	bool	m_bEnable;		// 是否禁用
	bool	m_bAutoRestart; // 是否自动重启。如果StartCmd不为空则0，否则1
	bool   m_bAutoGenerate; // 如果是自动生成的进程，会在下次重新加载时不包含期内
	int    m_nSortIndex;	// 放在第几个，缺省为-1表示无所谓，0表示放在第一个位置. 大于最大值表示放在最后的位置

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
