#pragma once

#include <QStringList>

class QString;

namespace local_llama {

struct Session;

// Opens a llama.cpp CPU session for the GGUF translation model at
// modelFilePath. Returns null and fills error on failure. The caller owns
// the session and must release it with closeSession().
[[nodiscard]] Session* openSession(const QString& modelFilePath, QString* error);
void closeSession(Session* session);

// Translates already-split segments for the requested target language
// ("zh" or "en") and joins the results in order. Whitespace-only segments
// are passed through untranslated.
[[nodiscard]] QString translateSegments(
    Session& session,
    const QStringList& segments,
    const QString& targetLanguage,
    QString* error);

// Translates each line independently; the result list always has the same
// size as the input so callers can align lines one to one. Whitespace-only
// lines pass through untranslated; lines longer than the segment limit are
// split internally and their translations re-joined into one entry.
[[nodiscard]] QStringList translateLines(
    Session& session,
    const QStringList& lines,
    const QString& targetLanguage,
    QString* error);

} // namespace local_llama
