#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QString>

namespace snipnexs {

class GlobalHotkey final : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject* parent = nullptr);
    ~GlobalHotkey() override;

    [[nodiscard]] bool registerCaptureShortcut();
    [[nodiscard]] QString shortcutText() const;
    [[nodiscard]] bool isUsingFallback() const;
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void activated();

private:
    enum class Shortcut
    {
        None,
        F1,
        CtrlShiftA
    };

    static constexpr int kHotkeyId = 0x534E;
    bool registered_ = false;
    Shortcut shortcut_ = Shortcut::None;
};

} // namespace snipnexs
