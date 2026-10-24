#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSplitter;

namespace snipnexs {

class TranslationModelInstaller;
class TranslationService;

class OcrResultWindow final : public QWidget
{
    Q_OBJECT

public:
    OcrResultWindow(
        QString text,
        const QStringList& lines,
        const QString& languageTag,
        qint64 elapsedMs,
        TranslationService* translationService = nullptr,
        QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void copyText();
    void copyTranslation();
    void retranslateUi();
    void translateTo(const QString& targetLanguage);
    void startLocalTranslation();
    void startAlignedTranslation();
    void setAlignedMode(bool aligned);
    void showAlignedTranslation(const QStringList& lines, qint64 elapsedMs);
    void requestModelDownload(const QString& packageId, const QString& licenseNote);
    void updateTranslationStatus();

    enum class TranslationState {
        Hidden,
        Downloading,
        Translating,
        Done,
    };

    QString languageTag_;
    QStringList ocrLines_;
    QStringList translatedLines_;
    qint64 elapsedMs_ = 0;
    TranslationService* translationService_ = nullptr;
    TranslationModelInstaller* modelInstaller_ = nullptr;
    QString pendingModelPackage_;
    TranslationState translationState_ = TranslationState::Hidden;
    bool awaitingTranslation_ = false;
    bool awaitingLines_ = false;
    bool alignedMode_ = false;
    int downloadPercent_ = -1;
    qint64 translationElapsedMs_ = 0;

    QComboBox* engineCombo_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    QPlainTextEdit* editor_ = nullptr;
    QLabel* translationStatusLabel_ = nullptr;
    QPlainTextEdit* translationEditor_ = nullptr;
    QSplitter* alignedSplitter_ = nullptr;
    QPlainTextEdit* alignedSourceEdit_ = nullptr;
    QPlainTextEdit* alignedTargetEdit_ = nullptr;
    QCheckBox* alignCheck_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QPushButton* copyTranslationButton_ = nullptr;
    QPushButton* localTranslateButton_ = nullptr;
    QPushButton* chineseButton_ = nullptr;
    QPushButton* englishButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
};

} // namespace snipnexs
