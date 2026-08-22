#include "WindowsOcr.h"

#include <QElapsedTimer>
#include <QImage>
#include <QStringList>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace snipnexs::windows_ocr {
namespace {

class Apartment final
{
public:
    Apartment()
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
    }

    ~Apartment()
    {
        winrt::uninit_apartment();
    }
};

QString fromHString(const winrt::hstring& value)
{
    return QString::fromWCharArray(value.c_str(), static_cast<qsizetype>(value.size()));
}

winrt::Windows::Graphics::Imaging::SoftwareBitmap toSoftwareBitmap(const QImage& source)
{
    using namespace winrt::Windows::Graphics::Imaging;

    const QImage image = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    SoftwareBitmap bitmap(
        BitmapPixelFormat::Bgra8,
        image.width(),
        image.height(),
        BitmapAlphaMode::Premultiplied);
    const BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
    const BitmapPlaneDescription plane = buffer.GetPlaneDescription(0);
    const auto reference = buffer.CreateReference();
    const auto byteAccess = reference.as<winrt::impl::IMemoryBufferByteAccess>();

    uint8_t* destination = nullptr;
    uint32_t capacity = 0;
    winrt::check_hresult(byteAccess->GetBuffer(&destination, &capacity));

    const size_t rowBytes = static_cast<size_t>(image.width()) * 4;
    if (plane.Stride < static_cast<int32_t>(rowBytes) || plane.StartIndex < 0) {
        throw std::runtime_error("Windows OCR bitmap layout is unsupported");
    }
    const size_t required = static_cast<size_t>(plane.StartIndex)
        + static_cast<size_t>(image.height() - 1) * static_cast<size_t>(plane.Stride)
        + rowBytes;
    if (destination == nullptr || required > capacity) {
        throw std::runtime_error("Windows OCR bitmap buffer is too small");
    }

    for (int y = 0; y < image.height(); ++y) {
        std::memcpy(
            destination + plane.StartIndex + static_cast<size_t>(y) * plane.Stride,
            image.constScanLine(y),
            rowBytes);
    }
    return bitmap;
}

QString textFromResult(
    const winrt::Windows::Media::Ocr::OcrResult& result,
    const QString& languageTag)
{
    if (!languageTag.startsWith(QStringLiteral("zh"), Qt::CaseInsensitive)
        && !languageTag.startsWith(QStringLiteral("ja"), Qt::CaseInsensitive)) {
        return fromHString(result.Text()).trimmed();
    }

    QStringList lines;
    for (const auto& line : result.Lines()) {
        QString text;
        for (const auto& word : line.Words()) {
            text += fromHString(word.Text());
        }
        lines.append(text);
    }
    return lines.join(QLatin1Char('\n')).trimmed();
}

} // namespace

Result recognize(const QImage& input)
{
    Result output;
    QElapsedTimer timer;
    timer.start();

    if (input.isNull()) {
        output.error = QStringLiteral("OCR 输入图像为空。");
        return output;
    }

    try {
        Apartment apartment;
        using winrt::Windows::Media::Ocr::OcrEngine;

        const OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
        if (!engine) {
            output.error = QStringLiteral("未安装与当前用户语言匹配的 Windows OCR 语言包。");
            return output;
        }

        QImage image = input;
        const int maximum = static_cast<int>(engine.MaxImageDimension());
        if (image.width() > maximum || image.height() > maximum) {
            image = image.scaled(maximum, maximum, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        const auto bitmap = toSoftwareBitmap(image);
        const auto result = engine.RecognizeAsync(bitmap).get();
        output.languageTag = fromHString(engine.RecognizerLanguage().LanguageTag());
        output.text = textFromResult(result, output.languageTag);
        output.elapsedMs = timer.elapsed();
        return output;
    } catch (const winrt::hresult_error& error) {
        output.error = QStringLiteral("Windows OCR 调用失败：%1")
                           .arg(fromHString(error.message()));
    } catch (const std::exception& error) {
        output.error = QStringLiteral("OCR 处理失败：%1")
                           .arg(QString::fromUtf8(error.what()));
    }
    output.elapsedMs = timer.elapsed();
    return output;
}

} // namespace snipnexs::windows_ocr
