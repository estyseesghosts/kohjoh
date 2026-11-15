"""Synthetic clip-plane checks without game assets or a renderer."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/capture'))
from g3_visibility import census, outcode

IDENTITY = [[float(i == j) for j in range(4)] for i in range(4)]
VIEWPORT = 255 << 16 | 191 << 24


class VisibilityTest(unittest.TestCase):
    def test_widened_sides(self):
        self.assertEqual(outcode((5000, 0, 0), IDENTITY, VIEWPORT, True, False), 0)
        self.assertNotEqual(outcode((5000, 0, 0), IDENTITY, VIEWPORT, False, False), 0)
        self.assertNotEqual(outcode((6000, 0, 0), IDENTITY, VIEWPORT, True, False), 0)

    def test_wbuffer_has_no_z_clip(self):
        self.assertEqual(outcode((0, 0, 8192), IDENTITY, VIEWPORT, True, True), 0)
        self.assertNotEqual(outcode((0, 0, 8192), IDENTITY, VIEWPORT, True, False), 0)

    def test_palette_and_common_plane(self):
        translated = [row[:] for row in IDENTITY]
        translated[3][0] = 4
        verts = [(0, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0, 1)]
        batch = (0, 3, 0, 2, 0, 0, 0, VIEWPORT, 0)
        summary, rows = census((0, 0, 0, verts, [0, 1, 2],
                                [translated, IDENTITY], [batch]))
        self.assertEqual(summary['potentially_visible_triangles'], 1)
        self.assertFalse(rows[0]['whole_batch_outside'])
        verts[2] = (0, 0, 0, 0)
        summary, _ = census((0, 0, 0, verts, [0, 1, 2],
                             [translated, IDENTITY], [batch]))
        self.assertEqual(summary['whole_batches_outside'], 1)
        self.assertEqual(summary['trivially_outside_triangles'], 1)

    def test_viewport_and_unknown(self):
        # A viewport on the far right can be invisible even at clip x=0.
        vp = 240 | 255 << 16 | 191 << 24
        self.assertNotEqual(outcode((8192, 0, 0), IDENTITY, vp, False, False), 0)
        bad = [row[:] for row in IDENTITY]
        bad[0][0] = float('nan')
        self.assertEqual(outcode((8192, 0, 0), bad, VIEWPORT, True, False), 0)


if __name__ == '__main__':
    unittest.main()
