#ifndef _Drv_FRAMEWORK_H_
#define _Drv_FRAMEWORK_H_

#define NULLASSTRING(x) x==NULL?"":x

#include <ace/Event_Handler.h>
#include <ace/OS_NS_strings.h>
#include <map>
#include <list>
#include <string>
#include "common/eviewdefine.h"
#include "pkdriver/pkdrvcmn.h"	// CONFIGHANDLE, DRVHANDLE
#include "tinyxml2/tinyxml2.h"
#include "string.h"

#define MAX_DRVOBJ_USERDATA_SIZE		20			// �û����ݵ������Ŀ
#define MAX_DRVOBJ_USERPTR_SIZE         20          //�û�ָ��������Ŀ

#define PK_LANG_UNKNOWN				0				// �Ȳ���cppҲ����python���Ե�����
#define PK_LANG_CPP					1				// cpp���Ե�����
#define PK_LANG_PYTHON					2			// python���Ե�����
using namespace std;

extern string	g_strDrvPath;
extern string	g_strDrvName;
extern int		g_nDrvLang; // ��������

typedef long (*PFN_InitDriver)(PKDRIVER *pDriver);
typedef long (*PFN_InitDevice)(PKDEVICE *pDevice);
typedef long (*PFN_UnInitDevice)(PKDEVICE *pDevice);
typedef void (*PFN_OnDeviceConnStateChanged)(PKDEVICE *pDevice, int nConnectState);

typedef long (*PFN_OnTimer)(PKDEVICE *pDevice, PKTIMER *timeInfo);
typedef long (*PFN_OnControl)(PKDEVICE *pDevice, PKTAG *pTag, const char *szStrValue, long lCmdId);

typedef long (*PFN_UnInitDriver)(PKDRIVER *pDriver);
typedef long (*PFN_GetVersion)();

extern PFN_InitDriver			g_pfnInitDriver;
extern PFN_UnInitDriver			g_pfnUnInitDriver;
extern PFN_InitDevice			g_pfnInitDevice;
extern PFN_UnInitDevice			g_pfnUnInitDevice;
extern PFN_OnDeviceConnStateChanged		g_pfnOnDeviceConnStateChanged;
extern PFN_OnTimer				g_pfnOnTimer;
extern PFN_OnControl			g_pfnOnControl;
extern PFN_GetVersion			g_pfnGetVersion;


/*
extern PyObject *				g_pyFuncInit;
extern PyObject *				g_pyFuncInitDevice;
extern PyObject *				g_pyFuncUnInitDevice;
extern PyObject *				g_pyFuncOnDeviceConnStateChanged;
extern PyObject *				g_pyFuncOnTimer;
extern PyObject *				g_pyFuncOnControl;
extern PyObject *				g_pyFuncUnInit;
extern PyObject *				g_pyFuncGetVersion;
*/

// ������������Ļ��࣬�������豸�顢�豸�����ݿ飩
class CDrvObjectBase
{
public:
	CDrvObjectBase()
	{
		m_bConfigChanged = false;
		m_strName = "";
		m_strDesc = "";
	}
	virtual ~CDrvObjectBase()
	{
		m_mapAttrNameToValue.clear();
	}
	virtual CDrvObjectBase &operator=(CDrvObjectBase &theDrvObj)
	{
		m_strName = theDrvObj.m_strName;
		m_strDesc = theDrvObj.m_strDesc;
		m_strParam1 = theDrvObj.m_strParam1;
		m_strParam2 = theDrvObj.m_strParam2;
		m_strParam3 = theDrvObj.m_strParam3;
		m_strParam4 = theDrvObj.m_strParam4;
		m_mapAttrNameToValue = theDrvObj.m_mapAttrNameToValue;

		return *this;
	}

	//�ж�������Ϣ�Ƿ���ͬ���Ƚϸ������ԣ����������Ӷ���ıȽϡ�����һ��ȱʡʵ��
	virtual bool operator==( CDrvObjectBase &theDrvObjectBase)
	{
#ifdef _WIN32
		if(stricmp(theDrvObjectBase.m_strName.c_str(), m_strName.c_str()) != 0)
			return false;
		if(stricmp(theDrvObjectBase.m_strDesc.c_str(), m_strDesc.c_str()) != 0)
			return false;
		if(stricmp(theDrvObjectBase.m_strParam1.c_str(), m_strParam1.c_str()) != 0)
			return false;
		if(stricmp(theDrvObjectBase.m_strParam2.c_str(), m_strParam2.c_str()) != 0)
			return false;
		if(stricmp(theDrvObjectBase.m_strParam3.c_str(), m_strParam3.c_str()) != 0)
			return false;
		if(stricmp(theDrvObjectBase.m_strParam4.c_str(), m_strParam4.c_str()) != 0)
			return false;
#else
		if(strcasecmp(theDrvObjectBase.m_strName.c_str(), m_strName.c_str()) != 0)
			return false;
		if(strcasecmp(theDrvObjectBase.m_strDesc.c_str(), m_strDesc.c_str()) != 0)
			return false;
		if(strcasecmp(theDrvObjectBase.m_strParam1.c_str(), m_strParam1.c_str()) != 0)
			return false;
		if(strcasecmp(theDrvObjectBase.m_strParam2.c_str(), m_strParam2.c_str()) != 0)
			return false;
		if(strcasecmp(theDrvObjectBase.m_strParam3.c_str(), m_strParam3.c_str()) != 0)
			return false;
		if(strcasecmp(theDrvObjectBase.m_strParam4.c_str(), m_strParam4.c_str()) != 0)
			return false;
#endif // WIN32

		if(m_mapAttrNameToValue.size() != theDrvObjectBase.m_mapAttrNameToValue.size())
			return false;

		map<string, string>::iterator itAttr = m_mapAttrNameToValue.begin();
		map<string, string>::iterator itAttr2 = theDrvObjectBase.m_mapAttrNameToValue.begin();
		for(; itAttr != m_mapAttrNameToValue.end(); itAttr ++,itAttr2++)
		{
			if(itAttr->first.compare(itAttr2->first) != 0 || itAttr->second.compare(itAttr2->second) != 0)
				return false;
		}
		
		return true;
	}

	int LoadParamsFromXml(tinyxml2::XMLElement *pNodeDrvObj)
	{
		if(!pNodeDrvObj)
			return -1;

		m_strName = NULLASSTRING(pNodeDrvObj->Attribute("name"));
		m_strDesc = NULLASSTRING(pNodeDrvObj->Attribute("desc"));
		m_strParam1 = NULLASSTRING(pNodeDrvObj->Attribute("param1"));
		m_strParam2 = NULLASSTRING(pNodeDrvObj->Attribute("param2"));
		m_strParam3 = NULLASSTRING(pNodeDrvObj->Attribute("param3"));
		m_strParam4 = NULLASSTRING(pNodeDrvObj->Attribute("param4"));

		//��ȡ�豸����������
		for (const tinyxml2::XMLAttribute *pAttribute =  pNodeDrvObj->FirstAttribute(); 
			pAttribute != NULL;// && pAttribute != pNodeDrvObj->LastAttribute(); 
			pAttribute = pAttribute->Next())
		{
			if (pAttribute->Value() != NULL)
				m_mapAttrNameToValue[pAttribute->Name()] = pAttribute->Value();
		}
		
		return PK_SUCCESS;
	}
public:
	string	m_strParam1, m_strParam2, m_strParam3,m_strParam4;// Ԥ������������
	string	m_strName;				// ����
	string	m_strDesc;		// ����
	map<string, string>	m_mapAttrNameToValue;	// ���ݿ�������ļ���ȡ�������Ժ�ֵ�Ե�ӳ��

public:
	bool	m_bConfigChanged;		// �Ƿ���Ϊ���߲����޸������ݿ飨����ʼ��ַ�����ȡ��ֽ����͡�˳��ȣ����豸��Ϣ���豸�鲻��Ҫ�жϣ���Ϊֻ���������岻��
};

#endif // _Drv_FRAMEWORK_H_
