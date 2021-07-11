// XLVideoCommServer.cpp : Defines the entry point for the console application.
//

//����Ӧ��ͷ�ļ�
#include "ace/ACE.h"
#include "SystemConfig.h"
#include "common/pkProcessController.h"
#include "MainTask.h"
#include "common/pklog.h"

//����ȫ�����ö���
CCVProcessController g_cvProcController("forwardservice");

long InitGlobals();
long ExitGlobals();
extern CPKLog PKLog;

void PrintCmdTips()
{
	printf("+=============================================+\n");
	printf("|  <<��ӭʹ������ת������>>                 |\n");
	printf("|  ������������ָ���������                   |\n");
	printf("|  ----q/Q:�˳�                               |\n");
	printf("+=============================================+\n");
}

void InteractiveLoop()
 {
	bool bAlive = true;
	while(bAlive)
	{
		if(g_cvProcController.CV_KbHit())
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
		
		if(g_cvProcController.IsProcessQuitSignaled())
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

	PKLog.SetLogFileName("forwardservice");
	//��ȡ�����ļ�
	if (!SYSTEM_CONFIG->ReadConfig())
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ�ʧ��");
		WaitToExit();
		return -1;
	}
	
	if(SYSTEM_CONFIG->m_mapModbusPortToForwardInfo.size() <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "�������ļ���δ��ȡ����Ч��ת������(����Ϊ0)");
		WaitToExit();
		return -1;
	}

	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ��ɹ�");

	//��ʼ��
	ACE::init();

	// �Ƿ���ռ�������
	g_cvProcController.SetConsoleParam(argc, argv);

	InitGlobals();

	int nListenPort = SYSTEM_CONFIG->m_mapModbusPortToForwardInfo.begin()->first;
	MAIN_TASK->SetListenPort(nListenPort);


	MAIN_TASK->Start();

	//�������߳��˻�����
	InteractiveLoop();

	//�̰߳�ȫ�˳�, send stop signal and wait for exit....
 	MAIN_TASK->Stop();

	ExitGlobals();
	ACE::fini();
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