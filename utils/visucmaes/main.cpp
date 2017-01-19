#include "visucmaes.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Visucmaes v;
    v.show();

    return app.exec();
}

