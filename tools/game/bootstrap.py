#!/usr/bin/env python3
"""Fetch the pinned pokeheartgold decompilation and apply our patches.

Adapted from pokeplatinum-3ds/tools/bootstrap.py (GPL-3.0). Platinum fetches
NitroSDK and NitroSystem as separate meson subprojects; HGSS has them in-tree
under lib/, so everything comes from one checkout.

Usage:
    python tools/bootstrap.py           # fetch (if needed) + apply patches
    python tools/bootstrap.py --clean   # delete build/upstream and start over

Result:
    build/upstream/   ctrl-cheeb-del/pokeheartgold at the commit in upstream.lock, then:
                        patches/NitroSDK/*.patch      -> applied under lib/NitroSDK
                        patches/NitroSystem/*.patch   -> applied under lib/NitroSystem
                        patches/pokeheartgold/*.patch -> applied at the repository root
    build/NitroSDK, build/NitroSystem
                      symlinks to build/upstream/lib/..., because the vendored
                      3ds_port/ Makefiles look for the SDKs there.

Patches are applied with `git am` (so each keeps its message) as commits on
top of the pinned commit. SDK patches are stored relative to their library
(`git format-patch --relative=lib/NitroSDK`) and re-rooted with
`git am --directory`. Any patch that does not apply cleanly stops the run.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
TOOLS = ROOT / "tools" / "game"
sys.path.insert(0, str(TOOLS))
import hgss_nitro  # noqa: E402,F401  (sets TMP for child processes)
from hgss_nitro import create_link, link_points_to  # noqa: E402 (shared link fallback, native Windows has no symlink privilege)

# (patch directory under patches/, directory inside the checkout it applies to)
PATCH_STACKS = (
    ("NitroSDK", "lib/NitroSDK"),
    ("NitroSystem", "lib/NitroSystem"),
    ("pokeheartgold", ""),
)
SDK_LINKS = ("NitroSDK", "NitroSystem")

# Committer identity for `git am`, so bootstrap works in a fresh container.
GIT_IDENT = {
    "GIT_COMMITTER_NAME": "kohjoh bootstrap",
    "GIT_COMMITTER_EMAIL": "bootstrap@invalid",
}


def read_lock(path: Path) -> dict[str, str]:
    """Return the decomp pin from kohjoh's upstream.lock (`decomp <url> <commit>`)."""
    for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = raw.split("#", 1)[0].split()
        if fields and fields[0] == "decomp":
            if len(fields) != 3:
                sys.exit(f"{path.name}:{line_no}: expected 'decomp <url> <commit>'")
            return {"url": fields[1], "commit": fields[2]}
    sys.exit(f"{path.name}: no 'decomp' pin")


def git(*args: str, cwd: Path | None = None, capture: bool = False) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=False,
        text=True,
        capture_output=capture,
        env={**os.environ, **GIT_IDENT},
    )
    if result.returncode != 0:
        detail = (result.stderr or "").strip() if capture else ""
        sys.exit(f"git {' '.join(args)} failed{': ' + detail if detail else ''}")
    return (result.stdout or "").strip() if capture else ""


def current_commit(repo: Path) -> str | None:
    if not (repo / ".git").exists():
        return None
    return git("rev-parse", "HEAD", cwd=repo, capture=True)


def fetch(dest: Path, url: str, commit: str) -> None:
    """Shallow-fetch exactly one commit (no full history download)."""
    print(f"Fetching {url} @ {commit[:12]} ...")
    dest.mkdir(parents=True, exist_ok=True)
    git("init", "--quiet", cwd=dest)
    git("remote", "add", "origin", url, cwd=dest)
    git("fetch", "--quiet", "--depth", "1", "origin", commit, cwd=dest)
    git("checkout", "--quiet", "--detach", "FETCH_HEAD", cwd=dest)


def apply_patches(dest: Path, patch_dir: Path, subdir: str) -> int:
    patches = sorted(patch_dir.glob("*.patch"))
    for patch in patches:
        print(f"  applying {patch_dir.name}/{patch.name}")
        args = ["am", "--quiet", "--keep-non-patch"]
        if subdir:
            args.append(f"--directory={subdir}")
        result = subprocess.run(
            ["git", *args, str(patch)], cwd=dest, text=True, capture_output=True,
            env={**os.environ, **GIT_IDENT},
        )
        if result.returncode != 0:
            subprocess.run(["git", "am", "--abort"], cwd=dest, capture_output=True)
            sys.exit(f"{patch.name} does not apply cleanly:\n{result.stdout}{result.stderr}")
    return len(patches)


def generate_headers(upstream: Path) -> None:
    """Generate the decomp's build-time headers in Python (no host compiler).

    tools/game/prepare_game_headers.py ports gen_fx_consts, msgenc, nitroarc,
    o2narc and csv2bin byte-exactly, so a native-Windows checkout needs no
    MSYS2 libc headers or Wine.
    """
    r = subprocess.run([sys.executable, str(TOOLS / "prepare_game_headers.py"),
                        "--root", str(upstream)], text=True, capture_output=True)
    if r.returncode != 0:
        sys.exit("prepare_game_headers.py failed:\n" + r.stdout + r.stderr)


def link_sdks(root: Path) -> None:
    """Make build/NitroSDK and build/NitroSystem reach upstream/lib/<name>.

    MSYS2 Python silently *copies* a directory when asked for a symlink, so a
    plain directory here may be a stale copy. It is refreshed, but only after
    proving it is not a link or junction to the real tree (same file on disk):
    deleting through a junction would delete the checkout itself.
    """
    build = root / "build"
    for name in SDK_LINKS:
        link = build / name
        target = Path("upstream") / "lib" / name
        real = build / target
        if link_points_to(link, target):
            continue
        if link.exists():
            probe = Path("include") / "nitro.h" if name == "NitroSDK" else Path("include")
            if (link / probe).exists() and os.path.samefile(link / probe, real / probe):
                continue
            if link.is_symlink():
                link.unlink()
            else:
                shutil.rmtree(link)
        create_link(link, target)


def bootstrap(root: Path, clean: bool = False) -> None:
    if shutil.which("git") is None:
        sys.exit("Git is not installed or not on PATH.")
    build = root / "build"
    upstream = build / "upstream"

    if clean:
        for d in (upstream, *(build / n for n in SDK_LINKS)):
            if d.is_symlink():
                d.unlink()
            elif d.exists():
                try:
                    # A Windows junction (is_symlink() is False for junctions):
                    # remove the link itself, never the target it points at.
                    os.readlink(d)
                    d.unlink()
                    continue
                except OSError:
                    pass
                print(f"Removing {d.relative_to(root)}")
                shutil.rmtree(d)

    lock = read_lock(root / "upstream.lock")
    have = current_commit(upstream)
    if have is None:
        fetch(upstream, lock["url"], lock["commit"])
    else:
        if subprocess.run(["git", "merge-base", "--is-ancestor", lock["commit"], "HEAD"],
                          cwd=upstream, capture_output=True).returncode != 0:
            sys.exit(f"build/upstream does not descend from {lock['commit'][:12]}.\nRun with --clean to re-fetch.")
        if git("status", "--porcelain", cwd=upstream, capture=True):
            sys.exit("build/upstream has local changes.\nRun with --clean to start fresh.")
        applied = int(git("rev-list", "--count", f"{lock['commit']}..HEAD", cwd=upstream, capture=True))
        wanted = sum(len(list((root / "patches" / n).glob("*.patch"))) for n, _ in PATCH_STACKS)
        if applied != wanted:
            sys.exit(f"build/upstream has {applied} patches applied but patches/ has {wanted}.\n"
                     "Run with --clean to re-apply from scratch.")
        link_sdks(root)
        generate_headers(upstream)
        print(f"upstream: already at {lock['commit'][:12]} with {applied} patches")
        return

    total = 0
    for name, subdir in PATCH_STACKS:
        total += apply_patches(upstream, root / "patches" / name, subdir)
    link_sdks(root)
    generate_headers(upstream)
    print(f"Done: build/upstream ({total} patches applied), build/NitroSDK, build/NitroSystem")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--clean", action="store_true", help="delete fetched sources first")
    parser.add_argument("--root", type=Path, default=ROOT, help=argparse.SUPPRESS)
    args = parser.parse_args()
    bootstrap(args.root.resolve(), args.clean)


if __name__ == "__main__":
    main()
