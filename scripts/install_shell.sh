#!/usr/bin/env bash

# Prekini izvršavanje skripte ako bilo koja komanda vrati grešku
set -e

# Određivanje korenske putanje projekta
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBC_DIR="$PROJECT_DIR/libc"
SHELL_DIR="$PROJECT_DIR/shell"

echo "==> Pokretanje pripreme i instalacije Dash shell-a..."

# 1. Provera da li postoji RISC-V musl toolchain
if [ ! -f "$LIBC_DIR/bin/riscv64-linux-musl-gcc" ]; then
  echo "Greška: Kompajler nije pronađen na putanji: $LIBC_DIR/bin/riscv64-linux-musl-gcc"
  exit 1
fi

# 2. Kloniranje izvornog koda direktno u ./shell direktorijum
if [ ! -d "$SHELL_DIR/.git" ]; then
  echo "==> Kloniranje dash spremišta u $SHELL_DIR..."
  git clone https://git.kernel.org/pub/scm/utils/dash/dash.git "$SHELL_DIR"
fi

cd "$SHELL_DIR"

# 3. Postavljanje odgovarajuće verzije
echo "==> Prebacivanje na tag v0.5.12..."
git checkout v0.5.12

# 4. Generisanje autotools konfiguracionih fajlova
echo "==> Pokretanje autogen.sh..."
./autogen.sh

# 5. Konfigurisanje build okruženja za cross-compilation
echo "==> Konfigurisanje projekta..."
CC="$LIBC_DIR/bin/riscv64-linux-musl-gcc" \
CFLAGS="-march=rv64gc -mabi=lp64d" \
LDFLAGS="-static" \
./configure --host=riscv64-linux-musl --disable-nls --without-libedit

# 6. Kompajliranje
echo "==> Kompajliranje (make)..."
make -j"$(nproc)"

# 7. Premeštanje kompajliranog fajla u koren ./shell direktorijuma
# (pošto ga make generiše unutar shell/src/dash)
if [ -f "src/dash" ]; then
  mv src/dash ./dash
fi

echo "==> Uspešno završeno! Binarni fajl se nalazi na: $SHELL_DIR/dash"