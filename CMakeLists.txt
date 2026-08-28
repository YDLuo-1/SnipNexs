cmake_minimum_required(VERSION 3.25)

project(SnipNexs
    VERSION 0.8.0
    DESCRIPTION "A lightweight and extensible screenshot and recording toolkit for Windows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)

# ---------------------------------------------------------------------------
# Local translation engine (see docs/local-translation-decision.md)
#
# CTranslate2 (MIT) runs OPUS-MT models on CPU; SentencePiece (Apache-2.0)
# tokenizes raw text for those models. Both are fetched at pinned tags and
# built from source. The engine stays CPU-only: no MKL/DNNL/CUDA, no GPU
# requirement, Windows 10 2004 x64 remains the floor.
# ---------------------------------------------------------------------------
include(FetchContent)

set(BUILD_SHARED_LIBS ON) # ctranslate2 becomes a runtime DLL next to SnipNexs.exe
set(OPENMP_RUNTIME "COMP" CACHE INTERNAL "Use the MSVC OpenMP runtime (vcomp)")
set(WITH_MKL OFF CACHE INTERNAL "No MKL for CTranslate2")
set(WITH_DNNL OFF CACHE INTERNAL "No DNNL for CTranslate2")
set(WITH_RUY ON CACHE INTERNAL "Ruy is CTranslate2's vendored CPU GEMM backend")
set(BUILD_CLI OFF CACHE INTERNAL "No CTranslate2 CLI tools")
set(BUILD_TESTS OFF CACHE INTERNAL "No CTranslate2 tests")
FetchContent_Declare(ctranslate2
    GIT_REPOSITORY https://github.com/OpenNMT/CTranslate2.git
    GIT_TAG v4.8.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ctranslate2)
# Keep the runtime DLL next to SnipNexs.exe and the test executables so
# Windows DLL search and CTest find it without extra PATH setup.
set_target_properties(ctranslate2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
unset(BUILD_SHARED_LIBS)

set(SPM_ENABLE_SHARED OFF CACHE INTERNAL "SentencePiece static only")
set(SPM_ENABLE_TEST OFF CACHE INTERNAL "No SentencePiece tests")
set(SPM_ENABLE_TRAINING OFF CACHE INTERNAL "No SentencePiece trainer")
set(SPM_BUILD_EXAMPLES OFF CACHE INTERNAL "No SentencePiece examples")
FetchContent_Declare(sentencepiece
    GIT_REPOSITORY https://github.com/google/sentencepiece.git
    GIT_TAG v0.2.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sentencepiece)

# SentencePiece exposes its headers via directory-scope include paths only,
# so consumers must add the source include directory themselves.
set(SNIPNEXS_SENTENCEPIECE_INCLUDE_DIR "${sentencepiece_SOURCE_DIR}/src")

set(SNIPNEXS_LICENSE_FILES
    LICENSE
    licenses/LGPL-3.0-only.txt
    licenses/MIT-Microsoft-SimpleRecorder.txt
    licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md
    licenses/CTranslate2-MIT.txt
    licenses/SentencePiece-Apache-2.0.txt
)

qt_add_executable(SnipNexs WIN32
    src/main.cpp
    src/app/AppIcon.cpp
    src/app/AppIcon.h
    src/app/AboutDialog.cpp
    src/app/AboutDialog.h
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/capture/CaptureController.cpp
    src/capture/CaptureController.h
    src/capture/CaptureGeometry.h
    src/capture/ToolbarIcons.cpp
    src/capture/ToolbarIcons.h
    src/capture/CaptureHistoryStore.cpp
    src/capture/CaptureHistoryStore.h
    src/capture/CaptureOverlay.cpp
    src/capture/CaptureOverlay.h
    src/editor/AnnotationDocument.cpp
    src/editor/AnnotationDocument.h
    src/ocr/OcrResultWindow.cpp
    src/ocr/OcrResultWindow.h
    src/ocr/OcrService.cpp
    src/ocr/OcrService.h
    src/ocr/WindowsOcr.cpp
    src/ocr/WindowsOcr.h
    src/pin/PinWindow.cpp
    src/pin/PinWindow.h
    src/platform/windows/GlobalHotkey.cpp
    src/platform/windows/GlobalHotkey.h
    src/platform/windows/CaptureExclusion.cpp
    src/platform/windows/CaptureExclusion.h
    src/platform/windows/WindowTargeting.cpp
    src/platform/windows/WindowTargeting.h
    src/recorder/NativeScreenRecorder.cpp
    src/recorder/NativeScreenRecorder.h
    src/recorder/RecorderController.cpp
    src/recorder/RecorderController.h
    src/recorder/RecordingIndicator.cpp
    src/recorder/RecordingIndicator.h
    src/recorder/RecordingSettings.h
    src/recorder/ScreenRecorderService.cpp
    src/recorder/ScreenRecorderService.h
    src/translate/BrowserTranslation.cpp
    src/translate/BrowserTranslation.h
    src/translate/LocalTranslation.cpp
    src/translate/LocalTranslation.h
    src/translate/TranslationModelInstaller.cpp
    src/translate/TranslationModelInstaller.h
    src/translate/TranslationModels.cpp
    src/translate/TranslationModels.h
    src/translate/TranslationService.cpp
    src/translate/TranslationService.h
    src/translate/TranslationTextSplitter.cpp
    src/translate/TranslationTextSplitter.h
)

target_include_directories(SnipNexs PRIVATE src ${SNIPNEXS_SENTENCEPIECE_INCLUDE_DIR})

target_link_libraries(SnipNexs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    ctranslate2
    sentencepiece-static
    d3d11
    dwmapi
    dxgi
    shcore
    windowsapp
)

target_compile_definitions(SnipNexs PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    SNIPNEXS_VERSION="${PROJECT_VERSION}"
)

qt_add_translations(SnipNexs
    TS_FILES translations/snipnexs_en.ts
    RESOURCE_PREFIX /i18n
)

qt_add_resources(SnipNexs "license_resources"
    PREFIX "/licenses"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${SNIPNEXS_LICENSE_FILES}
)

if(MSVC)
    target_compile_options(SnipNexs PRIVATE /W4 /permissive- /utf-8 /EHsc)
endif()

include(GNUInstallDirs)
if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    include(InstallRequiredSystemLibraries)
endif()

get_filename_component(SNIPNEXS_QT_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
set(SNIPNEXS_QTBASE_SBOM
    "${SNIPNEXS_QT_PREFIX}/sbom/qtbase-6.11.2.spdx")
if(NOT EXISTS "${SNIPNEXS_QTBASE_SBOM}")
    message(FATAL_ERROR "Qt 6.11.2 SBOM not found: ${SNIPNEXS_QTBASE_SBOM}")
endif()

install(TARGETS SnipNexs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
    LICENSE
    THIRD_PARTY_NOTICES.md
    DESTINATION .
)
install(FILES
    licenses/LGPL-3.0-only.txt
    licenses/MIT-Microsoft-SimpleRecorder.txt
    licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md
    licenses/CTranslate2-MIT.txt
    licenses/SentencePiece-Apache-2.0.txt
    ${SNIPNEXS_QTBASE_SBOM}
    DESTINATION licenses
)

qt_generate_deploy_app_script(
    TARGET SnipNexs
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS
        --no-translations
        --no-system-d3d-compiler
        --no-system-dxc-compiler
        --no-opengl-sw
        --no-compiler-runtime
        --skip-plugin-types generic,iconengines,networkinformation,tls
        --exclude-plugins qsvg
)
install(SCRIPT ${deploy_script})

# SentencePiece's own install rules ship its command-line tools; SnipNexs
# only links the static library, so drop them from the deployment layout.
install(CODE
    "foreach(_spm_tool spm_train spm_encode spm_decode spm_normalize spm_export_vocab)
         if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/bin/\${_spm_tool}.exe\")
             file(REMOVE \"\${CMAKE_INSTALL_PREFIX}/bin/\${_spm_tool}.exe\")
         endif()
     endforeach()")

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Test)

    add_test(NAME SnipNexsSelfTest COMMAND SnipNexs --self-test)
    set_tests_properties(SnipNexsSelfTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        PASS_REGULAR_EXPRESSION "self-test: ok"
    )

    add_executable(CaptureGeometryTests tests/CaptureGeometryTests.cpp)
    target_include_directories(CaptureGeometryTests PRIVATE src)
    target_link_libraries(CaptureGeometryTests PRIVATE Qt6::Core)
    add_test(NAME CaptureGeometryTests COMMAND CaptureGeometryTests)
    set_tests_properties(CaptureGeometryTests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureHistoryStoreTests
        tests/CaptureHistoryStoreTests.cpp
        src/capture/CaptureHistoryStore.cpp
        src/capture/CaptureHistoryStore.h
    )
    target_include_directories(CaptureHistoryStoreTests PRIVATE src)
    target_link_libraries(CaptureHistoryStoreTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
    )
    add_test(NAME CaptureHistoryStoreTests COMMAND CaptureHistoryStoreTests)
    set_tests_properties(CaptureHistoryStoreTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureOverlayTests
        tests/CaptureOverlayTests.cpp
        src/capture/CaptureOverlay.cpp
        src/capture/CaptureOverlay.h
        src/capture/CaptureGeometry.h
        src/capture/ToolbarIcons.cpp
        src/capture/ToolbarIcons.h
        src/editor/AnnotationDocument.cpp
        src/editor/AnnotationDocument.h
    )
    target_include_directories(CaptureOverlayTests PRIVATE src)
    target_link_libraries(CaptureOverlayTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME CaptureOverlayTests COMMAND CaptureOverlayTests)
    set_tests_properties(CaptureOverlayTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureExclusionTests
        tests/CaptureExclusionTests.cpp
        src/platform/windows/CaptureExclusion.cpp
        src/platform/windows/CaptureExclusion.h
    )
    target_include_directories(CaptureExclusionTests PRIVATE src)
    target_link_libraries(CaptureExclusionTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        dwmapi
        user32
    )
    add_test(NAME CaptureExclusionTests COMMAND CaptureExclusionTests)
    set_tests_properties(CaptureExclusionTests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        SKIP_RETURN_CODE 77
    )

    add_executable(PinWindowTests
        tests/PinWindowTests.cpp
        src/pin/PinWindow.cpp
        src/pin/PinWindow.h
        src/capture/ToolbarIcons.cpp
        src/capture/ToolbarIcons.h
    )
    target_include_directories(PinWindowTests PRIVATE src)
    target_link_libraries(PinWindowTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
    )
    add_test(NAME PinWindowTests COMMAND PinWindowTests)
    set_tests_properties(PinWindowTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(AboutDialogTests
        tests/AboutDialogTests.cpp
        src/app/AboutDialog.cpp
        src/app/AboutDialog.h
        src/app/AppIcon.cpp
        src/app/AppIcon.h
    )
    target_include_directories(AboutDialogTests PRIVATE src)
    target_link_libraries(AboutDialogTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
    target_compile_definitions(AboutDialogTests PRIVATE
        SNIPNEXS_VERSION="${PROJECT_VERSION}"
    )
    qt_add_resources(AboutDialogTests "about_license_resources"
        PREFIX "/licenses"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${SNIPNEXS_LICENSE_FILES}
    )
    add_test(NAME AboutDialogTests COMMAND AboutDialogTests)
    set_tests_properties(AboutDialogTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(RecordingIndicatorTests
        tests/RecordingIndicatorTests.cpp
        src/recorder/RecordingIndicator.cpp
        src/recorder/RecordingIndicator.h
    )
    target_include_directories(RecordingIndicatorTests PRIVATE src)
    target_link_libraries(RecordingIndicatorTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        user32
    )
    target_compile_definitions(RecordingIndicatorTests PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
    )
    add_test(NAME RecordingIndicatorTests COMMAND RecordingIndicatorTests)
    set_tests_properties(RecordingIndicatorTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(WindowTargetingTests
        tests/WindowTargetingTests.cpp
        src/platform/windows/WindowTargeting.cpp
        src/platform/windows/WindowTargeting.h
        src/capture/CaptureGeometry.h
    )
    target_include_directories(WindowTargetingTests PRIVATE src)
    target_link_libraries(WindowTargetingTests PRIVATE
        Qt6::Core
        dwmapi
        user32
    )
    target_compile_definitions(WindowTargetingTests PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
    )
    add_test(NAME WindowTargetingTests COMMAND WindowTargetingTests)
    set_tests_properties(WindowTargetingTests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(AnnotationDocumentTests
        tests/AnnotationDocumentTests.cpp
        src/editor/AnnotationDocument.cpp
        src/editor/AnnotationDocument.h
    )
    target_include_directories(AnnotationDocumentTests PRIVATE src)
    target_link_libraries(AnnotationDocumentTests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME AnnotationDocumentTests COMMAND AnnotationDocumentTests)
    set_tests_properties(AnnotationDocumentTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(BrowserTranslationTests
        tests/BrowserTranslationTests.cpp
        src/translate/BrowserTranslation.cpp
        src/translate/BrowserTranslation.h
    )
    target_include_directories(BrowserTranslationTests PRIVATE src)
    target_link_libraries(BrowserTranslationTests PRIVATE Qt6::Core)
    add_test(NAME BrowserTranslationTests COMMAND BrowserTranslationTests)
    set_tests_properties(BrowserTranslationTests PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(TranslationTextSplitterTests
        tests/TranslationTextSplitterTests.cpp
        src/translate/TranslationTextSplitter.cpp
        src/translate/TranslationTextSplitter.h
    )
    target_include_directories(TranslationTextSplitterTests PRIVATE src)
    target_link_libraries(TranslationTextSplitterTests PRIVATE Qt6::Core)
    add_test(NAME TranslationTextSplitterTests COMMAND TranslationTextSplitterTests)
    set_tests_properties(TranslationTextSplitterTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ToolbarIconsTests
        tests/ToolbarIconsTests.cpp
        src/capture/ToolbarIcons.cpp
        src/capture/ToolbarIcons.h
    )
    target_include_directories(ToolbarIconsTests PRIVATE src)
    target_link_libraries(ToolbarIconsTests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME ToolbarIconsTests COMMAND ToolbarIconsTests)
    set_tests_properties(ToolbarIconsTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(LocalTranslationTests
        tests/LocalTranslationTests.cpp
        src/translate/LocalTranslation.cpp
        src/translate/LocalTranslation.h
        src/translate/TranslationTextSplitter.cpp
        src/translate/TranslationTextSplitter.h
    )
    target_include_directories(LocalTranslationTests PRIVATE
        src ${SNIPNEXS_SENTENCEPIECE_INCLUDE_DIR})
    target_link_libraries(LocalTranslationTests PRIVATE
        Qt6::Core
        ctranslate2
        sentencepiece-static
    )
    add_test(NAME LocalTranslationTests COMMAND LocalTranslationTests)
    set_tests_properties(LocalTranslationTests PROPERTIES
        SKIP_RETURN_CODE 77
        TIMEOUT 120
        ENVIRONMENT "SNIPNEXS_TEST_MODEL_DIR=${CMAKE_SOURCE_DIR}/build/models/src/opus-mt-en-zh-int8"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(WindowsOcrTests
        tests/WindowsOcrTests.cpp
        src/ocr/WindowsOcr.cpp
        src/ocr/WindowsOcr.h
    )
    target_include_directories(WindowsOcrTests PRIVATE src)
    target_link_libraries(WindowsOcrTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets windowsapp)
    target_compile_definitions(WindowsOcrTests PRIVATE UNICODE _UNICODE NOMINMAX)
    if(MSVC)
        target_compile_options(WindowsOcrTests PRIVATE /W4 /permissive- /utf-8 /EHsc)
    endif()
    add_test(NAME WindowsOcrTests COMMAND WindowsOcrTests)
    set_tests_properties(WindowsOcrTests PROPERTIES
        SKIP_RETURN_CODE 77
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(NativeScreenRecorderTests
        tests/NativeScreenRecorderTests.cpp
        src/recorder/NativeScreenRecorder.cpp
        src/recorder/NativeScreenRecorder.h
        src/recorder/RecordingSettings.h
    )
    target_include_directories(NativeScreenRecorderTests PRIVATE src)
    target_link_libraries(NativeScreenRecorderTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        d3d11
        dxgi
        mfplat
        mfreadwrite
        mfuuid
        propsys
        shcore
        windowsapp
    )
    target_compile_definitions(NativeScreenRecorderTests PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )
    if(MSVC)
        target_compile_options(NativeScreenRecorderTests PRIVATE /W4 /permissive- /utf-8 /EHsc)
    endif()
    add_test(NAME NativeScreenRecorderTests COMMAND NativeScreenRecorderTests)
    set_tests_properties(NativeScreenRecorderTests PROPERTIES
        SKIP_RETURN_CODE 77
        TIMEOUT 20
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_test(NAME ReleaseLayoutTests
        COMMAND ${CMAKE_COMMAND}
            -DSNIPNEXS_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DSNIPNEXS_STAGE_DIR=${CMAKE_BINARY_DIR}/release-layout-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ReleaseLayoutTests.cmake
    )
endif()
