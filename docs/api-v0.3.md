# libbgt v0.3 API 文档（文件存档、声音播放、错误诊断）

本文档描述 `libbgt` v0.3 新增的三块接口：文件存档（把游戏数据保存成
记事本能直接打开的文本文件）、声音播放（音效和背景音乐）与错误诊断
（错误历史与按序号查询）。存档见 §1–§6，声音见 §7–§9，错误诊断见
§10–§12。

v0.1 的基础接口见 [api-v0.md](api-v0.md)，v0.2 的图片、随机数与碰撞检测
接口见 [api-v0.2.md](api-v0.2.md)。存档完整示例见
`examples/10_storage.cpp`，声音完整示例见 `examples/11_sound.cpp`，错误
诊断完整示例见 `examples/12_errors.cpp`。

## 1. 存档模型：节-键-值

内存里有一张存档表：**节名 → 键名 → 值**。

```cpp
bgt_set_int("最高分", "best", 120);      // 往 [最高分] 节里放一个键
bgt_save("save.txt");                    // 整表写进文件
```

```cpp
bgt_load("save.txt");                    // 文件整表读进内存
int best = bgt_get_int("最高分", "best", 0);   // 取不出来时给默认值
```

三个动词分工：`bgt_load`/`bgt_save` 负责**文件**，`bgt_set_*`/`bgt_get_*`
负责**内存表**。存档一词的本义就是"收集状态，写盘；下次运行，读回"。

- `bgt_load("save.txt")` 文件不存在 → 空表、不报错（第一次运行的常态）。
- `bgt_get_*` 节或键不存在 → 返回默认值，不报错。默认值是 API 的一部分。
- `bgt_set_*` 同一个 (节, 键) 再写一次就覆盖（跨类型也覆盖）。

## 2. 函数清单

### 2.1 文件

| 函数 | 说明 |
|---|---|
| `bool bgt_load(const char filename[])` | 把存档文件整表读进内存（替换原内存表） |
| `bool bgt_save(const char filename[])` | 把内存表整表写进文件（先写临时文件再替换，防写坏） |
| `bool bgt_file_exists(const char filename[])` | 判断文件是否存在，常用于首次运行检测 |

### 2.2 写入（内存表）

| 函数 | 说明 |
|---|---|
| `void bgt_set_int(const char section[], const char key[], int value)` | 存整数 |
| `void bgt_set_double(const char section[], const char key[], double value)` | 存小数 |
| `void bgt_set_string(const char section[], const char key[], const char value[])` | 存字符串（首尾空白会被去掉；去掉首尾空白后仍含换行会记错不存入） |

### 2.3 读取（内存表）

| 函数 | 说明 |
|---|---|
| `int bgt_get_int(const char section[], const char key[], int default_value)` | 读整数 |
| `double bgt_get_double(const char section[], const char key[], double default_value)` | 读小数 |
| `void bgt_get_string(const char section[], const char key[], char out[], int out_size, const char default_value[])` | 读字符串进 `out` 数组 |

`bgt_get_string` 的用法：

```cpp
char name[32];
bgt_get_string("玩家", "name", name, 32, "无名");   // 没有这个键时是 "无名"
```

字符串放不进 `out_size` 时只写入放得下的部分（保证不切半个中文）并记录
错误——看到错误就把数组开大。

## 3. 类型转换

存档表里存的是值的**文本**。读取时按类型解析：

| 存入的值 | 按 int 读 | 按 double 读 | 按字符串读 |
|---|---|---|---|
| `100`（整数） | `100` | `100.0` ✅ 整数可以当小数用 | `"100"` |
| `45.5`（小数） | ❌ 默认值+错误 | `45.5` | `"45.5"` |
| `张三`（字符串） | ❌ 默认值+错误 | ❌ 默认值+错误 | `张三` |

窄方向读不出来（给默认值并记录错误）；宽方向自然成立，与 C++ 里
"整数能赋给 double"的直觉一致。

## 4. 存档文件格式

`bgt_save("save.txt")` 写出的是纯文本（UTF-8，行尾 `\n`）：

```ini
[最高分]
best=120

[玩家]
name=张三

[盘面]
board=53..7....
```

- 每行 `键=值`；`[节名]` 单独一行；空行分隔各节；`#` 开头的整行是注释
  （库不写注释，但读的时候会跳过——学生可以在自己存档里写备注）。
- 节和键都按字典序输出，同一文件每次保存结果完全一样。
- 小数用最短表示：`45.5` 存 `45.5`，`0.1` 存 `0.1`，读回精确不丢。
- 手改时可用 `键 = 值` 带空格（读取时自动去掉首尾空白）；行尾是
  Windows 记事本的 `\r\n` 也没问题；记事本另存为 UTF-8 带的文件头
  标记（BOM）也能读。
- 格式不对的行会被跳过并记录错误，其他行照常读入——改坏了能发现，
  但整个存档不会因此作废。
- 节名不能含 `[`、`]`；键名不能含 `=`，且**不能以 `#` 或 `[` 开头**（这样的键写进文件后会读不回来）；字符串值去掉首尾空白后仍不能含换行。

## 5. 错误处理

存档错误统一使用错误码 `BGT_ERROR_STORAGE`，可用 `bgt_has_error()` /
`bgt_error_code()` / `bgt_print_error()` 查询（用法与 v0.1 相同）：

| 场景 | 行为 |
|---|---|
| `bgt_load` 文件不存在 | 空表，**无错误**（首次运行常态） |
| `bgt_load` 文件存在但读不开（被其他程序占用等） | 记错误，返回 `false` |
| `bgt_load` 有格式不对的行 | 跳过该行，其余照常，记一次错误，返回 `false` |
| `bgt_get_*` 节或键不存在 | 返回默认值，**无错误** |
| 按窄类型读（小数按 int 读等） | 返回默认值 + 记错误 |
| `bgt_get_string` 放不下 | 截断到完整字符 + 记错误 |
| `bgt_set_*` 节名/键名不合法 | 记错误，不存入 |
| `bgt_save` 写失败 | 返回 `false` + 记错误 |

## 6. 教学建议

- **先手写存档，再写程序读它**：存档格式就是文本 INI——可以让学生在
  记事本里手写一个存档（比如 `[玩家] hp=80`），再写程序 `bgt_load` +
  `bgt_get_int` 读出来，直观建立"文件-内存表"的映射。
- **对拍练习**：可以让学生先用 `bgt_file_exists` + 屏幕提示实现
  "第一次运行向导"，再对比自己手写的 if/else 版本。
- **错误是可见的**：手改存档改坏一行，程序里 `bgt_load` 返回 false、
  `bgt_print_error` 能打印原因——这是讲"数据校验"的好素材。

## 7. 声音模型：音效和背景音乐

声音分两类，用法完全不同：

- **音效**：短促、可重叠。比如跳跃、射击、得分。先加载成编号
  （`bgt_load_sound`），再随时触发（`bgt_play_sound`）；连续触发时多个
  声音自动混在一起。
- **背景音乐**：长、循环、全局只有一首。直接 `bgt_play_music("bgm.wav")`
  一步起播，边读文件边播放（流式），放一整首长曲子也不会占很多内存；
  播放中再调用会自动切到新曲子。

```cpp
int jump = bgt_load_sound("jump.wav");   // 开头加载一次
bgt_play_sound(jump);                    // 每次跳跃时触发
bgt_play_music("bgm.wav");               // 进入主循环前起播
```

支持的文件格式：WAV、MP3、OGG 等常见格式（由随仓库提供的 SDL_mixer 子模块解码，
默认静态链接进示例程序，运行时不需要附带 DLL）。

## 8. 声音函数清单

| 函数 | 说明 |
|---|---|
| `int bgt_load_sound(const char filename[])` | 加载音效，返回编号；失败返回 0 并记录错误 |
| `void bgt_play_sound(int id)` | 播放一次；连按连响、自动混音；无效编号记错误 |
| `void bgt_set_sound_volume(int id, int volume)` | 按 ID 设音量 0–100；影响之后的播放 |
| `bool bgt_play_music(const char filename[])` | 起播（默认无限循环）；正在播放则自动切歌；成功返回 true，失败返回 false 并记录错误 |
| `void bgt_stop_music()` | 停止；没有音乐在播时是安全空操作 |
| `void bgt_set_music_volume(int volume)` | 音乐音量 0–100，立即生效 |

- **音量**：0–100 的整数，0 静音、100 最大，超出范围会被收到边界。
  音效音量按 ID 各自记忆（把脚步声调轻、把胜利声调响）；改音量影响
  **之后**的播放，正在响的不变。音乐音量即时生效。
- **资源生命周期**：音效和音乐都不需要（也没有）释放函数——
  `bgt_close_window()` 和程序退出时统一清理，与图片同款约定。
- **声音相关错误**统一用错误码 `BGT_ERROR_AUDIO`（`bgt_has_error()` /
  `bgt_print_error()` / `bgt_draw_error()` 的用法与 v0.1 相同）：文件不
  存在、格式不支持、无效编号会记录错误；机器没有声卡时程序也不会崩，
  所有声音调用变成安全空操作。

## 9. 声音教学建议

- **先听再说**：把 `11_jump.wav` 换成学生自己录的 wav（手机就能录），
  两行代码就能让程序发出“自己的声音”——正反馈立竿见影。
- **音效与音乐分开讲**：先讲音效（编号、触发、重叠），再讲音乐（流式、
  循环、单实例）——两类的心智模型完全不同，混在一起讲容易乱。
- **错误是可见的**：播放不存在的文件、用没加载过的编号，
  `bgt_draw_error` 都能画出原因——适合讲“失败路径也要处理”。

## 10. 错误历史模型

从 v0.3 起，库把最近发生的错误记成一份编号列表：

- 最多保留 **10 条**：更早的错误会被最老的挤出。
- 序号 **0 是最老的一条**，`bgt_error_count() - 1` 是最新的一条。
- `bgt_clear_error()` 清空整份历史。
- 错误码沿用 v0.1 的 `BGT_ERROR_*` 常量，没有新增。

三条查询铁律（实现与文档都以此为准）：

1. **查询不产生新错误**：`bgt_error_count()`、`bgt_error_code()`、
   `bgt_error_text()`、`bgt_has_error()` 任何情况都不会往历史里新增
   条目——查历史这个动作本身永远干净。
2. **越界静默**：序号越界（含负数、含历史为空）时，`bgt_error_code()`
   返回 0，`bgt_error_text()` 得到空串，`bgt_print_error()` 和
   `bgt_draw_error()` 什么都不做。
3. **截断静默**：`bgt_error_text()` 的数组放不下时按 UTF-8 字符边界
   安全截断（不会切在半个汉字中间），不记错误。

## 11. 函数清单

| 函数 | 说明 |
|---|---|
| `bool bgt_has_error()` | 历史里有至少一条错误时返回 true |
| `int bgt_error_count()` | 历史条数（0 到 10） |
| `int bgt_error_code(int index)` | 按序号取错误码；越界返回 `BGT_ERROR_NONE` |
| `int bgt_error_code()` | 最新一条的错误码；等价于 `bgt_error_code(bgt_error_count() - 1)` |
| `void bgt_error_text(int index, char out[], int out_size)` | 按序号把消息文本复制进 out 数组；放不下时按 UTF-8 边界截断 |
| `void bgt_print_error(int index)` | 按序号打印到控制台：`libbgt error 码: 消息` |
| `void bgt_print_error()` | 打印最新一条；v0.1 的无参用法保持不变 |
| `void bgt_draw_error(int x, int y, int size, int index)` | 按序号绘制；消息太长自动按窗口宽度换行，逐行向下画 |
| `void bgt_draw_error(int x, int y, int size)` | 绘制最新一条；v0.1 的无参用法保持不变 |
| `void bgt_clear_error()` | 清空整份历史 |

无参的三个形态（`bgt_error_code()`、`bgt_print_error()`、
`bgt_draw_error(x, y, size)`）是 v0.1 的老用法，内部就是取最新一条，
老程序不用改。

## 12. 教学建议

- **画整份历史就是三行循环**：

```cpp
for (int i = 0; i < bgt_error_count(); i = i + 1) {
    bgt_draw_error(40, 100 + i * 48, 16, i);
}
```

- 学生的游戏可以在角落常驻这样一个小面板，运行中出了什么错一眼可见；
  想按错误类型做处理，用 `bgt_error_code(i)` 循环统计。
- `bgt_error_text()` 把文本取进自己的数组后想怎么显示都可以——适合做
  "错误详情页"这类练习。
- 手改存档（见 §4）会产生连串读档错误时，历史列表正好用来观察"每一行
  坏行都被记了一条"。
