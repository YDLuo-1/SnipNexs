#pragma once

#include <QImage>
#include <QList>
#include <QString>

namespace snipnexs {

// Owns the small, local-only screenshot history used by the capture overlay.
// The store deliberately exposes images, not file names, so the controller and
// UI never depend on the on-disk representation.
class CaptureHistoryStore final
{
public:
    explicit CaptureHistoryStore(QString directory = {});

    QList<QImage> load();
    bool append(const QImage& image);

    [[nodiscard]] QString directory() const { return directory_; }

private:
    QString directory_;
};

} // namespace snipnexs
