# Cry of Fear co-op v0.7.2 (2026-10-31)

v0.7.1 plus these changes. Maps byte-identical; adapter and launcher only. Native tests pass. Nothing here has run in the game.

| Report | Cause | Change |
|---|---|---|
| Tester: "if player 2 uses a ladder there is a softlock potential" | A busy ladder hands the second climber a copy, but the game repeats the name lookup after the copy. A copy in a reused low entity slot sits before the original, so that lookup reached the busy original and took it from its climber, who stayed frozen. | The lookup after a copy ends there. Details: docs/V072_LADDERS.md. |
| Same audit | Any player's camera counted as a cutscene at spawn, so whoever joined or respawned during a climb got a permanent black screen ("Please wait until cutscene has finished."). | A climb no longer counts as a cutscene. |
| Same audit | A scene could end a climb from outside: the player stayed frozen, kept godmode, the old camera stayed in the world, and the ladder drove the scene's camera. | The climb stops, its camera goes, and the player gets control back when the scene is over. |
| Same audit | Leftover ladder state: the next climb ended at once or its camera did not rise, a scene's end unfroze a climber, and a climber who left kept a model on the ladder. | Each climb starts clean, a climber stays frozen, and a departure removes the model. |
| Xash3D co-op changelog: respawn during a revive | The revive's get-up mode survived the respawn: no firing, no move keys, unseen by monsters. A reviver killed during CPR kept the same trap and lay on the floor. | Cleared after the respawn. Details: docs/V072_REVIVE_WEAPONS.md. |
| Xash3D co-op changelog: crash after a dropped weapon | Queued and last weapons were guarded since v0.5.7. The stored night vision and action weapons were not, so they could point at a weapon on the floor or one already freed. Nobody has seen this crash here. | Both are cleared when the player no longer owns that weapon. |
| Same changelog | A get-up with the queued weapon gone emptied the hand. | The weapon in hand comes back. |

New in the player ZIP: `Cry of Fear Co-op.exe`, a themed launcher with Install, Uninstall, Run and an update check. It has only been tested with fake scripts: a real install, uninstall and update with it have not been run. The `.cmd` files are still there and do the same steps.
