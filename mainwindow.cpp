#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(AutoShutdownCore *core, QWidget *parent)
    : QMainWindow(parent)
    , m_core(core)
    , m_shutdownDialog(nullptr)
{
    setWindowTitle("AutoShutdown");
    setFixedSize(480, 680);

    buildUi();
    setupTrayIcon();
    applyTheme();

    // コアからのステータス通知を接続
    connect(m_core, &AutoShutdownCore::statusChanged, this, &MainWindow::updateDisplay);
    connect(m_core, &AutoShutdownCore::shutdownRequested, this, &MainWindow::showShutdownDialog);

    // 初期状態の表示
    updateDisplay(m_core->getStatus());
}

MainWindow::~MainWindow()
{
}

void MainWindow::buildUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---------------------------
    // ヘッダー部
    // ---------------------------
    QFrame *headerFrame = new QFrame(centralWidget);
    headerFrame->setObjectName("headerFrame");
    headerFrame->setFixedHeight(100);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(0, 10, 0, 10);
    headerLayout->setSpacing(2);

    QLabel *lblIcon = new QLabel("⏻", headerFrame);
    lblIcon->setAlignment(Qt::AlignCenter);
    lblIcon->setStyleSheet("font-size: 32px; color: #e94560;");
    headerLayout->addWidget(lblIcon);

    QLabel *lblTitle = new QLabel("Auto Shutdown", headerFrame);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #eaeaea;");
    headerLayout->addWidget(lblTitle);

    QLabel *lblSub = new QLabel("無操作検出・TCPコマンド対応シャットダウン", headerFrame);
    lblSub->setAlignment(Qt::AlignCenter);
    lblSub->setStyleSheet("font-size: 11px; color: #a0a0b8;");
    headerLayout->addWidget(lblSub);

    mainLayout->addWidget(headerFrame);

    // ---------------------------
    // コンテンツ部
    // ---------------------------
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(24, 12, 24, 12);
    contentLayout->setSpacing(10);

    // ステータスカード
    QFrame *statusFrame = new QFrame(centralWidget);
    statusFrame->setObjectName("statusFrame");
    statusFrame->setFixedHeight(95);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(16, 8, 16, 8);
    statusLayout->setSpacing(4);

    QLabel *lblStatusTitle = new QLabel("現在のステータス", statusFrame);
    lblStatusTitle->setStyleSheet("font-size: 10px; color: #606080;");
    statusLayout->addWidget(lblStatusTitle);

    QHBoxLayout *statusRow = new QHBoxLayout();
    
    // アイドル時間カラム
    QVBoxLayout *idleCol = new QVBoxLayout();
    QLabel *lblIdleTitle = new QLabel("アイドル時間", statusFrame);
    lblIdleTitle->setAlignment(Qt::AlignCenter);
    lblIdleTitle->setStyleSheet("font-size: 10px; color: #606080;");
    m_lblIdle = new QLabel("--:--", statusFrame);
    m_lblIdle->setAlignment(Qt::AlignCenter);
    m_lblIdle->setStyleSheet("font-size: 24px; font-weight: bold; color: #eaeaea;");
    idleCol->addWidget(lblIdleTitle);
    idleCol->addWidget(m_lblIdle);
    statusRow->addLayout(idleCol);

    // 仕切り線
    QFrame *divider = new QFrame(statusFrame);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Sunken);
    divider->setStyleSheet("color: #606080;");
    statusRow->addWidget(divider);

    // 残り時間カラム
    QVBoxLayout *remCol = new QVBoxLayout();
    QLabel *lblRemTitle = new QLabel("シャットダウンまで", statusFrame);
    lblRemTitle->setAlignment(Qt::AlignCenter);
    lblRemTitle->setStyleSheet("font-size: 10px; color: #606080;");
    m_lblRemaining = new QLabel("--:--", statusFrame);
    m_lblRemaining->setAlignment(Qt::AlignCenter);
    m_lblRemaining->setStyleSheet("font-size: 24px; font-weight: bold; color: #2ecc71;");
    remCol->addWidget(lblRemTitle);
    remCol->addWidget(m_lblRemaining);
    statusRow->addLayout(remCol);

    statusLayout->addLayout(statusRow);
    contentLayout->addWidget(statusFrame);

    // 警告メッセージ
    m_lblMsg = new QLabel("", centralWidget);
    m_lblMsg->setAlignment(Qt::AlignCenter);
    m_lblMsg->setStyleSheet("font-size: 11px; color: #e74c3c; font-weight: bold;");
    m_lblMsg->setFixedHeight(18);
    contentLayout->addWidget(m_lblMsg);

    // コントロール：有効トグル
    QHBoxLayout *toggleRow = new QHBoxLayout();
    QLabel *lblToggle = new QLabel("自動シャットダウン", centralWidget);
    lblToggle->setStyleSheet("font-size: 13px; color: #eaeaea;");
    m_toggleBtn = new QPushButton("OFF", centralWidget);
    m_toggleBtn->setObjectName("toggleBtn");
    m_toggleBtn->setFixedWidth(70);
    m_toggleBtn->setProperty("enabled", false);
    connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::toggleEnabled);
    toggleRow->addWidget(lblToggle);
    toggleRow->addWidget(m_toggleBtn);
    contentLayout->addLayout(toggleRow);

    // コントロール：無操作タイムアウト
    QLabel *lblTimeout = new QLabel("無操作タイムアウト", centralWidget);
    lblTimeout->setStyleSheet("font-size: 13px; color: #eaeaea;");
    contentLayout->addWidget(lblTimeout);

    QHBoxLayout *timeoutRow = new QHBoxLayout();
    m_sliderTimeout = new QSlider(Qt::Horizontal, centralWidget);
    m_sliderTimeout->setRange(60, 3600);
    m_sliderTimeout->setValue(m_core->idleTimeout());
    connect(m_sliderTimeout, &QSlider::valueChanged, this, &MainWindow::onTimeoutSliderChanged);
    
    m_lblTimeoutVal = new QLabel(formatTime(m_sliderTimeout->value()), centralWidget);
    m_lblTimeoutVal->setFixedWidth(80);
    m_lblTimeoutVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblTimeoutVal->setStyleSheet("font-family: Monospace; font-size: 12px; color: #e94560;");
    
    timeoutRow->addWidget(m_sliderTimeout);
    timeoutRow->addWidget(m_lblTimeoutVal);
    contentLayout->addLayout(timeoutRow);

    // コントロール：チェック間隔
    QLabel *lblInterval = new QLabel("チェック間隔", centralWidget);
    lblInterval->setStyleSheet("font-size: 13px; color: #eaeaea;");
    contentLayout->addWidget(lblInterval);

    QHBoxLayout *intervalRow = new QHBoxLayout();
    m_sliderInterval = new QSlider(Qt::Horizontal, centralWidget);
    m_sliderInterval->setRange(5, 60);
    m_sliderInterval->setValue(m_core->checkInterval());
    connect(m_sliderInterval, &QSlider::valueChanged, this, &MainWindow::onIntervalSliderChanged);
    
    m_lblIntervalVal = new QLabel(QString::number(m_sliderInterval->value()) + "秒", centralWidget);
    m_lblIntervalVal->setFixedWidth(80);
    m_lblIntervalVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblIntervalVal->setStyleSheet("font-family: Monospace; font-size: 12px; color: #e94560;");
    
    intervalRow->addWidget(m_sliderInterval);
    intervalRow->addWidget(m_lblIntervalVal);
    contentLayout->addLayout(intervalRow);

    // ---------------------------
    // TCP リモート設定セクション
    // ---------------------------
    QFrame *tcpSectionLine = new QFrame(centralWidget);
    tcpSectionLine->setFrameShape(QFrame::HLine);
    tcpSectionLine->setFrameShadow(QFrame::Sunken);
    tcpSectionLine->setStyleSheet("color: #0f3460;");
    contentLayout->addWidget(tcpSectionLine);

    m_chkTcpEnabled = new QCheckBox("TCPリモート制御を有効化", centralWidget);
    m_chkTcpEnabled->setChecked(m_core->isTcpEnabled());
    contentLayout->addWidget(m_chkTcpEnabled);

    QHBoxLayout *tcpPortRow = new QHBoxLayout();
    QLabel *lblTcpPort = new QLabel("TCPポート", centralWidget);
    lblTcpPort->setStyleSheet("font-size: 12px; color: #eaeaea;");
    m_spinTcpPort = new QSpinBox(centralWidget);
    m_spinTcpPort->setRange(1, 65535);
    m_spinTcpPort->setValue(m_core->tcpPort());
    m_spinTcpPort->setFixedWidth(100);
    tcpPortRow->addWidget(lblTcpPort);
    tcpPortRow->addStretch();
    tcpPortRow->addWidget(m_spinTcpPort);
    contentLayout->addLayout(tcpPortRow);

    QHBoxLayout *tcpTokenRow = new QHBoxLayout();
    QLabel *lblTcpToken = new QLabel("シャットダウントークン", centralWidget);
    lblTcpToken->setStyleSheet("font-size: 12px; color: #eaeaea;");
    m_txtTcpToken = new QLineEdit(centralWidget);
    m_txtTcpToken->setText(m_core->tcpToken());
    m_txtTcpToken->setPlaceholderText("secret123");
    m_txtTcpToken->setFixedWidth(160);
    tcpTokenRow->addWidget(lblTcpToken);
    tcpTokenRow->addStretch();
    tcpTokenRow->addWidget(m_txtTcpToken);
    contentLayout->addLayout(tcpTokenRow);

    m_lblTcpStatus = new QLabel("", centralWidget);
    m_lblTcpStatus->setStyleSheet("font-size: 11px; color: #606080;");
    contentLayout->addWidget(m_lblTcpStatus);

    // ボタン行（保存・キャンセル）
    QHBoxLayout *btnRow = new QHBoxLayout();
    m_saveBtn = new QPushButton("✓  設定を保存", centralWidget);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::applySettings);
    btnRow->addWidget(m_saveBtn);

    m_cancelBtn = new QPushButton("✕  キャンセル", centralWidget);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setVisible(false);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::cancelShutdown);
    btnRow->addWidget(m_cancelBtn);
    contentLayout->addLayout(btnRow);

    // フッター操作行
    QHBoxLayout *footerRow = new QHBoxLayout();
    m_logBtn = new QPushButton("📋  ログを開く", centralWidget);
    m_logBtn->setObjectName("logBtn");
    m_logBtn->setFixedWidth(120);
    connect(m_logBtn, &QPushButton::clicked, this, &MainWindow::openLog);
    footerRow->addWidget(m_logBtn);
    footerRow->addStretch();

    contentLayout->addLayout(footerRow);

    mainLayout->addLayout(contentLayout);

    // コピーライト
    QLabel *lblCopy = new QLabel("AutoShutdown  •  MIT License", centralWidget);
    lblCopy->setAlignment(Qt::AlignCenter);
    lblCopy->setStyleSheet("font-size: 9px; color: #606080; margin-bottom: 8px;");
    mainLayout->addWidget(lblCopy);
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(createTrayIcon(m_core->isEnabled(), false));
    m_trayIcon->setToolTip("AutoShutdown");

    m_trayMenu = new QMenu(this);
    m_trayMenu->setStyleSheet("QMenu { background-color: #16213e; color: #eaeaea; border: 1px solid #0f3460; }"
                              "QMenu::item:selected { background-color: #e94560; color: white; }");

    QAction *actOpen = new QAction("設定を開く", this);
    connect(actOpen, &QAction::triggered, this, &MainWindow::showNormal);
    m_trayMenu->addAction(actOpen);

    m_toggleAction = new QAction("✓ 有効", this);
    connect(m_toggleAction, &QAction::triggered, this, &MainWindow::toggleEnabled);
    m_trayMenu->addAction(m_toggleAction);

    m_trayMenu->addSeparator();

    QAction *actLog = new QAction("ログを開く", this);
    connect(actLog, &QAction::triggered, this, &MainWindow::openLog);
    m_trayMenu->addAction(actLog);

    QAction *actQuit = new QAction("終了", this);
    connect(actQuit, &QAction::triggered, this, &MainWindow::quitApp);
    m_trayMenu->addAction(actQuit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
}

void MainWindow::applyTheme()
{
    QString qss = R"(
        QMainWindow {
            background-color: #1a1b2e;
        }
        #headerFrame {
            background-color: #16213e;
            border-bottom: 1px solid #0f3460;
        }
        #statusFrame {
            background-color: #0f3460;
            border-radius: 8px;
        }
        QPushButton {
            background-color: #e94560;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #ff6b81;
        }
        QPushButton:pressed {
            background-color: #d13b54;
        }
        #cancelBtn {
            background-color: #e74c3c;
        }
        #cancelBtn:hover {
            background-color: #ff6b6b;
        }
        #cancelBtn:pressed {
            background-color: #c0392b;
        }
        #toggleBtn {
            font-weight: bold;
            font-size: 11px;
            border-radius: 4px;
            padding: 4px;
        }
        #toggleBtn[enabled="true"] {
            background-color: #2ecc71;
        }
        #toggleBtn[enabled="true"]:hover {
            background-color: #2ee287;
        }
        #toggleBtn[enabled="false"] {
            background-color: #16213e;
            color: #606080;
            border: 1px solid #0f3460;
        }
        #logBtn {
            background-color: #16213e;
            color: #a0a0b8;
            border: 1px solid #0f3460;
            font-size: 10px;
            padding: 6px;
        }
        #logBtn:hover {
            background-color: #0f3460;
            color: #eaeaea;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 6px;
            background: #16213e;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #e94560;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #ff6b81;
        }
        QSlider::sub-page:horizontal {
            background: #e94560;
            border-radius: 3px;
        }
        QLineEdit, QSpinBox {
            background-color: #16213e;
            color: #eaeaea;
            border: 1px solid #0f3460;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QCheckBox {
            color: #eaeaea;
            font-size: 12px;
        }
    )";
    setStyleSheet(qss);
}

QIcon MainWindow::createTrayIcon(bool enabled, bool warning)
{
    int size = 64;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor = enabled ? QColor(233, 69, 96) : QColor(80, 80, 100);
    if (warning) {
        bgColor = QColor(243, 156, 18);
    }

    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, size - 4, size - 4);

    painter.setPen(QPen(Qt::white, 4, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);

    int r_outer = size / 2 - 12;
    QRectF arcRect(size/2 - r_outer, size/2 - r_outer, r_outer * 2, r_outer * 2);
    painter.drawArc(arcRect, (90 - 130) * 16, (360 - 100) * 16);

    painter.drawLine(size/2, size/2 - r_outer - 2, size/2, size/2 - (r_outer - 8));

    return QIcon(pixmap);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::updateDisplay(const QVariantMap &status)
{
    bool enabled = status["enabled"].toBool();
    QVariant idle = status["idle_time"];
    QVariant remaining = status["remaining"];
    bool warning = status["shutdown_triggered"].toBool();
    bool warningShown = status["warning_shown"].toBool();

    bool tcpEnabled = status["tcp_enabled"].toBool();
    int tcpPort = status["tcp_port"].toInt();
    bool tcpListening = status["tcp_listening"].toBool();
    QString tcpToken = status["tcp_token"].toString();

    // 有効/無効トグルボタン更新
    m_toggleBtn->setText(enabled ? "ON" : "OFF");
    m_toggleBtn->setProperty("enabled", enabled);
    m_toggleBtn->style()->unpolish(m_toggleBtn);
    m_toggleBtn->style()->polish(m_toggleBtn);

    m_toggleAction->setText(enabled ? "✓ 有効" : "✗ 無効");

    // アイドル時間更新
    if (idle.isValid() && enabled) {
        m_lblIdle->setText(formatTime(idle.toDouble()));
    } else {
        m_lblIdle->setText("--:--");
    }

    // 残り時間更新
    if (remaining.isValid() && enabled) {
        double remSec = remaining.toDouble();
        m_lblRemaining->setText(formatTime(remSec));

        double pct = remSec / qMax(1, m_core->idleTimeout());
        if (pct > 0.3) {
            m_lblRemaining->setStyleSheet("font-size: 24px; font-weight: bold; color: #2ecc71;"); // 緑
        } else if (pct > 0.1) {
            m_lblRemaining->setStyleSheet("font-size: 24px; font-weight: bold; color: #f39c12;"); // オレンジ
        } else {
            m_lblRemaining->setStyleSheet("font-size: 24px; font-weight: bold; color: #e74c3c;"); // 赤
        }
    } else {
        m_lblRemaining->setText("--:--");
        m_lblRemaining->setStyleSheet("font-size: 24px; font-weight: bold; color: #eaeaea;");
    }

    // TCPステータス表示
    if (m_lblTcpStatus) {
        if (tcpEnabled) {
            if (tcpListening) {
                m_lblTcpStatus->setText(QString("🟢 待受中 (ポート: %1)").arg(tcpPort));
                m_lblTcpStatus->setStyleSheet("font-size: 11px; color: #2ecc71;");
            } else {
                m_lblTcpStatus->setText(QString("🔴 ポート %1 でバインド失敗").arg(tcpPort));
                m_lblTcpStatus->setStyleSheet("font-size: 11px; color: #e74c3c;");
            }
        } else {
            m_lblTcpStatus->setText("⚪ TCPリモート制御は無効");
            m_lblTcpStatus->setStyleSheet("font-size: 11px; color: #606080;");
        }
    }

    // 警告メッセージとキャンセルボタン
    if (warning) {
        m_lblMsg->setText("⚠  シャットダウンが開始されました！");
        m_cancelBtn->setVisible(true);
    } else if (warningShown) {
        m_lblMsg->setText("⚠  シャットダウンまで1分を切りました！");
        m_cancelBtn->setVisible(true);
    } else {
        m_lblMsg->setText("");
        m_cancelBtn->setVisible(false);
    }

    // ダイアログの表示/非表示
    if (warningShown && !warning) {
        // 残り60秒に達した: ダイアログを表示
        if (!m_shutdownDialog) {
            showShutdownDialog(60);
        }
    } else if (!warningShown && !warning) {
        // キャンセルされた: ダイアログを閉じる
        if (m_shutdownDialog) {
            m_shutdownDialog->close();
            m_shutdownDialog = nullptr;
        }
    }

    // トレイアイコンの更新
    m_trayIcon->setIcon(createTrayIcon(enabled, warning));
}

void MainWindow::onTimeoutSliderChanged(int value)
{
    m_lblTimeoutVal->setText(formatTime(value));
}

void MainWindow::onIntervalSliderChanged(int value)
{
    m_lblIntervalVal->setText(QString::number(value) + "秒");
}

void MainWindow::toggleEnabled()
{
    bool nextState = !m_core->isEnabled();
    m_core->setEnabled(nextState);
}

void MainWindow::applySettings()
{
    m_core->setIdleTimeout(m_sliderTimeout->value());
    m_core->setCheckInterval(m_sliderInterval->value());
    m_core->setEnabled(m_toggleBtn->property("enabled").toBool());

    if (m_chkTcpEnabled) {
        m_core->setTcpEnabled(m_chkTcpEnabled->isChecked());
    }
    if (m_spinTcpPort) {
        m_core->setTcpPort(m_spinTcpPort->value());
    }
    if (m_txtTcpToken) {
        m_core->setTcpToken(m_txtTcpToken->text().trimmed());
    }

    m_core->saveConfig();

    m_lblMsg->setStyleSheet("font-size: 11px; color: #2ecc71; font-weight: bold;");
    m_lblMsg->setText("✓  設定を保存しました");
    QTimer::singleShot(3000, this, [this]() {
        m_lblMsg->setStyleSheet("font-size: 11px; color: #e74c3c; font-weight: bold;");
        m_lblMsg->setText("");
    });
}

void MainWindow::cancelShutdown()
{
    m_core->cancelShutdown();
}

void MainWindow::openLog()
{
    QString logPath = QDir::home().filePath(".local/var/log/auto_shutdown.log");
    QProcess::startDetached("xdg-open", QStringList() << logPath);
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            raise();
            activateWindow();
        }
    }
}

void MainWindow::quitApp()
{
    m_core->stop();
    m_trayIcon->hide();
    QApplication::quit();
}

QString MainWindow::formatTime(double seconds)
{
    int sec = static_cast<int>(seconds);
    int m = sec / 60;
    int s = sec % 60;
    int h = m / 60;
    m = m % 60;

    if (h > 0) {
        return QString("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }
    return QString("%1:%2")
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void MainWindow::showShutdownDialog(int secondsRemaining)
{
    if (m_shutdownDialog) {
        m_shutdownDialog->close();
        m_shutdownDialog = nullptr;
    }

    m_shutdownDialog = new ShutdownDialog(m_core, secondsRemaining, nullptr);
    m_shutdownDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_shutdownDialog, &QDialog::destroyed, this, [this]() {
        m_shutdownDialog = nullptr;
    });

    m_shutdownDialog->show();
    m_shutdownDialog->raise();
    m_shutdownDialog->activateWindow();
}
