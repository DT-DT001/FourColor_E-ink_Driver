"""Standard-library-only tests; run: python -m unittest discover -s tests -v."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from convert_image import FRAME_BYTES, c_array_source, pack_codes, unpack_frame
from epd_dither import ALGORITHMS, PALETTE, classify_pixel, dither_rgb


class ConversionTests(unittest.TestCase):
    def test_four_native_colours_pack_to_1b(self):
        self.assertEqual(pack_codes(bytes([0, 1, 2, 3]), 4, 1), b"\x1b")
        self.assertEqual(unpack_frame(b"\x1b", 4, 1), bytes([0, 1, 2, 3]))

    def test_full_frame_round_trip(self):
        codes = bytes([0, 1, 2, 3]) * FRAME_BYTES
        packed = pack_codes(codes)
        self.assertEqual(len(packed), 105984)
        self.assertEqual(unpack_frame(packed), codes)

    def test_invalid_codes_and_dimensions(self):
        for codes, width, height in ((b"", 0, 1), (b"", 4, 0), (b"\0" * 3, 3, 1),
                                     (b"\0\0\0\x04", 4, 1), (b"\0", 4, 1)):
            with self.assertRaises(ValueError):
                pack_codes(codes, width, height)
        with self.assertRaises(ValueError):
            unpack_frame(b"", 4, 1)

    def test_native_inks_stay_solid(self):
        for algorithm in ALGORITHMS:
            for code, colour in enumerate(PALETTE):
                self.assertEqual(dither_rgb(bytes(colour) * 256, 16, 16, algorithm),
                                 bytes([code]) * 256)

    def test_gray_is_only_black_white(self):
        rgb = b"".join(bytes([x, x, x]) for _ in range(8) for x in range(256))
        self.assertEqual(set(dither_rgb(rgb, 256, 8)), {0, 1})

    def test_floyd_preserves_gray_coverage(self):
        for level in (32, 64, 128, 192, 224, 240):
            codes = dither_rgb(bytes([level]) * (64 * 64 * 3), 64, 64)
            self.assertLess(abs(sum(codes) * 255 / len(codes) - level), 3.0)

    def test_accent_families_do_not_cross(self):
        self.assertTrue(set(dither_rgb(bytes([150, 20, 20]) * 256, 16, 16)) <= {0, 1, 3})
        self.assertTrue(set(dither_rgb(bytes([180, 140, 20]) * 256, 16, 16)) <= {0, 1, 2})
        self.assertEqual(classify_pixel((248, 231, 226))[0], 0)

    def test_white_next_to_shading_stays_white(self):
        rgb = b"".join(bytes((128, 128, 128) if x < 8 else PALETTE[1])
                       for _ in range(16) for x in range(16))
        codes = dither_rgb(rgb, 16, 16)
        for y in range(16):
            self.assertEqual(codes[y * 16 + 8:(y + 1) * 16], bytes([1]) * 8)

    def test_dither_validation_and_repeatability(self):
        with self.assertRaises(ValueError):
            dither_rgb(b"", 1, 1)
        with self.assertRaises(ValueError):
            dither_rgb(b"\0\0\0", 1, 1, "unsupported")
        rgb = bytes([110, 110, 110]) * 17
        self.assertEqual(dither_rgb(rgb, 1, 17), dither_rgb(rgb, 1, 17))

    def test_c_export_validates_symbol_and_size(self):
        with self.assertRaises(ValueError):
            c_array_source(bytes(FRAME_BYTES), "bad-name")
        with self.assertRaises(ValueError):
            c_array_source(b"\0", "image")


if __name__ == "__main__":
    unittest.main()
