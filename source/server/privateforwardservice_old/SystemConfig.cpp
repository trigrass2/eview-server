/**************************************************************
 *  Filename:    SystemConfig.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 读取配置信息.
 *
**************************************************************/
#include "ace/ACE.h"
#include "ace/OS_NS_strings.h"
#include "ace/DLL.h"
#include "SystemConfig.h"
#include "tinyxml.h"
#include "common/pkcomm.h"
#include "common/pklog.h"
#include "common/pkGlobalHelper.h"
#include <algorithm>
#include "ace/OS_NS_stdio.h"

#define NULLASSTRING(x) x==NULL?"":x
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CPKLog PKLog;

CSystemConfig::CSystemConfig()
{
}

CSystemConfig::~CSystemConfig()
{

}

/**
 *  读取配置文件.
 *
 *
 *  @version     02/23/2009  liuqifeng  Initial Version.
 *  modify by sjp, arrange codes
 */
bool CSystemConfig::LoadConfig()
{		
	// 可能只有服务端的端口号、域名可以读取。点的地址、名称放在forwardAccessor中读取了
	long nStatus = 0;
	// 形成配置文件名称路径
	char szConfigFile[PK_SHORTFILENAME_MAXLEN];
	char *szCVProjCfgPath = (char *)CVComm.GetCVProjCfgPath(); 
	if(szCVProjCfgPath)
		sprintf(szConfigFile, "%s%c%s",szCVProjCfgPath, ACE_DIRECTORY_SEPARATOR_CHAR, "innerforward.xml");
	else
		sprintf(szConfigFile, "%s", "innerforward.xml");

	// 加载配置文件xml格式
	TiXmlElement* pElmChild = NULL;
	TiXmlDocument doc(szConfigFile);
	if( !doc.LoadFile() ) 
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s failed, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Load Config File %s Successful!", szConfigFile);

	// 2.Get Root Element.
	TiXmlElement* pElmRoot = doc.RootElement(); // "innerforward" node
	if(!pElmRoot)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no root Elem, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	m_strProjectCode = NULLASSTRING(pElmRoot->Attribute("project"));

	if(m_strProjectCode.empty())
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no project code is null, errCode=%d!", 
			szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	
	TiXmlElement* pNodeServer = pElmRoot->FirstChildElement("server");
	//为了检查组名是否重复 
	if(!pNodeServer)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no innerforward/server node, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	m_strServerIp = NULLASSTRING(pNodeServer->Attribute("ip")); // ip or domain
	string strServerPort = NULLASSTRING(pNodeServer->Attribute("port")); // ip or domain
	m_nServerPort = ::atoi(strServerPort.c_str());
	string strCycleMS = NULLASSTRING(pNodeServer->Attribute("cycle_ms")); // ip or domain
	m_nCycleMS = ::atoi(strCycleMS.c_str());

	if(m_strServerIp.empty())
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no innerforward/server::ip is null, errCode=%d!", 
			szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}
	if(m_nServerPort <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Inner forward Driver Config File %s innerforward/server::port(%s) is null or negative , errCode=%d! default to 8890", 
			szConfigFile, strServerPort.c_str(), EC_ICV_DA_CONFIG_FILE_INVALID);
		m_nServerPort = 8890;
	}

	if(m_nCycleMS <= 0 || m_nCycleMS > 1000*60*60)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Inner forward Driver Config File %s innerforward/server::cycle_ms(%s) is null or negative , errCode=%d! default to 1 minutes(1000*60ms)", 
			szConfigFile, strCycleMS.c_str(), EC_ICV_DA_CONFIG_FILE_INVALID);
		m_nCycleMS = 1000*60;
	}

	TiXmlElement* pNodeGateway = pElmRoot->FirstChildElement("gateway");

	if(!pNodeGateway)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no driverupdate/gateway node, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	m_strGatewayName = NULLASSTRING(pNodeGateway->Attribute("name")); // gateway name
	if(m_strGatewayName.empty() || m_strGatewayName.length() > 16)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "DriverUpdater Config File %s has no driverupdate/gateway::name is null, errCode=%d!", 
			szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	m_strGatewayPath = NULLASSTRING(pNodeGateway->Attribute("path")); // gateway name
	if(m_strGatewayPath.empty() || m_strGatewayPath.length() > 255)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "DriverUpdater Config File %s has no driverupdate/gateway::path is null, errCode=%d!", 
			szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}
	if (m_strGatewayPath.at(m_strGatewayPath.length()-1) == '\\' || m_strGatewayPath.at(m_strGatewayPath.length()-1) == '/')
	{
		m_strGatewayPath = m_strGatewayPath.substr(0, m_strGatewayPath.length() - 1);
	}

	string::size_type pos = 0;

	while((pos = m_strGatewayPath.find("\\", pos)) != string::npos) {
		m_strGatewayPath.replace(pos, 1, "/");
		pos++;
	}

	return true;
}

