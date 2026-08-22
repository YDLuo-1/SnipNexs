#pragma once

#include <QMainWindow>

class QCloseEvent;
class QLabel;
class QSystemTrayIcon;

namespace snipnexs {

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    void setCaptureStatus(const QString& text);
    void showNotification(const QString& title, const QString& message);

signals:
    void captureRequested();

public slots:
    void showAndActivate();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupTray();

    QSystemTrayIcon* trayIcon_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    bool trayHintShown_ = false;
};

} // namespace snipnexs
