cmake_minimum_required(VERSION 3.25)

project(SnipNexs
    VERSION 0.1.0
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
)

target_link_libraries(SnipNexs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(SnipNexs PRIVATE
    UNICODE
    _UNICODE
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
    add_test(NAME SnipNexsSelfTest COMMAND SnipNexs --self-test)
    set_tests_properties(SnipNexsSelfTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        PASS_REGULAR_EXPRESSION "self-test: ok"
    )
endif()
