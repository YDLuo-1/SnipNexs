#include "app/AppIcon.h"
#include "app/MainWindow.h"
#include "capture/CaptureController.h"
#include "platform/windows/GlobalHotkey.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTextStream>
#include <QTimer>

namespace {

constexpr auto kInstanceName = "SnipNexs-0f355d9a-7ec7-44f8-9b7d-4df0350c2908";

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

int runSelfTest(QApplication& app)
{
    const bool versionOk = !QCoreApplication::applicationVersion().isEmpty();
    const bool qtOk = QLibraryInfo::version() >= QVersionNumber(6, 8);
    snipnexs::MainWindow window;
    snipnexs::CaptureController captureController(window);
    snipnexs::GlobalHotkey captureHotkey;
    QObject::connect(&window, &snipnexs::MainWindow::captureRequested,
        &captureController, &snipnexs::CaptureController::startCapture);
    QObject::connect(&captureHotkey, &snipnexs::GlobalHotkey::activated,
        &captureController, &snipnexs::CaptureController::startCapture);
    if (!captureHotkey.registerCaptureShortcut()) {
        window.setCaptureStatus(
            QStringLiteral("全局快捷键 Ctrl+Shift+A 已被其他程序占用。\n"
                           "仍可点击“区域截图”按钮使用截图功能。"));
    }
    window.show();

    QTimer::singleShot(50, &app, [&app, &window, versionOk, qtOk]() {
        const bool windowOk = window.isVisible() && !window.windowTitle().isEmpty();
        QTextStream output(stdout);
        output << "SnipNexs " << QCoreApplication::applicationVersion() << '\n'
               << "Qt " << QLibraryInfo::version().toString() << '\n'
               << "self-test: " << (versionOk && qtOk && windowOk ? "ok" : "failed") << '\n';
        output.flush();
        app.exit(versionOk && qtOk && windowOk ? 0 : 1);
    });
    return app.exec();
}

int runCaptureProbe(QApplication& app)
{
    snipnexs::MainWindow window;
    snipnexs::CaptureController captureController(window);
    window.show();
    QTimer::singleShot(50, &captureController, &snipnexs::CaptureController::startCapture);
    QTimer::singleShot(850, &app, [&app]() {
        QWidget* captureWindow = nullptr;
        for (QWidget* topLevel : QApplication::topLevelWidgets()) {
            if (topLevel->windowTitle() == QStringLiteral("SnipNexs Capture")) {
                captureWindow = topLevel;
                break;
            }
        }

        const bool ok = captureWindow != nullptr
            && captureWindow->isVisible()
            && captureWindow->width() > 0
            && captureWindow->height() > 0;
        QTextStream output(stdout);
        output << "capture-probe: " << (ok ? "ok" : "failed");
        if (captureWindow != nullptr) {
            output << " (" << captureWindow->width() << 'x' << captureWindow->height() << ')';
            captureWindow->close();
        }
        output << '\n';
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

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("SnipNexs screenshot toolkit"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption selfTestOption(
        QStringLiteral("self-test"), QStringLiteral("Run a non-interactive startup check."));
    const QCommandLineOption captureProbeOption(
        QStringLiteral("capture-probe"),
        QStringLiteral("Capture the current screen and verify the selection overlay."));
    parser.addOption(selfTestOption);
    parser.addOption(captureProbeOption);
    parser.process(app);

    if (parser.isSet(selfTestOption)) {
        return runSelfTest(app);
    }
    if (parser.isSet(captureProbeOption)) {
        return runCaptureProbe(app);
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
