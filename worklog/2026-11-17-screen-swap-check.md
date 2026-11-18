# Screen-swap check (#19), LOTUS, HG, battle-trainer script

Switch frames come from the `[screen]` log times (x60): title 2047,
mainmenu 2615, field 3420, battle 4059. Top screens were captured on both
sides of each switch:

| frame | screen | top screen |
|---|---|---|
| 2040 | intro, before title | intro art, 910 colours |
| 2055 | title entry | flat white (title flash), all 400 columns |
| 2610 / 2625 | -> mainmenu | flat black (fade), sha 19066b84 |
| 3415 / 3430 | -> field | flat black, same sha |
| 4055 / 4070 | -> battle | flat black, same sha |
| 4670 | battle | battle scene, wide terrain, pillarboxed box |

Every switch frame is one colour across the full 400 px: no previous
screen's WideArt bands, layout rows or side fill survive into the next
screen, and the margins match the centre. Bottom screen is likewise
unchanged (sha 4d856b7a) through the fades. Emulator evidence only; the
O3DS run is the maintainer's.
