
#include "python/Python.h"
#include <string>
#include "pkdriver/pkdrvcmn.h"
#include "pkdata/pkdata.h"
#include "pklog/pklog.h"
#include "ace/Basic_Types.h"
#include "common/eviewcomm_internal.h"

#ifndef PK_SUCCESS
#define PK_SUCCESS			0
#endif	// PK_SUCCESS

CPKLog g_logger;

PKHANDLE HandleFromInt(ACE_UINT64 &nHandle)
{
	return (PKHANDLE)nHandle;
}

ACE_UINT64 HandleToInt(PKHANDLE &hPKData)
{
	return (ACE_UINT64)hPKData;
}

//////////������python��������Ҫ�����ĺ���/////////////////////////////////////////////////
// ��ʼ��pkInit����������IP���û���������
// ����2Ԫ��(status, handle), ���status==1��������handle
static PyObject *PyData_Init(PyObject *self, PyObject *args)
{
	g_logger.SetLogFileName(NULL);
	const char *szServerIp = NULL;
	const char *szUserName = NULL;
	const char *szPassword = NULL;

	int nResult = PyArg_ParseTuple(args, "|sss", &szServerIp, &szUserName, &szPassword);
	string strServerIp = "";
	string strUserName = "";
	string strPassword = "";
	if (szServerIp)
		strServerIp = szServerIp;
	if (szUserName)
		strUserName = szUserName;
	if (szPassword)
		strPassword = szPassword;

	if (!nResult)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_InitServer(ip=%s, userName=%s, password=%s)ʱ, �������벻��", 
			strServerIp.c_str(), strUserName.c_str(), strPassword.c_str());
		PyObject *pyResult = Py_BuildValue("K", 0);
		return pyResult;
	}

	PKHANDLE hPKData = pkInit((char *)szServerIp, (char *)szUserName, (char *)szPassword);
	if (!hPKData)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_InitServer(serverIp=%s, userName=%s, password=%s) return 0",
			strServerIp.c_str(), strUserName.c_str(), strPassword.c_str());
		PyObject *pyResult = Py_BuildValue("K", 0);
		return pyResult;
	}

	ACE_UINT64 nHandle = HandleToInt(hPKData);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PyData_InitServer(serverIp=%s, userName=%s, password=%s), return handle:%ld"
		, strServerIp.c_str(), strUserName.c_str(), strPassword.c_str(), nHandle);
	PyObject *pyResult = Py_BuildValue("K", nHandle);
	//PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", 0));
	//PyTuple_SetItem(pyResult, 1, Py_BuildValue("I", nHandle));


	return pyResult;
}

// ��ʼ��pkInit�����룺PyData_Init���صĶ�Ԫ��ĵڶ���Ԫ�أ������
// ����1��������0��ʾ�ɹ�
static PyObject *PyData_Exit(PyObject *self, PyObject *args)
{
	ACE_UINT64 nPtrValue = 0;
	int nResult = PyArg_ParseTuple(args, "|K", &nPtrValue); //unsigned long long
	PKHANDLE hPKData = HandleFromInt(nPtrValue);
	// memcpy(&hPKData, (void *)&nPtrValue, sizeof(int));

	int nRet = pkExit(hPKData);
	return Py_BuildValue("i", 0);
}

// ��ȡtagֵ�����룺handle, tagName [,fieldname]����������IP���û���������
// ����2Ԫ��(status, �ַ���ֵ), ���status==0���ַ���ֵ��Ч
static PyObject *PyData_Get(PyObject *self, PyObject *args)
{
	ACE_UINT64 nHandle = 0;
	const char *szTagName = NULL;
	const char *szTagField = NULL;

	PyObject *pyResult = PyTuple_New(2);
	int nResult = PyArg_ParseTuple(args, "Ks|s",&nHandle, &szTagName, &szTagField); // tagField��ѡ
	if (!nResult)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_Get(szTagName, szTagField)ʱ, �������벻��");
		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -100));
		PyTuple_SetItem(pyResult, 1, Py_BuildValue("s", ""));
		return pyResult;
	}

	PKHANDLE hPKData = HandleFromInt(nHandle);
	char *szTagData = new char[PROCESSQUEUE_TAG_DATASIZE_LENGHT];
	//char szTagData[PK_TAGDATA_MAXLEN];
	szTagData[0] = szTagData[PROCESSQUEUE_TAG_DATASIZE_LENGHT - 1] = '\0';
	int nTagDataLen = 0;
	int nRet = pkGet(hPKData, szTagName, szTagField, szTagData, PROCESSQUEUE_TAG_DATASIZE_LENGHT, &nTagDataLen);
	if (nRet == 0)
	{
		szTagData[nTagDataLen] = '\0';
		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", 0));
		PyTuple_SetItem(pyResult, 1, Py_BuildValue("s", szTagData));
		delete[] szTagData;
		return pyResult;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_Get(szTagName=%s, szTagField), return %d", szTagName, nRet);
		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -100));
		PyTuple_SetItem(pyResult, 1, Py_BuildValue("s", ""));
		delete[] szTagData;
		return pyResult;
	}
	
	return pyResult;
}


// ����handle,tag�������ԣ�ֵ
// ����2Ԫ��int, ���status=01����ʾ���ͳɹ���
static PyObject *PyData_Control(PyObject *self, PyObject *args)
{
	ACE_UINT64 nHandle = 0;
	const char *szTagName = NULL;
	const char *szTagField = NULL;
	const char *szTagValue = NULL;
	PyObject *pyResult = PyTuple_New(2);
	int nResult = PyArg_ParseTuple(args, "Kss|s", &nHandle, &szTagName, &szTagValue, &szTagField); // tagField��ѡ
	if (!nResult)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_Get(Handle, szTagName, szTagValue, szTagField)ʱ, �������벻��");
		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -100));
		PyTuple_SetItem(pyResult, 1, Py_BuildValue("s", ""));
		return pyResult;
	}

	PKHANDLE hPKData = HandleFromInt(nHandle); 
	int nRet = pkControl(hPKData, szTagName, szTagField, szTagValue);
	if (nRet == 0)
	{
 		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", 0));
		return pyResult;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PyData_Get(szTagName=%s, szTagField), return %d", szTagName, nRet);
		PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", nRet));
		return pyResult;
	}

	return pyResult;
}


/////////////////-----------------------------
static PyMethodDef AllMyMethods[] = {
	{ "Init", (PyCFunction)PyData_Init, METH_VARARGS },
	{ "Exit", (PyCFunction)PyData_Exit, METH_VARARGS },
	{ "Get", (PyCFunction)PyData_Get, METH_VARARGS },
	{ "Control", (PyCFunction)PyData_Control, METH_VARARGS },
    { NULL, NULL, 0, NULL}
};

//extern "C"              //���ӻᵼ���Ҳ���initexample
PyMODINIT_FUNC initpydata()
{
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "pydata");
	PyObject *m, *d;
	m = Py_InitModule("pydata", AllMyMethods);
	d = PyModule_GetDict(m);
}
