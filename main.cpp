#include "mainwindow.h"
#include "arithmetic_coding.h"
#include "statistical_tools.h"
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
