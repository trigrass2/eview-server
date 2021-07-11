
#include "python/Python.h"
#include <string>
#include "pkdriver/pkdrvcmn.h"
#include "../pkdrvcmn/Device.h"

#ifndef PK_SUCCESS
#define PK_SUCCESS			0
#endif	// PK_SUCCESS

//////////������python��������Ҫ�����ĺ���/////////////////////////////////////////////////
// �������ݿ�����ݡ�״̬��ʱ���, ���µ�Shm��ȥ

PKTAG *Py_PyTag2CTag(PyObject *pyTag)
{
    PyObject *pCTag = PyDict_GetItemString(pyTag, "_ctag");
    ACE_UINT64 nPtrValue = 0;
    int bSuccess = PyArg_Parse(pCTag, "K", &nPtrValue);
    if (!bSuccess)
        return NULL;

    PKTAG *pTag = NULL;
    memcpy(&pTag, &nPtrValue, sizeof(PKTAG *));
    return pTag;
}

// �����ֵ���µ�pdb��, �������tags�����飨list����ÿ��tags�¶���value��quality��time�����ֶ�
int Py_UpdateTagsData(CDevice *pDevice, PyObject * pyTagsList)
{
    char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    int nTagNum = PyList_Size(pyTagsList);
    if (nTagNum <= 0)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "UpdateTagsData(pytaglist) tagList size <= 0");
        return -1;
    }

    vector<PKTAG *> vecTag;
    int i = 0;
    for (i = 0; i < nTagNum; i++)
    {
        PyObject *pyTag = PyList_GetItem(pyTagsList, i);
        PKTAG *pTag = Py_PyTag2CTag(pyTag);
        if (!pTag)
        {
            Drv_LogMessage(PK_LOGLEVEL_ERROR, "����PyTagû���ҵ�C��DRVTAG");
            continue;
        }
        vecTag.push_back(pTag);
    }

    Drv_UpdateTagsData(pDevice->m_pOutDeviceInfo, vecTag.data(), vecTag.size());

    vecTag.clear();

    return 0;
}

CDevice *PyDevice2CDevice(PyObject *pyDevice)
{
    PyObject *pCDevice = PyDict_GetItemString(pyDevice, "_InternalRef");
    ACE_UINT64 nPtrValue = 0;
    int nResult = PyArg_Parse(pCDevice, "K", &nPtrValue);
    //if (nResult != 0)
    //	return NULL;

    CDevice *pChannel = NULL;
    memcpy(&pChannel, &nPtrValue, sizeof(CDevice *)); // ָ������8���ֽ�
    return pChannel;
}


CUserTimer * Py_CreateAndStartTimer(CDevice *pDevice, int nPeroidMS, PyObject *pyTimerParam)
{
	if (!pDevice)
		return NULL;

    PKTIMER timerInfo;
    timerInfo.nPeriodMS = nPeroidMS;
    void *pTimerHandle = Drv_CreateTimer(pDevice->m_pOutDeviceInfo, &timerInfo);
    if (pTimerHandle == NULL)
    {
        Drv_LogMessage(PK_LOGLEVEL_INFO, "Drv_CreateTimer(pDevice:%s, period:%d) return failed", pDevice->m_strName.c_str(), nPeroidMS);
        return NULL;
    }

    //CUserTimer *pTimer = pDevice->CreateAndStartTimer(&timerInfo);
    CUserTimer *pBlockTimer = (CUserTimer *)pTimerHandle;
    pBlockTimer->m_pyTimerParam = pyTimerParam;
	Py_INCREF(pyTimerParam); // ���ⱻ�ͷ�
	Py_INCREF(pBlockTimer->m_pyTimerParam); // ���ⱻ�ͷ�
    return pBlockTimer;
}

static PyObject *PyDrv_UpdateTagsData(PyObject *self, PyObject *args) // (PyObject *pyDevice, PyObject *pyTagsList)
{
    PyObject *pyDevice = NULL;
    PyObject *pyTagsList = NULL;

    int nResult = PyArg_ParseTuple(args, "OO", &pyDevice, &pyTagsList); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_UpdateTagsData(Device*, tagList)ʱ, �������벻��");
        return Py_BuildValue("i", -1);
    }


    if (pyDevice == NULL || pyTagsList == NULL)
        return Py_BuildValue("i", -2);

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
    if (!pChannel)
        return Py_BuildValue("i", -3);

    int nRet = Py_UpdateTagsData(pChannel, pyTagsList);
    return Py_BuildValue("i", nRet);
//        return Py_BuildValue("i", -1);
}

// ���ط��͵��ֽڸ���
// �����pyBuffer������python����struct.pack�Ķ���
//PyObject *pyDevice, PyObject *pyBuffer, PyObject *pyTimeOutMS)
static PyObject * PyDrv_Send(PyObject *self, PyObject *args) //PyObject *pyDevice, PyObject *pyBuffer, PyObject *pyTimeOutMS)
{
    PyObject *pyDevice = NULL;
    const char *szBuffer = NULL;
    int nBuffLen = 0;
    int nTimeOutMS = 0;

    int nResult = PyArg_ParseTuple(args, "Os#|i", &pyDevice, &szBuffer, &nBuffLen, &nTimeOutMS); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_SendToDevice(Device*,szBuffer, nBuffLen, nTimeOutMS)ʱ, �������벻��");
        return PyInt_FromLong(-1);
    }

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
    if (!pChannel)
        return Py_BuildValue("i", -2);
    int nSent = Drv_Send(pChannel->m_pOutDeviceInfo, szBuffer, nBuffLen, nTimeOutMS);
    //int nSent = pChannel->SendToDevice(szBuffer, nBuffLen, nTimeOutMS);
    //Py_DECREF(szBuffer);

    PyObject *pyResult = Py_BuildValue("i", nSent);
    return pyResult;
//    return Py_BuildValue("i", -1);
}

// �����յ����ֽڸ���.���� pDevice, nMaxRecvBuffLen, nTimeOutMS 
// ������pyBuffer������python�ж�Ԫ�飬��һ��Ԫ���ǣ�����ֵ���ڶ���Ԫ���ǻ���������Ҫ��struct.unpack������������len()����ȡ�����ȣ�
static PyObject * PyDrv_Recv(PyObject *self, PyObject *args)
{
    PyObject *pyDevice = NULL;
    int nMaxRecvBuffLen = 0;
    int nTimeOutMS = 1000; // default 1000ms

    char szBuff[32] = { 0 };

    PyObject *pyResult = PyTuple_New(2);
    int nResult = PyArg_ParseTuple(args, "Oi|i", &pyDevice, &nMaxRecvBuffLen, &nTimeOutMS); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_RecvFromDevice(Device*, nTimeOutMS)ʱ, �������벻��");
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -1));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
        return pyResult;
    }

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
    if (!pChannel)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸��������ʧ��(��ʱ %d ), �����豸���Ϊ��", nTimeOutMS);
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -2));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
        return pyResult;
    }
	
    if (nMaxRecvBuffLen <= 0)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸��������ʧ��(��ʱ %d ), ���������ջ��������� <= 0", nTimeOutMS);
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -3));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
        return pyResult;
    }
    if (nTimeOutMS < 0)
        nTimeOutMS = 0;

    char *szRecvBuff = new char[nMaxRecvBuffLen];
    int nRecv = Drv_Recv(pChannel->m_pOutDeviceInfo, szRecvBuff, nMaxRecvBuffLen, nTimeOutMS);
    // int nRecv = pChannel->RecvFromDevice(szRecvBuff, nMaxRecvBuffLen, nTimeOutMS);
    if (nRecv > 0)
    {
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", 0));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szRecvBuff, nRecv));
    }
    else
    {
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -5));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
    }
    delete[]szRecvBuff;
    return pyResult;
//    return Py_BuildValue("i", -1);
}

// �㻹����ָ����ѡ�Ĳ�����ֻ��Ҫͨ���ڸ�ʽ�ַ����а���һ��"|"�ַ�����
// (pydevice,periodMS,param)
static PyObject* PyDrv_CreateTimer(PyObject *self, PyObject *args)//PyObject *pyTimerParam)
{
    PyObject *pyDevice = NULL;
    int nPeriodMS = 0;
    PyObject *pyTimerParam = NULL;

    int nResult = PyArg_ParseTuple(args, "Oi|O", &pyDevice, &nPeriodMS, &pyTimerParam); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "CreateTimer(pkDevice,nPeriodMS,timerParam[optional])ʱ, �������벻��, return���");
        return PyInt_FromLong(0);
    }

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
	if (!pChannel)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_CreateTimer, pDevice(_InternalRef)==0 invalid! must start with xxxxdriver.exe!!");
		return Py_BuildValue("i", 0);
	}

    CUserTimer *pTimer = Py_CreateAndStartTimer(pChannel, nPeriodMS, pyTimerParam);
    return Py_BuildValue("l", (unsigned long)pTimer);
//    return Py_BuildValue("i", 0);
}

// �����pyTimerInfo��ʵû�������棬ֻ������������������,pyTimerParam=(period,param),����param����Ҫ���������ģ��Ա���OnTimer�д���
static PyObject * PyDrv_DestroyTimer(PyObject *self, PyObject *args)//PyObject *pyTimerParam)
{
    PyObject *pyDevice = NULL;
    int nTimerPtr = 0;

    int nResult = PyArg_ParseTuple(args, "Oi", &pyDevice, &nTimerPtr); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "DestroyTimer(pkDevice, timerHandle)ʱ, �������벻��");
        return PyInt_FromLong(-1);
    }

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
    if (!pChannel)
        return Py_BuildValue("i", -2);

    CUserTimer *pUserTimer = NULL;
    memcpy(&pUserTimer, &nTimerPtr, sizeof(CUserTimer *));
    int nRet = Drv_DestroyTimer(pChannel->m_pOutDeviceInfo, pUserTimer);
    return Py_BuildValue("i", nRet);
//    return Py_BuildValue("i", 0);
}


static PyObject * PyDrv_Test(PyObject *self, PyObject *args)
{
	printf("PyDrv_Test\n");
	return Py_BuildValue("i", -1);
}

static PyObject * PyDrv_LogMessage(PyObject *self, PyObject *args)
{
    int nLogLevel = PK_LOGLEVEL_INFO;
    const char * szBuffer = NULL;
    if (!PyArg_ParseTuple(args, "is", &nLogLevel, &szBuffer))
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "LogMessage ParseTuple return false,format. LogMessage(loglevel, logmessage)");
        //Py_DECREF(szBuffer);
        return Py_BuildValue("i", -1);
    }
    Drv_LogMessage(nLogLevel, "%s", szBuffer);
	return Py_BuildValue("i", 0);
//    return Py_BuildValue("i", 0);
}

// ����tag��ַ�ҵ�tag����
static PyObject * PyDrv_GetTagsByAddr(PyObject *self, PyObject *args)
{
    const char * szTagAddr = NULL;
    PyObject *pyDevice = NULL;
    if (!PyArg_ParseTuple(args, "Os", &pyDevice, &szTagAddr))
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "GetTagsByAddr(pydevice, szTagAddress), ���������������! return list");
        //Py_DECREF(szTagAddr);
        PyObject *pyResult = PyList_New(0);
        return pyResult;
    }

    CDevice *pDevie = PyDevice2CDevice(pyDevice);
    if (!pDevie)
    {
        //Py_DECREF(szTagAddr);
        PyObject *pyResult = PyList_New(0);
        return pyResult;
    }

    vector<PKTAG *> vecTag;
	PKTAG *ppTags[100000]; // �����ظ��ܶ�����
	int nTagNum = Drv_GetTagsByAddr(pDevie->m_pOutDeviceInfo, szTagAddr, ppTags, 100000); // ����ֵ���ҵ���tag�����
	if (nTagNum <= 0)
    {
        //Py_DECREF(szTagAddr);
        PyObject *pyResult = PyList_New(0);
        return pyResult;
    }

	PyObject *pyResult = PyList_New(nTagNum);
	for (int i = 0; i < nTagNum; i++)
    {
		PKTAG *pTag = ppTags[i];
        PyObject *pyTag = (PyObject *)pTag->pData1;
        pyTag->ob_refcnt++; // -----------���ô��������1���������python�б��ͷŵ����Ӷ����º���GetItemString���쳣
        PyList_SetItem(pyResult, i, pyTag);
    }

    return pyResult;
//    return PyList_New(0);
}

// ����Ԫ���е�tag���ֵΪ �ַ�����ֵ
// pkdevice, pktaglist, tagvalue(string)
static PyObject * PyDrv_SetTagsValue(PyObject *self, PyObject *args)
{
    const char *szTagValue = NULL;
    PyObject *pyDevice = NULL;
    PyObject *pyTagList = NULL;
    if (!PyArg_ParseTuple(args, "OOs", &pyDevice, &pyTagList, &szTagValue))
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "SetTagsValue(pyDevice, tagList, szTagValue), ��������Ƿ���ӦΪ������ʽ!");
        PyObject *pyResult = Py_BuildValue("i",0);
        return pyResult;
    }

    CDevice *pDevie = PyDevice2CDevice(pyDevice);
    if (!pDevie)
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "�豸:%s, PyDrv_SetTagsValue ������豸����δ��ת��, ֵ:%s",szTagValue);
        return Py_BuildValue("i", -1);
    }

    int nTagNum = PyList_Size(pyTagList);
    if (nTagNum <= 0)
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "�豸:%s, PyDrv_SetTagsValue �����tag�б����Ϊ0,û�����ݿ��Ը���, ֵ:%s", pDevie->m_strName.c_str(), szTagValue);
        return Py_BuildValue("i", -2);
    }

    vector<PKTAG *> vecTag;
    int i = 0;
    for (i = 0; i < nTagNum; i++)
    {
        PyObject *pyTag = PyList_GetItem(pyTagList, i);
        PKTAG *pTag = Py_PyTag2CTag(pyTag);
        if (!pTag)
        {
            Drv_LogMessage(PK_LOGLEVEL_ERROR, "����PyTagû���ҵ�C��DRVTAG");
            continue;
        }
        Drv_SetTagData_Text(pTag, szTagValue, 0, 0, 0);
    }

    return Py_BuildValue("i", 0);
}

/////////////////-----------------------------
static PyMethodDef AllMyMethods[] = {
	{ "PyDrv_Test", (PyCFunction)PyDrv_Test, METH_VARARGS },
	{ "LogMessage", (PyCFunction)PyDrv_LogMessage, METH_VARARGS },
	{ "CreateTimer", (PyCFunction)PyDrv_CreateTimer, METH_VARARGS },
	{ "DestroyTimer", (PyCFunction)PyDrv_DestroyTimer, METH_VARARGS },
	{ "Send", (PyCFunction)PyDrv_Send, METH_VARARGS },
	{ "Recv", (PyCFunction)PyDrv_Recv, METH_VARARGS },
	{ "GetTagsByAddr", (PyCFunction)PyDrv_GetTagsByAddr, METH_VARARGS },
	{ "SetTagsValue", (PyCFunction)PyDrv_SetTagsValue, METH_VARARGS },
	{ "UpdateTagsData", (PyCFunction)PyDrv_UpdateTagsData, METH_VARARGS },
    { NULL, NULL, 0, NULL}
};

//extern "C"              //���ӻᵼ���Ҳ���initexample
PyMODINIT_FUNC initpydriver()
{
    //Drv_LogMessage(PK_LOGLEVEL_INFO, "begin to init pydriver");
	PyObject *m, *d;
	m = Py_InitModule("pydriver", AllMyMethods);
	d = PyModule_GetDict(m);
}
