#include "autoshutdowncore.h"
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

AutoShutdownCore::AutoShutdownCore(const QString &configPath, QObject *parent)
    : QObject(parent)
    , m_configPath(configPath)
    , m_idleTimeout(300)
    , m_checkInterval(10)
    , m_enabled(true)
    , m_running(false)
    , m_shutdownTriggered(false)
    , m_currentIdleTime(-1)
{
    if (m_configPath.isEmpty()) {
        m_configPath = QDir(qApp->applicationDirPath()).filePath("config.ini");
    }
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AutoShutdownCore::checkLoop);

    loadConfig();
}

AutoShutdownCore::~AutoShutdownCore()
{
    stop();
}

void AutoShutdownCore::loadConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    m_idleTimeout = settings.value("general/idle_timeout", 300).toInt();
    m_checkInterval = settings.value("general/check_interval", 10).toInt();
    m_enabled = settings.value("general/enabled", true).toBool();

    qDebug() << "Config loaded: idle_timeout =" << m_idleTimeout
             << ", check_interval =" << m_checkInterval
             << ", enabled =" << m_enabled;
}

void AutoShutdownCore::saveConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setValue("general/idle_timeout", m_idleTimeout);
    settings.setValue("general/check_interval", m_checkInterval);
    settings.setValue("general/enabled", m_enabled);
    settings.sync();

    qDebug() << "Config saved: idle_timeout =" << m_idleTimeout
             << ", check_interval =" << m_checkInterval
             << ", enabled =" << m_enabled;
}

void AutoShutdownCore::start()
{
    if (m_running) return;
    m_running = true;
    m_timer->start(m_checkInterval * 1000);
    sendNotification("AutoShutdown", "Start Auto Shutdown");
    qDebug() << "AutoShutdown core started";
}

void AutoShutdownCore::stop()
{
    if (!m_running) return;
    m_running = false;
    m_timer->stop();
    qDebug() << "AutoShutdown core stopped";
}

void AutoShutdownCore::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    emit statusChanged(getStatus());
}

void AutoShutdownCore::setIdleTimeout(int timeout)
{
    if (m_idleTimeout == timeout) return;
    m_idleTimeout = timeout;
    emit statusChanged(getStatus());
}

void AutoShutdownCore::setCheckInterval(int interval)
{
    if (m_checkInterval == interval) return;
    m_checkInterval = interval;
    if (m_running) {
        m_timer->start(m_checkInterval * 1000);
    }
    emit statusChanged(getStatus());
}

QVariantMap AutoShutdownCore::getStatus() const
{
    QVariantMap status;
    status["enabled"] = m_enabled;
    status["idle_time"] = m_currentIdleTime >= 0 ? QVariant(m_currentIdleTime) : QVariant();
    status["idle_timeout"] = m_idleTimeout;
    status["check_interval"] = m_checkInterval;

    QVariant remaining;
    if (m_currentIdleTime >= 0 && m_enabled) {
        remaining = qMax(0.0, static_cast<double>(m_idleTimeout) - m_currentIdleTime);
    }
    status["remaining"] = remaining;
    status["running"] = m_running;
    status["shutdown_triggered"] = m_shutdownTriggered;

    return status;
}

void AutoShutdownCore::checkLoop()
{
    if (!m_enabled) {
        m_currentIdleTime = -1;
        emit statusChanged(getStatus());
        return;
    }

    double idleTime = getIdleTime();
    m_currentIdleTime = idleTime;

    emit statusChanged(getStatus());

    if (idleTime >= 0) {
        if (idleTime >= m_idleTimeout && !m_shutdownTriggered) {
            qWarning() << "Idle timeout reached:" << idleTime << ">=" << m_idleTimeout;
            shutdown();
        }
    } else {
        qWarning() << "Could not determine idle time, retrying...";
    }
}

double AutoShutdownCore::getIdleTime()
{
    double idle = getIdleTimeDBus();
    if (idle >= 0) {
        return idle;
    }
    return getIdleTimeXprintidle();
}

double AutoShutdownCore::getIdleTimeDBus()
{
    QDBusInterface interface("org.gnome.Mutter.IdleMonitor",
                             "/org/gnome/Mutter/IdleMonitor/Core",
                             "org.gnome.Mutter.IdleMonitor",
                             QDBusConnection::sessionBus());
    if (!interface.isValid()) {
        return -1.0;
    }

    QDBusMessage reply = interface.call("GetIdletime");
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        qulonglong idleMs = reply.arguments().at(0).toULongLong();
        return static_cast<double>(idleMs) / 1000.0;
    }

    return -1.0;
}

double AutoShutdownCore::getIdleTimeXprintidle()
{
    QProcess process;
    process.start("xprintidle", QStringList());
    if (!process.waitForFinished(3000)) {
        return -1.0;
    }

    if (process.exitCode() == 0) {
        QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        bool ok;
        qulonglong idleMs = output.toULongLong(&ok);
        if (ok) {
            return static_cast<double>(idleMs) / 1000.0;
        }
    }

    return -1.0;
}

void AutoShutdownCore::shutdown()
{
    qWarning() << "Initiating shutdown...";
    m_shutdownTriggered = true;
    emit statusChanged(getStatus());

    // 1分後にシャットダウンを実行
    QProcess::startDetached("sudo", QStringList() << "shutdown" << "-h" << "+1" << "Auto shutdown due to inactivity");
    sendNotification("AutoShutdown", "Idle timeout reached. Shutdown triggered in 1 minute!");
}

void AutoShutdownCore::cancelShutdown()
{
    qWarning() << "Cancelling shutdown...";
    QProcess::startDetached("sudo", QStringList() << "shutdown" << "-c");
    m_shutdownTriggered = false;
    emit statusChanged(getStatus());
    sendNotification("AutoShutdown", "Shutdown cancelled");
}

void AutoShutdownCore::sendNotification(const QString &title, const QString &message)
{
    QProcess::startDetached("notify-send", QStringList() << title << message);
}
