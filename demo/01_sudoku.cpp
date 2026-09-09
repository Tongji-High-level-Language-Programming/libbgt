#include "bgt.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <string>

// NOLINTBEGIN(readability-magic-numbers, readability-identifier-length,
// bugprone-easily-swappable-parameters)

namespace {

constexpr int kGridSize = 9;
constexpr int kBoxSize = 3;
constexpr int kMinDigit = 1;
constexpr int kMaxDigit = 9;
constexpr std::size_t kCellCount =
    static_cast<std::size_t>(kGridSize * kGridSize);
constexpr int kCellSize = 92;
constexpr int kBoardMargin = 72;
constexpr int kBoardX = kBoardMargin;
constexpr int kBoardY = kBoardMargin;
constexpr int kBoardSize = kGridSize * kCellSize;
constexpr int kPanelGap = 72;
constexpr int kPanelWidth = 420;
constexpr int kPanelPadding = 28;
constexpr int kPanelX = kBoardX + kBoardSize + kPanelGap;
constexpr int kPanelY = kBoardY;
constexpr int kPanelHeight = kBoardSize;
constexpr int kPanelInnerX = kPanelX + kPanelPadding;
constexpr int kPanelContentWidth = kPanelWidth - (kPanelPadding * 2);
constexpr int kWindowRightMargin = 72;
constexpr int kWindowBottomMargin = 120;
constexpr int kWindowWidth = kPanelX + kPanelWidth + kWindowRightMargin;
constexpr int kWindowHeight = kBoardY + kBoardSize + kWindowBottomMargin;
constexpr int kTargetFps = 60;

constexpr int kThinLineWidth = 1;
constexpr int kMediumLineWidth = 2;
constexpr int kThickLineWidth = 4;

constexpr int kTitleFontSize = 42;
constexpr int kHelpFontSize = 22;
constexpr int kButtonFontSize = 28;
constexpr int kCellDigitFontSize = 52;
constexpr int kNoteFontSize = 20;
constexpr int kPuzzleFontSize = 20;

constexpr int kTitleY = kPanelY + 24;
constexpr int kHelpFirstY = kPanelY + 96;
constexpr int kHelpLineGap = 32;
constexpr int kModeButtonY = kPanelY + 210;
constexpr int kModeButtonGap = 20;
constexpr int kModeButtonWidth = (kPanelContentWidth - kModeButtonGap) / 2;
constexpr int kModeButtonHeight = 58;
constexpr int kDigitButtonY = kPanelY + 330;
constexpr int kDigitButtonGapX = 22;
constexpr int kDigitButtonGapY = 22;
constexpr int kDigitButtonSize = 72;
constexpr int kDigitButtonGridWidth =
    (kDigitButtonSize * kBoxSize) + (kDigitButtonGapX * (kBoxSize - 1));
constexpr int kDigitButtonX =
    kPanelInnerX + ((kPanelContentWidth - kDigitButtonGridWidth) / 2);
constexpr int kClearButtonY = kPanelY + 626;
constexpr int kClearButtonHeight = 60;
constexpr int kPuzzleLabelY = kPanelY + 766;
constexpr int kCellDigitYOffset = 2;
constexpr int kNoteInset = 10;
constexpr int kNoteSlotSize = (kCellSize - (kNoteInset * 2)) / kBoxSize;

constexpr unsigned kColorBackground = 0xFFF6F0E8U;
constexpr unsigned kColorBoard = 0xFFFFFCF5U;
constexpr unsigned kColorPanel = 0xFFFFF8ECU;
constexpr unsigned kColorGrid = 0xFF6D5D4DU;
constexpr unsigned kColorThickGrid = 0xFF342820U;
constexpr unsigned kColorSelected = 0xFFF1B85FU;
constexpr unsigned kColorRelated = 0xFFFFE7B8U;
constexpr unsigned kColorSameNumber = 0xFFFFCE86U;
constexpr unsigned kColorPresetText = 0xFF1C1815U;
constexpr unsigned kColorUserText = 0xFF2266AAU;
constexpr unsigned kColorNoteText = 0xFF777777U;
constexpr unsigned kColorButton = 0xFFFFFFFFU;
constexpr unsigned kColorButtonActive = 0xFFFFD994U;
constexpr unsigned kColorButtonBorder = 0xFF8A765FU;
constexpr unsigned kColorDisabled = 0xFFB7ADA2U;

struct SudokuState {
    std::array<int, kCellCount> given{};
    std::array<int, kCellCount> value{};
    std::array<int, kCellCount> notes{};
    int selected_row = 0;
    int selected_col = 0;
    bool note_mode = false;
    bool loaded_from_file = false;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

int cell_index(int row, int col)
{
    return (row * kGridSize) + col;
}

bool same_box(int row_a, int col_a, int row_b, int col_b)
{
    return (row_a / kBoxSize == row_b / kBoxSize) &&
           (col_a / kBoxSize == col_b / kBoxSize);
}

Rect board_rect()
{
    return {kBoardX, kBoardY, kBoardSize, kBoardSize};
}

Rect panel_rect()
{
    return {kPanelX, kPanelY, kPanelWidth, kPanelHeight};
}

Rect cell_rect(int row, int col)
{
    return {kBoardX + (col * kCellSize), kBoardY + (row * kCellSize), kCellSize,
            kCellSize};
}

Rect fill_mode_button_rect()
{
    return {kPanelInnerX, kModeButtonY, kModeButtonWidth, kModeButtonHeight};
}

Rect note_mode_button_rect()
{
    return {kPanelInnerX + kModeButtonWidth + kModeButtonGap, kModeButtonY,
            kModeButtonWidth, kModeButtonHeight};
}

Rect digit_button_rect(int digit)
{
    const int button_col = (digit - kMinDigit) % kBoxSize;
    const int button_row = (digit - kMinDigit) / kBoxSize;
    return {
        kDigitButtonX + (button_col * (kDigitButtonSize + kDigitButtonGapX)),
        kDigitButtonY + (button_row * (kDigitButtonSize + kDigitButtonGapY)),
        kDigitButtonSize, kDigitButtonSize};
}

Rect clear_button_rect()
{
    return {kPanelInnerX, kClearButtonY, kPanelContentWidth,
            kClearButtonHeight};
}

bool read_puzzle_from_file(SudokuState &s, const char *filename)
{
    std::ifstream file(filename);
    if (!file) {
        return false;
    }

    std::string content;
    char ch = '\0';
    while (file.get(ch)) {
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0 || ch == '.') {
            content += ch;
        }
    }
    if (content.size() < kCellCount) {
        return false;
    }

    for (std::size_t i = 0; i < kCellCount; ++i) {
        const char value = content[i];
        const int digit = value >= '1' && value <= '9' ? value - '0' : 0;
        s.given[i] = digit;
        s.value[i] = digit;
        s.notes[i] = 0;
    }
    return true;
}

void load_puzzle(SudokuState &s)
{
    s.loaded_from_file =
        read_puzzle_from_file(s, "01_sudoku_puzzle.txt") ||
        read_puzzle_from_file(s, "demo/01_sudoku_puzzle.txt");
    if (s.loaded_from_file) {
        return;
    }

    const char *fallback = "530070000"
                           "600195000"
                           "098000060"
                           "800060003"
                           "400803001"
                           "700020006"
                           "060000280"
                           "000419005"
                           "000080079";
    for (std::size_t i = 0; i < kCellCount; ++i) {
        const char value = fallback[i];
        const int digit = value >= '1' && value <= '9' ? value - '0' : 0;
        s.given[i] = digit;
        s.value[i] = digit;
        s.notes[i] = 0;
    }
}

void draw_text_centered(const Rect &rect, const char *text, int size)
{
    const int text_width = bgt_text_width(text, size);
    const int text_height = bgt_text_height(text, size);
    bgt_draw_text(rect.x + ((rect.width - text_width) / 2),
                  rect.y + ((rect.height - text_height) / 2), text, size);
}

void draw_label(int x, int y, const char *text)
{
    bgt_set_color(BGT_DARK_GRAY);
    bgt_draw_text(x, y, text, kHelpFontSize);
}

void draw_button(const Rect &rect, const char *text, bool active, bool disabled)
{
    bgt_set_color(active ? kColorButtonActive : kColorButton);
    bgt_fill_rect(rect.x, rect.y, rect.width, rect.height);
    bgt_set_line_width(kMediumLineWidth);
    bgt_set_color(kColorButtonBorder);
    bgt_draw_rect(rect.x, rect.y, rect.width, rect.height);
    bgt_set_color(disabled ? kColorDisabled : BGT_BLACK);
    draw_text_centered(rect, text, kButtonFontSize);
}

bool point_in_rect(int px, int py, const Rect &rect)
{
    return px >= rect.x && px < rect.x + rect.width && py >= rect.y &&
           py < rect.y + rect.height;
}

std::string digit_text(int digit)
{
    std::string text;
    text += static_cast<char>('0' + digit);
    return text;
}

void select_cell_from_mouse(SudokuState &s, int mouse_x, int mouse_y)
{
    if (!point_in_rect(mouse_x, mouse_y, board_rect())) {
        return;
    }
    s.selected_col = (mouse_x - kBoardX) / kCellSize;
    s.selected_row = (mouse_y - kBoardY) / kCellSize;
}

void place_digit(SudokuState &s, int digit)
{
    const int index = cell_index(s.selected_row, s.selected_col);
    if (s.given[static_cast<std::size_t>(index)] != 0) {
        return;
    }

    if (s.note_mode) {
        if (s.value[static_cast<std::size_t>(index)] == 0) {
            const int mask = 1 << digit;
            s.notes[static_cast<std::size_t>(index)] ^= mask;
        }
    }
    else {
        s.value[static_cast<std::size_t>(index)] = digit;
        s.notes[static_cast<std::size_t>(index)] = 0;
    }
}

void clear_selected_cell(SudokuState &s)
{
    const int index = cell_index(s.selected_row, s.selected_col);
    if (s.given[static_cast<std::size_t>(index)] != 0) {
        return;
    }
    s.value[static_cast<std::size_t>(index)] = 0;
    s.notes[static_cast<std::size_t>(index)] = 0;
}

int selected_value(const SudokuState &s)
{
    return s.value[static_cast<std::size_t>(
        cell_index(s.selected_row, s.selected_col))];
}

void draw_cell_backgrounds(const SudokuState &s)
{
    const int chosen_value = selected_value(s);
    for (int row = 0; row < kGridSize; ++row) {
        for (int col = 0; col < kGridSize; ++col) {
            unsigned color = kColorBoard;
            const bool selected =
                row == s.selected_row && col == s.selected_col;
            const bool related =
                row == s.selected_row || col == s.selected_col ||
                same_box(row, col, s.selected_row, s.selected_col);
            const int value =
                s.value[static_cast<std::size_t>(cell_index(row, col))];
            if (related) {
                color = kColorRelated;
            }
            if (chosen_value != 0 && value == chosen_value) {
                color = kColorSameNumber;
            }
            if (selected) {
                color = kColorSelected;
            }

            const Rect cell = cell_rect(row, col);
            bgt_set_color(color);
            bgt_fill_rect(cell.x, cell.y, cell.width, cell.height);
        }
    }
}

void draw_notes(const Rect &cell, int mask)
{
    bgt_set_color(kColorNoteText);
    for (int digit = kMinDigit; digit <= kMaxDigit; ++digit) {
        if ((mask & (1 << digit)) == 0) {
            continue;
        }
        const int note_col = (digit - kMinDigit) % kBoxSize;
        const int note_row = (digit - kMinDigit) / kBoxSize;
        const std::string text = digit_text(digit);
        const Rect note{cell.x + kNoteInset + (note_col * kNoteSlotSize),
                        cell.y + kNoteInset + (note_row * kNoteSlotSize),
                        kNoteSlotSize, kNoteSlotSize};
        draw_text_centered(note, text.c_str(), kNoteFontSize);
    }
}

void draw_cell_values(const SudokuState &s)
{
    for (int row = 0; row < kGridSize; ++row) {
        for (int col = 0; col < kGridSize; ++col) {
            const int index = cell_index(row, col);
            Rect cell = cell_rect(row, col);
            const int value = s.value[static_cast<std::size_t>(index)];
            if (value != 0) {
                const std::string text = digit_text(value);
                bgt_set_color(s.given[static_cast<std::size_t>(index)] != 0
                                  ? kColorPresetText
                                  : kColorUserText);
                cell.y += kCellDigitYOffset;
                draw_text_centered(cell, text.c_str(), kCellDigitFontSize);
            }
            else {
                draw_notes(cell, s.notes[static_cast<std::size_t>(index)]);
            }
        }
    }
}

void draw_grid()
{
    for (int line = 0; line <= kGridSize; ++line) {
        const bool thick = line % kBoxSize == 0;
        bgt_set_line_width(thick ? kThickLineWidth : kThinLineWidth);
        bgt_set_color(thick ? kColorThickGrid : kColorGrid);
        const int offset = line * kCellSize;
        bgt_draw_line(kBoardX + offset, kBoardY, kBoardX + offset,
                      kBoardY + kBoardSize);
        bgt_draw_line(kBoardX, kBoardY + offset, kBoardX + kBoardSize,
                      kBoardY + offset);
    }
    bgt_set_line_width(kThinLineWidth);
}

void draw_board(const SudokuState &s)
{
    const Rect board = board_rect();
    bgt_set_color(kColorBoard);
    bgt_fill_rect(board.x, board.y, board.width, board.height);
    draw_cell_backgrounds(s);
    draw_cell_values(s);
    draw_grid();
}

void draw_panel(const SudokuState &s)
{
    const Rect panel = panel_rect();
    bgt_set_color(kColorPanel);
    bgt_fill_rect(panel.x, panel.y, panel.width, panel.height);
    bgt_set_color(kColorThickGrid);
    bgt_set_line_width(kMediumLineWidth);
    bgt_draw_rect(panel.x, panel.y, panel.width, panel.height);

    bgt_set_color(kColorThickGrid);
    bgt_draw_text(kPanelInnerX, kTitleY, "数独示例", kTitleFontSize);
    draw_label(kPanelInnerX, kHelpFirstY, "鼠标选择格子，键盘或按钮填数");
    draw_label(kPanelInnerX, kHelpFirstY + kHelpLineGap,
               "Tab/Space 切换填数/标记");
    draw_label(kPanelInnerX, kHelpFirstY + (kHelpLineGap * 2),
               "0 或退格清除当前格");

    draw_button(fill_mode_button_rect(), "填数", !s.note_mode, false);
    draw_button(note_mode_button_rect(), "标记", s.note_mode, false);

    for (int digit = kMinDigit; digit <= kMaxDigit; ++digit) {
        const std::string text = digit_text(digit);
        draw_button(digit_button_rect(digit), text.c_str(), false, false);
    }

    draw_button(clear_button_rect(), "清除当前格", false,
                s.given[static_cast<std::size_t>(
                    cell_index(s.selected_row, s.selected_col))] != 0);

    bgt_set_color(BGT_DARK_GRAY);
    bgt_draw_text(kPanelInnerX, kPuzzleLabelY,
                  s.loaded_from_file ? "谜题：01_sudoku_puzzle.txt"
                                     : "谜题：内置备用题目",
                  kPuzzleFontSize);
}

void handle_keyboard(SudokuState &s)
{
    if (bgt_key_just_pressed(BGT_KEY_LEFT)) {
        s.selected_col =
            s.selected_col > 0 ? s.selected_col - 1 : kGridSize - 1;
    }
    if (bgt_key_just_pressed(BGT_KEY_RIGHT)) {
        s.selected_col = (s.selected_col + 1) % kGridSize;
    }
    if (bgt_key_just_pressed(BGT_KEY_UP)) {
        s.selected_row =
            s.selected_row > 0 ? s.selected_row - 1 : kGridSize - 1;
    }
    if (bgt_key_just_pressed(BGT_KEY_DOWN)) {
        s.selected_row = (s.selected_row + 1) % kGridSize;
    }
    if (bgt_key_just_pressed(BGT_KEY_SPACE) ||
        bgt_key_just_pressed(BGT_KEY_TAB)) {
        s.note_mode = !s.note_mode;
    }
    if (bgt_key_just_pressed(BGT_KEY_BACKSPACE) ||
        bgt_key_just_pressed(BGT_KEY_0)) {
        clear_selected_cell(s);
    }

    for (int digit = kMinDigit; digit <= kMaxDigit; ++digit) {
        if (bgt_key_just_pressed(BGT_KEY_0 + digit)) {
            place_digit(s, digit);
        }
    }
}

void handle_panel_click(SudokuState &s, int mouse_x, int mouse_y)
{
    if (point_in_rect(mouse_x, mouse_y, fill_mode_button_rect())) {
        s.note_mode = false;
        return;
    }
    if (point_in_rect(mouse_x, mouse_y, note_mode_button_rect())) {
        s.note_mode = true;
        return;
    }
    if (point_in_rect(mouse_x, mouse_y, clear_button_rect())) {
        clear_selected_cell(s);
        return;
    }

    for (int digit = kMinDigit; digit <= kMaxDigit; ++digit) {
        if (point_in_rect(mouse_x, mouse_y, digit_button_rect(digit))) {
            place_digit(s, digit);
            return;
        }
    }
}

void handle_mouse(SudokuState &s)
{
    if (!bgt_mouse_just_pressed(BGT_MOUSE_LEFT)) {
        return;
    }
    const int mouse_x = bgt_mouse_x();
    const int mouse_y = bgt_mouse_y();
    select_cell_from_mouse(s, mouse_x, mouse_y);
    handle_panel_click(s, mouse_x, mouse_y);
}

} // namespace

int main()
{
    SudokuState sudoku;
    load_puzzle(sudoku);

    if (!bgt_open_window(kWindowWidth, kWindowHeight, "libbgt sudoku")) {
        bgt_print_error();
        return 1;
    }

    bgt_set_fps_limit(kTargetFps);
    bgt_set_background(kColorBackground);

    while (bgt_window_is_open()) {
        handle_keyboard(sudoku);
        handle_mouse(sudoku);

        draw_board(sudoku);
        draw_panel(sudoku);
        bgt_update_window();
    }

    bgt_close_window();
    return 0;
}

// NOLINTEND(readability-magic-numbers, readability-identifier-length,
// bugprone-easily-swappable-parameters)
