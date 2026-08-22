#include "OcrResultWindow.h"

#include "translate/BrowserTranslation.h"

#include <QClipboard>
#include <QDesktopServices>
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
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("SnipNexs OCR"));
    resize(640, 420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* summary = new QLabel(
        QStringLiteral("本地 Windows OCR · %1 · %2 ms")
            .arg(languageTag.isEmpty() ? QStringLiteral("自动语言") : languageTag)
            .arg(elapsedMs),
        this);
    editor_ = new QPlainTextEdit(this);
    editor_->setPlainText(std::move(text));
    editor_->setPlaceholderText(QStringLiteral("未识别到文字。"));

    auto* buttons = new QHBoxLayout();
    auto* copyButton = new QPushButton(QStringLiteral("复制文字"), this);
    auto* chineseButton = new QPushButton(QStringLiteral("浏览器翻译为中文"), this);
    auto* englishButton = new QPushButton(QStringLiteral("Translate to English"), this);
    auto* closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttons->addWidget(copyButton);
    buttons->addStretch();
    buttons->addWidget(chineseButton);
    buttons->addWidget(englishButton);
    buttons->addWidget(closeButton);

    layout->addWidget(summary);
    layout->addWidget(editor_, 1);
    layout->addLayout(buttons);

    connect(copyButton, &QPushButton::clicked, this, &OcrResultWindow::copyText);
    connect(chineseButton, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("zh-CN"));
    });
    connect(englishButton, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("en"));
    });
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void OcrResultWindow::copyText()
{
    QGuiApplication::clipboard()->setText(editor_->toPlainText());
}

void OcrResultWindow::translateTo(const QString& targetLanguage)
{
    const QString text = editor_->toPlainText().trimmed();
    const QUrl url = browserTranslationUrl(text, targetLanguage);
    if (url.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("无法翻译"), QStringLiteral("没有可翻译的文字。"));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("发送文字到翻译网站"),
        QStringLiteral("将把最多 4000 个字符发送到 translate.google.com。\n"
                       "不会发送截图。是否继续？"));
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, QStringLiteral("无法打开浏览器"), QStringLiteral("请检查默认浏览器设置。"));
    }
}

} // namespace snipnexs
