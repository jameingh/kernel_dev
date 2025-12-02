#include "terminal.h"

void kmain(void) {
    // 初始化终端
    terminal_initialize();
    
    // 设置颜色为绿色
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    
    // 显示欢迎信息
    terminal_writestring("========================================\n");
    terminal_writestring("    Welcome to MyOS Kernel v1.0!\n");
    terminal_writestring("========================================\n\n");
    
    // 设置颜色为白色
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    // 显示系统信息
    terminal_writestring("System Information:\n");
    terminal_writestring("  - VGA Text Mode: 80x25\n");
    terminal_writestring("  - Memory Address: 0xB8000\n");
    terminal_writestring("  - Status: RUNNING\n\n");
    
    // 彩色演示
    terminal_writestring("Color Demo:\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
    terminal_writestring("  Red Text\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("  Green Text\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
    terminal_writestring("  Blue Text\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("  Yellow Text\n");
    
    // 恢复默认颜色
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\nKernel initialization complete!\n");
    
    // 无限循环
    while (1) {
        // 在这里可以添加其他功能
    }
}
