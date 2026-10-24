#include "SettingsDialog.h"

#include "UpdateChecker.h"
#include "UpdateVersion.h"
#include "translate/TranslationModels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace snipnexs {

namespace {

QString sizeText(qint64 bytes)
{
    if (bytes <= 0) {
        return QStringLiteral("—");
    }
    return QStringLiteral("%1 MB").arg(
        QString::number(bytes / (1024.0 * 1024.0), 'f', bytes > 512 * 1024 * 1024 ? 1 : 0));
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(460);

    audioCombo_ = new QComboBox(this);
    mirrorEdit_ = new QLineEdit(this);
    mirrorEdit_->setPlaceholderText(
        tr("留空使用默认 GitHub Release（https 或 file 镜像，目录布局需一致）"));

    updateCheck_ = new QCheckBox(this);
    checkNowButton_ = new QPushButton(this);

    modelsTable_ = new QTableWidget(this);
    modelsTable_->setColumnCount(3);
    modelsTable_->verticalHeader()->hide();
    modelsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    modelsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    modelsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    removeModelButton_ = new QPushButton(this);
    removeModelButton_->setEnabled(false);

    auto* recordingGroup = new QGroupBox(this);
    auto* recordingForm = new QFormLayout(recordingGroup);
    recordingForm->addRow(tr("录音音频"), audioCombo_);

    auto* translationGroup = new QGroupBox(this);
    auto* translationForm = new QFormLayout(translationGroup);
    translationForm->addRow(tr("模型镜像 URL"), mirrorEdit_);

    auto* updateGroup = new QGroupBox(this);
    auto* updateLayout = new QHBoxLayout(updateGroup);
    updateLayout->addWidget(updateCheck_, 1);
    updateLayout->addWidget(checkNowButton_);

    auto* modelsGroup = new QGroupBox(this);
    auto* modelsLayout = new QVBoxLayout(modelsGroup);
    modelsLayout->addWidget(modelsTable_, 1);
    auto* modelsButtons = new QHBoxLayout();
    modelsButtons->addStretch();
    modelsButtons->addWidget(removeModelButton_);
    modelsLayout->addLayout(modelsButtons);

    recordingGroup_ = recordingGroup;
    translationGroup_ = translationGroup;
    updateGroup_ = updateGroup;
    modelsGroup_ = modelsGroup;

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(recordingGroup);
    layout->addWidget(translationGroup);
    layout->addWidget(updateGroup);
    layout->addWidget(modelsGroup, 1);
    layout->addWidget(buttons);

    // Fill current values.
    mirrorEdit_->setText(QSettings().value(
        QStringLiteral("translation/modelBaseUrl")).toString());
    updateCheck_->setChecked(QSettings().value(
        QStringLiteral("update/checkAtStartup"), false).toBool());
    checkNowButton_->setEnabled(true);

    connect(checkNowButton_, &QPushButton::clicked, this, &SettingsDialog::checkForUpdate);
    connect(removeModelButton_, &QPushButton::clicked, this, &SettingsDialog::removeSelectedModel);
    connect(modelsTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = modelsTable_->currentRow();
        removeModelButton_->setEnabled(row >= 0
            && modelsTable_->item(row, 1)->data(Qt::UserRole).toBool());
    });

    refreshModels();
    retranslateUi();
}

void SettingsDialog::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(QStringLiteral("recording/audio"),
        audioCombo_->currentIndex() == 1   ? QStringLiteral("microphone")
        : audioCombo_->currentIndex() == 2 ? QStringLiteral("none")
                                           : QStringLiteral("system"));
    settings.setValue(QStringLiteral("translation/modelBaseUrl"),
        mirrorEdit_->text().trimmed());
    settings.setValue(QStringLiteral("update/checkAtStartup"),
        updateCheck_->isChecked());
    QDialog::accept();
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(tr("设置"));
    audioCombo_->clear();
    audioCombo_->addItem(tr("系统声音（默认）"));
    audioCombo_->addItem(tr("麦克风"));
    audioCombo_->addItem(tr("无（纯视频）"));
    // Keep the persisted selection across retranslation.
    const QString audio = QSettings().value(
        QStringLiteral("recording/audio")).toString();
    audioCombo_->setCurrentIndex(
        audio == QStringLiteral("microphone") ? 1
        : audio == QStringLiteral("none") ? 2
                                          : 0);

    recordingGroup_->setTitle(tr("录屏"));
    translationGroup_->setTitle(tr("本地翻译"));
    updateGroup_->setTitle(tr("更新"));
    modelsGroup_->setTitle(tr("本地翻译模型"));
    updateCheck_->setText(tr("启动时检查更新（仅在你开启后联网访问 GitHub）"));
    checkNowButton_->setText(tr("立即检查"));
    removeModelButton_->setText(tr("删除所选模型…"));
    modelsTable_->setHorizontalHeaderLabels(
        {tr("语言包"), tr("状态"), tr("大小")});
    modelsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    refreshModels();
}

void SettingsDialog::refreshModels()
{
    modelsTable_->setRowCount(0);
    for (const TranslationModelStatus& status : translationModelStatuses()) {
        const int row = modelsTable_->rowCount();
        modelsTable_->insertRow(row);

        auto* idItem = new QTableWidgetItem(status.id);
        idItem->setData(Qt::UserRole, status.id);
        QString engineName = status.engine == TranslationEngineId::LlamaHyMT2
            ? tr("高质量引擎")
            : tr("默认引擎");
        auto* statusItem = new QTableWidgetItem(
            status.installed ? tr("已安装 · %1").arg(engineName)
                             : tr("未安装 · %1").arg(engineName));
        statusItem->setData(Qt::UserRole, status.installed);
        auto* sizeItem = new QTableWidgetItem(
            status.installed
                ? tr("占用 %1").arg(sizeText(status.installedBytes))
                : tr("下载约 %1").arg(sizeText(status.packageBytes)));

        modelsTable_->setItem(row, 0, idItem);
        modelsTable_->setItem(row, 1, statusItem);
        modelsTable_->setItem(row, 2, sizeItem);
    }
    removeModelButton_->setEnabled(false);
}

void SettingsDialog::removeSelectedModel()
{
    const int row = modelsTable_->currentRow();
    if (row < 0) {
        return;
    }
    const QString id = modelsTable_->item(row, 0)->data(Qt::UserRole).toString();
    const auto answer = QMessageBox::question(
        this,
        tr("删除本地翻译模型"),
        tr("将删除语言包 %1 及其文件，再次翻译时会重新询问下载。是否继续？").arg(id));
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!removeTranslationModel(id, &error)) {
        QMessageBox::warning(this, tr("删除本地翻译模型"), error);
        return;
    }
    refreshModels();
}

void SettingsDialog::checkForUpdate()
{
    checkNowButton_->setEnabled(false);
    updateChecker_ = new UpdateChecker(this);
    connect(updateChecker_, &UpdateChecker::finished, this,
        [this](bool newerFound, const QString& version, const QString& error) {
            updateChecker_->deleteLater();
            updateChecker_ = nullptr;
            checkNowButton_->setEnabled(true);
            handleUpdateResult(newerFound, version, error);
        });
    updateChecker_->check();
}

void SettingsDialog::handleUpdateResult(
    bool newerFound, const QString& version, const QString& error)
{
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("检查更新"), tr("检查更新失败：%1").arg(error));
        return;
    }
    if (!newerFound) {
        QMessageBox::information(
            this, tr("检查更新"), tr("已是最新版本（当前 %1）。").arg(QLatin1String(SNIPNEXS_VERSION)));
        return;
    }
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("检查更新"));
    box.setText(tr("发现新版本 %1。").arg(version));
    QPushButton* open = box.addButton(tr("打开发布页"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Close);
    box.exec();
    if (box.clickedButton() == open) {
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://github.com/YDLuo-1/SnipNexs/releases/latest")));
    }
}

} // namespace snipnexs
