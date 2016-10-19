#include "mainwindow.h"
//#include <QtWidgets>
#include <QtGui>
#include <QDebug>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    QPalette pa;                                //红色字体
    pa.setColor(QPalette::WindowText,Qt::red);

    //lamp1 start
    lamp1_status = 0; // 0关 1开
    lamp1_mode = 0;   //0手动 1自动
    light1 = 0; //光照

    label_lamp1_status = new QLabel(this);
    label_light1 = new QLabel("Light: 0");//("光照：0");
    label_lamp1_mode = new QLabel("Mode: Manual");//("模式:手动");
    lamp1_img = new QPixmap("://light_off.png");
    is_lamp1_online = false;
    label_lamp1_online = new QLabel("Offline");//("离线");
    label_lamp1_online->setPalette(pa);

    int lampStatusLabelWidth = 200, lampStatusLabelHeight = 200;
    *lamp1_img = lamp1_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_lamp1_status->setAlignment(Qt::AlignCenter);
    label_lamp1_status->setPixmap(*lamp1_img);

    lamp1StatusButton = new QPushButton(lamp1_status?"Close":"Open"); //"关闭":"打开");
    lamp1ModeButton = new QPushButton(lamp1_mode?"Manual":"Auto");//"手动":"自动");

    QVBoxLayout* label_lamp1_Layout = new QVBoxLayout();
    label_lamp1_Layout->addWidget(label_lamp1_online);
    label_lamp1_Layout->addWidget(label_light1);
    label_lamp1_Layout->addWidget(label_lamp1_mode);
    label_lamp1_Layout->addWidget(lamp1StatusButton);
    label_lamp1_Layout->addWidget(lamp1ModeButton);
    //lamp1 end


    //lamp2 start
    lamp2_status = 0; // 0关 1开
    lamp2_mode = 0;   //0手动 1自动
    light2 = 0; //光照

    label_lamp2_status = new QLabel(this);
    label_light2 = new QLabel("Light: 0");//("光照：0");
    label_lamp2_mode = new QLabel("Mode: Manual");//("模式:手动");
    lamp2_img = new QPixmap("://light_off.png");
    is_lamp2_online = false;
    label_lamp2_online = new QLabel("Offline");//("离线");
    label_lamp2_online->setPalette(pa);


    *lamp2_img = lamp2_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_lamp2_status->setAlignment(Qt::AlignCenter);
    label_lamp2_status->setPixmap(*lamp2_img);


    lamp2StatusButton = new QPushButton(lamp2_status?"Close":"Open");//"关闭":"打开");
    lamp2ModeButton = new QPushButton(lamp2_mode?"Manual":"Auto");//"手动":"自动");

    QVBoxLayout* label_lamp2_Layout = new QVBoxLayout();
    label_lamp2_Layout->addWidget(label_lamp2_online);
    label_lamp2_Layout->addWidget(label_light2);
    label_lamp2_Layout->addWidget(label_lamp2_mode);
    label_lamp2_Layout->addWidget(lamp2StatusButton);
    label_lamp2_Layout->addWidget(lamp2ModeButton);
    //lamp2 end

    //fan start
    fan_status = 0;
    temprature = 10;

    label_fan_img = new QLabel(this);
    label_fan_status = new QLabel("Status: On");//("状态 ：开");
    label_tempruature = new QLabel("Temprature: 10");//("温度 ：10");
    fan_img = new QPixmap("://AirConImage.png");
    is_fan_online = false;
    label_fan_online = new QLabel("Offline");//("离线");
    label_fan_online->setPalette(pa);


    *fan_img = fan_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_fan_img->setAlignment(Qt::AlignCenter);
    label_fan_img->setPixmap(*fan_img);

    fanStatusButton = new QPushButton(fan_status?"Close":"Open");//"关闭":"打开");


    QVBoxLayout* label_fan_Layout = new QVBoxLayout();
    label_fan_Layout->addWidget(label_fan_online);
    label_fan_Layout->addWidget(label_fan_status);
    label_fan_Layout->addWidget(label_tempruature);
    label_fan_Layout->addWidget(fanStatusButton);
    //fan end

    //switch start
    switch_status = 0;

    label_switch_status = new QLabel(this);
    switch_img = new QPixmap("://switch_off.png");
    is_switch_online = false;
    label_switch_online = new QLabel("Offline");//("离线");
    label_switch_online->setPalette(pa);

    *switch_img = switch_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_switch_status->setAlignment(Qt::AlignCenter);
    label_switch_status->setPixmap(*switch_img);
    switchStatusButton = new QPushButton(switch_status?"Close":"Open");//"关闭":"打开");
    QVBoxLayout* label_switch_Layout = new QVBoxLayout();
    label_switch_Layout->addWidget(label_switch_online);
    label_switch_Layout->addWidget(switchStatusButton);
    //switch end



    //! [grid layout]
    QGridLayout* mainLayout = new QGridLayout();
    mainLayout->addWidget(label_lamp1_status,0,0);
    mainLayout->addLayout(label_lamp1_Layout,0, 1, Qt::AlignCenter);

    mainLayout->addWidget(label_lamp2_status,0,2);
    mainLayout->addLayout(label_lamp2_Layout,0, 3, Qt::AlignCenter);

    mainLayout->addWidget(label_fan_img,1,0);
    mainLayout->addLayout(label_fan_Layout,1, 1, Qt::AlignCenter);

    mainLayout->addWidget(label_switch_status,1,2);
    mainLayout->addLayout(label_switch_Layout,1, 3, Qt::AlignCenter);
    //! [grid layout]



    QWidget* widget = new QWidget();
    setCentralWidget(widget);

    widget->setLayout(mainLayout);
    setWindowTitle(tr("PHONE"));

    recvThread = new RecvMsgThread();
    connect(recvThread, SIGNAL(sig_recvDataOk(Msgbuf*)), this, SLOT(deal_recvData(Msgbuf*)));
    recvThread->start();


    connect(lamp1StatusButton, SIGNAL(clicked(bool)), this, SLOT(lamp1StatusButton_click()));
    connect(lamp1ModeButton, SIGNAL(clicked(bool)), this, SLOT(lamp1ModeButton_click()));

    connect(lamp2StatusButton, SIGNAL(clicked(bool)), this, SLOT(lamp2StatusButton_click()));
    connect(lamp2ModeButton, SIGNAL(clicked(bool)), this, SLOT(lamp2ModeButton_click()));

    connect(fanStatusButton, SIGNAL(clicked(bool)), this, SLOT(fanStatusButton_click()));

    connect(switchStatusButton, SIGNAL(clicked(bool)), this, SLOT(switchStatusButton_click()));
}

MainWindow::~MainWindow()
{
    delete label_lamp1_status;
    delete label_light1;
    delete lamp1_img;
    delete label_lamp1_mode;
}

void MainWindow::deal_recvData(Msgbuf* msg)
{
    //QMessageBox::information(this, tr("my app"), tr(msg->mtext));
    qDebug() << msg->mtext;
    qDebug() << msg->mtext + 10;

    if(!strcmp(msg->mtext, "lamp1"))
    {
        if(!strcmp(msg->mtext+10, "on"))
        {
            lamp1_status = 1;
        }
        else if(!strcmp(msg->mtext+10, "off"))
        {
            lamp1_status = 0;
        }
        else if(!strcmp(msg->mtext+10, "manual"))
        {
            lamp1_mode = 0;
        }
        else if(!strcmp(msg->mtext+10, "auto"))
        {
            lamp1_mode = 1;
        }
        else if(!strcmp(msg->mtext+10, "login"))
        {
            is_lamp1_online = true;
        }
        else if(!strcmp(msg->mtext+10, "logout"))
        {
            is_lamp1_online = false;
        }
        else
        {
            light1 = atoi(msg->mtext+10);
            if(lamp1_mode == 1)
            {
                if(light1 > 50)
                    lamp1_status = 0;
                else
                    lamp1_status = 1;
            }
        }
    }
    else if(!strcmp(msg->mtext, "lamp2"))
    {
        if(!strcmp(msg->mtext+10, "on"))
        {
            lamp2_status = 1;
        }
        else if(!strcmp(msg->mtext+10, "off"))
        {
            lamp2_status = 0;
        }
        else if(!strcmp(msg->mtext+10, "manual"))
        {
            lamp2_mode = 0;
        }
        else if(!strcmp(msg->mtext+10, "auto"))
        {
            lamp2_mode = 1;
        }
        else if(!strcmp(msg->mtext+10, "login"))
        {
            is_lamp2_online = true;
        }
        else if(!strcmp(msg->mtext+10, "logout"))
        {
            is_lamp2_online = false;
        }
        else
        {
            light2 = atoi(msg->mtext+10);
            if(lamp2_mode == 1)
            {
                if(light2 > 50)
                    lamp2_status = 0;
                else
                    lamp2_status = 1;
            }
        }
    }
    else if(!strcmp(msg->mtext, "fan1"))
    {
        if(!strcmp(msg->mtext+10, "on"))
        {
            fan_status = 1;
        }
        else if(!strcmp(msg->mtext+10, "off"))
        {
            fan_status = 0;
        }
        else if(!strcmp(msg->mtext+10, "login"))
        {
            is_fan_online = true;
        }
        else if(!strcmp(msg->mtext+10, "logout"))
        {
            is_fan_online = false;
        }
        else
        {
            temprature = atoi(msg->mtext+10);
        }
    }
    else if(!strcmp(msg->mtext, "switch1"))
    {
        if(!strcmp(msg->mtext+10, "on"))
        {
            switch_status = 1;
        }
        else if(!strcmp(msg->mtext+10, "off"))
        {
            switch_status = 0;
        }
        else if(!strcmp(msg->mtext+10, "login"))
        {
            is_switch_online = true;
        }
        else if(!strcmp(msg->mtext+10, "logout"))
        {
            is_switch_online = false;
        }
    }

    delete msg; //在

    updateView();
}

void MainWindow::updateView()
{
    QPalette pa_red;                                //红色字体
    pa_red.setColor(QPalette::WindowText,Qt::red);
    QPalette pa_green;                              //绿色字体
    pa_green.setColor(QPalette::WindowText,Qt::green);

    //lamp1
    if(lamp1_img != NULL)
        delete lamp1_img;
    if(lamp1_status)
        lamp1_img = new QPixmap("://light_on.png");
    else
        lamp1_img = new QPixmap("://light_off.png");

    int lampStatusLabelWidth = 200, lampStatusLabelHeight = 200;
    *lamp1_img = lamp1_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);

    label_lamp1_status->setPixmap(*lamp1_img);
    QString tmp = lamp1_mode?"Auto":"Manual"; //"自动":"手动";
    label_light1->setText(/*"光照： "*/ "Light: "+ QString::number(light1, 10));
    label_lamp1_mode->setText(/*"模式： "*/ "Mode: " + tmp);
    label_lamp1_online->setText(is_lamp1_online?"Online":"Offline");//"在线":"离线");
    if(is_lamp1_online)
        label_lamp1_online->setPalette(pa_green);
    else
        label_lamp1_online->setPalette(pa_red);


    //lamp2
    if(lamp2_img != NULL)
        delete lamp2_img;
    if(lamp2_status)
        lamp2_img = new QPixmap("://light_on.png");
    else
        lamp2_img = new QPixmap("://light_off.png");

    *lamp2_img = lamp2_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);

    label_lamp2_status->setPixmap(*lamp2_img);
    tmp = lamp2_mode?"Auto":"Manual"; //"自动":"手动";
    label_light2->setText(/*"光照： "*/ "Light: " + QString::number(light2, 10));
    label_lamp2_mode->setText(/*"模式： "*/ "Mode: " + tmp);
    label_lamp2_online->setText(is_lamp2_online?"Online":"Offline");//"在线":"离线");
    if(is_lamp2_online)
        label_lamp2_online->setPalette(pa_green);
    else
        label_lamp2_online->setPalette(pa_red);


    //fan
    tmp = fan_status?"On":"Off";//"开":"关";
    label_fan_status->setText(/*"状态："*/ "Status: " + tmp);
    label_tempruature->setText(/*"温度： "*/ "Temprature: " + QString::number(temprature, 10));
    label_fan_online->setText(is_fan_online?"Online":"Offline");//"在线":"离线");
    if(is_fan_online)
        label_fan_online->setPalette(pa_green);
    else
        label_fan_online->setPalette(pa_red);

    //switch
    if(switch_img != NULL)
        delete switch_img;
    if(switch_status)
        switch_img = new QPixmap("://switch_on.png");
    else
        switch_img = new QPixmap("://switch_off.png");

    *switch_img = switch_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_switch_status->setPixmap(*switch_img);
    label_switch_online->setText(is_switch_online?"Online":"Offline");//"在线":"离线");
    if(is_switch_online)
        label_switch_online->setPalette(pa_green);
    else
        label_switch_online->setPalette(pa_red);

    lamp1StatusButton->setText(lamp1_status?"Close":"Open");//"关闭":"打开");
    lamp1ModeButton->setText(lamp1_mode?"Manual":"Auto");//"手动":"自动");

    lamp2StatusButton->setText(lamp2_status?"Close":"Open");//"关闭":"打开");
    lamp2ModeButton->setText(lamp2_mode?"Manual":"Auto");//"手动":"自动");

    fanStatusButton->setText(fan_status?"Close":"Open");//"关闭":"打开");
    switchStatusButton->setText(switch_status?"Close":"Open");//"手动":"自动");

}


void MainWindow::lamp1StatusButton_click()
{
    if(is_lamp1_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }

    if(lamp1_mode == 1)
    {
        QMessageBox::information(this, "Info", "Please close the auto mode first!");//tr("提示"), tr("请先关闭自动模式！"));
        return ;
    }
    char cmd[20] = {0};
    memcpy(cmd, "lamp1", 5);
    memcpy(cmd+10, lamp1_status?"off":"on", lamp1_status?3:2);

    addMsg(recvThread->getMsgId(), cmd, 20);
}

void MainWindow::lamp1ModeButton_click()
{
    if(is_lamp1_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        //QMessageBox::information(this, tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }
    char cmd[20] = {0};
    memcpy(cmd, "lamp1", 5);
    memcpy(cmd+10, lamp1_mode?"manual":"auto", lamp1_mode?6:4);

    addMsg(recvThread->getMsgId(), cmd, 20);
}

void MainWindow::lamp2StatusButton_click()
{
    if(is_lamp2_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        //QMessageBox::information(this, tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }
    if(lamp2_mode == 1)
    {
        QMessageBox::information(this, "Info", "Please close the auto mode first!");//tr("提示"), tr("请先关闭自动模式！"));
        //QMessageBox::information(this, tr("提示"), tr("请先关闭自动模式！"));
        return ;
    }

    char cmd[20] = {0};
    memcpy(cmd, "lamp2", 5);
    memcpy(cmd+10, lamp2_status?"off":"on", lamp2_status?3:2);

    addMsg(recvThread->getMsgId(), cmd, 20);
}

void MainWindow::lamp2ModeButton_click()
{
    if(is_lamp2_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        //QMessageBox::information(this, tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }
    char cmd[20] = {0};
    memcpy(cmd, "lamp2", 5);
    memcpy(cmd+10, lamp2_mode?"manual":"auto", lamp2_mode?6:4);

    addMsg(recvThread->getMsgId(), cmd, 20);
}

void MainWindow::fanStatusButton_click()
{
    if(is_fan_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        //QMessageBox::information(this, tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }
    char cmd[20] = {0};
    memcpy(cmd, "fan1", 4);
    memcpy(cmd+10, fan_status?"off":"on", fan_status?3:2);

    addMsg(recvThread->getMsgId(), cmd, 20);
}

void MainWindow::switchStatusButton_click()
{
    if(is_switch_online == 0)
    {
        QMessageBox::information(this, "Info", "Device is not online, please login the device.");//tr("提示"), tr("设备不在线，请重新登录设备！"));
        //QMessageBox::information(this, tr("提示"), tr("设备不在线，请重新登录设备！"));
        return;
    }
    char cmd[20] = {0};
    memcpy(cmd, "switch1", 7);
    memcpy(cmd+10, switch_status?"off":"on", switch_status?3:2);

    addMsg(recvThread->getMsgId(), cmd, 20);

}

int MainWindow::addMsg(int msgid, const char* buf, int size)
{
            Msgbuf msgbuf;
            bzero(&msgbuf, sizeof(Msgbuf));

            memcpy(msgbuf.mtext, buf, size);

            msgbuf.mtype = MSG_QTTOPHONE; //设置发送消息的类型

            int ret = msgsnd(msgid, (void *)&msgbuf, size, 0); //阻塞发送消息

            if(ret < 0)
                    return -1;
            return 1;
}
