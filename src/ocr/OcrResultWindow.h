#pragma once

#include <QString>
#include <QWidget>

class QPlainTextEdit;
class QEvent;
class QLabel;
class QPushButton;

namespace snipnexs {

class OcrResultWindow final : public QWidget
{
    Q_OBJECT

public:
    OcrResultWindow(
        QString text,
        const QString& languageTag,
        qint64 elapsedMs,
        QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void copyText();
    void retranslateUi();
    void translateTo(const QString& targetLanguage);

    QString languageTag_;
    qint64 elapsedMs_ = 0;
    QLabel* summaryLabel_ = nullptr;
    QPlainTextEdit* editor_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QPushButton* chineseButton_ = nullptr;
    QPushButton* englishButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
};

} // namespace snipnexs
