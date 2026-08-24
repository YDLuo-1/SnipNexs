#pragma once

class QWidget;

namespace snipnexs {

[[nodiscard]] bool setWindowExcludedFromCapture(QWidget& window, bool excluded);

} // namespace snipnexs
