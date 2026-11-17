"""Issue #1 reproduction: boot an in-game save, open the pokegear, capture.

Usage: python kohjoh/tools/lotus/pokegear.py <outdir>

Boots a fresh session on the kohjoh-prof card with a real-cart save staged,
drives title -> START -> A (Continue) -> field, taps SELECT to open the
pokegear, and captures both screens before and after.
"""
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

CTL = "sinjoh/tools/lotus/ctl.py"
CWD = Path(r"C:\Users\terezi\workdir")
CARD = CWD / (r"LOTUS-v0.0.3-windows-x64\data\citnlo\sessions\cards\kohjoh-prof"
              r"\sdmc\3ds\kohjoh")
SAVE_SRC = Path(r"C:\Users\terezi\Downloads\saves"
                r"\Gen 4 - Pokemon HeartGold (ENG).sav")
A, START, SELECT = 0x001, 0x400, 0x800


def call(name, args, timeout=600):
    out = subprocess.run([sys.executable, CTL, "call", name, json.dumps(args)],
                         capture_output=True, text=True, cwd=CWD, timeout=timeout)
    text = out.stdout + out.stderr
    try:
        return json.loads(text[text.find("{"):])
    except (ValueError, IndexError):
        raise SystemExit(f"ctl {name} failed: {text[:400]}")


def run(sid, frames):
    left = frames
    while left > 0:
        n = min(600, left)
        r = call("lotus_run_until", {"session": sid, "frames": n})
        if r.get("last_error"):
            print("last_error:", r["last_error"], flush=True)
        left -= n


def tap(sid, button, after=60):
    call("lotus_input", {"session": sid, "owner": "live", "buttons": button,
                         "x": 0, "y": 0, "touch": False})
    run(sid, 20)
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 0, "y": 0, "touch": False})
    run(sid, after)


def capture(sid, outdir, tag):
    call("lotus_focus_session", {"session": sid})
    out = subprocess.run([sys.executable, CTL, "capture", sid, "--out", outdir],
                         capture_output=True, text=True, cwd=CWD, timeout=120)
    if "saved:" not in out.stdout:
        raise SystemExit(f"capture failed: {out.stdout} {out.stderr}")
    top = out.stdout.split("saved:")[1].strip().split(", ")[0]
    base = top[: top.rfind("/")]
    shutil.move(top, f"{base}/{tag}-top.png")
    shutil.move(top[: -len("top.png")] + "bottom.png", f"{base}/{tag}-bottom.png")
    print(f"{tag} captured", flush=True)


def main():
    outdir = Path(sys.argv[1])
    outdir.mkdir(parents=True, exist_ok=True)
    CARD.joinpath("saves").mkdir(parents=True, exist_ok=True)
    shutil.copy2(SAVE_SRC, CARD / "saves" / "heartgold.sav")
    # fresh session
    out = subprocess.run([sys.executable, CTL, "status"], capture_output=True,
                         text=True, cwd=CWD, timeout=60)
    d = json.loads(out.stdout[out.stdout.find("{"):])
    for s in d["sessions"]:
        if s["card"] == "kohjoh-prof":
            subprocess.run([sys.executable, CTL, "stop", s["id"]],
                           capture_output=True, text=True, cwd=CWD)
            time.sleep(1)
    out = subprocess.run([sys.executable, CTL, "create", "--kind", "primary",
                          "--card", "kohjoh-prof",
                          "--asset", "kohjoh-kj/pokeheartgold.3dsx"],
                         capture_output=True, text=True, cwd=CWD, timeout=600)
    sid = json.loads(out.stdout[out.stdout.find("{"):])["session"]
    print("session", sid, flush=True)
    run(sid, 3000)          # title
    tap(sid, START, after=400)
    tap(sid, A, after=800)  # main menu appears
    run(sid, 600)
    tap(sid, A, after=800)  # CONTINUE
    run(sid, 1800)          # into the field
    call("lotus_pause", {"session": sid})
    capture(sid, outdir, "field")
    r = call("lotus_control", {"session": sid, "operation": "run"})
    print("run:", r, flush=True)
    tap(sid, SELECT, after=300)   # pause menu
    call("lotus_pause", {"session": sid})
    capture(sid, outdir, "menu")
    r = call("lotus_control", {"session": sid, "operation": "run"})
    # POKEGEAR icon, bottom-left of the lower panel (native 320x240)
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 45, "y": 208, "touch": True})
    run(sid, 20)
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 0, "y": 0, "touch": False})
    run(sid, 300)
    call("lotus_pause", {"session": sid})
    capture(sid, outdir, "gear")
    r = call("lotus_control", {"session": sid, "operation": "run"})
    # phone tab is open: call Prof. Elm (second contact) and capture the call
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 160, "y": 48, "touch": True})
    run(sid, 20)
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 0, "y": 0, "touch": False})
    run(sid, 120)
    tap(sid, A, after=600)     # confirm the call
    run(sid, 600)              # call in progress
    call("lotus_pause", {"session": sid})
    capture(sid, outdir, "call")
    r = call("lotus_control", {"session": sid, "operation": "run"})
    run(sid, 600)
    call("lotus_pause", {"session": sid})
    capture(sid, outdir, "call2")


if __name__ == "__main__":
    main()
