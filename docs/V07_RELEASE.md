# Cry of Fear co-op v0.7 (2026-10-30)

v0.6.3 (the Playtest 3 fixes, Unlock extras, Doctor's shot for everyone, the
save window with five slots) plus the player model menu and one save fix.
Maps byte-identical; adapter, menu and installer only. Native tests pass;
**not run in the game.**

| Item | Change |
|---|---|
| Player Model (V07_TODO 2) | The main menu reads Host, Join, Player Model, Options, Quit. Player Model opens a box listing the models in `cryoffear/models/player`; type one and click Log on. The choice is kept (`storycoop_model.txt`) and sent with every start (`setinfo sc_model`). |
| Custom models (V07_TODO 3) | `.mdl` files in the mod's `playermodels` folder are copied to `models/player/<name>/<name>.mdl` by "Run Co-op Mod.cmd" (never replacing one already there). |
| Seeing the model | The host shows a player with their chosen model when the host has that model too (`MODEL player=N now <name>`); otherwise Simon and a notice to that player. The host's models are offered for download to joiners. |
| Save window (re-review) | The native save sequence makes the host "not a target" before the co-op save ran one frame later, so the save would have been refused ("player controls frozen"). The co-op save now runs on the double-click itself. |

Still open (need a playtest log or clip): elevator scene, crawler, invincible
enemies in the college, crouching on a save station, a door opened from the
other side; player model part 2 (outfit models built at install).
