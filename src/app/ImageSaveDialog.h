#pragma once

#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QString>

namespace snipnexs {

inline QString imageSuffixForFilter(const QString& filter)
{
    if (filter.contains(QStringLiteral("*.jpg"))
        || filter.contains(QStringLiteral("*.jpeg"))) {
        return QStringLiteral("jpg");
    }
    if (filter.contains(QStringLiteral("*.bmp"))) {
        return QStringLiteral("bmp");
    }
    return QStringLiteral("png");
}

// QFileDialog::getSaveFileName() does not apply the selected filter's suffix,
// so "shot" reaches QImage::save() without a format. Keep the default suffix
// in sync with the filter and append it when the native dialog does not.
inline QString getImageSaveFileName(
    QWidget* parent,
    const QString& title,
    const QString& initialPath,
    const QString& filters)
{
    QFileDialog dialog(parent, title, initialPath, filters);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDefaultSuffix(QStringLiteral("png"));
    QObject::connect(&dialog, &QFileDialog::filterSelected, &dialog,
        [&dialog](const QString& filter) {
            dialog.setDefaultSuffix(imageSuffixForFilter(filter));
        });

    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    QString fileName = dialog.selectedFiles().value(0);
    if (!fileName.isEmpty() && QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QLatin1Char('.') + dialog.defaultSuffix();
    }
    return fileName;
}

} // namespace snipnexs
