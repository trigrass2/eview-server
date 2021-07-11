#include "videotest.h"
//#include <Windows.h>
#include <time.h>
#include <QLibrary>
#include <QtWidgets/QMessageBox>
//#include <QtWidgets/QTableWidget>
#include <dlfcn.h>

#include <QDebug>
videoTest::videoTest(QWidget *parent)
    :QWidget(parent)
{
    ui.setupUi(this);
    ui.StartTimeEdit->setParent(this);
    ui.EndTimeEdit->setParent(this);
    bInit = false;
   initSlot();
   initFunc();
   loginVideo();


}
void videoTest::initSlot()
{
    QObject::connect(ui.playBtn,SIGNAL(clicked()),this,SLOT(on_PlayBtn_clicked()));
    QObject::connect(ui.stopBtn,SIGNAL(clicked()),this,SLOT(on_StopBtn_clicked()));
    QObject::connect(ui.playbackBtn,SIGNAL(clicked()),this,SLOT(on_PlaybackBtn_clicked()));
    QObject::connect(ui.queryEventBtn,SIGNAL(clicked()),this,SLOT(on_queryEventBtn_clicked()));
    QObject::connect(ui.stopPbBtn,SIGNAL(clicked()),this,SLOT(on_stopPbBtn_clicked()));
    QObject::connect(ui.loginout,SIGNAL(clicked()),this,SLOT(on_loginoutBtn_clicked()));

}

void videoTest::initFunc()
{
   // HINSTANCE hDlllnst = LoadLibrary(TEXT("DVRDH.dll"));
   void* hDlllnst = dlopen("/home/peak//eview-server/bin/libdvrdh.so",RTLD_NOW);
   if(!hDlllnst)
   {
       QMessageBox::information(this,"ttt",QString(dlerror()));
       //qDebug()<<"error"<<endl;
       //printf("----------------%s",dlerror());
       return;
   }
/* QLibrary  *myLib=NULL;
  myLib=new QLibrary("/home/peak/Documents/videoTest/so/libDVRDH.so");
  myLib->load();
  if(!myLib->isLoaded()){
      QMessageBox::information(this,"ttt","error");
  }
  //myLib->resolve("VideoInitSDK");*/
    //void* Error = dlerror();
   // if (Error)
   // {
     //   QMessageBox::information(this,"ttt","error");
       // dlclose(hDlllnst);
      //return;
   //}

   VideoInitSDK = (pFunInitSDK)dlsym(hDlllnst, "VideoInitSDK");	//��ʼ��SDK
    VideoLogin = (pFunVideoLogin)dlsym(hDlllnst, "VideoLogin");	//��¼
    VideoLogout = (pFunVideoLogout)dlsym(hDlllnst, "VideoLogout");	//�ǳ�
    VideoPlayVideo = (pFunVideoPlayVideo)dlsym(hDlllnst, "VideoPlayVideo");	//����ʵʱ
    VideoStopPlayVideo = (pFunVideoLogout)dlsym(hDlllnst, "VideoStopPlayVideo");	//ֹͣ����
    VideoPlayBackbyTime = (pFunVideoPlayBackbyTime)dlsym(hDlllnst, "VideoPlayBackbyTime");	//����ʱ�䲥����ʷ
    VideoStopPlayBack = (pFunVideoLogout)dlsym(hDlllnst, "VideoStopPlayBack");			//ֹͣ��ʷ����
    VideoPlayStream = (pFunVideoPlayStream)dlsym(hDlllnst, "VideoPlayStream");			//������
    VideoQueryEvent = (pFunVideoLogout)dlsym(hDlllnst, "VideoQueryEvent");				//��ѯ����
    VideoBindPlayPort = (pFunVideoBindPlayPort)dlsym(hDlllnst, "VideoBindPlayPort");	//�󶨲��Ŷ˿�
    VideoStopStream = (pFunInitSDK)dlsym(hDlllnst, "VideoStopStream");	//ֹͣ��
    VideoPlayBackStreambyTime = (pFunVideoPlayBackbyTime)dlsym(hDlllnst, "VideoPlayBackStreambyTime");	//���ط�
    VideoStopPlayBackStream = (pFunVideoLogout)dlsym(hDlllnst, "VideoStopPlayBackStream");	//ֹͣ���ط�
    VideoQueryDiskInfo = (pFunVideoQueryDiskInfo)dlsym(hDlllnst, "VideoQueryDiskInfo");	// HDDDISK




}

void videoTest::loginVideo()
{
	if (!bInit)
	{
        long lRet = VideoInitSDK();
		if (lRet < 0)
		{
			return;
		}
		else
		{
			bInit = true;
		}
	}
    long iRet= VideoLogin("192.168.10.111","37777","admin","admin","",nLoginID );
    if(!iRet)
    {
        printf("ok");
    }

}
videoTest::~videoTest()
{

}

void videoTest::on_PlayBtn_clicked()
{

    long iRet= VideoBindPlayPort(nLoginID);
    char* szChannel = new char[10];
    szChannel       ="1";
  //  ui.playLbl->update();

    iRet = VideoPlayStream(nLoginID,szChannel, (void*)ui.playLbl->winId());
  // VideoPlayVideo(nLoginID, szChannel, (HWND)ui.playLbl->winId(), 0, nPlayID);
}

void videoTest::on_StopBtn_clicked()
{
    long iRet= VideoStopStream();
}

void videoTest::on_PlaybackBtn_clicked()
{
    long iRet= VideoBindPlayPort(nLoginID);
    char* szChannel = new char[10];
    szChannel       = "1";
    struct tm t;
    time_t tStart;
    t.tm_year=ui.StartTimeEdit->date().year() - 1900;
    t.tm_mon=ui.StartTimeEdit->date().month()-1;/*�·���0-11��ʾ��������5��ʾ����*/
    t.tm_mday=ui.StartTimeEdit->date().day() ;
    t.tm_hour=ui.StartTimeEdit->time().hour()+8;
    t.tm_min=ui.StartTimeEdit->time().minute()-5;
    t.tm_sec=ui.StartTimeEdit->time().msec();
    tStart=mktime(&t);

    struct tm tt;
    time_t tEnd;
    tt.tm_year=ui.EndTimeEdit->date().year() -1900;
    tt.tm_mon=ui.EndTimeEdit->date().month()-1;/*�·���0-11��ʾ��������5��ʾ����*/
    tt.tm_mday=ui.EndTimeEdit->date().day();
    tt.tm_hour=ui.EndTimeEdit->time().hour()+8;
    tt.tm_min=ui.EndTimeEdit->time().minute()-5;
    tt.tm_sec=ui.EndTimeEdit->time().msec();
    tEnd=mktime(&tt);

	nPlayID = 0;
   // VideoPlayBackbyTime(nLoginID, szChannel, tmStart, tmEnd,  (HWND)ui.playLbl->winId(), nPlayID);
    iRet = VideoPlayBackStreambyTime(nLoginID, szChannel, tStart, tEnd,  (void*)ui.playLbl->winId(), nPlayID);
}

void videoTest::on_queryEventBtn_clicked()
{
    long iAllTotalSpace = 0;
    long iAllFreeSpace = 0;
    VideoQueryDiskInfo(nLoginID, iAllTotalSpace, iAllFreeSpace);
    ui.statusLbl->setText("ALL:"+QString::number(iAllTotalSpace)+"\nFree:"+QString::number(iAllFreeSpace));

}

void videoTest::on_stopPbBtn_clicked()
{
    long iRet= VideoStopPlayBackStream(nPlayID);
}

void videoTest::on_loginoutBtn_clicked()
{
    VideoLogout(nLoginID);
    this->close();
}
