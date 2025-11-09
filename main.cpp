#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setWindowTitle("KeyRecorder v0.0.1");

    w.show();
    return a.exec();
}
