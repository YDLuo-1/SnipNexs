#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QTabWidget;

namespace snipnexs {

class OpenSourceLicensesDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit OpenSourceLicensesDialog(QWidget* parent = nullptr);

private:
    QTabWidget* tabs_ = nullptr;
    QPushButton* closeButton_ = nullptr;
};

class AboutDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void retranslateUi();
    void showOpenSourceLicenses();

    QLabel* productLabel_ = nullptr;
    QLabel* copyrightLabel_ = nullptr;
    QLabel* licenseLabel_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QPushButton* licensesButton_ = nullptr;
    QPushButton* aboutQtButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
};

} // namespace snipnexs
