#include "mainwindow.h"
#include <QApplication>
#include <signal.h>


int main(int argc, char *argv[])
{
    //signal(SIGINT, signal_fun);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
