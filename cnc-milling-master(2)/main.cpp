#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();//窗口最大化
    return a.exec();
}
