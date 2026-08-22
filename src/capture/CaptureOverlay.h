#pragma once

#include <QImage>
#include <QPixmap>
#include <QWidget>

#include "editor/AnnotationDocument.h"

class QButtonGroup;
class QFrame;
class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QShowEvent;

namespace snipnexs {

class CaptureOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureOverlay(QPixmap screenshot, QWidget* parent = nullptr);
    void setSelection(const QRect& selection);
    void setAnnotationTool(AnnotationTool tool);
    [[nodiscard]] QImage selectedImage() const;
    [[nodiscard]] QRect selectedPixelRect() const;

signals:
    void copyRequested(const QImage& image);
    void ocrRequested(const QImage& image);
    void saveRequested(const QImage& image);
    void pinRequested(const QImage& image);
    void recordRequested(const QRect& pixelRect);
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
    void acceptOcr();
    void acceptPin();
    void acceptRecord();
    void acceptSave();
    void positionToolbar();
    void updateEditorActions();
    void updateSelection(const QPoint& point);

    QPixmap screenshot_;
    QPixmap dimmedScreenshot_;
    QRect selection_;
    QPoint dragOrigin_;
    qreal devicePixelRatio_ = 1.0;
    bool dragging_ = false;
    bool annotationDrawing_ = false;
    AnnotationTool annotationTool_ = AnnotationTool::None;
    AnnotationDocument annotations_;
    QFrame* toolbar_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    QButtonGroup* toolButtons_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* redoButton_ = nullptr;
};

} // namespace snipnexs
