#include "mainwindow.h"
#include <QtWidgets>
#include <QDebug>

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
    label_light1 = new QLabel("光照：0");
    label_lamp1_mode = new QLabel("模式:手动");
    lamp1_img = new QPixmap("://light_off.png");
    is_lamp1_online = false;
    label_lamp1_online = new QLabel("离线");
    label_lamp1_online->setPalette(pa);

    int lampStatusLabelWidth = 200, lampStatusLabelHeight = 200;
    *lamp1_img = lamp1_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_lamp1_status->setAlignment(Qt::AlignCenter);
    label_lamp1_status->setPixmap(*lamp1_img);

    QVBoxLayout* label_lamp1_Layout = new QVBoxLayout();
    label_lamp1_Layout->addWidget(label_lamp1_online);
    label_lamp1_Layout->addWidget(label_light1);
    label_lamp1_Layout->addWidget(label_lamp1_mode);
    //lamp1 end

    //lamp2 start
    lamp2_status = 0; // 0关 1开
    lamp2_mode = 0;   //0手动 1自动
    light2 = 0; //光照

    label_lamp2_status = new QLabel(this);
    label_light2 = new QLabel("光照：0");
    label_lamp2_mode = new QLabel("模式:手动");
    lamp2_img = new QPixmap("://light_off.png");
    is_lamp2_online = false;
    label_lamp2_online = new QLabel("离线");
    label_lamp2_online->setPalette(pa);


    *lamp2_img = lamp2_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_lamp2_status->setAlignment(Qt::AlignCenter);
    label_lamp2_status->setPixmap(*lamp2_img);

    QVBoxLayout* label_lamp2_Layout = new QVBoxLayout();
    label_lamp2_Layout->addWidget(label_lamp2_online);
    label_lamp2_Layout->addWidget(label_light2);
    label_lamp2_Layout->addWidget(label_lamp2_mode);
    //lamp2 end

    //fan start
    fan_status = 0;
    temprature = 10;

    label_fan_img = new QLabel(this);
    label_fan_status = new QLabel("状态 ：开");
    label_tempruature = new QLabel("温度 ：10");
    fan_img = new QPixmap("://AirConImage.png");
    is_fan_online = false;
    label_fan_online = new QLabel("离线");
    label_fan_online->setPalette(pa);

    *fan_img = fan_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_fan_img->setAlignment(Qt::AlignCenter);
    label_fan_img->setPixmap(*fan_img);

    QVBoxLayout* label_fan_Layout = new QVBoxLayout();
    label_fan_Layout->addWidget(label_fan_online);
    label_fan_Layout->addWidget(label_fan_status);
    label_fan_Layout->addWidget(label_tempruature);
    //fan end

    //switch start
    switch_status = 0;

    label_switch_status = new QLabel(this);
    switch_img = new QPixmap("://switch_off.png");
    is_switch_online = false;
    label_switch_online = new QLabel("离线");
    label_switch_online->setPalette(pa);

    *switch_img = switch_img->scaled(lampStatusLabelWidth, lampStatusLabelHeight, Qt::KeepAspectRatio);
    label_switch_status->setAlignment(Qt::AlignCenter);
    label_switch_status->setPixmap(*switch_img);
    QVBoxLayout* label_switch_Layout = new QVBoxLayout();
    label_switch_Layout->addWidget(label_switch_online);

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
    setWindowTitle(tr("DEVICES"));

    recvThread = new RecvMsgThread();
    connect(recvThread, SIGNAL(sig_recvDataOk(Msgbuf*)), this, SLOT(deal_recvData(Msgbuf*)));
    recvThread->start();

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
    QString tmp = lamp1_mode?"自动":"手动";
    label_light1->setText("光照： " + QString::number(light1, 10));
    label_lamp1_mode->setText("模式： " + tmp);
    label_lamp1_online->setText(is_lamp1_online?"在线":"离线");
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
    tmp = lamp2_mode?"自动":"手动";
    label_light2->setText("光照： " + QString::number(light2, 10));
    label_lamp2_mode->setText("模式： " + tmp);
    label_lamp2_online->setText(is_lamp2_online?"在线":"离线");
    if(is_lamp2_online)
        label_lamp2_online->setPalette(pa_green);
    else
        label_lamp2_online->setPalette(pa_red);



    //fan
    tmp = fan_status?"开":"关";
    label_fan_status->setText("状态：" + tmp);
    label_tempruature->setText("温度： " + QString::number(temprature, 10));

    label_fan_online->setText(is_fan_online?"在线":"离线");
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

    label_switch_online->setText(is_switch_online?"在线":"离线");
    if(is_switch_online)
        label_switch_online->setPalette(pa_green);
    else
        label_switch_online->setPalette(pa_red);



}
