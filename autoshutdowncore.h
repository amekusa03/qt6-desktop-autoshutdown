#ifndef AUTOSHUTDOWNCORE_H
#define AUTOSHUTDOWNCORE_H

#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QTcpServer>
#include <QTcpSocket>

class AutoShutdownCore : public QObject
{
    Q_OBJECT
public:
    explicit AutoShutdownCore(const QString &configPath = QString(), QObject *parent = nullptr);
    ~AutoShutdownCore();

    void start();
    void stop();

    // ゲッター／セッター
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    int idleTimeout() const { return m_idleTimeout; }
    void setIdleTimeout(int timeout);

    int checkInterval() const { return m_checkInterval; }
    void setCheckInterval(int interval);

    // TCP ゲッター／セッター
    bool isTcpEnabled() const { return m_tcpEnabled; }
    void setTcpEnabled(bool enabled);

    int tcpPort() const { return m_tcpPort; }
    void setTcpPort(int port);

    QString tcpToken() const { return m_tcpToken; }
    void setTcpToken(const QString &token);

    QString tcpTokenVolUp() const { return m_tcpTokenVolUp; }
    void setTcpTokenVolUp(const QString &token);

    QString tcpTokenVolDown() const { return m_tcpTokenVolDown; }
    void setTcpTokenVolDown(const QString &token);

    QString tcpTokenVolMute() const { return m_tcpTokenVolMute; }
    void setTcpTokenVolMute(const QString &token);

    QString tcpTokenVolGet() const { return m_tcpTokenVolGet; }
    void setTcpTokenVolGet(const QString &token);

    bool isShutdownTriggered() const { return m_shutdownTriggered; }

    QVariantMap getStatus() const;
    void saveConfig();
    void cancelShutdown();
    void cancelWarning();
    void shutdownNow();

    // 音量コントロール
    void volumeUp();
    void volumeDown();
    void volumeMuteToggle();
    void getVolumeAndMute(int &volume, bool &isMuted);

signals:
    void statusChanged(const QVariantMap &status);
    void shutdownRequested(int secondsRemaining);
    void shutdownCancelled();

private slots:
    void checkLoop();
    void onNewTcpConnection();
    void onTcpReadyRead();

private:
    void loadConfig();
    double getIdleTime();
    double getIdleTimeDBus();
    double getIdleTimeXprintidle();
    void shutdown();
    void sendNotification(const QString &title, const QString &message);
    void setupTcpServer();

    QString m_configPath;
    int m_idleTimeout;      // 秒単位
    int m_checkInterval;    // 秒単位
    bool m_enabled;
    bool m_running;
    bool m_shutdownTriggered;
    bool m_warningShown;
    double m_currentIdleTime;

    // TCP関連
    bool m_tcpEnabled;
    int m_tcpPort;
    QString m_tcpToken;
    QString m_tcpTokenVolUp;
    QString m_tcpTokenVolDown;
    QString m_tcpTokenVolMute;
    QString m_tcpTokenVolGet;
    QTcpServer *m_tcpServer;

    QTimer *m_timer;
};

#endif // AUTOSHUTDOWNCORE_H
