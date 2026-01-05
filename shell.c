#include "shell.h"
#include "terminal.h"
#include "string.h"

#define CMD_BUF_SIZE 256

static char cmd_buffer[CMD_BUF_SIZE];
static int cmd_len = 0;

// 简单的端口输入输出，用于重启命令
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void shell_prompt() {
    terminal_writestring("root@myos /> ");
}

void shell_init() {
    terminal_writestring("\nWelcome to MyOS Shell!\n");
    terminal_writestring("Type 'help' for commands.\n");
    cmd_len = 0;
    shell_prompt();
}

void cmd_help() {
    terminal_writestring("Available commands:\n");
    terminal_writestring("  help     - Show this list\n");
    terminal_writestring("  clear    - Clear screen\n");
    terminal_writestring("  reboot   - Reboot system\n");
    terminal_writestring("  ls       - List files\n");
    terminal_writestring("  cat <f>  - Print file content\n");
}

void cmd_clear() {
    terminal_initialize();
}

void cmd_reboot() {
    terminal_writestring("Rebooting...\n");
    // 键盘控制器重启命令
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    asm volatile("hlt");
}

void cmd_ls() {
    // TODO: 链接 Week 10 的 VFS 接口
    terminal_writestring("Listing files:\n");
    terminal_writestring("  hello.txt\n");
    terminal_writestring("  kernel.bin\n");
    terminal_writestring("  (Mock data - VFS not linked)\n");
}

void cmd_cat(char* args) {
    if (!args || !*args) {
        terminal_writestring("Usage: cat <filename>\n");
        return;
    }
    terminal_writestring("Content of ");
    terminal_writestring(args);
    terminal_writestring(":\n");
    
    if (strcmp(args, "hello.txt") == 0) {
        terminal_writestring("Hello, World! This is a text file.\n");
    } else {
        terminal_writestring("File not found.\n");
    }
}

void shell_execute() {
    terminal_putchar('\n');
    
    if (cmd_len == 0) {
        shell_prompt();
        return;
    }

    cmd_buffer[cmd_len] = '\0';
    
    // 简单的命令解析：分离命令和参数
    char* cmd = cmd_buffer;
    char* args = 0;
    
    for (int i = 0; i < cmd_len; i++) {
        if (cmd_buffer[i] == ' ') {
            cmd_buffer[i] = '\0';
            args = &cmd_buffer[i + 1];
            // 跳过额外的空格
            while (*args == ' ') args++;
            break;
        }
    }

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cat") == 0) {
        cmd_cat(args);
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_putchar('\n');
    }

    cmd_len = 0;
    shell_prompt();
}

void shell_input(char c) {
    if (c == '\n') {
        shell_execute();
    } else if (c == '\b') {
        if (cmd_len > 0) {
            cmd_len--;
            // 视觉上的退格：退格 -> 空格 -> 退格
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
        }
    } else {
        if (cmd_len < CMD_BUF_SIZE - 1) {
            cmd_buffer[cmd_len++] = c;
            terminal_putchar(c);
        }
    }
}