#include <QApplication>
#include "mainwindow.h"
#include "autoshutdowncore.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // ウィンドウがすべて閉じられてもアプリ自体は終了しないように設定（トレイ常駐のため）
    app.setQuitOnLastWindowClosed(false);

    AutoShutdownCore core;
    core.start();

    MainWindow mainWindow(&core);
    mainWindow.show();

    return app.exec();
}
