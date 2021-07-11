#pragma once

#include "python/Python.h"

class CDevice;
class CUserTimer;
class CDriver;

extern PyObject *	g_pyModule;

extern void Py_CDeviceConstructor(CDevice *pDevice);
extern int Py_InitDevice(CDevice *pDevice);
extern int Py_UnInitDevice(CDevice *pDevice);
extern void Py_CopyDeviceInfo2OutDevice(CDevice *pDevice);
extern int Py_UpdateTagsData(CDevice *pDevice, PyObject * pyTagsList);
extern int Py_PyCreateAndStartTimer(CDevice *pDevice, int nPeroid, PyObject *pyTimerParam);
extern PKTAG *Py_Tag2CTag(PyObject *pyTag);
extern int Py_OnControl(CDevice *pDevice, PKTAG *pTag, string & strTagValue);

//////////////////////////
extern void Py_CDriverConstructor(CDriver *pDriver);
extern int Py_InitDriver(CDriver *pDriver);
extern int Py_UnInitDriver(CDriver *pDriver);

////////////////////////////////-----------
extern int Py_InitPython();
extern void Py_UnInitPython();
extern bool IsPythonDriver();
extern int Py_OnTimer(CUserTimer *pUserTimer);
