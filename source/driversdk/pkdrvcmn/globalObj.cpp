#include <ace/Process_Mutex.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS.h>
#include <ace/DLL.h>
#include <iostream>
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "shmqueue/ProcessQueue.h"
#include "pkprocesscontroller/pkprocesscontroller.h"

using namespace std;

#define PK_LANG_UNKNOWN				0			// 既不是cpp也不是python语言的驱动
#define PK_LANG_CPP					1			// cpp语言的驱动
#define PK_LANG_PYTHON					2			// python语言的驱动

CPKLog g_logger;

CPKProcessController *	g_pProcController = NULL;	// 进程控制器辅助
CProcessQueue *			g_pQueFromProcMgr = NULL;				// 和进程管理器进行通信的共享内存队列

bool		g_bDriverAlive = true;
ACE_DLL		g_hDrvDll;	// 驱动动态库
string		g_strDrvName = "";	// 不带后缀名的驱动
string		g_strDrvExtName = ""; // 后缀名，如 .pyo,.dll, .so, .pyc
string		g_strDrvPath = ""; // 得到没有扩展名的路径，如 /gateway/bin/drivers/modbus,不包含驱动.dll
int			g_nDrvLang = PK_LANG_CPP; // 驱动语言
