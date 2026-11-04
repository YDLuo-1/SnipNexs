#include "app/AppIcon.h"
#include "app/AppSettings.h"
#include "app/AboutDialog.h"
#include "app/MainWindow.h"
#include "capture/CaptureController.h"
#include "platform/windows/GlobalHotkey.h"
#include "recorder/RecorderController.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QLibraryInfo>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QPushButton>
#include <QSslSocket>
#include <QTextStream>
#include <QTimer>
#include <QTemporaryDir>
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

bool notifyExistingInstance(const QByteArray& command)
{
    QLocalSocket socket;
    socket.connectToServer(QString::fromLatin1(kInstanceName), QIODevice::WriteOnly);
    if (!socket.waitForConnected(180)) {
        return false;
    }
    socket.write(command);
    socket.waitForBytesWritten(180);
    return true;
}

void configureCaptureHotkey(
    snipnexs::GlobalHotkey& hotkey,
    snipnexs::MainWindow& window,
    bool showNotification)
{
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        window.setCaptureShortcut({});
        return;
    }
    const bool registered = hotkey.registerCaptureShortcut();
    window.setCaptureShortcut(hotkey.shortcutText());

    QString title;
    QString message;
    if (hotkey.isUsingFallback()) {
        title = QCoreApplication::translate("main", "快捷键已回退");
        message = QCoreApplication::translate(
            "main",
            "F1 无法注册（通常是被其他程序占用），当前已改用 Ctrl+Shift+A。");
    } else if (!registered) {
        title = QCoreApplication::translate("main", "全局快捷键不可用");
        message = QCoreApplication::translate(
            "main",
            "F1 和 Ctrl+Shift+A 都无法注册。\n"
            "仍可点击“区域截图”按钮使用截图功能。");
    }

    if (!message.isEmpty()) {
        window.setCaptureStatus(message);
        if (showNotification) {
            window.showNotification(title, message);
        }
    }
}

int runRuntimeSelfTest(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("SnipNexs"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SNIPNEXS_VERSION));

    // Deliberately avoids QApplication: this mode is also used against the
    // deployed layout, where only the plugins/ folder shipped with the app is
    // available (no qoffscreen/qwindows platform plugin required).
    const bool tlsOk = QSslSocket::supportsSsl();
    const QDir binaryDirectory(QCoreApplication::applicationDirPath());
    QLibrary llama(binaryDirectory.filePath(QStringLiteral("llama")));
    const bool llamaOk = llama.load();
    QLibrary ggml(binaryDirectory.filePath(QStringLiteral("ggml")));
    using LoadBackends = void (*)(const char*);
    using DeviceCount = size_t (*)();
    const auto loadBackends = reinterpret_cast<LoadBackends>(ggml.resolve("ggml_backend_load_all_from_path"));
    const auto deviceCount = reinterpret_cast<DeviceCount>(ggml.resolve("ggml_backend_dev_count"));
    if (loadBackends) {
        loadBackends(binaryDirectory.absolutePath().toUtf8().constData());
    }
    const bool cpuBackendOk = deviceCount && deviceCount() > 0;

    const bool ok = tlsOk && llamaOk && cpuBackendOk;
    QTextStream output(stdout);
    output << "SnipNexs " << QCoreApplication::applicationVersion() << '\n'
           << "tls: " << (tlsOk ? "ok" : "failed")
           << " (" << QSslSocket::sslLibraryVersionString() << ")\n"
           << "llama: " << (llamaOk ? "ok" : "failed");
    if (!llamaOk) {
        output << " (" << llama.errorString() << ')';
    }
    output << '\n'
           << "cpu-backend: " << (cpuBackendOk ? "ok" : "failed") << '\n'
           << "runtime-self-test: " << (ok ? "ok" : "failed") << '\n';
    output.flush();
    return ok ? 0 : 1;
}

int runSelfTest(
    QApplication& app,
    QTranslator& englishTranslator,
    bool translationOk)
{
    const bool versionOk = !QCoreApplication::applicationVersion().isEmpty();
    const bool qtOk = QLibraryInfo::version() >= QVersionNumber(6, 8);
    snipnexs::MainWindow window;
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir historyDirectory(QDir::current().filePath(QStringLiteral("test-temp/self-test-XXXXXX")));
    if (!historyDirectory.isValid()) {
        return 1;
    }
    snipnexs::CaptureController captureController(window, nullptr, historyDirectory.path());
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
            configureCaptureHotkey(captureHotkey, window, false);
        });
    configureCaptureHotkey(captureHotkey, window, false);
    if (window.trayAvailable()) {
        window.hide();
    } else {
        window.show();
    }
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
    window.setCaptureActive(true);
    window.showAndActivate();
    const bool captureBlocksMainWindow = !window.isVisible();
    window.setCaptureActive(false);
    if (window.trayAvailable()) {
        window.hide();
    } else {
        window.show();
    }

    const QString captureShortcut = captureHotkey.shortcutText();
    QTimer::singleShot(50, &app, [&app, &window, &language, &languageSignalReceived,
                                   initialLanguageIndex, languageControlFound,
                                   languageItemCount, versionOk, qtOk, translationOk,
                                   captureBlocksMainWindow, captureShortcut]() {
        const bool windowOk = window.isVisible() && !window.windowTitle().isEmpty();
        const auto* captureButton = window.findChild<QPushButton*>(
            QStringLiteral("primaryButton"));
        const QString expectedCaptureText = captureShortcut.isEmpty()
            ? QStringLiteral("Region Capture")
            : QStringLiteral("Region Capture  %1").arg(captureShortcut);
        const bool translatedUiOk = captureButton != nullptr
            && captureButton->text() == expectedCaptureText;
        const auto* aboutButton = window.findChild<QPushButton*>(
            QStringLiteral("aboutButton"));
        snipnexs::AboutDialog aboutDialog(&window);
        const auto* aboutProduct = aboutDialog.findChild<QLabel*>(
            QStringLiteral("aboutProduct"));
        const bool translatedAboutOk = aboutButton != nullptr
            && aboutButton->text() == QStringLiteral("About")
            && aboutProduct != nullptr
            && aboutProduct->text().contains(QStringLiteral("64-bit Windows"));
        const bool languageSwitchOk = languageSignalReceived
            && language == QLatin1String(kEnglishLanguage);
        const bool ok = versionOk && qtOk && windowOk && translationOk
            && languageSwitchOk && translatedUiOk && translatedAboutOk
            && captureBlocksMainWindow;
        QTextStream output(stdout);
        output << "SnipNexs " << QCoreApplication::applicationVersion() << '\n'
               << "Qt " << QLibraryInfo::version().toString() << '\n'
               << "translations: " << (translationOk ? "ok" : "failed") << '\n'
               << "language-control: " << (languageControlFound ? "ok" : "failed") << '\n'
               << "language-control-state: " << initialLanguageIndex
               << " -> " << languageItemCount << " items\n"
               << "language-switch: " << (languageSwitchOk ? "ok" : "failed") << '\n'
               << "capture-shortcut: "
               << (captureShortcut.isEmpty() ? "unavailable" : captureShortcut) << '\n'
               << "translated-ui: " << (translatedUiOk ? "ok" : "failed") << '\n'
               << "translated-about: " << (translatedAboutOk ? "ok" : "failed") << '\n'
               << "capture-blocks-main-window: "
               << (captureBlocksMainWindow ? "ok" : "failed") << '\n'
               << "self-test: " << (ok ? "ok" : "failed") << '\n';
        output.flush();
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}

} // namespace

int main(int argc, char* argv[])
{
    for (int index = 1; index < argc; ++index) {
        if (qstrcmp(argv[index], "--self-test-runtime") == 0) {
            return runRuntimeSelfTest(argc, argv);
        }
    }

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("SnipNexs"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SNIPNEXS_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("SnipNexs"));
    snipnexs::configureAppSettings(QCoreApplication::applicationDirPath(),
        QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .filePath(QStringLiteral("SnipNexs")));
    QApplication::setWindowIcon(snipnexs::createAppIcon());
    QApplication::setQuitOnLastWindowClosed(false);

    QString language = supportedLanguage(
        snipnexs::AppSettings().value(
            QStringLiteral("ui/language"), QString::fromLatin1(kChineseLanguage)).toString());
    QTranslator englishTranslator;
    const bool englishTranslationAvailable = englishTranslator.load(
        QString::fromLatin1(kEnglishTranslation));
    const bool englishTranslationOk = englishTranslationAvailable
        && englishTranslator.translate(
               "snipnexs::MainWindow", "区域截图")
            == QStringLiteral("Region Capture");
    language = applyLanguage(
        app, englishTranslator, englishTranslationOk, language);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("SnipNexs screenshot and recording toolkit"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption selfTestOption(
        QStringLiteral("self-test"), QStringLiteral("Run a non-interactive startup check."));
    const QCommandLineOption quitExistingOption(
        QStringLiteral("quit-existing"), QStringLiteral("Ask a running instance to exit."));
    parser.addOption(selfTestOption);
    parser.addOption(quitExistingOption);
    parser.process(app);

    if (parser.isSet(selfTestOption)) {
        app.removeTranslator(&englishTranslator);
        return runSelfTest(app, englishTranslator, englishTranslationOk);
    }
    if (parser.isSet(quitExistingOption)) {
        return notifyExistingInstance(QByteArrayLiteral("quit\n")) ? 0 : 3;
    }
    if (notifyExistingInstance(QByteArrayLiteral("activate\n"))) {
        return 0;
    }

    const QString instanceName = QString::fromLatin1(kInstanceName);
    QLocalServer instanceServer;
    if (!instanceServer.listen(instanceName)) {
        // A live instance may have started after the probe above. Ask it to
        // activate before removing an endpoint that may still be in use.
        if (notifyExistingInstance(QByteArrayLiteral("activate\n"))) {
            return 0;
        }
        QLocalServer::removeServer(instanceName);
        if (!instanceServer.listen(instanceName)) {
            QTextStream(stderr) << "Failed to create the single-instance endpoint: "
                                << instanceServer.errorString() << '\n';
            return 2;
        }
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
    configureCaptureHotkey(captureHotkey, window, true);
    QObject::connect(&window, &snipnexs::MainWindow::languageChangeRequested,
        &app, [&](const QString& requestedLanguage) {
            const QString nextLanguage = supportedLanguage(requestedLanguage);
            if (nextLanguage == language) {
                return;
            }

            language = applyLanguage(
                app, englishTranslator, englishTranslationOk, nextLanguage);
            window.setLanguageCode(language);
            configureCaptureHotkey(captureHotkey, window, false);
            if (nextLanguage == QLatin1String(kEnglishLanguage)
                && language != nextLanguage) {
                window.setCaptureStatus(QCoreApplication::translate(
                    "main", "无法加载英文翻译资源。"));
            }
            snipnexs::AppSettings settings;
            settings.setValue(QStringLiteral("ui/language"), language);
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                window.setCaptureStatus(QCoreApplication::translate(
                    "main", "语言已切换，但无法保存到配置文件：%1")
                    .arg(QDir::toNativeSeparators(settings.fileName())));
                window.showAndActivate();
            }
        });
    if (window.trayAvailable()) {
        window.hide();
    } else {
        // No working tray icon: keep the window visible instead of leaving
        // an unreachable background process.
        window.show();
    }

    QObject::connect(&instanceServer, &QLocalServer::newConnection, &window, [&]() {
        while (QLocalSocket* socket = instanceServer.nextPendingConnection()) {
            QObject::connect(socket, &QLocalSocket::readyRead, &window, [&window, socket]() {
                const QByteArray command = socket->readAll();
                if (command.startsWith("quit")) {
                    QTimer::singleShot(0, qApp, &QApplication::quit);
                } else if (command.startsWith("activate")) {
                    QTimer::singleShot(0, &window, &snipnexs::MainWindow::showAndActivate);
                }
                socket->disconnectFromServer();
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    return app.exec();
}
