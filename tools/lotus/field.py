"""Drive the kohjoh prof session from the title into the field (issue #10/#13 checks).

Usage: python tools/lotus/field.py <session>
START at the title, A on Continue, then run into the field and print the
guest log's [gpu]/[probe] lines for the last 600-frame window.
"""
import json
import sys
import subprocess

CTL = "sinjoh/tools/lotus/ctl.py"
CWD = r"C:\Users\terezi\workdir"
LOG = (r"C:\Users\terezi\workdir\LOTUS-v0.0.3-windows-x64\data\citnlo\sessions"
       r"\cards\kohjoh-prof\sdmc\3ds\kohjoh\logs\heartgold.log")
A = 0x001
START = 0x400


def call(name, args, timeout=300):
    out = subprocess.run([sys.executable, CTL, "call", name, json.dumps(args)],
                         capture_output=True, text=True, cwd=CWD, timeout=timeout)
    t = out.stdout + out.stderr
    return json.loads(t[t.find("{"):])


import json  # noqa: E402


def run(sid, frames):
    left = frames
    while left > 0:
        n = min(600, left)
        r = call("lotus_run_until", {"session": sid, "frames": n})
        if r.get("last_error"):
            print("last_error:", r["last_error"])
        left -= n


def tap(sid, button, hold=20, after=60):
    call("lotus_input", {"session": sid, "owner": "live", "buttons": button,
                         "x": 0, "y": 0, "touch": False})
    run(sid, hold)
    call("lotus_input", {"session": sid, "owner": "live", "buttons": 0,
                         "x": 0, "y": 0, "touch": False})
    run(sid, after)


def main():
    sid = sys.argv[1]
    run(sid, 3000)        # title settles
    tap(sid, START, after=400)
    tap(sid, A, after=800)
    run(sid, 1200)        # into the field
    with open(LOG, encoding="utf-8", errors="replace") as f:
        lines = [l for l in f if "[gpu] 600" in l or "[probe]" in l]
    for l in lines[-6:]:
        print(l.rstrip())


if __name__ == "__main__":
    main()
