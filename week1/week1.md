
```bash
brew install qemu
brew install i386-elf-gcc
brew --prefix i386-elf-gcc
x86_64-elf-gcc --version
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c main.c -o main.o
file main.o
```bash