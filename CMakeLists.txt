cmake_minimum_required(VERSION 3.25)

project(SnipNexs
    VERSION 0.4.0
    DESCRIPTION "A lightweight and extensible screenshot toolkit for Windows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_add_executable(SnipNexs WIN32
    src/main.cpp
    src/app/AppIcon.cpp
    src/app/AppIcon.h
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/capture/CaptureController.cpp
    src/capture/CaptureController.h
    src/capture/CaptureGeometry.h
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
    src/translate/BrowserTranslation.cpp
    src/translate/BrowserTranslation.h
)

target_include_directories(SnipNexs PRIVATE src)

target_link_libraries(SnipNexs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
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

if(MSVC)
    target_compile_options(SnipNexs PRIVATE /W4 /permissive- /utf-8 /EHsc)
endif()

include(GNUInstallDirs)
install(TARGETS SnipNexs
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
    LICENSE
    THIRD_PARTY_NOTICES.md
    DESTINATION .
)
install(FILES licenses/LGPL-3.0-only.txt DESTINATION licenses)

qt_generate_deploy_app_script(
    TARGET SnipNexs
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS
        --no-translations
        --no-system-d3d-compiler
        --no-system-dxc-compiler
        --no-opengl-sw
        --compiler-runtime
        --skip-plugin-types generic,iconengines,networkinformation,tls
        --exclude-plugins qsvg
)
install(SCRIPT ${deploy_script})

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

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

    add_executable(CaptureOverlayTests
        tests/CaptureOverlayTests.cpp
        src/capture/CaptureOverlay.cpp
        src/capture/CaptureOverlay.h
        src/capture/CaptureGeometry.h
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

    add_executable(AnnotationDocumentTests
        tests/AnnotationDocumentTests.cpp
        src/editor/AnnotationDocument.cpp
        src/editor/AnnotationDocument.h
    )
    target_include_directories(AnnotationDocumentTests PRIVATE src)
    target_link_libraries(AnnotationDocumentTests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME AnnotationDocumentTests COMMAND AnnotationDocumentTests)
    set_tests_properties(AnnotationDocumentTests PROPERTIES
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
endif()
