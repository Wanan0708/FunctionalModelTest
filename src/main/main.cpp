#include <QApplication>

#include "ui/widgets/WorkbenchWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    fm::ui::WorkbenchWindow window;
    window.show();

    return app.exec();
}