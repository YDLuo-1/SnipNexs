if(NOT DEFINED SNIPNEXS_BUILD_DIR OR NOT DEFINED SNIPNEXS_STAGE_DIR)
    message(FATAL_ERROR "SNIPNEXS_BUILD_DIR and SNIPNEXS_STAGE_DIR are required")
endif()

file(REMOVE_RECURSE "${SNIPNEXS_STAGE_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${SNIPNEXS_BUILD_DIR}"
        --config Release --prefix "${SNIPNEXS_STAGE_DIR}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Release install failed:\n${install_output}\n${install_error}")
endif()

set(required_files
    bin/SnipNexs.exe
    bin/Qt6Core.dll
    bin/Qt6Gui.dll
    bin/Qt6Widgets.dll
    bin/Qt6Network.dll
    bin/msvcp140.dll
    bin/msvcp140_atomic_wait.dll
    bin/vcruntime140.dll
    bin/vcruntime140_1.dll
    bin/vcomp140.dll
    bin/ctranslate2.dll
    bin/llama.dll
    bin/ggml.dll
    bin/ggml-base.dll
    bin/ggml-cpu.dll
    bin/qt.conf
    plugins/platforms/qwindows.dll
    plugins/styles/qmodernwindowsstyle.dll
    plugins/imageformats/qjpeg.dll
    LICENSE
    THIRD_PARTY_NOTICES.md
    licenses/LGPL-3.0-only.txt
    licenses/MIT-Microsoft-SimpleRecorder.txt
    licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md
    licenses/qtbase-6.11.2.spdx
)

foreach(relative_path IN LISTS required_files)
    if(NOT EXISTS "${SNIPNEXS_STAGE_DIR}/${relative_path}")
        message(FATAL_ERROR "Missing release file: ${relative_path}")
    endif()
endforeach()

file(GLOB tls_plugins "${SNIPNEXS_STAGE_DIR}/plugins/tls/*.dll")
if(NOT tls_plugins)
    message(FATAL_ERROR "Missing Qt TLS backend plugin under plugins/tls")
endif()

if(EXISTS "${SNIPNEXS_STAGE_DIR}/bin/vc_redist.x64.exe")
    message(FATAL_ERROR "Redundant VC++ installer must not be included with app-local runtime DLLs")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "QT_PLUGIN_PATH=${SNIPNEXS_STAGE_DIR}/plugins"
        "${SNIPNEXS_STAGE_DIR}/bin/SnipNexs.exe" --self-test-runtime
    RESULT_VARIABLE runtime_result
    OUTPUT_VARIABLE runtime_output
    ERROR_VARIABLE runtime_error
    TIMEOUT 60
)
if(NOT runtime_result EQUAL 0)
    message(FATAL_ERROR
        "Deployed runtime self-test failed (${runtime_result}):\n"
        "${runtime_output}\n${runtime_error}")
endif()

message(STATUS "Release layout contains all required runtime and license files")
file(REMOVE_RECURSE "${SNIPNEXS_STAGE_DIR}")
