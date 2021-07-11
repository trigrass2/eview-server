/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݿ���Ϣʵ����.
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
	CDevice*	m_pDevice;				// �����豸ָ��
	PKTIMER		m_timerInfo;			// ����c����ʹ�õĶ�ʱ������
	PyObject*	m_pyTimerParam;			// ����python����ʹ�õĶ�ʱ������
};

#endif  // _DATABLOCK_H_
