#ifndef AUTOSHUTDOWNCORE_H
#define AUTOSHUTDOWNCORE_H

#include <QObject>
#include <QTimer>
#include <QVariantMap>

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

    bool isShutdownTriggered() const { return m_shutdownTriggered; }

    QVariantMap getStatus() const;
    void saveConfig();
    void cancelShutdown();

signals:
    void statusChanged(const QVariantMap &status);

private slots:
    void checkLoop();

private:
    void loadConfig();
    double getIdleTime();
    double getIdleTimeDBus();
    double getIdleTimeXprintidle();
    void shutdown();
    void sendNotification(const QString &title, const QString &message);

    QString m_configPath;
    int m_idleTimeout;      // 秒単位
    int m_checkInterval;    // 秒単位
    bool m_enabled;
    bool m_running;
    bool m_shutdownTriggered;
    double m_currentIdleTime;

    QTimer *m_timer;
};

#endif // AUTOSHUTDOWNCORE_H
