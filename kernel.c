// VGA文本模式的显存地址
volatile char* VGA_BUFFER = (volatile char*) 0xB8000;

// 终端颜色
const char VGA_COLOR_WHITE = 15;
const char VGA_COLOR_BLACK = 0;

// 在指定位置打印一个字符
void terminal_putchar(char c, int col, int row, char color) {
    int offset = (row * 80 + col) * 2;
    VGA_BUFFER[offset] = c;
    VGA_BUFFER[offset + 1] = color;
}

// 打印一个字符串
void terminal_writestring(const char* str) {
    int i = 0;
    while (str[i] != 0) {
        terminal_putchar(str[i], i, 0, VGA_COLOR_WHITE);
        i++;
    }
}

// 内核主函数
void kmain() {
    terminal_writestring("Hello from C Kernel!");
}
