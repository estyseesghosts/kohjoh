import json, subprocess, sys, time
ctl = 'sinjoh/tools/lotus/ctl.py'
cwd = r'C:\Users\terezi\workdir'
sid = (json.loads(subprocess.run([sys.executable, ctl, 'status'], capture_output=True, text=True, cwd=cwd).stdout
       [subprocess.run([sys.executable, ctl, 'status'], capture_output=True, text=True, cwd=cwd).stdout.find('{'):])['sessions'][0]['id'])
print('session', sid)
for i in range(12):
    subprocess.run([sys.executable, ctl, 'run', sid, '300'], capture_output=True, cwd=cwd)
    cap = subprocess.run([sys.executable, ctl, 'capture', sid, '--out',
                          rf'C:\Users\terezi\workdir\kohjoh\build\lotus\issue1-caps\movie{i:02d}'],
                         capture_output=True, text=True, cwd=cwd)
    print(i, 'captured' if 'top.png' in cap.stdout else cap.stdout[-80:])
subprocess.run([sys.executable, ctl, 'stop', sid], capture_output=True, cwd=cwd)
print('done')
