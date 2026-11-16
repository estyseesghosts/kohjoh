I’ll start by reading the project doc and surveying the workdir.

Ran 3 commands, read kohjoh.md
Big scope. Let me survey the environment: toolchain, Azahar, reference repos, ROMs, and the docs folder.

ROMs, toolchain and reference repos are present. Checking devkitARM, Azahar, and the owner’s engineering docs.

Now the prior art — the hgss-3ds structure, its README, and Coffeepot’s HGSS case study, which should tell me where time actually went.

The case study shows game logic is cheap, so the cost is in the renderer; I’m now looking at how hgss-3ds compiles the game to see what to reuse.

Network, ninja and 32 cores are available, but makerom and bannertool are missing and I’ll fetch them pinned later. Next I’m tracing how the game gets compiled, since the build stays even though the renderer is being replaced.

The build has several learned workarounds, so before deciding what to keep I’m checking whether the existing game build runs here and how long it takes, in the background into scratch.

While that builds, the game loop itself — this decides the scheduling bridge.

The main loop already runs logic at 30 Hz, so a single-threaded design fits: the game thread can publish PICA commands right after the second VBlank, with no drawer-thread races. I’m moving on to the Coffeepot tile-renderer measurements that will drive the 2D design.

Compiler temp-file failure, since TMP points at C:\Windows in this shell. Rerunning with TMP set to scratch.

Memory layout is settled: DS regions are plain arrays, BG/OBJ VRAM are logical views, and texture and ext-palette banks sit in an LCDC array. Next I’m pulling the exact hook surface the patched SDK calls, since that’s the boundary my new runtime has to satisfy.

I’ve finished reading the docs and surveying hgss-3ds, and found that MSYS strips TMP/TEMP from child processes, so I’m setting them explicitly in my build driver. Next I’m retrying the reference build with that fix to get a known-good baseline.

Meanwhile, setting up the kohjoh repo: identity, pins and skeleton.

Ran 3 commands, created upstream.lock
While that compiles, the owner’s 2D and frame-pipeline contracts:

The 2D design fits the plan, with a palette-expanded atlas, stencil for blend targets, and ordered layer passes. I’m checking the reference build next.

The archive step’s “argument list too long” failure comes from my deep scratch path, not the compiles, so I’m switching to short in-repo build paths before reproducing the bootstrap and header-generation steps.

Reusing the decomp’s GCC compile layer would carry over hgss-3ds’s fragile driver, so I’m vendoring only that layer with provenance and rewriting the shell, scheduler, renderer, storage and packaging. Next I’m checking how the nitro library picks its sources so I can cut out the renderer.

I’ll vendor the compile layer into src/compat (renderer files stripped) and the drivers into tools/game, then let the linker list the missing symbols. Those become my new runtime’s API.

Kohjoh now has the game-compile layer vendored with renderer files stripped and the bootstrap re-pinned, and the decomp checkout and patch stack are applied. I’m fixing a host-tool compile error in the decomp’s gen_fx_consts step next.

Status: the patched decomp is in place. The one gap is a host tool that generates fx_const.h, because MSYS gcc lacks libc headers. I’m switching that step to zig cc.

prepare_game_headers already generates it in Python; bootstrap’s host-tool step is redundant. Dropping it.

Status: bootstrap works except for the SDK link step; MSYS Python makes copies instead of symlinks. Making it refresh copies safely (it never deletes through a junction).

Only line endings differ (CRLF vs LF); contents match. Bootstrap done. Now the first game build with the vendored driver, to collect the missing-runtime symbols.

Build tooling is pinned and bootstrapped. Next I’m splitting the build so the DS code uses Nitro headers with 4-byte enums and the 3DS side uses libctru’s ABI, joined only by a plain-C bridge header.

Now patching the driver: DS-side runtime sources, a separate 3DS-side source set with libctru ABI, and per-directory shader blobs.

Now the bridge header and a minimal runtime, enough to link and expose missing symbols.

Writing the bridge header and minimal runtime to get a first link.

Audio’s port layer is already thread-safe: mixing on an audio thread, alarm replies pumped on the game thread. Writing the DS-side boot and frame bridge:

Next, the frame bridge. HGSS’s patch stack never calls VBlankWorkDone, so I’ll add a game patch that marks the publish point after the vwait task queue.

Now the DS-side frame bridge (kj_frame.c):

The old engine did all vertex transforms and clipping on the CPU, which accounts for its 35 ms per scene. I’m keeping the matrix stack, lighting and texgen on the CPU, moving transforms to the PICA, and checking how port_gx.c exposes GXSTAT next.

Now the geometry engine. First a slim replacement nitro_port_gx3d.h for the compat layer, then kj_g3.c.

Writing the geometry engine (kj_g3.c), following GBATEK semantics and cross-checked against the old engine’s verified lighting and texgen fixed-point:

Ran a command, created kj_g3.c
Fixing a bug before building: palette copies and newly pushed clip matrices share one list, so a batch’s palette slice wouldn’t stay contiguous. Splitting them into a CPU pool and a palette array.

Next is the 3DS-side app. My pacing is one DS V-blank per physical V-blank, so C3D_FrameBegin’s own frame-rate sync must not add a second wait. Checking citro3d 1.7.1’s actual source for that:

Citro3D only waits on the previous GPU queue, so the single per-V-blank gspWaitForVBlank is the sole pacing wait and an unchanged bottom screen can skip its transfer; I’m writing the host app now.

Writing the 3DS-side pieces: logging (ring-buffered, flushed off the frame path), input, app lifecycle and a renderer stub. That’s enough for the first boot test.

Applied (it was just slow I/O). Now save stub, audio, perf counters and renderer stub.

I’ll use Write for these files instead.

Now the renderer skeleton: targets, frame begin/end and publish plumbing. 2D and 3D passes come next.

I referenced a nonexistent kj_vertex_storage in the renderer; fixing that. Next: the screen-rotation math. I’ll take it from citro3d’s own Mtx_OrthoTilt rather than guess.

OrthoTilt’s mapping gives the clip-space transform for the widened 3D, and I’m writing the shaders and render targets next.

PICA allows uniforms only as source 1; reordering operands.

Shaders assemble. Now the PICA layer: targets, programs and attribute layouts, and a bound-state cache (citro3d dirties textures even on a self-rebind).

Claude writes: The tile atlas is set to 1024×1024 RGBA5551, and slot validity is checked by exact byte comparison plus a palette version rather than a hash, so a missed VRAM write can’t leave a stale tile.

The 2D compositor is next: priority-ordered layer passes, stencil bits for blend targets, two-pass alpha blending, and brightness quads. I’m starting with the shared GPU state helpers.

Replacing the generated-header includes with direct extern declarations of the shader blobs:

Before writing 2D, checking where the patched SDK stores extended palettes:

Ext palettes live in the LCDC array at their bank offsets, so they resolve through VRAMCNT. Writing the 2D module:

I’m writing the 2D compositor next, covering text and affine backgrounds, OBJ sprites, per-scanline spans, window masking, and blending.

Fixing a few issues I spotted on review: the 2D shader wasn’t normalising vertex colours, write_windows leaves colour writes off for later layers, and two helper functions were missing.

Now the 3D backend. Checking the old engine’s verified front-face convention so I don’t guess:

DS front faces are counter-clockwise in clip space, which my tilted fixup preserves, and attribute value 0 draws both faces (per the verified old engine). Writing the texture cache and 3D draw:

I’m folding the viewport, widescreen, tilt and depth remap into one fixup matrix applied per palette entry on the CPU, so the vertex shader needs only a single dp4; the texture cache is next.

Now the 3D draw path:

Wiring texture frame-begin into publish, then the first full build:

Remaining hgss/compat path references; fixing all of them.

Source list is a wildcard, so the stripped files drop out automatically (test targets aren’t built). Rebuilding:

Python resolves paths under /home/terezi, which this MSYS’s mkdir -p can’t see; I’ll invoke with absolute /c/ paths. Separately, dsprot.h is missing:

Python’s cwd resolves to /home/terezi while the script path stays /c/.... Passing absolute paths everywhere:

All 1092 game files now compile. Fixing the last header gap (the deleted renderer header supplied <stdint.h>) so the compat libraries build.

Libraries build. The pak reader’s ROM allowlist comes from the ROM verifier; vendoring the pack builder into tools/rompack:

Not incremental: every run recompiles all 1092 game files. Adding a staleness check (object newer than source, unchanged flags, unchanged upstream HEAD), plus the API fix:

Ran 4 commands, read and edited hgss_game.py
grep -n "def compile_one" -A28 tools/game/hgss_game.py
1288:def compile_one(cc: str, flags: list[str], src: Path, obj: Path,
1289-                language: str, log_dir: Path, root: Path | None = None,
1290-                alias_env: dict[str, str] | None = None,
1291-                nm: str | None = None, readelf: str | None = None,
1292-                out: Path | None = None
1293-                ) -> tuple[bool, int, str]:
1294-    obj.parent.mkdir(parents=True, exist_ok=True)
1295-    proc = run(compile_command(cc, flags, src, obj, language))
1296-    text = proc.stdout + proc.stderr
1297-    failures = error_count(text)
1298-    if proc.returncode != 0 and failures == 0:
1299-        failures = 1  # e.g. the compiler itself failed
1300-    failed = proc.returncode != 0
1301-    if not failed and root is not None:
1302-        aliased, alias_text = apply_object_aliases(root, src, obj, alias_env,
1303-                                                   nm, readelf, out)
1304-        if not aliased:
1305-            text += alias_text
1306-            failures = max(1, error_count(alias_text))
1307-            failed = True
1308-    if failed:
1309-        log = log_dir / (str(obj.relative_to(log_dir.parent)) + ".log")
1310-        log.parent.mkdir(parents=True, exist_ok=True)
1311-        log.write_text(text, encoding="utf-8")
1312-        return False, failures, str(log)
1313-    return True, 0, ""
1314-
1315-
1316-def compile_objects(cc: str, flags: list[str], sources: list[tuple[Path, Path, str]],
K=/c/Users/terezi/workdir/kohjoh; (time python $K/tools/game/hgss_game.py --root $K/build/upstream --target 3ds --out $K/build/kj -j 32) > build/kj-build.log 2>&1; echo rc=$?; sed -n '/== compiling the DS/,$p' build/kj-build.log | grep -v "warning:\|note:\|^\s*|\|^\s*[0-9]* |\|In function" | head -60
Finished in background
C:\Users\terezi\AppData\Local\Temp\claude\596f382ffeba1215\1b405a68-14ba-5972-b2d2-2a87a5293b52\tasks\
b8skhnqhp.output
While it builds (this first incremental pass still compiles everything to generate dependency files), checking how hgss-3ds drove Azahar and where its virtual SD lives:

While waiting, writing the provenance record (required before the first commit):

cd /c/Users/terezi/workdir/kohjoh && until grep -q "^real" build/kj-build.log; do sleep 5; done; sed -n '/== compiling the DS/,$p' build/kj-build.log | grep -v "warning:\|note:\|^\s*|\|^\s*[0-9]* |\|In function" | head -60
== compiling the DS-side runtime (11 files)
== compiling the 3DS-side runtime (14 files)
== link -> /c/Users/terezi/workdir/kohjoh/build/kj/pokeheartgold-3ds.elf
   link failed (0 unresolved symbols); log: /c/Users/terezi/workdir/kohjoh/build/kj/link.log
o error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_md5.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_packet.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_platform.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_protocol.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_result.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_screens.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_scroll_bar.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_session.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_settings.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_share.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_state.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_ui_parts.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_ui_system.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_usb_connector_screen.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_wcm.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wifi_setup/wifi_setup_worker.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wireless_manager.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wireless_status_icon.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/wireless_trade_select_mon.o: No error
C:/devkitPro/devkitARM/bin/../lib/gcc/arm-none-eabi/16.1.0/../../../../arm-none-eabi/bin/ld.exe: cannot find /c/Users/terezi/workdir/kohjoh/build/kj/obj/src/yes_no_prompt.o: No error
collect2.exe: error: ld returned 1 exit status


real	3m24.256s
user	0m0.000s
sys	0m0.030s
Native ld can’t resolve /c/... paths inside the response file (MSYS only converts command-line arguments). Writing them as C:/...:

The .d files list generated headers under build/kj/gen that get rewritten every run, which defeats the mtime check. Finding which:

Bug: partition(":") split at the drive letter. Fixing to split at the target’s ": ", and fixing the response-file write:

Ran 2 commands, read and edited hgss_game.py
Down to a single unresolved symbol, NitroPort_EngineAOnTop (formerly in the 2D renderer). Implementing it from POWCNT1:

First full link: pokeheartgold.3dsx (5.49 MB). Committing this milestone.

Committed (75ae75c). Now an Azahar harness: stage the files to the virtual SD, launch, take timed window screenshots and collect the log.

It runs: 600 publishes, every one at exactly two V-blanks, zero misses (Azahar timing only). Screenshots:

The intro scene renders through the GPU path, but sprite tiles look sliced, and the window capture is cropped. Enlarging the window from the harness for readable shots:

Ran 2 commands, read t030.png
Capture is DPI-virtualised (only part of the window comes through). Making the harness DPI-aware:

Ran 2 commands, read t030.png
Azahar shows 30 FPS on the full view, with the Lyra intro recognisable but horizontal 8 px banding that may be a transition or a span bug. I’ll check it against hgss-3ds on later static scenes.

Changed 33 files

upstream.lock
24 additions, 0 deletions
+24
−0

bootstrap.py
1 addition, 2 deletions
+1
−2

kj_bridge.h
205 additions, 0 deletions
+205
−0

See all
5,000 additions, 7 deletions
+5,000
−7
That screenshot is a regression from the build I launched for the route test. It has the display-list event-replay cache I added minutes ago, which is uncommitted and wasn’t verified before that run. Two symptoms:

White map with sprites intact. Map geometry is drawing untextured, while the billboard sprites are fine. Those sprites go through the uncached path, so a replayed block is most likely being drawn in a batch without its texture state.
About 1% speed, ~2.4 s per frame. Something in the record/replay path is pathological, most likely re-recording on every call or an expensive per-call fallback.
What’s safe and what isn’t:

Committed and verified in Azahar (4063c5e): intro and Goldenrod render correctly, and buttons and touch work.
Uncommitted, built before the event-replay cache: the tile-edge inset for your horizontal line gaps, the V-blank pacing lock for the music desync, the geometry fast paths, the retained 2D layers with static-screen skip, and the Pokémon-data keystream cache. Of these, only the geometry fast paths have been run since: Goldenrod rendered correctly and logic dropped from about 37 ms to 25 ms.
The event-replay cache is the suspect.
Proposed next steps:

Turn the event-replay cache off behind a switch and re-run the intro and Goldenrod. That confirms everything else is clean, and then I commit it.
Debug the replay against frame dumps: compare replayed against live-decoded batches for the same frame, and fix the texture and recording bugs before re-enabling it.
Then do your other two items: the missing Lugia/Ho-Oh on the title screen’s lower screen, and the walk south from Goldenrod into the tall grass and a wild battle, with profiling along the way.
Azahar may still be running from that test. Should I go ahead with this plan, or would you rather I handle it differently?

Disabling the event-replay cache behind a switch first:

naw, your plan is good, and you've been delivering good results so far, so keep working. always commit your work.

Saving your standing preference, then verifying:

Created a file, ran 2 commands, read 2 files
Intro and Goldenrod are correct again with the cache off. Committing this verified state (pacing lock, tile inset, geometry fast paths, retained 2D, keystream cache, gated DL cache):

Committed verified state
Committed as 40c6984; next I’m adding a verify mode to the display-list cache that decodes live on hits and logs the first mismatches against the recording.

Ran 3 commands
Frame 900 was never reached, so the slowdown is in the recording path itself, not replay. Checking the log:

Inspected verify run log tail
The game stalls as soon as the first 3D scene starts. Sampling where it spins:

Profiled the stalled verify build
The game is stuck spinning in OS_WaitIrq, waiting for an interrupt that never comes, probably a DMA or geometry-FIFO completion blocked by the cache. I’m reading the wait mask and caller from the stuck thread’s registers next.

Finished 2 background tasks, ran 3 commands, 4 failed
The stuck thread is just the normal main-loop V-blank wait; the interrupt storm in the samples suggests the game is starved, not deadlocked. Next I’ll run the intro cache build without verify to tell slow from hung.

Building cache-enabled variant via bash
