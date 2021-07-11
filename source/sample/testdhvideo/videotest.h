#ifndef VIDEOTEST_H
#define VIDEOTEST_H

#include <QtWidgets/QWidget>
#include "ui_videotest.h"
//#include<time.h>
//#include "DVRFunc.h"
typedef long(*pFunInitSDK)();
typedef long(*pFunVideoLogin)(char*, char*, char*, char*, char*, long&);
typedef long(*pFunVideoLogout)(long);
typedef long(*pFunVideoPlayVideo)(long, char*, void*, long, long&);
typedef long(*pFunVideoPlayBackbyTime)(long, char*, time_t, time_t, void*, long&);
typedef long(*pFunVideoBindPlayPort)(long);
typedef long(*pFunVideoPlayStream)(long, char*,void* );
typedef long(*pFunVideoQueryDiskInfo)(long, long&,long& );

class videoTest : public QWidget
{
	Q_OBJECT

public:
	videoTest(QWidget *parent = 0);
    void initSlot();
    void loginVideo();
    void initFunc();
	bool bInit;
	~videoTest();
public:
    pFunInitSDK VideoInitSDK,VideoStopStream;
    pFunVideoLogout VideoLogout, VideoStopPlayVideo, VideoStopPlayBack, VideoQueryEvent;
	pFunVideoLogin VideoLogin;
	pFunVideoPlayVideo VideoPlayVideo;
	pFunVideoPlayBackbyTime VideoPlayBackbyTime,VideoPlayBackStreambyTime;
    pFunVideoBindPlayPort VideoBindPlayPort, VideoStopPlayBackStream;
    pFunVideoPlayStream VideoPlayStream;
    pFunVideoQueryDiskInfo VideoQueryDiskInfo;

public:
    long nLoginID;
    long nPlayID;

private:
	Ui::videoTestClass ui;
public slots:
	void on_PlayBtn_clicked();
	void on_StopBtn_clicked();
	void on_PlaybackBtn_clicked();
	void on_queryEventBtn_clicked();
	void on_stopPbBtn_clicked();
    void on_loginoutBtn_clicked();

//private slots:
   // void on_playLbl_linkActivated(const QString &link);
};

#endif // VIDEOTEST_H
