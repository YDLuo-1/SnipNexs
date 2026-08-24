#include "MainWindow.h"

#include "AboutDialog.h"
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
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

namespace snipnexs {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("SnipNexs"));
    setWindowIcon(createAppIcon());
    setMinimumSize(520, 340);
    resize(600, 380);
    setupUi();
    setupTray();
    retranslateUi();
}

void MainWindow::showAndActivate()
{
    if (captureActive_) {
        return;
    }
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::setCaptureActive(bool active)
{
    captureActive_ = active;
    if (openAction_ != nullptr) {
        openAction_->setEnabled(!active);
        aboutAction_->setEnabled(!active);
    }
    if (aboutButton_ != nullptr) {
        aboutButton_->setEnabled(!active);
    }
    if (active) {
        hide();
    }
}

void MainWindow::setCaptureShortcut(const QString& shortcut)
{
    captureShortcut_ = shortcut;
    retranslateUi();
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
    statusLabel_->setText(
        tr("截图选区支持标注、贴图和本地文字识别。\n"
           "区域录屏使用 Windows GPU 捕获并保存 H.264 MP4（当前不含音频）。"));
    languageLabel_->setText(tr("语言"));
    languageCombo_->setItemText(0, tr("简体中文"));
    languageCombo_->setItemText(1, QStringLiteral("English"));
    hideButton_->setText(tr("最小化到托盘"));
    const QString captureText = tr("区域截图");
    captureButton_->setText(captureShortcut_.isEmpty()
        ? captureText
        : QStringLiteral("%1  %2").arg(captureText, captureShortcut_));
    recordButton_->setText(tr("区域录屏"));
    if (captureAction_ != nullptr) {
        captureAction_->setText(captureShortcut_.isEmpty()
            ? captureText
            : QStringLiteral("%1\t%2").arg(captureText, captureShortcut_));
        recordAction_->setText(tr("区域录屏"));
        openAction_->setText(tr("打开 SnipNexs"));
        aboutAction_->setText(tr("关于 SnipNexs"));
        quitAction_->setText(tr("退出"));
    }
    aboutButton_->setText(tr("关于"));
}

void MainWindow::showAbout()
{
    if (captureActive_) {
        return;
    }
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(14);

    auto* header = new QHBoxLayout();
    header->setSpacing(10);
    auto* title = new QLabel(QStringLiteral("SnipNexs"), central);
    title->setObjectName(QStringLiteral("title"));
    header->addWidget(title);
    header->addStretch();
    languageLabel_ = new QLabel(central);
    languageCombo_ = new QComboBox(central);
    languageCombo_->setObjectName(QStringLiteral("languageCombo"));
    languageCombo_->addItem(QString(), QStringLiteral("zh_CN"));
    languageCombo_->addItem(QString(), QStringLiteral("en"));
    header->addWidget(languageLabel_);
    header->addWidget(languageCombo_);
    root->addLayout(header);

    subtitleLabel_ = new QLabel(central);
    subtitleLabel_->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(subtitleLabel_);

    root->addStretch();

    auto* actions = new QHBoxLayout();
    actions->setSpacing(12);
    captureButton_ = new QPushButton(central);
    captureButton_->setObjectName(QStringLiteral("primaryButton"));
    captureButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    recordButton_ = new QPushButton(central);
    recordButton_->setObjectName(QStringLiteral("recordButton"));
    QPixmap recordDot(10, 10);
    recordDot.fill(Qt::transparent);
    {
        QPainter dotPainter(&recordDot);
        dotPainter.setRenderHint(QPainter::Antialiasing);
        dotPainter.setPen(Qt::NoPen);
        dotPainter.setBrush(QColor(232, 76, 61));
        dotPainter.drawEllipse(0, 0, 10, 10);
    }
    recordButton_->setIcon(recordDot);
    recordButton_->setIconSize(QSize(10, 10));
    recordButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    actions->addWidget(captureButton_);
    actions->addWidget(recordButton_);
    root->addLayout(actions);

    auto* card = new QFrame(central);
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    statusLabel_ = new QLabel(card);
    statusLabel_->setWordWrap(true);
    cardLayout->addWidget(statusLabel_);
    root->addWidget(card);

    auto* footer = new QHBoxLayout();
    footer->setSpacing(10);
    hideButton_ = new QPushButton(central);
    hideButton_->setObjectName(QStringLiteral("secondaryButton"));
    footer->addWidget(hideButton_);
    footer->addStretch();
    aboutButton_ = new QPushButton(central);
    aboutButton_->setObjectName(QStringLiteral("aboutButton"));
    footer->addWidget(aboutButton_);
    root->addLayout(footer);

    setCentralWidget(central);

    connect(hideButton_, &QPushButton::clicked, this, &QWidget::hide);
    connect(recordButton_, &QPushButton::clicked, this, &MainWindow::recordRequested);
    connect(captureButton_, &QPushButton::clicked, this, &MainWindow::captureRequested);
    connect(aboutButton_, &QPushButton::clicked, this, &MainWindow::showAbout);
    connect(languageCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString languageCode = languageCombo_->itemData(index).toString();
        if (!languageCode.isEmpty() && languageCode != languageCode_) {
            emit languageChangeRequested(languageCode);
        }
    });

    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #12161c; color: #e8edf2; }
        QLabel { background: transparent; }
        QLabel#title { font-size: 22px; font-weight: 700; color: #ffffff; }
        QLabel#subtitle { font-size: 13px; color: #bdc8d2; }
        QLabel#languageLabel { font-size: 13px; color: #bdc8d2; }
        QComboBox { min-height: 30px; padding: 0 10px; color: #e8edf2; background: #181f27; border: 1px solid #2c3947; border-radius: 6px; }
        QComboBox:hover { border-color: #3c4c5c; }
        QComboBox QAbstractItemView { color: #f1f5f9; background: #181f27; selection-background-color: #2c5960; selection-color: #ffffff; }
        QPushButton { min-height: 42px; padding: 0 18px; border-radius: 8px; font-weight: 600; font-size: 14px; }
        QPushButton#primaryButton { background: #39d0be; color: #0b2622; border: 0; }
        QPushButton#primaryButton:hover { background: #4fdccd; }
        QPushButton#primaryButton:pressed { background: #2fb8a7; }
        QPushButton#recordButton { background: #1c2530; color: #e8edf2; border: 1px solid #2c3947; }
        QPushButton#recordButton:hover { background: #243040; border-color: #3c4c5c; }
        QPushButton#recordButton:pressed { background: #1a222c; }
        QPushButton#secondaryButton, QPushButton#aboutButton { min-height: 32px; padding: 0 14px; background: transparent; color: #c2ccd6; border: 1px solid #3a4856; font-weight: 400; font-size: 13px; }
        QPushButton#secondaryButton:hover, QPushButton#aboutButton:hover { background: #1a222c; color: #ffffff; }
        QFrame#card { background: #181f27; border: 1px solid #263039; border-radius: 10px; }
        QFrame#card QLabel { font-size: 13px; color: #cbd4dc; }
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
    aboutAction_ = menu->addAction({}, this, &MainWindow::showAbout);
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
