#include "ocr/OcrResultWindow.h"
#include "translate/TranslationService.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFontDatabase>
#include <QFont>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QtTest>

namespace {
QStringList submittedLines;
int requests = 0;
int cancelRequests = 0;
QString submittedSource, submittedTarget;
}

namespace snipnexs {
// Replace inference only: exercise the real result window and its signal wiring.
TranslationService::TranslationService(QObject* parent) : QObject(parent) {}
TranslationService::~TranslationService() = default;
void TranslationService::cancel() { cancelRequested_.store(true); ++cancelRequests; }
bool TranslationService::translate(const QString& text, const QString& source, const QString& target)
{
    if (busy_) { return false; }
    busy_ = true;
    cancelRequested_.store(false);
    submittedSource = source;
    submittedTarget = target;
    ++requests;
    submittedLines = text.split(QLatin1Char('\n'));
    emit busyChanged(true);
    QTimer::singleShot(1, this, [this, text] {
        busy_ = false;
        if (cancelRequested_.load()) { emit canceled(); emit busyChanged(false); return; }
        emit busyChanged(false);
        emit translated(text.toUpper(), 1);
    });
    return true;
}
bool TranslationService::translateLines(const QStringList& lines, const QString& source, const QString& target)
{
    if (busy_) { return false; }
    busy_ = true;
    cancelRequested_.store(false);
    submittedSource = source;
    submittedTarget = target;
    ++requests;
    submittedLines = lines;
    emit busyChanged(true);
    QTimer::singleShot(1, this, [this, lines] {
        busy_ = false;
        if (cancelRequested_.load()) { emit canceled(); emit busyChanged(false); return; }
        emit busyChanged(false);
        emit linesTranslated(lines, 1);
    });
    return true;
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (!qEnvironmentVariable("SNIPNEXS_UI_EVIDENCE").isEmpty()) {
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/msyh.ttc"));
        app.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    }
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/ocr-ui-XXXXXX")));
    if (!temporary.isValid()) { return 1; }
    QCoreApplication::setOrganizationName(QStringLiteral("SnipNexsTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    snipnexs::TranslationService service;
    snipnexs::OcrResultWindow window("one\ntwo", {"one", "two"}, "zh-Hans-CN", 0, &service);
    auto* editor = window.findChild<QPlainTextEdit*>("sourceEditor");
    auto* source = window.findChild<QPlainTextEdit*>("alignedSource");
    auto* target = window.findChild<QPlainTextEdit*>("alignedTarget");
    auto* engine = window.findChild<QComboBox*>("engineCombo");
    auto* align = window.findChild<QCheckBox*>("alignCheck");
    auto* translate = window.findChild<QPushButton*>("localTranslateButton");
    bool ok = editor && source && target && !engine && align && translate;
    if (!ok) { return 1; }
    editor->setPlainText("corrected\nsecond");
    align->setChecked(true);
    ok &= submittedLines == QStringList({"corrected", "second"});
    ok &= submittedSource == QStringLiteral("en") && submittedTarget.isEmpty();
    ok &= editor->isReadOnly();
    snipnexs::OcrResultWindow other("waiting", {"waiting"}, "en", 0, &service);
    auto* otherTranslate = other.findChild<QPushButton*>("localTranslateButton");
    ok &= otherTranslate && !otherTranslate->isEnabled();
    QTest::qWait(20);
    ok &= otherTranslate->isEnabled();
    ok &= target->toPlainText() == editor->toPlainText() && requests == 1;
    auto* language = window.findChild<QComboBox*>("targetCombo");
    if (!language) { return 1; }
    language->setCurrentIndex(language->findData(QStringLiteral("zh")));
    ok &= target->toPlainText().isEmpty();
    translate->click();
    QTest::qWait(20);
    ok &= requests == 2 && submittedTarget == QStringLiteral("zh");
    editor->setPlainText("changed\nagain");
    ok &= target->toPlainText().isEmpty() && source->toPlainText() == editor->toPlainText();
    translate->click();
    QTest::qWait(20);
    ok &= requests == 3 && submittedLines == QStringList({"changed", "again"});
    align->setChecked(false);
    auto* plain = window.findChild<QPlainTextEdit*>("translationEditor");
    ok &= plain->toPlainText() == target->toPlainText();
    auto* cancel = window.findChild<QPushButton*>("cancelTranslationButton");
    ok &= language && cancel;
    language->setCurrentIndex(language->findData(QStringLiteral("zh")));
    translate->click();
    ok &= submittedSource.isEmpty() && submittedTarget == QStringLiteral("zh");
    cancel->click();
    QTest::qWait(20);
    ok &= cancelRequests == 1 && plain->toPlainText().isEmpty() && !service.isBusy();
    auto* closing = new snipnexs::OcrResultWindow("close me", {"close me"}, "zh", 0, &service);
    closing->findChild<QPushButton*>("localTranslateButton")->click();
    delete closing;
    QTest::qWait(20);
    ok &= cancelRequests == 2 && !service.isBusy();
    // Closing an unrelated window must not cancel somebody else's request.
    translate->click();
    delete new snipnexs::OcrResultWindow("other", {"other"}, "en", 0, &service);
    QTest::qWait(20);
    ok &= cancelRequests == 2 && !plain->toPlainText().isEmpty();
    if (!qEnvironmentVariable("SNIPNEXS_UI_EVIDENCE").isEmpty()) {
        window.show();
        QTest::qWait(20);
        ok &= window.grab().save(qEnvironmentVariable("SNIPNEXS_UI_EVIDENCE"));
        window.hide();
    }
    QTextStream(stdout) << "OCR result state: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
