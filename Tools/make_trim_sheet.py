"""Paints the kit's one trim sheet: Content/Kit/T_Kit_Trim.png (1024 x 1024).

Eight horizontal bands of 128 px, each a surface the kit is made of. Faces map their long axis
along U (tiling; one U repeat is TILE_CM of surface) and their short axis across the band's V
range. Colours are the palette in CLAUDE.md; amber is for power and error states only, so it
appears just in the hazard band the generator and door sills wear.

Run with the system Python (needs Pillow): python Tools/make_trim_sheet.py
"""
import os
import random
from PIL import Image, ImageDraw

W = H = 1024
BAND = 128
# name -> (band index from the top, base colour)
BANDS = {
    "bone": (0, (0xE8, 0xE4, 0xDA)),
    "slate": (1, (0x3A, 0x47, 0x50)),
    "copper": (2, (0x5F, 0x8A, 0x7D)),
    "ink": (3, (0x1C, 0x21, 0x26)),
    "hazard": (4, (0xD9, 0x9A, 0x2B)),
    "grating": (5, (0x2E, 0x38, 0x40)),
    "ceiling": (6, (0x33, 0x3F, 0x47)),
    "signage": (7, (0xE8, 0xE4, 0xDA)),
}
TILE_CM = 100.0  # one U repeat is a metre of surface


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


def band_box(name):
    i = BANDS[name][0]
    return (0, i * BAND, W, (i + 1) * BAND)


def main():
    random.seed(7)
    img = Image.new("RGB", (W, H))
    d = ImageDraw.Draw(img)
    for name, (i, col) in BANDS.items():
        d.rectangle(band_box(name), fill=col)

    # grain: a little per-pixel variation so flat colour reads as enamel and paint, not plastic
    px = img.load()
    for y in range(H):
        for x in range(W):
            n = random.randint(-4, 4)
            r, g, b = px[x, y]
            px[x, y] = (max(0, min(255, r + n)), max(0, min(255, g + n)), max(0, min(255, b + n)))

    # bone: enamel panels a metre wide, a dark seam at each edge, a faint lower shadow line
    x0, y0, x1, y1 = band_box("bone")
    for sx in range(0, W, W // 2):
        d.line([(sx, y0), (sx, y1)], fill=shade(BANDS["bone"][1], 0.55), width=3)
        d.line([(sx + 3, y0), (sx + 3, y1)], fill=shade(BANDS["bone"][1], 0.92), width=2)
    d.line([(x0, y1 - 2), (x1, y1 - 2)], fill=shade(BANDS["bone"][1], 0.7), width=2)

    # slate: a dado with a rivet row top and bottom, a seam per half metre
    x0, y0, x1, y1 = band_box("slate")
    base = BANDS["slate"][1]
    for sx in range(0, W, W // 4):
        d.line([(sx, y0), (sx, y1)], fill=shade(base, 0.6), width=2)
    for rx in range(16, W, 64):
        for ry in (y0 + 14, y1 - 14):
            d.ellipse([rx - 4, ry - 4, rx + 4, ry + 4], fill=shade(base, 1.35))
            d.ellipse([rx - 2, ry - 3, rx + 1, ry], fill=shade(base, 0.75))

    # copper: brushed, oxidised: long faint streaks and a darker edge top and bottom
    x0, y0, x1, y1 = band_box("copper")
    base = BANDS["copper"][1]
    for _ in range(400):
        y = random.randint(y0 + 4, y1 - 4)
        x = random.randint(0, W)
        length = random.randint(20, 160)
        d.line([(x, y), (x + length, y)], fill=shade(base, random.choice((0.9, 1.08, 1.15))), width=1)
    d.line([(x0, y0 + 1), (x1, y0 + 1)], fill=shade(base, 0.6), width=3)
    d.line([(x0, y1 - 2), (x1, y1 - 2)], fill=shade(base, 0.6), width=3)

    # ink: matte, a faint grille every 8 px in the lower half for vents
    x0, y0, x1, y1 = band_box("ink")
    base = BANDS["ink"][1]
    for gy in range(y0 + BAND // 2, y1, 8):
        d.line([(x0, gy), (x1, gy)], fill=shade(base, 0.6), width=2)

    # hazard: amber and ink diagonals, the only amber on the sheet
    x0, y0, x1, y1 = band_box("hazard")
    ink = BANDS["ink"][1]
    for sx in range(-BAND, W + BAND, 64):
        d.polygon([(sx, y1), (sx + 32, y1), (sx + 32 + BAND, y0), (sx + BAND, y0)], fill=ink)

    # grating: floor plate with a dark grid every 32 px and a lighter lip
    x0, y0, x1, y1 = band_box("grating")
    base = BANDS["grating"][1]
    for gx in range(0, W, 32):
        d.line([(gx, y0), (gx, y1)], fill=shade(base, 0.45), width=3)
    for gy in range(y0, y1, 32):
        d.line([(x0, gy), (x1, gy)], fill=shade(base, 0.45), width=3)
    for gx in range(0, W, 32):
        d.line([(gx + 3, y0), (gx + 3, y1)], fill=shade(base, 1.25), width=1)

    # ceiling: panels with a recessed seam and a bone-coloured service strip along the middle
    x0, y0, x1, y1 = band_box("ceiling")
    base = BANDS["ceiling"][1]
    for sx in range(0, W, W // 4):
        d.line([(sx, y0), (sx, y1)], fill=shade(base, 0.5), width=3)
    d.rectangle([x0, y0 + BAND // 2 - 6, x1, y0 + BAND // 2 + 6], fill=shade(base, 0.7))

    # signage: bone with an ink stripe and sector marks; the enamel sign look, no words yet
    x0, y0, x1, y1 = band_box("signage")
    d.rectangle([x0, y0 + 40, x1, y0 + 88], fill=BANDS["ink"][1])
    for sx in range(32, W, 128):
        d.rectangle([sx, y0 + 50, sx + 12, y0 + 78], fill=BANDS["bone"][1])
        d.rectangle([sx + 40, y0 + 50, sx + 64, y0 + 78], fill=BANDS["bone"][1])

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Content", "Kit", "T_Kit_Trim.png")
    img.save(os.path.normpath(out))
    print("wrote", os.path.normpath(out))


if __name__ == "__main__":
    main()
