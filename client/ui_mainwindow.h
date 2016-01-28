/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWidget>
#include <mainwindow.h>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionMenu;
    QAction *actionPreferences;
    QAction *actionCommand_List;
    QAction *actionAbout;
    QAction *actionHelp;
    QAction *actionConnect;
    QAction *actionPreferences_2;
    QAction *actionExit;
    QAction *actionOpen;
    QAction *actionCommands;
    QAction *actionAbout_2;
    QAction *actionMute_Sounds;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *chat_input;
    QPushButton *chat_send;
    QHBoxLayout *horizontalLayout;
    QTabWidget *tabs;
    QWidget *Status_Page;
    QHBoxLayout *horizontalLayout_6;
    QHBoxLayout *horizontalLayout_5;
    QTextBrowser *status_browser;
    QMenuBar *menuBar;
    QMenu *menuMenu;
    QMenu *menuSettings;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(900, 550);
        MainWindow->setMinimumSize(QSize(900, 550));
        MainWindow->setMaximumSize(QSize(1920, 1080));
        actionMenu = new QAction(MainWindow);
        actionMenu->setObjectName(QStringLiteral("actionMenu"));
        actionPreferences = new QAction(MainWindow);
        actionPreferences->setObjectName(QStringLiteral("actionPreferences"));
        actionCommand_List = new QAction(MainWindow);
        actionCommand_List->setObjectName(QStringLiteral("actionCommand_List"));
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionHelp = new QAction(MainWindow);
        actionHelp->setObjectName(QStringLiteral("actionHelp"));
        actionConnect = new QAction(MainWindow);
        actionConnect->setObjectName(QStringLiteral("actionConnect"));
        actionPreferences_2 = new QAction(MainWindow);
        actionPreferences_2->setObjectName(QStringLiteral("actionPreferences_2"));
        actionPreferences_2->setEnabled(false);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        actionCommands = new QAction(MainWindow);
        actionCommands->setObjectName(QStringLiteral("actionCommands"));
        actionAbout_2 = new QAction(MainWindow);
        actionAbout_2->setObjectName(QStringLiteral("actionAbout_2"));
        actionMute_Sounds = new QAction(MainWindow);
        actionMute_Sounds->setObjectName(QStringLiteral("actionMute_Sounds"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        centralWidget->setMinimumSize(QSize(900, 400));
        centralWidget->setMaximumSize(QSize(1920, 1080));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        chat_input = new QLineEdit(centralWidget);
        chat_input->setObjectName(QStringLiteral("chat_input"));

        horizontalLayout_2->addWidget(chat_input);

        chat_send = new QPushButton(centralWidget);
        chat_send->setObjectName(QStringLiteral("chat_send"));

        horizontalLayout_2->addWidget(chat_send);


        gridLayout->addLayout(horizontalLayout_2, 1, 0, 1, 2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        tabs = new QTabWidget(centralWidget);
        tabs->setObjectName(QStringLiteral("tabs"));
        tabs->setTabsClosable(true);
        Status_Page = new QWidget();
        Status_Page->setObjectName(QStringLiteral("Status_Page"));
        horizontalLayout_6 = new QHBoxLayout(Status_Page);
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        status_browser = new QTextBrowser(Status_Page);
        status_browser->setObjectName(QStringLiteral("status_browser"));
        status_browser->setStyleSheet(QStringLiteral(""));

        horizontalLayout_5->addWidget(status_browser);


        horizontalLayout_6->addLayout(horizontalLayout_5);

        tabs->addTab(Status_Page, QString());

        horizontalLayout->addWidget(tabs);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 2);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 900, 21));
        menuMenu = new QMenu(menuBar);
        menuMenu->setObjectName(QStringLiteral("menuMenu"));
        menuSettings = new QMenu(menuBar);
        menuSettings->setObjectName(QStringLiteral("menuSettings"));
        MainWindow->setMenuBar(menuBar);

        menuBar->addAction(menuMenu->menuAction());
        menuBar->addAction(menuSettings->menuAction());
        menuMenu->addAction(actionExit);
        menuSettings->addAction(actionConnect);
        menuSettings->addAction(actionPreferences_2);
        menuSettings->addAction(actionMute_Sounds);

        retranslateUi(MainWindow);
        QObject::connect(actionExit, SIGNAL(triggered()), MainWindow, SLOT(close()));

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "NADL Test Client", 0));
        actionMenu->setText(QApplication::translate("MainWindow", "Exit", 0));
        actionPreferences->setText(QApplication::translate("MainWindow", "Preferences", 0));
        actionCommand_List->setText(QApplication::translate("MainWindow", "Command List", 0));
        actionAbout->setText(QApplication::translate("MainWindow", "About", 0));
        actionHelp->setText(QApplication::translate("MainWindow", "NEL", 0));
        actionConnect->setText(QApplication::translate("MainWindow", "Connect", 0));
        actionPreferences_2->setText(QApplication::translate("MainWindow", "Preferences", 0));
        actionExit->setText(QApplication::translate("MainWindow", "Exit", 0));
        actionExit->setShortcut(QApplication::translate("MainWindow", "Ctrl+Q", 0));
        actionOpen->setText(QApplication::translate("MainWindow", "Open", 0));
        actionCommands->setText(QApplication::translate("MainWindow", "Commands", 0));
        actionAbout_2->setText(QApplication::translate("MainWindow", "About", 0));
        actionMute_Sounds->setText(QApplication::translate("MainWindow", "Mute Sounds", 0));
        chat_send->setText(QApplication::translate("MainWindow", "Send", 0));
        status_browser->setHtml(QApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", 0));
        tabs->setTabText(tabs->indexOf(Status_Page), QApplication::translate("MainWindow", "Status", 0));
        menuMenu->setTitle(QApplication::translate("MainWindow", "Menu", 0));
        menuSettings->setTitle(QApplication::translate("MainWindow", "Settings", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
