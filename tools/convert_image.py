#!/usr/bin/env python3
"""Convert a local picture to native MSPQ1TMN1001QNX 2bpp data offline."""

import argparse
from pathlib import Path
import re

from epd_dither import ALGORITHMS, PALETTE, dither_rgb

WIDTH, HEIGHT = 768, 552
FRAME_BYTES = WIDTH * HEIGHT // 4


def pack_codes(codes, width=WIDTH, height=HEIGHT):
    if width <= 0 or height <= 0 or width % 4 or len(codes) != width * height:
        raise ValueError("Codes must match a positive image with width divisible by four")
    if any(code < 0 or code > 3 for code in codes):
        raise ValueError("Native colour codes must be 0..3")
    return bytes((codes[i] << 6) | (codes[i + 1] << 4) |
                 (codes[i + 2] << 2) | codes[i + 3]
                 for i in range(0, len(codes), 4))


def unpack_frame(frame, width=WIDTH, height=HEIGHT):
    if width <= 0 or height <= 0 or width % 4 or len(frame) != width * height // 4:
        raise ValueError("Frame length does not match dimensions")
    return bytes((byte >> shift) & 3 for byte in frame for shift in (6, 4, 2, 0))


def c_array_source(frame, symbol):
    if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", symbol):
        raise ValueError("Use a C identifier starting with an ASCII letter")
    if len(frame) != FRAME_BYTES:
        raise ValueError("C export requires a complete 768x552 frame")
    lines = ['#include <stdint.h>', '', f'const uint8_t {symbol}[{len(frame)}] = {{']
    for offset in range(0, len(frame), 16):
        lines.append('    ' + ', '.join(f'0x{byte:02x}' for byte in frame[offset:offset + 16]) + ',')
    return '\n'.join(lines + ['};', ''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path, help="Output raw .bin frame")
    parser.add_argument("--dither", choices=ALGORITHMS, default="floyd-steinberg")
    parser.add_argument("--preview", type=Path, help="Optional decoded four-colour PNG")
    parser.add_argument("--c-array", type=Path, help="Optional C array source")
    parser.add_argument("--symbol", default="epd_image", help="C array identifier")
    args = parser.parse_args()

    # Pillow is only needed by the CLI, not by packing/dithering unit tests.
    from PIL import Image, ImageOps

    with Image.open(args.input) as source:
        source = ImageOps.exif_transpose(source)
        # Flatten transparency onto white before fitting; never transmit alpha.
        rgba = source.convert("RGBA")
        canvas = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
        canvas.alpha_composite(rgba)
        fitted = ImageOps.fit(canvas.convert("RGB"), (WIDTH, HEIGHT),
                              method=Image.Resampling.LANCZOS)
    codes = dither_rgb(fitted.tobytes(), WIDTH, HEIGHT, args.dither)
    frame = pack_codes(codes)
    if unpack_frame(frame) != codes:
        raise RuntimeError("Native frame packing failed round-trip verification")

    c_source = c_array_source(frame, args.symbol) if args.c_array else None
    outputs = [args.output] + [path for path in (args.preview, args.c_array) if path]
    if len({path.resolve() for path in outputs}) != len(outputs):
        raise ValueError("Output paths must be distinct")
    if args.input.resolve() in {path.resolve() for path in outputs}:
        raise ValueError("Refusing to overwrite the input image")
    for output in outputs:
        output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(frame)
    if args.c_array:
        args.c_array.write_text(c_source, encoding="utf-8", newline="\n")
    if args.preview:
        preview = Image.new("P", (WIDTH, HEIGHT))
        preview.putpalette([component for colour in PALETTE for component in colour] + [0] * (256 * 3 - 12))
        preview.frombytes(unpack_frame(frame))
        preview.convert("RGB").save(args.preview)
    print(f"Written {len(frame)} bytes (768x552, 2bpp, {args.dither}): {args.output}")


if __name__ == "__main__":
    main()
