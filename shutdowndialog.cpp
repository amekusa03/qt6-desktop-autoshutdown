#include "shutdowndialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QApplication>

ShutdownDialog::ShutdownDialog(AutoShutdownCore *core, int seconds, QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint | Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint)
    , m_core(core)
    , m_remainingSeconds(seconds)
{
    setWindowTitle(QString::fromUtf8("シャットダウンの確認 - AutoShutdown"));
    setFixedSize(420, 270);

    buildUi();
    applyTheme();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ShutdownDialog::updateCountdown);
    m_timer->start(1000);

    if (m_core) {
        connect(m_core, &AutoShutdownCore::shutdownCancelled, this, &QDialog::accept);
    }
}

ShutdownDialog::~ShutdownDialog()
{
}

void ShutdownDialog::buildUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 警告アイコンとタイトル
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    QLabel *lblIcon = new QLabel(QString::fromUtf8("⚠️"), this);
    lblIcon->setStyleSheet("font-size: 32px;");
    headerLayout->addWidget(lblIcon);

    m_lblTitle = new QLabel(QString::fromUtf8("無操作シャットダウンの警告"), this);
    m_lblTitle->setObjectName("dialogTitle");
    m_lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #e94560;");
    headerLayout->addWidget(m_lblTitle, 1);

    mainLayout->addLayout(headerLayout);

    // 説明文
    m_lblMsg = new QLabel(QString::fromUtf8("無操作時間を検出したため、シャットダウンがスケジュールされました。\nシャットダウンをキャンセルしますか？"), this);
    m_lblMsg->setWordWrap(true);
    m_lblMsg->setStyleSheet("font-size: 12px; color: #eaeaea;");
    mainLayout->addWidget(m_lblMsg);

    // カウントダウン表示カード
    QFrame *countFrame = new QFrame(this);
    countFrame->setObjectName("countFrame");
    countFrame->setStyleSheet("background-color: #16213e; border-radius: 8px; padding: 6px;");
    QHBoxLayout *countLayout = new QHBoxLayout(countFrame);
    countLayout->setContentsMargins(12, 6, 12, 6);

    QLabel *lblRemText = new QLabel(QString::fromUtf8("自動シャットダウンまで:"), countFrame);
    lblRemText->setStyleSheet("font-size: 13px; color: #a0a0b8;");
    countLayout->addWidget(lblRemText);

    m_lblCountdown = new QLabel(QString::fromUtf8("残り %1 秒").arg(m_remainingSeconds), countFrame);
    m_lblCountdown->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblCountdown->setStyleSheet("font-size: 18px; font-weight: bold; color: #e94560;");
    countLayout->addWidget(m_lblCountdown, 1);

    mainLayout->addWidget(countFrame);

    // ボタン配置
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnCancel = new QPushButton(QString::fromUtf8("シャットダウンをキャンセル"), this);
    m_btnCancel->setObjectName("btnCancelDialog");
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setFixedHeight(36);

    m_btnNow = new QPushButton(QString::fromUtf8("今すぐシャットダウン"), this);
    m_btnNow->setObjectName("btnNowDialog");
    m_btnNow->setCursor(Qt::PointingHandCursor);
    m_btnNow->setFixedHeight(36);

    btnLayout->addWidget(m_btnCancel, 3);
    btnLayout->addWidget(m_btnNow, 2);

    mainLayout->addLayout(btnLayout);

    connect(m_btnCancel, &QPushButton::clicked, this, &ShutdownDialog::onCancelClicked);
    connect(m_btnNow, &QPushButton::clicked, this, &ShutdownDialog::onShutdownNowClicked);
}

void ShutdownDialog::applyTheme()
{
    QString qss = QString::fromUtf8(R"(
        QDialog {
            background-color: #1a1a2e;
            color: #eaeaea;
        }
        #btnCancelDialog {
            background-color: #e94560;
            color: #ffffff;
            font-weight: bold;
            border: none;
            border-radius: 6px;
        }
        #btnCancelDialog:hover {
            background-color: #ff6b81;
        }
        #btnNowDialog {
            background-color: #16213e;
            color: #a0a0b8;
            border: 1px solid #0f3460;
            border-radius: 6px;
        }
        #btnNowDialog:hover {
            background-color: #0f3460;
            color: #ffffff;
        }
    )");
    setStyleSheet(qss);
}

void ShutdownDialog::updateCountdown()
{
    m_remainingSeconds--;
    if (m_remainingSeconds <= 0) {
        m_lblCountdown->setText(QString::fromUtf8("残り 0 秒"));
        m_timer->stop();
        // タイムアウト: 即シャットダウン
        if (m_core) {
            m_core->shutdownNow();
        }
        accept();
    } else {
        m_lblCountdown->setText(QString::fromUtf8("残り %1 秒").arg(m_remainingSeconds));
    }
}

void ShutdownDialog::onCancelClicked()
{
    if (m_core) {
        m_core->cancelWarning();
    }
    accept();
}

void ShutdownDialog::onShutdownNowClicked()
{
    if (m_core) {
        m_core->shutdownNow();
    }
    accept();
}
