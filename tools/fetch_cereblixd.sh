#!/usr/bin/env bash
# Download cereblixd from https://github.com/CereblixCRB/cereblix/releases
set -euo pipefail

TAG="${CEREBLIXD_TAG:-v2.4.0}"
PLATFORM="${1:-linux-amd64}"
OUT_FILE="${2:?output path required}"

case "$PLATFORM" in
    windows)       ASSET="cereblixd-windows-amd64.exe" ;;
    linux-amd64) ASSET="cereblixd-linux-amd64" ;;
    linux-arm64) ASSET="cereblixd-linux-arm64" ;;
    *) echo "Unknown platform: $PLATFORM" >&2; exit 1 ;;
esac

URL="https://github.com/CereblixCRB/cereblix/releases/download/${TAG}/${ASSET}"
mkdir -p "$(dirname "$OUT_FILE")"

echo "Fetching cereblixd ${TAG} (${ASSET}) ..."
curl -fsSL "$URL" -o "$OUT_FILE"

if [[ ! -s "$OUT_FILE" ]] || [[ $(stat -c%s "$OUT_FILE" 2>/dev/null || stat -f%z "$OUT_FILE") -lt 1024 ]]; then
    echo "Download failed or file too small: $OUT_FILE" >&2
    exit 1
fi

chmod +x "$OUT_FILE" 2>/dev/null || true
echo "Saved: $OUT_FILE"
