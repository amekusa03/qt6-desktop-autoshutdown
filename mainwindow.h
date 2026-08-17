#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QCloseEvent>
#include "autoshutdowncore.h"
#include "shutdowndialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AutoShutdownCore *core, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateDisplay(const QVariantMap &status);
    void onTimeoutSliderChanged(int value);
    void onIntervalSliderChanged(int value);
    void toggleEnabled();
    void applySettings();
    void cancelShutdown();
    void openLog();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void quitApp();
    void showShutdownDialog(int secondsRemaining);

private:
    void buildUi();
    void setupTrayIcon();
    void applyTheme();
    QIcon createTrayIcon(bool enabled, bool warning);
    QString formatTime(double seconds);

    AutoShutdownCore *m_core;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_toggleAction;

    // UI要素
    QLabel *m_lblIdle;
    QLabel *m_lblRemaining;
    QLabel *m_lblMsg;
    QPushButton *m_toggleBtn;
    QSlider *m_sliderTimeout;
    QLabel *m_lblTimeoutVal;
    QSlider *m_sliderInterval;
    QLabel *m_lblIntervalVal;

    // TCP UI要素
    QCheckBox *m_chkTcpEnabled;
    QSpinBox *m_spinTcpPort;
    QLineEdit *m_txtTcpToken;
    QLabel *m_lblTcpStatus;

    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_logBtn;
    ShutdownDialog *m_shutdownDialog;
};

#endif // MAINWINDOW_H
