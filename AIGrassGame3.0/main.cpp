#include <QApplication>
#include "Gamewindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Gamewindow w;
    w.show();
    return a.exec();
}
