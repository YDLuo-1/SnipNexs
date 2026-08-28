#include "OcrResultWindow.h"

#include "translate/BrowserTranslation.h"
#include "translate/TranslationModelInstaller.h"
#include "translate/TranslationModels.h"
#include "translate/TranslationService.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace snipnexs {

OcrResultWindow::OcrResultWindow(
    QString text,
    const QString& languageTag,
    qint64 elapsedMs,
    TranslationService* translationService,
    QWidget* parent)
    : QWidget(parent)
    , languageTag_(languageTag)
    , elapsedMs_(elapsedMs)
    , translationService_(translationService)
{
    setAttribute(Qt::WA_DeleteOnClose);
    resize(640, 420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    summaryLabel_ = new QLabel(this);
    editor_ = new QPlainTextEdit(this);
    editor_->setPlainText(std::move(text));

    translationStatusLabel_ = new QLabel(this);
    translationStatusLabel_->hide();
    translationEditor_ = new QPlainTextEdit(this);
    translationEditor_->setReadOnly(true);
    translationEditor_->hide();

    auto* translationButtons = new QHBoxLayout();
    copyTranslationButton_ = new QPushButton(this);
    translationButtons->addWidget(copyTranslationButton_);
    translationButtons->addStretch();

    auto* buttons = new QHBoxLayout();
    copyButton_ = new QPushButton(this);
    localTranslateButton_ = new QPushButton(this);
    chineseButton_ = new QPushButton(this);
    englishButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);
    buttons->addWidget(copyButton_);
    buttons->addStretch();
    buttons->addWidget(localTranslateButton_);
    buttons->addWidget(chineseButton_);
    buttons->addWidget(englishButton_);
    buttons->addWidget(closeButton_);

    layout->addWidget(summaryLabel_);
    layout->addWidget(editor_, 1);
    layout->addWidget(translationStatusLabel_);
    layout->addWidget(translationEditor_, 1);
    layout->addLayout(translationButtons);
    layout->addLayout(buttons);

    connect(copyButton_, &QPushButton::clicked, this, &OcrResultWindow::copyText);
    connect(copyTranslationButton_, &QPushButton::clicked, this,
        &OcrResultWindow::copyTranslation);
    connect(localTranslateButton_, &QPushButton::clicked, this,
        &OcrResultWindow::startLocalTranslation);
    connect(chineseButton_, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("zh-CN"));
    });
    connect(englishButton_, &QPushButton::clicked, this, [this]() {
        translateTo(QStringLiteral("en"));
    });
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::close);

    if (translationService_) {
        connect(translationService_, &TranslationService::translated, this,
            [this](const QString& text, qint64 elapsed) {
                translationState_ = TranslationState::Done;
                translationElapsedMs_ = elapsed;
                translationEditor_->setPlainText(text);
                translationStatusLabel_->show();
                translationEditor_->show();
                copyTranslationButton_->show();
                updateTranslationStatus();
            });
        connect(translationService_, &TranslationService::failed, this,
            [this](const QString& message) {
                translationState_ = TranslationState::Hidden;
                updateTranslationStatus();
                QMessageBox::warning(this, tr("本地翻译失败"), message);
            });
        connect(translationService_, &TranslationService::modelMissing, this,
            &OcrResultWindow::requestModelDownload);
        connect(translationService_, &TranslationService::busyChanged, this,
            [this](bool busy) {
                localTranslateButton_->setEnabled(!busy);
            });
    } else {
        localTranslateButton_->hide();
    }

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

void OcrResultWindow::copyTranslation()
{
    QGuiApplication::clipboard()->setText(translationEditor_->toPlainText());
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
    copyTranslationButton_->setText(tr("复制译文"));
    localTranslateButton_->setText(tr("本地翻译"));
    chineseButton_->setText(tr("浏览器翻译为中文"));
    englishButton_->setText(tr("浏览器翻译为英文"));
    closeButton_->setText(tr("关闭"));
    updateTranslationStatus();
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

void OcrResultWindow::startLocalTranslation()
{
    const QString text = editor_->toPlainText().trimmed();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("无法翻译"), tr("没有可翻译的文字。"));
        return;
    }
    if (!translationService_) {
        return;
    }

    translationState_ = TranslationState::Translating;
    translationStatusLabel_->show();
    updateTranslationStatus();
    if (!translationService_->translate(text, languageTag_, QString())) {
        // translate() reports the reason through signals; busy or missing
        // models are handled by busyChanged/modelMissing handlers.
        if (translationState_ == TranslationState::Translating) {
            translationState_ = TranslationState::Hidden;
            updateTranslationStatus();
        }
    }
}

void OcrResultWindow::requestModelDownload(
    const QString& packageId, const QString& licenseNote)
{
    translationState_ = TranslationState::Hidden;
    updateTranslationStatus();
    if (modelInstaller_ && modelInstaller_->packageId() != packageId) {
        QMessageBox::information(
            this, tr("正在下载其他模型"),
            tr("另一个模型包正在下载，请等待其完成后再试。"));
        return;
    }

    qint64 totalBytes = 0;
    const QList<TranslationModelSpec> models = knownTranslationModels();
    for (const TranslationModelSpec& spec : models) {
        if (spec.id == packageId) {
            for (const auto& file : spec.files) {
                totalBytes += file.sizeBytes;
            }
        }
    }
    const QString sizeText = totalBytes > 0
        ? tr("约 %1 MB").arg(QString::number(totalBytes / (1024.0 * 1024.0), 'f', 0))
        : tr("一百余 MB");

    const auto answer = QMessageBox::question(
        this,
        tr("下载本地翻译模型"),
        tr("本地翻译需要先下载语言包（%1），下载后可永久离线使用。\n"
           "模型：%2\n是否现在下载？")
            .arg(sizeText, licenseNote));
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!modelInstaller_) {
        for (const TranslationModelSpec& spec : knownTranslationModels()) {
            if (spec.id == packageId) {
                modelInstaller_ = new TranslationModelInstaller(spec, this);
                break;
            }
        }
    }
    if (!modelInstaller_) {
        QMessageBox::warning(this, tr("下载本地翻译模型"), tr("未找到该语言包。"));
        return;
    }

    pendingModelPackage_ = packageId;
    translationState_ = TranslationState::Downloading;
    translationStatusLabel_->show();
    updateTranslationStatus();

    connect(modelInstaller_, &TranslationModelInstaller::progress, this,
        [this](qint64 received, qint64 total, const QString&) {
            downloadPercent_ = total > 0
                ? static_cast<int>(received * 100 / total)
                : -1;
            updateTranslationStatus();
        });
    connect(modelInstaller_, &TranslationModelInstaller::finished, this,
        [this](bool success, const QString& error) {
            if (pendingModelPackage_.isEmpty()) {
                return;
            }
            if (success) {
                translationState_ = TranslationState::Hidden;
                downloadPercent_ = -1;
                const QString packageId = pendingModelPackage_;
                pendingModelPackage_.clear();
                updateTranslationStatus();
                if (modelInstaller_->packageId() == packageId) {
                    startLocalTranslation();
                }
            } else {
                translationState_ = TranslationState::Hidden;
                downloadPercent_ = -1;
                updateTranslationStatus();
                if (!error.isEmpty()) {
                    QMessageBox::warning(this, tr("下载本地翻译模型"), error);
                }
            }
        });

    modelInstaller_->start();
}

void OcrResultWindow::updateTranslationStatus()
{
    switch (translationState_) {
    case TranslationState::Downloading:
        translationStatusLabel_->setText(
            tr("正在下载模型…%1%")
                .arg(downloadPercent_ >= 0 ? QString::number(downloadPercent_)
                                           : QStringLiteral("…")));
        break;
    case TranslationState::Translating:
        translationStatusLabel_->setText(tr("正在翻译…"));
        break;
    case TranslationState::Done:
        translationStatusLabel_->setText(
            tr("本地翻译 · %1 ms").arg(translationElapsedMs_));
        break;
    case TranslationState::Hidden:
        translationStatusLabel_->clear();
        break;
    }
}

} // namespace snipnexs
