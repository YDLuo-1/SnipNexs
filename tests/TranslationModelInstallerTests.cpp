#include "translate/TranslationModelInstaller.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUrl>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/installer-XXXXXX")));
    if (!temporary.isValid()) { return 1; }
    QCoreApplication::setOrganizationName(QStringLiteral("SnipNexsTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ModelInstaller"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    if (app.arguments().contains(QStringLiteral("--network"))) {
        if (!qEnvironmentVariableIsSet("SNIPNEXS_TEST_LLAMA_DOWNLOAD")) { return 77; }
        auto spec = snipnexs::knownTranslationModels().first();
        // Explicit opt-in: validates the real 1.1 GB Hy-MT2 download.
        snipnexs::TranslationModelInstaller installer(spec, nullptr, temporary.path());
        QSignalSpy done(&installer, &snipnexs::TranslationModelInstaller::finished);
        installer.start();
        const bool received = !done.isEmpty() || done.wait(540000);
        const bool passed = received && done.first().at(0).toBool();
        QTextStream(stdout) << "HTTPS model download: " << (passed ? "ok" : "failed")
            << (received ? done.first().at(1).toString() : QStringLiteral(" timeout")) << '\n';
        return passed ? 0 : 1;
    }
    const QString root = QDir(temporary.path()).filePath(QStringLiteral("models"));
    const QString source = QDir(temporary.path()).filePath(QStringLiteral("source.bin"));
    auto write = [](const QString& path, const QByteArray& bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    const QByteArray payload("tiny verified model");
    bool ok = write(source, payload);
    auto spec = snipnexs::knownTranslationModels().first();
    spec.distributionBase = QUrl::fromLocalFile(temporary.path() + QLatin1Char('/')).toString();
    spec.files = {{QStringLiteral("model.bin"), QStringLiteral("source.bin"),
        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex(), payload.size()}};
    const QString target = QDir(snipnexs::translationModelDirectory(spec.id, root)).filePath("model.bin");
    auto succeeded = [](QSignalSpy& spy) {
        return (!spy.isEmpty() || spy.wait(5000)) && spy.size() == 1 && spy.first().at(0).toBool();
    };
    snipnexs::TranslationModelInstaller first(spec, nullptr, root);
    QSignalSpy firstResult(&first, &snipnexs::TranslationModelInstaller::finished);
    first.start();
    snipnexs::TranslationModelInstaller second(spec, nullptr, root);
    QSignalSpy secondResult(&second, &snipnexs::TranslationModelInstaller::finished);
    second.start();
    ok &= secondResult.size() == 1 && !secondResult.first().at(0).toBool();
    QString error;
    ok &= !snipnexs::removeTranslationModel(spec.id, &error, root);
    second.cancel();
    ok &= succeeded(firstResult);
    ok &= snipnexs::isTranslationModelInstalled(spec, root);

    // Corrupt, too large, and short responses must preserve the existing good file.
    for (const auto& bad : {QByteArray(payload.size(), 'x'), QByteArray(300'000, 'x'), QByteArray("x")}) {
        ok &= write(source, bad);
        snipnexs::TranslationModelInstaller installer(spec, nullptr, root);
        QSignalSpy done(&installer, &snipnexs::TranslationModelInstaller::finished);
        installer.start();
        ok &= !succeeded(done) && done.size() == 1;
        QFile verified(target);
        ok &= verified.open(QIODevice::ReadOnly) && verified.readAll() == payload;
    }
    ok &= write(source, payload);
    const QString foreign = target + QStringLiteral(".part");
    ok &= write(foreign, "owned elsewhere");
    {
        snipnexs::TranslationModelInstaller installer(spec, nullptr, root);
        QSignalSpy done(&installer, &snipnexs::TranslationModelInstaller::finished);
        installer.start();
        installer.cancel();
        ok &= done.size() == 1 && !done.first().at(0).toBool();
    }
    ok &= QFile::exists(foreign);
    snipnexs::TranslationModelInstaller retry(spec, nullptr, root);
    QSignalSpy done(&retry, &snipnexs::TranslationModelInstaller::finished);
    retry.start();
    ok &= succeeded(done);
    QTextStream(stdout) << "translation installer: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
