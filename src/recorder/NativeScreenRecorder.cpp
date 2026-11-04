#include "NativeScreenRecorder.h"
#include "AudioBuffer.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>

#include <Windows.h>
#include <audioclient.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <mmdeviceapi.h>
#include <robuffer.h>
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
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <vector>

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
    com_ptr<::IInspectable> inspectable;
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
    com_ptr<::IInspectable> inspectable;
    check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgiSurface.Get(), inspectable.put()));
    return inspectable.as<IDirect3DSurface>();
}

QString transcodeFailure(TranscodeFailureReason reason)
{
    switch (reason) {
    case TranscodeFailureReason::CodecNotFound:
        return QCoreApplication::translate(
            "NativeScreenRecorder", "系统中没有可用的 H.264 编码器。");
    case TranscodeFailureReason::InvalidProfile:
        return QCoreApplication::translate(
            "NativeScreenRecorder", "系统不支持当前 H.264 编码参数。");
    default:
        return QCoreApplication::translate(
            "NativeScreenRecorder", "系统无法准备 MP4 编码器。");
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

// Compatibility path for desktops where WGC is unavailable. Capture only the
// selected rectangle; Windows capture exclusion still applies to BitBlt.
class DesktopFrameCapture final
{
    struct Resources {
        HDC screen = GetDC(nullptr);
        HDC memory = screen ? CreateCompatibleDC(screen) : nullptr;
        HBITMAP bitmap = nullptr;
        HGDIOBJ previous = nullptr;
        ~Resources() {
            if (previous) { SelectObject(memory, previous); }
            if (bitmap) { DeleteObject(bitmap); }
            if (memory) { DeleteDC(memory); }
            if (screen) { ReleaseDC(nullptr, screen); }
        }
    } resources_;
public:
    DesktopFrameCapture(HMONITOR monitor, QRect region, int frameRate)
        : region_(std::move(region)), interval_(1'000'000 / std::clamp(frameRate, 1, 60))
    {
        MONITORINFO info{sizeof(info)};
        check_bool(GetMonitorInfoW(monitor, &info));
        region_.translate(info.rcMonitor.left, info.rcMonitor.top);
        check_bool(resources_.screen && resources_.memory);
        BITMAPINFO bitmap{};
        bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = region_.width();
        bitmap.bmiHeader.biHeight = -region_.height();
        bitmap.bmiHeader.biPlanes = 1;
        bitmap.bmiHeader.biBitCount = 32;
        bitmap.bmiHeader.biCompression = BI_RGB;
        resources_.bitmap = CreateDIBSection(resources_.screen, &bitmap, DIB_RGB_COLORS, &pixels_, nullptr, 0);
        check_bool(resources_.bitmap && pixels_);
        resources_.previous = SelectObject(resources_.memory, resources_.bitmap);
        check_bool(resources_.previous && resources_.previous != HGDI_ERROR);
        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);
        frequency_ = frequency.QuadPart;
    }

    CapturedSurface next(ID3D11Device* device)
    {
        std::this_thread::sleep_until(nextFrame_);
        nextFrame_ = std::chrono::steady_clock::now() + interval_;
        check_bool(BitBlt(resources_.memory, 0, 0, region_.width(), region_.height(),
            resources_.screen, region_.x(), region_.y(), SRCCOPY | CAPTUREBLT));
        CURSORINFO cursor{sizeof(cursor)};
        if (GetCursorInfo(&cursor) && cursor.flags == CURSOR_SHOWING) {
            ICONINFO icon{};
            if (GetIconInfo(cursor.hCursor, &icon)) {
                DrawIconEx(resources_.memory,
                    cursor.ptScreenPos.x - region_.x() - static_cast<int>(icon.xHotspot),
                    cursor.ptScreenPos.y - region_.y() - static_cast<int>(icon.yHotspot),
                    cursor.hCursor, 0, 0, 0, nullptr, DI_NORMAL);
                if (icon.hbmColor) { DeleteObject(icon.hbmColor); }
                if (icon.hbmMask) { DeleteObject(icon.hbmMask); }
            }
        }
        GdiFlush();
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        D3D11_TEXTURE2D_DESC description{};
        description.Width = static_cast<UINT>(region_.width());
        description.Height = static_cast<UINT>(region_.height());
        description.MipLevels = description.ArraySize = description.SampleDesc.Count = 1;
        description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        D3D11_SUBRESOURCE_DATA data{pixels_, static_cast<UINT>(region_.width() * 4), 0};
        ComPtr<ID3D11Texture2D> texture;
        check_hresult(device->CreateTexture2D(&description, &data, &texture));
        ComPtr<IDXGISurface> surface;
        check_hresult(texture.As(&surface));
        com_ptr<::IInspectable> inspectable;
        check_hresult(CreateDirect3D11SurfaceFromDXGISurface(surface.Get(), inspectable.put()));
        return {inspectable.as<IDirect3DSurface>(), TimeSpan{audioClockHns(now.QuadPart, frequency_)}};
    }
private:
    QRect region_;
    void* pixels_ = nullptr;
    int64_t frequency_ = 1;
    std::chrono::microseconds interval_;
    std::chrono::steady_clock::time_point nextFrame_{};
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
        int maximumDurationMs,
        HMONITOR monitor,
        int frameRate)
        : d3dDevice_(d3dDevice)
        , context_(context)
        , region_(std::move(region))
        , stopRequested_(stopRequested)
        , maximumDurationMs_(maximumDurationMs)
        , item_(item)
    {
        timer_.start();
        if (!item_) {
            desktop_ = std::make_unique<DesktopFrameCapture>(monitor, region_, frameRate);
            return;
        }
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
        if (session_) { session_.Close(); }
        if (framePool_) { framePool_.Close(); }
    }

    std::optional<CapturedSurface> next()
    {
        if (desktop_) {
            std::scoped_lock lock(mutex_);
            if (shouldStopLocked()) { return std::nullopt; }
            auto frame = desktop_->next(d3dDevice_.Get());
            if (shouldStopLocked()) { return std::nullopt; }
            ++capturedFrames_;
            return frame;
        }
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

    bool stopped() const
    {
        std::scoped_lock lock(mutex_);
        return shouldStopLocked();
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
    std::unique_ptr<DesktopFrameCapture> desktop_;
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

// WASAPI shared-mode capture. The loopback endpoint delivers no packets
// while nothing plays, so the polling thread synthesizes silence to keep the
// sample timeline continuous; timestamps are derived from the delivered
// frame count on the QPC timeline; hardware synchronization needs real-device tests.
class AudioCaptureSource final
{
public:
    struct PcmChunk
    {
        std::vector<int16_t> samples;
        int64_t timestampHns = 0;
    };

    AudioCaptureSource(
        RecordingAudio mode,
        const std::atomic_bool& stopRequested,
        std::function<bool()> captureStopped)
        : stopRequested_(stopRequested)
        , captureStopped_(std::move(captureStopped))
    {
        check_hresult(CoCreateInstance(
            __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            IID_PPV_ARGS(&enumerator_)));
        ComPtr<IMMDevice> device;
        check_hresult(enumerator_->GetDefaultAudioEndpoint(
            mode == RecordingAudio::Microphone ? eCapture : eRender,
            eConsole,
            &device));
        check_hresult(device->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
            reinterpret_cast<void**>(audioClient_.GetAddressOf())));

        WAVEFORMATEX* mixFormat = nullptr;
        check_hresult(audioClient_->GetMixFormat(&mixFormat));
        const auto freeFormat = [mixFormat]() {
            CoTaskMemFree(mixFormat);
        };

        const WAVEFORMATEX& format = *mixFormat;
        sampleRate_ = static_cast<int>(format.nSamplesPerSec);
        channels_ = static_cast<int>(format.nChannels);
        bool isFloat = false;
        bool isPcm16 = format.wFormatTag == WAVE_FORMAT_PCM
            && format.wBitsPerSample == 16;
        if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT
            && format.wBitsPerSample == 32) {
            isFloat = true;
        } else if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE
            && format.cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
            const auto* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&format);
            if (extensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
                && format.wBitsPerSample == 32) {
                isFloat = true;
            } else if (extensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM
                && format.wBitsPerSample == 16) {
                isPcm16 = true;
            }
        }
        const bool formatSupported =
            (isFloat || isPcm16) && sampleRate_ > 0 && sampleRate_ <= 384'000
            && channels_ > 0 && channels_ <= 8;
        if (!formatSupported) {
            freeFormat();
            check_hresult(E_INVALIDARG);
        }
        isFloat_ = isFloat;
        const HRESULT initialized = audioClient_->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            mode == RecordingAudio::Microphone
                ? 0
                : AUDCLNT_STREAMFLAGS_LOOPBACK,
            2'000'000, // 200 ms buffer
            0,
            mixFormat,
            nullptr);
        freeFormat();
        check_hresult(initialized);

        check_hresult(audioClient_->GetService(
            IID_PPV_ARGS(&captureClient_)));
        check_hresult(audioClient_->Start());

        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);
        qpcFrequency_ = frequency.QuadPart;
        LARGE_INTEGER start{};
        QueryPerformanceCounter(&start);
        startQpc_ = start.QuadPart;
        startHns_ = audioClockHns(startQpc_, qpcFrequency_);
        chunkSamples_ = std::max(1, sampleRate_ / 50) * channels_; // ~20 ms, whole frames
        pending_.setCapacity(static_cast<std::size_t>(sampleRate_) * channels_ * 2);

        thread_ = std::thread([this, mode] { captureLoop(mode); });
    }

    ~AudioCaptureSource()
    {
        stopping_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
        if (audioClient_ != nullptr) {
            audioClient_->Stop();
        }
    }

    [[nodiscard]] int sampleRate() const { return sampleRate_; }
    [[nodiscard]] int channels() const { return channels_; }

    // A temporary gap is not end-of-stream; wait until capture actually stops.
    [[nodiscard]] PcmChunk next()
    {
        std::unique_lock lock(mutex_);
        sampleAvailable_.wait(lock, [this] {
            return !pending_.empty() || finished_ || !error_.empty();
        });
        if (!error_.empty()) {
            throw hresult_error(E_FAIL, error_);
        }
        if (pending_.empty()) {
            return {};
        }
        PcmChunk chunk{pending_.take(chunkSamples_), 0};
        chunk.timestampHns =
            startHns_ + audioClockHns(deliveredFrames_, sampleRate_);
        deliveredFrames_ += static_cast<int64_t>(chunk.samples.size()) / channels_;
        return chunk;
    }

private:
    void captureLoop(RecordingAudio mode)
    {
        try {
            ApartmentGuard apartment;
            const bool silenceFill = mode != RecordingAudio::Microphone;
            while (!stopping_ && !stopRequested_.load(std::memory_order_relaxed)
                && !captureStopped_()) {
                drainPackets();
                if (silenceFill) {
                    fillSilenceToClock();
                }
                Sleep(8);
            }
        } catch (const hresult_error& error) {
            std::scoped_lock lock(mutex_);
            error_ = error.message();
        } catch (const std::exception&) {
            std::scoped_lock lock(mutex_);
            error_ = L"Audio capture buffer allocation failed.";
        }
        {
            std::scoped_lock lock(mutex_);
            finished_ = true;
        }
        sampleAvailable_.notify_all();
    }

    void drainPackets()
    {
        for (;;) {
            UINT32 packetFrames = 0;
            check_hresult(captureClient_->GetNextPacketSize(&packetFrames));
            if (packetFrames == 0) {
                return;
            }
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            check_hresult(captureClient_->GetBuffer(&data, &frames, &flags, nullptr, nullptr));
            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            std::vector<int16_t> converted(
                static_cast<std::size_t>(frames) * channels_, 0);
            if (!silent && data != nullptr) {
                if (isFloat_) {
                    const auto* floats = reinterpret_cast<const float*>(data);
                    for (std::size_t i = 0; i < converted.size(); ++i) {
                        const float value = std::clamp(floats[i], -1.0f, 1.0f);
                        converted[i] = static_cast<int16_t>(
                            std::lround(value * 32767.0f));
                    }
                } else {
                    std::memcpy(converted.data(), data,
                        converted.size() * sizeof(int16_t));
                }
            }
            captureClient_->ReleaseBuffer(frames);
            append(converted);
        }
    }

    // The loopback endpoint stops delivering packets during silence; pad the
    // timeline so chunks keep flowing for the whole recording.
    void fillSilenceToClock()
    {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const int64_t elapsedHns = audioClockHns(now.QuadPart - startQpc_, qpcFrequency_);
        const int64_t expected =
            elapsedHns * sampleRate_ / 10'000'000 * channels_;
        std::scoped_lock lock(mutex_);
        const int64_t slack = sampleRate_ / 10 * channels_; // 100 ms, whole frames
        if (appendedSamples_ < expected - slack) {
            const int64_t missing = expected - slack - appendedSamples_;
            if (!pending_.appendSilence(static_cast<std::size_t>(missing))) {
                throw hresult_error(E_FAIL, L"Audio encoder is over two seconds behind; recording stopped.");
            }
            appendedSamples_ += missing;
            sampleAvailable_.notify_one();
        }
    }

    void append(const std::vector<int16_t>& samples)
    {
        if (samples.empty()) {
            return;
        }
        std::scoped_lock lock(mutex_);
        if (!pending_.append(samples)) {
            throw hresult_error(E_FAIL, L"Audio encoder is over two seconds behind; recording stopped.");
        }
        appendedSamples_ += static_cast<int64_t>(samples.size());
        sampleAvailable_.notify_one();
    }

    ComPtr<IMMDeviceEnumerator> enumerator_;
    ComPtr<IAudioClient> audioClient_;
    ComPtr<IAudioCaptureClient> captureClient_;
    const std::atomic_bool& stopRequested_;
    std::function<bool()> captureStopped_;
    std::thread thread_;
    std::atomic_bool stopping_ = false;
    std::mutex mutex_;
    std::condition_variable sampleAvailable_;
    AudioBuffer pending_;
    bool finished_ = false;
    hstring error_;
    int64_t appendedSamples_ = 0;
    int64_t deliveredFrames_ = 0;
    int64_t startQpc_ = 0;
    int64_t qpcFrequency_ = 1;
    int64_t startHns_ = 0;
    std::size_t chunkSamples_ = 1;
    int sampleRate_ = 0;
    int channels_ = 0;
    bool isFloat_ = false;
};

RecordingResult record(const RecordingSettings& settings, const std::atomic_bool& stopRequested,
    const std::function<void()>& ready, QString& stage)
{
    RecordingResult result;
    QElapsedTimer elapsed;
    elapsed.start();

    if (settings.outputPath.isEmpty() || settings.sourceRect.isEmpty()) {
        result.error = QCoreApplication::translate(
            "NativeScreenRecorder", "录屏输出路径或选区无效。");
        return result;
    }
    stage = QCoreApplication::translate(
        "NativeScreenRecorder", "检查 Windows Graphics Capture");

    const HMONITOR monitor = settings.monitorHandle != 0
        ? reinterpret_cast<HMONITOR>(settings.monitorHandle)
        : monitorForName(settings.screenName);
    if (monitor == nullptr) {
        result.error = QCoreApplication::translate(
            "NativeScreenRecorder", "找不到录屏显示器：%1").arg(settings.screenName);
        return result;
    }

    stage = QCoreApplication::translate("NativeScreenRecorder", "创建 D3D11 设备");
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> context;
    const IDirect3DDevice winrtDevice = createDirect3DDevice(d3dDevice, context);
    stage = QCoreApplication::translate("NativeScreenRecorder", "创建显示器捕获目标");
    GraphicsCaptureItem item{nullptr};
    if (GraphicsCaptureSession::IsSupported()) {
        try {
            item = createCaptureItem(monitor);
        } catch (const hresult_error& error) {
            if (error.code() != hresult(E_ACCESSDENIED)
                && error.code() != hresult(DXGI_ERROR_UNSUPPORTED)) { throw; }
        }
    }
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    check_bool(GetMonitorInfoW(monitor, &monitorInfo));
    const SizeInt32 itemSize = item ? item.Size() : SizeInt32{
        monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top};
    result.compatibilityCapture = !item;

    QRect region = settings.sourceRect.intersected(QRect(0, 0, itemSize.Width, itemSize.Height));
    region.setWidth(region.width() & ~1);
    region.setHeight(region.height() & ~1);
    if (region.width() < 2 || region.height() < 2) {
        result.error = QCoreApplication::translate(
            "NativeScreenRecorder", "录屏选区超出显示器范围或尺寸过小。");
        return result;
    }

    stage = QCoreApplication::translate("NativeScreenRecorder", "启动 GPU 帧池");
    LARGE_INTEGER clockFrequency{}, clockStart{};
    QueryPerformanceFrequency(&clockFrequency);
    QueryPerformanceCounter(&clockStart);
    const int64_t timelineStartHns = audioClockHns(clockStart.QuadPart, clockFrequency.QuadPart);
    CaptureFrameSource frameSource(
        winrtDevice,
        d3dDevice.Get(),
        context.Get(),
        item,
        region,
        stopRequested,
        settings.maximumDurationMs,
        monitor,
        settings.frameRate);

    stage = QCoreApplication::translate("NativeScreenRecorder", "初始化音频捕获");
    std::optional<AudioCaptureSource> audioSource;
    AudioStreamDescriptor audioDescriptor{nullptr};
    if (settings.audio != RecordingAudio::None) {
        try {
            audioSource.emplace(settings.audio, stopRequested, [&frameSource] { return frameSource.stopped(); });
            audioDescriptor = AudioStreamDescriptor(
                AudioEncodingProperties::CreatePcm(
                    static_cast<uint32_t>(audioSource->sampleRate()),
                    static_cast<uint32_t>(audioSource->channels()),
                    16));
        } catch (const hresult_error& error) {
            // A missing endpoint or unsupported mix format must not kill the
            // whole recording; fall back to the previous video-only pipeline.
            audioSource.reset();
            audioDescriptor = nullptr;
            result.warning = QCoreApplication::translate("NativeScreenRecorder",
                "音频设备不可用，本次仅保存视频：%1").arg(QString::fromWCharArray(error.message().c_str()));
        }
    }

    stage = QCoreApplication::translate("NativeScreenRecorder", "创建媒体流");
    const auto inputProperties = VideoEncodingProperties::CreateUncompressed(
        MediaEncodingSubtypes::Bgra8(),
        static_cast<uint32_t>(region.width()),
        static_cast<uint32_t>(region.height()));
    inputProperties.FrameRate().Numerator(static_cast<uint32_t>(std::clamp(settings.frameRate, 1, 60)));
    inputProperties.FrameRate().Denominator(1);
    inputProperties.PixelAspectRatio().Numerator(1);
    inputProperties.PixelAspectRatio().Denominator(1);
    const VideoStreamDescriptor videoDescriptor(inputProperties);
    const MediaStreamSource mediaSource = audioDescriptor
        ? MediaStreamSource(videoDescriptor, audioDescriptor)
        : MediaStreamSource(videoDescriptor);
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
        [](auto const&, MediaStreamSourceStartingEventArgs const& args) {
            args.Request().SetActualStartPosition(TimeSpan::zero());
        });
    const event_token sampleToken = mediaSource.SampleRequested(
        [&frameSource, &audioSource, &audioDescriptor, &submittedFrames,
         &setCallbackError, timelineStartHns](
            auto const&, MediaStreamSourceSampleRequestedEventArgs const& args) {
            try {
                if (audioDescriptor
                    && args.Request().StreamDescriptor() == audioDescriptor) {
                    auto chunk = audioSource->next();
                    if (chunk.samples.empty()) {
                        args.Request().Sample(nullptr);
                        return;
                    }
                    const auto streamBuffer = pcmBuffer(chunk.samples);
                    MediaStreamSample sample = MediaStreamSample::CreateFromBuffer(
                        streamBuffer, TimeSpan{std::max<int64_t>(0, chunk.timestampHns - timelineStartHns)});
                    sample.Duration(TimeSpan{
                        static_cast<int64_t>(chunk.samples.size())
                        / audioSource->channels() * 10'000'000
                        / audioSource->sampleRate()});
                    args.Request().Sample(sample);
                    return;
                }
                const auto frame = frameSource.next();
                if (!frame) {
                    args.Request().Sample(nullptr);
                    return;
                }
                args.Request().Sample(MediaStreamSample::CreateFromDirect3D11Surface(
                    frame->surface, TimeSpan{std::max<int64_t>(0, frame->timestamp.count() - timelineStartHns)}));
                ++submittedFrames;
            } catch (const hresult_error& error) {
                setCallbackError(error);
                args.Request().Sample(nullptr);
                frameSource.requestStop();
            }
        });

    const auto detachHandlers = qScopeGuard([&] {
        frameSource.requestStop();
        mediaSource.Starting(startingToken);
        mediaSource.SampleRequested(sampleToken);
    });

    const int frameRate = std::clamp(settings.frameRate, 1, 60);
    const int bitrate = settings.bitrate > 0
        ? settings.bitrate
        : automaticBitrate(region, frameRate);
    auto profile = MediaEncodingProfile::CreateMp4(VideoEncodingQuality::HD720p);
    profile.Container().Subtype(MediaEncodingSubtypes::Mpeg4());
    profile.Video().Subtype(MediaEncodingSubtypes::H264());
    profile.Video().Width(static_cast<uint32_t>(region.width()));
    profile.Video().Height(static_cast<uint32_t>(region.height()));
    profile.Video().Bitrate(static_cast<uint32_t>(bitrate));
    profile.Video().FrameRate().Numerator(static_cast<uint32_t>(frameRate));
    profile.Video().FrameRate().Denominator(1);
    profile.Video().PixelAspectRatio().Numerator(1);
    profile.Video().PixelAspectRatio().Denominator(1);
    if (audioDescriptor) {
        profile.Audio(AudioEncodingProperties::CreateAac(
            static_cast<uint32_t>(audioSource->sampleRate()),
            static_cast<uint32_t>(audioSource->channels()),
            static_cast<uint32_t>(std::max(24'000, settings.audioBitrate))));
    } else {
        profile.Audio(nullptr);
    }

    stage = QCoreApplication::translate(
        "NativeScreenRecorder", "打开输出文件 %1")
                .arg(QDir::toNativeSeparators(settings.outputPath));
    const std::wstring outputPath = settings.outputPath.toStdWString();
    winrt::Windows::Storage::Streams::IRandomAccessStream stream{nullptr};
    check_hresult(CreateRandomAccessStreamOnFile(
        outputPath.c_str(),
        static_cast<DWORD>(FileAccessMode::ReadWrite),
        guid_of<winrt::Windows::Storage::Streams::IRandomAccessStream>(),
        put_abi(stream)));
    stream.Size(0);

    stage = QCoreApplication::translate("NativeScreenRecorder", "准备 H.264 编码器");
    MediaTranscoder transcoder;
    transcoder.HardwareAccelerationEnabled(!result.compatibilityCapture);
    PrepareTranscodeResult preparation{nullptr};
    try {
        preparation = transcoder.PrepareMediaStreamSourceTranscodeAsync(mediaSource, stream, profile).get();
    } catch (const hresult_error&) {
        if (result.compatibilityCapture) { throw; }
        // Some drivers expose a hardware encoder but reject the BGRA input.
        // Retry only preparation, before any output is committed.
        transcoder.HardwareAccelerationEnabled(false);
        preparation = transcoder.PrepareMediaStreamSourceTranscodeAsync(mediaSource, stream, profile).get();
    }
    if (!preparation.CanTranscode() && transcoder.HardwareAccelerationEnabled()) {
        transcoder.HardwareAccelerationEnabled(false);
        preparation = transcoder.PrepareMediaStreamSourceTranscodeAsync(mediaSource, stream, profile).get();
    }
    if (!preparation.CanTranscode()) {
        result.error = transcodeFailure(preparation.FailureReason());
        return result;
    }

    if (ready) {
        ready();
    }
    stage = QCoreApplication::translate("NativeScreenRecorder", "写入 MP4");
    preparation.TranscodeAsync().get();
    stream.FlushAsync().get();
    stream.Close();

    result.elapsedMs = elapsed.elapsed();
    result.capturedFrames = frameSource.capturedFrames();
    result.submittedFrames = submittedFrames.load(std::memory_order_relaxed);
    result.audioCaptured = audioDescriptor != nullptr;
    result.outputBytes = QFileInfo(settings.outputPath).size();
    result.succeeded = callbackError.isEmpty() && result.outputBytes > 0 && result.submittedFrames > 0;
    if (!result.succeeded) {
        result.error = callbackError.isEmpty()
            ? QCoreApplication::translate(
                  "NativeScreenRecorder", "录屏未产生有效视频帧。")
            : QCoreApplication::translate(
                  "NativeScreenRecorder", "录屏帧处理失败：%1").arg(callbackError);
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
        QString stage = QCoreApplication::translate("NativeScreenRecorder", "初始化 WinRT");
        try {
            return record(settings, stopRequested, ready, stage);
        } catch (const hresult_error& error) {
            RecordingResult result;
            result.unavailable = error.code() == hresult(HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST))
                || error.code() == hresult(E_ACCESSDENIED)
                || error.code() == hresult(DXGI_ERROR_UNSUPPORTED);
            result.error = QCoreApplication::translate(
                "NativeScreenRecorder", "%1失败：%2 (0x%3)")
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
        result.error = QCoreApplication::translate(
            "NativeScreenRecorder", "Windows 录屏失败：%1 (0x%2)")
            .arg(QString::fromWCharArray(error.message().c_str()))
            .arg(static_cast<quint32>(error.code().value), 8, 16, QLatin1Char('0'));
        return result;
    } catch (const std::exception& error) {
        RecordingResult result;
        result.error = QCoreApplication::translate(
            "NativeScreenRecorder", "录屏失败：%1").arg(QString::fromUtf8(error.what()));
        return result;
    }
}

} // namespace snipnexs
