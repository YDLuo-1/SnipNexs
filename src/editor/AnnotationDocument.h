#pragma once

#include <QColor>
#include <QPointF>
#include <QVector>

#include <optional>

class QPainter;

namespace snipnexs {

enum class AnnotationTool {
    None,
    Pen,
    Rectangle,
    Arrow,
};

struct Annotation {
    AnnotationTool tool = AnnotationTool::None;
    QVector<QPointF> points;
    QColor color = QColor(245, 74, 74);
    qreal width = 3.0;
};

class AnnotationDocument final
{
public:
    void begin(AnnotationTool tool, const QPointF& point);
    void update(const QPointF& point);
    void commit();
    void cancel();
    void clear();

    [[nodiscard]] bool undo();
    [[nodiscard]] bool redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] int itemCount() const;

    void paint(QPainter& painter) const;

private:
    static void paintAnnotation(QPainter& painter, const Annotation& annotation);

    QVector<Annotation> items_;
    qsizetype appliedCount_ = 0;
    std::optional<Annotation> draft_;
};

} // namespace snipnexs
