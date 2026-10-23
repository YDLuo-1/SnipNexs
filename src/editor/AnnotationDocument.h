#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QTransform>
#include <QVector>

#include <optional>

class QImage;
class QPainter;

namespace snipnexs {

enum class AnnotationTool {
    None,
    Pen,
    Rectangle,
    Arrow,
    Text,
    Mosaic,
    Blur,
};

struct Annotation {
    AnnotationTool tool = AnnotationTool::None;
    QVector<QPointF> points;
    QString text;
    QColor color = QColor(245, 74, 74);
    qreal width = 3.0;
    int fontPixelSize = 20;
};

// Pixel-effect annotations (Mosaic/Blur) sample the untouched screenshot at
// paint time instead of storing rasterized pixels, so undo/redo stays cheap
// and the result follows the device pixel ratio of whatever surface is being
// composited (live overlay preview and final export alike).
struct AnnotationSource {
    const QImage* image = nullptr;
    QTransform toImagePixels;
};

class AnnotationDocument final
{
public:
    void begin(AnnotationTool tool, const QPointF& point);
    void addText(const QString& text, const QPointF& point);
    void update(const QPointF& point);
    void commit();
    void cancel();
    void clear();

    [[nodiscard]] bool undo();
    [[nodiscard]] bool redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] int itemCount() const;

    void paint(QPainter& painter,
        const AnnotationSource& source = AnnotationSource()) const;

private:
    static void paintAnnotation(
        QPainter& painter, const Annotation& annotation,
        const AnnotationSource& source);

    QVector<Annotation> items_;
    qsizetype appliedCount_ = 0;
    std::optional<Annotation> draft_;
};

} // namespace snipnexs
