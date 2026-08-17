#ifndef SHUTDOWNDIALOG_H
#define SHUTDOWNDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "autoshutdowncore.h"

class ShutdownDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShutdownDialog(AutoShutdownCore *core, int seconds = 60, QWidget *parent = nullptr);
    ~ShutdownDialog();

private slots:
    void updateCountdown();
    void onCancelClicked();
    void onShutdownNowClicked();

private:
    void buildUi();
    void applyTheme();

    AutoShutdownCore *m_core;
    QTimer *m_timer;
    int m_remainingSeconds;

    QLabel *m_lblTitle;
    QLabel *m_lblMsg;
    QLabel *m_lblCountdown;
    QPushButton *m_btnCancel;
    QPushButton *m_btnNow;
};

#endif // SHUTDOWNDIALOG_H
