/********************************************************************************
** Form generated from reading UI file 'logindialog.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINDIALOG_H
#define UI_LOGINDIALOG_H

#include <QtCore/QVariant>
#include <QtGui>
//#include <QtWidgets/QAction>
//#include <QtWidgets/QApplication>
//#include <QtWidgets/QButtonGroup>
//#include <QtWidgets/QDialog>
//#include <QtWidgets/QFormLayout>
//#include <QtWidgets/QHeaderView>
//#include <QtWidgets/QLabel>
//#include <QtWidgets/QLineEdit>
//#include <QtWidgets/QPushButton>
//#include <QtWidgets/QSpacerItem>
//#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoginDialog
{
public:
    QLabel *label_3;
    QWidget *layoutWidget;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *lineEdit_name;
    QLabel *label_2;
    QLineEdit *lineEdit_pass;
    QPushButton *pushButton;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *LoginDialog)
    {
        if (LoginDialog->objectName().isEmpty())
            LoginDialog->setObjectName(/*/*QStringLiteral*/("LoginDialog"));
        LoginDialog->resize(400, 300);
        label_3 = new QLabel(LoginDialog);
        label_3->setObjectName(/*/*QStringLiteral*/("label_3"));
        label_3->setGeometry(QRect(130, 20, 181, 31));
        QFont font;
        font.setFamily(QString::fromUtf8("\346\226\207\346\263\211\351\251\277\346\255\243\351\273\221"));
        font.setPointSize(20);
        label_3->setFont(font);
        layoutWidget = new QWidget(LoginDialog);
        layoutWidget->setObjectName(/*/*QStringLiteral*/("layoutWidget"));
        layoutWidget->setGeometry(QRect(60, 80, 281, 161));
        formLayout = new QFormLayout(layoutWidget);
        formLayout->setObjectName(/*/*QStringLiteral*/("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(layoutWidget);
        label->setObjectName(/*QStringLiteral*/("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        lineEdit_name = new QLineEdit(layoutWidget);
        lineEdit_name->setObjectName(/*QStringLiteral*/("lineEdit_name"));

        formLayout->setWidget(0, QFormLayout::FieldRole, lineEdit_name);

        label_2 = new QLabel(layoutWidget);
        label_2->setObjectName(/*QStringLiteral*/("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        lineEdit_pass = new QLineEdit(layoutWidget);
        lineEdit_pass->setObjectName(/*QStringLiteral*/("lineEdit_pass"));

        formLayout->setWidget(1, QFormLayout::FieldRole, lineEdit_pass);

        pushButton = new QPushButton(layoutWidget);
        pushButton->setObjectName(/*QStringLiteral*/("pushButton"));

        formLayout->setWidget(3, QFormLayout::FieldRole, pushButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        formLayout->setItem(2, QFormLayout::FieldRole, verticalSpacer);


        retranslateUi(LoginDialog);

        QMetaObject::connectSlotsByName(LoginDialog);
    } // setupUi

    void retranslateUi(QDialog *LoginDialog)
    {
        LoginDialog->setWindowTitle(QApplication::translate("LoginDialog", "Dialog", 0));
        label_3->setText(QApplication::translate("LoginDialog", "User Login", 0));
        label->setText(QApplication::translate("LoginDialog", "Name:", 0));
        label_2->setText(QApplication::translate("LoginDialog", "Password:", 0));
        pushButton->setText(QApplication::translate("LoginDialog", "login", 0));
    } // retranslateUi

};

namespace Ui {
    class LoginDialog: public Ui_LoginDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINDIALOG_H
