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

#define PK_LANG_UNKNOWN				0			// �Ȳ���cppҲ����python���Ե�����
#define PK_LANG_CPP					1			// cpp���Ե�����
#define PK_LANG_PYTHON					2			// python���Ե�����

CPKLog g_logger;

CPKProcessController *	g_pProcController = NULL;	// ���̿���������
CProcessQueue *			g_pQueFromProcMgr = NULL;				// �ͽ��̹���������ͨ�ŵĹ����ڴ����

bool		g_bDriverAlive = true;
ACE_DLL		g_hDrvDll;	// ������̬��
string		g_strDrvName = "";	// ������׺��������
string		g_strDrvExtName = ""; // ��׺������ .pyo,.dll, .so, .pyc
string		g_strDrvPath = ""; // �õ�û����չ����·������ /gateway/bin/drivers/modbus,����������.dll
int			g_nDrvLang = PK_LANG_CPP; // ��������
