#include "autoshutdowncore.h"
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
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
    , m_tcpEnabled(true)
    , m_tcpPort(12345)
    , m_tcpToken("secret123")
    , m_tcpServer(nullptr)
{
    if (m_configPath.isEmpty()) {
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        m_configPath = QDir(configDir).filePath("config.ini");
    }
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AutoShutdownCore::checkLoop);

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &AutoShutdownCore::onNewTcpConnection);

    loadConfig();
}

AutoShutdownCore::~AutoShutdownCore()
{
    stop();
}

void AutoShutdownCore::loadConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.beginGroup("Settings");
    m_idleTimeout = settings.value("idle_timeout", 300).toInt();
    m_checkInterval = settings.value("check_interval", 10).toInt();
    m_enabled = settings.value("enabled", true).toBool();
    m_tcpEnabled = settings.value("tcp_enabled", true).toBool();
    m_tcpPort = settings.value("tcp_port", 12345).toInt();
    m_tcpToken = settings.value("tcp_token", "secret123").toString();
    settings.endGroup();

    qDebug() << "Config loaded: idle_timeout =" << m_idleTimeout
             << ", check_interval =" << m_checkInterval
             << ", enabled =" << m_enabled
             << ", tcp_enabled =" << m_tcpEnabled
             << ", tcp_port =" << m_tcpPort
             << ", tcp_token =" << m_tcpToken;
}

void AutoShutdownCore::saveConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.beginGroup("Settings");
    settings.setValue("idle_timeout", m_idleTimeout);
    settings.setValue("check_interval", m_checkInterval);
    settings.setValue("enabled", m_enabled);
    settings.setValue("tcp_enabled", m_tcpEnabled);
    settings.setValue("tcp_port", m_tcpPort);
    settings.setValue("tcp_token", m_tcpToken);
    settings.endGroup();
    settings.sync();

    qDebug() << "Config saved: idle_timeout =" << m_idleTimeout
             << ", check_interval =" << m_checkInterval
             << ", enabled =" << m_enabled
             << ", tcp_enabled =" << m_tcpEnabled
             << ", tcp_port =" << m_tcpPort
             << ", tcp_token =" << m_tcpToken;
}

void AutoShutdownCore::start()
{
    if (m_running) return;
    m_running = true;
    m_timer->start(m_checkInterval * 1000);
    setupTcpServer();
    qDebug() << "AutoShutdown core started";
}

void AutoShutdownCore::stop()
{
    if (!m_running) return;
    m_running = false;
    m_timer->stop();
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    qDebug() << "AutoShutdown core stopped";
}

void AutoShutdownCore::setupTcpServer()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    if (m_tcpEnabled && m_running) {
        if (m_tcpServer->listen(QHostAddress::Any, static_cast<quint16>(m_tcpPort))) {
            qDebug() << "TCP Server listening on port" << m_tcpPort;
        } else {
            qWarning() << "TCP Server failed to listen on port" << m_tcpPort << ":" << m_tcpServer->errorString();
        }
    }
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

void AutoShutdownCore::setTcpEnabled(bool enabled)
{
    if (m_tcpEnabled == enabled) return;
    m_tcpEnabled = enabled;
    setupTcpServer();
    emit statusChanged(getStatus());
}

void AutoShutdownCore::setTcpPort(int port)
{
    if (m_tcpPort == port) return;
    m_tcpPort = port;
    setupTcpServer();
    emit statusChanged(getStatus());
}

void AutoShutdownCore::setTcpToken(const QString &token)
{
    if (m_tcpToken == token) return;
    m_tcpToken = token;
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

    status["tcp_enabled"] = m_tcpEnabled;
    status["tcp_port"] = m_tcpPort;
    status["tcp_token"] = m_tcpToken;
    status["tcp_listening"] = m_tcpServer ? m_tcpServer->isListening() : false;

    return status;
}

void AutoShutdownCore::onNewTcpConnection()
{
    while (m_tcpServer && m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &AutoShutdownCore::onTcpReadyRead);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void AutoShutdownCore::onTcpReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QString receivedToken = QString::fromUtf8(data).trimmed();

    qDebug() << "Received TCP payload from" << socket->peerAddress().toString() << ":" << receivedToken;

    if (!m_tcpToken.isEmpty() && receivedToken == m_tcpToken.trimmed()) {
        qWarning() << "Matching shutdown token received via TCP! Triggering immediate shutdown.";
        socket->write("SHUTDOWN_OK\n");
        socket->flush();
        socket->disconnectFromHost();

        shutdownNow();
    } else {
        qWarning() << "Invalid token received via TCP:" << receivedToken;
        socket->write("INVALID_TOKEN\n");
        socket->flush();
        socket->disconnectFromHost();
    }
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
    qWarning() << "Initiating standard shutdown (1 min delay)...";
    m_shutdownTriggered = true;
    emit statusChanged(getStatus());

    sendNotification("AutoShutdown", "Idle timeout reached. Shutdown triggered in 1 minute!");
    QProcess::startDetached("sudo", QStringList() << "shutdown" << "-h" << "+1" << "Auto shutdown due to inactivity");
}

void AutoShutdownCore::shutdownNow()
{
    qWarning() << "Initiating immediate shutdown via TCP command...";
    m_shutdownTriggered = true;
    emit statusChanged(getStatus());

    sendNotification("AutoShutdown", "Instant shutdown command received via TCP!");
    QProcess::startDetached("sudo", QStringList() << "shutdown" << "-h" << "now" << "Instant shutdown via TCP command");
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
