#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_tool() {
    if ! command -v "$1" &>/dev/null; then
        echo -e "${RED}Missing: $1${NC}"
        return 1
    fi
    return 0
}

install_cross_compiler() {
    echo -e "${YELLOW}Installing i686-elf cross-compiler...${NC}"

    if command -v apt-get &>/dev/null; then
        sudo apt-get update
        sudo apt-get install -y build-essential bison flex libgmp3-dev \
            libmpc-dev libmpfr-dev texinfo nasm grub-pc-bin \
            grub-common xorriso mtools qemu-system-x86
    elif command -v pacman &>/dev/null; then
        sudo pacman -Sy --noconfirm base-devel gmp libmpc mpfr nasm \
            grub xorriso mtools qemu-system-x86
    fi

    if ! command -v i686-elf-gcc &>/dev/null; then
        echo -e "${YELLOW}Building cross-compiler from source (this takes ~15 min)...${NC}"
        CROSS_PREFIX="$HOME/opt/cross"
        TARGET=i686-elf
        export PATH="$CROSS_PREFIX/bin:$PATH"

        mkdir -p /tmp/cross-build && cd /tmp/cross-build

        BINUTILS_VER=2.41
        GCC_VER=13.2.0

        if [ ! -f "binutils-${BINUTILS_VER}.tar.xz" ]; then
            wget "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.xz"
        fi
        if [ ! -f "gcc-${GCC_VER}.tar.xz" ]; then
            wget "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz"
        fi

        tar xf "binutils-${BINUTILS_VER}.tar.xz"
        mkdir -p build-binutils && cd build-binutils
        "../binutils-${BINUTILS_VER}/configure" --target=$TARGET --prefix="$CROSS_PREFIX" \
            --with-sysroot --disable-nls --disable-werror
        make -j$(nproc)
        make install
        cd ..

        tar xf "gcc-${GCC_VER}.tar.xz"
        mkdir -p build-gcc && cd build-gcc
        "../gcc-${GCC_VER}/configure" --target=$TARGET --prefix="$CROSS_PREFIX" \
            --disable-nls --enable-languages=c --without-headers
        make -j$(nproc) all-gcc all-target-libgcc
        make install-gcc install-target-libgcc
        cd ..

        echo -e "${GREEN}Cross-compiler installed to $CROSS_PREFIX${NC}"
        echo "Add to PATH: export PATH=\"$CROSS_PREFIX/bin:\$PATH\""
    fi
}

echo -e "${GREEN}=== Claude OS Build System ===${NC}"
echo ""

MISSING=0
check_tool nasm       || MISSING=1
check_tool grub-mkrescue || MISSING=1
check_tool xorriso    || MISSING=1
check_tool i686-elf-gcc || {
    if check_tool gcc; then
        echo -e "${YELLOW}Using system gcc with -m32 (cross-compiler recommended)${NC}"
        export CC="gcc -m32 -ffreestanding"
        export LD="ld -m elf_i386"
        sed -i 's/i686-elf-gcc/gcc -m32/g' Makefile
        sed -i 's/i686-elf-ld/ld -m elf_i386/g' Makefile
    else
        MISSING=1
    fi
}

if [ $MISSING -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}Install missing tools? [y/N]${NC}"
    read -r ans
    if [ "$ans" = "y" ] || [ "$ans" = "Y" ]; then
        install_cross_compiler
    else
        echo -e "${RED}Cannot build without required tools.${NC}"
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}Building Claude OS...${NC}"
make clean 2>/dev/null || true
make all

if [ -f claudeos.iso ]; then
    echo ""
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "Output: ${YELLOW}claudeos.iso${NC}"
    echo ""
    echo "To test in QEMU:"
    echo "  qemu-system-i386 -cdrom claudeos.iso -m 128M"
    echo ""
    echo "To burn to USB (replace /dev/sdX):"
    echo "  sudo dd if=claudeos.iso of=/dev/sdX bs=4M status=progress"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
