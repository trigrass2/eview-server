/**************************************************************
 *  Filename:    RMSvcMain.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RMService Main 
 *    Seperate it from RMService.cpp For UT.
 *
 *  @author:     chenzhiquan
 *  @version     05/27/2008  chenzhiquan  Initial Version
**************************************************************/

#include "RMService.h"
#include <ace/ace_wchar.h>
#include "common/pkProcessController.h"

// 进程启停控制器
//extern CCVProcessController g_cvProcController;


/**
 *  Main Function.
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
/*int main(int argc, ACE_TCHAR* args[])
{
	// assure only one process is running
	if(g_cvProcController.SetProcessAliveFlag() != ICV_SUCCESS)
		return -1;
	// 是否接收键盘输入
	g_cvProcController.SetConsoleParam(argc,args);

	long nErr = ICV_SUCCESS;
	nErr = RM_SERVICE::instance()->InitService(argc, args);
	if (nErr != ICV_SUCCESS)
	{ // Initialize Failed.
		LH_LOG((LOG_LEVEL_CRITICAL, ACE_TEXT("Error Initializing RMService with Error Code %d"), nErr));
		return nErr;
	}

	RM_SERVICE::instance()->RunService();

	RM_SERVICE::instance()->FiniService();

	return nErr;
}
//*/


