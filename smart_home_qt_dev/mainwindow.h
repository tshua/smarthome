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

    void updateView();

public slots:
    void deal_recvData(Msgbuf* msg);

private:
    QLabel* label_lamp1_status;
    QLabel* label_light1;
    QLabel* label_lamp1_mode;
    QPixmap* lamp1_img;
    bool is_lamp1_online; //lamp1是否在线

    QLabel* label_lamp2_status;
    QLabel* label_light2;
    QLabel* label_lamp2_mode;
    QPixmap* lamp2_img;
    bool is_lamp2_online; //lamp2是否在线

    QLabel* label_fan_status;
    QLabel* label_fan_img;
    QLabel* label_tempruature;
    QPixmap* fan_img;
    bool is_fan_online; //fan是否在线

    QLabel* label_switch_status;
    QPixmap* switch_img;
    bool is_switch_online; //开关是否在线

    RecvMsgThread* recvThread;

};

#endif // MAINWINDOW_H
