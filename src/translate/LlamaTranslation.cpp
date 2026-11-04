#include "LlamaTranslation.h"
#include "TranslationTextSplitter.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QObject>
#include <QString>

#include <llama.h>

#include <algorithm>
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

// Use half the logical processors, capped at 8. This leaves headroom for the
// UI on large CPUs without oversubscribing small ones.
int32_t generationThreads()
{
    static const int32_t threads = [] {
        const unsigned hardware = std::thread::hardware_concurrency();
        if (hardware <= 2u) {
            return 1;
        }
        return static_cast<int32_t>(std::clamp(hardware / 2u, 1u, 8u));
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

Session* openSession(const QString& modelFilePath, QString* error, const std::atomic_bool* canceled)
{
    // Restrict plugin discovery to the shipped DLL directory, not the caller's CWD.
    if (!ggml_backend_reg_count()) {
        const QByteArray backendDirectory = QCoreApplication::applicationDirPath().toUtf8();
        ggml_backend_load_all_from_path(backendDirectory.constData());
    }
    if (!ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU)) {
        if (error) { *error = QObject::tr("未找到兼容的 CPU 翻译后端，请重新解压完整安装包。"); }
        return nullptr;
    }
    llama_backend_init();

    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = 0; // CPU-only by design
    modelParams.progress_callback = [](float, void* data) {
        const auto* flag = static_cast<const std::atomic_bool*>(data);
        return !flag || !flag->load();
    };
    modelParams.progress_callback_user_data = const_cast<std::atomic_bool*>(canceled);
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

constexpr int32_t kMaxNewTokens = 1024;

std::vector<llama_token> promptTokens(Session& session, const QString& userMessage)
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
    if (tokenCount <= 0) { return {}; }
    tokens.resize(static_cast<std::size_t>(tokenCount));
    return tokens;
}

QString generateOnce(
    Session& session, std::vector<llama_token>& tokens,
    QString* error, const std::atomic_bool* canceled)
{
    if (llama_memory_t memory = llama_get_memory(session.context)) {
        llama_memory_clear(memory, true);
    }
    llama_sampler_reset(session.sampler);
    // The callback is valid only for this call; sessions may outlive a request.
    struct AbortGuard {
        llama_context* context;
        ~AbortGuard() { llama_set_abort_callback(context, nullptr, nullptr); }
    } guard{session.context};
    llama_set_abort_callback(session.context, [](void* data) {
        const auto* flag = static_cast<const std::atomic_bool*>(data);
        return flag && flag->load();
    }, const_cast<std::atomic_bool*>(canceled));
    const int32_t tokenCount = static_cast<int32_t>(tokens.size());
    std::string output;
    auto decode = [&](llama_token* batchTokens, int32_t count) -> bool {
        if (canceled && canceled->load()) { return false; }
        llama_batch batch = llama_batch_get_one(batchTokens, count);
        return llama_decode(session.context, batch) == 0;
    };

    const int32_t batchSize = static_cast<int32_t>(llama_n_batch(session.context));
    for (int32_t offset = 0; offset < tokenCount; offset += batchSize) {
        if (!decode(tokens.data() + offset, std::min(batchSize, tokenCount - offset))) {
            if (error) { *error = QObject::tr("翻译已取消或模型推理失败。"); }
            return {};
        }
    }

    bool finished = false;
    for (int32_t generated = 0; generated < kMaxNewTokens; ++generated) {
        if (canceled && canceled->load()) {
            if (error) { *error = QObject::tr("翻译已取消。"); }
            return {};
        }
        llama_token next = llama_sampler_sample(session.sampler, session.context, -1);
        llama_sampler_accept(session.sampler, next);
        if (llama_vocab_is_eog(session.vocab, next)) {
            finished = true;
            break;
        }

        std::vector<char> piece(64);
        int32_t pieceLength = llama_token_to_piece(
            session.vocab, next, piece.data(), static_cast<int32_t>(piece.size()), 0, false);
        if (pieceLength < 0) {
            piece.resize(static_cast<std::size_t>(-pieceLength));
            pieceLength = llama_token_to_piece(
                session.vocab, next, piece.data(), static_cast<int32_t>(piece.size()), 0, false);
        }
        if (pieceLength > 0) {
            output.append(piece.data(), static_cast<std::size_t>(pieceLength));
        }

        if (!decode(&next, 1)) {
            if (error) {
                *error = QObject::tr("翻译失败：模型推理出错。");
            }
            return QString();
        }
    }

    if (!finished) {
        if (error) { *error = QObject::tr("译文超过输出长度限制，请缩短输入后重试。"); }
        return {};
    }
    return QString::fromUtf8(
        output.c_str(), static_cast<qsizetype>(output.size())).trimmed();
}

} // namespace

namespace {

bool sourceLooksChinese(const QString& text)
{
    for (const QChar& character : text) {
        const char16_t code = character.unicode();
        if ((code >= 0x4E00 && code <= 0x9FFF)
            || (code >= 0x3400 && code <= 0x4DBF)) {
            return true;
        }
    }
    return false;
}

QString cleanTranslation(QString translated)
{
    // The model echoes the backtick delimiters used in the prompt.
    if (translated.size() >= 2
        && translated.startsWith(u'`') && translated.endsWith(u'`')) {
        translated = translated.mid(1, translated.size() - 2).trimmed();
    }
    return translated;
}

} // namespace

QString translateSegments(
    Session& session,
    const QStringList& segments,
    const QString& targetLanguage,
    QString* error, const std::atomic_bool* canceled)
{
    if (error) { error->clear(); }
    QString output;
    QStringList pending = segments;
    for (qsizetype index = 0; index < pending.size(); ++index) {
        if (canceled && canceled->load()) {
            if (error) { *error = QObject::tr("翻译已取消。"); }
            return {};
        }
        const QString segment = pending.at(index);
        if (segment.trimmed().isEmpty()) {
            output += segment;
            continue;
        }
        auto tokens = promptTokens(session, promptFor(segment, sourceLooksChinese(segment), targetLanguage));
        const auto promptBudget = llama_n_ctx(session.context) - kMaxNewTokens;
        if (tokens.empty()) {
            if (error) { *error = QObject::tr("分词失败：无法对输入文本编码。"); }
            return {};
        }
        if (tokens.size() > promptBudget) {
            qsizetype split = segment.size() / 2;
            if (split > 0 && segment.at(split).isLowSurrogate()) { --split; }
            if (split <= 0) {
                if (error) { *error = QObject::tr("输入超过模型上下文限制。"); }
                return {};
            }
            pending[index] = segment.left(split);
            pending.insert(index + 1, segment.mid(split));
            --index;
            continue;
        }
        QString segmentError;
        const QString translated = cleanTranslation(generateOnce(session, tokens, &segmentError, canceled));
        if (segmentError.isEmpty() && translated.isEmpty()) {
            segmentError = QObject::tr("模型未返回译文，请调整输入后重试。");
        }
        if (!segmentError.isEmpty()) {
            if (error) {
                *error = segmentError;
            }
            return QString();
        }
        output += translated;
        output += QLatin1Char(' ');
    }
    return output.trimmed();
}

QStringList translateLines(
    Session& session,
    const QStringList& lines,
    const QString& targetLanguage,
    QString* error, const std::atomic_bool* canceled)
{
    if (error) { error->clear(); }
    // Long OCR lines are split to the same segment budget as whole-text
    // translation; the sub-results are re-joined into the line's entry.
    constexpr int kSegmentCharacters = 600;
    QStringList results;
    results.reserve(lines.size());
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            results.append(line);
            continue;
        }
        QString lineError;
        const QString translated = translateSegments(
            session,
            snipnexs::splitTranslationSegments(line, kSegmentCharacters),
            targetLanguage,
            &lineError, canceled);
        if (!lineError.isEmpty()) {
            if (error) {
                *error = lineError;
            }
            return {};
        }
        results.append(translated);
    }
    return results;
}

} // namespace local_llama
