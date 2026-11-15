"""Full-chip sound checks on an emulated ARMv6K core; no game assets required.

Compiles tests/native/sound_chip_prog.c with devkitARM against a reference
revision of port_sound_chip.c and against the working tree, runs both under
Unicorn and compares:

  coverage  random register programs (every format, routing, capture, loops,
            data rewritten under playing channels), both interpolation modes:
            the digests must match.
  bench     an HGSS-like workload (about 11 looping ADPCM voices): digests
            must match; prints mixer instructions per channel-frame (Unicorn
            counts instructions only, as Azahar does: no cycles, no caches).

Run with a Python that has unicorn and pyelftools, for example
build/diagnosis/venv/Scripts/python.exe tests/native/sound_arm.py
  [--reference HEAD] [--programs 300] [--ticks 1200] [--define NAME ...]
  [--interpolate 0|1] [--expect-differ]
"""
import argparse
import bisect
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_BLOCK
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC

ROOT = Path(__file__).resolve().parents[2]
CHIP = 'src/compat/nitro/src/port_sound_chip.c'
# MSYS2 and Git Bash export /opt/devkitpro/... (or a rewrite of it) that a
# Windows Python cannot run; fall back to the standard install.
DEVKITARM = Path(os.environ.get('DEVKITARM', ''))
if not (DEVKITARM / 'bin').is_dir():
    DEVKITARM = Path('C:/devkitPro/devkitARM')
GCC = DEVKITARM / 'bin' / 'arm-none-eabi-gcc'
NM = DEVKITARM / 'bin' / 'arm-none-eabi-nm'
# The port's flags for the 3DS (src/compat/common.mk), plus a bare link.
CFLAGS = ['-std=gnu99', '-fno-strict-aliasing', '-fno-short-enums', '-fsigned-char', '-fwrapv',
          '-fno-aggressive-loop-optimizations', '-fno-delete-null-pointer-checks', '-O2',
          '-march=armv6k', '-mtune=mpcore', '-mfpu=vfp', '-mfloat-abi=hard', '-mtp=soft',
          '-mword-relocations', '-ffunction-sections', '-D__3DS__', '-DHGSS_PROFILE_OFF',
          '-nostdlib', '-nostartfiles', '-Wl,-e,main', '-Wl,--no-enum-size-warning', '-Wall', '-Wno-unused-function']
# The harness's own code; 32-bit division is the harness's (% in its RNG):
# the mixer divides only by constants, and its 64-bit step division stays.
HARNESS = ('main', 'memcpy', '__udivsi3', '__aeabi_uidiv', '__aeabi_uidivmod')


def build(source, exe, mode, defines):
    args = [str(GCC), *CFLAGS, '-I', str(ROOT / 'src/compat/nitro/include'),
            f'-DSOURCE="{Path(source).as_posix()}"', *mode, *[f'-D{d}' for d in defines],
            str(ROOT / 'tests/native/sound_chip_prog.c'), '-o', str(exe), '-lgcc']
    subprocess.run(args, check=True)


def chip_ranges(elf_path):
    """Address ranges of the mixer's own functions (not the harness's)."""
    ranges = []
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        for sym in elf.get_section_by_name('.symtab').iter_symbols():
            if sym['st_info']['type'] != 'STT_FUNC' or not sym['st_size']:
                continue
            if sym.name.startswith('harness_') or sym.name in HARNESS:
                continue
            start = sym['st_value'] & ~1
            ranges.append((start, start + sym['st_size'], sym.name))
    ranges.sort()
    return ranges


def run(elf_path, count):
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        u = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        segs = [(s['p_vaddr'], s['p_memsz'], s.data()) for s in elf.iter_segments()
                if s['p_type'] == 'PT_LOAD']
        lo = min(a for a, n, d in segs) & ~4095
        hi = max(a + n for a, n, d in segs)
        u.mem_map(lo, (hi - lo + 4095) & ~4095)
        for a, n, d in segs:
            u.mem_write(a, d)
        syms = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    u.mem_map(0x8000000, 0x100000)
    u.reg_write(UC_ARM_REG_SP, 0x80ffff0)
    u.mem_map(0x9000000, 4096)
    u.reg_write(UC_ARM_REG_LR, 0x9000000)
    u.reg_write(UC_ARM_REG_C1_C0_2, 0xf << 20)
    u.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
    counted = [0, 0]
    if count:
        ranges = chip_ranges(elf_path)
        starts = [r[0] for r in ranges]

        def block(uc, addr, size, data):
            counted[1] += size // 4
            i = bisect.bisect_right(starts, addr) - 1
            if i >= 0 and addr < ranges[i][1]:
                counted[0] += size // 4
        u.hook_add(UC_HOOK_BLOCK, block)
    u.emu_start(syms['main'], 0x9000000)
    word = lambda name, n: struct.unpack('<Q' if n == 8 else '<I', u.mem_read(syms[name], n))[0]
    return dict(hash=f"{word('result', 8):016x}", frames=word('result_frames', 4),
                channel_frames=word('result_channel_frames', 4), chip_instructions=counted[0],
                all_instructions=counted[1])


def sizes(elf_path):
    out = subprocess.check_output([str(NM), '-S', '--size-sort', str(elf_path)], text=True)
    keep = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2] in 'tT' and not parts[3].startswith('harness_') \
                and parts[3] not in HARNESS:
            keep[parts[3]] = int(parts[1], 16)
    return keep


def main():
    a = argparse.ArgumentParser()
    a.add_argument('--reference', default='HEAD')
    a.add_argument('--programs', type=int, default=300)
    a.add_argument('--ticks', type=int, default=1200)
    a.add_argument('--interpolate', type=int, choices=(0, 1), default=None,
                   help='bench interpolation (default: both)')
    a.add_argument('--define', action='append', default=[],
                   help='extra define for the working-tree build only')
    a.add_argument('--reference-define', action='append', default=[],
                   help='extra define for the reference build only')
    a.add_argument('--skip-coverage', action='store_true')
    a.add_argument('--expect-differ', action='store_true',
                   help='report digests without failing (deliberate output changes)')
    args = a.parse_args()
    failed = False
    with tempfile.TemporaryDirectory(prefix='kohjoh-sound-') as tmp:
        tmp = Path(tmp)
        ref = tmp / 'reference.c'
        ref.write_text(subprocess.check_output(['git', 'show', f'{args.reference}:{CHIP}'],
                                               cwd=ROOT, text=True))
        variants = [('reference', ref, args.reference_define), ('working', ROOT / CHIP, args.define)]
        if not args.skip_coverage:
            digests = []
            for name, source, defines in variants:
                exe = tmp / f'{name}-coverage.elf'
                build(source, exe, [f'-DSOUND_PROGRAM_COVERAGE={args.programs}'], defines)
                digests.append(run(exe, False)['hash'])
            same = digests[0] == digests[1]
            failed |= not same and not args.expect_differ
            print(f"{'PASS' if same else 'DIFFER'} coverage {args.programs} programs: "
                  f'{digests[0]} {digests[1]}', flush=True)
        modes = [args.interpolate] if args.interpolate is not None else [0, 1]
        for interpolate in modes:
            results = []
            for name, source, defines in variants:
                exe = tmp / f'{name}-bench-{interpolate}.elf'
                build(source, exe, [f'-DSOUND_PROGRAM_BENCH={args.ticks}',
                                    f'-DSOUND_BENCH_INTERPOLATE={interpolate}'], defines)
                r = run(exe, True)
                r['sizes'] = sizes(exe)
                results.append(r)
            same = results[0]['hash'] == results[1]['hash']
            failed |= not same and not args.expect_differ
            print(f"{'PASS' if same else 'DIFFER'} bench interpolate={interpolate}: "
                  f"{results[0]['hash']} {results[1]['hash']}", flush=True)
            for (name, _, _), r in zip(variants, results):
                per = r['chip_instructions'] / max(r['channel_frames'], 1)
                print(f"  {name:9} mixer instructions {r['chip_instructions']:>11} "
                      f"per channel-frame {per:7.2f} (frames {r['frames']}, "
                      f"channel-frames {r['channel_frames']})", flush=True)
            delta = results[1]['chip_instructions'] / max(results[0]['chip_instructions'], 1) - 1
            print(f'  change {delta * 100:+.2f}%', flush=True)
            names = sorted(set(results[0]['sizes']) | set(results[1]['sizes']),
                           key=lambda n: -max(results[0]['sizes'].get(n, 0),
                                              results[1]['sizes'].get(n, 0)))
            print('  function bytes (reference -> working): ' + ', '.join(
                f"{n} {results[0]['sizes'].get(n, 0)}->{results[1]['sizes'].get(n, 0)}"
                for n in names[:8]), flush=True)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
