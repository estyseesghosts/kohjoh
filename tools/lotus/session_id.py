"""Print the session id for a given LOTUS card (default kohjoh)."""
import json
import subprocess
import sys

card = sys.argv[1] if len(sys.argv) > 1 else "kohjoh"
ctl = r"C:\Users\terezi\workdir\sinjoh\tools\lotus\ctl.py"
out = subprocess.run([sys.executable, "sinjoh/tools/lotus/ctl.py", "status"], capture_output=True, text=True, cwd=r"C:\Users\terezi\workdir")
text = out.stdout
start = text.find("{")
d = json.loads(text[start:])
for s in d["sessions"]:
    if s["card"] == card:
        print(s["id"], s["telemetry"]["state"], s["telemetry"]["frames"], flush=True)
        break
