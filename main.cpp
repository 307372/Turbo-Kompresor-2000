#include <QApplication>

#include "archive_window.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ArchiveWindow w;

    w.show();
    return a.exec();
}
