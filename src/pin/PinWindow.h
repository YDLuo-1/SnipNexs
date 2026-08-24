#pragma once

#include <QPixmap>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace snipnexs {

class PinWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit PinWindow(const QImage& image, QWidget* parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void resizeBy(qreal factor, const QPointF& anchorPosition);
    QRectF imageRect() const;

    QPixmap pixmap_;
    QPoint dragOffset_;
    bool moving_ = false;
};

} // namespace snipnexs
