#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

namespace snipnexs {

class GlobalHotkey final : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject* parent = nullptr);
    ~GlobalHotkey() override;

    [[nodiscard]] bool registerCaptureShortcut();
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void activated();

private:
    static constexpr int kHotkeyId = 0x534E;
    bool registered_ = false;
};

} // namespace snipnexs
