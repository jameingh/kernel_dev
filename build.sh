#!/bin/bash
set -e

# 编译引导扇区
nasm -f bin boot.asm -o boot.bin

# 编译汇编文件
nasm -f elf32 gdt.asm -o gdt.o
nasm -f elf32 isr.asm -o isr.o

# 编译C文件
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c kernel.c -o kernel.o
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c terminal.c -o terminal.o
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c gdt.c -o gdt_c.o
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c idt.c -o idt.o
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c interrupts.c -o interrupts.o

# 链接所有目标文件
x86_64-elf-ld -m elf_i386 -T linker.ld -o kernel.elf \
    kernel.o terminal.o gdt.o gdt_c.o isr.o idt.o interrupts.o

# 提取纯二进制代码
x86_64-elf-objcopy -O binary kernel.elf kernel.bin

# 创建镜像（需要更多扇区）
dd if=/dev/zero of=os.img bs=512 count=20 conv=notrunc 2>/dev/null
dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc 2>/dev/null
dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc 2>/dev/null

echo "Build successful! Running QEMU..."
qemu-system-i386 -drive file=os.img,format=raw,if=ide
