"""Exercise production draw_layer's stencil ordering with a two-pixel GPU model.

Run from the repository root: python tests/native/test_blend_order.py [--cc gcc]
The model covers stencil region matching and bit-zero updates, blend selection,
and colour writes; it does not emulate PICA precision or texture sampling.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src/graphics/g2d/g2d.c"


def function(source, signature):
    start = source.index(signature)
    body = source.index("{", start)
    depth = 1
    end = body + 1
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    return source[start:end]


HARNESS = r"""
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define HGSS_PROFILE_OFF
#define GPU_EQUAL 1
#define GPU_ALWAYS 2
#define GPU_STENCIL_KEEP 0
#define GPU_STENCIL_REPLACE 1
#define GPU_STENCIL_INVERT 2
#define LAYER_OBJ 4

typedef struct {
    uint16_t bldcnt;
    uint8_t region_enable[4];
    bool windows, objwin;
    int mode;
    float eva, evb;
} fx;
typedef struct { unsigned count; } drawable;
typedef struct { unsigned stencil; float colour; unsigned writes; } pixel;

static pixel pixels[8];
static unsigned test_func, test_ref, test_mask, write_mask, pass_op;
static bool blending;
static float eva, evb;

static void C3D_StencilTest(bool enabled, unsigned func, unsigned ref,
                            unsigned mask, unsigned write)
{
    if (!enabled) abort();
    test_func = func; test_ref = ref; test_mask = mask; write_mask = write;
}
static void C3D_StencilOp(unsigned sfail, unsigned dfail, unsigned pass)
{
    if (sfail != GPU_STENCIL_KEEP || dfail != GPU_STENCIL_KEEP) abort();
    pass_op = pass;
}
static void blend_off(void) { blending = false; }
static void blend_alpha(float a, float b) { blending = true; eva = a; evb = b; }
static void stencil_write_all(uint8_t ref, uint8_t mask)
{
    C3D_StencilTest(true, mask ? GPU_EQUAL : GPU_ALWAYS, ref, mask, 3);
    C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
}
static void draw_quads(const drawable *d)
{
    if (!d->count) return;
    for (unsigned i = 0; i < 8; ++i) {
        pixel *p = &pixels[i];
        if (test_func == GPU_EQUAL &&
            (p->stencil & test_mask) != (test_ref & test_mask)) continue;
        p->colour = blending ? 100.0f * eva + p->colour * evb : 100.0f;
        ++p->writes;
        if (pass_op == GPU_STENCIL_INVERT) p->stencil ^= write_mask;
        if (pass_op == GPU_STENCIL_REPLACE)
            p->stencil = (p->stencil & ~write_mask) | (test_ref & write_mask);
    }
}

/* This is the exact function body extracted from the production source. */
%s

static void check(bool t1, bool t2, bool force, bool effects, bool windows)
{
    const int layer = force ? LAYER_OBJ : 1;
    const drawable d = { 1 };
    fx f = { 0 };
    f.mode = 1; f.eva = 0.25f; f.evb = 0.5f;
    f.bldcnt = (uint16_t)((t1 ? 1u << layer : 0) | (t2 ? 1u << (8 + layer) : 0));
    f.windows = windows; f.objwin = windows;
    for (int r = 0; r < 4; ++r) {
        f.region_enable[r] = (uint8_t)((1u << layer) | (effects ? 0x20u : 0));
        for (int under = 0; under < 2; ++under) {
            pixel *p = &pixels[2 * r + under];
            p->stencil = (unsigned)(windows ? r << 4 : 0) | (unsigned)under;
            p->colour = 40.0f; p->writes = 0;
        }
    }
    draw_layer(&f, layer, &d, force);
    for (int r = 0; r < 4; ++r) {
        for (int under = 0; under < 2; ++under) {
            const pixel *p = &pixels[2 * r + under];
            const bool alpha = (t1 || force) && (effects || force) && under;
            const float expected = alpha ? 45.0f : 100.0f;
            if (p->colour != expected || p->writes != 1 || (p->stencil & 1) != t2) {
                fprintf(stderr, "failed t1=%%d t2=%%d semi=%%d effects=%%d windows=%%d "
                        "region=%%d below=%%d colour=%%g writes=%%u stencil=%%u\n",
                        t1, t2, force, effects, windows, r, under,
                        (double)p->colour, p->writes, p->stencil);
                exit(1);
            }
        }
    }
}
int main(void)
{
    for (int t1 = 0; t1 < 2; ++t1)
        for (int t2 = 0; t2 < 2; ++t2)
            for (int force = 0; force < 2; ++force)
                for (int effects = 0; effects < 2; ++effects)
                    for (int windows = 0; windows < 2; ++windows)
                        check(t1, t2, force, effects, windows);
    puts("PASS draw_layer: BG/OBJ, target membership, four window regions");
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default=shutil.which("gcc") or "gcc")
    args = parser.parse_args()
    production = function(SOURCE.read_text(encoding="utf-8"), "static void draw_layer(")
    marker = "const bool blended = (pass == 0) == t2;"
    if marker not in production:
        raise SystemExit("production pass selector changed; update the mutation tests")

    # Both naive fixed orders are wrong in one of the two target configurations.
    variants = {
        "production": (production, True),
        "blended-first mutation": (production.replace(marker, "const bool blended = pass == 0;"), False),
        "opaque-first mutation": (production.replace(marker, "const bool blended = pass != 0;"), False),
    }
    with tempfile.TemporaryDirectory(prefix="kj-blend-") as tmp:
        tmp = Path(tmp)
        for name, (body, expect_success) in variants.items():
            source = tmp / "blend_test.c"
            exe = tmp / "blend_test.exe"
            source.write_text(HARNESS % body, encoding="utf-8")
            subprocess.run([args.cc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                            str(source), "-o", str(exe)], check=True)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            if (result.returncode == 0) != expect_success:
                raise SystemExit(f"{name}: unexpected result\n{result.stdout}{result.stderr}")
            print(f"{name}: {'passed' if expect_success else 'rejected'}")


if __name__ == "__main__":
    main()
