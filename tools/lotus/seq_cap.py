import json, subprocess, sys, time, re
ctl = 'sinjoh/tools/lotus/ctl.py'
cwd = r'C:\Users\terezi\workdir'
LOG = r'C:\Users\terezi\workdir\LOTUS-v0.0.3-windows-x64\data\citnlo\sessions\cards\kohjoh-prof\sdmc\3ds\kohjoh\logs\heartgold.log'
CAPD = r'C:\Users\terezi\workdir\kohjoh\build\lotus\issue1-caps'
FREEZES = (1200, 1800, 2100, 2400, 2700)

def sh(*a):
    return subprocess.run([sys.executable, *a], capture_output=True, text=True, cwd=cwd)

open(r'C:\Users\terezi\workdir\LOTUS-v0.0.3-windows-x64\data\citnlo\sessions\cards\kohjoh-prof\sdmc\3ds\kohjoh\script.txt', 'w').write(
    'every: 10\n' + '\n'.join(f'freeze {f} 20' for f in FREEZES) + '\n')
out = sh(ctl, 'create', '--kind', 'primary', '--card', 'kohjoh-prof', '--asset', 'kohjoh-prof/pokeheartgold.3dsx').stdout
sid = json.loads(out[out.find('{'):])['session']
print('session', sid)
seen = set()
left = 7800
while left > 0:
    n = min(600, left)
    sh(ctl, 'run', sid, str(n))
    left -= n
    time.sleep(1)
    txt = open(LOG, errors='replace').read()
    for f in FREEZES:
        key = f'holding game frame {f}'
        if key in txt and f not in seen:
            seen.add(f)
            for attempt in range(4):
                cap = sh(ctl, 'capture', sid, '--out', rf'{CAPD}\seqf{f}')
                if 'top.png' in cap.stdout:
                    print('captured freeze', f)
                    break
                time.sleep(3)
print('done, seen:', seen)
sh(ctl, 'stop', sid)
