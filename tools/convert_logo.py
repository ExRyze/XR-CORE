from pathlib import Path

import numpy as np
from PIL import Image

SRC = Path("data") / "logo.png"
OUT = Path("data") / "logo.raw"
PREVIEW = Path("tools") / "logo_preview.png"
W = 54
H = 54

ALPHA_MIN = 8        # alpha below this (0-255) counts as transparent -> OFF
LUMA_MIN = 40        # luma below this (0-255) counts as black -> OFF
FILL_THRESHOLD = 0.4 # coverage after downscaling needed for a pixel to be ON

# Work on the original-size RGBA so the alpha channel is not lost.
img = Image.open(SRC).convert("RGBA")
rgba = np.asarray(img)
alpha = rgba[..., 3]
luma = (0.299 * rgba[..., 0] + 0.587 * rgba[..., 1] + 0.114 * rgba[..., 2])

# ON = visible and not black. Transparent or black = OFF.
mask = (alpha >= ALPHA_MIN) & (luma >= LUMA_MIN)

# Keep the full canvas (transparent margins included) and scale it to W x H.
# Area averaging keeps thin gaps; the threshold is applied afterwards.
cov = Image.fromarray((mask * 255).astype(np.uint8)).resize((W, H), Image.Resampling.BOX)
bits = np.asarray(cov) >= int(FILL_THRESHOLD * 255)  # True = pixel ON

# Adafruit_GFX drawBitmap format: 1 bpp, MSB first (bit 7 = leftmost pixel),
# rows top to bottom, each row padded to ceil(W / 8) bytes.
row_bytes = (W + 7) // 8
data = bytearray()
for y in range(H):
    for bx in range(row_bytes):
        value = 0
        for bit in range(8):
            x = bx * 8 + bit
            if x < W and bits[y, x]:
                value |= 0x80 >> bit
        data.append(value)

OUT.write_bytes(data)

# Preview: what the OLED will show (white on black), upscaled.
Image.fromarray((bits * 255).astype(np.uint8)).resize(
    (W * 8, H * 8), Image.Resampling.NEAREST
).save(PREVIEW)

print(f"Generated {OUT} ({W}x{H}, {len(data)} bytes)")