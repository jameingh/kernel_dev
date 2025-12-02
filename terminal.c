#include "terminal.h"
#include <stddef.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static terminal_t terminal;

// 字符串长度函数
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

// 合并前景色和背景色
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// 创建VGA条目（字符 + 颜色）
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// 初始化终端
void terminal_initialize(void) {
    terminal.row = 0;
    terminal.column = 0;
    terminal.color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal.buffer = VGA_MEMORY;
    
    // 清屏
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal.buffer[index] = vga_entry(' ', terminal.color);
        }
    }
}

// 设置颜色
void terminal_setcolor(uint8_t color) {
    terminal.color = color;
}

// 在指定位置放置字符
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal.buffer[index] = vga_entry(c, color);
}

// 处理换行
static void terminal_newline(void) {
    terminal.column = 0;
    if (++terminal.row == VGA_HEIGHT) {
        terminal.row = 0;
    }
}

// 输出单个字符
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    
    terminal_putentryat(c, terminal.color, terminal.column, terminal.row);
    if (++terminal.column == VGA_WIDTH) {
        terminal_newline();
    }
}

// 写入字符串
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

// 写入以null结尾的字符串
void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}
