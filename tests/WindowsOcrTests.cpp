#include "ocr/WindowsOcr.h"

#include <QApplication>
#include <QImage>
#include <QTextStream>

#include <windows.h>

namespace {

QImage createTestImage()
{
    constexpr int width = 1200;
    constexpr int height = 260;
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
        -150, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HGDIOBJ previousFont = SelectObject(device, font);
    SetBkMode(device, TRANSPARENT);
    SetTextColor(device, RGB(0, 0, 0));
    const wchar_t text[] = L"1234567890";
    TextOutW(device, 65, 45, text, 10);

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

    const bool ok = result.text.contains(QStringLiteral("1234567890"));
    QTextStream(stdout) << (ok ? "windows ocr: ok" : "windows ocr: failed")
                        << " (" << result.languageTag << ", " << result.elapsedMs << " ms, text="
                        << result.text << ")\n";
    return ok ? 0 : 1;
}
