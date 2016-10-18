#include "mainwindow.h"
#include <QApplication>
#include "logindialog.h"



int main(int argc, char *argv[])
{
    //signal(SIGINT, signal_fun);
    QApplication a(argc, argv);
    MainWindow w;

    LoginDialog LoginDialog;
    if(LoginDialog.exec() == QDialog::Accepted)
    {
        w.show();
        return a.exec();
    }
    else
        return 0;


    //w.show();

    //return a.exec();
}
