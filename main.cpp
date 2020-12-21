#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <string>
#include <iostream>

#include "mainwindow.h"



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //MyDialog d;//(&w);

    w.show();
    //d.show();
    return a.exec();
}
