
#include "mainwindow.h"
#include <QApplication>
#include <stdio.h>
#include <QTimer>

MainWindow *w;





int main(int argc, char *argv[]) {
    QApplication a(argc, argv);


    w = new MainWindow;

    w->setWindowIcon(QIcon(":/oldguy.ico"));

    w->show();

    return a.exec();
}


