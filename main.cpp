#include <QApplication>
#include "mainwindow.h"
#include "autoshutdowncore.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AutoShutdown");
    app.setOrganizationName("AutoShutdown");

    // ウィンドウがすべて閉じられてもアプリ自体は終了しないように設定（トレイ常駐のため）
    app.setQuitOnLastWindowClosed(false);

    // --minimized 引数が指定された場合はウィンドウを表示しない（自動起動時向け）
    bool minimized = app.arguments().contains("--minimized");

    AutoShutdownCore core;
    core.start();

    MainWindow mainWindow(&core);
    if (!minimized) {
        mainWindow.show();
    }

    return app.exec();
}
