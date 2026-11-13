"""Host differential checks against a git revision; no game assets required.
Run: python tests/native/check_hotpaths.py [--reference HEAD] [--cc gcc]
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
a = argparse.ArgumentParser()
a.add_argument('--reference', default='HEAD')
a.add_argument('--cc', default=shutil.which('gcc') or 'C:/devkitPro/msys2/usr/bin/gcc.exe')
a = a.parse_args()

def run(args):
    return subprocess.check_output([str(x) for x in args], cwd=root, text=True).strip()

with tempfile.TemporaryDirectory(prefix='kohjoh-test-') as tmp:
    tmp = Path(tmp)
    (tmp/'3ds.h').write_text('#include <stdint.h>\n#define SYSCLOCK_ARM11 268111856u\nuint64_t svcGetSystemTick(void);\n')
    (tmp/'citro3d.h').write_text('#include <stdint.h>\nuint32_t C3D_FrameCounter(int);\n')
    (tmp/'nitro.h').write_text('#include <stdint.h>\ntypedef uint64_t u64; typedef uint32_t u32;\n')
    (tmp/'nitro_port.h').write_text('#include <stddef.h>\n#include <stdint.h>\nvoid NitroPort_Log(const char *, ...);\n')
    shutil.copy(root/'src/compat/nitro/include/nitro_port_gx3d.h',tmp)
    common=[a.cc,'-O2','-fwrapv','-fno-strict-aliasing','-I',tmp,'-I',root/'src/runtime','-I',root/'src/compat/nitro/include']
    for name,path in [('sound','src/compat/nitro/src/port_sound_chip.c'),('g3','src/runtime/kj_g3.c')]:
        before=tmp/(name+'_before.c')
        before.write_text(run(['git','show',f'{a.reference}:{path}'])+'\n')
        results=[]
        for variant,source in [('before',before),('after',root/path)]:
            exe=tmp/(name+'_'+variant+'.exe')
            run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{name}_diff.c','-o',exe])
            results.append(run([exe]))
        assert results[0]==results[1], (name,results)
        print(f'PASS {name}: {results[0]}',flush=True)
    exe=tmp/'g3_verify.exe'
    source=root/'src/runtime/kj_g3.c'
    run([*common,'-DHGSS_PROFILE_OFF','-DKJ_DL_VERIFY',f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_diff.c','-o',exe])
    assert run([exe])==results[1], 'cached/live geometry mismatch'
    print('PASS g3: live decode verification',flush=True)
    exe=tmp/'perf_test.exe'
    run([*common,'-I',root/'src',root/'tests/native/perf_test.c','-o',exe])
    print(run([exe]))
