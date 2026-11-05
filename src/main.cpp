#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Musebook"));
    QApplication::setApplicationVersion(QStringLiteral(MUSEBOOK_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Musebook"));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
