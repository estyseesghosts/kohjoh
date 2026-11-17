# LOTUS input scripts

Copy one to the card's `sdmc/3ds/kohjoh/script.txt`, then load/reset the
session. V-blank indexed from boot; deterministic in LOTUS (same audio hash
at the same frame across runs). After its last input event the script hands
the pad back to `lotus_input`.

- `r29_battle.script`: kohjoh-prof card save (Cherrygrove) -> Route 29 grass ->
  wild Rattata battle; command menu at frame ~7600. Touch FIGHT at (160,100),
  move 1 at (80,70) (3DS bottom-screen coordinates).
- `trainer_battle.script`: kohjoh-prof card save -> Continue -> field, then
  `battle: 3900 8` starts a trainer battle (Youngster Joey) at v-blank 3900.
  `battle: <v-blank> <trainer> [trainer2]` fires once, on the first field
  frame at or after that v-blank where the player could move; the same ID
  twice is a double battle (e.g. `10 10`, Twins Amy & Mimi); Falkner is 20.
  Implemented by game patch 2010 and `NitroPort_DebugBattle` (script.c).
  Two more numbers name the backdrop (decomp `BattleBg` and `Terrain`):
  `battle: 3900 8 0 9 5` is a cave, `battle: 3900 8 0 1 7` water (patch 2011).
