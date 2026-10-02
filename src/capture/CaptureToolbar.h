#pragma once

#include <QFrame>
#include <QPushButton>

class QButtonGroup;

namespace snipnexs {

// The capture toolbar widget with all action buttons preconfigured (icons,
// tooltips, styling, shadow). Tool buttons carry AnnotationTool ids in
// toolButtons and are exclusive/checkable. Signal wiring stays with the
// owner (CaptureOverlay); button tooltips intentionally translate in the
// CaptureOverlay context so existing translation entries keep resolving.
struct CaptureToolbar {
    QFrame* widget = nullptr;
    QButtonGroup* toolButtons = nullptr;
    QPushButton* penButton = nullptr;
    QPushButton* rectangleButton = nullptr;
    QPushButton* arrowButton = nullptr;
    QPushButton* textButton = nullptr;
    QPushButton* colorPickerButton = nullptr;
    QPushButton* undoButton = nullptr;
    QPushButton* redoButton = nullptr;
    QPushButton* ocrButton = nullptr;
    QPushButton* pinButton = nullptr;
    QPushButton* recordButton = nullptr;
    QPushButton* copyButton = nullptr;
    QPushButton* saveButton = nullptr;
    QPushButton* cancelButton = nullptr;
};

[[nodiscard]] CaptureToolbar buildCaptureToolbar(QWidget* parent);

} // namespace snipnexs
