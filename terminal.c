#include "terminal.h"
#include <stddef.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static terminal_t terminal;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// 关闭 VGA 硬件光标，避免屏幕闪烁（某些显示器对光标闪烁敏感）
static void terminal_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

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

// 初始化终端：
// - 设置游标位置、默认颜色与缓冲区指针
// - 关闭硬件光标以减少视觉闪烁
// - 将整个 80x25 文本缓冲区清为背景色空格
void terminal_initialize(void) {
    terminal.row = 0;
    terminal.column = 0;
    terminal.color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal.buffer = VGA_MEMORY;
    terminal_disable_cursor();
    
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

// 在指定位置放置字符：
// - 直接计算线性索引 y*WIDTH+x 并写入字符与颜色
// - VGA 文本模式每个单元为 2 字节：低字节字符，高字节颜色
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal.buffer[index] = vga_entry(c, color);
}

// 处理换行：
// - 列归零，行+1；到达底部后从顶部重新开始（简单环形，不滚屏）
// - 如需滚屏，可改为上移一行并清空最后一行
static void terminal_newline(void) {
    terminal.column = 0;
    if (++terminal.row == VGA_HEIGHT) {
        terminal.row = 0;
    }
}

// 输出单个字符：
// - '\n'：调用换行处理
// - '\b'：行内退格，向左移动并将当前位置擦为背景色空格
// - 普通字符：写入当前位置并推进列；行末自动换行
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    if (c == '\b') {
        // 行内退格：向左移动并清空该位置，不跨行处理
        if (terminal.column > 0) {
            terminal.column--;
            terminal_putentryat(' ', terminal.color, terminal.column, terminal.row);
        }
        return;
    }
    
    terminal_putentryat(c, terminal.color, terminal.column, terminal.row);
    if (++terminal.column == VGA_WIDTH) {
        terminal_newline();
    }
}

// 写入字符串：
// - 连续调用 terminal_putchar 写入 size 个字符
// - 保持逐字符语义，便于处理控制字符（如 \n、\b）
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

// 写入以 null 结尾的字符串：
// - 通过 strlen 计算长度后调用 terminal_write
// - 与 printf 不同，不解析格式，仅原样输出
void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}
