#!/usr/bin/env bash
set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBC_DIR="$PROJECT_DIR/libc"
SHELL_DIR="$PROJECT_DIR/shell/busybox-src"
TARGET_DIR="$PROJECT_DIR/shell"

echo "==> Starting BusyBox build and setup..."

if [ ! -f "$LIBC_DIR/bin/riscv64-linux-musl-gcc" ]; then
  echo "Error: Compiler not found at: $LIBC_DIR/bin/riscv64-linux-musl-gcc"
  exit 1
fi

export PATH="$LIBC_DIR/bin:$PATH"

if [ ! -d "$SHELL_DIR/.git" ]; then
  echo "==> Cloning BusyBox repository from GitHub mirror into $SHELL_DIR..."
  git clone --depth 1 --branch 1_36_1 https://github.com/mirror/busybox.git "$SHELL_DIR"
fi

cd "$SHELL_DIR"

rm -f .config .config.old

echo "==> Generating allnoconfig baseline (nothing enabled)..."
make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" allnoconfig

echo "==> Enabling required applets..."

ENABLE_SYMS="
CONFIG_STATIC
CONFIG_SH_IS_ASH
CONFIG_SHELL_ASH
CONFIG_ASH
CONFIG_ASH_OPTIMIZE_FOR_SIZE
CONFIG_ASH_INTERNAL_GLOB
CONFIG_ASH_BASH_COMPAT
CONFIG_LS
CONFIG_CAT
CONFIG_ECHO
CONFIG_PRINTF
CONFIG_MKDIR
CONFIG_RM
CONFIG_RMDIR
CONFIG_TOUCH
CONFIG_PWD
CONFIG_TRUE
CONFIG_FALSE
CONFIG_CP
CONFIG_MV
CONFIG_STAT
CONFIG_CHMOD
CONFIG_KILL
CONFIG_SLEEP
CONFIG_TIME
CONFIG_GREP
CONFIG_HEAD
CONFIG_TAIL
CONFIG_WC
CONFIG_SORT
CONFIG_UNIQ
CONFIG_TR
CONFIG_CUT
CONFIG_CMP
CONFIG_LN
CONFIG_READLINK
CONFIG_TEE
CONFIG_XARGS
CONFIG_FIND
CONFIG_SED
CONFIG_AWK
CONFIG_DIFF
CONFIG_NL
CONFIG_DIRNAME
CONFIG_BASENAME
CONFIG_ENV
CONFIG_PRINTENV
CONFIG_TIMEOUT
CONFIG_NOHUP
CONFIG_TAR
CONFIG_FEATURE_TAR_CREATE
CONFIG_GZIP
CONFIG_GUNZIP
CONFIG_INSTALL_APPLET_SYMLINKS
"

for sym in $ENABLE_SYMS; do
  sed -i "/^# ${sym} is not set/d" .config
  sed -i "/^${sym}=/d" .config
  echo "${sym}=y" >> .config
done

echo "==> Resolving dependencies..."
yes "" | make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" oldconfig > /dev/null

echo "==> Verifying required applets are enabled..."
REQUIRED_SYMS="CONFIG_STATIC CONFIG_ASH CONFIG_ECHO CONFIG_CAT CONFIG_LS CONFIG_MKDIR CONFIG_TOUCH CONFIG_PWD CONFIG_TRUE CONFIG_FALSE CONFIG_RM CONFIG_CP CONFIG_MV"
MISSING=0
for sym in $REQUIRED_SYMS; do
  if ! grep -qxF "${sym}=y" .config; then
    echo "  MISSING: ${sym}"
    MISSING=1
  fi
done

if [ "$MISSING" -ne 0 ]; then
  echo "Error: one or more required applets are not enabled in .config."
  echo "Run: grep <SYMBOL> .config   to inspect why."
  exit 1
fi
echo "==> All required applets confirmed enabled."

echo "==> Compiling BusyBox statically (no-PIE)..."
make ARCH=riscv CROSS_COMPILE="riscv64-linux-musl-" \
     CFLAGS="-march=rv64gc -mabi=lp64d -fno-pie -fno-PIC" \
     CONFIG_EXTRA_CFLAGS="-march=rv64gc -mabi=lp64d -fno-pie -fno-PIC" \
     CONFIG_EXTRA_LDFLAGS="-static -no-pie" \
     -j"$(nproc)"

if [ -f "busybox_unstripped" ]; then
  cp busybox_unstripped "$TARGET_DIR/shell"
  echo "==> Build successful! Unstripped binary located at: $TARGET_DIR/shell"
elif [ -f "busybox" ]; then
  echo "Warning: busybox_unstripped not found, falling back to stripped binary."
  cp busybox "$TARGET_DIR/shell"
  echo "==> Build successful! Binary located at: $TARGET_DIR/shell"
else
  echo "Error: BusyBox binary was not compiled successfully."
  exit 1
fi