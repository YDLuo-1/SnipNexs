#pragma once

#include <QImage>
#include <QPixmap>
#include <QWidget>

class QContextMenuEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;
class QFrame;
class QTimer;

namespace snipnexs {

class PinWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit PinWindow(const QImage& image, QWidget* parent = nullptr);
    void moveImageTopLeft(const QPoint& globalTopLeft);

signals:
    void copyRequested(const QImage& image);
    void saveRequested(const QImage& image);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void positionToolbar();
    void setToolbarVisible(bool visible);
    void resizeBy(qreal factor, const QPointF& anchorPosition);
    void showZoomIndicator();
    QRectF imageRect() const;

    QImage sourceImage_;
    QPixmap pixmap_;
    QFrame* toolbar_ = nullptr;
    QPoint dragOffset_;
    bool moving_ = false;
    qreal initialImageWidth_ = 0.0;
    int zoomPercent_ = 100;
    bool zoomIndicatorVisible_ = false;
    QTimer* zoomIndicatorTimer_ = nullptr;
};

} // namespace snipnexs
