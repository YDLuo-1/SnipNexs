#pragma once

#include "RecordingSettings.h"

namespace snipnexs {

// Consumes the pending path. A nonempty failed recording remains recoverable.
QString finishRecordingOutput(
    const RecordingResult& result, QString& pendingPath, const QString& finalPath);

} // namespace snipnexs
