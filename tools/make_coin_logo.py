"""Build transparent circular Cereblix coin icons from the logo source image."""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
ASSETS = Path(
    r"C:\Users\DREADQUILL11168\.cursor\projects\c-CRB-QT-Wallet\assets"
)
SRC_COIN = ASSETS / (
    "c__Users_DREADQUILL11168_AppData_Roaming_Cursor_User_workspaceStorage_"
    "914e84bccb8cd795560a3d8902886838_images_Cereblix_logo-a9f085be-"
    "d39d-433c-a705-015187d2aff9.png"
)
SRC_SCREENSHOT = ASSETS / (
    "c__Users_DREADQUILL11168_AppData_Roaming_Cursor_User_workspaceStorage_"
    "914e84bccb8cd795560a3d8902886838_images_Cereblix_logo-35cc1074-"
    "205a-4148-9b3a-96d1b8790d83.png"
)
SCREENSHOT_ICON_BOX = (484, 108, 558, 182)
OUT_DIR = ROOT / "qt-wallet" / "resources" / "images"
WORK_SIZE = 512


def _luminance(r: int, g: int, b: int) -> float:
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def _saturation(r: int, g: int, b: int) -> float:
    mx, mn = max(r, g, b), min(r, g, b)
    return (mx - mn) / max(mx, 1)


def _is_matte_background(r: int, g: int, b: int) -> bool:
    lum = _luminance(r, g, b)
    sat = _saturation(r, g, b)
    if lum < 42 and sat < 0.16:
        return True
    return False


def _is_gradient_pixel(r: int, g: int, b: int) -> bool:
    lum = _luminance(r, g, b)
    sat = _saturation(r, g, b)
    return lum > 50 and sat > 0.10


def _coin_center_and_radius(rgba: Image.Image) -> tuple[float, float, float]:
    w, h = rgba.size
    px = rgba.load()
    total_x = total_y = total_w = 0.0
    max_r = 0.0

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 16 or _is_matte_background(r, g, b):
                continue
            if not _is_gradient_pixel(r, g, b):
                continue
            weight = max(_luminance(r, g, b), 24.0)
            total_x += x * weight
            total_y += y * weight
            total_w += weight
            max_r = max(max_r, math.hypot(x - w / 2.0, y - h / 2.0))

    if total_w <= 0:
        return w / 2.0, h / 2.0, min(w, h) / 2.0 - 2.0

    cx = total_x / total_w
    cy = total_y / total_w
    return cx, cy, max(1.0, max_r - 2.5)


def trim_round_logo(src: Image.Image) -> Image.Image:
    rgba = src.convert("RGBA")
    w, h = rgba.size
    cx, cy, radius = _coin_center_and_radius(rgba)

    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    src_px = rgba.load()
    out_px = out.load()

    for y in range(h):
        for x in range(w):
            r, g, b, a = src_px[x, y]
            if a < 16 or _is_matte_background(r, g, b):
                continue
            if math.hypot(x - cx, y - cy) > radius:
                continue
            out_px[x, y] = (r, g, b, 255)

    pad = 0
    left = max(0, int(cx - radius) - pad)
    top = max(0, int(cy - radius) - pad)
    right = min(w, int(cx + radius) + pad + 1)
    bottom = min(h, int(cy + radius) + pad + 1)
    return out.crop((left, top, right, bottom))


def defringe_circular(img: Image.Image, inset: float = 0.05) -> Image.Image:
    """Remove dark resize halos and feather the outer rim."""
    rgba = img.convert("RGBA")
    w, h = rgba.size
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    radius = min(w, h) / 2.0
    px = rgba.load()

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 1:
                continue

            dist = math.hypot(x - cx, y - cy)
            norm = dist / radius
            lum = _luminance(r, g, b)
            sat = _saturation(r, g, b)

            if norm > 1.0:
                px[x, y] = (0, 0, 0, 0)
                continue

            # Drop dark outer ring / black scaling artifacts (keep center helix).
            if norm > 0.60 and lum < 90 and sat < 0.40:
                px[x, y] = (0, 0, 0, 0)
                continue
            if norm > 0.55 and r < 24 and g < 24 and b < 30:
                px[x, y] = (0, 0, 0, 0)
                continue

            if norm > 1.0 - inset:
                fade = max(0.0, 1.0 - (norm - (1.0 - inset)) / inset)
                px[x, y] = (r, g, b, int(a * fade))

    return rgba


def polish_logo(img: Image.Image) -> Image.Image:
    big = img.resize((WORK_SIZE, WORK_SIZE), Image.Resampling.LANCZOS)
    big = defringe_circular(big, inset=0.045)
    big = big.filter(ImageFilter.SMOOTH_MORE)
    return defringe_circular(big, inset=0.04)


def load_source_emblem() -> Image.Image:
    if SRC_COIN.exists():
        emblem = trim_round_logo(Image.open(SRC_COIN))
    elif SRC_SCREENSHOT.exists():
        emblem = Image.open(SRC_SCREENSHOT).convert("RGB").crop(SCREENSHOT_ICON_BOX)
        trim = max(1, emblem.height // 18)
        emblem = trim_round_logo(emblem.crop((0, 0, emblem.width, emblem.height - trim)))
    else:
        raise SystemExit("No logo source image found in assets folder.")
    return polish_logo(emblem)


def export_logo(emblem: Image.Image, size: int) -> Image.Image:
    scaled = emblem.resize((size, size), Image.Resampling.LANCZOS)
    scaled = defringe_circular(scaled, inset=0.07)

    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, size - 1, size - 1), fill=255)
    scaled.putalpha(ImageChops.darker(scaled.split()[3], mask))
    return scaled


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    emblem = load_source_emblem()

    outputs = (
        (256, "cereblix-coin.png"),
        (64, "cereblix-coin-64.png"),
        (32, "cereblix-coin-32.png"),
    )
    for size, name in outputs:
        export_logo(emblem, size).save(OUT_DIR / name)
        print(f"Wrote {OUT_DIR / name}")

    icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    ico_images = [export_logo(emblem, s) for s, _ in icon_sizes]
    ico_images[0].save(
        OUT_DIR / "cereblix-coin.ico",
        format="ICO",
        sizes=icon_sizes,
        append_images=ico_images[1:],
    )
    print(f"Wrote {OUT_DIR / 'cereblix-coin.ico'}")


if __name__ == "__main__":
    main()
