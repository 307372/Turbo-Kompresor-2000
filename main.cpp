#include <QApplication>

#include "archive_window.h"
#include "cli.hpp"


int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        QApplication a(argc, argv);
        ArchiveWindow w;

        w.show();
        return a.exec();
    }
    else 
    {
        cli::handleArgs(argc, argv);
    }
}
