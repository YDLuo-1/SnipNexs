#pragma once

#include <QString>
#include <QWidget>

class QEvent;
class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace snipnexs {

class TranslationModelInstaller;
class TranslationService;

class OcrResultWindow final : public QWidget
{
    Q_OBJECT

public:
    OcrResultWindow(
        QString text,
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
    void requestModelDownload(const QString& packageId, const QString& licenseNote);
    void updateTranslationStatus();

    enum class TranslationState {
        Hidden,
        Downloading,
        Translating,
        Done,
    };

    QString languageTag_;
    qint64 elapsedMs_ = 0;
    TranslationService* translationService_ = nullptr;
    TranslationModelInstaller* modelInstaller_ = nullptr;
    QString pendingModelPackage_;
    TranslationState translationState_ = TranslationState::Hidden;
    int downloadPercent_ = -1;
    qint64 translationElapsedMs_ = 0;

    QLabel* summaryLabel_ = nullptr;
    QPlainTextEdit* editor_ = nullptr;
    QLabel* translationStatusLabel_ = nullptr;
    QPlainTextEdit* translationEditor_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QPushButton* copyTranslationButton_ = nullptr;
    QPushButton* localTranslateButton_ = nullptr;
    QPushButton* chineseButton_ = nullptr;
    QPushButton* englishButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
};

} // namespace snipnexs
