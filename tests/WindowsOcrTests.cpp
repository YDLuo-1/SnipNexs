#include "ocr/WindowsOcr.h"

#include <QApplication>
#include <QImage>
#include <QTextStream>

#include <windows.h>

namespace {

QImage createTestImage(const wchar_t* first = L"1234567890", const wchar_t* second = L"0987654321", int fontSize = 120)
{
    constexpr int width = 1200;
    constexpr int height = 400;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC device = CreateCompatibleDC(nullptr);
    if (bitmap == nullptr || device == nullptr || bits == nullptr) {
        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }
        if (device != nullptr) {
            DeleteDC(device);
        }
        return {};
    }

    HGDIOBJ previousBitmap = SelectObject(device, bitmap);
    PatBlt(device, 0, 0, width, height, WHITENESS);
    HFONT font = CreateFontW(
        -fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HGDIOBJ previousFont = SelectObject(device, font);
    SetBkMode(device, TRANSPARENT);
    SetTextColor(device, RGB(0, 0, 0));
    TextOutW(device, 65, 50, first, static_cast<int>(wcslen(first)));
    TextOutW(device, 65, 230, second, static_cast<int>(wcslen(second)));
    GdiFlush();

    QImage image(
        static_cast<const uchar*>(bits),
        width,
        height,
        width * 4,
        QImage::Format_RGB32);
    QImage copy = image.copy();

    SelectObject(device, previousFont);
    SelectObject(device, previousBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(device);
    return copy;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    const QImage image = createTestImage();
    if (image.isNull()) {
        QTextStream(stdout) << "windows ocr: test image creation failed\n";
        return 1;
    }

    const snipnexs::windows_ocr::Result result = snipnexs::windows_ocr::recognize(image);
    if (!result.error.isEmpty()) {
        QTextStream(stdout) << "windows ocr: skipped (" << result.error << ")\n";
        return 77;
    }

    bool ok = result.text.contains(QStringLiteral("1234567890"))
        && result.text.contains(QStringLiteral("0987654321"));
    // The line list drives the bilingual view: joining it must reproduce the
    // joined text, and the two rendered rows must come back as two lines.
    ok &= !result.lines.isEmpty()
        && result.lines.join(QLatin1Char('\n')).trimmed() == result.text.trimmed();
    ok &= result.lines.size() == 2
        && result.lines.constFirst().contains(QStringLiteral("1234567890"))
        && result.lines.constLast().contains(QStringLiteral("0987654321"));
    const auto english = snipnexs::windows_ocr::recognize(createTestImage(
        L"Hello world.", L"Open the file.", 75));
    ok &= english.error.isEmpty() && english.text.contains(QStringLiteral("Hello world"))
        && english.text.contains(QStringLiteral("Open the file"));
    QTextStream(stdout) << "English OCR: " << english.text << " (" << english.languageTag << ")\n";
    const auto chinese = snipnexs::windows_ocr::recognize(createTestImage(
        L"你好世界", L"打开文件", 75));
    ok &= chinese.error.isEmpty() && chinese.text.contains(QStringLiteral("你好世界"))
        && chinese.text.contains(QStringLiteral("打开文件"));
    QTextStream(stdout) << "Chinese OCR: " << chinese.text << '\n';
    QTextStream(stdout) << (ok ? "windows ocr: ok" : "windows ocr: failed")
                        << " (" << result.languageTag << ", " << result.elapsedMs << " ms, text="
                        << result.text << ")\n";
    return ok ? 0 : 1;
}
