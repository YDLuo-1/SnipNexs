#pragma once

#include <QString>
#include <QStringList>

class QImage;

namespace snipnexs::windows_ocr {

struct Result {
    QString text;
    // One entry per recognized layout line (CJK word runs joined without
    // spaces), aligned with the visual reading order. Empty when the
    // recognizer returned no lines.
    QStringList lines;
    QString languageTag;
    QString error;
    qint64 elapsedMs = 0;
};

[[nodiscard]] Result recognize(const QImage& image);

} // namespace snipnexs::windows_ocr
