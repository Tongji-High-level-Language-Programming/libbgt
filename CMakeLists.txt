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
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
endfunction()

add_library(bgt ${BGT_LIBRARY_TYPE} src/bgt.cpp)
add_library(libbgt::bgt ALIAS bgt)

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
    add_custom_command(
        OUTPUT "${BGT_VENDORED_LIBRARY}"
        COMMAND "${CMAKE_AR}" /NOLOGO "/OUT:${BGT_VENDORED_LIBRARY}"
                "$<TARGET_FILE:bgt>"
                "$<TARGET_FILE:SDL3-static>"
                "$<TARGET_FILE:SDL3_ttf-static>"
                "$<TARGET_FILE:SDL3_image-static>"
                "$<TARGET_FILE:SDL3_mixer-static>"
        DEPENDS bgt SDL3-static SDL3_ttf-static SDL3_image-static
            SDL3_mixer-static
        COMMENT "Merging libbgt and vendored dependencies"
        VERBATIM)
    add_custom_target(bgt_vendored ALL DEPENDS "${BGT_VENDORED_LIBRARY}")
endif()

if(MSVC)
    # 库自身的源码与中文注释都是 UTF-8，与 BGT_SOURCE_ENCODING（用户代码的运行期
    # 编码）无关，因此始终按 UTF-8 编译。
    target_compile_options(bgt PRIVATE /W4 /utf-8)
else()
    target_compile_options(bgt PRIVATE -Wall -Wextra -Wpedantic)
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
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/bgt.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME
    "libbgt-${PROJECT_VERSION}-msvc-x64")
include(CPack)
