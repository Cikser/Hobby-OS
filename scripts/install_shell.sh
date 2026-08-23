#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Determine the project root directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBC_DIR="$PROJECT_DIR/libc"
SHELL_DIR="$PROJECT_DIR/shell/busybox-src"
TARGET_DIR="$PROJECT_DIR/shell"

echo "==> Starting BusyBox build and setup..."

# 1. Check if the RISC-V musl toolchain exists
if [ ! -f "$LIBC_DIR/bin/riscv64-linux-musl-gcc" ]; then
  echo "Error: Compiler not found at: $LIBC_DIR/bin/riscv64-linux-musl-gcc"
  exit 1
fi

# Add musl toolchain to PATH so BusyBox sub-makes can find it
export PATH="$LIBC_DIR/bin:$PATH"

# 2. Clone BusyBox repository into ./shell/busybox-src using official GitHub mirror
if [ ! -d "$SHELL_DIR/.git" ]; then
  echo "==> Cloning BusyBox repository from GitHub mirror into $SHELL_DIR..."
  git clone --depth 1 --branch 1_36_1 https://github.com/mirror/busybox.git "$SHELL_DIR"
fi

cd "$SHELL_DIR"

# 3. Generate minimal clean baseline configuration
echo "==> Generating minimal baseline configuration (allnoconfig)..."
make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" allnoconfig

# 4. Apply minimal set of applets and STRICT NON-PIE STATIC config
echo "==> Applying static link and non-PIE options to .config..."

# Remove any existing PIE/STATIC lines to avoid duplicates
sed -i '/CONFIG_PIE/d' .config
sed -i '/CONFIG_STATIC/d' .config
sed -i '/CONFIG_BUILD_LIBBUSYBOX/d' .config

cat << 'EOF' >> .config
# Force Static, non-PIE Link
CONFIG_STATIC=y
# CONFIG_PIE is not set
# CONFIG_BUILD_LIBBUSYBOX is not set

CONFIG_CROSS_COMPILER_PREFIX="riscv64-linux-musl-"
CONFIG_SHOW_USAGE=y
CONFIG_FEATURE_VERBOSE_USAGE=y

# Shell (ASH)
CONFIG_ASH=y
CONFIG_ASH_OPTIMIZE_FOR_SIZE=y
CONFIG_ASH_INTERNAL_GLOB=y
CONFIG_ASH_BASH_COMPAT=y

# Base Utilities (Phase 1)
CONFIG_LS=y
CONFIG_CAT=y
CONFIG_ECHO=y
CONFIG_MKDIR=y
CONFIG_RM=y
CONFIG_TOUCH=y
CONFIG_PWD=y
CONFIG_TRUE=y
CONFIG_FALSE=y
EOF

# Update config non-interactively
echo "==> Resolving configuration dependencies..."
yes "" | make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" oldconfig

# 5. Compile with strict Non-PIE static flags
echo "==> Compiling BusyBox statically (no-PIE)..."
make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" \
     CFLAGS="-march=rv64gc -mabi=lp64d -fno-pie -fno-PIC" \
     CONFIG_EXTRA_CFLAGS="-march=rv64gc -mabi=lp64d -fno-pie -fno-PIC" \
     CONFIG_EXTRA_LDFLAGS="-static -no-pie" \
     -j"$(nproc)"

# 6. Copy the compiled binary to ./shell/busybox
if [ -f "busybox" ]; then
  cp busybox "$TARGET_DIR/shell"
  echo "==> Build successful! Binary located at: $TARGET_DIR/shell"
else
  echo "Error: BusyBox binary was not compiled successfully."
  exit 1
fi