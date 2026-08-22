#pragma once

#include "ScreenRecorderService.h"

#include <QObject>
#include <QPointer>

class QRect;
class QString;

namespace snipnexs {

class MainWindow;
class RecordingIndicator;

class RecorderController final : public QObject
{
    Q_OBJECT

public:
    explicit RecorderController(MainWindow& mainWindow, QObject* parent = nullptr);
    ~RecorderController() override;

public slots:
    void startRegion(quintptr monitorHandle, const QString& screenName, const QRect& pixelRect);
    void stop();

private:
    void handleCompleted(const RecordingResult& result);

    MainWindow& mainWindow_;
    ScreenRecorderService service_;
    QPointer<RecordingIndicator> indicator_;
    QString finalPath_;
    QString partialPath_;
};

} // namespace snipnexs
