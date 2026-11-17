"""A/B bisect driver for the #13 join bug.

Stages a script.txt with a given debug mask on the kohjoh card, boots a fresh
LOTUS session on the current build/kj 3dsx, captures the title frames, and
compares against the baseline. Removes the script afterwards.

Usage: python kohjoh/tools/lotus/join_bisect.py <mask-hex> <outdir>
"""
import json
import shutil
import subprocess
import sys
from pathlib import Path

CTL = "sinjoh/tools/lotus/ctl.py"
CWD = Path(r"C:\Users\terezi\workdir")
CARD_ROOT = CWD / (r"LOTUS-v0.0.3-windows-x64\data\citnlo\sessions\cards\kohjoh"
                   r"\sdmc\3ds\kohjoh")
SCRIPT = CARD_ROOT / "script.txt"
BASE = "kohjoh/build/lotus/issue10-baseline"
FRAMES = [300, 600, 1200, 1800, 2400, 2700, 3000]


def main():
    mask = sys.argv[1]
    outdir = Path(sys.argv[2])
    SCRIPT.write_text(f"debug: {mask}\n")
    try:
        out = subprocess.run([sys.executable, CTL, "create", "--kind", "primary",
                              "--card", "kohjoh", "--asset",
                              "kohjoh-kj/pokeheartgold.3dsx"],
                             capture_output=True, text=True, cwd=CWD, timeout=600)
        session = json.loads(out.stdout[out.stdout.find("{"):])["session"]
        subprocess.run([sys.executable, "kohjoh/tools/lotus/shots.py", session,
                        str(outdir)] + [str(f) for f in FRAMES],
                       cwd=CWD, timeout=3600, check=True)
        subprocess.run([sys.executable, "kohjoh/tools/lotus/compare.py",
                        BASE, str(outdir)], cwd=CWD)
    finally:
        SCRIPT.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
