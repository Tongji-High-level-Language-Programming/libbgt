# 第三方依赖（SDL3 / SDL3_ttf / SDL3_image / SDL3_mixer）的获取与配置。
#
# 单独占一个目录作用域，是为了把 MSVC 的 /utf-8 限制在 vendored 源码上：
# /utf-8 与 /execution-charset:<代码页> 互斥（MSVC 报 D8016），而 libbgt 在
# BGT_SOURCE_ENCODING=GBK 时要给消费端指定执行字符集，因此项目自身的目标不能
# 继承 /utf-8。
#
# vendored 源码含有非 ASCII 字符，必须按 UTF-8 读取：在中文 Windows（ANSI
# 代码页 936）下按 GBK 解析，轻则 C4819 告警，重则把字节当作双字节字符而串行、
# 产生语法错误。

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
    message(FATAL_ERROR "third_party/SDL is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL_ttf/CMakeLists.txt")
    message(FATAL_ERROR "third_party/SDL_ttf is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/CMakeLists.txt")
    message(FATAL_ERROR "third_party/SDL_image is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer/CMakeLists.txt")
    message(FATAL_ERROR "third_party/SDL_mixer is missing. Run: git submodule update --init --recursive")
endif()

# Build the bundled dependencies only as static libraries. Besides making the
# examples self-contained, the explicit settings prevent SDL3::SDL3 from
# selecting SDL's shared target when both variants are available.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLTTF_TESTS OFF CACHE BOOL "" FORCE)
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_SHARED OFF CACHE BOOL "" FORCE)

# SDL_mixer：只保留内置单文件解码器（WAV / dr_mp3 / drflac /
# stb_vorbis），关闭所有带嵌套子模块的解码器——零递归依赖。
set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)
set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC_LIBFLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)

set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

add_subdirectory(SDL SYSTEM EXCLUDE_FROM_ALL)
add_subdirectory(SDL_ttf SYSTEM EXCLUDE_FROM_ALL)
add_subdirectory(SDL_image SYSTEM EXCLUDE_FROM_ALL)
add_subdirectory(SDL_mixer SYSTEM EXCLUDE_FROM_ALL)
