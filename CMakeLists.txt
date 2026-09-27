cmake_minimum_required(VERSION 3.22)

project(libbgt VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 字符集选项按目标下发，不能用顶层 add_compile_options：MSVC 的 /utf-8 与
# /execution-charset:<代码页> 互斥（D8016），而 BGT_SOURCE_ENCODING=GBK 时
# 消费端必须指定执行字符集。bgt_apply_text_encoding() 见下方定义。
#
# 本仓库自身的源码一律是 UTF-8；BGT_SOURCE_ENCODING 描述的是“窄字符串在
# 运行期是什么编码”，也就是学生源码所在的编码。详见 docs/encoding.md。

option(BGT_BUILD_EXAMPLES "Build libbgt examples" ON)
option(BGT_BUILD_TESTS "Build libbgt tests" OFF)
option(BGT_BUILD_SHARED "Build libbgt as a shared library" OFF)
option(BGT_BUILD_VENDORED "Merge libbgt and vendored dependencies into one MSVC static library" OFF)
option(BGT_USE_SYSTEM_SDL "Use system-installed SDL3 and SDL3_ttf packages" OFF)

# 运行期窄字符串编码，也就是学生源码所在的编码。UTF-8 是默认值，行为与旧版一致；
# GBK 让学生的源码、printf 输出与存档文件统一保持 GBK，由 libbgt 在 SDL 边界做
# GBK <-> UTF-8 转换。详见 docs/encoding.md。
set(BGT_SOURCE_ENCODING "UTF-8" CACHE STRING
    "Narrow string encoding used by user code: UTF-8 or GBK")
set_property(CACHE BGT_SOURCE_ENCODING PROPERTY STRINGS UTF-8 GBK)

string(TOUPPER "${BGT_SOURCE_ENCODING}" BGT_SOURCE_ENCODING_UPPER)
if(BGT_SOURCE_ENCODING_UPPER STREQUAL "UTF-8" OR BGT_SOURCE_ENCODING_UPPER STREQUAL "UTF8")
    set(BGT_ENCODING_IS_GBK OFF)
elseif(BGT_SOURCE_ENCODING_UPPER STREQUAL "GBK")
    set(BGT_ENCODING_IS_GBK ON)
else()
    message(FATAL_ERROR
        "BGT_SOURCE_ENCODING must be UTF-8 or GBK, got '${BGT_SOURCE_ENCODING}'")
endif()

if(BGT_ENCODING_IS_GBK AND NOT WIN32)
    message(FATAL_ERROR
        "BGT_SOURCE_ENCODING=GBK is currently implemented on Windows only "
        "(WinAPI); the iconv backend is reserved, see docs/encoding.md")
endif()

if(BGT_BUILD_VENDORED AND (NOT MSVC OR BGT_BUILD_SHARED OR BGT_USE_SYSTEM_SDL))
    message(FATAL_ERROR
        "BGT_BUILD_VENDORED requires MSVC, bundled SDL, and BGT_BUILD_SHARED=OFF")
endif()

set(BGT_LIBRARY_TYPE STATIC)
if(BGT_BUILD_SHARED)
    set(BGT_LIBRARY_TYPE SHARED)
endif()

if(BGT_USE_SYSTEM_SDL)
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_ttf CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
    find_package(SDL3_mixer CONFIG REQUIRED)

    if(NOT TARGET SDL3::SDL3-static OR NOT TARGET SDL3_ttf::SDL3_ttf-static
       OR NOT TARGET SDL3_image::SDL3_image-static
       OR NOT TARGET SDL3_mixer::SDL3_mixer-static)
        message(FATAL_ERROR
            "BGT_USE_SYSTEM_SDL requires SDL3, SDL3_ttf, SDL3_image and SDL3_mixer packages with static targets")
    endif()
else()
    # 依赖的获取与配置都在 third_party 目录作用域里，这样 MSVC 的 /utf-8 只作用
    # 于 vendored 源码，不会传染给需要指定执行字符集的项目目标。
    add_subdirectory(third_party)
endif()

# 给消费 libbgt 的目标（示例、演示、测试、学生工程）下发字符集选项。
# 仓库内的源码都是 UTF-8 文件，所以默认的 UTF-8 模式就是 MSVC 的 /utf-8。
function(bgt_apply_text_encoding target)
    if(MSVC)
        if(BGT_ENCODING_IS_GBK)
            # 源码是 UTF-8 文件，但运行期窄字符串要是 GBK：源字符集 UTF-8、执行字符集
            # 936。/utf-8 与 /execution-charset 互斥（D8016），不能同时使用。
            target_compile_options(${target} PRIVATE
                /source-charset:utf-8 /execution-charset:.936)
        else()
            target_compile_options(${target} PRIVATE /utf-8)
        endif()
    elseif(BGT_ENCODING_IS_GBK)
        # GCC / Clang：同样是“UTF-8 源码 -> GBK 执行字符集”，依赖编译器的 iconv 支持。
        target_compile_options(${target} PRIVATE
            -finput-charset=UTF-8 -fexec-charset=GBK)
    endif()
endfunction()

add_library(bgt ${BGT_LIBRARY_TYPE} src/bgt.cpp src/bgt_encoding.cpp)
add_library(libbgt::bgt ALIAS bgt)

# 库自身始终按 UTF-8 编译（源码与中文注释都是 UTF-8，与 BGT_SOURCE_ENCODING 无关）。
# BGT_BUILDING_LIBRARY 关闭头文件里面向消费端的编译期编码检查。
target_compile_definitions(bgt PRIVATE BGT_BUILDING_LIBRARY)
if(BGT_ENCODING_IS_GBK)
    # 公开传递，让消费端头文件里的编码检查与库的构建模式一致。
    target_compile_definitions(bgt PUBLIC BGT_SOURCE_ENCODING=BGT_ENCODING_GBK)
endif()

target_compile_features(bgt PUBLIC cxx_std_20)
set_target_properties(bgt PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(
    bgt
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if(BGT_BUILD_SHARED)
    target_link_libraries(bgt PRIVATE SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static
        SDL3_image::SDL3_image-static SDL3_mixer::SDL3_mixer-static)
else()
    target_compile_definitions(bgt PUBLIC BGT_STATIC_DEFINE)
    target_link_libraries(bgt PUBLIC SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static
        SDL3_image::SDL3_image-static SDL3_mixer::SDL3_mixer-static)
endif()

set_target_properties(bgt PROPERTIES
    DEBUG_POSTFIX d
    WINDOWS_EXPORT_ALL_SYMBOLS "${BGT_BUILD_SHARED}")

if(BGT_BUILD_VENDORED)
    set(BGT_VENDORED_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/bgt_vendored.lib")

    # SDL3_image 只是把 libpng / zlib 作为 PRIVATE 静态库链接，lib.exe 不会自动把
    # 它们收进合并结果；不显式列出的话，学生工程一调用 bgt_load_image 就会缺
    # png_* 符号。这里按“目标存在才合并”的方式处理，方便裁剪 PNG 支持的构建。
    set(BGT_VENDORED_EXTRA_TARGETS "")
    foreach(candidate png_static zlibstatic)
        if(TARGET ${candidate})
            list(APPEND BGT_VENDORED_EXTRA_TARGETS ${candidate})
        endif()
    endforeach()
    set(BGT_VENDORED_EXTRA_FILES "")
    foreach(extra ${BGT_VENDORED_EXTRA_TARGETS})
        list(APPEND BGT_VENDORED_EXTRA_FILES "$<TARGET_FILE:${extra}>")
    endforeach()

    add_custom_command(
        OUTPUT "${BGT_VENDORED_LIBRARY}"
        COMMAND "${CMAKE_AR}" /NOLOGO "/OUT:${BGT_VENDORED_LIBRARY}"
                "$<TARGET_FILE:bgt>"
                "$<TARGET_FILE:SDL3-static>"
                "$<TARGET_FILE:SDL3_ttf-static>"
                "$<TARGET_FILE:SDL3_image-static>"
                ${BGT_VENDORED_EXTRA_FILES}
                "$<TARGET_FILE:SDL3_mixer-static>"
        DEPENDS bgt SDL3-static SDL3_ttf-static SDL3_image-static
            ${BGT_VENDORED_EXTRA_TARGETS} SDL3_mixer-static
        COMMENT "Merging libbgt and vendored dependencies"
        VERBATIM)
    add_custom_target(bgt_vendored ALL DEPENDS "${BGT_VENDORED_LIBRARY}")
endif()

if(MSVC)
    target_compile_options(bgt PRIVATE /W4 /utf-8)
else()
    target_compile_options(bgt PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# GBK 分发：为学生工程生成 GBK 版头文件与 Visual Studio 属性表。
#
# 仓库内的源码一律是 UTF-8，但 GBK 模式的学生工程需要一份 GBK 版头文件：MSVC 的
# /source-charset 是“按编译单元”生效的，无法只对某一个 #include 指定编码；而把
# UTF-8 头文件按 GBK 解析会直接产生语法错误（不是仅仅注释乱码）。详见 docs/encoding.md。
# ---------------------------------------------------------------------------
if(BGT_ENCODING_IS_GBK)
    add_executable(bgt_transcode tools/bgt_transcode.cpp src/bgt_encoding.cpp)
    target_include_directories(bgt_transcode PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set_target_properties(bgt_transcode PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    if(MSVC)
        # 工具自身按 UTF-8 编译，并以默认（UTF-8）模式构建，与库的模式无关。
        target_compile_options(bgt_transcode PRIVATE /utf-8)
    endif()

    set(BGT_GBK_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/gbk-include")
    set(BGT_GBK_HEADER "${BGT_GBK_HEADER_DIR}/bgt.h")
    add_custom_command(
        OUTPUT "${BGT_GBK_HEADER}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BGT_GBK_HEADER_DIR}"
        COMMAND bgt_transcode
                "${CMAKE_CURRENT_SOURCE_DIR}/include/bgt.h"
                "${BGT_GBK_HEADER}"
                gbk
                --prepend "#define BGT_SOURCE_ENCODING BGT_ENCODING_GBK"
        DEPENDS bgt_transcode "${CMAKE_CURRENT_SOURCE_DIR}/include/bgt.h"
        COMMENT "Generating the GBK-encoded bgt.h for distribution"
        VERBATIM)
    add_custom_target(bgt_gbk_header ALL DEPENDS "${BGT_GBK_HEADER}")
endif()

if(BGT_BUILD_EXAMPLES)
    function(bgt_add_example target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE libbgt::bgt)
        set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
        bgt_apply_text_encoding(${target})

    endfunction()

    bgt_add_example(bgt_hello examples/01_hello.cpp)
    bgt_add_example(bgt_shapes examples/02_shapes.cpp)
    bgt_add_example(bgt_text examples/03_text.cpp)
    bgt_add_example(bgt_input examples/04_input.cpp)
    bgt_add_example(bgt_transparency examples/05_transparency.cpp)
    bgt_add_example(bgt_sudoku demo/01_sudoku.cpp)
    bgt_add_example(bgt_hanoi demo/02_hanoi.cpp)
    bgt_add_example(bgt_breakout demo/03_breakout.cpp)
    bgt_add_example(bgt_minesweeper demo/04_minesweeper.cpp)
    bgt_add_example(bgt_shooter demo/05_shooter.cpp)
    bgt_add_example(bgt_api_tour examples/06_api_tour.cpp)
    bgt_add_example(bgt_images examples/07_images.cpp)
    bgt_add_example(bgt_random examples/08_random.cpp)
    bgt_add_example(bgt_collision examples/09_collision.cpp)
    bgt_add_example(bgt_storage examples/10_storage.cpp)
    bgt_add_example(bgt_sound examples/11_sound.cpp)
    bgt_add_example(bgt_errors examples/12_errors.cpp)

    add_custom_command(
        TARGET bgt_images
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/07_image.png"
                "$<TARGET_FILE_DIR:bgt_images>/07_image.png"
        VERBATIM)

    foreach(sound_asset jump ding boom melody)
        add_custom_command(
            TARGET bgt_sound
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/examples/11_${sound_asset}.wav"
                    "$<TARGET_FILE_DIR:bgt_sound>/11_${sound_asset}.wav"
            VERBATIM)
    endforeach()

    foreach(breakout_asset bounce brick lose win)
        add_custom_command(
            TARGET bgt_breakout
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/demo/03_${breakout_asset}.wav"
                    "$<TARGET_FILE_DIR:bgt_breakout>/03_${breakout_asset}.wav"
            VERBATIM)
    endforeach()

    foreach(minesweeper_asset mine win)
        add_custom_command(
            TARGET bgt_minesweeper
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/demo/04_${minesweeper_asset}.wav"
                    "$<TARGET_FILE_DIR:bgt_minesweeper>/04_${minesweeper_asset}.wav"
            VERBATIM)
    endforeach()

    foreach(shooter_asset shoot boom lose bgm)
        add_custom_command(
            TARGET bgt_shooter
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/demo/05_${shooter_asset}.wav"
                    "$<TARGET_FILE_DIR:bgt_shooter>/05_${shooter_asset}.wav"
            VERBATIM)
    endforeach()

    foreach(shooter_image ship enemy)
        add_custom_command(
            TARGET bgt_shooter
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/demo/05_${shooter_image}.png"
                    "$<TARGET_FILE_DIR:bgt_shooter>/05_${shooter_image}.png"
            VERBATIM)
    endforeach()

    add_custom_command(
        TARGET bgt_sudoku
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/demo/01_sudoku_puzzle.txt"
                "$<TARGET_FILE_DIR:bgt_sudoku>/01_sudoku_puzzle.txt"
        VERBATIM)
endif()

if(BGT_BUILD_TESTS)
    enable_testing()

    add_executable(bgt_test_random tests/test_random.cpp)
    target_link_libraries(bgt_test_random PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_random PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_random)
    add_test(NAME bgt_test_random COMMAND bgt_test_random)

    add_executable(bgt_test_collision tests/test_collision.cpp)
    target_link_libraries(bgt_test_collision PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_collision PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_collision)
    add_test(NAME bgt_test_collision COMMAND bgt_test_collision)

    add_executable(bgt_test_storage tests/test_storage.cpp)
    target_link_libraries(bgt_test_storage PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_storage PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_storage)
    add_test(NAME bgt_test_storage COMMAND bgt_test_storage)

    add_executable(bgt_test_encoding tests/test_encoding.cpp)
    target_link_libraries(bgt_test_encoding PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_encoding PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_encoding)
    # 该测试直接验证内部转换层，需要能包含 src/bgt_encoding.h。
    target_include_directories(bgt_test_encoding PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME bgt_test_encoding COMMAND bgt_test_encoding)

    add_executable(bgt_test_sound tests/test_sound.cpp)
    target_link_libraries(bgt_test_sound PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_sound PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_sound)
    add_test(NAME bgt_test_sound COMMAND bgt_test_sound
        WORKING_DIRECTORY $<TARGET_FILE_DIR:bgt_test_sound>)

    foreach(sound_asset jump ding boom melody)
        add_custom_command(
            TARGET bgt_test_sound
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/examples/11_${sound_asset}.wav"
                    "$<TARGET_FILE_DIR:bgt_test_sound>/11_${sound_asset}.wav"
            VERBATIM)
    endforeach()

    add_executable(bgt_test_errors tests/test_errors.cpp)
    target_link_libraries(bgt_test_errors PRIVATE libbgt::bgt)
    set_target_properties(bgt_test_errors PROPERTIES CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    bgt_apply_text_encoding(bgt_test_errors)
    add_test(NAME bgt_test_errors COMMAND bgt_test_errors)
endif()

if(BGT_BUILD_VENDORED)
    install(FILES "${BGT_VENDORED_LIBRARY}"
        TYPE LIB)
else()
    install(
        TARGETS bgt
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
if(BGT_ENCODING_IS_GBK)
    # 学生工程按 GBK 读取源码，因此分发的头文件也必须是 GBK（库自身仍用仓库里的
    # UTF-8 头文件编译）。生成的 GBK 头文件第一行就写入了
    # BGT_SOURCE_ENCODING=BGT_ENCODING_GBK，自带构建模式声明：学生工程只要添加
    # include 目录并链接库即可，既不用配字符集选项，也不用在项目属性里定义宏。
    # 其余说明见仓库里的 docs/encoding.md，不进分发物。
    install(FILES "${BGT_GBK_HEADER}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/bgt.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

set(CPACK_GENERATOR ZIP)
# 两种编码模式的二进制包不能混淆，文件名带后缀区分。
set(BGT_PACKAGE_ENCODING_SUFFIX "")
if(BGT_ENCODING_IS_GBK)
    set(BGT_PACKAGE_ENCODING_SUFFIX "-gbk")
endif()
set(CPACK_PACKAGE_FILE_NAME
    "libbgt-${PROJECT_VERSION}-msvc-x64${BGT_PACKAGE_ENCODING_SUFFIX}")
include(CPack)
