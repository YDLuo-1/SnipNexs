cmake_minimum_required(VERSION 3.25)

project(SnipNexs
    VERSION 0.8.4
    DESCRIPTION "A lightweight and extensible screenshot and recording toolkit for Windows"
    LANGUAGES CXX
)

if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)

# ---------------------------------------------------------------------------
# Local translation: pinned llama.cpp + Hy-MT2 GGUF, fully on CPU.
# ---------------------------------------------------------------------------
include(FetchContent)

set(LLAMA_BUILD_IS_DEV OFF CACHE INTERNAL "No llama dev build")
set(LLAMA_BUILD_TESTS OFF CACHE INTERNAL "No llama tests")
set(LLAMA_BUILD_TOOLS OFF CACHE INTERNAL "No llama tools")
set(LLAMA_BUILD_EXAMPLES OFF CACHE INTERNAL "No llama examples")
set(LLAMA_BUILD_SERVER OFF CACHE INTERNAL "No llama server")
set(LLAMA_CURL OFF CACHE INTERNAL "No curl dependency")
set(GGML_NATIVE OFF CACHE BOOL "Portable CPU dispatch" FORCE)
set(GGML_BACKEND_DL ON CACHE BOOL "Load the compatible CPU backend" FORCE)
set(GGML_CPU_ALL_VARIANTS ON CACHE BOOL "Include baseline x64 and optimized CPU backends" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(BUILD_SHARED_LIBS ON)
FetchContent_Declare(llama
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG 4da6337
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(llama)
unset(BUILD_SHARED_LIBS)
get_property(_ggml_targets DIRECTORY "${llama_SOURCE_DIR}/ggml/src" PROPERTY BUILDSYSTEM_TARGETS)
foreach(_llama_target llama ggml ggml-base ${_ggml_targets})
    if(TARGET ${_llama_target})
        set_target_properties(${_llama_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        if(MSVC)
            # llama.cpp uses u8 string literals but builds as C++17 upstream;
            # our project-wide C++20 would break them without this.
            target_compile_options(${_llama_target} PRIVATE /Zc:char8_t-)
        endif()
    endif()
endforeach()

set(SNIPNEXS_LICENSE_FILES
    LICENSE
    licenses/LGPL-3.0-only.txt
    licenses/MIT-Microsoft-SimpleRecorder.txt
    licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md
    licenses/Material-Symbols-Apache-2.0.txt
    licenses/llama.cpp-MIT.txt
)

set(SNIPNEXS_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/SnipNexs.rc")
configure_file(resources/SnipNexs.rc.in "${SNIPNEXS_RC_FILE}" @ONLY)
set_source_files_properties("${SNIPNEXS_RC_FILE}" PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"")

qt_add_executable(SnipNexs WIN32
    "${SNIPNEXS_RC_FILE}"
    src/main.cpp
    src/app/AppIcon.cpp
    src/app/AppIcon.h
    src/app/ImageSaveDialog.h
    src/app/AboutDialog.cpp
    src/app/AboutDialog.h
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/app/SettingsDialog.cpp
    src/app/SettingsDialog.h
    src/app/UpdateChecker.cpp
    src/app/UpdateChecker.h
    src/app/UpdateVersion.h
    src/capture/CaptureController.cpp
    src/capture/CaptureController.h
    src/capture/CaptureGeometry.h
    src/capture/ToolbarIcons.cpp
    src/capture/ToolbarIcons.h
    src/capture/CaptureHistoryStore.cpp
    src/capture/CaptureHistoryStore.h
    src/capture/CaptureOverlay.cpp
    src/capture/CaptureOverlay.h
    src/platform/windows/NativeTrayIcon.cpp
    src/platform/windows/NativeTrayIcon.h
    src/capture/CaptureToolbar.cpp
    src/capture/CaptureToolbar.h
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
    src/recorder/RecordingOutput.cpp
    src/recorder/RecordingOutput.h
    src/recorder/AudioBuffer.h
    src/recorder/RecordingIndicator.cpp
    src/recorder/RecordingIndicator.h
    src/recorder/RecordingSettings.h
    src/recorder/ScreenRecorderService.cpp
    src/recorder/ScreenRecorderService.h
    src/translate/BrowserTranslation.cpp
    src/translate/BrowserTranslation.h
    src/translate/LlamaTranslation.cpp
    src/translate/LlamaTranslation.h
    src/translate/TranslationModelInstaller.cpp
    src/translate/TranslationModelInstaller.h
    src/translate/TranslationModels.cpp
    src/translate/TranslationModels.h
    src/translate/TranslationService.cpp
    src/translate/TranslationService.h
    src/translate/TranslationTextSplitter.cpp
    src/translate/TranslationTextSplitter.h
)

set_target_properties(SnipNexs PROPERTIES
    QT_WINDOWS_APP_PROJECT_IDENTIFIER "io.github.ydluo.snipnexs"
    QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL "asInvoker"
)

target_include_directories(SnipNexs PRIVATE src)

target_link_libraries(SnipNexs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    llama
    d3d11
    dwmapi
    dxgi
    shcore
    windowsapp
)

if(MSVC)
    # Load the translation engine only when first used.
    target_link_options(SnipNexs PRIVATE
        "/DELAYLOAD:llama.dll")
    target_link_libraries(SnipNexs PRIVATE delayimp)
endif()

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
    # ggml is built with /openmp, so the portable package
    # must ship vcomp140.dll next to the application-local CRT DLLs.
    set(CMAKE_INSTALL_OPENMP_LIBRARIES TRUE)
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
    licenses/Material-Symbols-Apache-2.0.txt
    licenses/llama.cpp-MIT.txt
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
        --skip-plugin-types generic,iconengines,networkinformation
        --exclude-plugins qsvg
)
install(SCRIPT ${deploy_script})

# Third-party install rules ship their command-line tools and dev files
# (headers, import/static libs, CMake configs); SnipNexs only needs the
# runtime DLLs, so strip them from the deployment layout.
install(CODE
    [[file(REMOVE_RECURSE
         "${CMAKE_INSTALL_PREFIX}/include"
         "${CMAKE_INSTALL_PREFIX}/lib")]])

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Test)

    # Some security configurations deny unsigned test executables access to
    # the user's default temp folder. Keep test scratch files inside the build
    # tree so CTest is reproducible without changing the product paths.
    set(SNIPNEXS_TEST_TEMP_DIR "${CMAKE_BINARY_DIR}/test-temp")
    file(MAKE_DIRECTORY "${SNIPNEXS_TEST_TEMP_DIR}")

    add_test(NAME SnipNexsSelfTest COMMAND SnipNexs --self-test)
    set_tests_properties(SnipNexsSelfTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        PASS_REGULAR_EXPRESSION "self-test: ok"
    )

    # Runs without a GUI platform plugin so the deployed layout can be
    # checked in place: Qt TLS backend and llama.dll must all
    # load from the application directory and its plugins/ folder.
    add_test(NAME SnipNexsRuntimeSelfTest COMMAND SnipNexs --self-test-runtime)
    set_tests_properties(SnipNexsRuntimeSelfTest PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        PASS_REGULAR_EXPRESSION "runtime-self-test: ok"
        TIMEOUT 60
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
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;TEMP=${SNIPNEXS_TEST_TEMP_DIR};TMP=${SNIPNEXS_TEST_TEMP_DIR}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureOverlayTests
        tests/CaptureOverlayTests.cpp
        src/capture/CaptureOverlay.cpp
        src/capture/CaptureOverlay.h
        src/capture/CaptureToolbar.cpp
        src/capture/CaptureToolbar.h
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

    add_executable(NativeTrayIconTests
        tests/NativeTrayIconTests.cpp
        src/app/AppIcon.cpp
        src/app/AppIcon.h
        src/platform/windows/NativeTrayIcon.cpp
        src/platform/windows/NativeTrayIcon.h
    )
    target_include_directories(NativeTrayIconTests PRIVATE src)
    target_link_libraries(NativeTrayIconTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
    )
    # Registers a real shell tray icon briefly; headless sessions without a
    # tray must skip rather than fail.
    add_test(NAME NativeTrayIconTests COMMAND NativeTrayIconTests)
    set_tests_properties(NativeTrayIconTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        SKIP_RETURN_CODE 77
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
        dbghelp
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

    add_executable(TranslationModelsTests
        tests/TranslationModelsTests.cpp
        src/translate/TranslationModels.cpp
        src/translate/TranslationModels.h
    )
    target_include_directories(TranslationModelsTests PRIVATE src)
    target_link_libraries(TranslationModelsTests PRIVATE Qt6::Core)
    add_test(NAME TranslationModelsTests COMMAND TranslationModelsTests)
    set_tests_properties(TranslationModelsTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(LocalLlamaTests
        tests/LocalLlamaTests.cpp
        src/translate/LlamaTranslation.cpp
        src/translate/LlamaTranslation.h
        src/translate/TranslationTextSplitter.cpp
        src/translate/TranslationTextSplitter.h
    )
    target_include_directories(LocalLlamaTests PRIVATE src)
    target_link_libraries(LocalLlamaTests PRIVATE Qt6::Core llama)
    add_test(NAME LocalLlamaTests COMMAND LocalLlamaTests)
    add_test(NAME LocalLlamaBaselineTests COMMAND LocalLlamaTests --baseline)
    set_tests_properties(LocalLlamaTests LocalLlamaBaselineTests PROPERTIES
        SKIP_RETURN_CODE 77
        TIMEOUT 300
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
    add_test(NAME WindowsChineseOcrTests COMMAND WindowsOcrTests --chinese)
    set_tests_properties(WindowsOcrTests WindowsChineseOcrTests PROPERTIES
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
        TIMEOUT 180
        ENVIRONMENT "TEMP=${SNIPNEXS_TEST_TEMP_DIR};TMP=${SNIPNEXS_TEST_TEMP_DIR}"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    function(snipnexs_regression_test name)
        add_executable(${name} tests/${name}.cpp ${ARGN})
        target_include_directories(${name} PRIVATE src)
        target_link_libraries(${name} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Test)
        target_compile_definitions(${name} PRIVATE UNICODE _UNICODE NOMINMAX
            SNIPNEXS_VERSION="${PROJECT_VERSION}")
        target_compile_options(${name} PRIVATE /W4 /permissive- /utf-8 /EHsc)
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES TIMEOUT 30
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endfunction()

    snipnexs_regression_test(RecordingCoreTests
        src/recorder/ScreenRecorderService.cpp src/recorder/RecordingOutput.cpp)
    target_link_libraries(RecordingCoreTests PRIVATE windowsapp)
    snipnexs_regression_test(AudioCaptureTests)
    target_link_libraries(AudioCaptureTests PRIVATE windowsapp d3d11 dxgi shcore ole32)
    set_tests_properties(AudioCaptureTests PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME MicrophoneCaptureTests COMMAND AudioCaptureTests --microphone)
    set_tests_properties(MicrophoneCaptureTests PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    snipnexs_regression_test(TranslationModelInstallerTests
        src/translate/TranslationModels.cpp src/translate/TranslationModelInstaller.cpp)
    add_test(NAME TranslationModelDownloadTests COMMAND TranslationModelInstallerTests --network)
    set_tests_properties(TranslationModelDownloadTests PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 600
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    snipnexs_regression_test(TranslationServiceTests
        src/translate/TranslationService.cpp src/translate/TranslationTextSplitter.cpp)
    snipnexs_regression_test(OcrResultWindowTests
        src/ocr/OcrResultWindow.cpp src/translate/TranslationService.h
        src/translate/TranslationModelInstaller.cpp src/translate/TranslationModels.cpp
        src/translate/BrowserTranslation.cpp)
    snipnexs_regression_test(MainWindowTests
        src/app/MainWindow.cpp src/app/SettingsDialog.cpp src/app/UpdateChecker.cpp
        src/app/AppIcon.cpp src/app/AboutDialog.cpp
        src/translate/TranslationModels.cpp src/platform/windows/NativeTrayIcon.cpp)

    add_test(NAME ReleaseFilesTests COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass
        -File ${CMAKE_SOURCE_DIR}/tests/ReleaseFilesTests.ps1 -WorkRoot ${SNIPNEXS_TEST_TEMP_DIR})
    set_tests_properties(ReleaseFilesTests PROPERTIES TIMEOUT 60)

    add_test(NAME ReleaseLayoutTests
        COMMAND ${CMAKE_COMMAND}
            -DSNIPNEXS_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DSNIPNEXS_STAGE_DIR=${CMAKE_BINARY_DIR}/release-layout-test
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ReleaseLayoutTests.cmake
    )
    # Desktop capture tests return 77 under offscreen. Opt in to actual desktop
    # acceptance explicitly in a separate build with this option enabled.
    option(SNIPNEXS_DESKTOP_TESTS "Allow interactive desktop acceptance tests" OFF)
    get_property(snipnexs_tests DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS snipnexs_tests)
        set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
            "TEMP=${SNIPNEXS_TEST_TEMP_DIR}" "TMP=${SNIPNEXS_TEST_TEMP_DIR}")
        if(SNIPNEXS_DESKTOP_TESTS AND test MATCHES "^(NativeTrayIconTests|CaptureExclusionTests|NativeScreenRecorderTests)$")
            set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=windows")
        else()
            set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        endif()
    endforeach()
endif()
