
#include "python/Python.h"
#include <string>
#include "pkdriver/pkdrvcmn.h"
#include "../pkdrvcmn/Device.h"

#ifndef PK_SUCCESS
#define PK_SUCCESS			0
#endif	// PK_SUCCESS

//////////下面是python驱动所需要导出的函数/////////////////////////////////////////////////
// 更新数据块的数据、状态、时间戳, 更新到Shm中去

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

// 将点的值更新到pdb中, 传入的是tags的数组（list），每个tags下多了value、quality、time三个字段
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
            Drv_LogMessage(PK_LOGLEVEL_ERROR, "根据PyTag没有找到C中DRVTAG");
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
    memcpy(&pChannel, &nPtrValue, sizeof(CDevice *)); // 指针总是8个字节
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
	Py_INCREF(pyTimerParam); // 避免被释放
	Py_INCREF(pBlockTimer->m_pyTimerParam); // 避免被释放
    return pBlockTimer;
}

static PyObject *PyDrv_UpdateTagsData(PyObject *self, PyObject *args) // (PyObject *pyDevice, PyObject *pyTagsList)
{
    PyObject *pyDevice = NULL;
    PyObject *pyTagsList = NULL;

    int nResult = PyArg_ParseTuple(args, "OO", &pyDevice, &pyTagsList); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_UpdateTagsData(Device*, tagList)时, 参数传入不对");
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

// 返回发送的字节个数
// 传入的pyBuffer必须是python中用struct.pack的对象
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
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_SendToDevice(Device*,szBuffer, nBuffLen, nTimeOutMS)时, 参数传入不对");
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

// 返回收到的字节个数.传入 pDevice, nMaxRecvBuffLen, nTimeOutMS 
// 传出的pyBuffer必须是python中二元组，第一个元素是：返回值，第二个元素是缓冲区（需要用struct.unpack解析，可以用len()方法取出长度）
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
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "PyDrv_RecvFromDevice(Device*, nTimeOutMS)时, 参数传入不对");
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -1));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
        return pyResult;
    }

    CDevice *pChannel = PyDevice2CDevice(pyDevice);
    if (!pChannel)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "向设备接收数据失败(超时 %d ), 传入设备句柄为空", nTimeOutMS);
        PyTuple_SetItem(pyResult, 0, Py_BuildValue("i", -2));
        PyTuple_SetItem(pyResult, 1, Py_BuildValue("s#", szBuff, 1));
        return pyResult;
    }
	
    if (nMaxRecvBuffLen <= 0)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "向设备接收数据失败(超时 %d ), 传入最大接收缓冲区长度 <= 0", nTimeOutMS);
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

// 你还可以指定可选的参数，只需要通过在格式字符串中包含一个"|"字符即可
// (pydevice,periodMS,param)
static PyObject* PyDrv_CreateTimer(PyObject *self, PyObject *args)//PyObject *pyTimerParam)
{
    PyObject *pyDevice = NULL;
    int nPeriodMS = 0;
    PyObject *pyTimerParam = NULL;

    int nResult = PyArg_ParseTuple(args, "Oi|O", &pyDevice, &nPeriodMS, &pyTimerParam); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "CreateTimer(pkDevice,nPeriodMS,timerParam[optional])时, 参数传入不对, return句柄");
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

// 传入的pyTimerInfo其实没有做保存，只是输入而不是输出参数,pyTimerParam=(period,param),其中param是需要保存起来的，以便在OnTimer中传入
static PyObject * PyDrv_DestroyTimer(PyObject *self, PyObject *args)//PyObject *pyTimerParam)
{
    PyObject *pyDevice = NULL;
    int nTimerPtr = 0;

    int nResult = PyArg_ParseTuple(args, "Oi", &pyDevice, &nTimerPtr); // oi|o
    if (!nResult)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "DestroyTimer(pkDevice, timerHandle)时, 参数传入不对");
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

// 根据tag地址找到tag数组
static PyObject * PyDrv_GetTagsByAddr(PyObject *self, PyObject *args)
{
    const char * szTagAddr = NULL;
    PyObject *pyDevice = NULL;
    if (!PyArg_ParseTuple(args, "Os", &pyDevice, &szTagAddr))
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "GetTagsByAddr(pydevice, szTagAddress), 传入参数解析错误! return list");
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
	PKTAG *ppTags[100000]; // 考虑重复很多的情况
	int nTagNum = Drv_GetTagsByAddr(pDevie->m_pOutDeviceInfo, szTagAddr, ppTags, 100000); // 返回值是找到的tag点个数
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
        pyTag->ob_refcnt++; // -----------引用次数必须加1，否则会在python中被释放掉，从而导致后续GetItemString的异常
        PyList_SetItem(pyResult, i, pyTag);
    }

    return pyResult;
//    return PyList_New(0);
}

// 更新元组中的tag点的值为 字符串的值
// pkdevice, pktaglist, tagvalue(string)
static PyObject * PyDrv_SetTagsValue(PyObject *self, PyObject *args)
{
    const char *szTagValue = NULL;
    PyObject *pyDevice = NULL;
    PyObject *pyTagList = NULL;
    if (!PyArg_ParseTuple(args, "OOs", &pyDevice, &pyTagList, &szTagValue))
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "SetTagsValue(pyDevice, tagList, szTagValue), 传入参数非法，应为上述格式!");
        PyObject *pyResult = Py_BuildValue("i",0);
        return pyResult;
    }

    CDevice *pDevie = PyDevice2CDevice(pyDevice);
    if (!pDevie)
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "设备:%s, PyDrv_SetTagsValue 传入的设备对象未能转换, 值:%s",szTagValue);
        return Py_BuildValue("i", -1);
    }

    int nTagNum = PyList_Size(pyTagList);
    if (nTagNum <= 0)
    {
        //Py_DECREF(szTagValue);
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "设备:%s, PyDrv_SetTagsValue 传入的tag列表个数为0,没有数据可以更新, 值:%s", pDevie->m_strName.c_str(), szTagValue);
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
            Drv_LogMessage(PK_LOGLEVEL_ERROR, "根据PyTag没有找到C中DRVTAG");
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

//extern "C"              //不加会导致找不到initexample
PyMODINIT_FUNC initpydriver()
{
    //Drv_LogMessage(PK_LOGLEVEL_INFO, "begin to init pydriver");
	PyObject *m, *d;
	m = Py_InitModule("pydriver", AllMyMethods);
	d = PyModule_GetDict(m);
}
