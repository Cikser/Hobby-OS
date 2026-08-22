#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Determine the project root directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBC_DIR="$PROJECT_DIR/libc"
SHELL_DIR="$PROJECT_DIR/shell"

echo "==> Starting Dash shell build and setup..."

# 1. Check if the RISC-V musl toolchain exists
if [ ! -f "$LIBC_DIR/bin/riscv64-linux-musl-gcc" ]; then
  echo "Error: Compiler not found at: $LIBC_DIR/bin/riscv64-linux-musl-gcc"
  exit 1
fi

# 2. Clone the repository directly into the ./shell directory if not present
if [ ! -d "$SHELL_DIR/.git" ]; then
  echo "==> Cloning dash repository into $SHELL_DIR..."
  git clone https://git.kernel.org/pub/scm/utils/dash/dash.git "$SHELL_DIR"
fi

cd "$SHELL_DIR"

# 3. Checkout the target version
echo "==> Checking out tag v0.5.12..."
git checkout v0.5.12

# 4. Generate autotools configuration files
echo "==> Running autogen.sh..."
./autogen.sh

# 5. Configure the build environment for cross-compilation
echo "==> Configuring build environment..."
CC="$LIBC_DIR/bin/riscv64-linux-musl-gcc" \
CFLAGS="-march=rv64gc -mabi=lp64d" \
LDFLAGS="-static" \
./configure --host=riscv64-linux-musl --disable-nls --without-libedit

# 6. Compile
echo "==> Compiling (make)..."
make -j"$(nproc)"

# 7. Move the compiled binary to the root of ./shell
if [ -f "src/dash" ]; then
  mv src/dash ./dash
fi

echo "==> Build successful! Binary located at: $SHELL_DIR/dash"