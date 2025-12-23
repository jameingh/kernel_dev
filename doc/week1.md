
### 安装环境依赖

```bash
brew install qemu
brew install i386-elf-gcc
brew --prefix i386-elf-gcc
x86_64-elf-gcc --version
```

### 交叉编译main.c为一个32位的.o文件

```bash
x86_64-elf-gcc -m32 -ffreestanding -nostdlib -c main.c -o main.o
file main.o
```