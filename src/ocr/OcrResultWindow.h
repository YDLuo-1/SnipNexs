#pragma once

#include <QWidget>

class QPlainTextEdit;
class QString;

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

private:
    void copyText();
    void translateTo(const QString& targetLanguage);

    QPlainTextEdit* editor_ = nullptr;
};

} // namespace snipnexs
