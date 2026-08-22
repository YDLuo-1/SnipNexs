#pragma once

#include "RecordingSettings.h"

#include <atomic>
#include <functional>

namespace snipnexs {

[[nodiscard]] RecordingResult recordScreenNative(
    const RecordingSettings& settings,
    const std::atomic_bool& stopRequested,
    const std::function<void()>& ready = {});

} // namespace snipnexs
