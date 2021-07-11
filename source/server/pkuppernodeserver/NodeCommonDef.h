/**************************************************************
*  Filename:    SockHandler.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: EAProcessor.h.
*
*  @author:     shijupu
*  @version     05/29/2015  shijupu  Initial Version
**************************************************************/

#ifndef _NODESERVER_COMMON_DEFINE_H_
#define _NODESERVER_COMMON_DEFINE_H_

#include <ace/Basic_Types.h>
#include <ace/Time_Value.h>
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "mosquitto/mosquitto.h"
#include "mqttImpl.h"
#include <string>
#include <vector>
#include <map>
using namespace std;

extern CPKLog g_logger;

#define EPSINON		0.000001	// 一个很小的值，用于浮点数比较

class CNodeTag;

class CNodeInfo
{
public: // 静态配置信息
	string m_strId;
	string m_strCode;	// 唯一标志
	string m_strName;
	string m_strDesc; // 描述
	string m_strAuthInfo; // 输入的鉴权信息，可能是用户密码获得的唯一鉴权信息

	vector<CNodeTag *> m_vecTag;

public:
	string	m_strTopicRealData;		// 发送实时数据的主题
	string	m_strTopicControl;		// 接收控制命令的主题
	string m_strTopicConfig;		// 发送配置到上级节点的topic

public:
	ACE_Time_Value	m_tvLastData;		// 上次收发数据的成功时间，包括：发送成功，收到命令等
	bool		m_bAlive;			// 超过30秒没有收到数据则认为死掉了
public:
	CNodeInfo()
	{
		m_strId = m_strCode = m_strName = m_strDesc = "";
		m_strAuthInfo = "";
		m_strTopicRealData = m_strTopicControl = "";
		m_tvLastData = ACE_Time_Value::zero;
		m_vecTag.clear();
		m_bAlive = true;
	}
};

class CNodeTag
{
public:
	string	m_strName;
	string	m_strAddress;
	string	m_strDataType;
	string	m_strDataLen; 
 	string	m_strDesc; 	
	string m_strInitValue;	// 数据库的初始值

	CNodeInfo * m_pNode;
	CNodeTag()
	{
		m_strName = m_strAddress = m_strDataType = "";
		m_strDataLen = "";
 		m_strDesc = "";
		m_strInitValue = "";
		m_pNode = NULL;
	}
};


#endif // _NODESERVER_COMMON_DEFINE_H_
