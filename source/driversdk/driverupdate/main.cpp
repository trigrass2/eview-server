// XLVideoCommServer.cpp : Defines the entry point for the console application.
//

//����Ӧ��ͷ�ļ�
#include "ace/ACE.h"
#include "SystemConfig.h"
#include "common/pkProcessController.h"
#include "MainTask.h"
#include "common/pklog.h"

//����ȫ�����ö���
CPKProcessController g_pkProcController("driverupdate");

long InitGlobals();
long ExitGlobals();
extern CPKLog PKLog;

void PrintCmdTips()
{
	printf("+=============================================+\n");
	printf("|  <<��ӭʹ�����ظ�����ϳ���>>                 |\n");
	printf("|  ������������ָ���������                   |\n");
	printf("|  ----q/Q:�˳�                               |\n");
	printf("+=============================================+\n");
}

void InteractiveLoop()
 {
	bool bAlive = true;
	while(bAlive)
	{
		if(g_pkProcController.CV_KbHit())
		{
			char chCmd = tolower(getchar());

			switch(chCmd)
			{
			case 'q':
				{
					bAlive = false;
				}
				break;

			default:
				break;
			}
		}
		
		if(g_pkProcController.IsProcessQuitSignaled())
		{
			bAlive = false;
			break;
		}

		ACE_Time_Value tvTimeOut(1,0);
		ACE_OS::sleep(tvTimeOut);
	}

}


void WaitToExit()
{
	printf("ready to exit, press any key to continue.....");
	getchar();
}

//���������
int ACE_TMAIN(int argc, char* argv[])
{
	PrintCmdTips();	

	PKLog.SetLogFileName("driverupdate");
	//��ȡ�����ļ�
	if (!SYSTEM_CONFIG->LoadConfig())
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ�ʧ��");
		WaitToExit();
		return -1;
	}
	
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡ�����ļ��ɹ�");

	//��ʼ��
	//ACE::init();

	// �Ƿ���ռ�������
	g_pkProcController.SetConsoleParam(argc, argv);

	InitGlobals();

	MAIN_TASK->Start();

	//�������߳��˻�����
	InteractiveLoop();

	//�̰߳�ȫ�˳�, send stop signal and wait for exit....
 	MAIN_TASK->Stop();

	ExitGlobals();
	//ACE::fini();
	return 0;
}

long InitGlobals()
{

	return 0;
}

long ExitGlobals()
{
	return 0;
}