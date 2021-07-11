/**************************************************************
**************************************************************/
#include "MainTask.h"
#include "UserTimer.h"
#include "Device.h"
#include "PythonHelper.h"
#include <string>
#include "pklog/pklog.h"
#include "pythonerr.h"

extern CPKLog g_logger;
using namespace std;

extern string		g_strDrvName;	// ������׺��������
extern string		g_strDrvExtName; // ��׺������ .pyo,.dll, .so, .pyc
extern string		g_strDrvPath; // �õ�û����չ����·������ /gateway/bin/drivers/modbus,����������.dll

PyObject *				g_pyModule = NULL;

PyObject *				g_pyFuncInitDriver = NULL;
PyObject *				g_pyFuncInitDevice = NULL;
PyObject *				g_pyFuncUnInitDevice = NULL;
//PyObject *			g_pyFuncOnDeviceConnStateChanged = NULL;
PyObject *				g_pyFuncOnTimer = NULL;
PyObject *				g_pyFuncOnControl = NULL;
PyObject *				g_pyFuncUnInitDriver = NULL;
PyObject *				g_pyFuncGetVersion = NULL;

#ifdef DRV_SUPPORT_PYTHON // modserverҲ�����õ�����ļ�

static char g_szDataTypeNames[][17] = { DT_NAME_BOOL, DT_NAME_CHAR, DT_NAME_UCHAR, DT_NAME_INT16, DT_NAME_UINT16, DT_NAME_INT32, DT_NAME_UINT32, 
DT_NAME_INT64, DT_NAME_UINT64, DT_NAME_FLOAT, DT_NAME_DOUBLE, DT_NAME_TEXT, DT_NAME_BLOB, DT_NAME_FLOAT16 };
int PKDataTypeId2Name(int nDataTypeId, char *szDataTypename, int nDataTypeNameLen)
{
	if (nDataTypeId <= 0 || nDataTypeId > TAG_DT_MAX)
	{
		strcpy(szDataTypename, "unknown");
	}
	else
		PKStringHelper::Safe_StrNCpy(szDataTypename, g_szDataTypeNames[nDataTypeId], nDataTypeNameLen);

	return 0;
}

void Py_CDeviceConstructor(CDevice *pDevice)
{
}

int Py_InitDevice(CDevice *pDevice)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;

	if(!g_pyFuncInitDevice)
		return -1;

	// ���õ���ģ�����ں���, ��ȡ���ؽ��. �������һ���ֵ�
	PyObject *pyArgs = PyTuple_New(1);
	PyTuple_SetItem(pyArgs, 0, pDevice->m_pyDevice);
	Py_INCREF(pDevice->m_pyDevice); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���
	PyObject *pReturn = PyEval_CallObject(g_pyFuncInitDevice, pyArgs); // pargs);
	if (pyArgs)
		Py_DECREF(pyArgs);
	LogPythonError(); // ���Ӵ�����Ϣ��ӡ
	if (pReturn == NULL) {
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "Call Driver Py_InitDevice(%s) with retvalue.", pDevice->m_strName.c_str());
		return -1;
	}
	if (pReturn)
		Py_DECREF(pReturn);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Call Driver Py_InitDevice(%s) without retvalue.", pDevice->m_strName.c_str());

	return 0;
}


int Py_UnInitDevice(CDevice *pDevice)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;

	if (!g_pyFuncUnInitDevice)
		return -1;

	//PyObject *pargs = PyTuple_New(1);
	//PyTuple_SetItem(pargs, 0, pDevice->m_pyDevice);

	// ���õ���ģ�����ں���, ��ȡ���ؽ��. �������һ���ֵ�
	PyObject *pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs, 0, pDevice->m_pyDevice);
	Py_INCREF(pDevice->m_pyDevice); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���
	PyObject *pReturn = PyEval_CallObject(g_pyFuncUnInitDevice, pArgs); // pargs);
	LogPythonError(); // ���Ӵ�����Ϣ��ӡ
	if (pArgs)
		Py_DECREF(pArgs);
	if (pReturn == NULL) {
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "Call Driver Py_UnInitDevice(%s) with retvalue.", pDevice->m_strName.c_str());
		return -1;
	}
	if (pReturn)
		Py_DECREF(pReturn);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Call Driver Py_UnInitDevice(%s) without retvalue.", pDevice->m_strName.c_str());

	return 0;
}


void Py_CopyDeviceInfo2OutDevice(CDevice *pDevice)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return;

	// ����python�������õ����ݽṹ
	//PyObject *pCDevice = Py_BuildValue("O", pDevice);
	//PyDict_SetItemString(pDevice->m_pyDevice, "_InternalRef", Py_BuildValue("O", pDevice)); // ��ָ��ǿ��ת������ֵ��������
	pDevice->m_pyDevice = PyDict_New(); // �豸��Ϣ���ֵ䴫��ȥ�������Ƚ����

	// �豸���滹��Ҫ����tag���ָ��͸����������Ͳ���ÿ�θ����豸��ȥ��tag��������
	PyObject *pyVecTags = PyList_New(pDevice->m_vecAllTag.size());
	int iTag = 0;
	for (int iTag = 0; iTag < pDevice->m_vecAllTag.size(); iTag++)
	{
		PKTAG *pTag = pDevice->m_vecAllTag[iTag];

		PyObject *pyTag = PyDict_New();
		pTag->pData1 = (void *)pyTag;	// ����tag�����Ϣ, ����ʹ�á���C��������ʱ���ܻ��õ�pvData1������ʱ�಻��Ҫ����python������ָ�룬���Ը��Ǿ͸����˰�
		PyDict_SetItemString(pyTag, "_ctag", Py_BuildValue("K", reinterpret_cast<ACE_UINT64>(pTag))); // ��ָ��ǿ��ת������ֵ�����������Ա�Updatetagdata��OnControl֮��
#ifndef _WIN32
        PyObject* pyTagName = Py_BuildValue("s", pTag->szName);
        PyObject* pyTagAddress = Py_BuildValue("s", pTag->szAddress);
        //PyObject* pyTagName = PyUnicode_FromUnicode((const Py_UNICODE*)pTag->szName,strlen(pTag->szName));
        //PyObject* pyTagAddress = PyUnicode_FromUnicode((const Py_UNICODE*)pTag->szAddress,strlen(pTag->szAddress));
#else
        PyObject* pyTagName = Py_BuildValue("s", pTag->szName);
        PyObject* pyTagAddress = Py_BuildValue("s", pTag->szAddress);
#endif
        PyDict_SetItemString(pyTag, "name", pyTagName);
        PyDict_SetItemString(pyTag, "address", pyTagAddress);

		char szDataTypeName[32] = { 0 };
		PKDataTypeId2Name(pTag->nDataType, szDataTypeName, sizeof(szDataTypeName));
		PyDict_SetItemString(pyTag, "datatype", Py_BuildValue("s", szDataTypeName));
		PyDict_SetItemString(pyTag, "datatypeid", Py_BuildValue("i", pTag->nDataType));
		PyDict_SetItemString(pyTag, "pollrate", Py_BuildValue("i", pTag->nPollRate));
		PyDict_SetItemString(pyTag, "lenbits", Py_BuildValue("i", pTag->nLenBits));
		PyDict_SetItemString(pyTag, "param", Py_BuildValue("s", pTag->szParam));
		
		
		PyList_SetItem(pyVecTags, iTag, pyTag);
		Py_INCREF(pyTag);
	}
	PyDict_SetItemString(pDevice->m_pyDevice, "tags", pyVecTags);
	Py_INCREF(pyVecTags);

	PyDict_SetItemString(pDevice->m_pyDevice, "_InternalRef", Py_BuildValue("K", (ACE_UINT64)(pDevice))); // ��ָ��ǿ��ת������ֵ��������
	PyDict_SetItemString(pDevice->m_pyDevice, "name", Py_BuildValue("s", pDevice->m_strName.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "desc", Py_BuildValue("s", pDevice->m_strDesc.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "conntype", Py_BuildValue("s", pDevice->m_strConnType.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "conntimeout", Py_BuildValue("i", pDevice->m_nConnTimeOut));
	PyDict_SetItemString(pDevice->m_pyDevice, "recvtimeout", Py_BuildValue("i", pDevice->m_nRecvTimeOut));
	PyDict_SetItemString(pDevice->m_pyDevice, "connparam", Py_BuildValue("s", pDevice->m_strConnParam.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "param1", Py_BuildValue("s", pDevice->m_strParam1.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "param2", Py_BuildValue("s", pDevice->m_strParam2.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "param3", Py_BuildValue("s", pDevice->m_strParam3.c_str()));
	PyDict_SetItemString(pDevice->m_pyDevice, "param4", Py_BuildValue("s", pDevice->m_strParam4.c_str()));
	CDriver &driver = MAIN_TASK->m_driverInfo;
	PyDict_SetItemString(pDevice->m_pyDevice, "driver", driver.m_pyDriverInfo);
	Py_INCREF(driver.m_pyDriverInfo);
}

int Py_OnControl(CDevice *pDevice, PKTAG *pTag, string & strTagValue)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;

	if (!g_pyFuncOnControl)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Python, OnControl NOT IMPLEMENTED.device:%s, tag:%s(addr:%s)", pDevice->m_strName.c_str(), pTag->szName, strTagValue.c_str());
		return -1;
	}

	PyObject *pyTag = (PyObject *)pTag->pData1;
	Py_INCREF(pyTag);
	PyObject *pyTagData = Py_BuildValue("s", strTagValue.c_str());// , pTagData->nDataLen);
	PyObject *pyArgs = PyTuple_New(3);
	PyTuple_SetItem(pyArgs, 0, pDevice->m_pyDevice);
	PyTuple_SetItem(pyArgs, 1, pyTag);
	PyTuple_SetItem(pyArgs, 2, pyTagData);
	Py_INCREF(pDevice->m_pyDevice); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���
	PyObject *pReturn = PyEval_CallObject(g_pyFuncOnControl, pyArgs);
	LogPythonError(); // ���Ӵ�����Ϣ��ӡ��������쳣
	if (pyArgs)  // ���ﲻ�ܵ���ɾ��
		Py_DECREF(pyArgs);
	//if (pyTagData)
	//	Py_DECREF(pyTagData);
	if (pReturn == NULL) {
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "OnControl return with no retvalue.device:%s, tag:%s(addr:%s, value:%s)", pDevice->m_strName.c_str(), pTag->szName, pTag->szAddress, strTagValue.c_str());
		return 0;
	}
	if (pReturn)
		Py_DECREF(pReturn);
	return 0;
}

////----------------------------CDriver
void Py_CDriverConstructor(CDriver *pDriver)
{
	pDriver->m_pyDriverInfo = NULL;
}

int Py_InitDriver(CDriver *pDriver)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;

	// ��Ҫ��pDriver->m_pyDriverInfo���и�ֵ����Ԫ�飬�����ֵ䣿
	pDriver->m_pyDriverInfo = PyDict_New();

	PyDict_SetItemString(pDriver->m_pyDriverInfo, "name", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szName));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "desc", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szDesc));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "param1", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szParam1));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "param2", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szParam2));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "param3", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szParam3));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "param4", Py_BuildValue("s", pDriver->m_pOutDriverInfo->szParam4));
	PyDict_SetItemString(pDriver->m_pyDriverInfo, "_InternalRef", Py_BuildValue("K", reinterpret_cast<ACE_UINT64>(pDriver->m_pOutDriverInfo->_pInternalRef)));

	if (!g_pyFuncInitDriver)
		return 0;

	PyObject *pyArgs = PyTuple_New(1);
	PyTuple_SetItem(pyArgs, 0, pDriver->m_pyDriverInfo);
	Py_INCREF(pDriver->m_pyDriverInfo); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���
	// ������Ԫ�鴫���ȥ��
	PyObject *pReturn = PyEval_CallObject(g_pyFuncInitDriver, pyArgs);// , pyDriverInfo); // python�в��ܵ���print����!���򷵻ش���
	LogPythonError(); // ���Ӵ�����Ϣ��ӡ
	//if (pyArgs)
	//	Py_DECREF(pyArgs);
	if (pReturn == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "call InitDriver(%s) without retvalue", pDriver->m_strName.c_str());
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "call InitDriver(%s) with retvalue", pDriver->m_strName.c_str());
	if (pReturn)
		Py_DECREF(pReturn);
	return 0;
}

int Py_UnInitDriver(CDriver *pDriver)
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;
	return 0;
}

/////-----------------------------------------------
int Py_InitPython()
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return 0;

	// ��ʼ��������
	string strDllPath = g_strDrvPath;
	strDllPath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
	strDllPath += g_strDrvName; // drivers/modbus/modbus

	string strHomePath(PKComm::GetHomePath());
	string strPyHomePath = strHomePath + PK_OS_DIR_SEPARATOR + "python";
	PKStringHelper::Replace(strPyHomePath, ACE_DIRECTORY_SEPARATOR_STR, "/"); // binĿ¼
	Py_SetPythonHome((char *)strPyHomePath.c_str()); // python��homepath


	Py_Initialize();
	if (!Py_IsInitialized())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Py_IsInitialized failed!");
		return -1001;
	}

	// ���Python·��, ������ǰ·��, �����ܵ��뵱ǰ·���µ�ģ��
	char szImported[1024] = { 0 };
	// windows��sys.path.append('D:\\Work\\Gateway\\bin\\drivers\\opcdrv')����������˫б�ܺ�
	string strDirPathTmp = g_strDrvPath;
	//PyRun_SimpleString("import sys;sys.path.append('D:/Work/Gateway/bin/drivers/opcdrv')");
	//g_logger.LogMessage(PK_LOGLEVEL_INFO, "run to: PyRun_SimpleString(import sys;)");
	PyRun_SimpleString("import sys;"); //
	PKStringHelper::Replace(strDirPathTmp, ACE_DIRECTORY_SEPARATOR_STR, "/");
	snprintf(szImported, sizeof(szImported), "sys.path.append('%s')", strDirPathTmp.c_str());
    printf("driver:%s, set sys.path: %s\n", g_strDrvName.c_str(), szImported);
	PyRun_SimpleString(szImported);

    // add bin to path,because
	strDirPathTmp = PKComm::GetBinPath();
	PKStringHelper::Replace(strDirPathTmp, ACE_DIRECTORY_SEPARATOR_STR, "/"); // D:\Work\Gateway\bin\drivers\opcdrv �����Ϊ��D:/Work/Gateway/bin/drivers/opcdrv
	snprintf(szImported, sizeof(szImported), "sys.path.append('%s')", strDirPathTmp.c_str());

    PyRun_SimpleString(szImported);

	//snprintf(szImported, sizeof(szImported), "from %s import *", g_strDrvName.c_str()); // ��������У���������޷����á�InitDriver����������Ҫ���á�pytestdrv.InitDriver��
	//PyRun_SimpleString(szImported);

    // ����ģ������, ͨ��Ϊ���õ�Python�ű�������
	// PyRun_SimpleString("print sys.path"); //
	string strDrvModule = g_strDrvPath + ACE_DIRECTORY_SEPARATOR_STR + g_strDrvName;
	g_pyModule = PyImport_ImportModule(g_strDrvName.c_str());
	LogPythonError();
	if (g_pyModule == NULL) {
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "driver %s import python module:%s  failed! please check syntax and file existence!", g_strDrvName.c_str(), g_strDrvName.c_str());
		return -100;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "successful to load python driver:%s!", strDrvModule.c_str());

	// ��õ���ģ��ĺ�������
	g_pyFuncInitDriver = PyObject_GetAttrString(g_pyModule, "InitDriver");
	g_pyFuncInitDevice = PyObject_GetAttrString(g_pyModule, "InitDevice");
	g_pyFuncOnTimer = PyObject_GetAttrString(g_pyModule, "OnTimer");
	g_pyFuncOnControl = PyObject_GetAttrString(g_pyModule, "OnControl");
	g_pyFuncUnInitDevice = PyObject_GetAttrString(g_pyModule, "UnInitDevice");
	g_pyFuncUnInitDriver = PyObject_GetAttrString(g_pyModule, "UnInitDriver");
	//g_pyFuncOnDeviceConnStateChanged = PyObject_GetAttrString(g_pyModule, "OnDeviceConnStateChanged");
	g_pyFuncGetVersion = PyObject_GetAttrString(g_pyModule, "GetVersion");

	if (g_pyFuncInitDevice == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "driver %s, MUST IMPLEMENTInitDevice FUNCTION %s", strDllPath.c_str());
		return -200;
	}
	return 0;
}

void Py_UnInitPython()
{
	if (g_nDrvLang != PK_LANG_PYTHON)
		return;

	if(g_pyFuncUnInitDriver)
	{
		PyObject *pyArgs = PyTuple_New(1);
		PyTuple_SetItem(pyArgs, 0, MAIN_TASK->m_driverInfo.m_pyDriverInfo);
		Py_INCREF(MAIN_TASK->m_driverInfo.m_pyDriverInfo); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���

		PyObject *pReturn = PyEval_CallObject(g_pyFuncUnInitDriver, pyArgs);
		LogPythonError(); // ���Ӵ�����Ϣ��ӡ
		if (pyArgs)
			Py_DECREF(pyArgs);
		if(pReturn == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "call UnInitDriver failed");
		}
		if (pReturn)
			Py_DECREF(pReturn);
	}
	// �رս�����
	Py_Finalize();
}

bool IsPythonDriver()
{
	bool bIsPythonDriver = false;
	list<string> fileNameList;
	PKFileHelper::ListFilesInDir(g_strDrvPath.c_str(), fileNameList);
	for (list<string>::iterator itFileName = fileNameList.begin(); itFileName != fileNameList.end(); itFileName++)
	{
		string &strFileName = *itFileName;
		size_t nPos = strFileName.find(g_strDrvName); // modbus.pyo
		if (nPos == string::npos)
			continue;

		string strFileExt = strFileName.substr(nPos + g_strDrvName.length());  // .pyo
		string strFileExtLower = strFileExt;
		transform(strFileExtLower.begin(), strFileExtLower.end(), strFileExtLower.begin(), (int(*)(int))tolower); // linux�¶���Сд
		if (strFileExtLower.compare(".py") == 0 || strFileExtLower.compare(".pyc") == 0 || strFileExtLower.compare(".pyo") == 0 || strFileExtLower.compare(".pyd") == 0)
		{
			g_nDrvLang = PK_LANG_PYTHON;
			g_strDrvExtName = strFileExt;
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Directory:%s , Found python file with name:%s, assume this is PYTHON driver!!", g_strDrvPath.c_str(), strFileName.c_str());
			return true;
		}
	}
	return false;
}

////////////////////////////////---------------------------
PKDRIVER_EXPORTS void PyDrv_Test(int a, char *szTip)
{
	//printf("param1:%d, param2:%s\n", a, szTip);
	Drv_LogMessage(PK_LOGLEVEL_NOTICE, "param1:%d, param2:%s", a, szTip);
}

//////////////=======================

CUserTimer *Py_Timer2CTimer(PyObject *pyTimer)
{
	PyObject *pyCTimer = PyDict_GetItemString(pyTimer, "_ctimer");
	int nPtrValue = 0;
	int nResult = PyArg_Parse(pyCTimer, "i", &nPtrValue);
	if(nResult != 0)
		return NULL;

	CUserTimer *pUserTimer = NULL;
	memcpy(&pUserTimer, &nPtrValue, sizeof(int));
	return pUserTimer;
}

int Py_OnTimer(CUserTimer *pUserTimer)
{
	if(!g_pyFuncOnTimer)
		return -1;

	PyObject *pyTimerObj = PyDict_New();
	PyDict_SetItemString(pyTimerObj, "period", Py_BuildValue("i", pUserTimer->m_timerInfo.nPeriodMS));

	PyObject *pyArgs = PyTuple_New(3);
	PyTuple_SetItem(pyArgs, 0, pUserTimer->m_pDevice->m_pyDevice); // ��һ�����������豸������ڶ��������Ƕ�ʱ������
	PyTuple_SetItem(pyArgs, 1, pyTimerObj);
	PyTuple_SetItem(pyArgs, 2, pUserTimer->m_pyTimerParam);
	Py_INCREF(pUserTimer->m_pDevice->m_pyDevice); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���
	Py_INCREF(pUserTimer->m_pyTimerParam); // ��ֹ��ɾ�����Ԥ�ȷ���õĶ���

	// ���õ���ģ�����ں���, ��ȡ���ؽ��
	PyObject *pReturn = PyEval_CallObject(g_pyFuncOnTimer, pyArgs);
	LogPythonError(); // ���Ӵ�����Ϣ��ӡ
	if (pyArgs)
		Py_DECREF(pyArgs);
	if (pReturn)
		Py_DECREF(pReturn);
	//if (pReturn == NULL) {
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����OnTimer�����쳣������OnTimer�����﷨");
	//	return -1;
	//}
	return 0;
}

#else // DRV_SUPPORT_PYTHON

void Py_CDeviceConstructor(CDevice *pDevice){}
int Py_InitDevice(CDevice *pDevice){return 0;}
int Py_UnInitDevice(CDevice *pDevice){return 0;}
void Py_CopyDeviceInfo2OutDevice(CDevice *pDevice){ return; }
int Py_UpdateTagsData(CDevice *pDevice, PyObject * pyTagsList){return 0;}
int Py_PyCreateAndStartTimer(CDevice *pDevice, int nPeroid, PyObject *pyTimerParam){return 0;}
PKTAG *Py_Tag2CTag(PyObject *pyTag){return NULL;}
int Py_OnControl(CDevice *pDevice, PKTAG *pTag, string & strTagValue){return 0;}

//////////////////////////
void Py_CDriverConstructor(CDriver *pDriver){return;}
int Py_InitDriver(CDriver *pDriver){return 0;}
int Py_UnInitDriver(CDriver *pDriver){return 0;}

////////////////////////////////-----------
int Py_InitPython(){return 0;}
void Py_UnInitPython(){return;}
bool IsPythonDriver(){return false;}
int Py_OnTimer(CUserTimer *pUserTimer){return 0;}
#endif
