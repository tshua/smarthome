#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QtCore>
#include "recvmsgthread.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPixmap;
class QGridLayout;
class QPushButton;
class QGroupBox;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    int lamp1_status; //灯1
    int lamp1_mode;
    int light1;

    int lamp2_status; //灯2
    int lamp2_mode;
    int light2;

    int fan_status; //电扇
    int temprature;

    int switch_status; //智能开关
    QString dev_info;
    QString phone_info;
    QLabel* label_dev_info;
    QLabel* label_phone_info;



    void updateView();
    int addMsg(int msgid, const char* buf, int size);

public slots:
    void deal_recvData(Msgbuf* msg);

    void lamp1StatusButton_click();
    void lamp1ModeButton_click();
    void lamp2StatusButton_click();
    void lamp2ModeButton_click();
    void fanStatusButton_click();
    void switchStatusButton_click();

private:

    bool is_lamp1_online; //lamp1是否在线
    QLabel* label_lamp1_online;

    bool is_lamp2_online; //lamp2是否在线
    QLabel* label_lamp2_online;

    bool is_fan_online; //fan是否在线
    QLabel* label_fan_online;

    bool is_switch_online; //开关是否在线
    QLabel* label_switch_online;

    QLabel* label_lamp1_status;
    QLabel* label_light1;
    QLabel* label_lamp1_mode;
    QPixmap* lamp1_img;

    QLabel* label_lamp2_status;
    QLabel* label_light2;
    QLabel* label_lamp2_mode;
    QPixmap* lamp2_img;

    QLabel* label_fan_status;
    QLabel* label_fan_img;
    QLabel* label_tempruature;
    QPixmap* fan_img;

    QLabel* label_switch_status;
    QPixmap* switch_img;

    QPushButton *lamp1StatusButton;
    QPushButton *lamp1ModeButton;

    QPushButton *lamp2StatusButton;
    QPushButton *lamp2ModeButton;

    QPushButton *fanStatusButton;

    QPushButton *switchStatusButton;

    RecvMsgThread* recvThread;


//    QGroupBox* group_dev;
//    QGroupBox* group_phone;
//    QGroupBox* group_tm_info;
};

#endif // MAINWINDOW_H
