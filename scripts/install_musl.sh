#!/usr/bin/env bash
set -e

# Determine project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIBC_DIR="${PROJECT_ROOT}/libc"
MCM_DIR="${LIBC_DIR}/musl-cross-make"

GCC_VER="12.4.0"
BINUTILS_VER="2.44"
MPFR_VER="4.2.2"
MPC_VER="1.3.1"
GMP_VER="6.3.0"
MUSL_VER="git-v1.2.4"
TARGET="riscv64-linux-musl"

echo "=== Preparing environment in: ${LIBC_DIR} ==="
mkdir -p "${LIBC_DIR}"

# 1. Clone musl-cross-make if it does not exist
if [ ! -d "${MCM_DIR}/.git" ]; then
    echo "=== Cloning musl-cross-make ==="
    rm -rf "${MCM_DIR}"
    git clone --depth 1 https://github.com/richfelker/musl-cross-make.git "${MCM_DIR}"
else
    echo "=== musl-cross-make directory already exists, skipping clone ==="
fi

cd "${MCM_DIR}"
SOURCES_DIR="${MCM_DIR}/sources"
mkdir -p "${SOURCES_DIR}"

# Download function with fallback mirror support
download_tar() {
    local primary_url="$1"
    local fallback_url="$2"
    local archive="$3"
    
    if [ ! -f "${SOURCES_DIR}/${archive}" ]; then
        echo "Downloading: ${archive}..."
        if ! wget -4 --no-check-certificate -c --tries=3 --timeout=10 "${primary_url}" -O "${SOURCES_DIR}/${archive}"; then
            echo "Primary mirror unreachable, trying fallback URL..."
            wget -4 --no-check-certificate -c --tries=5 --timeout=15 "${fallback_url}" -O "${SOURCES_DIR}/${archive}"
        fi
    fi
}

echo "=== Checking and downloading source archives ==="
download_tar "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz" \
             "https://mirrors.kernel.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz" \
             "gcc-${GCC_VER}.tar.xz"

download_tar "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.gz" \
             "https://mirrors.kernel.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.gz" \
             "binutils-${BINUTILS_VER}.tar.gz"

download_tar "https://ftp.gnu.org/gnu/mpfr/mpfr-${MPFR_VER}.tar.xz" \
             "https://mirrors.kernel.org/gnu/mpfr/mpfr-${MPFR_VER}.tar.xz" \
             "mpfr-${MPFR_VER}.tar.xz"

download_tar "https://ftp.gnu.org/gnu/mpc/mpc-${MPC_VER}.tar.gz" \
             "https://mirrors.kernel.org/gnu/mpc/mpc-${MPC_VER}.tar.gz" \
             "mpc-${MPC_VER}.tar.gz"

download_tar "https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VER}.tar.xz" \
             "https://mirrors.kernel.org/gnu/gmp/gmp-${GMP_VER}.tar.xz" \
             "gmp-${GMP_VER}.tar.xz"

# Clean up any residual temporary files
rm -rf "${SOURCES_DIR}"/*.tmp "${MCM_DIR}"/*.tmp

echo "=== Extracting archives ==="
tar --no-same-owner -xf "${SOURCES_DIR}/gcc-${GCC_VER}.tar.xz" -C "${MCM_DIR}"
tar --no-same-owner -xf "${SOURCES_DIR}/binutils-${BINUTILS_VER}.tar.gz" -C "${MCM_DIR}"
tar --no-same-owner -xf "${SOURCES_DIR}/mpfr-${MPFR_VER}.tar.xz" -C "${MCM_DIR}"
tar --no-same-owner -xf "${SOURCES_DIR}/mpc-${MPC_VER}.tar.gz" -C "${MCM_DIR}"
tar --no-same-owner -xf "${SOURCES_DIR}/gmp-${GMP_VER}.tar.xz" -C "${MCM_DIR}"

# Touch marker files to prevent Make from triggering network downloads
touch "${SOURCES_DIR}/gcc-${GCC_VER}.tar.xz"
touch "${SOURCES_DIR}/binutils-${BINUTILS_VER}.tar.gz"
touch "${SOURCES_DIR}/mpfr-${MPFR_VER}.tar.xz"
touch "${SOURCES_DIR}/mpc-${MPC_VER}.tar.gz"
touch "${SOURCES_DIR}/gmp-${GMP_VER}.tar.xz"

# Link dependencies into the GCC source tree
ln -sf "../mpfr-${MPFR_VER}" "gcc-${GCC_VER}/mpfr"
ln -sf "../mpc-${MPC_VER}" "gcc-${GCC_VER}/mpc"
ln -sf "../gmp-${GMP_VER}" "gcc-${GCC_VER}/gmp"

echo "=== Starting build (jobs: $(nproc)) ==="
make -j$(nproc) \
  TARGET="${TARGET}" \
  GCC_VER="${GCC_VER}" \
  BINUTILS_VER="${BINUTILS_VER}" \
  MUSL_VER="${MUSL_VER}" \
  MUSL_REPO="https://git.musl-libc.org/git/musl" \
  OUTPUT="${LIBC_DIR}"

echo "=== Installing into ${LIBC_DIR} ==="
make install \
  TARGET="${TARGET}" \
  GCC_VER="${GCC_VER}" \
  BINUTILS_VER="${BINUTILS_VER}" \
  MUSL_VER="${MUSL_VER}" \
  MUSL_REPO="https://git.musl-libc.org/git/musl" \
  OUTPUT="${LIBC_DIR}"

echo "=== Cleaning temporary build directories ==="
rm -rf "${MCM_DIR}/gcc-${GCC_VER}" \
       "${MCM_DIR}/binutils-${BINUTILS_VER}" \
       "${MCM_DIR}/mpfr-${MPFR_VER}" \
       "${MCM_DIR}/mpc-${MPC_VER}" \
       "${MCM_DIR}/gmp-${GMP_VER}" \
       "${MCM_DIR}/build"

echo "=== Successfully installed! Toolchain located at: ${LIBC_DIR}/bin/${TARGET}-gcc ==="