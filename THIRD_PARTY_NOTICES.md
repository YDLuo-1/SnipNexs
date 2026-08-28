# Third-party notices

## Qt 6.11.2

SnipNexs dynamically links the Qt Core, Gui, Widgets, and Network modules and deploys the required Qt platform, style, and image-format plugins.

- Project: <https://www.qt.io/>
- Corresponding source copy for v0.7.0: <https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz>
- Source archive SHA-256: `5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`
- License used by this distribution: GNU Lesser General Public License version 3
- License text: [licenses/LGPL-3.0-only.txt](licenses/LGPL-3.0-only.txt)
- Human-readable bundled-component notice: [licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md](licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md)
- Machine-readable component and attribution inventory: `licenses/qtbase-6.11.2.spdx`

The Qt DLLs are separate shared libraries. SnipNexs does not modify Qt and does not use Qt WebEngine, QML, or statically linked Qt libraries. To use a modified Qt build, exit SnipNexs, replace the Qt DLLs in `bin` and the matching files under `plugins` with an ABI-compatible Qt 6.11.2 shared build, and keep the directory structure and `bin/qt.conf`. SnipNexs applies no signature check or technical restriction to replacement. A distributor who ships modified Qt libraries must provide the corresponding modified Qt source and retain the applicable notices.

## Microsoft SimpleRecorder sample

The Windows recording pipeline design was informed by Microsoft's SimpleRecorder sample. SnipNexs does not distribute the sample binaries or its SharpDX dependency.

- Project: <https://github.com/MicrosoftDocs/SimpleRecorder>
- Reviewed source revision: `e6ec15684e4c51abca518b4cadd2d89d2b359509`
- Code license: MIT License
- License text: [licenses/MIT-Microsoft-SimpleRecorder.txt](licenses/MIT-Microsoft-SimpleRecorder.txt)

## CTranslate2 (local translation engine)

SnipNexs dynamically links CTranslate2 to run the local translation models described in [docs/local-translation-decision.md](docs/local-translation-decision.md). The library is built from the pinned release v4.8.1 and distributed as a separate shared library next to the Qt DLLs.

- Project: <https://github.com/OpenNMT/CTranslate2>
- Source revision used by this distribution: tag `v4.8.1`
- Code license: MIT License
- License text: [licenses/CTranslate2-MIT.txt](licenses/CTranslate2-MIT.txt)
- The distributed `ctranslate2.dll` statically includes CTranslate2's vendored third-party components: Ruy and cpu_features (Apache-2.0) and spdlog (MIT); their license texts ship inside the CTranslate2 source repository at the revision listed above.

## SentencePiece (tokenizer for local translation)

SnipNexs statically links SentencePiece to tokenize text for the local translation models. The bundled build includes SentencePiece's vendored abseil-cpp (Apache-2.0) and protobuf (BSD-3-Clause) components, which are covered by SentencePiece's own licensing files in its source repository.

- Project: <https://github.com/google/sentencepiece>
- Source revision used by this distribution: tag `v0.2.0`
- Code license: Apache License 2.0
- License text: [licenses/SentencePiece-Apache-2.0.txt](licenses/SentencePiece-Apache-2.0.txt)

## Local translation models

The optional translation language packages are converted from the Helsinki-NLP OPUS-MT models and downloaded on first use; they are not part of the application ZIP.

- `opus-mt-en-zh-int8`: Helsinki-NLP/opus-mt-en-zh, Apache License 2.0
- `opus-mt-zh-en-int8`: Helsinki-NLP/opus-mt-zh-en, CC-BY 4.0
- Conversion pipeline and selection rationale: [docs/local-translation-decision.md](docs/local-translation-decision.md)
- Each installed package records its file digests in `manifest.json`; attribution is also shown in the download prompt and can be reviewed offline in the package manifest.

## Microsoft Visual C++ runtime

The portable ZIP includes application-local release CRT DLLs copied only from the Visual Studio 2022 `VC/Redist/MSVC/.../x64/Microsoft.VC143.CRT` directory. This avoids an administrator-level prerequisite installation. These Microsoft files are distributed under the applicable Visual Studio license terms and are not part of SnipNexs or covered by its GPL license.
