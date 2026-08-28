#include "LocalTranslation.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <ctranslate2/translator.h>
#include <sentencepiece_processor.h>

#include <QObject>

#include <exception>
#include <memory>
#include <thread>
#include <vector>

namespace local_translation {

namespace {

// Leave cores for the UI thread; the MT model is small enough that four
// intra-op threads are plenty for OCR-sized text.
std::size_t intraThreads()
{
    static const std::size_t threads = [] {
        const unsigned hardware = std::thread::hardware_concurrency();
        if (hardware <= 2u) {
            return std::size_t{1};
        }
        const std::size_t halved = hardware / 2u;
        return halved > 4u ? std::size_t{4} : (halved < 1u ? std::size_t{1} : halved);
    }();
    return threads;
}

bool readFileBytes(const QString& path, std::string& contents)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray bytes = file.readAll();
    contents.assign(bytes.constData(), static_cast<std::size_t>(bytes.size()));
    return true;
}

} // namespace

struct Session
{
    std::unique_ptr<ctranslate2::Translator> translator;
    std::unique_ptr<sentencepiece::SentencePieceProcessor> sourceTokenizer;
    std::unique_ptr<sentencepiece::SentencePieceProcessor> targetTokenizer;
};

Session* openSession(const QString& modelDirectory, QString* error)
{
    const QDir directory(modelDirectory);
    const QStringList requiredFiles = {
        QStringLiteral("model.bin"),
        QStringLiteral("source.spm"),
        QStringLiteral("target.spm"),
    };
    for (const QString& fileName : requiredFiles) {
        if (!QFileInfo::exists(directory.filePath(fileName))) {
            if (error) {
                *error = QObject::tr("模型文件缺失：%1").arg(fileName);
            }
            return nullptr;
        }
    }

    auto session = std::make_unique<Session>();
    try {
        ctranslate2::ReplicaPoolConfig config;
        config.num_threads_per_replica = intraThreads();
        session->translator = std::make_unique<ctranslate2::Translator>(
            modelDirectory.toStdString(),
            ctranslate2::Device::CPU,
            ctranslate2::ComputeType::DEFAULT,
            std::vector<int>{0},
            false,
            config);
    } catch (const std::exception& exception) {
        if (error) {
            *error = QObject::tr("加载翻译模型失败：%1")
                .arg(QString::fromStdString(exception.what()));
        }
        return nullptr;
    }

    // The tokenizer models are loaded from serialized protos so that install
    // paths outside the ASCII range (e.g. a Chinese Windows user name) never
    // go through std::ifstream path handling.
    for (bool target : {false, true}) {
        std::string serialized;
        if (!readFileBytes(directory.filePath(target ? QStringLiteral("target.spm")
                                                     : QStringLiteral("source.spm")),
                           serialized)) {
            if (error) {
                *error = QObject::tr("无法读取分词模型：%1")
                    .arg(target ? QStringLiteral("target.spm") : QStringLiteral("source.spm"));
            }
            return nullptr;
        }
        auto processor = std::make_unique<sentencepiece::SentencePieceProcessor>();
        const auto status = processor->LoadFromSerializedProto(serialized);
        if (!status.ok()) {
            if (error) {
                *error = QObject::tr("解析分词模型失败：%1")
                    .arg(QString::fromStdString(std::string(status.message())));
            }
            return nullptr;
        }
        if (target) {
            session->targetTokenizer = std::move(processor);
        } else {
            session->sourceTokenizer = std::move(processor);
        }
    }

    return session.release();
}

void closeSession(Session* session)
{
    delete session;
}

QString translateSegments(
    Session& session, const QStringList& segments, QString* error)
{
    std::vector<std::vector<std::string>> batch;
    std::vector<int> batchSegmentIndexes;
    batch.reserve(static_cast<std::size_t>(segments.size()));
    batchSegmentIndexes.reserve(static_cast<std::size_t>(segments.size()));

    for (int index = 0; index < segments.size(); ++index) {
        const QString& segment = segments.at(index);
        if (segment.trimmed().isEmpty()) {
            continue;
        }
        std::vector<std::string> pieces;
        if (!session.sourceTokenizer->Encode(segment.toStdString(), &pieces).ok()
            || pieces.empty()) {
            continue;
        }
        batch.push_back(std::move(pieces));
        batchSegmentIndexes.push_back(index);
    }

    QString output;
    output.reserve(segments.isEmpty() ? 0 : segments.front().size() * 2);
    if (batch.empty()) {
        return output;
    }

    try {
        ctranslate2::TranslationOptions options;
        options.beam_size = 2;
        options.max_input_length = 512;
        options.max_decoding_length = 512;
        const std::vector<ctranslate2::TranslationResult> results =
            session.translator->translate_batch(batch, options);

        int cursor = 0;
        for (int index = 0; index < segments.size(); ++index) {
            if (cursor < static_cast<int>(batchSegmentIndexes.size())
                && batchSegmentIndexes[static_cast<std::size_t>(cursor)] == index) {
                const auto& hypothesis = results
                    .at(static_cast<std::size_t>(cursor))
                    .hypotheses.at(0);
                std::string translated;
                if (!session.targetTokenizer
                        ->Decode(hypothesis, &translated)
                        .ok()) {
                    translated.clear();
                }
                output += QString::fromStdString(translated);
                ++cursor;
            } else {
                output += segments.at(index);
            }
        }
    } catch (const std::exception& exception) {
        if (error) {
            *error = QObject::tr("翻译失败：%1")
                .arg(QString::fromStdString(exception.what()));
        }
        return QString();
    }

    return output;
}

} // namespace local_translation
