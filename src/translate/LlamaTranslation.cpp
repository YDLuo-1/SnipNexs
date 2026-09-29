#include "LlamaTranslation.h"

#include <QFileInfo>
#include <QObject>
#include <QString>

#include <llama.h>

#include <string>
#include <thread>
#include <vector>

namespace local_llama {

namespace {

// Hy-MT2 default-translation instruction (model card examples). The
// instruction language follows the source text language; the target uses the
// full language name. The source text is delimited by backticks.
QString promptFor(const QString& segment, bool sourceIsChinese, const QString& targetLanguage)
{
    if (sourceIsChinese) {
        const QString target = targetLanguage == QStringLiteral("en")
            ? QStringLiteral("英语")
            : QStringLiteral("中文");
        return QStringLiteral(
            "将以下文本翻译为 `%1`，注意只需要输出翻译后的结果，不要额外解释：\n\n`%2`")
            .arg(target, segment);
    }
    const QString target = targetLanguage == QStringLiteral("zh")
        ? QStringLiteral("Chinese")
        : QStringLiteral("English");
    return QStringLiteral(
        "Translate the following text into `%1`. Note that you should only "
        "output the translated result without any additional explanation:\n\n`%2`")
        .arg(target, segment);
}

// Two sampling threads leave headroom for the UI while roughly doubling
// generation speed over a single thread.
int32_t generationThreads()
{
    static const int32_t threads = [] {
        const unsigned hardware = std::thread::hardware_concurrency();
        if (hardware <= 2u) {
            return 1;
        }
        return hardware / 2u > 6u ? 4 : static_cast<int32_t>(hardware / 2u);
    }();
    return threads;
}

std::string utf8Path(const QString& path)
{
    const QByteArray bytes = path.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

} // namespace

struct Session
{
    llama_model* model = nullptr;
    llama_context* context = nullptr;
    llama_sampler* sampler = nullptr;
    const llama_vocab* vocab = nullptr;
};

Session* openSession(const QString& modelFilePath, QString* error)
{
    llama_backend_init();

    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = 0; // CPU-only by design
    auto session = std::make_unique<Session>();
    session->model = llama_model_load_from_file(
        utf8Path(modelFilePath).c_str(), modelParams);
    if (session->model == nullptr) {
        if (error) {
            *error = QObject::tr("加载翻译模型失败：%1")
                .arg(QFileInfo(modelFilePath).fileName());
        }
        llama_backend_free();
        return nullptr;
    }

    llama_context_params contextParams = llama_context_default_params();
    contextParams.n_ctx = 2048;
    contextParams.n_batch = 512;
    contextParams.n_threads = generationThreads();
    contextParams.n_threads_batch = generationThreads();
    session->context = llama_init_from_model(session->model, contextParams);
    if (session->context == nullptr) {
        if (error) {
            *error = QObject::tr("创建翻译上下文失败。");
        }
        llama_model_free(session->model);
        llama_backend_free();
        return nullptr;
    }

    session->vocab = llama_model_get_vocab(session->model);
    llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
    session->sampler = llama_sampler_chain_init(samplerParams);
    llama_sampler_chain_add(session->sampler, llama_sampler_init_greedy());
    return session.release();
}

void closeSession(Session* session)
{
    if (session == nullptr) {
        return;
    }
    if (session->sampler != nullptr) {
        llama_sampler_free(session->sampler);
    }
    if (session->context != nullptr) {
        llama_free(session->context);
    }
    if (session->model != nullptr) {
        llama_model_free(session->model);
    }
    delete session;
    llama_backend_free();
}

namespace {

QString generateOnce(
    Session& session,
    const QString& userMessage,
    QString* error)
{
    // Rendered manually after llama.cpp's automatic chat-template detection
    // misfired for this model: the official Hy-MT2 dense format is
    // <｜hy_begin▁of▁sentence｜><｜hy_User｜>{content}<｜hy_Assistant｜>
    // (BOS marker embedded in the prompt; eos is <｜hy_place▁holder▁no▁2｜>).
    // parse_special=true makes the tokenizer map these markers to the real
    // special-token ids.
    const std::string promptBytes = QStringLiteral(
        "<\uFF5Chy_begin\u2581of\u2581sentence\uFF5C><\uFF5Chy_User\uFF5C>%1<\uFF5Chy_Assistant\uFF5C>")
        .arg(userMessage).toStdString();
    const int32_t promptLength = static_cast<int32_t>(promptBytes.size());

    const int32_t estimatedTokens = promptLength / 2 + 16;
    std::vector<llama_token> tokens(static_cast<std::size_t>(estimatedTokens));
    // The rendered chat template already contains the BOS marker.
    int32_t tokenCount = llama_tokenize(
        session.vocab,
        promptBytes.c_str(),
        promptLength,
        tokens.data(),
        estimatedTokens,
        false,
        true);
    if (tokenCount < 0) {
        tokens.resize(static_cast<std::size_t>(-tokenCount));
        tokenCount = llama_tokenize(
            session.vocab,
            promptBytes.c_str(),
            promptLength,
            tokens.data(),
            -tokenCount,
            false,
            true);
    }
    if (tokenCount <= 0) {
        if (error) {
            *error = QObject::tr("分词失败：无法对输入文本编码。");
        }
        return QString();
    }

    std::string output;
    auto decode = [&](llama_token* batchTokens, int32_t count) -> bool {
        llama_batch batch = llama_batch_get_one(batchTokens, count);
        return llama_decode(session.context, batch) == 0;
    };

    if (!decode(tokens.data(), tokenCount)) {
        if (error) {
            *error = QObject::tr("翻译失败：模型推理出错。");
        }
        return QString();
    }

    constexpr int32_t kMaxNewTokens = 1024;
    for (int32_t generated = 0; generated < kMaxNewTokens; ++generated) {
        llama_token next = llama_sampler_sample(session.sampler, session.context, -1);
        llama_sampler_accept(session.sampler, next);
        if (llama_vocab_is_eog(session.vocab, next)) {
            break;
        }

        char pieceBuffer[64];
        const int32_t pieceLength = llama_token_to_piece(
            session.vocab,
            next,
            pieceBuffer,
            sizeof(pieceBuffer),
            0,
            false);
        if (pieceLength > 0) {
            output.append(pieceBuffer, static_cast<std::size_t>(pieceLength));
        }

        if (!decode(&next, 1)) {
            if (error) {
                *error = QObject::tr("翻译失败：模型推理出错。");
            }
            return QString();
        }
    }

    return QString::fromUtf8(
        output.c_str(), static_cast<qsizetype>(output.size())).trimmed();
}

} // namespace

QString translateSegments(
    Session& session,
    const QStringList& segments,
    const QString& targetLanguage,
    QString* error)
{
    QString output;
    for (const QString& segment : segments) {
        if (segment.trimmed().isEmpty()) {
            output += segment;
            continue;
        }
        const bool sourceIsChinese = [](const QString& text) {
            for (const QChar& character : text) {
                const char16_t code = character.unicode();
                if ((code >= 0x4E00 && code <= 0x9FFF)
                    || (code >= 0x3400 && code <= 0x4DBF)) {
                    return true;
                }
            }
            return false;
        }(segment);
        QString segmentError;
        const QString translated = generateOnce(
            session,
            promptFor(segment, sourceIsChinese, targetLanguage),
            &segmentError);
        if (!segmentError.isEmpty()) {
            if (error) {
                *error = segmentError;
            }
            return QString();
        }
        QString cleaned = translated;
        // The model echoes the backtick delimiters used in the prompt.
        if (cleaned.size() >= 2 && cleaned.startsWith(u'`') && cleaned.endsWith(u'`')) {
            cleaned = cleaned.mid(1, cleaned.size() - 2).trimmed();
        }
        output += cleaned.isEmpty() ? segment.trimmed() : cleaned;
        output += QLatin1Char(' ');
    }
    return output.trimmed();
}

} // namespace local_llama
