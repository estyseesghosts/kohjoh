# Widescreen QA corpus (issue #21)

`qa.py` + `manifest.json`: versioned cases (edition, save fixture SHA-256,
input script, capture frames, log expectations), A/B modes that switch one
widescreen stage off through the script's `debug:` flags, and a checker that
diffs the 256-column DS centre and the 32-column margins separately against
a recorded baseline (`build/qa/baseline/`, never committed: game pixels).

| mode          | debug flag | what is off                                  |
|---------------|-----------:|----------------------------------------------|
| `full`        |          0 | nothing                                      |
| `no-wideart`  |     0x100  | WideArt bands                                |
| `no-layout`   |     0x200  | layout rows (2D layers keep the DS columns)  |
| `no-sidefill` |     0x400  | side columns painted black after the frame   |
| `centred`     |     0x700  | all three                                    |

Loop: `stage` -> run the session in LOTUS to each case frame and capture the
top screen into `build/qa/caps/<case>-<mode>/f<NNNNN>-top.bin` -> `check`
(first time: `record`). LOTUS results are emulator evidence only; O3DS
pacing comes from hardware runs of the same cases ([pace]/[gputime] lines).

## Coverage (2026-11-17)

| class | cases | status |
|---|---|---|
| intro / title | intro-wideart, title-wide | baselines: title-wide |
| battle | trainer, Elite Four, Rocket, Kimono Girl, wild R29 | captured in LOTUS; see worklog |
| field | field-route (Continue, walk east) | LOTUS HG: log passes (field, layout rows A 2), baseline recorded |
| applications | app-start-menu (X menu, first app) | staged; not yet captured |
| scripted events, stress | none yet | open: need input traces |
| melonDS reference | none | open: no melonDS automation here |
| O3DS pacing | none | hardware runs by the maintainer |
