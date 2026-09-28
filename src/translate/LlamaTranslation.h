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

} // namespace local_llama
