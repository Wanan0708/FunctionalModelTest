#include <QApplication>

#include "ui/widgets/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    fm::ui::MainWindow window;
    window.showMaximized();

    return app.exec();
}