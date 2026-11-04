#pragma once

#include <winrt/Windows.Storage.Streams.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <span>
#include <vector>

namespace snipnexs {

inline int64_t audioClockHns(int64_t ticks, int64_t frequency)
{
    return (ticks / frequency) * 10'000'000
        + (ticks % frequency) * 10'000'000 / frequency;
}

inline winrt::Windows::Storage::Streams::IBuffer pcmBuffer(std::span<const int16_t> samples)
{
    winrt::Windows::Storage::Streams::DataWriter writer;
    const auto* bytes = reinterpret_cast<const uint8_t*>(samples.data());
    writer.WriteBytes(winrt::array_view<const uint8_t>(bytes, bytes + samples.size_bytes()));
    return writer.DetachBuffer();
}

// Called under the audio source's mutex. Reject overflow before allocating.
class AudioBuffer final
{
public:
    void setCapacity(std::size_t samples) { capacity_ = samples; }
    bool append(std::span<const int16_t> samples)
    {
        if (samples.size() > capacity_ - pending_.size()) {
            return false;
        }
        pending_.insert(pending_.end(), samples.begin(), samples.end());
        return true;
    }
    bool appendSilence(std::size_t count)
    {
        if (count > capacity_ - pending_.size()) {
            return false;
        }
        pending_.insert(pending_.end(), count, 0);
        return true;
    }
    std::vector<int16_t> take(std::size_t maximum)
    {
        const auto count = std::min(maximum, pending_.size());
        std::vector<int16_t> result;
        result.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            result.push_back(pending_.front());
            pending_.pop_front();
        }
        return result;
    }
    bool empty() const { return pending_.empty(); }
private:
    std::deque<int16_t> pending_;
    std::size_t capacity_ = 0;
};

} // namespace snipnexs
