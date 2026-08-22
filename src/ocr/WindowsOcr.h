#pragma once

#include <QString>

class QImage;

namespace snipnexs::windows_ocr {

struct Result {
    QString text;
    QString languageTag;
    QString error;
    qint64 elapsedMs = 0;
};

[[nodiscard]] Result recognize(const QImage& image);

} // namespace snipnexs::windows_ocr
