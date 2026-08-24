#pragma once

#include <QMainWindow>

class QAction;
class QCloseEvent;
class QComboBox;
class QEvent;
class QLabel;
class QPushButton;
class QSystemTrayIcon;

namespace snipnexs {

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    void setCaptureActive(bool active);
    void setCaptureShortcut(const QString& shortcut);
    void setLanguageCode(const QString& languageCode);
    void setCaptureStatus(const QString& text);
    void showNotification(const QString& title, const QString& message);

signals:
    void captureRequested();
    void languageChangeRequested(const QString& languageCode);
    void recordRequested();

public slots:
    void showAndActivate();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void retranslateUi();
    void showAbout();
    void setupUi();
    void setupTray();

    QSystemTrayIcon* trayIcon_ = nullptr;
    QLabel* subtitleLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* languageLabel_ = nullptr;
    QComboBox* languageCombo_ = nullptr;
    QPushButton* hideButton_ = nullptr;
    QPushButton* captureButton_ = nullptr;
    QPushButton* recordButton_ = nullptr;
    QPushButton* aboutButton_ = nullptr;
    QAction* captureAction_ = nullptr;
    QAction* recordAction_ = nullptr;
    QAction* openAction_ = nullptr;
    QAction* aboutAction_ = nullptr;
    QAction* quitAction_ = nullptr;
    QString languageCode_ = QStringLiteral("zh_CN");
    QString captureShortcut_ = QStringLiteral("F1");
    bool captureActive_ = false;
    bool trayHintShown_ = false;
};

} // namespace snipnexs
