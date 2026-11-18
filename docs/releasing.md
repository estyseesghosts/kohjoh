# Releasing

How to cut a tagged release (for example `v0.0.3`). Read this before you
touch a version number, tag or the release page.

## Where the version lives

There is **no single version constant**. The version is:

1. **The git tag** (`vMAJOR.MINOR.PATCH`). This is the source of truth and what
   GitHub shows.
2. **The CIA version**: `-major 0 -minor 0 -micro N` on the `makerom` line in
   [`tools/forwarder/Makefile`](../tools/forwarder/Makefile). Keep `micro`
   equal to the tag's patch number. This is the only version field in the
   tree. The `.3dsx` files carry no version, only a commit id in the SMDH
   title (see `smdh_names` in `tools/game/hgss_game.py`).

So "bump the version" = edit that one Makefile line, commit, then tag. Edit it
without changing line endings (the file has mixed CRLF/LF; `sed -i` rewrote a
CRLF line once). Use Python with `newline=''`.

## Checklist

1. **Commit everything.** `git status` must show nothing but throwaway files
   (the `kohjoh*.zip` snapshots in the root are not tracked on purpose). Never
   add a `Co-Authored-By` trailer; identity is `terezi <jules@foxtails.me>`.
2. **Bump** `-micro` in `tools/forwarder/Makefile`; commit.
3. **Clean-build all four variants** (see below) and check the logs.
4. **Build the CIAs** (see below).
5. **Push** `main`, then **tag** and push the tag:
   `git tag -a vX.Y.Z -m vX.Y.Z && git push origin main vX.Y.Z`.
6. **Create the GitHub release** with `gh release create vX.Y.Z` and the
   assets below. Release notes follow the earlier releases: lowercase,
   short, a "what you need" and "setup" section (`gh release view v0.0.2`).

## The four builds

Names used in conversation: **rc** = the normal, uninstrumented build
(`gold-normal`, `silver-normal`); **dbg** = the profiling build with
`HGSS_PROFILE=on` (`gold-prof`, `silver-prof`).

Clean means delete the old output first, so no stale object is reused:

```sh
rm -rf build/kj-gold-normal build/kj-gold-prof build/kj-silver-normal build/kj-silver-prof build/kj-*-build.log
C:/devkitPro/msys2/usr/bin/bash.exe -lc '
  export PATH="/c/Program Files/Git/cmd:$PATH"
  cd /c/Users/<you>/workdir/kohjoh
  python tools/game/check_patches.py && python tools/game/build_variants.py
' > build/release-X.Y.Z-build.log 2>&1
```

It takes roughly 20-30 minutes; run it in the background. A variant counts
only if its log ends with `packaged ... .3dsx` and has no `error:` or
`vendored library build failed` line (details in
[building-windows.md](building-windows.md)). Outputs are
`build/kj-<variant>/pokeheartgold.3dsx` and `pokesoulsilver.3dsx`.

## Release assets

Follow the v0.0.2 asset names exactly, because the README's install steps and
the CIA forwarder look for them:

| Asset | Source |
| --- | --- |
| `kohjoh-gold.3dsx` | `build/kj-gold-normal/pokeheartgold.3dsx` (rc) |
| `kohjoh-silver.3dsx` | `build/kj-silver-normal/pokesoulsilver.3dsx` (rc) |
| `kohjoh-gold-dbg.3dsx`, `kohjoh-silver-dbg.3dsx` | the `-prof` builds (dbg) |
| `kohjoh-gold.cia`, `kohjoh-silver.cia` | forwarder CIAs, rebuilt with the new `-micro` |
| `kohjoh-asset-tools.zip` | `tools/rompack/{prepare.bat,prepare.py,verify_rom.py,extract_rom.py,pack.py}`, flat, no folder |
| `SHA256SUMS` | `sha256sum -b` of every file above, binary-mode format (`<hash> *<name>`) |

The CIA is only a forwarder that launches `/3ds/kohjoh-<edition>.3dsx`, so it
rarely changes except for its version. Building one needs `makerom` and
`bannertool` (not installed by default). Without `bannertool`, reuse the
existing `build/{gold,silver}.bnr`, `build/{gold,silver}.smdh` and
`build/forwarder-{gold,silver}.elf` and run the `makerom` line from the
Makefile directly (title ids `0xD7101` gold / `0xD7102` silver; the Makefile
`sed`s them into `forwarder.rsf`).

## CI

`.github/workflows/build.yml` runs on every push and pull request: it
bootstraps the decomp and builds both editions (normal only) in the
`devkitpro/devkitarm` container, then uploads the `.3dsx` files as a workflow
artifact. It does **not** build the dbg variants or the CIAs, does not read the
version number, and does not create releases or tags. Releases are made by hand
with `gh`. After pushing, check that the run for the release commit is green
(`gh run list --limit 3`) before announcing the release.
