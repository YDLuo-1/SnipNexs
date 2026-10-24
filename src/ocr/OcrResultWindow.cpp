#include "OcrResultWindow.h"

#include "translate/BrowserTranslation.h"
#include "translate/TranslationModelInstaller.h"
#include "translate/TranslationModels.h"
#include "translate/TranslationService.h"

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace snipnexs {

OcrResultWindow::OcrResultWindow(
    QString text,
    const QStringList& lines,
    const QString& languageTag,
    qint64 elapsedMs,
    TranslationService* translationService,
    QWidget* parent)
    : QWidget(parent)
    , languageTag_(languageTag)
    , ocrLines_(lines)
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

    alignedSourceEdit_ = new QPlainTextEdit(this);
    alignedSourceEdit_->setReadOnly(true);
    alignedSourceEdit_->setPlainText(ocrLines_.join(QLatin1Char('\n')));
    alignedTargetEdit_ = new QPlainTextEdit(this);
    alignedTargetEdit_->setReadOnly(true);
    alignedSplitter_ = new QSplitter(Qt::Horizontal, this);
    alignedSplitter_->addWidget(alignedSourceEdit_);
    alignedSplitter_->addWidget(alignedTargetEdit_);
    alignedSplitter_->setStretchFactor(0, 1);
    alignedSplitter_->setStretchFactor(1, 1);
    alignedSplitter_->hide();
    // Side-by-side reading works only when both panes scroll together.
    connect(alignedSourceEdit_->verticalScrollBar(), &QScrollBar::valueChanged,
        alignedTargetEdit_->verticalScrollBar(), &QScrollBar::setValue);
    connect(alignedTargetEdit_->verticalScrollBar(), &QScrollBar::valueChanged,
        alignedSourceEdit_->verticalScrollBar(), &QScrollBar::setValue);

    alignCheck_ = new QCheckBox(this);
    alignCheck_->setObjectName(QStringLiteral("alignCheck"));
    alignCheck_->hide();

    auto* translationButtons = new QHBoxLayout();
    translationButtons->addWidget(alignCheck_);
    copyTranslationButton_ = new QPushButton(this);
    translationButtons->addWidget(copyTranslationButton_);
    translationButtons->addStretch();

    auto* buttons = new QHBoxLayout();
    copyButton_ = new QPushButton(this);
    engineCombo_ = new QComboBox(this);
    engineCombo_->setObjectName(QStringLiteral("engineCombo"));
    localTranslateButton_ = new QPushButton(this);
    chineseButton_ = new QPushButton(this);
    englishButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);
    buttons->addWidget(copyButton_);
    buttons->addStretch();
    buttons->addWidget(engineCombo_);
    buttons->addWidget(localTranslateButton_);
    buttons->addWidget(chineseButton_);
    buttons->addWidget(englishButton_);
    buttons->addWidget(closeButton_);

    layout->addWidget(summaryLabel_);
    layout->addWidget(editor_, 1);
    layout->addWidget(translationStatusLabel_);
    layout->addWidget(translationEditor_, 1);
    layout->addWidget(alignedSplitter_, 1);
    layout->addLayout(translationButtons);
    layout->addLayout(buttons);

    connect(alignCheck_, &QCheckBox::toggled, this, &OcrResultWindow::setAlignedMode);
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

    connect(engineCombo_, &QComboBox::activated, this, [this](int index) {
        if (index >= 0) {
            QSettings settings;
            settings.setValue(QStringLiteral("translation/engine"),
                              engineCombo_->itemData(index).toString());
        }
    });

    if (translationService_) {
        connect(translationService_, &TranslationService::translated, this,
            [this](const QString& text, qint64 elapsed) {
                if (!awaitingTranslation_) {
                    return;
                }
                awaitingTranslation_ = false;
                translationState_ = TranslationState::Done;
                translationElapsedMs_ = elapsed;
                translationEditor_->setPlainText(text);
                translationStatusLabel_->show();
                if (!alignedMode_) {
                    translationEditor_->show();
                }
                copyTranslationButton_->show();
                updateTranslationStatus();
            });
        connect(translationService_, &TranslationService::linesTranslated, this,
            [this](const QStringList& lines, qint64 elapsed) {
                if (!awaitingLines_) {
                    return;
                }
                awaitingLines_ = false;
                translationState_ = TranslationState::Done;
                translationElapsedMs_ = elapsed;
                showAlignedTranslation(lines, elapsed);
                copyTranslationButton_->show();
                updateTranslationStatus();
            });
        connect(translationService_, &TranslationService::failed, this,
            [this](const QString& message) {
                if (!awaitingTranslation_ && !awaitingLines_) {
                    return;
                }
                awaitingTranslation_ = false;
                awaitingLines_ = false;
                translationState_ = TranslationState::Hidden;
                updateTranslationStatus();
                QMessageBox::warning(this, tr("本地翻译失败"), message);
            });
        connect(translationService_, &TranslationService::modelMissing, this,
            [this](const QString& packageId, const QString& licenseNote) {
                if (!awaitingTranslation_ && !awaitingLines_) {
                    return;
                }
                awaitingTranslation_ = false;
                awaitingLines_ = false;
                requestModelDownload(packageId, licenseNote);
            });
        connect(translationService_, &TranslationService::busyChanged, this,
            [this](bool busy) {
                localTranslateButton_->setEnabled(!busy);
                updateTranslationStatus();
            });
        // Line alignment needs at least two lines to be worth the space.
        if (ocrLines_.size() >= 2) {
            alignCheck_->show();
        }
    } else {
        engineCombo_->hide();
        localTranslateButton_->hide();
        alignCheck_->hide();
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
    QGuiApplication::clipboard()->setText(
        alignedMode_ ? alignedTargetEdit_->toPlainText()
                     : translationEditor_->toPlainText());
}

void OcrResultWindow::retranslateUi()
{
    setWindowTitle(tr("SnipNexs OCR"));
    summaryLabel_->setText(
        tr("本地 Windows OCR · %1 · %2 ms")
            .arg(languageTag_.isEmpty() ? tr("自动语言") : languageTag_)
            .arg(elapsedMs_));
    editor_->setPlaceholderText(tr("未识别到文字。"));
    if (engineCombo_ != nullptr) {
        const QVariant previous = engineCombo_->currentData();
        engineCombo_->blockSignals(true);
        engineCombo_->clear();
        engineCombo_->addItem(tr("默认引擎(快)"), QStringLiteral("opusmt"));
        engineCombo_->addItem(tr("高质量引擎(更准，较慢)"), QStringLiteral("hymt2"));
        const QVariant saved = QSettings().value(
            QStringLiteral("translation/engine"), QStringLiteral("opusmt"));
        const int index = engineCombo_->findData(saved);
        engineCombo_->setCurrentIndex(index >= 0 ? index : 0);
        if (previous.isValid()) {
            const int previousIndex = engineCombo_->findData(previous);
            if (previousIndex >= 0) {
                engineCombo_->setCurrentIndex(previousIndex);
            }
        }
        engineCombo_->blockSignals(false);
    }
    copyButton_->setText(tr("复制文字"));
    copyTranslationButton_->setText(tr("复制译文"));
    alignCheck_->setText(tr("对照"));
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
    if (alignedMode_) {
        startAlignedTranslation();
        return;
    }
    if (awaitingTranslation_
        || translationState_ == TranslationState::Translating
        || translationState_ == TranslationState::Downloading) {
        return;
    }

    translationState_ = TranslationState::Translating;
    awaitingTranslation_ = true;
    translationStatusLabel_->show();
    updateTranslationStatus();
    const TranslationEngineId engine = engineCombo_->currentData().toString()
            == QStringLiteral("hymt2")
        ? TranslationEngineId::LlamaHyMT2
        : TranslationEngineId::OpusMt;
    if (!translationService_->translate(text, languageTag_, QString(), engine)) {
        // translate() reports the reason through signals; busy or missing
        // models are handled by busyChanged/modelMissing handlers.
        awaitingTranslation_ = false;
        if (translationState_ == TranslationState::Translating) {
            translationState_ = TranslationState::Hidden;
            updateTranslationStatus();
        }
    }
}

void OcrResultWindow::startAlignedTranslation()
{
    if (awaitingLines_
        || translationState_ == TranslationState::Translating
        || translationState_ == TranslationState::Downloading) {
        return;
    }
    if (translatedLines_.size() == ocrLines_.size() && !translatedLines_.isEmpty()) {
        showAlignedTranslation(translatedLines_, translationElapsedMs_);
        return;
    }

    translationState_ = TranslationState::Translating;
    awaitingLines_ = true;
    translationStatusLabel_->show();
    updateTranslationStatus();
    const TranslationEngineId engine = engineCombo_->currentData().toString()
            == QStringLiteral("hymt2")
        ? TranslationEngineId::LlamaHyMT2
        : TranslationEngineId::OpusMt;
    if (!translationService_->translateLines(ocrLines_, languageTag_, engine)) {
        // Same signal-based error handling as the plain-text path.
        awaitingLines_ = false;
        if (translationState_ == TranslationState::Translating) {
            translationState_ = TranslationState::Hidden;
            updateTranslationStatus();
        }
    }
}

void OcrResultWindow::setAlignedMode(bool aligned)
{
    alignedMode_ = aligned;
    if (aligned) {
        translationEditor_->hide();
        alignedSplitter_->show();
        startAlignedTranslation();
    } else {
        alignedSplitter_->hide();
        if (translationState_ == TranslationState::Done) {
            translationEditor_->show();
        }
    }
}

void OcrResultWindow::showAlignedTranslation(
    const QStringList& lines, qint64 elapsedMs)
{
    translatedLines_ = lines;
    translationElapsedMs_ = elapsedMs;
    alignedTargetEdit_->setPlainText(lines.join(QLatin1Char('\n')));
    translationStatusLabel_->show();
    if (alignedMode_) {
        alignedSplitter_->show();
    }
    updateTranslationStatus();
}

void OcrResultWindow::requestModelDownload(
    const QString& packageId, const QString& licenseNote)
{
    translationState_ = TranslationState::Hidden;
    updateTranslationStatus();
    if (modelInstaller_ != nullptr) {
        QMessageBox::information(
            this, tr("正在下载其他模型"),
            tr("另一个模型包正在下载，请等待其完成后再试。"));
        return;
    }

    TranslationModelSpec modelSpec;
    qint64 totalBytes = 0;
    for (const TranslationModelSpec& spec : knownTranslationModels()) {
        if (spec.id != packageId) {
            continue;
        }
        modelSpec = spec;
        for (const TranslationModelFile& file : spec.files) {
            totalBytes += file.sizeBytes;
        }
        break;
    }
    if (modelSpec.id.isEmpty()) {
        QMessageBox::warning(this, tr("下载本地翻译模型"), tr("未找到该语言包。"));
        return;
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

    // One installer per attempt: a failed download must not accumulate
    // progress/finished connections or reuse stale file counters.
    auto* installer = new TranslationModelInstaller(modelSpec, this);
    modelInstaller_ = installer;
    pendingModelPackage_ = packageId;
    downloadPercent_ = -1;
    translationState_ = TranslationState::Downloading;
    translationStatusLabel_->show();
    updateTranslationStatus();

    connect(installer, &TranslationModelInstaller::progress, this,
        [this, installer](qint64 received, qint64 total, const QString&) {
            if (modelInstaller_ != installer) {
                return;
            }
            downloadPercent_ = total > 0
                ? static_cast<int>(received * 100 / total)
                : -1;
            updateTranslationStatus();
        });
    connect(installer, &TranslationModelInstaller::finished, this,
        [this, installer](bool success, const QString& error) {
            if (modelInstaller_ != installer) {
                return;
            }
            modelInstaller_ = nullptr;
            installer->deleteLater();
            if (pendingModelPackage_.isEmpty()) {
                return;
            }
            pendingModelPackage_.clear();
            translationState_ = TranslationState::Hidden;
            downloadPercent_ = -1;
            updateTranslationStatus();
            if (success) {
                startLocalTranslation();
            } else if (!error.isEmpty()) {
                QMessageBox::warning(this, tr("下载本地翻译模型"), error);
            }
        });

    installer->start();
}

void OcrResultWindow::updateTranslationStatus()
{
    const bool busy = translationState_ == TranslationState::Translating
        || translationState_ == TranslationState::Downloading;
    alignCheck_->setEnabled(!busy);
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
