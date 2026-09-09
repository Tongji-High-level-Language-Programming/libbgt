// =====================================================================
// libbgt 图片 API 演示 —— 严格 RST 变换模型（v0.2）
//
// 每个板块介绍一小块图片 API，按【空格】进入下一板块，随时可以按
// 【Esc】退出。图片变换遵循图形学经典 RST 模型：
//
//   M = T * R * S
//
// 变换按 S → R → T 的顺序合成，全部围绕图片的局部原点（左上角）。
// 翻转没有独立函数——负缩放就是翻转。
//
//   板块 1  加载与基本信息
//   板块 2  绘制基础
//   板块 3  S — 缩放
//   板块 4  S — 翻转（负缩放）
//   板块 5  R — 旋转
//   板块 6  T — 平移
//   板块 7  RST 组合与清除
//   板块 8  错误路径
// =====================================================================

#include "bgt.h"

#include <cmath>
#include <cstdio>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)

int main()
{
    // ---------- 常量 ----------
    const int window_width = 1000;
    const int window_height = 700;
    const int target_fps = 60;

    if (!bgt_open_window(window_width, window_height, "libbgt 图片 API 演示")) {
        bgt_print_error();
        return 1;
    }
    bgt_set_fps_limit(target_fps);

    // ---------- 加载图片（在主循环外，只做一次）----------
    // 测试图 07_image.png 是 128x128 的四象限图：
    //   左上=不透明红   右上=半透明蓝   左下=完全透明   右下=绿色圆
    // 不对称的象限让我们能一眼看出翻转和旋转的方向。
    //
    // 加载三份：img_a 始终保持恒等变换（作参考），
    // img_b / img_c 在各板块中被重新设置变换状态。
    // 同一文件多次加载得到独立编号——这正是"独立变换"需要的行为。
    const int img_a = bgt_load_image("07_image.png");
    const int img_b = bgt_load_image("07_image.png");
    const int img_c = bgt_load_image("07_image.png");

    // ---------- 各板块需要的"跨帧状态"（就是普通变量）----------

    int part = 1; // 当前板块号：1 到 8

    // 板块 5（旋转）使用：按 ←/→ 调角度
    double demo_angle = 0.0;

    // 板块 6（平移）使用：弹跳动画的进度
    double bounce_t = 0.0;

    // 板块 8（错误）使用
    bool error_printed = false;       // bgt_print_error 只打印一次
    bool tried_bad_load = false;     // 板块 8：只尝试一次加载不存在的文件

    // ---------- 主循环 ----------
    while (bgt_window_is_open()) {

        // 随时退出
        if (bgt_key_just_pressed(BGT_KEY_ESCAPE)) {
            bgt_close_window();
            break;
        }

        // 加载失败时：所有板块都显示错误提示
        if (img_a == BGT_IMAGE_NONE) {
            bgt_set_background(BGT_WHITE);
            bgt_set_color(BGT_RED);
            bgt_draw_text(40, 40, "图片加载失败：", 24);
            bgt_draw_error(40, 80, 20);
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 120,
                          "请从 build 目录运行本程序（07_image.png 在 exe 旁"
                          "边）。",
                          18);
            bgt_update_window();
            continue;
        }

        if (part == 1) {
            // =====================================================
            // 板块 1：加载与基本信息
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 1：加载与基本信息");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 1：加载与基本信息", 32);
            bgt_draw_text(40, 94,
                          "bgt_load_image(文件名) 加载图片，返回一个大于 0 的"
                          "编号。",
                          20);
            bgt_draw_text(40, 128,
                          "加载失败时返回 BGT_IMAGE_NONE(0)，可用 "
                          "bgt_print_error() 查看原因。",
                          20);
            bgt_draw_text(40, 162,
                          "请在主循环外加载：同一文件多次加载得到多个独立"
                          "编号。",
                          20);
            bgt_draw_text(40, 196, "关闭窗口后所有编号失效，需要重新加载。",
                          20);

            // 显示图片的原始尺寸
            char size_text[96];
            std::snprintf(size_text, sizeof(size_text),
                          "bgt_image_width = %d    bgt_image_height = %d",
                          bgt_image_width(img_a), bgt_image_height(img_a));
            bgt_set_color(BGT_BLUE);
            bgt_draw_text(40, 260, size_text, 20);

            // 原样绘制（板块 2 详细讲）
            bgt_draw_image(img_a, 80, 320);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(240, 320,
                          "这就是刚加载的图（bgt_draw_image 原样绘制）。", 18);
            bgt_draw_text(240, 350,
                          "左上红色、右上半透明蓝、右下绿色圆、"
                          "左下完全透明。",
                          18);
        }
        else if (part == 2) {
            // =====================================================
            // 板块 2：绘制基础
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 2：绘制基础");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 2：绘制基础", 32);
            bgt_draw_text(
                40, 94, "bgt_draw_image(编号, x, y)：在 (x, y) 绘制图片。", 20);
            bgt_draw_text(40, 128, "(x, y) 是图片的局部原点——即图片的左上角。",
                          20);
            bgt_draw_text(40, 162,
                          "同一张图片可以画在多个位置，编号不用重复加载。", 20);

            // 画三个副本，展示 (x, y) 控制左上角位置
            bgt_draw_image(img_a, 80, 220);
            bgt_draw_image(img_a, 300, 220);
            bgt_draw_image(img_a, 520, 220);

            // 用十字标记中间那张图的左上角 = 局部原点
            bgt_set_color(BGT_RED);
            bgt_draw_line(290, 220, 310, 220);
            bgt_draw_line(300, 210, 300, 230);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(240, 380,
                          "红色十字标记了中间那张图的局部原点 (300, 220)：",
                          18);
            bgt_draw_text(240, 410, "改变 (x, y) 就是把左上角搬到新位置。", 18);

            bgt_draw_text(40, 480,
                          "图片的透明部分会正确地与背景混合，"
                          "绘制不受 bgt_set_color 影响。",
                          18);
        }
        else if (part == 3) {
            // =====================================================
            // 板块 3：S — 缩放
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 3：S — 缩放");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 3：S — 缩放", 32);
            bgt_draw_text(40, 94,
                          "bgt_scale_image(编号, sx, sy) 设置缩放倍率：", 20);
            bgt_draw_text(40, 128,
                          "  1.0 = 原始大小    2.0 = 放大一倍    "
                          "0.5 = 缩小一半",
                          20);
            bgt_draw_text(40, 162,
                          "缩放围绕局部原点（左上角）——左上角不动，"
                          "图片向右下伸缩。",
                          20);

            // img_a 恒等变换 → 原始大小
            // img_b 缩小 0.5 倍
            // img_c 放大 2.0 倍
            bgt_scale_image(img_b, 0.5, 0.5);
            bgt_scale_image(img_c, 2.0, 2.0);

            bgt_draw_image(img_a, 80, 220);  // 128x128
            bgt_draw_image(img_b, 320, 220); // 64x64
            bgt_draw_image(img_c, 600, 220); // 256x256

            // 标签：分开足够远，不互相遮挡
            bgt_draw_text(80, 510, "scale(1, 1)", 18);
            bgt_draw_text(320, 510, "scale(0.5, 0.5)", 18);
            bgt_draw_text(600, 510, "scale(2, 2)", 18);

            // 板块结束时恢复 img_b / img_c 的变换（不影响后面的板块）
            bgt_scale_image(img_b, 1.0, 1.0);
            bgt_scale_image(img_c, 1.0, 1.0);
        }
        else if (part == 4) {
            // =====================================================
            // 板块 4：S — 翻转（负缩放）
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 4：S — 翻转");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 4：S — 翻转（负缩放）", 32);
            bgt_draw_text(40, 94, "翻转没有独立函数——负缩放就是翻转：", 20);
            bgt_draw_text(40, 128,
                          "  sx = -1.0  →  左右翻转    sy = -1.0  →  "
                          "上下翻转",
                          20);

            bgt_set_color(BGT_RED);
            bgt_draw_text(40, 162, "注意：翻转后图片翻到原点的另一侧！", 20);
            bgt_set_color(BGT_BLACK);

            // img_a：原始
            bgt_draw_image(img_a, 80, 220);
            bgt_draw_text(80, 370, "原始 scale(1, 1)", 18);

            // img_b：左右翻转。原点在 (450, 220)，图片翻到左侧
            // 实际占据 x: 450-128=322 到 450（原图结束于 x=208，间隔 114px）
            bgt_scale_image(img_b, -1.0, 1.0);
            bgt_draw_image(img_b, 450, 220);

            // 标记翻转原点
            bgt_set_color(BGT_RED);
            bgt_draw_line(440, 220, 460, 220);
            bgt_draw_line(450, 210, 450, 230);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(330, 370, "scale(-1, 1) 左右翻转", 18);
            bgt_draw_text(330, 400, "（图片在十字的左侧）", 16);

            // img_c：上下翻转。原点在 (700, 500)，图片翻到上方
            // 实际占据 y: 500-128=372 到 500
            bgt_scale_image(img_c, 1.0, -1.0);
            bgt_draw_image(img_c, 700, 500);

            bgt_set_color(BGT_RED);
            bgt_draw_line(690, 500, 710, 500);
            bgt_draw_line(700, 490, 700, 510);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(650, 520, "scale(1, -1) 上下翻转", 18);
            bgt_draw_text(650, 550, "（图片在十字的上方）", 16);

            bgt_draw_text(40, 580, "红色十字标记局部原点——翻转不改变原点位置，",
                          18);
            bgt_draw_text(40, 610, "只是图片的'身体'翻到了另一侧。", 18);

            // 恢复变换
            bgt_scale_image(img_b, 1.0, 1.0);
            bgt_scale_image(img_c, 1.0, 1.0);
        }
        else if (part == 5) {
            // =====================================================
            // 板块 5：R — 旋转
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 5：R — 旋转");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 5：R — 旋转", 32);
            bgt_draw_text(40, 94,
                          "bgt_rotate_image(编号, angle)：设置旋转角度"
                          "（度，正值顺时针）。",
                          20);
            bgt_draw_text(40, 128,
                          "旋转围绕图片的局部原点（左上角）——图片绕它转动。",
                          20);
            bgt_draw_text(40, 162, "按 ←/→ 调整角度，观察图片绕红色十字旋转。",
                          20);

            // 交互：←/→ 改角度
            if (bgt_key_is_down(BGT_KEY_LEFT)) {
                demo_angle = demo_angle - 90.0 * bgt_delta_time();
            }
            if (bgt_key_is_down(BGT_KEY_RIGHT)) {
                demo_angle = demo_angle + 90.0 * bgt_delta_time();
            }

            // img_a：参考图（不旋转）
            bgt_draw_image(img_a, 600, 220);

            // img_b：旋转的图
            bgt_rotate_image(img_b, demo_angle);
            bgt_draw_image(img_b, 200, 220);

            // 标记旋转中心（局部原点）
            bgt_set_color(BGT_RED);
            bgt_draw_line(190, 220, 210, 220);
            bgt_draw_line(200, 210, 200, 230);

            char angle_text[64];
            std::snprintf(angle_text, sizeof(angle_text), "当前角度：%.0f 度",
                          demo_angle);
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(200, 430, angle_text, 20);
            bgt_draw_text(600, 370, "参考：未旋转", 18);

            bgt_draw_text(40, 490,
                          "看红色十字：旋转时左上角始终不动，图片绕它摆动。",
                          18);
            bgt_draw_text(40, 520, "旋转围绕局部原点(0, 0)，不是围绕图片中心。",
                          18);

            // 恢复变换
            bgt_rotate_image(img_b, 0.0);
        }
        else if (part == 6) {
            // =====================================================
            // 板块 6：T — 平移
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 6：T — 平移");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 6：T — 平移", 32);
            bgt_draw_text(40, 94,
                          "bgt_translate_image(编号, dx, dy) 设置偏移量。", 20);
            bgt_draw_text(40, 128,
                          "实际显示位置 = bgt_draw_image 的 (x, y) + "
                          "这里的 (dx, dy)。",
                          20);
            bgt_draw_text(40, 162,
                          "适合做弹跳、后坐力等'逻辑位置不变、视觉位置微调'"
                          "的动画。",
                          20);

            // 弹跳动画：逻辑位置 (500, 320) 不变，视觉位置上下浮动
            bounce_t = bounce_t + bgt_delta_time() * 3.0;
            const double bounce = std::sin(bounce_t) * 20.0;
            const int bounce_offset = static_cast<int>(bounce);

            // img_b：有偏移的图（每帧更新偏移量）
            bgt_translate_image(img_b, 0, bounce_offset);
            bgt_draw_image(img_b, 500, 320);

            // img_a：无偏移的参考图
            bgt_draw_image(img_a, 200, 320);

            // 逻辑位置标记（十字 = 逻辑位置，图片在它上下浮动）
            bgt_set_color(BGT_RED);
            bgt_draw_line(490, 320, 510, 320);
            bgt_draw_line(500, 310, 500, 330);
            bgt_set_color(BGT_BLACK);

            bgt_draw_text(200, 470, "无偏移（参考）", 18);
            bgt_draw_text(430, 470, "有偏移：红色十字是逻辑位置，图片在浮动",
                          18);

            char offset_text[64];
            std::snprintf(offset_text, sizeof(offset_text),
                          "当前偏移 translate(0, %d)", bounce_offset);
            bgt_draw_text(40, 530, offset_text, 20);

            bgt_draw_text(40, 580,
                          "偏移是图片的属性，设置一次持续生效直到改变。", 18);

            // 恢复变换
            bgt_translate_image(img_b, 0, 0);
        }
        else if (part == 7) {
            // =====================================================
            // 板块 7：RST 组合与清除
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 7：RST 组合与清除");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 7：RST 组合与清除", 32);
            bgt_draw_text(40, 94, "三个变换可以同时生效，合成顺序固定：", 20);
            bgt_draw_text(40, 128, "  先缩放（S），再旋转（R），最后平移（T）",
                          20);
            bgt_draw_text(40, 162, "  M = T * R * S    （先 S 后 R 后 T）", 20);

            // img_a：原始图
            bgt_draw_image(img_a, 100, 240);

            // img_b：缩放 + 旋转 + 平移的组合
            bgt_scale_image(img_b, 1.5, 1.5);   // S：放大 1.5 倍
            bgt_rotate_image(img_b, 30.0);      // R：转 30 度
            bgt_translate_image(img_b, 50, 20); // T：向右下偏移
            bgt_draw_image(img_b, 400, 240);

            bgt_draw_text(100, 390, "原始", 18);
            bgt_draw_text(400, 460,
                          "scale(1.5) + rotate(30) + "
                          "translate(50, 20)",
                          16);
            bgt_draw_text(400, 490, "（先缩放，再绕左上角转，最后偏移）", 16);

            // img_c：清除后恢复原样
            bgt_clear_image_transform(img_c);
            bgt_draw_image(img_c, 750, 240);
            bgt_draw_text(750, 390, "clear 后", 18);

            bgt_draw_text(40, 540,
                          "bgt_clear_image_transform 一次调用把 T、R、S "
                          "全部恢复默认：",
                          18);
            bgt_draw_text(40, 570, "  T(0, 0)、R(0 度)、S(1.0, 1.0) = 恒等变换",
                          18);
            bgt_draw_text(40, 600, "图片回到刚加载时的样子。", 18);

            // 恢复 img_b
            bgt_clear_image_transform(img_b);
        }
        else {
            // =====================================================
            // 板块 8：错误路径
            // =====================================================
            bgt_set_background(bgt_rgb(250, 250, 250));
            bgt_set_window_title("libbgt 图片演示 - 板块 8：错误路径");

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 36, "板块 8：错误路径", 32);
            bgt_draw_text(40, 94,
                          "图片操作失败不会崩溃——会记录错误，"
                          "你可以看到它。",
                          20);

            // 故意加载一个不存在的文件（只在第一次进入本板块时尝试，
            // tried_bad_load 保证后续帧不再重复发起文件 IO）
            if (!tried_bad_load) {
                tried_bad_load = true;
                const int bad =
                    bgt_load_image("this_file_does_not_exist.png");

                if (bad == BGT_IMAGE_NONE) {
                bgt_set_color(BGT_RED);
                bgt_draw_text(
                    40, 150, "加载不存在的文件 → 返回 BGT_IMAGE_NONE(0)：", 20);
                bgt_draw_text(40, 190, "错误码可通过 bgt_error_code() 查看：",
                              18);
                char code_text[64];
                std::snprintf(code_text, sizeof(code_text),
                              "  bgt_error_code() = %d（BGT_ERROR_IMAGE）",
                              bgt_error_code());
                bgt_draw_text(60, 220, code_text, 18);

                // 完整错误信息打印到控制台（首次进入本板块时）
                if (!error_printed) {
                    error_printed = true;
                    bgt_print_error();
                }
                bgt_set_color(BGT_DARK_GRAY);
                bgt_draw_text(
                    40, 260,
                    "完整错误信息已用 bgt_print_error() 打印到控制台。", 18);
                }
            }

            // 故意用无效编号绘制
            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 330,
                          "故意用编号 0（无效）绘制 → 记录错误，不崩溃：", 20);
            bgt_clear_error();
            bgt_draw_image(BGT_IMAGE_NONE, 100, 380);
            if (bgt_has_error()) {
                bgt_set_color(BGT_RED);
                bgt_draw_text(40, 380, "bgt_has_error() 为 true", 18);
                bgt_draw_text(40, 410, "（bgt_draw_error 的内容见控制台）", 16);
            }

            bgt_set_color(BGT_BLACK);
            bgt_draw_text(40, 480,
                          "用 bgt_clear_error() 清除后，bgt_has_error() "
                          "变回 false。",
                          20);

            bgt_draw_text(40, 640, "按【空格】关闭窗口（Esc 也可以）。", 20);
        }

        // 底部统一提示（最后一个板块自己提示了关闭方式）
        if (part < 8) {
            bgt_set_color(BGT_DARK_GRAY);
            bgt_draw_text(40, 648, "按【空格】进入下一板块，按【Esc】退出程序",
                          18);
        }

        // 每个板块都支持按空格进入下一板块；最后一个板块按空格关闭
        if (bgt_key_just_pressed(BGT_KEY_SPACE)) {
            if (part == 8) {
                break;
            }
            part = part + 1;
        }

        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// readability-function-cognitive-complexity)
