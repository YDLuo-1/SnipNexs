#pragma once

#include <QImage>
#include <QList>
#include <QPixmap>
#include <QWidget>

#include "editor/AnnotationDocument.h"

class QButtonGroup;
class QEvent;
class QFrame;
class QLabel;
class QKeyEvent;
class QLineEdit;
class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
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
    void setWindowTargets(QList<QRect> targets);
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
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
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
    void cancelTextEditing();
    [[nodiscard]] QColor colorAt(const QPoint& point, QPoint* sourcePoint = nullptr) const;
    void commitTextEditing();
    void copyPickedColor();
    void drawColorPicker(QPainter& painter) const;
    void drawHistoryHint(QPainter& painter) const;
    [[nodiscard]] QRect handleRect(ResizeHandle handle) const;
    void positionCaptureHint();
    void positionSelectionSizeLabel();
    [[nodiscard]] ResizeHandle resizeHandleAt(const QPoint& point) const;
    [[nodiscard]] bool selectionGeometryEditable() const;
    void positionToolbar();
    void rebuildDimmedScreenshot();
    void switchHistory(int offset);
    void setColorPickerActive(bool active);
    void startTextEditing(const QPoint& point);
    void updateCaptureHintVisibility(const QPoint& point);
    void updateCursorForPosition(const QPoint& point);
    void updateEditorActions();
    void updateInteraction(const QPoint& point);
    void updateWindowTarget(const QPoint& point);
    [[nodiscard]] QRect windowTargetAt(const QPoint& point) const;

    QPixmap liveScreenshot_;
    QPixmap screenshot_;
    QPixmap dimmedScreenshot_;
    QList<QImage> history_;
    QImage activeHistoryImage_;
    qsizetype historyIndex_ = 0;
    QList<QRect> windowTargets_;
    QRect hoveredWindowTarget_;
    QRect pressedWindowTarget_;
    QRect selection_;
    QRect interactionStartSelection_;
    QPoint interactionOrigin_;
    qreal devicePixelRatio_ = 1.0;
    Interaction interaction_ = Interaction::None;
    ResizeHandle activeResizeHandle_ = ResizeHandle::None;
    bool annotationDrawing_ = false;
    AnnotationTool annotationTool_ = AnnotationTool::None;
    AnnotationDocument annotations_;
    QLabel* captureHint_ = nullptr;
    QFrame* toolbar_ = nullptr;
    QLabel* selectionSizeLabel_ = nullptr;
    QButtonGroup* toolButtons_ = nullptr;
    QLineEdit* textEditor_ = nullptr;
    QPushButton* colorPickerButton_ = nullptr;
    QPushButton* recordButton_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* redoButton_ = nullptr;
    QPoint textAnchor_;
    QPoint colorPickerPoint_;
    bool colorPickerActive_ = false;
    bool colorPickerHex_ = false;
};

} // namespace snipnexs
