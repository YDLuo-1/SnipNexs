#include "OcrResultWindow.h"

#include "translate/BrowserTranslation.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

namespace snipnexs {

OcrResultWindow::OcrResultWindow(
    QString text,
    const QString& languageTag,
    qint64 elapsedMs,
    QWidget* parent)
    : QWidget(parent)
    , languageTag_(languageTag)
    , elapsedMs_(elapsedMs)
{
    setAttribute(Qt::WA_DeleteOnClose);
    resize(640, 420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    summaryLabel_ = new QLabel(this);
    editor_ = new QPlainTextEdit(this);
    editor_->setPlainText(std::move(text));

    auto* buttons = new QHBoxLayout();
    copyButton_ = new QPushButton(this);
    chineseButton_ = new QPushButton(this);
    englishButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);
    buttons->addWidget(copyButton_);
    buttons->addStretch();
    buttons->addWidget(chineseButton_);
    buttons->addWidget(englishButton_);
    buttons->addWidget(closeButton_);

    layout->addWidget(summaryLabel_);
    layout->addWidget(editor_, 1);
    layout->addLayout(buttons);

    connect(copyButton_, &QPushButton::clicked, this, &OcrResultWindow::copyText);
    connect(chineseButton_, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("zh-CN"));
    });
    connect(englishButton_, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("en"));
    });
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::close);
    retranslateUi();
}

void OcrResultWindow::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void OcrResultWindow::copyText()
{
    QGuiApplication::clipboard()->setText(editor_->toPlainText());
}

void OcrResultWindow::retranslateUi()
{
    setWindowTitle(tr("SnipNexs OCR"));
    summaryLabel_->setText(
        tr("本地 Windows OCR · %1 · %2 ms")
            .arg(languageTag_.isEmpty() ? tr("自动语言") : languageTag_)
            .arg(elapsedMs_));
    editor_->setPlaceholderText(tr("未识别到文字。"));
    copyButton_->setText(tr("复制文字"));
    chineseButton_->setText(tr("浏览器翻译为中文"));
    englishButton_->setText(tr("浏览器翻译为英文"));
    closeButton_->setText(tr("关闭"));
}

void OcrResultWindow::translateTo(const QString& targetLanguage)
{
    const QString text = editor_->toPlainText().trimmed();
    const QUrl url = browserTranslationUrl(text, targetLanguage);
    if (url.isEmpty()) {
        QMessageBox::information(this, tr("无法翻译"), tr("没有可翻译的文字。"));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        tr("发送文字到翻译网站"),
        tr("将把最多 4000 个字符发送到 translate.google.com。\n"
           "不会发送截图。是否继续？"));
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, tr("无法打开浏览器"), tr("请检查默认浏览器设置。"));
    }
}

} // namespace snipnexs
