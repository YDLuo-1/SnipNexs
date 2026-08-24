#include "recorder/RecordingIndicator.h"

#include <QApplication>
#include <QPushButton>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    bool ok = snipnexs::recordingIndicatorBottomRightPosition(
        QRect(100, 50, 1000, 700), QSize(240, 50)) == QPoint(844, 684);
    ok &= snipnexs::recordingIndicatorBottomRightPosition(
        QRect(-1920, 0, 1920, 1080), QSize(260, 56), 24) == QPoint(-284, 1000);
    ok &= snipnexs::recordingIndicatorBottomRightPosition(
        QRect(0, 0, 100, 80), QSize(160, 100)) == QPoint(0, 0);

    snipnexs::RecordingIndicator indicator;
    const auto* stopButton = indicator.findChild<QPushButton*>(
        QStringLiteral("stopButton"));
    ok &= indicator.cursor().shape() == Qt::SizeAllCursor
        && !indicator.toolTip().isEmpty()
        && stopButton != nullptr
        && stopButton->cursor().shape() == Qt::ArrowCursor;

    QTextStream(stdout)
        << "recording indicator: " << (ok ? "ok" : "failed")
        << " size=" << indicator.size().width() << 'x' << indicator.size().height()
        << '\n';
    return ok ? 0 : 1;
}
