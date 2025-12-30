/* 测试演示程序 - libbgt 图形库功能展示 */

// 本测试由 DeepSeek 生成，如果有问题请联系作者反馈

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

#include "libbgt.h"

using namespace std;

// 辅助函数：显示提示信息并等待任意按键
static void wait_continue(const char* prompt, int bg_r, int bg_g, int bg_b,  int x = 20, int y = 600) {
	const char* continue_msg = "按任意键继续...";
	const int line_height = bgt_get_font_height();

    // 显示新的提示信息
    bgt_show_str(x, y, prompt, 255, 255, 0, BGT_ALPHA_OPAQUE, false);  // 黄色提示文字
    bgt_show_str(x, y + line_height, continue_msg, 200, 200, 200);

    // 等待任意按键
    bgt_getch();

	bgt_rectangle(x, y, bgt_measure_text(prompt), bgt_get_font_height(), bg_r, bg_g, bg_b, BGT_ALPHA_OPAQUE, false);
    bgt_rectangle(x, y + line_height, bgt_measure_text(continue_msg), bgt_get_font_height(), bg_r, bg_g, bg_b);
}

// 演示1：基本图形绘制
void demo_basic_shapes() {
    bgt_cls(0, 0, 40);  // 浅蓝色背景

    // 绘制标题
    bgt_show_str(300, 20, "基本图形绘制演示", 255, 255, 255);

    // 绘制矩形
    bgt_rectangle(100, 100, 200, 100, 255, 0, 0);  // 红色矩形
    bgt_rectangle(350, 100, 200, 100, 0, 255, 0, 150);  // 绿色半透明矩形
    bgt_rectangle(600, 100, 200, 100, 0, 0, 255);  // 蓝色矩形

    bgt_show_str(150, 220, "矩形", 255, 255, 255);
    bgt_show_str(400, 220, "半透明矩形", 255, 255, 255);
    bgt_show_str(650, 220, "蓝色矩形", 255, 255, 255);

	wait_continue("矩形绘制完成", 0, 0, 40);

    // 绘制直线
    bgt_line(100, 300, 300, 500, 255, 255, 0);  // 黄色斜线
    bgt_line(400, 300, 600, 300, 0, 255, 255);  // 青色横线
    bgt_line(550, 350, 550, 550, 255, 0, 255);  // 紫色竖线

    bgt_show_str(200, 520, "各种直线", 255, 255, 255);

	wait_continue("直线绘制完成", 0, 0, 40);

    // 绘制圆形
    bgt_circle(200, 400, 50, 255, 200, 0);  // 橙色圆形
    bgt_circle(450, 400, 80, 255, 255, 255, 100);  // 白色半透明圆形
    bgt_circle(700, 400, 60, 0, 255, 255);  // 青色圆形

    bgt_show_str(180, 470, "圆形", 255, 255, 255);
    bgt_show_str(420, 490, "半透明圆", 255, 255, 255);
    bgt_show_str(680, 470, "圆形", 255, 255, 255);

    wait_continue("演示1：基本图形绘制完成", 0, 0, 40);
}

// 演示2：文字输出功能
void demo_text_output() {
    bgt_cls(30, 30, 60);  // 深紫色背景

    bgt_show_str(300, 20, "文字输出功能演示", 255, 255, 255);

    // 使用 bgt_show_str 输出
    bgt_show_str(100, 100, "1. 使用 bgt_show_str 输出单行文字", 255, 200, 100);
    bgt_show_str(100, 140, "Hello, libbgt!", 100, 255, 100);
    bgt_show_str(100, 180, "中文测试：你好，世界！", 100, 200, 255);

    // 半透明文字
    bgt_show_str(100, 220, "2. 半透明文字效果", 255, 255, 0);
    bgt_show_str(100, 260, "透明度 50%", 255, 100, 100, 128);
    bgt_show_str(100, 300, "透明度 75%", 100, 255, 100, 192);

    // 使用 bgt_cout 格式化输出
    bgt_show_str(100, 360, "3. 使用 bgt_cout 格式化输出", 200, 200, 255);

    double pi = 3.141592653589793;
    int score = 95;
    double price = 123.456;

    bgt_cout(100, 400, 255, 255, 255) << "圆周率: " << setiosflags(ios::fixed)
        << setprecision(4) << pi;
    bgt_cout(100, 440, 255, 255, 255) << "分数: " << score << "分";
    bgt_cout(100, 480, 255, 255, 255) << "价格: ￥" << setprecision(2) << price;

    // 显示字体尺寸信息
    int font_width = bgt_get_font_width();
    int font_height = bgt_get_font_height();

    bgt_show_str(500, 100, "字体尺寸信息：", 200, 255, 200);
    bgt_cout(500, 140, 255, 255, 255) << "字体宽度: " << font_width << " 像素";
    bgt_cout(500, 180, 255, 255, 255) << "字体高度: " << font_height << " 像素";

    // 测量字符串宽度
    const char* test_str1 = "Hello World";
    const char* test_str2 = "你好世界";
    const char* test_str3 = "混合：Hello 世界";

    bgt_show_str(500, 260, "字符串宽度测量：", 200, 255, 200);
    bgt_cout(500, 300, 255, 255, 255) << "\"" << test_str1 << "\" 宽度: "
        << bgt_measure_text(test_str1) << " 像素";
    bgt_cout(500, 340, 255, 255, 255) << "\"" << test_str2 << "\" 宽度: "
        << bgt_measure_text(test_str2) << " 像素";
    bgt_cout(500, 380, 255, 255, 255) << "\"" << test_str3 << "\" 宽度: "
        << bgt_measure_text(test_str3) << " 像素";

    wait_continue("演示2：文字输出功能完成", 30, 30, 60);
}

// 演示3：输入功能
void demo_input_functions() {
    bgt_cls(40, 60, 40);  // 深绿色背景

    bgt_show_str(300, 20, "输入功能演示", 255, 255, 255);

    // 数字输入演示
    bgt_show_str(100, 100, "1. 数字输入演示", 255, 200, 100);
    bgt_show_str(100, 140, "请在下面的输入框中输入一个整数：", 200, 200, 255);

    // 绘制输入区域背景
    bgt_rectangle(98, 178, 204, 44, 100, 100, 100);  // 边框
    bgt_rectangle(100, 180, 200, 40, 50, 50, 50);  // 灰色背景

    int number = bgt_input_number(100, 180, 255, 255, 0, 255, 50, 50, 50);

    // 显示输入结果
    bgt_cls(40, 60, 40);  // 清除之前的显示
    bgt_show_str(300, 20, "输入功能演示", 255, 255, 255);
    bgt_show_str(100, 100, "1. 数字输入演示", 255, 200, 100);
    bgt_cout(100, 140, 0, 255, 255) << "你输入的数字是: " << number;

    wait_continue("数字输入完成，继续字符串输入演示", 40, 60, 40);

    // 字符串输入演示
    bgt_cls(40, 60, 40);
    bgt_show_str(300, 20, "输入功能演示", 255, 255, 255);
    bgt_show_str(100, 100, "2. 字符串输入演示", 255, 200, 100);
    bgt_show_str(100, 140, "请输入你的名字（英文）：", 200, 200, 255);

    // 绘制输入区域背景
    bgt_rectangle(98, 178, 304, 44, 100, 100, 100);  // 边框
    bgt_rectangle(100, 180, 300, 40, 40, 60, 40);  // 灰色背景

    char name[100];
    int result = bgt_input_ascii(100, 180, name, 100, 0, 255, 255, 255, 40, 60, 40);

    // 显示输入结果
    bgt_cls(40, 60, 40);
    bgt_show_str(300, 20, "输入功能演示", 255, 255, 255);
    bgt_show_str(100, 100, "2. 字符串输入演示", 255, 200, 100);
    bgt_cout(100, 140, 0, 255, 255) << "你输入的名字是: " << name;

    wait_continue("演示3：输入功能完成", 40, 60, 40);
}

// 演示4：鼠标事件处理
void demo_mouse_events() {
    bgt_cls(60, 40, 40);  // 深红色背景

    bgt_show_str(300, 20, "鼠标事件演示", 255, 255, 255);
    bgt_show_str(100, 60, "请移动鼠标、点击、双击、滚动滚轮...", 200, 255, 200);
    bgt_show_str(100, 100, "按任意键盘键结束演示", 255, 200, 200);

    // 显示鼠标事件区域
    bgt_rectangle(50, 150, 700, 400, 30, 30, 30);
    bgt_show_str(320, 160, "鼠标事件检测区", 255, 255, 0);

    // 显示说明
    bgt_show_str(100, 570, "事件说明：", 200, 200, 255);
    bgt_show_str(100, 600, "左键点击=红点，左键双击=黄点，右键点击=绿点，右键双击=紫点，滚轮=蓝点，移动=白点"
        , 200, 200, 255);

    int mouse_x, mouse_y, mouse_action;
    int keycode, key_modifier;
    bool running = true;

    while (running) {
        int event_type = bgt_read_keyboard_and_mouse(mouse_x, mouse_y, mouse_action, keycode, key_modifier);

        if (event_type == BGT_KEYBOARD_EVENT) {
            // 按任意键盘键结束
            running = false;
            bgt_show_str(320, 500, "键盘按键检测到，结束鼠标演示", 255, 100, 100);
            bgt_delay(1000);
        }
        else if (event_type == BGT_MOUSE_EVENT) {
            if (mouse_x < 50 || mouse_x > 750 || mouse_y < 150 || mouse_y > 550) {
                // 鼠标在区域外，不处理
                continue;
			}
            // 在鼠标位置绘制不同颜色的点表示不同事件
            if (mouse_action == MOUSE_LEFT_BUTTON_CLICK) {
                bgt_rectangle(mouse_x - 3, mouse_y - 3, 6, 6, 255, 0, 0);  // 红色点
                bgt_show_str(mouse_x + 10, mouse_y - 5, "左键", 255, 100, 100);
            }
            else if (mouse_action == MOUSE_RIGHT_BUTTON_CLICK) {
                bgt_rectangle(mouse_x - 3, mouse_y - 3, 6, 6, 0, 255, 0);  // 绿色点
                bgt_show_str(mouse_x + 10, mouse_y - 5, "右键", 100, 255, 100);
            }
            else if (mouse_action == MOUSE_LEFT_BUTTON_DOUBLE_CLICK) {
                bgt_circle(mouse_x, mouse_y, 10, 255, 255, 0);  // 黄色圈
                bgt_show_str(mouse_x + 15, mouse_y - 5, "双击", 255, 255, 100);
            }
            else if (mouse_action == MOUSE_RIGHT_BUTTON_DOUBLE_CLICK) {
                bgt_circle(mouse_x, mouse_y, 10, 255, 0, 255);  // 紫色圈
                bgt_show_str(mouse_x + 15, mouse_y - 5, "右键双击", 255, 100, 255);
			}
            else if (mouse_action == MOUSE_WHEEL_CLICK) {
                bgt_rectangle(mouse_x - 4, mouse_y - 4, 8, 8, 0, 200, 255);  // 青色点
                bgt_show_str(mouse_x + 10, mouse_y - 5, "滚轮按下", 100, 200, 255);
			}
            else if (mouse_action == MOUSE_WHEEL_MOVED_UP) {
                bgt_rectangle(mouse_x - 4, mouse_y - 4, 8, 8, 0, 200, 255);  // 青色点
                bgt_show_str(mouse_x + 10, mouse_y - 5, "滚轮上", 100, 200, 255);
            }
            else if (mouse_action == MOUSE_WHEEL_MOVED_DOWN) {
                bgt_rectangle(mouse_x - 4, mouse_y - 4, 8, 8, 0, 200, 255);  // 青色点
                bgt_show_str(mouse_x + 10, mouse_y - 5, "滚轮下", 100, 200, 255);
            }
            else if (mouse_action == MOUSE_ONLY_MOVED) {
                bgt_rectangle(mouse_x, mouse_y, 2, 2, 255, 255, 255);  // 白色小点
            }

            // 显示鼠标坐标
            bgt_rectangle(750, 570, 140, 60, 40, 40, 40);
            bgt_cout(760, 580, 255, 255, 255) << "X: " << mouse_x;
            bgt_cout(760, 610, 255, 255, 255) << "Y: " << mouse_y;
        }

        // 避免过于频繁的刷新
        bgt_delay(10);
    }

    wait_continue("演示4：鼠标事件完成", 60, 40, 40);
}

// 演示5：键盘事件和混合模式
void demo5_draw_background() {
    // 绘制一些背景图形用于测试混合效果
    for (int i = 0; i < 5; i++) {
        bgt_circle(200 + i * 120, 200, 40, 255, 100, 100, 100, false);  // 半透明红色圆形
        bgt_rectangle(150 + i * 120, 400, 80, 60, 100, 255, 100, 100, false);  // 半透明绿色矩形
    }
}

void demo5_draw_scene(int box_x, int box_y, int box_size, unsigned int blend_mode) {
	// 先清除整个屏幕
	bgt_cls(40, 40, 60);

	// 重新绘制背景图形
    demo5_draw_background();

    // 在绘制方块的时候使用不同混合模式
	bgt_set_blend_mode(blend_mode);
	// 绘制方块
	bgt_rectangle(box_x, box_y, box_size, box_size, 0, 200, 255, 140, false);  // 青色半透明方块
	bgt_set_blend_mode(BGT_BLENDMODE_BLEND);  // 恢复默认混合模式

	// 绘制所有文字
	bgt_show_str(300, 20, "键盘事件和混合模式演示", 255, 255, 255, BGT_ALPHA_OPAQUE, false);
	bgt_show_str(100, 60, "请按键盘上的方向键移动方块，按ESC退出", 200, 255, 200, BGT_ALPHA_OPAQUE, false);

	// 根据当前混合模式显示不同的提示
	bgt_rectangle(100, 90, 450, 35, 40, 40, 60, BGT_ALPHA_OPAQUE, false);  // 清除旧提示的背景
	switch (blend_mode) {
	case BGT_BLENDMODE_NONE:
		bgt_show_str(100, 100, "当前混合模式：1-BGT_BLENDMODE_NONE（无混合，覆盖）", 255, 200, 200, BGT_ALPHA_OPAQUE, false);
		break;
	case BGT_BLENDMODE_BLEND:
		bgt_show_str(100, 100, "当前混合模式：2-BGT_BLENDMODE_BLEND（默认，半透明）", 255, 200, 200, BGT_ALPHA_OPAQUE, false);
		break;
	case BGT_BLENDMODE_ADD:
		bgt_show_str(100, 100, "当前混合模式：3-BGT_BLENDMODE_ADD（加性混合，光照）", 255, 200, 200, BGT_ALPHA_OPAQUE, false);
		break;
	case BGT_BLENDMODE_MOD:
		bgt_show_str(100, 100, "当前混合模式：4-BGT_BLENDMODE_MOD（乘性混合，阴影）", 255, 200, 200, BGT_ALPHA_OPAQUE, false);
		break;
	}

	// 显示方块坐标
	bgt_rectangle(600, 500, 180, 60, 30, 30, 30, BGT_ALPHA_OPAQUE, false);
	bgt_cout(610, 510, 255, 255, 255, BGT_ALPHA_OPAQUE, false) << "方块位置:";
	bgt_cout(610, 540, 255, 255, 255, BGT_ALPHA_OPAQUE, false) << "(" << box_x << ", " << box_y << ")";

	// 显示按键说明
	bgt_rectangle(100, 550, 400, 40, 30, 30, 30, BGT_ALPHA_OPAQUE, false);
	bgt_show_str(110, 560, "按1-4切换混合模式，ESC退出", 200, 200, 255, BGT_ALPHA_OPAQUE, false);

	// 最后统一刷新
	bgt_flush();
}


void demo_keyboard_and_blend() {
    // 初始方块位置
    int box_x = 400, box_y = 300;
    int box_size = 50;

    // 混合模式初始值
    unsigned int blend_mode = BGT_BLENDMODE_BLEND;

    // 初始绘制
    demo5_draw_scene(box_x, box_y, box_size, blend_mode);

    bool running = true;

    while (running) {
        // 处理键盘输入
        int key = bgt_getch();

        // 根据按键移动方块或切换混合模式
        switch (key) {
        case BGTK_UP:
            box_y -= 30;
            break;
        case BGTK_DOWN:
            box_y += 30;
            break;
        case BGTK_LEFT:
            box_x -= 30;
            break;
        case BGTK_RIGHT:
            box_x += 30;
            break;
        case BGTK_ESCAPE:
            running = false;
            break;
        case BGTK_1:
            blend_mode = BGT_BLENDMODE_NONE;
            break;
        case BGTK_2:
            blend_mode = BGT_BLENDMODE_BLEND;
            break;
        case BGTK_3:
            blend_mode = BGT_BLENDMODE_ADD;
            break;
        case BGTK_4:
            blend_mode = BGT_BLENDMODE_MOD;
            break;
        }

        // 限制方块在窗口内
        if (box_x < 0) box_x = 0;
        if (box_y < 0) box_y = 0;
        if (box_x > 800 - box_size) box_x = 800 - box_size;
        if (box_y > 600 - box_size) box_y = 600 - box_size;

        // 重新绘制整个场景
        demo5_draw_scene(box_x, box_y, box_size, blend_mode);
    }

    wait_continue("演示5：键盘事件和混合模式完成", 40, 40, 60);
}

// 演示6：动画和计时功能
void demo_animation() {
    bgt_cls(20, 20, 40);  // 深蓝色背景

    bgt_show_str(300, 20, "动画和计时功能演示", 255, 255, 255);
    bgt_show_str(100, 60, "演示简单的动画效果，按任意键结束", 200, 255, 200);

    // 获取初始时间
    unsigned long long start_time = bgt_get_ticks();

    bool running = true;
    int sun_x = 100;
    int sun_y = 400;
    int sun_radius = 40;
    int direction = 1;  // 1表示向右上，-1表示向左下

    while (running) {
        // 清除太阳旧位置
        bgt_rectangle(sun_x - sun_radius - 2, sun_y - sun_radius - 2,
            sun_radius * 2 + 4, sun_radius * 2 + 4, 20, 20, 40, BGT_ALPHA_OPAQUE, false);

        // 更新太阳位置
        sun_x += 3 * direction;
        sun_y -= 2 * direction;

        // 边界检测，改变方向
        if (sun_x > 700 || sun_x < 100) {
            direction = -direction;
        }
        if (sun_y > 500 || sun_y < 100) {
            direction = -direction;
        }

        // 绘制太阳（渐变颜色）
        for (int i = sun_radius; i > 0; i--) {
            int intensity = 255 - (sun_radius - i) * 5;
            if (intensity < 100) intensity = 100;
            bgt_circle(sun_x, sun_y, i, 255, intensity, 0, 200, false);  // 红色到黄色的渐变
        }

        // 绘制轨迹
        bgt_rectangle(sun_x, sun_y, 2, 2, 255, 255, 100, 100, false);

        // 显示运行时间
        unsigned long long current_time = bgt_get_ticks();
        unsigned long long elapsed = current_time - start_time;

        bgt_rectangle(600, 550, 180, 40, 30, 30, 50, BGT_ALPHA_OPAQUE, false);
        bgt_cout(610, 560, 255, 255, 255) << "时间: " << elapsed << "ms";

        // 控制动画速度
        bgt_delay(20);

        if (elapsed >= 15'000) {
            // 15秒后自动结束
			running = false;
        }
    }

    wait_continue("演示6：动画和计时功能完成", 20, 20, 40);
}

int main() {
    std::cout << "libbgt 图形库演示程序" << std::endl;
    // 初始化窗口
    if (!bgt_init(1024, 768, "libbgt 图形库演示程序", "SimSun", 20)) {
        cout << "初始化失败: " << bgt_get_error() << endl;
        return -1;
    }

    // 显示欢迎信息
    bgt_cls(0, 0, 30);
    bgt_show_str(250, 150, "libbgt 图形库演示程序", 255, 255, 0);
    bgt_show_str(200, 200, "基于 SDL3 封装的简易图形界面工具集", 200, 200, 255);
    bgt_show_str(300, 300, "按任意键开始演示", 100, 255, 100);
    bgt_getch();

    // 依次运行演示
    demo_basic_shapes();       // 演示1：基本图形
    demo_text_output();        // 演示2：文字输出
    demo_input_functions();    // 演示3：输入功能
    demo_mouse_events();       // 演示4：鼠标事件
    demo_keyboard_and_blend(); // 演示5：键盘和混合模式
    demo_animation();          // 演示6：动画和计时

    // 结束演示
    bgt_cls(0, 20, 40);
    bgt_show_str(300, 200, "演示结束", 255, 255, 0);
    bgt_show_str(250, 250, "感谢使用 libbgt 图形库", 200, 200, 255);
    bgt_show_str(300, 300, "按任意键退出", 100, 255, 100);
    bgt_getch();

    // 清理资源
    bgt_quit();

    return 0;
}