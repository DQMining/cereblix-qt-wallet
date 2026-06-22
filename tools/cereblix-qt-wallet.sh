#!/usr/bin/env bash
# Launcher for the portable Linux build (same folder as the binary).
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$DIR/cereblix-qt-wallet"

if [[ ! -x "$BIN" ]]; then
    echo "cereblix-qt-wallet: binary not found at $BIN" >&2
    exit 1
fi

if [[ -d "$DIR/plugins" ]]; then
    export QT_PLUGIN_PATH="$DIR/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
fi

exec "$BIN" "$@"
