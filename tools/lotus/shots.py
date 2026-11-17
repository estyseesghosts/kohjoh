"""Run the kohjoh LOTUS session to chosen frames, capturing PNGs at each.

Usage: python tools/lotus/shots.py <session> <outdir> <frame> [<frame>...]

Frames must be increasing. Each target is reached by 600-frame chunks whose
progress is re-read from the session status after every chunk (a chunk may
stop early at the host deadline). Captures are written as
<outdir>/f<NNNNN>-top.png and f<NNNNN>-bottom.png.
"""
import json
import shutil
import subprocess
import sys

CTL = "sinjoh/tools/lotus/ctl.py"
CWD = r"C:\Users\terezi\workdir"


def call(name, args, timeout=900):
    out = subprocess.run([sys.executable, CTL, "call", name, json.dumps(args)],
                         capture_output=True, text=True, cwd=CWD, timeout=timeout)
    text = out.stdout + out.stderr
    try:
        return json.loads(text[text.find("{"):])
    except (ValueError, IndexError):
        raise SystemExit(f"ctl {name} failed: {text[:400]}")


def session_frames(session):
    out = subprocess.run([sys.executable, CTL, "status"],
                         capture_output=True, text=True, cwd=CWD)
    d = json.loads(out.stdout[out.stdout.find("{"):])
    for s in d["sessions"]:
        if s["id"].startswith(session[:8]):
            return s["telemetry"]["frames"]
    raise SystemExit(f"session {session} not found")


def main():
    session, outdir = sys.argv[1], sys.argv[2]
    targets = [int(x) for x in sys.argv[3:]]
    if targets != sorted(set(targets)):
        raise SystemExit("targets must be increasing and unique")
    for target in targets:
        done = session_frames(session)
        while done < target:
            r = call("lotus_run_until", {"session": session, "frames": min(600, target - done)})
            if r.get("last_error"):
                print("last_error:", r["last_error"], flush=True)
            done = session_frames(session)
        # Freeze at the target frame BEFORE capturing: an extra vblank between
        # run_until returning and the capture reading the framebuffer made the
        # shot nondeterministically frame or frame+1.
        if done < target:
            r = call("lotus_run_until", {"session": session, "frames": target - done})
            done = session_frames(session)
        call("lotus_pause", {"session": session})
        done = session_frames(session)
        out = subprocess.run([sys.executable, CTL, "capture", session, "--out", outdir],
                             capture_output=True, text=True, cwd=CWD)
        if "saved:" not in out.stdout:
            raise SystemExit(out.stdout + out.stderr)
        top = out.stdout.split("saved:")[1].strip().split(", ")[0]
        base = top[: top.rfind("/")]
        shutil.move(top, f"{base}/f{done:05d}-top.png")
        shutil.move(top[: -len("top.png")] + "bottom.png", f"{base}/f{done:05d}-bottom.png")
        print(f"f{done:05d} captured", flush=True)


if __name__ == "__main__":
    main()
