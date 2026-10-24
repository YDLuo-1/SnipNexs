#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace snipnexs {

class UpdateChecker;

class SettingsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void accept() override;
    void retranslateUi();
    void refreshModels();
    void removeSelectedModel();
    void checkForUpdate();
    void handleUpdateResult(bool newerFound, const QString& version, const QString& error);

    QComboBox* audioCombo_ = nullptr;
    QLineEdit* mirrorEdit_ = nullptr;
    QCheckBox* updateCheck_ = nullptr;
    QPushButton* checkNowButton_ = nullptr;
    QGroupBox* recordingGroup_ = nullptr;
    QGroupBox* translationGroup_ = nullptr;
    QGroupBox* updateGroup_ = nullptr;
    QGroupBox* modelsGroup_ = nullptr;
    QTableWidget* modelsTable_ = nullptr;
    QPushButton* removeModelButton_ = nullptr;
    UpdateChecker* updateChecker_ = nullptr;
};

} // namespace snipnexs
