#include "capture/CaptureHistoryStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QImage makeImage(const QColor& color, const QSize& size = QSize(12, 8), qreal dpr = 1.0)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    image.setDevicePixelRatio(dpr);
    return image;
}

QStringList historyFiles(const QString& directory)
{
    QStringList files = QDir(directory).entryList(
        {QStringLiteral("capture-*.png")}, QDir::Files, QDir::Name);
    return files;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        return 1;
    }

    snipnexs::CaptureHistoryStore store(temporary.path());
    const QImage first = makeImage(QColor(220, 40, 40), QSize(20, 10), 1.5);
    const QImage second = makeImage(QColor(40, 80, 220), QSize(16, 12), 2.0);
    const bool firstWritten = store.append(first);
    const bool secondWritten = store.append(second);
    snipnexs::CaptureHistoryStore restartedStore(temporary.path());
    const QList<QImage> loaded = restartedStore.load();
    const bool orderAndDprOk = loaded.size() == 2
        && loaded.at(0).size() == first.size()
        && qAbs(loaded.at(0).devicePixelRatio() - 1.5) < 0.001
        && loaded.at(1).size() == second.size()
        && qAbs(loaded.at(1).devicePixelRatio() - 2.0) < 0.001
        && loaded.at(0).pixelColor(0, 0) == first.pixelColor(0, 0)
        && loaded.at(1).pixelColor(0, 0) == second.pixelColor(0, 0);

    const QStringList beforeCorruption = historyFiles(temporary.path());
    bool corruptionSkipped = false;
    if (!beforeCorruption.isEmpty()) {
        QFile::remove(QDir(temporary.path()).filePath(beforeCorruption.first()));
        const QList<QImage> afterCorruption = store.load();
        corruptionSkipped = afterCorruption.size() == 1
            && historyFiles(temporary.path()).size() == 1;
    }

    QTemporaryDir capacityDirectory;
    snipnexs::CaptureHistoryStore capacityStore(capacityDirectory.path());
    bool entriesWritten = true;
    for (int i = 0; i < 21; ++i) {
        entriesWritten &= capacityStore.append(
            makeImage(QColor(i, 100, 180), QSize(12, 8), 1.0));
    }
    const bool entryLimitOk = capacityStore.load().size() == 20;

    QTemporaryDir byteLimitDirectory;
    snipnexs::CaptureHistoryStore byteLimitStore(byteLimitDirectory.path());
    const bool firstLargeWritten = byteLimitStore.append(
        makeImage(QColor(10, 20, 30), QSize(4096, 2200), 1.0));
    const bool secondLargeWritten = byteLimitStore.append(
        makeImage(QColor(30, 20, 10), QSize(4096, 2200), 1.0));
    const bool byteLimitOk = byteLimitStore.load().size() == 1;

    const bool ok = firstWritten && secondWritten && orderAndDprOk
        && corruptionSkipped && entriesWritten && entryLimitOk
        && firstLargeWritten && secondLargeWritten && byteLimitOk;
    QTextStream(stdout)
        << "capture history store: " << (ok ? "ok" : "failed")
        << " persistence=" << (firstWritten && secondWritten && orderAndDprOk)
        << " corruption=" << corruptionSkipped
        << " entryLimit=" << entryLimitOk
        << " byteLimit=" << byteLimitOk
        << " largeWrites=" << firstLargeWritten << ',' << secondLargeWritten
        << " largeCount=" << byteLimitStore.load().size()
        << " largeBytes=" << makeImage(QColor(10, 20, 30), QSize(4096, 2200), 1.0).sizeInBytes()
        << '\n';
    return ok ? 0 : 1;
}
