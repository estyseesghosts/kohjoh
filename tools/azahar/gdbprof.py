#!/usr/bin/env python3
"""Statistical PC sampler for a program running in Azahar (GDB stub).

Azahar is not a timing authority, but it charges one tick per guest
instruction (measured: [calib] in the log), so a PC histogram from it is an
instruction-count profile: exactly what is needed to find where the ARM11
work goes. Real O3DS cost is higher wherever code misses caches.

    python tools/azahar/gdbprof.py build/kj/pokeheartgold-3ds.elf \
        --samples 2000 --interval 0.01 --delay 140

Speaks the GDB remote serial protocol directly (devkitARM's gdb has no
Python): interrupt (0x03), read the PC (register 15), continue.
"""
from __future__ import annotations

import argparse
import bisect
import collections
import socket
import subprocess
import sys
import time


class Rsp:
    def __init__(self, host: str, port: int):
        self.s = socket.create_connection((host, port), timeout=10)
        self.buf = b""

    def _read_packet(self) -> str:
        while True:
            while b"$" not in self.buf:
                self.buf += self.s.recv(4096)
            start = self.buf.index(b"$")
            while b"#" not in self.buf[start:] or len(self.buf) < self.buf.index(b"#", start) + 3:
                self.buf += self.s.recv(4096)
            end = self.buf.index(b"#", start)
            data = self.buf[start + 1:end]
            self.buf = self.buf[end + 3:]
            self.s.sendall(b"+")
            return data.decode("latin-1")

    def send(self, data: str) -> None:
        csum = sum(data.encode("latin-1")) & 0xFF
        self.s.sendall(f"${data}#{csum:02x}".encode("latin-1"))
        # wait for ack
        while True:
            if not self.buf:
                self.buf += self.s.recv(4096)
            c, self.buf = self.buf[:1], self.buf[1:]
            if c == b"+":
                return
            if c == b"-":
                self.s.sendall(f"${data}#{csum:02x}".encode("latin-1"))

    def request(self, data: str) -> str:
        self.send(data)
        return self._read_packet()

    def interrupt(self) -> str:
        self.s.sendall(b"\x03")
        return self._read_packet()


def load_symbols(elf: str, nm: str):
    out = subprocess.run([nm, "-n", "-C", elf], capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        parts = line.split(None, 2)
        if len(parts) == 3 and parts[1].lower() in ("t", "w"):
            addrs.append(int(parts[0], 16))
            names.append(parts[2])
    return addrs, names


def sample(a, r, hist, callers, watch, sym, pcs):
    for _ in range(a.samples):
        time.sleep(a.interval)
        r.interrupt()
        pc_hex = r.request("p f")
        try:
            pc = int.from_bytes(bytes.fromhex(pc_hex[:8]), "little")
        except ValueError:
            continue
        name = sym(pc)
        hist[name] += 1
        if name in a.pcs_set:
            pcs[pc] += 1
        if name in watch:
            lr_hex = r.request("p e")
            try:
                lr = int.from_bytes(bytes.fromhex(lr_hex[:8]), "little")
                callers[f"{name} <- {sym(lr)}"] += 1
            except ValueError:
                pass
        r.send("c")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("--port", type=int, default=24689)
    ap.add_argument("--samples", type=int, default=1000)
    ap.add_argument("--interval", type=float, default=0.01)
    ap.add_argument("--delay", type=float, default=0.0, help="seconds to let the program run first")
    ap.add_argument("--nm", default="C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--callers", default="", help="comma list of functions: also histogram their callers (LR)")
    ap.add_argument("--pcs", default="", help="comma list of functions: also histogram their PCs")
    a = ap.parse_args(argv)
    a.pcs_set = set(x for x in a.pcs.split(",") if x)
    pcs = collections.Counter()

    addrs, names = load_symbols(a.elf, a.nm)
    r = Rsp("127.0.0.1", a.port)
    r.request("?")
    r.send("c")
    if a.delay:
        time.sleep(a.delay)
    hist = collections.Counter()
    callers = collections.Counter()
    watch = set(x for x in a.callers.split(",") if x)

    def sym(addr):
        j = bisect.bisect_right(addrs, addr) - 1
        return names[j] if j >= 0 else f"0x{addr:08x}"

    try:
        sample(a, r, hist, callers, watch, sym, pcs)
    except (ConnectionError, OSError) as e:
        print(f"(sampling stopped early: {e})")
    total = sum(hist.values()) or 1
    print(f"{total} samples")
    for name, n in hist.most_common(a.top):
        print(f"{100.0 * n / total:6.2f}%  {n:5d}  {name}")
    if callers:
        print("callers:")
        for name, n in callers.most_common(a.top):
            print(f"  {n:5d}  {name}")
    if pcs:
        print("pcs:")
        for pc, n in sorted(pcs.items()):
            print(f"  0x{pc:08x} {n:5d}")
    try:
        r.interrupt()
        r.send("D")
    except (ConnectionError, OSError):
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
