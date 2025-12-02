#!/bin/bash
nasm -f bin boot.asm -o boot.bin
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c kernel.c -o kernel.o
x86_64-elf-ld -m elf_i386 -T linker.ld -o kernel.elf kernel.o
x86_64-elf-objcopy -O binary kernel.elf kernel.bin
cat boot.bin kernel.bin > os.img
# 填充镜像到至少11个扇区（5632字节），确保有足够数据供bootloader读取
dd if=/dev/zero of=os.img bs=512 count=11 conv=notrunc 2>/dev/null
dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc 2>/dev/null
dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc 2>/dev/null
qemu-system-i386 -hda os.img