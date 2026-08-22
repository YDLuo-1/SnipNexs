#include "NativeScreenRecorder.h"

#include <QElapsedTimer>
#include <QDir>
#include <QFileInfo>

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <shcore.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <wrl/client.h>

#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.MediaProperties.h>
#include <winrt/Windows.Media.Transcoding.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>

namespace snipnexs {
namespace {

using Microsoft::WRL::ComPtr;
using namespace std::chrono_literals;
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::Graphics;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media::Core;
using namespace winrt::Windows::Media::MediaProperties;
using namespace winrt::Windows::Media::Transcoding;
using namespace winrt::Windows::Storage;

struct ApartmentGuard
{
    ApartmentGuard() { init_apartment(apartment_type::multi_threaded); }
    ~ApartmentGuard() { uninit_apartment(); }
};

struct MonitorSearch
{
    QString requestedName;
    HMONITOR monitor = nullptr;
};

BOOL CALLBACK findMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM value)
{
    auto& search = *reinterpret_cast<MonitorSearch*>(value);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        return TRUE;
    }

    const QString deviceName = QString::fromWCharArray(info.szDevice);
    if (search.requestedName.isEmpty()
        ? (info.dwFlags & MONITORINFOF_PRIMARY) != 0
        : deviceName.compare(search.requestedName, Qt::CaseInsensitive) == 0) {
        search.monitor = monitor;
        return FALSE;
    }
    return TRUE;
}

HMONITOR monitorForName(const QString& name)
{
    MonitorSearch search{name, nullptr};
    EnumDisplayMonitors(nullptr, nullptr, findMonitor, reinterpret_cast<LPARAM>(&search));
    return search.monitor;
}

IDirect3DDevice createDirect3DDevice(ComPtr<ID3D11Device>& d3dDevice, ComPtr<ID3D11DeviceContext>& context)
{
    constexpr D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL selectedLevel{};
    check_hresult(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        levels,
        static_cast<UINT>(std::size(levels)),
        D3D11_SDK_VERSION,
        &d3dDevice,
        &selectedLevel,
        &context));
    Q_UNUSED(selectedLevel);

    ComPtr<ID3D10Multithread> multithread;
    check_hresult(context.As(&multithread));
    multithread->SetMultithreadProtected(TRUE);

    ComPtr<IDXGIDevice> dxgiDevice;
    check_hresult(d3dDevice.As(&dxgiDevice));
    com_ptr<IInspectable> inspectable;
    check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), inspectable.put()));
    return inspectable.as<IDirect3DDevice>();
}

GraphicsCaptureItem createCaptureItem(HMONITOR monitor)
{
    auto interop = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    GraphicsCaptureItem item{nullptr};
    check_hresult(interop->CreateForMonitor(
        monitor, guid_of<GraphicsCaptureItem>(), put_abi(item)));
    return item;
}

IDirect3DSurface copyRegion(
    const Direct3D11CaptureFrame& frame,
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const QRect& region)
{
    auto access = frame.Surface().as<
        ::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    ComPtr<ID3D11Texture2D> source;
    check_hresult(access->GetInterface(IID_PPV_ARGS(&source)));

    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(region.width());
    description.Height = static_cast<UINT>(region.height());
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    ComPtr<ID3D11Texture2D> destination;
    check_hresult(device->CreateTexture2D(&description, nullptr, &destination));

    const D3D11_BOX sourceBox{
        static_cast<UINT>(region.left()),
        static_cast<UINT>(region.top()),
        0,
        static_cast<UINT>(region.left() + region.width()),
        static_cast<UINT>(region.top() + region.height()),
        1,
    };
    context->CopySubresourceRegion(destination.Get(), 0, 0, 0, 0, source.Get(), 0, &sourceBox);

    ComPtr<IDXGISurface> dxgiSurface;
    check_hresult(destination.As(&dxgiSurface));
    com_ptr<IInspectable> inspectable;
    check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgiSurface.Get(), inspectable.put()));
    return inspectable.as<IDirect3DSurface>();
}

QString transcodeFailure(TranscodeFailureReason reason)
{
    switch (reason) {
    case TranscodeFailureReason::CodecNotFound:
        return QStringLiteral("系统中没有可用的 H.264 编码器。");
    case TranscodeFailureReason::InvalidProfile:
        return QStringLiteral("系统不支持当前 H.264 编码参数。");
    default:
        return QStringLiteral("系统无法准备 MP4 编码器。");
    }
}

int automaticBitrate(const QRect& region, int frameRate)
{
    const qint64 estimated = static_cast<qint64>(region.width()) * region.height()
        * frameRate * 12 / 100;
    return static_cast<int>(std::clamp<qint64>(estimated, 1'000'000, 20'000'000));
}

struct CapturedSurface
{
    IDirect3DSurface surface{nullptr};
    TimeSpan timestamp{};
};

class CaptureFrameSource final
{
public:
    CaptureFrameSource(
        const IDirect3DDevice& winrtDevice,
        ID3D11Device* d3dDevice,
        ID3D11DeviceContext* context,
        const GraphicsCaptureItem& item,
        QRect region,
        const std::atomic_bool& stopRequested,
        int maximumDurationMs)
        : d3dDevice_(d3dDevice)
        , context_(context)
        , region_(std::move(region))
        , stopRequested_(stopRequested)
        , maximumDurationMs_(maximumDurationMs)
        , item_(item)
    {
        timer_.start();
        framePool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(
            winrtDevice,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            item.Size());
        frameArrivedToken_ = framePool_.FrameArrived([this](auto const& sender, auto const&) {
            Direct3D11CaptureFrame newest{nullptr};
            while (auto frame = sender.TryGetNextFrame()) {
                newest = std::move(frame);
            }
            if (newest == nullptr) {
                return;
            }
            {
                std::scoped_lock lock(mutex_);
                latestFrame_ = std::move(newest);
                ++capturedFrames_;
            }
            frameAvailable_.notify_one();
        });
        closedToken_ = item_.Closed([this](auto const&, auto const&) {
            {
                std::scoped_lock lock(mutex_);
                closed_ = true;
            }
            frameAvailable_.notify_all();
        });
        session_ = framePool_.CreateCaptureSession(item);
        if (ApiInformation::IsPropertyPresent(
                L"Windows.Graphics.Capture.GraphicsCaptureSession",
                L"IsCursorCaptureEnabled")) {
            session_.IsCursorCaptureEnabled(true);
        }
        session_.StartCapture();
    }

    ~CaptureFrameSource()
    {
        if (framePool_ != nullptr) {
            framePool_.FrameArrived(frameArrivedToken_);
        }
        if (item_ != nullptr) {
            item_.Closed(closedToken_);
        }
        session_.Close();
        framePool_.Close();
    }

    std::optional<CapturedSurface> next()
    {
        Direct3D11CaptureFrame frame{nullptr};
        {
            std::unique_lock lock(mutex_);
            while (latestFrame_ == nullptr && !shouldStopLocked()) {
                frameAvailable_.wait_for(lock, 100ms);
            }
            if (shouldStopLocked()) {
                return std::nullopt;
            }
            frame = std::move(latestFrame_);
            latestFrame_ = nullptr;
        }

        return CapturedSurface{
            copyRegion(frame, d3dDevice_.Get(), context_.Get(), region_),
            frame.SystemRelativeTime(),
        };
    }

    [[nodiscard]] int capturedFrames() const
    {
        std::scoped_lock lock(mutex_);
        return capturedFrames_;
    }

    void requestStop()
    {
        {
            std::scoped_lock lock(mutex_);
            closed_ = true;
        }
        frameAvailable_.notify_all();
    }

private:
    bool shouldStopLocked() const
    {
        return closed_
            || stopRequested_.load(std::memory_order_relaxed)
            || (maximumDurationMs_ > 0 && timer_.elapsed() >= maximumDurationMs_);
    }

    ComPtr<ID3D11Device> d3dDevice_;
    ComPtr<ID3D11DeviceContext> context_;
    QRect region_;
    const std::atomic_bool& stopRequested_;
    int maximumDurationMs_ = 0;
    QElapsedTimer timer_;
    Direct3D11CaptureFramePool framePool_{nullptr};
    GraphicsCaptureSession session_{nullptr};
    GraphicsCaptureItem item_{nullptr};
    event_token frameArrivedToken_{};
    event_token closedToken_{};
    mutable std::mutex mutex_;
    std::condition_variable frameAvailable_;
    Direct3D11CaptureFrame latestFrame_{nullptr};
    int capturedFrames_ = 0;
    bool closed_ = false;
};

RecordingResult record(const RecordingSettings& settings, const std::atomic_bool& stopRequested,
    const std::function<void()>& ready, QString& stage)
{
    RecordingResult result;
    QElapsedTimer elapsed;
    elapsed.start();

    if (settings.outputPath.isEmpty() || settings.sourceRect.isEmpty()) {
        result.error = QStringLiteral("录屏输出路径或选区无效。");
        return result;
    }
    stage = QStringLiteral("检查 Windows Graphics Capture");
    if (!GraphicsCaptureSession::IsSupported()) {
        result.unavailable = true;
        result.error = QStringLiteral("当前 Windows 或显卡驱动不支持 Windows Graphics Capture。");
        return result;
    }

    const HMONITOR monitor = settings.monitorHandle != 0
        ? reinterpret_cast<HMONITOR>(settings.monitorHandle)
        : monitorForName(settings.screenName);
    if (monitor == nullptr) {
        result.error = QStringLiteral("找不到录屏显示器：%1").arg(settings.screenName);
        return result;
    }

    stage = QStringLiteral("创建 D3D11 设备");
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> context;
    const IDirect3DDevice winrtDevice = createDirect3DDevice(d3dDevice, context);
    stage = QStringLiteral("创建显示器捕获目标");
    const GraphicsCaptureItem item = createCaptureItem(monitor);
    const SizeInt32 itemSize = item.Size();

    QRect region = settings.sourceRect.intersected(QRect(0, 0, itemSize.Width, itemSize.Height));
    region.setWidth(region.width() & ~1);
    region.setHeight(region.height() & ~1);
    if (region.width() < 2 || region.height() < 2) {
        result.error = QStringLiteral("录屏选区超出显示器范围或尺寸过小。");
        return result;
    }

    stage = QStringLiteral("启动 GPU 帧池");
    CaptureFrameSource frameSource(
        winrtDevice,
        d3dDevice.Get(),
        context.Get(),
        item,
        region,
        stopRequested,
        settings.maximumDurationMs);

    stage = QStringLiteral("创建媒体流");
    const auto inputProperties = VideoEncodingProperties::CreateUncompressed(
        MediaEncodingSubtypes::Bgra8(),
        static_cast<uint32_t>(region.width()),
        static_cast<uint32_t>(region.height()));
    const VideoStreamDescriptor videoDescriptor(inputProperties);
    const MediaStreamSource mediaSource(videoDescriptor);
    mediaSource.BufferTime(0ms);

    std::atomic_int submittedFrames = 0;
    QString callbackError;
    std::mutex callbackMutex;
    const auto setCallbackError = [&callbackError, &callbackMutex](const hresult_error& error) {
        std::scoped_lock lock(callbackMutex);
        if (callbackError.isEmpty()) {
            callbackError = QString::fromWCharArray(error.message().c_str());
        }
    };
    const event_token startingToken = mediaSource.Starting(
        [&frameSource, &setCallbackError](auto const&, MediaStreamSourceStartingEventArgs const& args) {
            try {
                if (auto frame = frameSource.next()) {
                    args.Request().SetActualStartPosition(frame->timestamp);
                }
            } catch (const hresult_error& error) {
                setCallbackError(error);
                frameSource.requestStop();
            }
        });
    const event_token sampleToken = mediaSource.SampleRequested(
        [&frameSource, &submittedFrames, &setCallbackError](
            auto const&, MediaStreamSourceSampleRequestedEventArgs const& args) {
            try {
                const auto frame = frameSource.next();
                if (!frame) {
                    args.Request().Sample(nullptr);
                    return;
                }
                args.Request().Sample(MediaStreamSample::CreateFromDirect3D11Surface(
                    frame->surface, frame->timestamp));
                ++submittedFrames;
            } catch (const hresult_error& error) {
                setCallbackError(error);
                args.Request().Sample(nullptr);
                frameSource.requestStop();
            }
        });

    const int frameRate = std::clamp(settings.frameRate, 1, 60);
    const int bitrate = settings.bitrate > 0
        ? settings.bitrate
        : automaticBitrate(region, frameRate);
    MediaEncodingProfile profile;
    profile.Container().Subtype(MediaEncodingSubtypes::Mpeg4());
    profile.Video().Subtype(MediaEncodingSubtypes::H264());
    profile.Video().Width(static_cast<uint32_t>(region.width()));
    profile.Video().Height(static_cast<uint32_t>(region.height()));
    profile.Video().Bitrate(static_cast<uint32_t>(bitrate));
    profile.Video().FrameRate().Numerator(static_cast<uint32_t>(frameRate));
    profile.Video().FrameRate().Denominator(1);
    profile.Video().PixelAspectRatio().Numerator(1);
    profile.Video().PixelAspectRatio().Denominator(1);

    stage = QStringLiteral("打开输出文件 %1").arg(QDir::toNativeSeparators(settings.outputPath));
    const std::wstring outputPath = settings.outputPath.toStdWString();
    winrt::Windows::Storage::Streams::IRandomAccessStream stream{nullptr};
    check_hresult(CreateRandomAccessStreamOnFile(
        outputPath.c_str(),
        static_cast<DWORD>(FileAccessMode::ReadWrite),
        guid_of<winrt::Windows::Storage::Streams::IRandomAccessStream>(),
        put_abi(stream)));
    stream.Size(0);

    stage = QStringLiteral("准备 H.264 编码器");
    MediaTranscoder transcoder;
    transcoder.HardwareAccelerationEnabled(true);
    const PrepareTranscodeResult preparation = transcoder
        .PrepareMediaStreamSourceTranscodeAsync(mediaSource, stream, profile)
        .get();
    if (!preparation.CanTranscode()) {
        result.error = transcodeFailure(preparation.FailureReason());
        mediaSource.Starting(startingToken);
        mediaSource.SampleRequested(sampleToken);
        return result;
    }

    if (ready) {
        ready();
    }
    stage = QStringLiteral("写入 MP4");
    preparation.TranscodeAsync().get();
    stream.FlushAsync().get();
    stream.Close();
    mediaSource.Starting(startingToken);
    mediaSource.SampleRequested(sampleToken);

    result.elapsedMs = elapsed.elapsed();
    result.capturedFrames = frameSource.capturedFrames();
    result.submittedFrames = submittedFrames.load(std::memory_order_relaxed);
    result.outputBytes = QFileInfo(settings.outputPath).size();
    result.succeeded = callbackError.isEmpty() && result.outputBytes > 0 && result.submittedFrames > 0;
    if (!result.succeeded) {
        result.error = callbackError.isEmpty()
            ? QStringLiteral("录屏未产生有效视频帧。")
            : QStringLiteral("录屏帧处理失败：%1").arg(callbackError);
    }
    return result;
}

} // namespace

RecordingResult recordScreenNative(
    const RecordingSettings& settings,
    const std::atomic_bool& stopRequested,
    const std::function<void()>& ready)
{
    try {
        ApartmentGuard apartment;
        QString stage = QStringLiteral("初始化 WinRT");
        try {
            return record(settings, stopRequested, ready, stage);
        } catch (const hresult_error& error) {
            RecordingResult result;
            result.unavailable = error.code() == hresult(HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST))
                || error.code() == hresult(E_ACCESSDENIED)
                || error.code() == hresult(DXGI_ERROR_UNSUPPORTED);
            result.error = QStringLiteral("%1失败：%2 (0x%3)")
                .arg(stage)
                .arg(QString::fromWCharArray(error.message().c_str()))
                .arg(static_cast<quint32>(error.code().value), 8, 16, QLatin1Char('0'));
            return result;
        }
    } catch (const hresult_error& error) {
        RecordingResult result;
        result.unavailable = error.code() == hresult(HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST))
            || error.code() == hresult(E_ACCESSDENIED)
            || error.code() == hresult(DXGI_ERROR_UNSUPPORTED);
        result.error = QStringLiteral("Windows 录屏失败：%1 (0x%2)")
            .arg(QString::fromWCharArray(error.message().c_str()))
            .arg(static_cast<quint32>(error.code().value), 8, 16, QLatin1Char('0'));
        return result;
    } catch (const std::exception& error) {
        RecordingResult result;
        result.error = QStringLiteral("录屏失败：%1").arg(QString::fromUtf8(error.what()));
        return result;
    }
}

} // namespace snipnexs
