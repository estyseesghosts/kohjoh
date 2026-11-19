#!/usr/bin/env python3
"""Check run.py's lotus_input masks against the LOTUS NativeButton protocol."""

from pathlib import Path
import runpy
import subprocess
import sys
import unittest


RUNNER = Path(__file__).resolve().parents[2] / "tools/lotus/run.py"


class LotusButtonTests(unittest.TestCase):
    def test_masks(self):
        buttons = runpy.run_path(str(RUNNER), run_name="lotus_runner_test")["BUTTONS"]
        self.assertEqual(buttons, {
            "A": 0x001, "B": 0x002, "X": 0x004, "Y": 0x008,
            "UP": 0x010, "DOWN": 0x020, "LEFT": 0x040, "RIGHT": 0x080,
            "L": 0x100, "R": 0x200, "START": 0x400, "SELECT": 0x800,
        })
        self.assertEqual(buttons["DOWN"] | buttons["A"], 0x021)

    def test_help_lists_keys_and_combination(self):
        result = subprocess.run([sys.executable, str(RUNNER), "--help"],
                                check=True, capture_output=True, text=True)
        self.assertIn("900:DOWN+A", result.stdout)
        self.assertIn("START/SELECT", result.stdout)


if __name__ == "__main__":
    unittest.main()
