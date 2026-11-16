"""run.py: boot kohjoh in LOTUS and capture its screens.

LOTUS runs the same Azahar-derived core as tools/azahar/run.ps1 but drives it
from a native worker over a local MCP server, so frames are advanced
deterministically instead of being screenshotted at wall-clock times. Like
Azahar, LOTUS is an integration debugger, never an O3DS timing authority
(kohjoh.md 1): every result written here is EMULATOR-only evidence.

    python tools/lotus/run.py --pack build/private/hg.pak --tag boot
    python tools/lotus/run.py --frames 3000 --shots 900,1500,2600 --tag intro

Every run creates a fresh LOTUS session with a private SD card, so the pack,
save and input script are staged into that session before the program starts.
The session root is data/kohjoh/sessions/<id>, separate from the profile the
OpenCode `lotus` MCP server uses.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import threading
import time
import zlib

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_LOTUS = r"C:\Users\terezi\workdir\LOTUS-v0.0.1-windows-x64"
SD_ROOT = "3ds/kohjoh"
BUTTONS = {"A": 1, "B": 2, "SELECT": 4, "START": 8, "RIGHT": 16, "LEFT": 32,
           "UP": 64, "DOWN": 128, "R": 256, "L": 512, "X": 1024, "Y": 2048}


def write_png(path: str, pixels: bytes, width: int, height: int) -> None:
    """Write BGRA8 pixels as an RGBA PNG using only the standard library."""
    rows = bytearray()
    for y in range(height):
        line = pixels[y * width * 4:(y + 1) * width * 4]
        rows.append(0)  # filter: none
        for x in range(0, len(line), 4):
            b, g, r, a = line[x:x + 4]
            rows += bytes((r, g, b, a))

    def chunk(kind: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", header))
        f.write(chunk(b"IDAT", zlib.compress(bytes(rows), 6)))
        f.write(chunk(b"IEND", b""))


class ToolError(RuntimeError):
    def __init__(self, payload):
        super().__init__(payload)
        self.payload = payload


class LotusMCP:
    """Minimal stdio MCP client for bin/lotus-mcp.exe (rmcp 3.10, JSON lines)."""

    def __init__(self, executable: str, config: str):
        self.proc = subprocess.Popen(
            [executable, config], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self._next = 0
        self._stderr: list[str] = []
        threading.Thread(target=self._drain, daemon=True).start()
        self.request("initialize", {
            "protocolVersion": "2024-11-05", "capabilities": {},
            "clientInfo": {"name": "kohjoh-run", "version": "0"}})
        self.notify("notifications/initialized", {})

    def _drain(self) -> None:
        for line in self.proc.stderr:
            self._stderr.append(line.decode(errors="replace"))

    def notify(self, method: str, params: dict) -> None:
        self.proc.stdin.write(json.dumps(
            {"jsonrpc": "2.0", "method": method, "params": params}).encode() + b"\n")
        self.proc.stdin.flush()

    def request(self, method: str, params: dict, timeout: float = 300) -> dict:
        self._next += 1
        rid = self._next
        self.proc.stdin.write(json.dumps(
            {"jsonrpc": "2.0", "id": rid, "method": method, "params": params}).encode() + b"\n")
        self.proc.stdin.flush()
        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("lotus-mcp closed its output")
            message = json.loads(line)
            if message.get("id") == rid:
                if "error" in message:
                    raise RuntimeError(f"{method}: {message['error']}")
                return message
        raise TimeoutError(method)

    def call(self, name: str, arguments: dict):
        message = self.request("tools/call", {"name": name, "arguments": arguments})
        result = message["result"]
        for item in result["content"]:
            if item.get("type") == "text":
                text = item["text"]
                try:
                    payload = json.loads(text)
                except ValueError:
                    payload = text
                if result.get("isError"):
                    if isinstance(payload, str):
                        brace = payload.find("{")
                        if brace > 0:
                            try:
                                detail = json.loads(payload[brace:])
                            except ValueError:
                                detail = None
                            if isinstance(detail, dict):
                                detail["_status"] = payload[:brace].strip(": ")
                                payload = detail
                    raise ToolError(payload)
                return payload
        if result.get("isError"):
            raise ToolError(result["content"])
        return result["content"]

    def read_resource(self, uri: str) -> bytes:
        import base64
        contents = self.request("resources/read", {"uri": uri})["result"]["contents"]
        return base64.b64decode(contents[0]["blob"])

    def close(self) -> str:
        try:
            self.request("shutdown", {}, timeout=30)
            self.notify("exit", {})
            self.proc.wait(timeout=30)
        except Exception:
            self.proc.kill()
        return "".join(self._stderr)


def host_config(lotus: str, artifact: str, backend: str) -> tuple[str, str]:
    """Write a dedicated host profile; return (config path, session root)."""
    profile = os.path.join(lotus, "data", "kohjoh")
    storage = os.path.join(profile, "sessions")
    os.makedirs(storage, exist_ok=True)
    config = os.path.join(profile, "host.json")
    text = {
        "worker": os.path.join(lotus, "runtimes", backend, "ctr-worker.exe"),
        "storage_root": storage,
        "assets": {"homebrew": artifact},
        "allow_control": True,
    }
    with open(config, "w", encoding="utf-8") as f:
        json.dump(text, f, indent=1)
    return config, storage


def stage(session_root: str, edition: str, pack: str | None, manifest: str | None,
          save: str | None, script: str | None, fresh_save: bool) -> None:
    sd = os.path.join(session_root, "sdmc", SD_ROOT)
    for sub in ("data", "saves", "logs"):
        os.makedirs(os.path.join(sd, sub), exist_ok=True)
    if pack:
        shutil.copyfile(pack, os.path.join(sd, "data", edition + ".pak"))
    if manifest:
        shutil.copyfile(manifest, os.path.join(sd, "data", edition + ".manifest.json"))
    save_path = os.path.join(sd, "saves", edition + ".sav")
    if save:
        shutil.copyfile(save, save_path)
    elif fresh_save and os.path.exists(save_path):
        os.remove(save_path)
    script_path = os.path.join(sd, "script.txt")
    if script:
        shutil.copyfile(script, script_path)
    elif os.path.exists(script_path):
        os.remove(script_path)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--artifact", default="build/kj/pokeheartgold.3dsx")
    p.add_argument("--edition", default="heartgold", choices=("heartgold", "soulsilver"))
    p.add_argument("--pack", help="game pack staged into this session's SD card")
    p.add_argument("--manifest", help="pack manifest beside the pack")
    p.add_argument("--save", help="save to install in the session SD card")
    p.add_argument("--fresh-save", action="store_true")
    p.add_argument("--script", help="scripted input file (tests/inputs/*.txt)")
    p.add_argument("--frames", type=int, default=600, help="frames to advance")
    p.add_argument("--chunk", type=int, default=60,
                   help="frames per native request (each has a 20 s host deadline)")
    p.add_argument("--shots", default="", help="comma-separated frames to capture at")
    p.add_argument("--press", action="append", default=[], metavar="FRAME:KEYS",
                   help="press keys at a frame, e.g. 600:START or 900:DOWN+A "
                        "(repeatable; keys are %s)" % "/".join(BUTTONS))
    p.add_argument("--touch", action="append", default=[], metavar="FRAME:X,Y",
                   help="touch the bottom screen at native 320x240 coordinates "
                        "for --press-frames frames (repeatable)")
    p.add_argument("--press-frames", type=int, default=8,
                   help="frames the keys stay held for each --press")
    p.add_argument("--hold", action="store_true",
                   help="pause before each capture (screens stay valid while paused)")
    p.add_argument("--backend", default="AZAHAR", choices=("AZAHAR", "CITnLO"))
    p.add_argument("--lotus", default=os.environ.get("LOTUS_HOME", DEFAULT_LOTUS))
    p.add_argument("--out-dir", default="build/lotus")
    p.add_argument("--tag", default="run")
    a = p.parse_args()

    artifact = os.path.abspath(os.path.join(REPO, a.artifact))
    if not os.path.isfile(artifact):
        raise SystemExit(f"no artifact: {artifact}")
    mcp = os.path.join(a.lotus, "bin", "lotus-mcp.exe")
    config, storage = host_config(a.lotus, artifact, a.backend)
    out = os.path.join(REPO, a.out_dir, a.tag)
    os.makedirs(out, exist_ok=True)

    shots = sorted({int(s) for s in a.shots.split(",") if s.strip()} | {a.frames})
    shots = [s for s in shots if 0 < s <= a.frames]
    presses = []
    for spec in a.press:
        frame, _, keys = spec.partition(":")
        mask = 0
        for key in keys.split("+"):
            key = key.strip().upper()
            if key:
                if key not in BUTTONS:
                    raise SystemExit(f"unknown key {key!r} in --press {spec}")
                mask |= BUTTONS[key]
        presses.append((int(frame), mask, None))
    for spec in a.touch:
        frame, _, point = spec.partition(":")
        x, _, y = point.partition(",")
        presses.append((int(frame), 0, (int(x), int(y))))

    server = LotusMCP(mcp, config)
    session = None
    try:
        print("asset", hashlib.sha256(open(artifact, "rb").read()).hexdigest()[:16],
              os.path.getsize(artifact), "bytes")
        session = server.call("lotus_create_session", {"asset": "homebrew"})["session"]
        root = os.path.join(storage, session)
        stage(root, a.edition, *[os.path.abspath(x) if x else None
                                for x in (a.pack, a.manifest, a.save)],
              os.path.abspath(a.script) if a.script else None, a.fresh_save)
        print("session", session, "->", root)
        server.call("lotus_control", {"session": session, "operation": "reset"})

        done = 0

        def advance(target: int) -> None:
            """Advance to `target` frames, tolerating the 20 s per-request deadline."""
            nonlocal done
            while done < target:
                step = min(a.chunk, target - done)
                try:
                    status = server.call("lotus_run_until",
                                         {"session": session, "frames": step})
                except ToolError as exc:
                    payload = exc.payload if isinstance(exc.payload, dict) else {}
                    frames = payload.get("frames", done)
                    if payload.get("last_error") != "host_deadline" or frames <= done:
                        raise
                    done = frames
                    continue
                if status.get("last_error"):
                    raise SystemExit(f"guest error: {status['last_error']}")
                previous, done = done, status["frames"]
                if done // 300 != previous // 300:
                    print(f"  .. {done} frames", flush=True)

        def key(mask: int, point: tuple[int, int] | None = None) -> None:
            x, y = point if point else (0, 0)
            server.call("lotus_input", {"session": session, "owner": "live",
                                        "buttons": mask, "x": x, "y": y,
                                        "touch": point is not None})

        def capture(frame: int) -> None:
            if a.hold:
                server.call("lotus_control", {"session": session, "operation": "pause"})
            links = [c for c in server.request(
                "tools/call", {"name": "lotus_capture",
                               "arguments": {"session": session}})["result"]["content"]
                     if c.get("type") == "resource_link"]
            for link in links:
                raw = server.read_resource(link["uri"])
                w = 400 if link["size"] == 384000 else 320
                path = os.path.join(out, f"f{frame:05d}-{link['name']}.png")
                write_png(path, raw, w, 240)
                print(f"f{frame:05d} {link['name']} "
                      f"{link['description'].split('SHA-256 ')[-1]} -> {path}")

        # Shots and inputs share one frame-ordered queue; an input advances the
        # clock by --press-frames, so a later shot captures that later frame.
        events = sorted([(f, "shot", 0, None) for f in shots] +
                        [(f, "input", mask, point) for f, mask, point in presses],
                        key=lambda e: (e[0], e[1] != "shot"))
        for frame, kind, mask, point in events:
            if kind == "shot":
                advance(frame)
                capture(frame)
                continue
            advance(frame)
            label = f"0x{mask:03x}" if mask else f"touch {point[0]},{point[1]}"
            print(f"f{frame:05d} input {label}", flush=True)
            key(mask, point)
            advance(done + a.press_frames)
            key(0)

        status = server.call("lotus_session_status", {"session": session})
        print("frames", status["frames"], "audio samples", status["audio_samples"],
              "dropped", status["audio_dropped"])
        print("log:", os.path.join(root, "sdmc", SD_ROOT, "logs", a.edition + ".log"))
    except Exception as exc:
        print("FAILED:", exc, file=sys.stderr)
        if session:
            print("session root:", os.path.join(storage, session), file=sys.stderr)
        return 1
    finally:
        stderr = server.close()
        if stderr.strip():
            print("worker stderr:\n" + stderr[:4000], file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
