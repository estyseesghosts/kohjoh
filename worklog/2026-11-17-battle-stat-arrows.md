# Battle stat-change arrows (OBJ window)

Report (chatlog14): stat up/down arrows were masked everywhere except the Pokemon.

The effect (`BattleAnimObjWindowBg_Start`, src/battle/battle_anim_effects.c) turns
on only the OBJ window (DISPCNT 0x119F18). It sets a mode-2 copy of the battler as
the window shape and WINOUT 0x363B: outside = BG0/1/3, OBJ, effects; inside the OBJ
window = BG1, BG2 (arrows), OBJ, effects. BG0 (the 3D battler) is off inside.

g2d had no OBJ window, so the whole screen was "outside". Two fixes:

1. g2d.c: mode-2 OBJs stamp stencil region 3 before the WIN0/WIN1 rectangles
   (WIN0 > WIN1 > OBJ window priority). Region 3 uses WINOUT bits 8-13. This is the
   stencil mask approach from coffeepot ctr/engineering/PICA200/rendering-2d.md.
2. g3d.c: the 3D layer now tests the window stencil too. BG0 is gated per region
   ("all", "only r", "all but r"). Before this, the 3D battler drew over the arrows.

Verified in LOTUS (kj-prof, Route 29, Cyndaquil Leer vs Hoothoot, frame 15246):
the arrows scroll inside Hoothoot's silhouette. Windows-off scenes are unchanged.

LOTUS notes: button bits use Citra order (Up 0x10, Down 0x20, Left 0x40,
Right 0x80, Start 0x400). Any sdmc script.txt owns input, even one with no input
lines. The replay is deterministic: the same inputs give the same audio hashes.
