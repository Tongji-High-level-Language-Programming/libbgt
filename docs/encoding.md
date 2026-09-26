# libbgt 编码支持（UTF-8 / GBK）

本文说明 libbgt 的两种窄字符串编码模式、库内部的转换边界，以及学生工程与构建
系统的接入方式。

## 1. 背景

SDL3、SDL3_ttf、SDL3_mixer 的字符串参数与返回值一律是 UTF-8：窗口标题、文本
绘制、文本测量、文件路径、错误文本都是如此。而课程要求学生的源码与运行期窄
字符串保持 GBK（Windows 代码页 936）：学生的编辑器、`printf` 输出、存档文件、
`strlen("中文")` 的字节数都按 GBK 理解。

libbgt 的做法是：**公开 API 只说“源编码”，库在真正跨越 SDL 边界的地方做
双向转换**。学生代码不需要任何额外调用。

## 2. 两种模式

| | `BGT_SOURCE_ENCODING=UTF-8`（默认） | `BGT_SOURCE_ENCODING=GBK` |
|---|---|---|
| 学生源码编码 | UTF-8 | GBK（代码页 936） |
| 运行期窄字符串 | UTF-8 | GBK |
| `strlen("中文")` | 6 | 4 |
| `printf` / 标准错误输出 | UTF-8 字节 | GBK 字节（与 CP936 控制台一致） |
| 存档文件内容 | UTF-8 | GBK |
| 库内是否转换 | 否（编译期消除，零开销） | 是（GBK <-> UTF-8） |
| 是否要求 Windows | 否 | 当前是（WinAPI 后端） |

UTF-8 模式是默认值，行为与引入编码支持之前完全一致：所有转换都被
`if constexpr` 消除，不产生额外函数调用或内存分配。

## 3. 库内部分层

```text
学生代码（源编码：UTF-8 或 GBK）
        |
        |  公开 API：入参与出参都是源编码
        v
   ┌─────────────────────────────────────────────┐
   │ libbgt 内部：所有 std::string 都是源编码      │
   │ 存档内存表 / 错误历史 / printf / 字体路径     │
   └─────────────────────────────────────────────┘
        |
        |  src/bgt_encoding.{h,cpp}：只在边界转换
        |  Utf8View（源编码 -> UTF-8，UTF-8 模式下零拷贝）
        |  from_utf8（UTF-8 -> 源编码）
        |  to_native_path（源编码 -> 本机文件系统路径）
        |  encoded_prefix_length / encoded_contains_ascii
        v
SDL3 / SDL3_ttf / SDL3_mixer / 文件系统（UTF-8 或 UTF-16）
```

保持“库内只有一种编码”有一个重要好处：`bgt_set_string("玩家", "name", "张三")`
存进去的字节、`bgt_get_string` 取出来的字节、`bgt_print_error` 打印的字节、存档
文件里的字节，与学生在自己的 `char[]` 里看到的字节完全一致，不会出现“一半 GBK
一半 UTF-8”的中间态。

## 4. 跨越 SDL 边界的转换点

入向（源编码 -> UTF-8）：

| 位置 | 内容 |
|---|---|
| `SDL_CreateWindow` / `SDL_SetWindowTitle` | 窗口标题 |
| `TTF_OpenFont` | 字体路径（含 `bgt_set_font` 传入的路径） |
| `TTF_RenderText_Blended` | `bgt_draw_text` 的文本 |
| `TTF_GetStringSize` / `TTF_MeasureString` | 文本测量（含错误消息自动换行） |
| `IMG_Load` | 图片路径 |
| `MIX_LoadAudio` / `SDL_IOFromFile` | 声音、音乐路径 |
| `SDL_IOFromFile`（`file_exists`） | `bgt_file_exists` 与默认字体探测 |

出向（UTF-8 -> 源编码）：

| 位置 | 内容 |
|---|---|
| `SDL_GetError()` | 拼进错误历史之前转换，保证错误历史只有源编码 |

不转换的地方（有意为之）：

- `SDL_getenv("WINDIR")`：Windows 上返回 ANSI 字符串，正好与源编码一致；
- `std::error_code::message()`：Windows 上同样是 ANSI；
- 存档文件内容、`fprintf(stderr, ...)`：按定义就是源编码。

## 5. 构建选项与编译器字符集

```bash
# 默认：UTF-8
cmake -S . -B build

# 课程使用：GBK
cmake -S . -B build-gbk -DBGT_SOURCE_ENCODING=GBK
cmake --build build-gbk --config Release
```

CMake 会按目标下发字符集选项（**不能**用顶层 `add_compile_options`：MSVC 的
`/utf-8` 与 `/execution-charset:<代码页>` 互斥，会报 D8016）：

| 目标 | UTF-8 模式 | GBK 模式 |
|---|---|---|
| `bgt` 库自身 | `/utf-8` | `/utf-8`（库源码与注释始终是 UTF-8） |
| 仓库内示例 / 演示 / 测试 | `/utf-8` | `/source-charset:utf-8 /execution-charset:.936` |
| 第三方 SDL（`third_party/`） | `/utf-8` | `/utf-8` |

也就是说，GBK 模式下仓库内的 UTF-8 示例源码被声明为“UTF-8 源字符集”，再由
编译器把窄字符串字面量转成 GBK 执行字符集——运行期编码与学生的 GBK 源码一致。

GCC / Clang 对应 `-finput-charset=UTF-8 -fexec-charset=GBK`。

## 6. 学生工程接入（Visual Studio + 单库分发）

GBK 模式下 `cmake --install` 或 `cpack` 产出的包：

```text
package/
  include/bgt.h          # GBK 编码（与学生的源码编码一致）
  lib/bgt_vendored.lib   # 合并了 SDL3 系列的单个静态库
  libbgt-gbk.props       # Visual Studio 属性表
  libbgt-encoding.md     # 本文档
```

接入步骤：

1. 工程属性里添加 `include` 目录、`lib` 目录，并链接 `bgt_vendored.lib`；
2. 属性管理器（View - Other Windows - Property Manager）右键工程，选择
   “添加现有属性表”，选中 `libbgt-gbk.props`。

属性表做两件事：给所有 `.cpp` 加 `/source-charset:.936 /execution-charset:.936`，
并定义 `BGT_SOURCE_ENCODING=BGT_ENCODING_GBK`。等价的命令行写法：

```text
cl /source-charset:.936 /execution-charset:.936 /I include main.cpp lib\bgt_vendored.lib
```

注意：

- 不要同时使用 `/utf-8`（与 `/execution-charset` 互斥，MSVC 报 D8016）；
- `.936` 与 `utf-8` 是两种不同的写法：代码页要写成带点的数字（`.936`），
  UTF-8 要写成名字（`utf-8`）。`/execution-charset:.utf-8` 这种写法会被 MSVC
  静默忽略，因此头文件里有编译期检查兜底；
- 头文件必须是 GBK 版。UTF-8 头文件被按 GBK 解析时会直接产生语法错误，所以
  GBK 模式的安装/打包会自动生成 GBK 版头文件（仓库里的 UTF-8 头文件供库自身
  与 UTF-8 模式使用）。

## 7. 行为约定

- **存档文件**：`bgt_save` 按源编码写出（GBK 模式下即 GBK），`bgt_load` 按源编码
  读入。文件以 UTF-8 BOM 开头时按 UTF-8 解释，读进来后转回源编码——这样学生用
  新版记事本另存为“UTF-8”的存档也能读。
- **中文文件名**：两种模式都先用 `to_native_path` 转成 UTF-16 再访问文件系统，
  因此中文路径在 UTF-8 模式下同样可用（此前会因窄路径按 ANSI 解释而失败）。
- **错误文本**：`bgt_error_text`、`bgt_print_error`、`bgt_draw_error` 给出的都是
  源编码字节；`bgt_error_text` 截断时按源编码的字符边界，不会把汉字切一半。
- **节名 / 键名校验**：GBK 双字节字符的尾字节可以是 `[` 与 `]`，因此名称校验
  按字符边界搜索，而不是直接 `find()`。
- **文本截断**：`bgt_get_string` 的 `out_size` 不足时按源编码字符边界截断；
  同一个 `out_size` 在两种模式下的可见字符数可能不同（GBK 一个汉字 2 字节，
  UTF-8 3 字节）。

## 8. 防呆机制

| 时机 | 机制 | 触发时的现象 |
|---|---|---|
| CMake 配置 | 选项取值校验、非 Windows 上使用 GBK 直接报错 | `FATAL_ERROR` |
| 编译 | 头文件里的 `static_assert(sizeof("中") - 1 == 2)` | 明确的编译错误（含需要加的选项） |
| 链接（MSVC） | `#pragma detect_mismatch` | `LNK2038`：头文件与库的编码模式不一致 |

有了三层防呆，配错字符集不会表现为“能跑但中文是乱码”，而是直接构建失败。

需要在特殊场景下关闭检查（例如把 libbgt 头文件放进预编译头）时，可以定义
`BGT_DISABLE_ENCODING_CHECK`。

## 9. 已知限制

- **GBK 模式目前只在 Windows 上实现**：转换层用 WinAPI（`MultiByteToWideChar` /
  `WideCharToMultiByte`）。`src/bgt_encoding.cpp` 预留了 iconv 后端的位置，
  实现后在 `src/bgt_encoding.h` 里定义 `BGT_HAVE_GBK_CODEC` 即可，调用方无需改动。
- **GBK 表示不了的字符**：GBK 模式下，GBK 无法表示的字符（如 emoji、部分数学
  符号）在交给 SDL 之前会被替换为 `?`；`bgt_transcode` 在生成分发物时使用严格
  转换，遇到无法表示的字符会带着行号直接失败，避免悄悄发出残缺文件。确实需要
  更大字符集时，可以把转换层的代码页换成 GB18030（Windows 代码页 54936）。
- **Windows 的“最佳拟合”**：某些字符在 CP936 下会被静默替换成形状相近的字符
  （例如 U+00B2 变成 `2`）。库内的宽松转换接受这种行为，`bgt_transcode` 的严格
  模式会拒绝它。
- **`BGT_SOURCE_ENCODING` 描述的是运行期编码**，不是文件编码：仓库内的源码始终
  是 UTF-8 文件，由编译选项决定它们的运行期编码。

## 10. 如何验证

```bash
# UTF-8 模式
cmake -S . -B build -DBGT_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release

# GBK 模式
cmake -S . -B build-gbk -DBGT_BUILD_TESTS=ON -DBGT_SOURCE_ENCODING=GBK
cmake --build build-gbk --config Release
ctest --test-dir build-gbk -C Release

# GBK 学生包（GBK 头文件 + 单库 + 属性表）
cmake -S . -B build-dist -G "Visual Studio 18 2026" -A x64 \
  -DBGT_SOURCE_ENCODING=GBK -DBGT_BUILD_VENDORED=ON -DBGT_BUILD_EXAMPLES=OFF
cmake --build build-dist --config Release
cmake --install build-dist --config Release --prefix package-gbk
```

`tests/test_encoding.cpp` 覆盖 GBK 与 UTF-8 互转、严格/宽松转换、字符边界截断、
GBK 尾字节陷阱、中文路径与存档 BOM 互操作；`tests/test_storage.cpp` 的断言在两种
模式下都成立（顺序与截断的期望值按当前编码现场计算，不写死字节）。
