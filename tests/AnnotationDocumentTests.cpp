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

    // Pixel-effect annotations sample the pristine source image instead of
    // storing rasterized pixels.
    QImage gradientSource(64, 64, QImage::Format_ARGB32);
    for (int x = 0; x < gradientSource.width(); ++x) {
        for (int y = 0; y < gradientSource.height(); ++y) {
            gradientSource.setPixel(x, y, qRgb(x * 4, 128, 240 - x * 3));
        }
    }
    const snipnexs::AnnotationSource gradientSourceRef{&gradientSource, {}};

    snipnexs::AnnotationDocument mosaicDocument;
    mosaicDocument.begin(snipnexs::AnnotationTool::Mosaic, QPointF(10, 32));
    mosaicDocument.update(QPointF(54, 32));
    mosaicDocument.commit();
    ok &= mosaicDocument.itemCount() == 1;
    QImage mosaicCanvas = gradientSource.copy();
    {
        QPainter painter(&mosaicCanvas);
        mosaicDocument.paint(painter, gradientSourceRef);
    }
    // Columns inside one mosaic block collapse to a single color even though
    // the source gradient differs per column; pixels outside the stroke stay
    // untouched.
    ok &= mosaicCanvas.pixelColor(20, 32) == mosaicCanvas.pixelColor(25, 32);
    ok &= mosaicCanvas.pixelColor(20, 32) != gradientSource.pixelColor(20, 32)
        || mosaicCanvas.pixelColor(25, 32) != gradientSource.pixelColor(25, 32);
    ok &= mosaicCanvas.pixelColor(32, 2) == gradientSource.pixelColor(32, 2);
    ok &= mosaicDocument.undo() && mosaicDocument.itemCount() == 0;
    QImage undoneCanvas = gradientSource.copy();
    {
        QPainter painter(&undoneCanvas);
        mosaicDocument.paint(painter, gradientSourceRef);
    }
    ok &= undoneCanvas.pixelColor(20, 32) == gradientSource.pixelColor(20, 32);

    // A single click is a mosaic dot, not an empty stroke.
    snipnexs::AnnotationDocument dotDocument;
    dotDocument.begin(snipnexs::AnnotationTool::Mosaic, QPointF(30, 30));
    dotDocument.commit();
    ok &= dotDocument.itemCount() == 1;
    QImage dotCanvas = gradientSource.copy();
    {
        QPainter painter(&dotCanvas);
        dotDocument.paint(painter, gradientSourceRef);
    }
    ok &= dotCanvas.pixelColor(25, 30) == dotCanvas.pixelColor(28, 30);

    QImage splitSource(64, 64, QImage::Format_ARGB32);
    for (int x = 0; x < splitSource.width(); ++x) {
        for (int y = 0; y < splitSource.height(); ++y) {
            splitSource.setPixel(x, y,
                x < 32 ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
        }
    }
    snipnexs::AnnotationDocument blurDocument;
    blurDocument.begin(snipnexs::AnnotationTool::Blur, QPointF(8, 32));
    blurDocument.update(QPointF(56, 32));
    blurDocument.commit();
    ok &= blurDocument.itemCount() == 1;
    QImage blurCanvas = splitSource.copy();
    {
        QPainter painter(&blurCanvas);
        blurDocument.paint(painter, {&splitSource, {}});
    }
    // The black/white boundary inside the stroke becomes gray; outside the
    // stroke it stays pure black/white.
    const QColor blurredMiddle = blurCanvas.pixelColor(32, 32);
    ok &= blurredMiddle.red() > 60 && blurredMiddle.red() < 196;
    ok &= blurCanvas.pixelColor(32, 2) == splitSource.pixelColor(32, 2);

    QTextStream(stdout) << (ok ? "annotation document: ok\n" : "annotation document: failed\n");
    return ok ? 0 : 1;
}
