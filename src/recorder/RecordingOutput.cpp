#include "RecordingOutput.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <Windows.h>
#include <utility>

namespace snipnexs {

QString finishRecordingOutput(
    const RecordingResult& result, QString& pendingPath, const QString& finalPath)
{
    if (pendingPath.isEmpty()) {
        return {};
    }
    const QString path = std::exchange(pendingPath, {});
    if (!result.succeeded) {
        if (QFileInfo(path).size() == 0) {
            QFile::remove(path);
            return result.error;
        }
        return result.error + QCoreApplication::translate(
            "snipnexs::RecorderController", "\n录屏未完成，临时文件保留在：%1")
            .arg(QDir::toNativeSeparators(path));
    }
    const auto source = QDir::toNativeSeparators(path).toStdWString();
    const auto target = QDir::toNativeSeparators(finalPath).toStdWString();
    if (!MoveFileExW(source.c_str(), target.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return QCoreApplication::translate("snipnexs::RecorderController",
            "MP4 已生成，但无法原子移动到目标路径（错误 %1）。临时文件保留在：%2")
            .arg(GetLastError()).arg(QDir::toNativeSeparators(path));
    }
    return {};
}

} // namespace snipnexs
