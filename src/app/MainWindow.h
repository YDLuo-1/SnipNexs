#pragma once

#include <QMainWindow>

class QCloseEvent;
class QSystemTrayIcon;

namespace snipnexs {

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void showAndActivate();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupTray();

    QSystemTrayIcon* trayIcon_ = nullptr;
    bool trayHintShown_ = false;
};

} // namespace snipnexs
