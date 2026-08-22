#pragma once

#include <QImage>
#include <QPixmap>
#include <QWidget>

class QFrame;
class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QShowEvent;

namespace snipnexs {

class CaptureOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureOverlay(QPixmap screenshot, QWidget* parent = nullptr);
    void setSelection(const QRect& selection);
    [[nodiscard]] QImage selectedImage() const;

signals:
    void copyRequested(const QImage& image);
    void saveRequested(const QImage& image);
    void canceled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void acceptCopy();
    void acceptSave();
    void positionToolbar();
    void updateSelection(const QPoint& point);

    QPixmap screenshot_;
    QPixmap dimmedScreenshot_;
    QRect selection_;
    QPoint dragOrigin_;
    qreal devicePixelRatio_ = 1.0;
    bool dragging_ = false;
    QFrame* toolbar_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
};

} // namespace snipnexs
