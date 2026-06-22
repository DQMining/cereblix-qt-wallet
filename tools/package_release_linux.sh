#!/usr/bin/env bash
# Package a Linux x64 release tarball with launcher script.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT/dist"
STAGE="$OUT_DIR/cereblix-qt-wallet-linux-x64"
ARCHIVE="$OUT_DIR/cereblix-qt-wallet-linux-x64.tar.gz"
BIN="$ROOT/qt-wallet/build/cereblix-qt-wallet"

"$ROOT/tools/build-linux.sh"

if [[ ! -x "$BIN" ]]; then
    echo "Build output missing: $BIN" >&2
    exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp "$BIN" "$STAGE/"
cp "$ROOT/tools/cereblix-qt-wallet.sh" "$STAGE/"
chmod +x "$STAGE/cereblix-qt-wallet" "$STAGE/cereblix-qt-wallet.sh"

cat >"$STAGE/README.txt" <<'EOF'
Cereblix Qt Wallet — Linux x64

Quick start:
  tar -xzf cereblix-qt-wallet-linux-x64.tar.gz
  cd cereblix-qt-wallet-linux-x64
  ./cereblix-qt-wallet.sh

Runtime dependencies (Ubuntu/Debian):
  sudo apt install libqt6widgets6 libqt6network6 libsodium23

Wallet file: ~/.cereblix/wallet.json (same as the Go CLI wallet)
EOF

mkdir -p "$OUT_DIR"
rm -f "$ARCHIVE"
tar -czf "$ARCHIVE" -C "$OUT_DIR" "$(basename "$STAGE")"
echo "Created: $ARCHIVE"
