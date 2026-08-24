#pragma once

#include <QElapsedTimer>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>

class QCloseEvent;
class QEvent;
class QLabel;
class QMouseEvent;
class QPushButton;
class QShowEvent;
class QTimer;

namespace snipnexs {

[[nodiscard]] QPoint recordingIndicatorBottomRightPosition(
    const QRect& availableGeometry, const QSize& indicatorSize, int margin = 16);

class RecordingIndicator final : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingIndicator(QWidget* parent = nullptr);
    void moveToBottomRight(const QString& screenName);
    void setRecordingReady();
    void setStopping();
    void finish();

signals:
    void stopRequested();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void retranslateUi();
    void updateElapsed();

    QLabel* stateLabel_ = nullptr;
    QLabel* elapsedLabel_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QTimer* timer_ = nullptr;
    QElapsedTimer elapsed_;
    QPoint dragOffset_;
    bool dragging_ = false;
    bool recording_ = true;
    bool stopping_ = false;
};

} // namespace snipnexs
