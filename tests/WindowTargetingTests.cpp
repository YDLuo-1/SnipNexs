#include "platform/windows/WindowTargeting.h"

#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QRect smallDetail(40, 40, 80, 50);
    const QRect largeDetail(20, 20, 300, 200);
    const QRect client(10, 10, 480, 360);
    const QRect frame(0, 0, 500, 400);
    const QList<QRect> targets = snipnexs::prioritizeWindowTargetGroup(
        {largeDetail, smallDetail, smallDetail, QRect()}, client, frame);

    bool ok = targets == QList<QRect> {
        smallDetail,
        largeDetail,
        client,
        frame,
    };
    const QList<QRect> deduplicated = snipnexs::prioritizeWindowTargetGroup(
        {client, frame}, client, frame);
    ok &= deduplicated == QList<QRect> {client, frame};

    QTextStream(stdout)
        << "window targeting: " << (ok ? "ok" : "failed")
        << " targets=" << targets.size()
        << " deduplicated=" << deduplicated.size() << '\n';
    return ok ? 0 : 1;
}
