#include "CaptureToolbar.h"

#include "capture/ToolbarIcons.h"
#include "editor/AnnotationDocument.h"

#include <QButtonGroup>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QCoreApplication>

namespace snipnexs {

namespace {

QString overlayTranslate(const char* sourceText)
{
    // Keep the CaptureOverlay translation context: the tooltips predate the
    // extraction and their .ts entries live under that context.
    return QCoreApplication::translate("snipnexs::CaptureOverlay", sourceText);
}

void configureToolbarButton(
    QPushButton* button,
    ToolbarIcon icon,
    const QString& tooltip)
{
    button->setIcon(makeToolbarIcon(icon));
    button->setIconSize(QSize(23, 23));
    button->setToolTip(tooltip);
    button->setAccessibleName(tooltip);
    button->setFixedSize(32, 30);
}

} // namespace

CaptureToolbar buildCaptureToolbar(QWidget* parent)
{
    CaptureToolbar toolbar;
    toolbar.widget = new QFrame(parent);
    toolbar.widget->setObjectName(QStringLiteral("captureToolbar"));
    toolbar.widget->setCursor(Qt::ArrowCursor);
    auto* toolbarShadow = new QGraphicsDropShadowEffect(toolbar.widget);
    toolbarShadow->setBlurRadius(16);
    toolbarShadow->setOffset(0, 2);
    toolbarShadow->setColor(QColor(0, 0, 0, 55));
    toolbar.widget->setGraphicsEffect(toolbarShadow);
    auto* layout = new QHBoxLayout(toolbar.widget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);

    toolbar.penButton = new QPushButton(toolbar.widget);
    toolbar.rectangleButton = new QPushButton(toolbar.widget);
    toolbar.arrowButton = new QPushButton(toolbar.widget);
    toolbar.textButton = new QPushButton(toolbar.widget);
    toolbar.textButton->setObjectName(QStringLiteral("textButton"));
    toolbar.colorPickerButton = new QPushButton(toolbar.widget);
    toolbar.colorPickerButton->setObjectName(QStringLiteral("colorPickerButton"));
    toolbar.undoButton = new QPushButton(toolbar.widget);
    toolbar.redoButton = new QPushButton(toolbar.widget);
    toolbar.ocrButton = new QPushButton(toolbar.widget);
    toolbar.ocrButton->setObjectName(QStringLiteral("ocrButton"));
    toolbar.pinButton = new QPushButton(toolbar.widget);
    toolbar.pinButton->setObjectName(QStringLiteral("pinButton"));
    toolbar.recordButton = new QPushButton(toolbar.widget);
    toolbar.recordButton->setObjectName(QStringLiteral("recordButton"));
    toolbar.copyButton = new QPushButton(toolbar.widget);
    toolbar.saveButton = new QPushButton(toolbar.widget);
    toolbar.saveButton->setObjectName(QStringLiteral("saveButton"));
    toolbar.cancelButton = new QPushButton(toolbar.widget);
    toolbar.copyButton->setObjectName(QStringLiteral("copyButton"));

    configureToolbarButton(toolbar.penButton, ToolbarIcon::Pen,
                           overlayTranslate("画笔"));
    configureToolbarButton(toolbar.rectangleButton, ToolbarIcon::Rectangle,
                           overlayTranslate("矩形"));
    configureToolbarButton(toolbar.arrowButton, ToolbarIcon::Arrow,
                           overlayTranslate("箭头"));
    configureToolbarButton(toolbar.textButton, ToolbarIcon::Text,
                           overlayTranslate("文字"));
    configureToolbarButton(toolbar.colorPickerButton, ToolbarIcon::ColorPicker,
                           overlayTranslate("取色"));
    toolbar.colorPickerButton->setCheckable(true);
    configureToolbarButton(toolbar.undoButton, ToolbarIcon::Undo,
                           overlayTranslate("撤销"));
    configureToolbarButton(toolbar.redoButton, ToolbarIcon::Redo,
                           overlayTranslate("重做"));
    configureToolbarButton(toolbar.ocrButton, ToolbarIcon::Ocr,
                           overlayTranslate("识字"));
    configureToolbarButton(toolbar.pinButton, ToolbarIcon::Pin,
                           overlayTranslate("贴图"));
    configureToolbarButton(toolbar.recordButton, ToolbarIcon::Record,
                           overlayTranslate("录屏"));
    configureToolbarButton(toolbar.copyButton, ToolbarIcon::Copy,
                           overlayTranslate("复制"));
    configureToolbarButton(toolbar.saveButton, ToolbarIcon::Save,
                           overlayTranslate("保存"));
    configureToolbarButton(toolbar.cancelButton, ToolbarIcon::Cancel,
                           overlayTranslate("取消"));

    auto addSeparator = [widget = toolbar.widget, layout]() {
        auto* separator = new QFrame(widget);
        separator->setStyleSheet(QStringLiteral("background: #d5dde4;"));
        separator->setFixedSize(1, 20);
        layout->addSpacing(2);
        layout->addWidget(separator);
        layout->addSpacing(2);
    };

    toolbar.toolButtons = new QButtonGroup(parent);
    toolbar.toolButtons->setExclusive(true);
    toolbar.toolButtons->addButton(
        toolbar.penButton, static_cast<int>(AnnotationTool::Pen));
    toolbar.toolButtons->addButton(
        toolbar.rectangleButton, static_cast<int>(AnnotationTool::Rectangle));
    toolbar.toolButtons->addButton(
        toolbar.arrowButton, static_cast<int>(AnnotationTool::Arrow));
    toolbar.toolButtons->addButton(
        toolbar.textButton, static_cast<int>(AnnotationTool::Text));
    for (auto* button : toolbar.toolButtons->buttons()) {
        button->setCheckable(true);
    }

    layout->addWidget(toolbar.penButton);
    layout->addWidget(toolbar.rectangleButton);
    layout->addWidget(toolbar.arrowButton);
    layout->addWidget(toolbar.textButton);
    layout->addWidget(toolbar.colorPickerButton);
    addSeparator();
    layout->addWidget(toolbar.undoButton);
    layout->addWidget(toolbar.redoButton);
    addSeparator();
    layout->addWidget(toolbar.ocrButton);
    layout->addWidget(toolbar.pinButton);
    layout->addWidget(toolbar.recordButton);
    addSeparator();
    layout->addWidget(toolbar.copyButton);
    layout->addWidget(toolbar.saveButton);
    layout->addWidget(toolbar.cancelButton);
    toolbar.widget->hide();

    toolbar.widget->setStyleSheet(QStringLiteral(R"(
        QFrame#captureToolbar { background: #ffffff; border: 1px solid #c9d4de; border-radius: 6px; }
        QPushButton { padding: 0; background: transparent; border: 0; border-radius: 5px; }
        QPushButton:hover { background: #e8f1f9; }
        QPushButton:pressed { background: #d7e7f4; }
        QPushButton:checked { background: #238bda; }
        QPushButton:disabled { background: transparent; }
    )"));
    return toolbar;
}

} // namespace snipnexs
