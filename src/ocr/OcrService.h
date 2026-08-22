#pragma once

#include <QObject>
#include <QThread>

class QImage;
class QString;

namespace snipnexs {

class OcrService final : public QObject
{
    Q_OBJECT

public:
    explicit OcrService(QObject* parent = nullptr);
    ~OcrService() override;

    [[nodiscard]] bool recognize(QImage image);

signals:
    void recognized(const QString& text, const QString& languageTag, qint64 elapsedMs);
    void failed(const QString& message);

private:
    QThread workerThread_;
    QObject* workerContext_ = nullptr;
    bool busy_ = false;
};

} // namespace snipnexs
