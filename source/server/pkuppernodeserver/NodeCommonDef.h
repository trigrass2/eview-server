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

#define EPSINON		0.000001	// һ����С��ֵ�����ڸ������Ƚ�

class CNodeTag;

class CNodeInfo
{
public: // ��̬������Ϣ
	string m_strId;
	string m_strCode;	// Ψһ��־
	string m_strName;
	string m_strDesc; // ����
	string m_strAuthInfo; // ����ļ�Ȩ��Ϣ���������û������õ�Ψһ��Ȩ��Ϣ

	vector<CNodeTag *> m_vecTag;

public:
	string	m_strTopicRealData;		// ����ʵʱ���ݵ�����
	string	m_strTopicControl;		// ���տ������������
	string m_strTopicConfig;		// �������õ��ϼ��ڵ��topic

public:
	ACE_Time_Value	m_tvLastData;		// �ϴ��շ����ݵĳɹ�ʱ�䣬���������ͳɹ����յ������
	bool		m_bAlive;			// ����30��û���յ���������Ϊ������
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
	string m_strInitValue;	// ���ݿ�ĳ�ʼֵ

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
