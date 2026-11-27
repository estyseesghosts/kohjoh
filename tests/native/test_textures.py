#!/usr/bin/env python3
"""Compile and run the texture-cache regression test (cleanup task 2).

Run from the repository root: python tests/native/test_textures.py
Set CC to a host C compiler if `gcc` is not on PATH.
"""

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]

THREE_DS_H = """
#ifndef STUB_3DS_H
#define STUB_3DS_H
#include <stdint.h>
#endif
"""

CITRO3D_H = """
#ifndef STUB_CITRO3D_H
#define STUB_CITRO3D_H
#include <stdbool.h>
#include <stdint.h>
typedef uint16_t u16;
typedef enum { GPU_RGBA8 = 0, GPU_RGBA5551 = 1 } GPU_TEXCOLOR;
typedef enum { GPU_NEAREST = 0, GPU_LINEAR = 1 } GPU_TEXTURE_FILTER;
typedef struct C3D_Tex {
    void *data;
    u16 width, height;
    GPU_TEXCOLOR fmt;
} C3D_Tex;
bool C3D_TexInit(C3D_Tex *tex, u16 width, u16 height, GPU_TEXCOLOR fmt);
void C3D_TexDelete(C3D_Tex *tex);
void C3D_TexSetFilter(C3D_Tex *tex, GPU_TEXTURE_FILTER mag, GPU_TEXTURE_FILTER min);
#endif
"""


def main():
    cc = os.environ.get("CC", "gcc")
    source = (ROOT / "src/graphics/g3d/textures.c").as_posix()
    with tempfile.TemporaryDirectory(prefix="kohjoh-tex-") as tmp:
        tmp = Path(tmp)
        (tmp / "3ds.h").write_text(THREE_DS_H)
        (tmp / "citro3d.h").write_text(CITRO3D_H)
        exe = tmp / "texture_cache_test.exe"
        args = [
            cc,
            "-std=gnu11",
            "-O2",
            "-Wall",
            "-Wextra",
            "-DHGSS_PROFILE_OFF",
            f'-DSOURCE="{source}"',
            "-I", tmp.as_posix(),
            "-I", (ROOT / "src").as_posix(),
            (ROOT / "tests/native/texture_cache_test.c").as_posix(),
            "-o", exe.as_posix(),
        ]
        subprocess.run(args, cwd=ROOT, check=True)
        subprocess.run([exe.as_posix()], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
