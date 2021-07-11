/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 数据块信息实体类.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _DATABLOCK_H_
#define _DATABLOCK_H_

#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <list>
#include <set>
#include "drvframework.h"
#include "common/CommHelper.h"
#include "PythonHelper.h"
class CDevice;

class CUserTimer: public ACE_Event_Handler
{
public:
	CUserTimer(CDevice *pDevice);
	virtual ~CUserTimer();
	CUserTimer &operator=(CUserTimer &theDataBlockTimer);
	void StartTimer();
	void StopTimer();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
	//bool operator==( CUserTimer &theDataBlock );
	//int PyOnTimer();
public:
	CDevice*	m_pDevice;				// 所属设备指针
	PKTIMER		m_timerInfo;			// 用于c驱动使用的定时器数据
	PyObject*	m_pyTimerParam;			// 用于python驱动使用的定时器数据
};

#endif  // _DATABLOCK_H_
