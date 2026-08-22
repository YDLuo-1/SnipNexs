#pragma once

#include <QElapsedTimer>
#include <QWidget>

class QCloseEvent;
class QEvent;
class QLabel;
class QPushButton;
class QShowEvent;
class QTimer;

namespace snipnexs {

class RecordingIndicator final : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingIndicator(QWidget* parent = nullptr);
    void setRecordingReady();
    void setStopping();
    void finish();

signals:
    void stopRequested();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void retranslateUi();
    void updateElapsed();

    QLabel* stateLabel_ = nullptr;
    QLabel* elapsedLabel_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QTimer* timer_ = nullptr;
    QElapsedTimer elapsed_;
    bool recording_ = true;
    bool stopping_ = false;
};

} // namespace snipnexs
