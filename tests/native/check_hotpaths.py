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
    common=[a.cc,'-O2','-fwrapv','-fno-strict-aliasing','-DKJ_G3_CPU_LIGHTING','-DKJ_G3_NO_CULL','-I',tmp,'-I',root/'src/runtime','-I',root/'src/compat/nitro/include']
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
    exe=tmp/'g3_profile.exe'
    run([*common,f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_diff.c','-o',exe])
    assert run([exe])==results[1], 'profiled geometry changed output'
    print('PASS g3: sampled attribution, nested totals, table overflow/reset, unchanged output',flush=True)
    for fixture in ('matrix_diff', 'dl_dependencies_test'):
        hashes=[]
        for variant,source in [('before',tmp/'g3_before.c'),('after',root/'src/runtime/kj_g3.c')]:
            exe=tmp/(fixture+'_'+variant+'.exe')
            run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{fixture}.c','-o',exe])
            hashes.append(run([exe]))
        if fixture == 'matrix_diff':
            assert hashes[0]==hashes[1], (fixture,hashes)
            print(f'PASS {fixture}: {hashes[1]}',flush=True)
        if fixture == 'dl_dependencies_test':
            for variant,source in [('before',tmp/'g3_before.c'),('after',root/'src/runtime/kj_g3.c')]:
                exe=tmp/(fixture+'_uncached_'+variant+'.exe')
                run([*common,'-DHGSS_PROFILE_OFF','-DKJ_DL_NO_CACHE',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{fixture}.c','-o',exe])
                oracle=run([exe])
                print(f'{fixture} {variant} uncached {oracle}, cached {hashes[0 if variant == "before" else 1]}',flush=True)
                assert oracle==hashes[1], (fixture,variant,'uncached oracle differs',oracle,hashes[1])
            print(f'PASS {fixture}: cached output matches both uncached decoders',flush=True)
            exe=tmp/'dependencies_verify.exe'
            source=root/'src/runtime/kj_g3.c'
            run([*common,'-DHGSS_PROFILE_OFF','-DKJ_DL_VERIFY',f'-DSOURCE="{source.as_posix()}"',root/f'tests/native/{fixture}.c','-o',exe])
            assert run([exe])==hashes[1], 'lit-normal cache replay changed geometry'
            print('PASS lit-normal dependencies: replay/live verification',flush=True)
    exe=tmp/'dl_key_test.exe'
    run([*common,'-DHGSS_PROFILE_OFF',f'-DSOURCE="{source.as_posix()}"',root/'tests/native/dl_key_test.c','-o',exe])
    print(run([exe]), flush=True)
    capture_hash = None
    for mode in ([], ['-DKJ_DL_NO_CACHE'], ['-DKJ_DL_VERIFY']):
        exe=tmp/'g3_capture_test.exe'
        run([*common,'-DHGSS_PROFILE_OFF',*mode,f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_capture_test.c','-o',exe])
        full, dry = run([exe]), run([exe, 'discard'])
        assert full == dry, ('discard changed DS state/next picture', mode, full, dry)
        if capture_hash is not None:
            assert capture_hash == dry, ('cache mode changed capture', mode, capture_hash, dry)
        capture_hash = dry
    print(f'PASS capture: {capture_hash}, unchanged DS tests/counts and following pictures; no discarded vertex writes', flush=True)
    plan_hash = None
    for variant, source, mode in [
        ('before', tmp/'g3_before.c', []),
        ('cached', root/'src/runtime/kj_g3.c', ['-DTEST_COMMAND_PLAN']),
        ('uncached', root/'src/runtime/kj_g3.c', ['-DKJ_DL_NO_CACHE']),
        ('verify', root/'src/runtime/kj_g3.c', ['-DKJ_DL_VERIFY', '-DTEST_COMMAND_PLAN']),
    ]:
        exe=tmp/'dl_plan_test.exe'
        run([*common,'-DHGSS_PROFILE_OFF',*mode,f'-DSOURCE="{source.as_posix()}"',root/'tests/native/dl_plan_test.c','-o',exe])
        value = run([exe])
        if plan_hash is not None:
            assert plan_hash == value, ('lit plan/shading changed output', variant, plan_hash, value)
        plan_hash = value
    print(f'PASS lit plans: {plan_hash}, changing matrices/materials/shininess/content/layout against original and uncached', flush=True)
    lighting_hash = None
    for variant, source, mode in [
        ('original', tmp/'g3_before.c', ['-DKJ_G3_CPU_LIGHTING']),
        ('CPU', root/'src/runtime/kj_g3.c', ['-DKJ_G3_CPU_LIGHTING']),
        ('GPU', root/'src/runtime/kj_g3.c', []),
        ('GPU uncached', root/'src/runtime/kj_g3.c', ['-DKJ_DL_NO_CACHE']),
        ('GPU verify', root/'src/runtime/kj_g3.c', ['-DKJ_DL_VERIFY']),
    ]:
        exe=tmp/'g3_lighting_test.exe'
        flags = [x for x in common if x != '-DKJ_G3_CPU_LIGHTING']
        run([*flags,'-DHGSS_PROFILE_OFF',*mode,f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_lighting_test.c','-o',exe])
        value = run([exe])
        if lighting_hash is not None:
            assert lighting_hash == value, ('NORMAL-time lighting context changed output', variant, lighting_hash, value)
        lighting_hash = value
    print(f'PASS GPU lighting inputs: {lighting_hash}, canonical triangles/colours/texgen match original CPU oracle', flush=True)
    cull = {}
    for variant, mode in [('cull', []), ('cull verify', ['-DKJ_DL_VERIFY']),
                          ('uncached', ['-DKJ_DL_NO_CACHE']), ('no cull', ['-DKJ_G3_NO_CULL'])]:
        exe=tmp/'g3_cull_test.exe'
        flags = [x for x in common if x not in ('-DKJ_G3_NO_CULL', '-DKJ_G3_CPU_LIGHTING')]
        run([*flags,'-DHGSS_PROFILE_OFF',*mode,f'-DSOURCE="{source.as_posix()}"',root/'tests/native/g3_cull_test.c','-o',exe])
        cull[variant] = subprocess.run([str(exe)], cwd=root, text=True, capture_output=True, check=True)
    assert len({r.stdout for r in cull.values()}) == 1, ('culling changed visible triangles', cull)
    assert cull['cull'].stderr != cull['uncached'].stderr, 'no retained block was culled'
    print(f"PASS cull: {cull['cull'].stdout.strip()}; {cull['cull'].stderr.strip()} against uncached {cull['uncached'].stderr.strip()}", flush=True)
    exe=tmp/'perf_test.exe'
    run([*common,'-I',root/'src',root/'tests/native/perf_test.c','-o',exe])
    print(run([exe]))

print(run(['python', root/'tests/native/pica_lighting_test.py']), flush=True)
