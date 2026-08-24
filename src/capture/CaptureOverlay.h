#pragma once

#include <QImage>
#include <QList>
#include <QPixmap>
#include <QWidget>

#include "editor/AnnotationDocument.h"

class QButtonGroup;
class QFrame;
class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPushButton;
class QShowEvent;

namespace snipnexs {

class CaptureOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureOverlay(QPixmap screenshot, QWidget* parent = nullptr);
    CaptureOverlay(
        QPixmap screenshot, QList<QImage> history, QWidget* parent = nullptr);
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
    enum class Interaction {
        None,
        Create,
        Move,
        Resize,
    };

    enum class ResizeHandle {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
    };

    void acceptCopy();
    void acceptOcr();
    void acceptPin();
    void acceptRecord();
    void acceptSave();
    void activateHistoryIndex(qsizetype index);
    void drawHistoryHint(QPainter& painter) const;
    [[nodiscard]] QRect handleRect(ResizeHandle handle) const;
    [[nodiscard]] ResizeHandle resizeHandleAt(const QPoint& point) const;
    [[nodiscard]] bool selectionGeometryEditable() const;
    void positionToolbar();
    void rebuildDimmedScreenshot();
    void switchHistory(int offset);
    void updateCursorForPosition(const QPoint& point);
    void updateEditorActions();
    void updateInteraction(const QPoint& point);

    QPixmap liveScreenshot_;
    QPixmap screenshot_;
    QPixmap dimmedScreenshot_;
    QList<QImage> history_;
    QImage activeHistoryImage_;
    qsizetype historyIndex_ = 0;
    QRect selection_;
    QRect interactionStartSelection_;
    QPoint interactionOrigin_;
    qreal devicePixelRatio_ = 1.0;
    Interaction interaction_ = Interaction::None;
    ResizeHandle activeResizeHandle_ = ResizeHandle::None;
    bool annotationDrawing_ = false;
    AnnotationTool annotationTool_ = AnnotationTool::None;
    AnnotationDocument annotations_;
    QFrame* toolbar_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    QButtonGroup* toolButtons_ = nullptr;
    QPushButton* recordButton_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* redoButton_ = nullptr;
};

} // namespace snipnexs
