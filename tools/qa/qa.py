#!/usr/bin/env python3
"""qa.py: the widescreen QA corpus (issue #21).

A case (tools/qa/manifest.json) names an edition, a save fixture (by
SHA-256, never committed), an input script, the frames to capture and what
the log must say there: the active screen, its layout rows, wide-art keys.
The same case runs in any A/B mode, which turns one widescreen stage off
through the script's `debug:` flags (src/app/script.h), so a regression is
pinned to a stage on the same commit, save and trace.

    python tools/qa/qa.py list
    python tools/qa/qa.py stage  <case> [--mode M] --card <card dir>
    python tools/qa/qa.py check  <case> [--mode M] --card <card dir> --caps <dir>
    python tools/qa/qa.py record <case> [--mode M] --caps <dir>

`stage` writes the case's script.txt (and checks the save) into the card's
sdmc/3ds/kohjoh. Captures are then taken at the case's frames by whatever
drives LOTUS (the LOTUS MCP `lotus_capture`, or tools/lotus/shots.py over
the bridge); each frame's top screen goes in --caps as f<NNNNN>-top.bin
(raw 400x240 BGRA from LOTUS) or f<NNNNN>-top.png. `check` compares them
with the recorded baseline, the 256-column DS centre and the two 32-column
margins separately (a centre change is a correctness bug, a margin change a
widescreen one), and checks the log's expectations. `record` makes the
current captures the baseline.

Baselines hold game pixels: they live in build/qa/ (ignored), never in git.
Every result is emulator evidence only; LOTUS timing never certifies O3DS
pacing (kohjoh.md 1). Hardware runs report through the [pace]/[gputime]
log lines the same cases produce.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import sys
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MANIFEST = REPO / "tools" / "qa" / "manifest.json"
BASELINES = REPO / "build" / "qa" / "baseline"
W, H = 400, 240
# DS columns on the top screen: 256 DS px at 1.25x = 320 physical, centred.
CENTRE = (40, 360)
MARGINS = ((0, 40), (360, 400))

MODES = {
    "full": 0,
    "no-wideart": 0x100,
    "no-layout": 0x200,
    "no-sidefill": 0x400,
    "centred": 0x100 | 0x200 | 0x400,
}


def load_manifest() -> dict:
    m = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if m.get("version") != 1:
        raise SystemExit(f"{MANIFEST}: unsupported manifest version {m.get('version')}")
    return m


def find_case(m: dict, name: str) -> dict:
    for c in m["cases"]:
        if c["id"] == name:
            return c
    raise SystemExit(f"no case {name!r}; see `qa.py list`")


def sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def script_text(case: dict, mode: str) -> str:
    src = (REPO / case["script"]).read_text(encoding="utf-8")
    flags = MODES[mode] | int(case.get("debug", 0))
    head = f"# qa case {case['id']} mode {mode}\n"
    if flags:
        head += f"debug: {flags:#x}\n"
    return head + src


def cmd_list(m: dict, _a) -> int:
    for c in m["cases"]:
        frames = ",".join(str(f["frame"]) for f in c["frames"])
        print(f"{c['id']:24} {c['edition']:3} {c['class']:12} frames {frames}  {c['title']}")
    return 0


def cmd_stage(m: dict, a) -> int:
    case = find_case(m, a.case)
    kj = Path(a.card) / "sdmc" / "3ds" / "kohjoh"
    if not kj.is_dir():
        raise SystemExit(f"{kj}: not a kohjoh card")
    save = kj / "saves" / ("heartgold.sav" if case["edition"] == "HG" else "soulsilver.sav")
    want = case.get("save_sha256")
    if want and (not save.exists() or sha256(save) != want):
        raise SystemExit(f"{save}: not the case's save fixture (sha256 {want[:12]}...); "
                         "restore it before running")
    (kj / "script.txt").write_text(script_text(case, a.mode), encoding="utf-8")
    log = kj / "logs" / ("heartgold.log" if case["edition"] == "HG" else "soulsilver.log")
    print(f"staged {case['id']} ({a.mode}); capture top screens at frames "
          + ", ".join(str(f["frame"]) for f in case["frames"]) + f"; log {log}")
    return 0


def png_pixels(p: Path) -> bytes:
    """RGBA8 PNG (as tools/lotus/run.py writes) -> BGRA rows."""
    data = p.read_bytes()
    pos, idat, w, h = 8, b"", 0, 0
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h = struct.unpack(">II", body[:8])
            if body[8] != 8 or body[9] != 6:
                raise SystemExit(f"{p}: only 8-bit RGBA PNGs")
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    raw, out, prev = zlib.decompress(idat), bytearray(), bytearray(w * 4)
    stride = w * 4
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b = prev[i]
            c = prev[i - 4] if i >= 4 else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        prev = line
        for x in range(0, stride, 4):
            r, g, bl, al = line[x:x + 4]
            out += bytes((bl, g, r, al))
    return bytes(out)


def top_pixels(caps: Path, frame: int) -> bytes | None:
    b = caps / f"f{frame:05d}-top.bin"
    if b.exists():
        d = b.read_bytes()
        if len(d) != W * H * 4:
            raise SystemExit(f"{b}: expected {W}x{H} BGRA")
        return d
    p = caps / f"f{frame:05d}-top.png"
    return png_pixels(p) if p.exists() else None


def region_diff(a: bytes, b: bytes, x0: int, x1: int, tol: int) -> tuple[int, int]:
    """Pixels in columns [x0, x1) that differ by more than tol in any channel."""
    bad = total = 0
    for y in range(H):
        o = (y * W + x0) * 4
        for i in range(o, o + (x1 - x0) * 4, 4):
            total += 1
            if (abs(a[i] - b[i]) > tol or abs(a[i + 1] - b[i + 1]) > tol
                    or abs(a[i + 2] - b[i + 2]) > tol):
                bad += 1
    return bad, total


def log_lines(case: dict, card: Path) -> list[str]:
    kj = card / "sdmc" / "3ds" / "kohjoh" / "logs"
    log = kj / ("heartgold.log" if case["edition"] == "HG" else "soulsilver.log")
    if not log.exists():
        return []
    lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
    # This boot only: the last "[script]" header starts it.
    for i in range(len(lines) - 1, -1, -1):
        if "[script]" in lines[i] and "input events" in lines[i]:
            return lines[i:]
    return lines


def cmd_check(m: dict, a) -> int:
    case = find_case(m, a.case)
    caps, base = Path(a.caps), BASELINES / case["id"] / a.mode
    tol = int(m.get("tolerance", 8))
    failures = 0
    for f in case["frames"]:
        frame = f["frame"]
        got = top_pixels(caps, frame)
        if got is None:
            print(f"FAIL f{frame}: no capture in {caps}")
            failures += 1
            continue
        ref = top_pixels(base, frame)
        if ref is None:
            print(f"SKIP f{frame}: no baseline ({base}); `record` one first")
            continue
        c_bad, c_all = region_diff(got, ref, *CENTRE, tol)
        m_bad = sum(region_diff(got, ref, x0, x1, tol)[0] for x0, x1 in MARGINS)
        verdict = "ok" if c_bad == 0 and m_bad == 0 else "FAIL"
        failures += verdict != "ok"
        print(f"{verdict:4} f{frame}: centre {c_bad}/{c_all} px differ, margins {m_bad} px "
              f"({f.get('note', '')})")
    lines = log_lines(case, Path(a.card)) if a.card else []
    for exp in case.get("expect_log", []):
        hit = any(re.search(exp, ln) for ln in lines)
        failures += not hit
        print(f"{'ok' if hit else 'FAIL':4} log /{exp}/")
    print(f"{case['id']} ({a.mode}): {'PASS' if failures == 0 else f'{failures} failure(s)'}")
    return 0 if failures == 0 else 1


def cmd_record(m: dict, a) -> int:
    case = find_case(m, a.case)
    caps, base = Path(a.caps), BASELINES / case["id"] / a.mode
    base.mkdir(parents=True, exist_ok=True)
    for f in case["frames"]:
        for ext in ("bin", "png"):
            src = caps / f"f{f['frame']:05d}-top.{ext}"
            if src.exists():
                shutil.copy2(src, base / src.name)
                print(f"recorded {base / src.name}")
                break
        else:
            raise SystemExit(f"no capture for frame {f['frame']} in {caps}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    for name in ("stage", "check", "record"):
        p = sub.add_parser(name)
        p.add_argument("case")
        p.add_argument("--mode", choices=sorted(MODES), default="full")
        p.add_argument("--card", help="the LOTUS card directory (holds sdmc/)")
        if name != "stage":
            p.add_argument("--caps", required=True, help="directory of f<NNNNN>-top captures")
    a = ap.parse_args()
    if a.cmd == "stage" and not a.card:
        ap.error("stage needs --card")
    m = load_manifest()
    return {"list": cmd_list, "stage": cmd_stage, "check": cmd_check, "record": cmd_record}[a.cmd](m, a)


if __name__ == "__main__":
    sys.exit(main())
