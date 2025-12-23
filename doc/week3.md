
### 构建工作流

```bash
nasm -f bin boot.asm -o boot.bin
x86_64-elf-gcc -m32 -ffreestanding -c kernel.c -o kernel.o
x86_64-elf-ld -m elf_i386 -T linker.ld -o kernel.bin kernel.o
cat boot.bin kernel.bin > os.img
qemu-system-i386 -hda os.img
```