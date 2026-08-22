#include "editor/AnnotationDocument.h"

#include <QImage>
#include <QPainter>
#include <QTextStream>

int main()
{
    snipnexs::AnnotationDocument document;
    document.begin(snipnexs::AnnotationTool::Rectangle, QPointF(10, 10));
    document.update(QPointF(50, 40));
    document.commit();

    bool ok = document.itemCount() == 1 && document.canUndo() && !document.canRedo();
    ok &= document.undo();
    ok &= document.itemCount() == 0 && document.canRedo();
    ok &= document.redo();
    ok &= document.itemCount() == 1;

    document.begin(snipnexs::AnnotationTool::Arrow, QPointF(15, 60));
    document.update(QPointF(70, 20));
    document.commit();
    ok &= document.itemCount() == 2;

    QImage image(100, 80, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    document.paint(painter);
    painter.end();

    ok &= image.pixelColor(10, 10).alpha() > 0;
    ok &= image.pixelColor(70, 20).alpha() > 0;

    QTextStream(stdout) << (ok ? "annotation document: ok\n" : "annotation document: failed\n");
    return ok ? 0 : 1;
}
