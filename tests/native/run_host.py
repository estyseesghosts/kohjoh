#!/usr/bin/env python3
"""Run the host-side native regression suite (cleanup task 11).

No game assets or proprietary material are needed: the font atlas is a
committed generated asset and every other test compiles a first-party module
against small stubs. Run from the repository root:

    python tests/native/run_host.py [--cc gcc]

The geometry/sound differential harness (check_hotpaths.py) is included.
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
CC = os.environ.get("CC", "gcc")


def run(args, **kwargs):
    print("+", " ".join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True, **kwargs)


def build_and_run(name, sources, includes, extra=(), link_extra=()):
    with tempfile.TemporaryDirectory(prefix="kohjoh-native-") as tmp:
        exe = Path(tmp) / (name + ".exe")
        args = [CC, "-std=gnu11", "-O2", "-Wall", "-Wextra", "-o", exe]
        for include in includes:
            include = Path(include)
            args += ["-I", include if include.is_absolute() else ROOT / include]
        args += [ROOT / source for source in sources]
        args += list(extra)
        args += list(link_extra)
        run(args)
        run([exe])


def build_and_run_perf():
    with tempfile.TemporaryDirectory(prefix="kohjoh-perf-") as tmp:
        tmp = Path(tmp)
        (tmp / "3ds.h").write_text(
            "#include <stdint.h>\n"
            "#define SYSCLOCK_ARM11 268111856u\n"
            "uint64_t svcGetSystemTick(void);\n")
        (tmp / "citro3d.h").write_text(
            "#include <stdint.h>\n"
            "uint32_t C3D_FrameCounter(int);\n")
        build_and_run("perf_test", ["tests/native/perf_test.c"],
                      ["src", tmp])


def main():
    global CC
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default=CC)
    args = parser.parse_args()
    CC = args.cc

    build_and_run("config_test", ["tests/native/config_test.c", "src/app/config.c"],
                  ["src"])
    build_and_run("field_view_test",
                  ["tests/native/field_view_test.c", "src/runtime/kj_field_view.c"],
                  ["src", "src/runtime"])
    build_and_run("layout_test",
                  ["tests/native/layout_test.c", "src/graphics/layout/layout.c",
                   "src/runtime/kj_layouts.c"], ["src", "src/runtime"])
    build_and_run("world_view_test",
                  ["tests/native/world_view_test.c", "src/graphics/world_view.c"],
                  ["src"], link_extra=["-lm"])
    build_and_run("text_test",
                  ["tests/native/text_test.c", "src/graphics/font/glyph_atlas.c",
                   "src/graphics/font/glyph_batch.c",
                   "src/graphics/font/text_window.c"], ["src"])
    build_and_run_perf()

    for script in ("test_textures.py", "test_save_backend.py", "test_save_import.py",
                   "test_touch_tp.py", "visibility_test.py", "pica_lighting_test.py",
                   "test_script_input.py"):
        run([sys.executable, ROOT / "tests/native" / script])

    run([sys.executable, ROOT / "tests/native/check_hotpaths.py"])


if __name__ == "__main__":
    main()
