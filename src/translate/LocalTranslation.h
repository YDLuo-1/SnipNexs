#pragma once

#include <QStringList>

class QString;

namespace local_translation {

struct Session;

// Opens a CTranslate2 CPU session for the converted model package in
// modelDirectory. Returns null and fills error on failure. The caller owns
// the session and must release it with closeSession().
[[nodiscard]] Session* openSession(const QString& modelDirectory, QString* error);
void closeSession(Session* session);

// Translates already-split segments and joins the results in order.
// Whitespace-only segments are passed through untranslated.
[[nodiscard]] QString translateSegments(
    Session& session, const QStringList& segments, QString* error);

// Translates each line independently; the result list always has the same
// size as the input so callers can align lines one to one. Whitespace-only
// lines (and lines the tokenizer cannot encode) are passed through
// untranslated in place.
[[nodiscard]] QStringList translateLines(
    Session& session, const QStringList& lines, QString* error);

} // namespace local_translation
