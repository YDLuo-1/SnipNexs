#include "editor/AnnotationDocument.h"

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
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

    snipnexs::AnnotationDocument textDocument;
    textDocument.addText(QStringLiteral("Text"), QPointF(8, 28));
    ok &= textDocument.itemCount() == 1 && textDocument.canUndo();
    QImage textImage(100, 50, QImage::Format_ARGB32_Premultiplied);
    textImage.fill(Qt::transparent);
    QPainter textPainter(&textImage);
    textDocument.paint(textPainter);
    textPainter.end();
    bool textPainted = false;
    for (int y = 0; y < textImage.height() && !textPainted; ++y) {
        for (int x = 0; x < textImage.width(); ++x) {
            if (textImage.pixelColor(x, y).alpha() > 0) {
                textPainted = true;
                break;
            }
        }
    }
    ok &= textPainted;

    QTextStream(stdout) << (ok ? "annotation document: ok\n" : "annotation document: failed\n");
    return ok ? 0 : 1;
}
