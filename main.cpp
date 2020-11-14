#include "mainwindow.h"
#include "arithmetic_coding.h"
#include "statistical_tools.h"
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <string>
#include <iostream>
#include "archive.h"



int main(int argc, char *argv[])
{
    //build_test_archive();
    //get_test_archives_content();

    //arithmetic_coding ac;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();

    return 0;
}
