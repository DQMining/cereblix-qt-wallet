#!/usr/bin/env bash
# Build Cereblix Qt Wallet on Linux (Ubuntu/Debian).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/qt-wallet"

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake is required. Install: sudo apt install cmake g++" >&2
    exit 1
fi

if ! pkg-config --exists Qt6Widgets 2>/dev/null; then
    echo "Qt 6 dev packages required. Install:" >&2
    echo "  sudo apt install qt6-base-dev qt6-base-dev-tools libsodium-dev" >&2
    exit 1
fi

if ! pkg-config --exists libsodium 2>/dev/null; then
    echo "libsodium dev required. Install: sudo apt install libsodium-dev" >&2
    exit 1
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc 2>/dev/null || echo 4)"
echo "Built: $ROOT/qt-wallet/build/cereblix-qt-wallet"
