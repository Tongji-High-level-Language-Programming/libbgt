# libbgt v0.2 API 文档（图片、随机数、碰撞检测）

本文档描述 `libbgt` v0.2 新增的图片、随机数与碰撞检测接口：图片部分从
文件加载图片，并通过设置每张图片的 RST 变换状态（RST = 平移-旋转-缩放）
来控制它在画面上的样子；随机数部分提供可播种的随机整数与随机浮点数；
碰撞检测部分回答"这两个形状碰上了吗""鼠标点中它了吗"这类游戏中最
常见的问题。
所有函数沿用 v0.1 的基本约定（`bgt_` 前缀、`snake_case`、整数坐标）。

v0.1 的基础接口（窗口、图形、文本、输入、时间、错误信息）见
[api-v0.md](api-v0.md)。

图片解码由随仓库提供的 SDL3_image 子模块完成，默认静态链接进示例程序，
运行时不需要附带额外的 DLL。图片完整示例见 `examples/07_images.cpp`，随机数完整示例见
`examples/08_random.cpp`，碰撞检测完整示例见 `examples/09_collision.cpp`。

## 1. RST 变换模型

libbgt 的图片变换遵循图形学中经典的 RST 模型。每张图片有三个独立的
变换状态，绘制时自动合成：

```text
M = T × R × S
    │   │   │
    │   │   └─ 缩放：bgt_scale_image(id, sx, sy)
    │   └─ 旋转：bgt_rotate_image(id, angle)
    └─ 平移：bgt_translate_image(id, dx, dy) + 绘制位置 (x, y)
```

变换按 **先缩放（S），再旋转（R），最后平移（T）** 的固定顺序合成，
全部围绕图片的**局部原点**（左上角）进行。

三个核心事实：

1. **旋转和缩放围绕左上角**——变换后左上角位置不变，图片绕它伸缩转动。
2. **翻转就是负缩放**——`S(-1, 1)` 左右翻转，`S(1, -1)` 上下翻转。
   没有独立的翻转函数。
3. **变换是图片的属性**——设置一次持续生效，直到再次修改或清除。
   这与 `bgt_set_color()` 的工作方式相同。

翻转时图片会翻到原点的**另一侧**：左右翻转后图片出现在原点左侧，
上下翻转后图片出现在原点上方。这是严格 RST 的正确行为——局部原点始终
不动，变的是图片"身体"相对于原点的方向。

## 2. 基本约定

图片在 libbgt 中用"图片编号"表示，学生不需要理解纹理或内存管理：

- 编号是一个 `int`，由 `bgt_load_image()` 返回，大于 0 表示有效图片。
- `BGT_IMAGE_NONE`（0）表示"没有图片"，例如加载失败时的返回值。
- 支持的图片格式：PNG、JPG、BMP。
- 图片文件路径按 UTF-8 处理，可以直接使用中文文件名。
- 图片由库统一管理，程序结束时自动释放；没有（也不需要）手动释放的函数。
- 变换状态（T、R、S）也是图片的一部分：每个编号独立记忆自己的变换，
  互不影响。

## 3. 图片函数

```cpp
int bgt_load_image(const char filename[]);

int bgt_image_width(int image_id);
int bgt_image_height(int image_id);

void bgt_translate_image(int image_id, int dx, int dy);
void bgt_rotate_image(int image_id, double angle);
void bgt_scale_image(int image_id, double sx, double sy);
void bgt_clear_image_transform(int image_id);

void bgt_draw_image(int image_id, int x, int y);
```

### `bgt_load_image`

从文件加载一张图片，返回它的图片编号。

```cpp
int bgt_load_image(const char filename[]);
```

参数：

- `filename`：图片文件路径，按 UTF-8 处理。

返回：

- 成功返回一个大于 0 的图片编号。
- 失败返回 `BGT_IMAGE_NONE`，可用 `bgt_print_error()` 查看原因。

说明：

- 支持 PNG、JPG、BMP 三种常见格式。
- 窗口尚未打开时调用会失败并记录 `BGT_ERROR_NOT_OPEN`。
- 文件名为空、文件不存在或解码失败时记录 `BGT_ERROR_IMAGE`。
- 应在主循环外加载图片，不要每帧调用（见第 5 节）。
- 同一文件多次加载会得到多个互相独立的编号（包括各自的变换状态）。
- 图片由库统一管理，程序结束时自动释放，没有手动释放的函数。
- 关闭窗口后所有编号失效，重新打开窗口后需要重新加载。
- 新加载的图片变换状态为恒等（无旋转、无缩放、无偏移）。

示例：

```cpp
int hero = bgt_load_image("hero.png");

if (hero == BGT_IMAGE_NONE) {
    bgt_print_error();
    return 1;
}
```

### `bgt_image_width` / `bgt_image_height`

返回图片的原始尺寸，单位是像素。

```cpp
int bgt_image_width(int image_id);
int bgt_image_height(int image_id);
```

返回：

- 图片的原始宽度或原始高度。
- 编号无效时返回 0，不记录错误。

说明：

- 返回值是图片文件自带的像素尺寸，不受缩放状态影响。
- 想知道缩放后的显示尺寸：`bgt_image_width(hero) * 2`（放大 2 倍时）。

示例：

```cpp
int x = (bgt_window_width() - bgt_image_width(hero)) / 2;
int y = (bgt_window_height() - bgt_image_height(hero)) / 2;
bgt_draw_image(hero, x, y);
```

### `bgt_translate_image`（T：平移）

设置图片相对绘制位置的偏移量。

```cpp
void bgt_translate_image(int image_id, int dx, int dy);
```

参数：

- `image_id`：图片编号。
- `dx`、`dy`：相对绘制位置的偏移量。

说明：

- 实际平移位置 = `bgt_draw_image` 的 `(x, y)` + 这里的 `(dx, dy)`。
- 图片的局部原点（左上角）最终落在 `(x + dx, y + dy)`。
- 默认 `(0, 0)`（无偏移）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。
- 适合做弹跳、后坐力等"逻辑位置不变、视觉位置微调"的动画。

示例：

```cpp
// 弹跳动画：逻辑位置 (400, 300) 不变，图片上下浮动
double t = bgt_total_time() * 3.0;
int bounce = static_cast<int>(std::sin(t) * 20.0);
bgt_translate_image(hero, 0, bounce);
bgt_draw_image(hero, 400, 300);
```

### `bgt_rotate_image`（R：旋转）

设置图片的旋转角度。

```cpp
void bgt_rotate_image(int image_id, double angle);
```

参数：

- `image_id`：图片编号。
- `angle`：旋转角度，单位是度，正值表示顺时针。

说明：

- 旋转围绕图片的局部原点（左上角）进行——旋转后左上角位置不变，
  图片整体绕它转动。
- 默认 0（不旋转）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。
- 想让图片"绕自己中心转"需要平移补偿，属于后续课程的话题。

示例：

```cpp
// 风车：每秒转 90 度
bgt_rotate_image(pinhweel, bgt_total_time() * 90.0);
bgt_draw_image(pinhweel, 400, 200);
```

### `bgt_scale_image`（S：缩放）

设置图片的缩放倍率。负值表示翻转。

```cpp
void bgt_scale_image(int image_id, double sx, double sy);
```

参数：

- `image_id`：图片编号。
- `sx`：横向缩放因子。`1.0` 原始大小、`2.0` 放大一倍、`0.5` 缩小一半。
- `sy`：纵向缩放因子，含义同上。

说明：

- 缩放围绕图片的局部原点（左上角）——缩放后左上角位置不变，
  图片向右下方向伸缩。
- **负值表示翻转**：`sx < 0` 时左右翻转（图片翻到原点左侧），
  `sy < 0` 时上下翻转（图片翻到原点上方）。
- `S(-1, 1)` 左右翻转、`S(1, -1)` 上下翻转、`S(-1, -1)` 等价于旋转
  180 度。
- 翻转后图片的实际显示位置会移动到原点的另一侧，这是严格 RST 的
  正确行为——局部原点不动，图片的身体翻到另一边。
- 默认 `(1.0, 1.0)`（原始大小）。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_scale_image(hero, 2.0, 2.0);      // 放大一倍
bgt_draw_image(hero, 100, 100);

bgt_scale_image(hero, -1.0, 1.0);     // 左右翻转（翻到原点左侧）
bgt_draw_image(hero, 100, 100);

bgt_scale_image(hero, 1.0, 1.0);      // 恢复原始大小
```

### `bgt_clear_image_transform`

清除图片的全部变换状态，恢复为恒等变换。

```cpp
void bgt_clear_image_transform(int image_id);
```

说明：

- 一次调用等同 `bgt_translate_image(id, 0, 0)` +
  `bgt_rotate_image(id, 0)` + `bgt_scale_image(id, 1.0, 1.0)`。
- 清除后图片回到刚加载时的样子。
- 编号无效时不修改并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_clear_image_transform(hero);
bgt_draw_image(hero, 100, 100);   // 和刚加载时一模一样
```

### `bgt_draw_image`

在指定位置绘制图片，应用该图当前的全部变换状态。

```cpp
void bgt_draw_image(int image_id, int x, int y);
```

参数：

- `image_id`：图片编号。
- `x`、`y`：基准位置。图片的局部原点（左上角）最终落在
  `(x + dx, y + dy)`，其中 `(dx, dy)` 是 `bgt_translate_image` 设置的
  偏移。

说明：

- 变换的合成顺序固定为：先缩放（S），再旋转（R），最后平移（T）。
- 图片绘制不受 `bgt_set_color()` 影响，画面颜色完全来自图片文件。
- 图片自带的透明部分会正确地与背景混合。
- 编号无效时不绘制，并记录 `BGT_ERROR_IMAGE`。

示例：

```cpp
bgt_draw_image(hero, 100, 100);               // 应用当前变换
bgt_draw_image(hero, 300, 100);               // 同一编号、同一变换、不同位置
```

## 4. 变换的组合顺序

当 S、R、T 同时生效时，合成的数学含义是：

```text
M = T(x + dx, y + dy) × R(angle) × S(sx, sy)

v_screen = M × v_local    （v_local 是图片的局部顶点）
```

每个顶点按以下步骤变换：

1. **S（缩放）**：`v_scaled = (lx × sx, ly × sy)`——左上角不动，
   图片伸缩或翻转。
2. **R（旋转）**：`v_rotated = rotate(v_scaled, angle)`——绕左上角转。
3. **T（平移）**：`v_screen = v_rotated + (x + dx, y + dy)`——搬到
   最终位置。

因为顺序固定，所以改变缩放不会影响旋转角，改变旋转角不会影响缩放倍率。
三个状态完全正交，学生只需要理解"每张图有三个属性"。

## 5. 教师注记

- 在主循环外加载图片。每帧调用 `bgt_load_image()` 会持续消耗显存，还会为
  同一张图片拿到重复的编号。
- 图片编号在关闭窗口后失效，重新打开窗口后需要重新加载。
- 同一文件多次加载不会去重，每次调用都会得到一个独立编号——包括独立的
  变换状态。需要同一张图的"原始版"和"翻转版"同时显示时，加载两次即可。
- 变换状态是"图片的属性"这一设计与 `bgt_set_color()` 一致——设置后持续
  生效，直到修改或清除。可以借此强化"状态"这一课程核心概念。
- 翻转（负缩放）会导致图片显示位置移到原点另一侧。如果需要"原地翻转"，
  学生可以在翻转时同时平移 `translate_image(id, image_width, 0)`——
  这本身是一个好的课堂练习（理解负缩放的位置影响）。
- 旋转围绕左上角（严格 RST），不是图片中心。学生转 45 度会看到图片
  "甩出去"。这与后续课程讲 OpenGL 变换矩阵时的行为完全一致——属于
  刻意安排的知识铺垫。
- 用无效编号绘制会记录错误，因此可以用 `bgt_draw_error()` 把最近错误画在
  窗口里。学生最常见的 bug——加载失败后没有检查返回值就继续绘制——会以
  这种方式显现出来，便于课堂演示。

## 6. 随机数

随机数让"每次运行都不一样"的程序成为可能：随机位置、随机事件、随机地图。
libbgt 提供一个统一入口 `bgt_random` 和一个播种函数：

```cpp
void bgt_random_seed(unsigned seed);
int bgt_random(int min, int max);
double bgt_random(double min, double max);
```

`bgt_random` 有两个版本（重载）：**整数参数得到随机整数，小数参数得到
随机小数**，区间都是半开区间 [min, max)：包含 min，不包含 max。

随机数不依赖窗口：即使没有打开窗口，也可以直接调用这些函数（做
命令行练习、生成测试数据时很有用）。完整示例见 `examples/08_random.cpp`。

### `bgt_random`（整数版）

返回一个 [min, max) 半开区间内的随机整数：包含 min、不包含 max。

```cpp
int bgt_random(int min, int max);
```

参数：

- `min`：区间下端（能取到）。
- `max`：区间上端（取不到）。

说明：

- `bgt_random(0, 6)` 等可能地返回 0、1、2、3、4、5，不会返回 6。
- **结果正好当数组下标**：`bgt_random(0, n)` 覆盖长度为 n 的数组的全部
  下标，不需要再减一。
- 掷六面骰写 `bgt_random(1, 7)`：可能得到 1~6，7 永远不会出现。
- 随机范围可以是负数：`bgt_random(-3, 4)` 从 -3 到 3 共 7 种结果。
- `min > max` 时自动交换两者，参数顺序写反不会出错。
- `min == max` 时（空区间）直接返回该值。

示例：

```cpp
int dice = bgt_random(1, 7);      // 掷骰子：1~6

int index = bgt_random(0, 10);    // 直接当长度 10 的数组下标

int dx = bgt_random(-3, 4);       // 随机方向：-3 ~ 3
```

### `bgt_random`（小数版）

返回一个 [min, max) 半开区间内的随机浮点数：包含 min、不包含 max。

```cpp
double bgt_random(double min, double max);
```

说明：

- `bgt_random(0.0, 1.0)` 可能得到 0.000，也可能得到 0.999，但永远
  不会得到 1.0。
- **想要小数，两个参数都要写小数**：`bgt_random(0, 1)` 会调用整数版
  （恒为 0），`bgt_random(0.0, 1.0)` 才是小数版。
- 混合写法（如 `bgt_random(0, 1.0)`）无法通过编译——两个版本都能
  匹配，但谁都不比谁更好，编译器无法选择，会报"对重载函数调用
  不明确"。
- 常用于"随机比例"：伤害打 7~9 折就是 `原伤害 * bgt_random(0.7, 0.9)`。
- `min > max` 时自动交换两者；`min == max` 时直接返回该值。
- 传入 NaN 时行为未定义。

示例：

```cpp
// 30% ~ 100% 的随机不透明度
int alpha = static_cast<int>(bgt_random(0.3, 1.0) * 255);
bgt_set_color(bgt_rgba(255, 0, 0, alpha));
```

### `bgt_random_seed`

设置随机数种子，让随机序列变得**可复现**。

```cpp
void bgt_random_seed(unsigned seed);
```

参数：

- `seed`：种子，任意 `unsigned` 值都可以。

说明：

- 不调用该函数时，库在第一次取随机数时自动播种，因此每次运行程序
  得到的随机数都不同——写游戏时通常正是想要的。
- 调用后，相同种子会得到完全相同的随机序列——调试、单元测试、
  课堂演示"可复现的随机"时非常有用。
- 种子影响的是"之后"的序列：中途重新播种，后续序列从新种子重新开始。

示例：

```cpp
bgt_random_seed(42);
int a = bgt_random(1, 100);     // 每次运行都得到同一个数

bgt_random_seed(42);
int b = bgt_random(1, 100);     // b 和 a 相同
```

### 随机数教学注记

- **半开区间与数组下标是同一课**：长度为 n 的数组合法下标是 0 到 n-1，
  `bgt_random(0, n)` 一次对齐——和 C++ 里迭代器、容器的"左闭右开"约定
  完全一致，可顺势铺垫。
- 学生最经典的疑问"为什么每次运行结果都一样？"在 libbgt 中不会出现
  （默认自动播种）；反过来，"怎么让每次都一样、方便调试"才是
  `bgt_random_seed` 的教学切入点。
- **整数/小数陷阱是必修一课**：`bgt_random(0, 1)` 恒为 0 而不是随机
  小数。建议课堂现场演示这个 bug，再展示 `bgt_random(0.0, 1.0)`——
  "参数类型决定调用哪个版本"本身就是重载的直观一课。
- 掷骰子写 `bgt_random(1, 7)` 比闭区间的 `bgt_random(1, 6)` 多一个 7，
  这是半开区间的代价；用数组下标场景作为对冲，学生就能接受。
- 直方图实验：掷 1000 次骰子并统计每个点数的次数，六根柱子接近一样
  高——"均匀分布"最直观的课堂演示（示例 10 板块 1）。
- 随机游走（示例 10 板块 3）是"随机数 + 每帧调用"的经典组合，也是后续
  课程里游戏 AI（随机巡逻）的雏形。

## 7. 碰撞检测

碰撞检测回答游戏开发里最常问的两个问题："这两个东西碰上了吗？""鼠标
点中它了吗？"libbgt 提供矩形、圆、点两两之间的全部组合（点与点用 `==`
比较即可，不单独提供）：

```cpp
bool bgt_hit_point_rect(int px, int py, int x, int y, int width, int height);
bool bgt_hit_point_circle(int px, int py, int x, int y, int radius);
bool bgt_hit_rect_rect(int x1, int y1, int width1, int height1,
                       int x2, int y2, int width2, int height2);
bool bgt_hit_circle_circle(int x1, int y1, int radius1,
                           int x2, int y2, int radius2);
bool bgt_hit_circle_rect(int cx, int cy, int radius,
                         int x, int y, int width, int height);
```

命名规则：**名字顺序就是参数顺序**；每个形状的参数与它的绘图函数完全
一致（矩形 `(x, y, width, height)` 同 `bgt_fill_rect`，圆 `(x, y, radius)`
同 `bgt_draw_circle`，点用 `px`、`py`）。

两条核心语义：

1. **形状与形状：实际重叠才算命中**。恰好相切、恰好贴边、恰好贴角都
   返回 false——"刚碰到"和"没碰到"是两种不同的状态，把相切归为不碰，
   游戏逻辑更简单。
2. **点测：命中范围与画出的像素一致**。点落在图形上（含边界像素）就算
   命中：`bgt_hit_point_rect` 的范围与 `bgt_fill_rect` 实际覆盖的像素完全
   相同，矩形边框上的点返回 true，边框外一个像素返回 false。

碰撞函数是纯几何计算，不依赖窗口（不开窗也能调用）；退化形状（宽、高或
半径小于等于 0）永远不命中，也不会记录错误。完整示例见
`examples/09_collision.cpp`。

### `bgt_hit_point_rect`

判断点 (px, py) 是否在矩形上。

```cpp
bool bgt_hit_point_rect(int px, int py, int x, int y, int width, int height);
```

参数：

- `px`、`py`：点的坐标。
- `x`、`y`：矩形左上角。
- `width`、`height`：矩形宽高。

说明：

- 命中范围是 `x <= px < x + width` 且 `y <= py < y + height`——与
  `bgt_fill_rect` 画出的像素完全一致。
- "点按钮"的标准写法：

```cpp
if (bgt_hit_point_rect(bgt_mouse_x(), bgt_mouse_y(), 100, 100, 200, 80)) {
    // 鼠标指针在 (100, 100, 200, 80) 这个"按钮"上
}
```

### `bgt_hit_point_circle`

判断点 (px, py) 是否在圆上。

```cpp
bool bgt_hit_point_circle(int px, int py, int x, int y, int radius);
```

参数：

- `px`、`py`：点的坐标。
- `x`、`y`：圆心坐标。
- `radius`：半径。

说明：

- 点到圆心的距离不超过 radius 就算命中：圆周上的点算，圆外一步不算。
- "打靶"点击的标准写法：

```cpp
if (bgt_mouse_just_pressed(BGT_MOUSE_LEFT) &&
    bgt_hit_point_circle(bgt_mouse_x(), bgt_mouse_y(), 500, 400, 60)) {
    // 这次点击命中了圆心 (500, 400)、半径 60 的靶子
}
```

- `radius` 小于等于 0 时恒返回 false。

### `bgt_hit_rect_rect`

判断两个矩形是否实际重叠。

```cpp
bool bgt_hit_rect_rect(int x1, int y1, int width1, int height1,
                       int x2, int y2, int width2, int height2);
```

说明：

- 判定方法是两个方向都"互相压过对方的边"：

```text
x1 < x2 + width2  且  x2 < x1 + width1   （横向重叠）
y1 < y2 + height2 且  y2 < y1 + height1  （纵向重叠）
```

- 只要共有一像素的面积才算命中；恰好共享一条边、只贴一个角、完全分离
  都不算。
- 一个矩形完全套住另一个矩形算命中。
- 宽或高小于等于 0 的矩形是"空的"，恒返回 false。

### `bgt_hit_circle_circle`

判断两个圆是否实际重叠。

```cpp
bool bgt_hit_circle_circle(int x1, int y1, int radius1,
                           int x2, int y2, int radius2);
```

说明：

- 圆心距的平方小于半径和的平方才命中：`dx*dx + dy*dy < (r1+r2)*(r1+r2)`。
- 恰好相切（圆心距恰好等于 r1 + r2）不算命中。
- 一个圆完全套住另一个圆算命中。
- 任一半径小于等于 0 时恒返回 false。

### `bgt_hit_circle_rect`

判断圆与矩形是否实际重叠。

```cpp
bool bgt_hit_circle_rect(int cx, int cy, int radius,
                         int x, int y, int width, int height);
```

说明：

- 判定方法：把圆心"夹"进矩形，得到矩形上离圆心最近的点，比较这段距离
  与半径——比半径小就命中。
- 圆心在矩形内部（含边上）时必然命中（此时最近距离是 0）。
- 圆在矩形外侧恰好擦到边或角（距离恰好等于半径）不算命中。
- 这是"球撞挡板"（Pong、打砖块类）的标准判断。

### 碰撞检测教学注记

- **相切不算碰要眼见为实**：示例 11 板块 4 让学生把圆移到"看起来刚碰上"
  的相切位置，亲眼看到返回 false——把语义规则变成可发现的实验。
- **先自己写，再和库对拍**：`bgt_hit_rect_rect`、`bgt_hit_point_rect`
  只有一行布尔表达式，是绝佳作业。让学生自己实现一个版本，再和库函数
  对拍几十组数据（相同输入应当返回相同结果）——比从零调试更有教学价值。
- `bgt_mouse_x()/bgt_mouse_y()` + 点测是最高频组合：点按钮、点菜单、
  点目标。
- 圆×矩形的"夹住求最近点"算法初学者自己发明不出来，正适合由库提供。
- 椭圆、三角形不提供碰撞：椭圆×椭圆需要坐标变换，三角形×三角形需要
  多边形分离轴算法，超出教学定位；有需要时用"圆或矩形包住"近似。
- 极端坐标：内部用 64 位整数计算，单轴坐标差 ±3×10⁹（或两轴同时各
  ±2×10⁹）内绝不溢出，覆盖一切教学场景；两轴同时近 ±3×10⁹ 的极端组合
  行为未定义。

## 8. 新增常量清单

v0.2 新增的常量：

```cpp
BGT_IMAGE_NONE
```

随机数与碰撞检测接口没有新增常量，也没有新增错误码。

v0.2 新增的错误码是 `BGT_ERROR_IMAGE`，追加在 v0.1 错误码序列的末尾：

```cpp
BGT_ERROR_NONE
BGT_ERROR_SDL
BGT_ERROR_TTF
BGT_ERROR_WINDOW
BGT_ERROR_RENDERER
BGT_ERROR_FONT
BGT_ERROR_NOT_OPEN
BGT_ERROR_IMAGE
```

错误码可通过 `bgt_error_code()` 读取。面向学生的示例通常只需要使用
`bgt_print_error()`。
