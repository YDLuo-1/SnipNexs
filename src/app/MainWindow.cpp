#include "MainWindow.h"

#include "AppIcon.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

namespace snipnexs {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("SnipNexs"));
    setWindowIcon(createAppIcon());
    setMinimumSize(640, 400);
    resize(760, 480);
    setupUi();
    setupTray();
}

void MainWindow::showAndActivate()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::setCaptureStatus(const QString& text)
{
    statusLabel_->setText(text);
}

void MainWindow::showNotification(const QString& title, const QString& message)
{
    if (trayIcon_ != nullptr && trayIcon_->isVisible()) {
        trayIcon_->showMessage(title, message, QSystemTrayIcon::Information, 2500);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (trayIcon_ != nullptr && trayIcon_->isVisible()) {
        hide();
        event->ignore();
        if (!trayHintShown_) {
            trayIcon_->showMessage(
                QStringLiteral("SnipNexs 仍在运行"),
                QStringLiteral("可从系统托盘重新打开或退出。"),
                QSystemTrayIcon::Information,
                2500);
            trayHintShown_ = true;
        }
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(40, 32, 40, 32);
    root->setSpacing(20);

    auto* title = new QLabel(QStringLiteral("SnipNexs"), central);
    title->setObjectName(QStringLiteral("title"));
    auto* subtitle = new QLabel(
        QStringLiteral("轻量、原生、可扩展的 Windows 截图工具"), central);
    subtitle->setObjectName(QStringLiteral("subtitle"));

    auto* card = new QFrame(central);
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 22, 24, 22);
    cardLayout->setSpacing(10);

    auto* stage = new QLabel(QStringLiteral("阶段 5 · 原生区域录屏"), card);
    stage->setObjectName(QStringLiteral("stage"));
    statusLabel_ = new QLabel(
        QStringLiteral("截图选区支持标注、贴图和本地文字识别。\n"
                       "区域录屏使用 Windows GPU 捕获并保存 H.264 MP4（当前不含音频）。"),
        card);
    statusLabel_->setWordWrap(true);
    cardLayout->addWidget(stage);
    cardLayout->addWidget(statusLabel_);

    auto* actions = new QHBoxLayout();
    actions->addStretch();
    auto* hideButton = new QPushButton(QStringLiteral("最小化到托盘"), central);
    hideButton->setObjectName(QStringLiteral("secondaryButton"));
    auto* captureButton = new QPushButton(QStringLiteral("区域截图  Ctrl+Shift+A"), central);
    captureButton->setObjectName(QStringLiteral("primaryButton"));
    auto* recordButton = new QPushButton(QStringLiteral("区域录屏"), central);
    recordButton->setObjectName(QStringLiteral("recordButton"));
    actions->addWidget(hideButton);
    actions->addWidget(recordButton);
    actions->addWidget(captureButton);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(card);
    root->addStretch();
    root->addLayout(actions);
    setCentralWidget(central);

    connect(hideButton, &QPushButton::clicked, this, &QWidget::hide);
    connect(recordButton, &QPushButton::clicked, this, &MainWindow::recordRequested);
    connect(captureButton, &QPushButton::clicked, this, &MainWindow::captureRequested);

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #11161d; color: #e8edf2; }
        QLabel { background: transparent; }
        QLabel#title { font-size: 34px; font-weight: 700; color: #ffffff; }
        QLabel#subtitle { font-size: 15px; color: #93a1af; }
        QFrame#card { background: #19212b; border: 1px solid #283440; border-radius: 12px; }
        QLabel#stage { font-size: 18px; font-weight: 600; color: #39d0be; }
        QFrame#card QLabel { font-size: 14px; color: #c5ced8; }
        QPushButton { min-height: 36px; padding: 0 18px; border-radius: 7px; font-weight: 600; }
        QPushButton#primaryButton { background: #39d0be; color: #0c2724; border: 0; }
        QPushButton#primaryButton:hover { background: #52dfce; }
        QPushButton#secondaryButton { background: transparent; color: #d8e0e7; border: 1px solid #3c4a58; }
        QPushButton#secondaryButton:hover { background: #202a35; }
        QPushButton#recordButton { background: #d84d57; color: #ffffff; border: 0; }
        QPushButton#recordButton:hover { background: #ea616b; }
    )"));
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QApplication::setQuitOnLastWindowClosed(true);
        return;
    }

    trayIcon_ = new QSystemTrayIcon(createAppIcon(), this);
    trayIcon_->setToolTip(QStringLiteral("SnipNexs"));

    auto* menu = new QMenu(this);
    menu->addAction(QStringLiteral("区域截图\tCtrl+Shift+A"), this, &MainWindow::captureRequested);
    menu->addAction(QStringLiteral("区域录屏"), this, &MainWindow::recordRequested);
    menu->addSeparator();
    menu->addAction(QStringLiteral("打开 SnipNexs"), this, &MainWindow::showAndActivate);
    menu->addSeparator();
    menu->addAction(QStringLiteral("退出"), qApp, &QApplication::quit);
    trayIcon_->setContextMenu(menu);

    connect(trayIcon_, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                showAndActivate();
            }
        });
    trayIcon_->show();
}

} // namespace snipnexs
