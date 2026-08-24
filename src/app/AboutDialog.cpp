#include "AboutDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace snipnexs {

namespace {

constexpr auto kRepositoryUrl = "https://github.com/YDLuo-1/SnipNexs";
constexpr auto kQtSourceUrl =
    "https://github.com/YDLuo-1/SnipNexs/releases/download/v0.6.0/"
    "qtbase-everywhere-src-6.11.2.tar.xz";

QString resourceText(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly | QIODevice::Text)
        ? QString::fromUtf8(file.readAll())
        : QString();
}

QTextBrowser* createLicenseBrowser(const QString& heading, const QString& summary,
    const QString& licenseText, QWidget* parent)
{
    auto* browser = new QTextBrowser(parent);
    browser->setObjectName(QStringLiteral("licenseBrowser"));
    browser->setOpenExternalLinks(true);
    browser->setHtml(QStringLiteral(
        "<h2>%1</h2><p>%2</p><pre style='white-space:pre-wrap'>%3</pre>")
            .arg(heading.toHtmlEscaped(), summary, licenseText.toHtmlEscaped()));
    return browser;
}

}

OpenSourceLicensesDialog::OpenSourceLicensesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("开源许可"));
    setMinimumSize(720, 520);
    resize(820, 620);

    auto* root = new QVBoxLayout(this);
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QStringLiteral("licenseTabs"));

    const QString sourceLink = QStringLiteral("<a href='%1'>%1</a>")
                                   .arg(QString::fromLatin1(kRepositoryUrl));
    tabs_->addTab(
        createLicenseBrowser(
            QStringLiteral("SnipNexs"),
            tr("SnipNexs 采用 GPL-3.0-or-later。对应版本源码：%1").arg(sourceLink),
            resourceText(QStringLiteral(":/licenses/LICENSE")),
            tabs_),
        tr("SnipNexs"));

    const QString qtSourceLink = QStringLiteral("<a href='%1'>%1</a>")
                                     .arg(QString::fromLatin1(kQtSourceUrl));
    const QString qtSummary = tr(
        "本发行版动态链接 Qt 6.11.2 Core、Gui、Widgets 与 Network，采用 "
        "LGPL-3.0-only。用户可以替换兼容的 Qt DLL；SnipNexs 未修改 Qt。"
        "对应 Qt 源码副本：%1").arg(qtSourceLink)
        ;
    const QString qtLicenseText =
        resourceText(QStringLiteral(
            ":/licenses/licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md"))
        + QStringLiteral("\n\n")
        + resourceText(QStringLiteral(":/licenses/licenses/LGPL-3.0-only.txt"));
    tabs_->addTab(
        createLicenseBrowser(
            QStringLiteral("Qt 6.11.2"),
            qtSummary,
            qtLicenseText,
            tabs_),
        QStringLiteral("Qt"));

    tabs_->addTab(
        createLicenseBrowser(
            QStringLiteral("Microsoft SimpleRecorder"),
            tr("录屏管线设计参考了固定提交版本的 Microsoft SimpleRecorder 示例；"
               "SnipNexs 不分发该示例的二进制文件。"),
            resourceText(QStringLiteral(":/licenses/licenses/MIT-Microsoft-SimpleRecorder.txt")),
            tabs_),
        QStringLiteral("SimpleRecorder"));

    root->addWidget(tabs_);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    closeButton_ = buttons->button(QDialogButtonBox::Close);
    closeButton_->setText(tr("关闭"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setModal(true);
    setMinimumWidth(500);
    setWindowIcon(parent != nullptr ? parent->windowIcon() : windowIcon());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 22);
    root->setSpacing(12);

    productLabel_ = new QLabel(this);
    productLabel_->setObjectName(QStringLiteral("aboutProduct"));
    productLabel_->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: 700;"));
    copyrightLabel_ = new QLabel(this);
    licenseLabel_ = new QLabel(this);
    licenseLabel_->setWordWrap(true);
    sourceLabel_ = new QLabel(this);
    sourceLabel_->setOpenExternalLinks(true);

    root->addWidget(productLabel_);
    root->addWidget(copyrightLabel_);
    root->addWidget(licenseLabel_);
    root->addWidget(sourceLabel_);
    root->addSpacing(8);

    auto* actions = new QHBoxLayout();
    licensesButton_ = new QPushButton(this);
    licensesButton_->setObjectName(QStringLiteral("licensesButton"));
    aboutQtButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);
    actions->addWidget(licensesButton_);
    actions->addWidget(aboutQtButton_);
    actions->addStretch();
    actions->addWidget(closeButton_);
    root->addLayout(actions);

    connect(licensesButton_, &QPushButton::clicked,
        this, &AboutDialog::showOpenSourceLicenses);
    connect(aboutQtButton_, &QPushButton::clicked, this, [this]() {
        QMessageBox::aboutQt(this, tr("关于 Qt"));
    });
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    retranslateUi();
}

void AboutDialog::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void AboutDialog::retranslateUi()
{
    setWindowTitle(tr("关于 SnipNexs"));
    productLabel_->setText(
        tr("SnipNexs %1（64 位 Windows）").arg(QStringLiteral(SNIPNEXS_VERSION)));
    copyrightLabel_->setText(QStringLiteral("© 2026 SnipNexs contributors"));
    licenseLabel_->setText(tr(
        "本程序采用 GPL-3.0-or-later，不提供任何担保。"
        "你可以在许可证允许的范围内使用、研究、修改和再分发。"));
    sourceLabel_->setText(tr("项目源码：<a href='%1'>%1</a>")
        .arg(QString::fromLatin1(kRepositoryUrl)));
    licensesButton_->setText(tr("开源许可"));
    aboutQtButton_->setText(tr("关于 Qt"));
    closeButton_->setText(tr("关闭"));
}

void AboutDialog::showOpenSourceLicenses()
{
    OpenSourceLicensesDialog dialog(this);
    dialog.exec();
}

} // namespace snipnexs
