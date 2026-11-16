#!/usr/bin/env python3
"""Build the four shipping variants, one at a time, and print their .3dsx paths.

Usage (inside the devkitPro MSYS2 login shell, after tools/bootstrap.py):

    python tools/game/build_variants.py                # gold/silver x normal/prof
    python tools/game/build_variants.py --only gold-prof
    python tools/game/build_variants.py --list
    python tools/game/build_variants.py --jobs 16

Each variant gets its own --out (build/kj-<variant>): the game objects differ
between editions and between HGSS_PROFILE=on (prof) and unset (normal). The
variants run strictly sequentially. They share the SDK/NitroSystem shim
(build/hgss-sdk, build/hgss-nns) and the headers generated under
build/upstream, so parallel variants race on both; the lost race is
`mklink /J ... Cannot create a file when that file already exists`.

A variant counts as built only when hgss_game.py exits 0, its .3dsx exists, and
its log has no `error:` or `vendored library build failed` line (see
docs/building-windows.md). The first failure stops the run and prints the log
path; a full run ends with the four paths.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
TOOLS = REPO / "tools" / "game"
DEFAULT_ROOT = REPO / "build" / "upstream"

sys.path.insert(0, str(TOOLS))
import hgss_nitro as nitro  # noqa: E402 (msys(): the /c/... spelling make needs)

# (variant, --game, HGSS_PROFILE). "normal" is the uninstrumented build.
VARIANTS = (
    ("gold-normal", "heartgold", False),
    ("gold-prof", "heartgold", True),
    ("silver-normal", "soulsilver", False),
    ("silver-prof", "soulsilver", True),
)

# A build that compiled nothing new but linked, or fell back to stale
# libraries, still exits 0 and still packages a .3dsx. docs/building-windows.md
# says to grep for these two markers.
BAD_MARKERS = ("error:", "vendored library build failed")


def output_dir(variant: str) -> Path:
    return REPO / "build" / f"kj-{variant}"


def log_path(variant: str) -> Path:
    return REPO / "build" / f"kj-{variant}-build.log"


def binary_name(variant: str) -> str:
    edition = variant.split("-")[0]
    return ("pokeheartgold" if edition == "gold" else "pokesoulsilver") + ".3dsx"


def run_variant(variant: str, game: str, profile: bool, root: Path,
                jobs: int) -> bool:
    """Build one variant; return True only if its .3dsx is really there."""
    out = output_dir(variant)
    log = log_path(variant)
    out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env.pop("HGSS_PROFILE", None)
    if profile:
        env["HGSS_PROFILE"] = "on"
    cmd = [sys.executable, str(TOOLS / "hgss_game.py"),
           "--root", nitro.msys(root), "--target", "3ds",
           "--out", nitro.msys(out), "-j", str(jobs)]
    if game != "heartgold":
        cmd += ["--game", game]

    print(f"\n=== {variant} ({game}, HGSS_PROFILE={'on' if profile else 'unset'})",
          flush=True)
    with log.open("w", encoding="utf-8", errors="replace") as fh:
        proc = subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True,
                                encoding="utf-8", errors="replace", bufsize=1)
        for line in proc.stdout:
            sys.stdout.write(line)
            fh.write(line)
        proc.wait()

    text = log.read_text(encoding="utf-8", errors="replace").lower()
    dsx = out / binary_name(variant)
    if proc.returncode != 0:
        print(f"{variant}: hgss_game.py exited {proc.returncode}; log: {log}")
        return False
    markers = [m for m in BAD_MARKERS if m in text]
    if markers:
        print(f"{variant}: log contains {markers!r}; log: {log}")
        return False
    if not dsx.is_file():
        print(f"{variant}: no {dsx}; log: {log}")
        return False
    print(f"{variant}: {dsx} ({dsx.stat().st_size} bytes)")
    return True


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", type=Path, default=DEFAULT_ROOT,
                    help="patched decomp checkout (default: build/upstream)")
    ap.add_argument("--jobs", "-j", type=int, default=os.cpu_count() or 1,
                    help="parallel compile jobs per variant (default: CPU count)")
    ap.add_argument("--only", action="append", default=None,
                    help="build just this variant (repeatable)")
    ap.add_argument("--list", action="store_true",
                    help="list the variants and exit")
    args = ap.parse_args(argv)

    if args.list:
        for name, game, profile in VARIANTS:
            print(f"{name:14} --game {game:10} "
                  f"HGSS_PROFILE={'on' if profile else 'unset'}")
        return 0

    wanted = VARIANTS
    if args.only:
        known = {name for name, _, _ in VARIANTS}
        unknown = [name for name in args.only if name not in known]
        if unknown:
            ap.error(f"unknown variant(s) {unknown}; choose from {sorted(known)}")
        wanted = [v for v in VARIANTS if v[0] in args.only]

    root = args.root.resolve()
    if not (root / "main.lsf").is_file():
        print(f"build_variants: {root} is not a bootstrapped checkout "
              f"(run python tools/game/bootstrap.py)", file=sys.stderr)
        return 2

    built = []
    for name, game, profile in wanted:
        if not run_variant(name, game, profile, root, args.jobs):
            return 1
        built.append(output_dir(name) / binary_name(name))

    print("\n== all variants built")
    for path in built:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
