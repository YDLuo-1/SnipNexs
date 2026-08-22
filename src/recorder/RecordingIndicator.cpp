#include "RecordingIndicator.h"

#include <QCloseEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>

#include <Windows.h>

namespace snipnexs {

RecordingIndicator::RecordingIndicator(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 10, 10);
    layout->setSpacing(10);
    stateLabel_ = new QLabel(this);
    elapsedLabel_ = new QLabel(QStringLiteral("00:00"), this);
    stopButton_ = new QPushButton(this);
    stopButton_->setObjectName(QStringLiteral("stopButton"));
    layout->addWidget(stateLabel_);
    layout->addWidget(elapsedLabel_);
    layout->addWidget(stopButton_);

    setStyleSheet(QStringLiteral(R"(
        QWidget { background: #151b22; color: #eef3f7; border: 1px solid #34414e; border-radius: 8px; }
        QLabel { border: 0; background: transparent; font-size: 13px; }
        QPushButton { min-height: 30px; padding: 0 13px; color: #ffffff; background: #d84d57; border: 0; border-radius: 5px; font-weight: 600; }
        QPushButton:hover { background: #ea616b; }
        QPushButton:disabled { background: #5d4145; color: #b8a7a9; }
    )"));

    timer_ = new QTimer(this);
    timer_->setInterval(250);
    connect(timer_, &QTimer::timeout, this, &RecordingIndicator::updateElapsed);
    connect(stopButton_, &QPushButton::clicked, this, [this]() {
        setStopping();
        emit stopRequested();
    });
    retranslateUi();
    adjustSize();
}

void RecordingIndicator::setRecordingReady()
{
    if (stopping_) {
        return;
    }
    elapsed_.start();
    timer_->start();
    retranslateUi();
}

void RecordingIndicator::setStopping()
{
    if (!recording_ || stopping_) {
        return;
    }
    stopping_ = true;
    stopButton_->setEnabled(false);
    timer_->stop();
    retranslateUi();
}

void RecordingIndicator::finish()
{
    recording_ = false;
    close();
}

void RecordingIndicator::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void RecordingIndicator::closeEvent(QCloseEvent* event)
{
    if (recording_) {
        setStopping();
        emit stopRequested();
        event->ignore();
        return;
    }
    QWidget::closeEvent(event);
}

void RecordingIndicator::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    const auto windowHandle = reinterpret_cast<HWND>(winId());
    SetWindowDisplayAffinity(windowHandle, WDA_EXCLUDEFROMCAPTURE);
}

void RecordingIndicator::retranslateUi()
{
    setWindowTitle(tr("SnipNexs Recording"));
    stopButton_->setText(tr("停止录制"));
    if (stopping_) {
        stateLabel_->setText(tr("正在完成 MP4…"));
    } else if (elapsed_.isValid()) {
        stateLabel_->setText(tr("● 录制中"));
    } else {
        stateLabel_->setText(tr("● 正在启动"));
    }
    adjustSize();
}

void RecordingIndicator::updateElapsed()
{
    if (!elapsed_.isValid()) {
        return;
    }
    const qint64 totalSeconds = elapsed_.elapsed() / 1000;
    elapsedLabel_->setText(QStringLiteral("%1:%2")
        .arg(totalSeconds / 60, 2, 10, QLatin1Char('0'))
        .arg(totalSeconds % 60, 2, 10, QLatin1Char('0')));
}

} // namespace snipnexs
