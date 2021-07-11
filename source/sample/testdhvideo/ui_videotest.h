/********************************************************************************
** Form generated from reading UI file 'videotest.ui'
**
** Created by: Qt User Interface Compiler version 5.1.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIDEOTEST_H
#define UI_VIDEOTEST_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QDateTimeEdit>

QT_BEGIN_NAMESPACE

class Ui_videoTestClass
{
public:
    QFrame *playLbl;
    QPushButton *playBtn;
    QPushButton *playbackBtn;
    QPushButton *stopBtn;
    QPushButton *queryEventBtn;
    QLabel *statusLbl;
    QPushButton *stopPbBtn;
    QPushButton *loginout;
    QDateTimeEdit *StartTimeEdit;
    QDateTimeEdit *EndTimeEdit;

    void setupUi(QWidget *videoTestClass)
    {
        if (videoTestClass->objectName().isEmpty())
            videoTestClass->setObjectName(QStringLiteral("videoTestClass"));
        videoTestClass->resize(600, 400);

        playLbl = new QFrame(videoTestClass);
        playLbl->setObjectName(QStringLiteral("playLbl"));
        playLbl->setGeometry(QRect(100, 20, 431, 221));
        playBtn = new QPushButton(videoTestClass);
        playBtn->setObjectName(QStringLiteral("playBtn"));
        playBtn->setGeometry(QRect(60, 290, 75, 23));
        playbackBtn = new QPushButton(videoTestClass);
        playbackBtn->setObjectName(QStringLiteral("playbackBtn"));
        playbackBtn->setGeometry(QRect(280, 290, 75, 23));
        stopBtn = new QPushButton(videoTestClass);
        stopBtn->setObjectName(QStringLiteral("stopBtn"));
        stopBtn->setGeometry(QRect(160, 290, 75, 23));
        queryEventBtn = new QPushButton(videoTestClass);
        queryEventBtn->setObjectName(QStringLiteral("queryEventBtn"));
        queryEventBtn->setGeometry(QRect(400, 290, 75, 23));
        statusLbl = new QLabel(videoTestClass);
        statusLbl->setObjectName(QStringLiteral("HDD"));
        statusLbl->setGeometry(QRect(10, 340, 361, 41));
        stopPbBtn = new QPushButton(videoTestClass);
        stopPbBtn->setObjectName(QStringLiteral("stopPbBtn"));
        stopPbBtn->setGeometry(QRect(500, 290, 75, 23));
        loginout = new QPushButton(videoTestClass);
        loginout->setObjectName(QStringLiteral("loginout"));
        loginout->setGeometry(QRect(500, 260, 75, 23));

        StartTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
        StartTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
        StartTimeEdit->setGeometry(QRect(400, 330, 194, 21));
        EndTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
        EndTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
        EndTimeEdit->setGeometry(QRect(400, 360, 194, 21));
        retranslateUi(videoTestClass);

        QMetaObject::connectSlotsByName(videoTestClass);
    } // setupUi

    void retranslateUi(QWidget *videoTestClass)
    {
        videoTestClass->setWindowTitle(QApplication::translate("videoTestClass", "videoTest", 0));
      //  playLbl->setText(QApplication::translate("videoTestClass", "", 0));
        playBtn->setText(QApplication::translate("videoTestClass", "play", 0));
        playbackBtn->setText(QApplication::translate("videoTestClass", "playback", 0));
        stopBtn->setText(QApplication::translate("videoTestClass", "stop", 0));
        queryEventBtn->setText(QApplication::translate("videoTestClass", "queryEvent", 0));
        statusLbl->setText(QApplication::translate("videoTestClass", " HDD", 0));
        stopPbBtn->setText(QApplication::translate("videoTestClass", "stopPB", 0));
         loginout->setText(QApplication::translate("videoTestClass", "out", 0));
    } // retranslateUi

};

namespace Ui {
    class videoTestClass: public Ui_videoTestClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIDEOTEST_H
