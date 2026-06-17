#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "khicas/upstream/giac90_1addin"
SIZE = (92, 64)
SCALE = 4


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    names = (
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial Bold.ttf" if bold else "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    )
    for name in names:
        path = Path(name)
        if path.exists():
            return ImageFont.truetype(str(path), size * SCALE)
    return ImageFont.load_default()


def rect(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], fill=None, outline=None, width: int = 1) -> None:
    scaled = tuple(v * SCALE for v in xy)
    draw.rectangle(scaled, fill=fill, outline=outline, width=width * SCALE)


def line(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], fill, width: int = 1) -> None:
    draw.line(tuple(v * SCALE for v in xy), fill=fill, width=width * SCALE)


def text(draw: ImageDraw.ImageDraw, xy: tuple[int, int], value: str, fill, size: int, bold: bool = False) -> None:
    draw.text((xy[0] * SCALE, xy[1] * SCALE), value, fill=fill, font=font(size, bold=bold))


def quantized(image: Image.Image) -> Image.Image:
    small = image.resize(SIZE, Image.Resampling.LANCZOS)
    return small.convert("P", palette=Image.Palette.ADAPTIVE, colors=16).convert("RGB")


def draw_icon(selected: bool) -> Image.Image:
    bg_top = (244, 245, 248) if not selected else (46, 91, 160)
    bg_bottom = (176, 180, 190) if not selected else (18, 45, 94)
    shadow = (124, 128, 140) if not selected else (8, 24, 56)
    panel = (252, 252, 250) if not selected else (238, 244, 255)
    bezel = (46, 49, 57)
    ink = (24, 27, 34)
    blue = (0, 78, 190)
    pink = (238, 76, 221)
    grid = (35, 39, 48)

    image = Image.new("RGB", (SIZE[0] * SCALE, SIZE[1] * SCALE), bg_top)
    draw = ImageDraw.Draw(image)

    for y in range(SIZE[1] * SCALE):
        t = y / (SIZE[1] * SCALE - 1)
        color = tuple(round(bg_top[i] * (1 - t) + bg_bottom[i] * t) for i in range(3))
        draw.line((0, y, SIZE[0] * SCALE, y), fill=color)

    rect(draw, (16, 9, 80, 56), fill=shadow)
    rect(draw, (12, 5, 76, 52), fill=bezel, outline=(18, 20, 25), width=1)
    rect(draw, (16, 9, 72, 48), fill=panel)
    rect(draw, (16, 9, 20, 48), fill=pink)
    rect(draw, (20, 9, 72, 16), fill=(255, 255, 255), outline=(112, 116, 126), width=1)
    rect(draw, (62, 9, 72, 16), fill=blue)
    text(draw, (65, 7), "R", fill=(255, 255, 255), size=8, bold=True)
    line(draw, (20, 17, 72, 17), fill=grid, width=1)

    # Matrix brackets and cells, scaled for icon readability.
    line(draw, (28, 24, 28, 40), fill=ink, width=2)
    line(draw, (28, 24, 33, 24), fill=ink, width=2)
    line(draw, (28, 40, 33, 40), fill=ink, width=2)
    line(draw, (60, 24, 60, 40), fill=ink, width=2)
    line(draw, (55, 24, 60, 24), fill=ink, width=2)
    line(draw, (55, 40, 60, 40), fill=ink, width=2)

    for x in (36, 44, 52):
        for y in (26, 33):
            rect(draw, (x, y, x + 4, y + 4), fill=blue if (x + y) % 3 == 0 else grid)

    text(draw, (26, 43), "MATRIX", fill=ink, size=7, bold=True)

    return quantized(image)


def draw_app_icon(label: str, accent: tuple[int, int, int], selected: bool) -> Image.Image:
    bg1 = (246, 247, 250) if not selected else tuple(max(0, v - 34) for v in accent)
    bg2 = (218, 221, 228) if not selected else tuple(max(0, v - 82) for v in accent)
    card = (255, 255, 255) if not selected else (248, 250, 255)
    ink = (25, 28, 35)
    muted = (120, 126, 138)

    image = Image.new("RGB", (SIZE[0] * SCALE, SIZE[1] * SCALE), bg1)
    draw = ImageDraw.Draw(image)
    for y in range(SIZE[1] * SCALE):
        t = y / (SIZE[1] * SCALE - 1)
        color = tuple(round(bg1[i] * (1 - t) + bg2[i] * t) for i in range(3))
        draw.line((0, y, SIZE[0] * SCALE, y), fill=color)

    draw.rounded_rectangle((13 * SCALE, 6 * SCALE, 79 * SCALE, 58 * SCALE), radius=14 * SCALE,
                           fill=card, outline=(175, 180, 190), width=SCALE)
    draw.rounded_rectangle((18 * SCALE, 11 * SCALE, 74 * SCALE, 53 * SCALE), radius=9 * SCALE,
                           outline=accent, width=2 * SCALE)
    rect(draw, (18, 11, 74, 19), fill=accent)

    if label == "CAS":
        text(draw, (27, 23), "CAS", accent, 14, True)
        line(draw, (28, 42, 64, 42), muted, 1)
        text(draw, (29, 44), "pure", ink, 7, True)
    elif label == "P3":
        line(draw, (26, 43, 40, 34), accent, 2)
        line(draw, (40, 34, 52, 38), accent, 2)
        line(draw, (52, 38, 66, 25), accent, 2)
        text(draw, (29, 22), "P3", ink, 18, True)
    else:
        for yy in (25, 32, 39):
            line(draw, (28, yy, 64, yy), muted if yy != 25 else accent, 2)
        text(draw, (31, 43), "TXT", ink, 7, True)

    return quantized(image)


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    draw_icon(False).save(OUT_DIR / "runmat_icon.png", optimize=True, compress_level=9)
    draw_icon(True).save(OUT_DIR / "runmat_icon_selected.png", optimize=True, compress_level=9)
    specs = {
        "cas": ("CAS", (116, 72, 196)),
        "casp3": ("P3", (45, 113, 219)),
        "notes": ("NT", (228, 171, 48)),
    }
    for name, (label, accent) in specs.items():
        draw_app_icon(label, accent, False).save(OUT_DIR / f"{name}_icon.png", optimize=True, compress_level=9)
        draw_app_icon(label, accent, True).save(OUT_DIR / f"{name}_icon_selected.png", optimize=True, compress_level=9)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
