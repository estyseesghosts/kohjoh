"""Issue #2: boot kohjoh with each real-cart HGSS save and look for a crash.

Usage: python kohjoh/tools/lotus/saves_test.py

For every full-size (524288-byte) HGSS save in C:/Users/terezi/Downloads/saves:
stages it as the kohjoh card's saves/heartgold.sav, stops and recreates the
card's primary session (fresh boot with the staged save), runs the title
(3000 frames), taps START, runs into the menu, taps A (Continue), runs on,
and reports the guest log tail plus any FATAL/assert.
The original card save is restored afterwards.
"""
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

CTL = "sinjoh/tools/lotus/ctl.py"
CWD = Path(r"C:\Users\terezi\workdir")
SAVES = Path(r"C:\Users\terezi\Downloads\saves")
CARD = CWD / (r"LOTUS-v0.0.3-windows-x64\data\citnlo\sessions\cards\kohjoh"
              r"\sdmc\3ds\kohjoh")
CARD_SAVE = CARD / "saves" / "heartgold.sav"
LOG = CARD / "logs" / "heartgold.log"
ASSET = "kohjoh-kj/pokeheartgold.3dsx"


def ctl(*args, timeout=900):
    out = subprocess.run([sys.executable, CTL, *args],
                         capture_output=True, text=True, cwd=CWD, timeout=timeout)
    return out.stdout + out.stderr


def call(name, args, timeout=900):
    out = subprocess.run([sys.executable, CTL, "call", name, json.dumps(args)],
                         capture_output=True, text=True, cwd=CWD, timeout=timeout)
    text = out.stdout + out.stderr
    try:
        return json.loads(text[text.find("{"):])
    except (ValueError, IndexError):
        raise SystemExit(f"ctl {name} failed: {text[:400]}")


def fresh_session():
    """Stop the kohjoh card's primary session and create a fresh one."""
    out = ctl("status")
    d = json.loads(out[out.find("{"):])
    for s in d["sessions"]:
        if s["card"] == "kohjoh":
            ctl("stop", s["id"])
            time.sleep(1)
    out = ctl("create", "--kind", "primary", "--card", "kohjoh",
              "--asset", ASSET, timeout=600)
    try:
        return json.loads(out[out.find("{"):])["session"]
    except (ValueError, KeyError):
        raise SystemExit(f"create failed: {out[:400]}")


def run(session, frames):
    left = frames
    while left > 0:
        n = min(600, left)
        r = call("lotus_run_until", {"session": session, "frames": n})
        if r.get("last_error"):
            print("last_error:", r["last_error"], flush=True)
        left -= n


def tap(session, button, frames_after):
    call("lotus_input", {"session": session, "owner": "live", "buttons": button,
                         "x": 0, "y": 0, "touch": False})
    run(session, 20)
    call("lotus_input", {"session": session, "owner": "live", "buttons": 0,
                         "x": 0, "y": 0, "touch": False})
    run(session, frames_after)


def log_tail(marker_lines=15):
    lines = LOG.read_text(errors="replace").splitlines()
    return lines[-marker_lines:]


def main():
    candidates = sorted(p for p in SAVES.iterdir()
                        if p.stat().st_size == 524288 and p.is_file()
                        and ("HeartGold" in p.name or "SoulSilver" in p.name
                             or "HGSS" in p.name))
    if not candidates:
        raise SystemExit("no full-size HGSS saves found")
    backup = CARD_SAVE.with_suffix(".sav.bak")
    if not backup.exists():
        shutil.copy2(CARD_SAVE, backup)
    results = []
    try:
        for p in candidates:
            shutil.copy2(p, CARD_SAVE)
            session = fresh_session()
            run(session, 3000)           # title boot
            tap(session, 0x400, 3400)    # START -> title menu
            tap(session, 0x001, 4200)    # A -> Continue/load save
            run(session, 4800)
            tail = log_tail()
            bad = [l for l in tail if "FATAL" in l or "assert" in l.lower()
                   or "abort" in l.lower()]
            results.append((p.name, "CRASH?" if bad else "ok", tail[-3:]))
            print(f"== {p.name}: {'CRASH' if bad else 'ok'}", flush=True)
            for l in tail:
                print("   ", l)
    finally:
        shutil.copy2(backup, CARD_SAVE)
    print("\nsummary:")
    for name, verdict, _ in results:
        print(f"  {verdict:6} {name}")


if __name__ == "__main__":
    main()
