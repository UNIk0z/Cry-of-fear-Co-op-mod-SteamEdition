# v0.5.7 test run

Both players: close the game, run "Install.cmd" from the v0.5.7 folder, then
"Run Co-op Mod.cmd". The host sends the newest
cryoffear\storycoop_logs\rc4-*.log after the session.

1. **Skips.** Skip any scene with R (both players). Expect your own view back
   at once, no second "Press R to skip" for the same scene. Good test spots:
   Chapter 1 elevator gate, the balcony, the bus-stop map, the subway train.
2. **Doctor cage (s4_asylum1).** Skip the first part; the yes/no question must
   appear and wait for an answer. E/SPACE on the choice must not skip it.
3. **Map changes.** Walk through a few doors holding a weapon: no hand flailing.
4. **Train crash (Chapter 6).** After the crash both inventories are empty
   (three slots).
5. **Hanging train (s4_bridge).** One player climbs out first: the other must
   appear beside them before the train falls.
6. **Broken floors.** s4_down and s4_fsewer1: after the floor breaks the
   second player can drop down too.
7. **Sawer.** Let the Sawer grab a player: notice, then the map restarts (no
   crash).
8. **Wheelchair (ending 4).** Both players get a wheelchair, also if one
   loads later.
9. The skip text now sits above the health bars.
