#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace snipnexs {

// Compares dotted release versions ("v0.8.1" vs "0.9.0"). The candidate is
// validated as a whole first: any non-integer part (e.g. a pre-release
// suffix like "0.9.0-beta1") reports no update, the conservative choice.
[[nodiscard]] inline bool isNewerReleaseVersion(
    const QString& currentVersion, const QString& candidateTag)
{
    const auto normalized = [](const QString& value) {
        QString text = value.trimmed();
        if (text.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
            text.remove(0, 1);
        }
        return text;
    };
    const QString current = normalized(currentVersion);
    const QString candidate = normalized(candidateTag);
    if (candidate.isEmpty()) {
        return false;
    }

    const QStringList currentParts = current.split(QLatin1Char('.'));
    const QStringList candidateParts = candidate.split(QLatin1Char('.'));
    if (candidateParts.size() < 2) {
        return false;
    }

    QList<int> currentValues;
    bool currentOk = true;
    for (const QString& part : currentParts) {
        bool ok = false;
        const int value = part.toInt(&ok);
        currentValues.append(value);
        currentOk &= ok;
    }
    QList<int> candidateValues;
    for (const QString& part : candidateParts) {
        bool ok = false;
        const int value = part.toInt(&ok);
        if (!ok) {
            return false;
        }
        candidateValues.append(value);
    }
    if (!currentOk) {
        return true; // malformed local version: prefer offering the update
    }

    const int count = qMax(currentValues.size(), candidateValues.size());
    for (int index = 0; index < count; ++index) {
        const int currentValue =
            index < currentValues.size() ? currentValues.at(index) : 0;
        const int candidateValue =
            index < candidateValues.size() ? candidateValues.at(index) : 0;
        if (candidateValue != currentValue) {
            return candidateValue > currentValue;
        }
    }
    return false;
}

} // namespace snipnexs
