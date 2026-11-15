"""Summarise [sndprof] lines from a kohjoh log.

python tools/azahar/sndprof.py LOG [--from SECONDS] [--to SECONDS]

Prints, between the first and last [sndprof] lines inside the window:
mixer ticks per channel-sample (Azahar: instructions; O3DS: cycles), the
mixer's and sequencer's share of a 268 MHz core, and active channels per
mix call. Counters are cumulative, so only differences are used. With
several runs in one log, pick the window with --from/--to or pass a log
that holds one run.
"""
import argparse
import re

CORE_HZ = 268111856
LINE = re.compile(r'^\s*([0-9.]+) \[sndprof\] (.*)$')


def parse(path, start, end):
    rows = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            if '--- new run ---' in line:
                rows.append(None)
                continue
            m = LINE.match(line)
            if not m:
                continue
            t = float(m.group(1))
            if t < start or t > end:
                continue
            words = m.group(2).split()
            values = {}
            for key, value in zip(words[0::2], words[1::2]):
                try:
                    values[key] = int(value)
                except ValueError:
                    pass
            values['time'] = t
            rows.append(values)
    # Only the last run in the window.
    if None in rows:
        rows = rows[len(rows) - rows[::-1].index(None):]
    return rows


def main():
    a = argparse.ArgumentParser()
    a.add_argument('log')
    a.add_argument('--from', dest='start', type=float, default=0.0)
    a.add_argument('--to', dest='end', type=float, default=1e12)
    args = a.parse_args()
    rows = parse(args.log, args.start, args.end)
    if len(rows) < 2:
        raise SystemExit('need two [sndprof] lines in the window')
    first, last = rows[0], rows[-1]
    d = {k: last[k] - first[k] for k in last if k in first}
    seconds = d['time']
    channels = d['active_ch'] / max(d['mix_calls'], 1)
    channel_samples = channels * d['mix_frames']
    print(f"window {first['time']:.1f}-{last['time']:.1f} s ({len(rows) - 1} intervals)")
    print(f"mix ticks per channel-sample {d['mix_ticks'] / max(channel_samples, 1):.2f}")
    print(f"active channels per mix {channels:.2f}; "
          f"adpcm {d.get('adpcm_ch', 0)} pcm8 {d.get('pcm8_ch', 0)} pcm16 {d.get('pcm16_ch', 0)} "
          f"psg {d.get('psg_ch', 0)} capture {d.get('cap_inst', 0)}")
    print(f"mixer core share {d['mix_ticks'] / seconds / CORE_HZ * 100:.2f}%  "
          f"sequencer {d['tick_ticks'] / seconds / CORE_HZ * 100:.2f}%")
    extra = [k for k in sorted(d) if k not in (
        'time', 'render_calls', 'render_frames', 'stretches', 'sndticks', 'mix_ticks',
        'tick_ticks', 'mix_calls', 'mix_frames', 'active_ch', 'pcm8_ch', 'pcm16_ch', 'adpcm_ch',
        'psg_ch', 'cap_inst', 'dep_ch')]
    for k in extra:
        print(f'  {k} {d[k]} (latest {last[k]})')


if __name__ == '__main__':
    main()
