# Running kohjoh in LOTUS

LOTUS is a second integration debugger beside Azahar. It runs the same
Azahar-derived core as `tools/azahar/run.ps1` but drives it from a native worker
process over a local MCP server, so frames are advanced deterministically
instead of being screenshotted at wall-clock times. It is **not** an O3DS timing
authority either: every result from it is EMULATOR-only evidence
([kohjoh.md 1](../kohjoh.md)).

Installed at `C:\Users\terezi\workdir\LOTUS-v0.0.1-windows-x64` (portable ZIP,
v0.0.1). Its own documentation is in that folder's `docs/`; the relevant pages
are `20`-`26`, especially `21-native-control.md` (worker protocol) and
`23-local-mcp.md` (the stdio MCP surface).

## Fixed configuration

The native worker runs Old 3DS, software rendering, HLE services, HLE/muted DSP,
guest ticks 0 and guest time 2000-01-01 UTC, with no window. Nothing here is
configurable through MCP; the host JSON is trusted startup input that the model
cannot change.

## Backend

Both backends run kohjoh. They are the same Azahar core at the same pin
(`05773bbb5`); the only build difference is `-DCTR_BACKEND_NAME`, which changes
the reported `backend_name` and nothing else. `LOTUS.cmd`/`CITnLO` is the
default; `LOTUS-AZAHAR.cmd`/`--backend AZAHAR` is the upstream-tracking one.

Two things look like failures and are not:

- **`Failed to find title id for ROM (Error 0)`** is logged by *every* `.3dsx`
  load, including sessions that then run for thousands of frames, so it is not a
  usable failure signal. kohjoh's `.3dsx` has no SMDH (`header_size = 0x20`, no
  `SMDH` magic, because `make_smdh` finds no icon), which is what the
  `SelfNCCH ... might be a 3dsx file` warning refers to; neither backend needs
  the banner. The Homebrew Launcher on real hardware does not either.
- **A session that ends with an 8-line worker log and an empty `sdmc/`** was
  usually created and then abandoned: sessions die with the MCP server, and
  `lotus_create_session` has by then loaded and paused the guest, so closing the
  server immediately leaves no guest log at all. Confirm with
  `lotus_session_status` before concluding the load failed.

## Sessions and the private SD card

Each session is its own worker process with its own root:

```text
<lotus>/data/<profile>/sessions/<handle>/
  sdmc/            # guest SD card: sdmc:/ maps here
  nand/  sysdata/  log/  cheats/
```

`lotus_create_session` loads the approved asset **immediately** and pauses, so
kohjoh starts before its data pack exists unless the pack is already there. The
sequence is therefore:

1. create the session (kohjoh boots, reports no pack, idles),
2. copy the pack, manifest, save and input script into that session's `sdmc`,
3. `reset` — the worker reloads the same content with the new SD contents.

Sessions belong to the MCP server process that created them and cannot be
re-attached later: closing the server (or OpenCode restarting it) ends them, and
each new session needs its own staged copy of the pack (125 MB, a few seconds on
local disk).

Guest paths kohjoh uses:

```text
sdmc:/3ds/kohjoh/data/<edition>.pak
sdmc:/3ds/kohjoh/data/<edition>.manifest.json
sdmc:/3ds/kohjoh/saves/<edition>.sav
sdmc:/3ds/kohjoh/script.txt
sdmc:/3ds/kohjoh/logs/<edition>.log
sdmc:/3ds/kohjoh/dumps/
```

## Harness: `tools/lotus/run.py`

The equivalent of `tools/azahar/run.ps1` for LOTUS. It creates a session,
stages the requested files, resets, advances frames in chunks, presses buttons
and writes PNG captures plus the guest log location.

```sh
python tools/lotus/run.py --pack build/prepare-test/heartgold.pak \
  --manifest build/prepare-test/heartgold.manifest.json \
  --artifact build/kj-gold-normal/pokeheartgold.3dsx \
  --frames 3000 --shots 1200,1800,2400,3000 --tag boot

python tools/lotus/run.py --artifact build/kj-gold-normal/pokeheartgold.3dsx \
  --pack build/prepare-test/heartgold.pak --manifest build/prepare-test/heartgold.manifest.json \
  --frames 1200 --press 600:START --shots 1200 --tag title
```

| Option | Effect |
| --- | --- |
| `--artifact` | `.3dsx` to run (default `build/kj/pokeheartgold.3dsx`) |
| `--edition` | `heartgold` (default) or `soulsilver`, selects the SD file names |
| `--pack`, `--manifest` | stage the data pack into this session's SD card |
| `--save path` / `--fresh-save` | install a save / delete the session's save |
| `--script file` | scripted input (`tests/inputs/*.txt`); absent, none is staged |
| `--frames`, `--chunk` | total frames, and frames per native request (default 60) |
| `--shots 10,30` | capture at those frame numbers (the last frame always captured) |
| `--press FRAME:KEYS` | hold keys over a frame boundary, e.g. `900:DOWN+A`, `600:START` |
| `--touch FRAME:X,Y` | touch the bottom screen at native `320x240` coordinates, e.g. `5000:160,120` |
| `--hold` | pause before each capture |
| `--backend`, `--lotus` | `AZAHAR` (default) or `CITnLO`; LOTUS install root |
| `--tag`, `--out-dir` | output folder: `build/lotus/<tag>/` |

Captures are written as `f<frame>-top.png` (400x240) and `f<frame>-bottom.png`
(320x240), converted from the worker's normalized BGRA8 output. Each capture's
SHA-256 is printed, so pictures can be compared with Azahar or melonDS runs.

## MCP from OpenCode

`C:\Users\terezi\workdir\opencode.json` registers the server, so this session and
any future one get the `lotus_*` tools:

```json
{
  "mcp": {
    "servers": {
      "lotus": {
        "type": "local",
        "command": [
          "C:\\Users\\terezi\\workdir\\LOTUS-v0.0.1-windows-x64\\bin\\lotus-mcp.exe",
          "C:\\Users\\terezi\\workdir\\LOTUS-v0.0.1-windows-x64\\data\\azahar\\host.json"
        ]
      }
    }
  }
}
```

Re-register with `opencode mcp add lotus -- <lotus-mcp.exe> <host.json>` and
check with `opencode mcp list`. The host JSON at the path above is the server's
whole configuration:

```json
{
  "worker": ".../runtimes/azahar/ctr-worker.exe",
  "storage_root": ".../data/azahar/sessions",
  "assets": {"homebrew": ".../build/kj-gold-normal/pokeheartgold.3dsx"},
  "allow_control": true
}
```

`launch.ps1 -Backend AZAHAR -Mode MCP -Fixture <3dsx>` rewrites it with absolute
paths, which is the supported way to point the server at a different artifact
(do not run it with `-Mode MCP` while the server is connected: it starts a
second, competing stdio server).

| Tool | Use |
| --- | --- |
| `lotus_status` | protocol, owned sessions, approved asset handles |
| `lotus_create_session` | load `homebrew` (the only asset) into a new private session |
| `lotus_run_until` | advance 1-600 frames, 20 host-second deadline |
| `lotus_control` | `pause`, `reset`, `stop` |
| `lotus_input` | complete buttons/touch state; owner `live` |
| `lotus_capture` | two `lotus://artifact/...` resource links (BGRA8) |
| `lotus_session_status` | frames, audio counters, last error |
| `lotus_read_memory` | 1-4096 mapped bytes at a safe point |

An agent driving kohjoh by hand:

1. `lotus_create_session {"asset": "homebrew"}` - note the session handle.
2. Copy the pack and manifest into
   `data/azahar/sessions/<handle>/sdmc/3ds/kohjoh/data/`.
3. `lotus_control {"session": ..., "operation": "reset"}`.
4. `lotus_run_until` in chunks of ~60 frames; read `frames` from the reply.
5. `lotus_capture`, then read each URI from `resources/read` (base64 blob).

The worker enforces four sessions, 600 frames and a 20 host-second deadline per
call, and 128 retained captures; a call that hits the deadline still reports the
frames it reached (`last_error: host_deadline`), so retry from that number.

## Limits and what LOTUS cannot show

- **Speed**: the software renderer runs far slower than real time for kohjoh's
  3D scenes; a 60-frame request can hit its 20-second deadline mid-intro. Do not
  read host frame rates as guest frame rates.
- **Audio**: no DSP firmware is supplied, so kohjoh logs
  `audio unavailable (is the DSP firmware dumped?); continuing silent` and
  `audio_samples` stays 0. Use Azahar with `sdmc/3ds/dspfirm.cdc` for anything
  audio.
- **No profiler**: LOTUS exposes registers and bounded memory reads only. Use
  Azahar's GDB stub (`tools/azahar/gdbprof.py`) and `[perf]`/`[gamecpu]` logs
  for hotspots.
- **No save-state or rewind**, no breakpoints, no trace capture: re-run from
  frame 0 instead.
- **Guest time is fixed** (2000-01-01) and ticks start at 0, which is
  reproducible but not a real console's RTC.

## Evidence so far

Observed on 2026-11-16, LOTUS v0.0.1 (AZAHAR backend, worker source
`05773bbb5`), artifact `build/kj-gold-normal/pokeheartgold.3dsx`
(`fb0440a-dirty`, sha256 `b3b14a4d…b212ed4`) with
`build/prepare-test/heartgold.pak`:

- **Run (emulator)**: the guest boots to the intro and plays it. The log shows
  `kohjoh fb0440a-dirty (heartgold), homebrew (SD pack), Old 3DS`,
  `renderer ready (linear free 26393 KiB, VRAM free 4794 KiB)`,
  `audio unavailable ...; continuing silent`, and
  `overlay intro_title loaded` — so the pack, overlays and GPU path are live.
  Captured: Game Freak splash (f1200), Lyra/New Bark 2D scene with the widened
  framing (f1700), trainer silhouette (f2400), legendary montage (f2700),
  Lugia (f3000), the title screen with Ho-Oh and `TOUCH TO START` (f4260), and
  the first-run info screen with `CONTROL INFO / ADVENTURE INFO / NO INFO
  NEEDED` after a `START` press and a bottom-screen touch (f5400).
- **Run (tooling)**: `drawbench.3dsx` produces top
  `f4bf5eed…e13ef` and bottom `d0600b9d…96cc`, byte-identical to the hashes
  LOTUS's release notes publish, which validates the capture conversion and
  staging path independently of kohjoh.
- **Both backends**: CITnLO runs the unmodified `fb0440a` `.3dsx` (30 native
  frames, guest log written) exactly as AZAHAR does. The `Failed to find title
  id` line is present in both and carries no information about success.
- **Not verified**: gameplay past the first-run info screen, saves, and
  `KJ_SND_*`/audio behaviour (no DSP firmware in a LOTUS session). Do not add a
  hardware or timing claim from any of this.
