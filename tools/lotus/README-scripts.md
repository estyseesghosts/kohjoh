# LOTUS input scripts

Copy one to the card's `sdmc/3ds/kohjoh/script.txt`, then load/reset the
session. V-blank indexed from boot; deterministic in LOTUS (same audio hash
at the same frame across runs). After its last input event the script hands
the pad back to `lotus_input`.

- `r29_battle.script`: kohjoh-prof card save (Cherrygrove) -> Route 29 grass ->
  wild Rattata battle; command menu at frame ~7600. Touch FIGHT at (160,100),
  move 1 at (80,70) (3DS bottom-screen coordinates).
