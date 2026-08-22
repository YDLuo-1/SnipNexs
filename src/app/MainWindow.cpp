#include "MainWindow.h"

#include "AppIcon.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
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
    retranslateUi();
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

void MainWindow::setLanguageCode(const QString& languageCode)
{
    languageCode_ = languageCode;
    const QSignalBlocker blocker(languageCombo_);
    const int index = languageCombo_->findData(languageCode_);
    if (index >= 0) {
        languageCombo_->setCurrentIndex(index);
    }
    retranslateUi();
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
                tr("SnipNexs 仍在运行"),
                tr("可从系统托盘重新打开或退出。"),
                QSystemTrayIcon::Information,
                2500);
            trayHintShown_ = true;
        }
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void MainWindow::retranslateUi()
{
    subtitleLabel_->setText(tr("轻量、原生、可扩展的 Windows 截图工具"));
    stageLabel_->setText(tr("阶段 5 · 原生区域录屏"));
    statusLabel_->setText(
        tr("截图选区支持标注、贴图和本地文字识别。\n"
           "区域录屏使用 Windows GPU 捕获并保存 H.264 MP4（当前不含音频）。"));
    languageLabel_->setText(tr("语言"));
    languageCombo_->setItemText(0, tr("简体中文"));
    languageCombo_->setItemText(1, QStringLiteral("English"));
    hideButton_->setText(tr("最小化到托盘"));
    captureButton_->setText(tr("区域截图  Ctrl+Shift+A"));
    recordButton_->setText(tr("区域录屏"));
    if (captureAction_ != nullptr) {
        captureAction_->setText(tr("区域截图\tCtrl+Shift+A"));
        recordAction_->setText(tr("区域录屏"));
        openAction_->setText(tr("打开 SnipNexs"));
        quitAction_->setText(tr("退出"));
    }
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(40, 32, 40, 32);
    root->setSpacing(20);

    auto* title = new QLabel(QStringLiteral("SnipNexs"), central);
    title->setObjectName(QStringLiteral("title"));
    subtitleLabel_ = new QLabel(central);
    subtitleLabel_->setObjectName(QStringLiteral("subtitle"));

    auto* card = new QFrame(central);
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 22, 24, 22);
    cardLayout->setSpacing(10);

    stageLabel_ = new QLabel(card);
    stageLabel_->setObjectName(QStringLiteral("stage"));
    statusLabel_ = new QLabel(card);
    statusLabel_->setWordWrap(true);
    cardLayout->addWidget(stageLabel_);
    cardLayout->addWidget(statusLabel_);

    auto* footer = new QVBoxLayout();
    footer->setSpacing(10);
    auto* languageRow = new QHBoxLayout();
    languageLabel_ = new QLabel(central);
    languageCombo_ = new QComboBox(central);
    languageCombo_->setObjectName(QStringLiteral("languageCombo"));
    languageCombo_->addItem(QString(), QStringLiteral("zh_CN"));
    languageCombo_->addItem(QString(), QStringLiteral("en"));
    languageRow->addWidget(languageLabel_);
    languageRow->addWidget(languageCombo_);
    languageRow->addStretch();
    auto* actions = new QHBoxLayout();
    actions->addStretch();
    hideButton_ = new QPushButton(central);
    hideButton_->setObjectName(QStringLiteral("secondaryButton"));
    captureButton_ = new QPushButton(central);
    captureButton_->setObjectName(QStringLiteral("primaryButton"));
    recordButton_ = new QPushButton(central);
    recordButton_->setObjectName(QStringLiteral("recordButton"));
    actions->addWidget(hideButton_);
    actions->addWidget(recordButton_);
    actions->addWidget(captureButton_);
    footer->addLayout(languageRow);
    footer->addLayout(actions);

    root->addWidget(title);
    root->addWidget(subtitleLabel_);
    root->addWidget(card);
    root->addStretch();
    root->addLayout(footer);
    setCentralWidget(central);

    connect(hideButton_, &QPushButton::clicked, this, &QWidget::hide);
    connect(recordButton_, &QPushButton::clicked, this, &MainWindow::recordRequested);
    connect(captureButton_, &QPushButton::clicked, this, &MainWindow::captureRequested);
    connect(languageCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString languageCode = languageCombo_->itemData(index).toString();
        if (!languageCode.isEmpty() && languageCode != languageCode_) {
            emit languageChangeRequested(languageCode);
        }
    });

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #11161d; color: #e8edf2; }
        QLabel { background: transparent; }
        QLabel#title { font-size: 34px; font-weight: 700; color: #ffffff; }
        QLabel#subtitle { font-size: 15px; color: #93a1af; }
        QFrame#card { background: #19212b; border: 1px solid #283440; border-radius: 12px; }
        QLabel#stage { font-size: 18px; font-weight: 600; color: #39d0be; }
        QFrame#card QLabel { font-size: 14px; color: #c5ced8; }
        QComboBox { min-height: 34px; padding: 0 10px; color: #e8edf2; background: #19212b; border: 1px solid #3c4a58; border-radius: 6px; }
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
    captureAction_ = menu->addAction({}, this, &MainWindow::captureRequested);
    recordAction_ = menu->addAction({}, this, &MainWindow::recordRequested);
    menu->addSeparator();
    openAction_ = menu->addAction({}, this, &MainWindow::showAndActivate);
    menu->addSeparator();
    quitAction_ = menu->addAction({}, qApp, &QApplication::quit);
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
