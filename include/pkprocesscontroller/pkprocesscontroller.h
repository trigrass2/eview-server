/**************************************************************
 *  Filename:    pkcomm.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
**************************************************************/
#ifndef _PK_PROCESSCONTROLLER_H_
#define _PK_PROCESSCONTROLLER_H_

class CPKProcessControllerImpl;
class CPKProcessController
{
public:
	CPKProcessController(char *szProcessName);
	~CPKProcessController();

public:
	bool IsProcessQuitSignaled();
	long SetProcessAliveFlag();
	bool SetConsoleParam(int argc, char* argvs[]);	// 是否接收键盘输入
	bool KbHit ();

private:
	CPKProcessControllerImpl*	m_pImpl;
};
#endif // _PK_PROCESSCONTROLLER_H_
