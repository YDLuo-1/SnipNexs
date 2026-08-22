#include "app/AppIcon.h"
#include "app/MainWindow.h"
#include "capture/CaptureController.h"
#include "platform/windows/GlobalHotkey.h"
#include "recorder/RecorderController.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QComboBox>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>

namespace {

constexpr auto kInstanceName = "SnipNexs-0f355d9a-7ec7-44f8-9b7d-4df0350c2908";
constexpr auto kChineseLanguage = "zh_CN";
constexpr auto kEnglishLanguage = "en";
constexpr auto kEnglishTranslation = ":/i18n/snipnexs_en.qm";

QString supportedLanguage(const QString& language)
{
    return language == QLatin1String(kEnglishLanguage)
        ? QString::fromLatin1(kEnglishLanguage)
        : QString::fromLatin1(kChineseLanguage);
}

QString applyLanguage(
    QApplication& app,
    QTranslator& englishTranslator,
    bool englishTranslationAvailable,
    const QString& requestedLanguage)
{
    const QString language = supportedLanguage(requestedLanguage);
    app.removeTranslator(&englishTranslator);
    if (language == QLatin1String(kEnglishLanguage)
        && (!englishTranslationAvailable || !app.installTranslator(&englishTranslator))) {
        return QString::fromLatin1(kChineseLanguage);
    }
    return language;
}

bool notifyExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer(QString::fromLatin1(kInstanceName), QIODevice::WriteOnly);
    if (!socket.waitForConnected(180)) {
        return false;
    }
    socket.write("activate\n");
    socket.waitForBytesWritten(180);
    return true;
}

int runSelfTest(
    QApplication& app,
    QTranslator& englishTranslator,
    bool translationOk)
{
    const bool versionOk = !QCoreApplication::applicationVersion().isEmpty();
    const bool qtOk = QLibraryInfo::version() >= QVersionNumber(6, 8);
    snipnexs::MainWindow window;
    snipnexs::CaptureController captureController(window);
    snipnexs::RecorderController recorderController(window);
    snipnexs::GlobalHotkey captureHotkey;
    QString language = QString::fromLatin1(kChineseLanguage);
    bool languageSignalReceived = false;
    QObject::connect(&window, &snipnexs::MainWindow::captureRequested,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&captureHotkey, &snipnexs::GlobalHotkey::activated,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&window, &snipnexs::MainWindow::recordRequested,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&captureController, &snipnexs::CaptureController::recordRegionRequested,
        &recorderController, &snipnexs::RecorderController::startRegion);
    QObject::connect(&window, &snipnexs::MainWindow::languageChangeRequested,
        &app, [&](const QString& requestedLanguage) {
            languageSignalReceived = true;
            language = applyLanguage(
                app, englishTranslator, translationOk, requestedLanguage);
            window.setLanguageCode(language);
        });
    if (!captureHotkey.registerCaptureShortcut()) {
        window.setCaptureStatus(
            QCoreApplication::translate(
                "main",
                "全局快捷键 Ctrl+Shift+A 已被其他程序占用。\n"
                "仍可点击“区域截图”按钮使用截图功能。"));
    }
    window.show();
    auto* languageCombo = window.findChild<QComboBox*>(
        QStringLiteral("languageCombo"));
    const bool languageControlFound = languageCombo != nullptr;
    const int initialLanguageIndex = languageCombo != nullptr
        ? languageCombo->currentIndex()
        : -1;
    const int languageItemCount = languageCombo != nullptr
        ? languageCombo->count()
        : 0;
    if (languageCombo != nullptr) {
        languageCombo->setCurrentIndex(1);
    }

    QTimer::singleShot(50, &app, [&app, &window, &language, &languageSignalReceived,
                                   initialLanguageIndex, languageControlFound,
                                   languageItemCount, versionOk, qtOk, translationOk]() {
        const bool windowOk = window.isVisible() && !window.windowTitle().isEmpty();
        const auto* captureButton = window.findChild<QPushButton*>(
            QStringLiteral("primaryButton"));
        const bool translatedUiOk = captureButton != nullptr
            && captureButton->text() == QStringLiteral("Region Capture  Ctrl+Shift+A");
        const bool languageSwitchOk = languageSignalReceived
            && language == QLatin1String(kEnglishLanguage);
        const bool ok = versionOk && qtOk && windowOk && translationOk
            && languageSwitchOk && translatedUiOk;
        QTextStream output(stdout);
        output << "SnipNexs " << QCoreApplication::applicationVersion() << '\n'
               << "Qt " << QLibraryInfo::version().toString() << '\n'
               << "translations: " << (translationOk ? "ok" : "failed") << '\n'
               << "language-control: " << (languageControlFound ? "ok" : "failed") << '\n'
               << "language-control-state: " << initialLanguageIndex
               << " -> " << languageItemCount << " items\n"
               << "language-switch: " << (languageSwitchOk ? "ok" : "failed") << '\n'
               << "translated-ui: " << (translatedUiOk ? "ok" : "failed") << '\n'
               << "self-test: " << (ok ? "ok" : "failed") << '\n';
        output.flush();
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("SnipNexs"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SNIPNEXS_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("SnipNexs"));
    QApplication::setWindowIcon(snipnexs::createAppIcon());
    QApplication::setQuitOnLastWindowClosed(false);

    QSettings settings;
    QString language = supportedLanguage(
        settings.value(
            QStringLiteral("ui/language"), QString::fromLatin1(kChineseLanguage)).toString());
    QTranslator englishTranslator;
    const bool englishTranslationAvailable = englishTranslator.load(
        QString::fromLatin1(kEnglishTranslation));
    const bool englishTranslationOk = englishTranslationAvailable
        && englishTranslator.translate(
               "snipnexs::MainWindow", "区域截图  Ctrl+Shift+A")
            == QStringLiteral("Region Capture  Ctrl+Shift+A");
    language = applyLanguage(
        app, englishTranslator, englishTranslationOk, language);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("SnipNexs screenshot and recording toolkit"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption selfTestOption(
        QStringLiteral("self-test"), QStringLiteral("Run a non-interactive startup check."));
    parser.addOption(selfTestOption);
    parser.process(app);

    if (parser.isSet(selfTestOption)) {
        app.removeTranslator(&englishTranslator);
        return runSelfTest(app, englishTranslator, englishTranslationOk);
    }
    if (notifyExistingInstance()) {
        return 0;
    }

    QLocalServer::removeServer(QString::fromLatin1(kInstanceName));
    QLocalServer instanceServer;
    if (!instanceServer.listen(QString::fromLatin1(kInstanceName))) {
        QTextStream(stderr) << "Failed to create the single-instance endpoint: "
                            << instanceServer.errorString() << '\n';
        return 2;
    }

    snipnexs::MainWindow window;
    window.setLanguageCode(language);
    snipnexs::CaptureController captureController(window);
    snipnexs::RecorderController recorderController(window);
    snipnexs::GlobalHotkey captureHotkey;
    QObject::connect(&window, &snipnexs::MainWindow::captureRequested,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&captureHotkey, &snipnexs::GlobalHotkey::activated,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&window, &snipnexs::MainWindow::recordRequested,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&captureController, &snipnexs::CaptureController::recordRegionRequested,
        &recorderController, &snipnexs::RecorderController::startRegion);
    if (!captureHotkey.registerCaptureShortcut()) {
        window.setCaptureStatus(
            QCoreApplication::translate(
                "main",
                "全局快捷键 Ctrl+Shift+A 已被其他程序占用。\n"
                "仍可点击“区域截图”按钮使用截图功能。"));
    }
    QObject::connect(&window, &snipnexs::MainWindow::languageChangeRequested,
        &app, [&](const QString& requestedLanguage) {
            const QString nextLanguage = supportedLanguage(requestedLanguage);
            if (nextLanguage == language) {
                return;
            }

            language = applyLanguage(
                app, englishTranslator, englishTranslationOk, nextLanguage);
            window.setLanguageCode(language);
            if (nextLanguage == QLatin1String(kEnglishLanguage)
                && language != nextLanguage) {
                window.setCaptureStatus(QCoreApplication::translate(
                    "main", "无法加载英文翻译资源。"));
            }
            settings.setValue(QStringLiteral("ui/language"), language);
            settings.sync();
        });
    window.show();

    QObject::connect(&instanceServer, &QLocalServer::newConnection, &window, [&]() {
        while (QLocalSocket* socket = instanceServer.nextPendingConnection()) {
            QObject::connect(socket, &QLocalSocket::readyRead, &window, [&window, socket]() {
                if (socket->readAll().startsWith("activate")) {
                    QTimer::singleShot(0, &window, &snipnexs::MainWindow::showAndActivate);
                }
                socket->disconnectFromServer();
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    return app.exec();
}
