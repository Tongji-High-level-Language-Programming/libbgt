# libbgt

`libbgt` 是一个面向 C++ 初学者的轻量级 2D 图形库，基于 SDL3 与 SDL3_ttf 构建。

它的目标是在教学早期隐藏 SDL、字体、渲染器、纹理、事件循环和资源生命周期等复杂概念，让学生只使用普通函数、整数、布尔值和字符串字面量，就能完成窗口绘图、中文显示和简单交互程序。

## 设计原则

- 基础接口统一使用 `snake_case`。
- 基础接口统一使用 `bgt_` 前缀。
- 基础接口不要求使用类、对象、结构体或指针。
- 默认只管理一个窗口和一张隐式画布。
- 中文显示开箱可用，默认使用系统中文字体。
- 项目使用 CMake 构建。
- SDL3、SDL3_ttf、SDL3_image 与 SDL3_mixer 通过 Git Submodule 管理。
- SDL3、SDL3_ttf、SDL3_image、SDL3_mixer 及其解码依赖默认静态链接，生成的示例程序无需附带 SDL DLL。
- 首版只提供低层、直观的基础能力，不提供网格、场景、按钮等高阶封装。

## 最小示例

```cpp
#include "bgt.h"

int main()
{
    bgt_open_window(800, 600, "我的第一个图形程序");
    bgt_set_background(BGT_WHITE);

    while (bgt_window_is_open()) {
        bgt_set_color(BGT_BLUE);
        bgt_fill_circle(400, 300, 50);

        bgt_set_color(BGT_BLACK);
        bgt_draw_text(280, 220, "你好，图形世界！", 32);

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}
```

## 快速开始

### 1. 获取源码

SDL3 与 SDL3_ttf 以 Git Submodule 存放在 `third_party/` 下，克隆时一并拉取：

```bash
git clone --recurse-submodules https://github.com/Tongji-High-level-Language-Programming/libbgt.git
cd libbgt
```

如果克隆时没有加 `--recurse-submodules`，已有仓库可以随时补拉子模块：

```bash
git submodule update --init --recursive
```

### 2. 构建

```bash
cmake -S . -B build
cmake --build build
```

默认把 `libbgt` 编译为静态库，并构建全部 14 个示例与演示程序。

### 3. 运行示例

示例程序生成在 `build/` 目录下；使用 Visual Studio 等多配置生成器时位于
`build/<配置>/`（例如 `build/Debug/`）。

```powershell
# Windows PowerShell
.\build\bgt_hello.exe
.\build\bgt_hanoi.exe
```

```bash
# Linux / macOS
./build/bgt_hello
./build/bgt_hanoi
```

先运行 `bgt_hello` 确认环境正常，再运行 `bgt_hanoi`（汉诺塔演示：三态流程、
手动游玩与递归自动求解，配套作业见[作业题库](docs/exercises.md)）。

带配套文件的程序（如 `demo/01_sudoku` 的谜题文本、`examples/11_sound` 的
合成音效）要从可执行文件所在目录运行——构建时这些文件已经复制到同一目录。

## 常用配置项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BGT_BUILD_EXAMPLES` | `ON` | 编译示例程序 |
| `BGT_BUILD_TESTS` | `OFF` | 编译纯函数测试（显式检查，用 ctest 运行） |
| `BGT_BUILD_SHARED` | `OFF` | 编译为共享库（默认静态） |
| `BGT_BUILD_VENDORED` | `OFF` | MSVC 下把库与依赖合并为单个 `bgt_vendored.lib`（见下文） |
| `BGT_USE_SYSTEM_SDL` | `OFF` | 使用系统安装的 SDL3 / SDL3_ttf / SDL3_image / SDL3_mixer 包 |

如改用系统安装的依赖，请确保其同时提供静态 CMake 目标，然后配置
`-DBGT_USE_SYSTEM_SDL=ON`。

## 项目结构

```text
libbgt/
  CMakeLists.txt
  README.md
  docs/
    design.md
    api-v0.md
    exercises.md
    api-v0.2.md
    api-v0.3.md
  include/
    bgt.h
  src/
    bgt.cpp
  examples/
    01_hello.cpp
    02_shapes.cpp
    03_text.cpp
    04_input.cpp
    05_transparency.cpp
    06_api_tour.cpp
    07_images.cpp
    07_image.png
    08_random.cpp
    09_collision.cpp
    10_storage.cpp
    11_sound.cpp
    11_jump.wav
    11_ding.wav
    11_boom.wav
    11_melody.wav
    make_sound_assets.py
    12_errors.cpp
  demo/
    01_sudoku.cpp
    01_sudoku_puzzle.txt
    02_hanoi.cpp
    03_breakout.cpp
    03_bounce.wav
    03_brick.wav
    03_lose.wav
    03_win.wav
    04_minesweeper.cpp
    04_mine.wav
    04_win.wav
    05_shooter.cpp
    05_shoot.wav
    05_boom.wav
    05_lose.wav
    05_bgm.wav
    05_ship.png
    05_enemy.png
    make_game_assets.py
  tests/
    test_random.cpp
    test_collision.cpp
    test_storage.cpp
    test_sound.cpp
    test_errors.cpp
  third_party/
    SDL/
    SDL_ttf/
    SDL_image/
    SDL_mixer/
```

当前仓库包含首版基础 API 实现、CMake 构建脚本、12 个功能示例（`examples/`）
与 5 个完整游戏演示（`demo/`）。文本绘制默认
使用系统自带的中文字体（Windows 下通常是微软雅黑），不依赖仓库内的字体文件；
系统缺少中文字体时，可以用 `bgt_set_font()` 指定可用字体。

## 依赖管理

SDL3、SDL3_ttf、SDL3_image 与 SDL3_mixer 通过 Git Submodule 管理，使用者克隆后初始化
子模块即可（命令见[快速开始](#快速开始)）。

## MSVC 二进制分发

面向 Visual Studio 使用者时，可以把 libbgt、SDL3、SDL3_ttf、SDL3_image、
SDL3_mixer 及其依赖的静态库合并为一个 `bgt_vendored.lib`。使用者不需要复制或链接 SDL DLL：

```powershell
cmake -S . -B build-dist -G "Visual Studio 18 2026" -A x64 `
  -DBGT_BUILD_VENDORED=ON -DBGT_BUILD_EXAMPLES=OFF
cmake --build build-dist --config Release
cmake --install build-dist --config Release --prefix package
```

安装目录包含：

```text
package/
  include/bgt.h
  lib/bgt_vendored.lib
```

使用者在 Visual Studio 中添加 `include` 目录、`lib` 目录和
`bgt_vendored.lib` 即可。头文件会为 MSVC 自动声明 SDL 所需的 Windows 系统
库，因此无需逐项配置 SDL 的传递依赖。文本绘制默认使用系统自带的中文字体，
无需附带字体文件。

也可以在构建目录运行 `cpack -C Release`，直接生成同样内容的 ZIP 包。

## 文档

- [设计文档](docs/design.md)
- [首版 API 文档](docs/api-v0.md)
- [作业题库（汉诺塔/打砖块/扫雷/太空射击）](docs/exercises.md)
- [v0.2 API 文档（图片、随机数、碰撞检测）](docs/api-v0.2.md)
- [v0.3 API 文档（文件存档、声音播放、错误诊断）](docs/api-v0.3.md)

## 首版范围

首版 `v0.1` 目标是提供：

- 窗口创建、关闭与刷新。
- 基本图形绘制。
- UTF-8 中文文本绘制。
- 键盘输入。
- 鼠标输入。
- 时间与帧率辅助。
- 基础错误信息。

首版入口文件：

- `include/bgt.h`
- `src/bgt.cpp`
- `examples/` 与 `demo/` 下的 8 个首批程序（`examples/` 01–06 与 `demo/` 01–02）

首版暂不提供：

- 图片加载。
- 音频。
- 网格工具。
- UI 控件。
- 场景管理。
- Tilemap。
- 精灵系统。
- 多窗口。
- 面向对象封装。

## 教学定位

`libbgt` 不试图成为完整游戏引擎。它是一个教学用图形入口，重点是让学生尽早看到图形反馈，并把更高层次的封装留作后续课程练习。
