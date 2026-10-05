"""Palette-aware error diffusion; independent of Pillow and the ESP32 driver.

Input is row-major RGB bytes. Output is row-major native codes 0..3. Neutral
pixels can only become black/white; warm accent pixels retain their hue family.
This is spatial dithering, not a different panel waveform or native grayscale.
"""

import colorsys

PALETTE = ((0, 0, 0), (255, 255, 255), (242, 202, 24), (208, 32, 32))
ALGORITHMS = ("floyd-steinberg", "atkinson", "none")
KERNELS = {
    "floyd-steinberg": ((1, 0, 7 / 16), (-1, 1, 3 / 16),
                        (0, 1, 5 / 16), (1, 1, 1 / 16)),
    "atkinson": ((1, 0, 1 / 8), (2, 0, 1 / 8), (-1, 1, 1 / 8),
                 (0, 1, 1 / 8), (1, 1, 1 / 8), (0, 2, 1 / 8)),
    "none": (),
}


def classify_pixel(rgb):
    """Return a stable source hue family and a cleaned RGB target.

    Families: 0 = neutral, 2 = yellow, 3 = red. Classification never uses the
    diffused error, so a neutral face cannot acquire colored error speckles.
    Unsupported cool hues are rendered in grayscale rather than false colors.
    """
    r, g, b = rgb
    hue, saturation, _ = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    if saturation < 0.25 or not (hue <= 85 / 360 or hue >= 330 / 360):
        level = 0.299 * r + 0.587 * g + 0.114 * b
        # Suppress incidental near-white paper grain and nearly black noise.
        if level >= 248:
            level = 255.0
        elif level <= 9:
            level = 0.0
        return 0, (level, level, level)
    family = 3 if hue <= 25 / 360 or hue >= 330 / 360 else 2
    accent = PALETTE[family]
    # Generated nominal red/yellow varies slightly. Keep these intended solid
    # inks solid; only meaningful shadows/highlights should become dots.
    if sum((rgb[c] - accent[c]) ** 2 for c in range(3)) <= 35 ** 2:
        return family, accent
    return family, tuple(float(c) for c in rgb)


def dither_rgb(rgb, width, height, algorithm="floyd-steinberg"):
    """Convert RGB bytes to native 2bpp indices using serpentine diffusion.

    Diffusion is limited to the same source hue family, protecting boundaries.
    Error storage is only three scanlines; there is no full float framebuffer.
    Pure native ink pixels are locked so adjacent shading cannot dirty them.
    """
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError("RGB byte count must match positive width and height")
    if algorithm not in ALGORITHMS:
        raise ValueError(f"Unknown dithering algorithm: {algorithm}")
    pixel_count = width * height
    families = bytearray(pixel_count)
    targets = []
    locks = bytearray([255]) * pixel_count
    for i in range(pixel_count):
        family, target = classify_pixel(rgb[i * 3:i * 3 + 3])
        families[i] = family
        targets.append(target)
        for code in (0, 1) if family == 0 else (0, 1, family):
            if target == PALETTE[code]:
                locks[i] = code
                break

    errors = [[0.0] * (width * 3) for _ in range(3)]
    codes = bytearray(pixel_count)
    kernel = KERNELS[algorithm]
    for y in range(height):
        direction = 1 if y % 2 == 0 else -1
        columns = range(width) if direction == 1 else range(width - 1, -1, -1)
        current = errors[y % 3]
        for x in columns:
            i, offset = y * width + x, x * 3
            if locks[i] != 255:
                codes[i] = locks[i]
                continue
            family, target = families[i], targets[i]
            adjusted = tuple(target[c] + current[offset + c] for c in range(3))
            allowed = (0, 1) if family == 0 else (0, 1, family)
            code = min(allowed, key=lambda p: sum(
                weight * (adjusted[c] - PALETTE[p][c]) ** 2
                for c, weight in enumerate((0.299, 0.587, 0.114))
            ))
            codes[i] = code
            residual = tuple(adjusted[c] - PALETTE[code][c] for c in range(3))
            for dx, dy, weight in kernel:
                nx, ny = x + dx * direction, y + dy
                if not (0 <= nx < width and ny < height):
                    continue
                ni = ny * width + nx
                if families[ni] != family or locks[ni] != 255:
                    continue
                next_row, next_offset = errors[ny % 3], nx * 3
                for c in range(3):
                    next_row[next_offset + c] += residual[c] * weight
        errors[y % 3] = [0.0] * (width * 3)
    return bytes(codes)
