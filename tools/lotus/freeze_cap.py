import json, subprocess, sys, time
ctl = 'sinjoh/tools/lotus/ctl.py'
cwd = r'C:\Users\terezi\workdir'
def call(op, args):
    return subprocess.run([sys.executable, ctl, 'call', op, json.dumps(args)], capture_output=True, text=True, cwd=cwd).stdout
sid = (json.loads(subprocess.run([sys.executable, ctl, 'status'], capture_output=True, text=True, cwd=cwd).stdout
       [subprocess.run([sys.executable, ctl, 'status'], capture_output=True, text=True, cwd=cwd).stdout.find('{'):])['sessions'][0]['id'])
print('session', sid)
# drive until publish 9400 (about 18800 host frames), then into the 45 s freeze
left = 19000
while left > 0:
    n = min(600, left)
    subprocess.run([sys.executable, ctl, 'run', sid, str(n)], capture_output=True, cwd=cwd)
    left -= n
print('in freeze window, capturing')
for i in range(4):
    out = subprocess.run([sys.executable, ctl, 'capture', sid, '--out',
                          rf'C:\Users\terezi\workdir\kohjoh\build\lotus\issue1-caps\frozen{i}'],
                         capture_output=True, text=True, cwd=cwd)
    print('cap', out.stdout[-120:], out.stderr[-120:])
    time.sleep(4)
subprocess.run([sys.executable, ctl, 'stop', sid], capture_output=True, cwd=cwd)
print('stopped')
