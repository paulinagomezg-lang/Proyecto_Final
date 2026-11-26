#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <exception>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    try {
        MainWindow w;
        w.show();
        return QApplication::exec();
    } catch (const exception& e) {
        QMessageBox::critical(nullptr, "Error al abrir el juego", QString::fromUtf8(e.what()));
        return 1;
    }
}
