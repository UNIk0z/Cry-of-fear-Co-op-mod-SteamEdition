# v0.7.2 test run

Close Cry of Fear on both PCs. Every player runs "Install.cmd" or the Install button of "Cry of Fear Co-op.exe", then runs the game the same way. Host log lines start with `[STORYCOOP_RC4]`.

## Launcher (first real run)

1. "Cry of Fear Co-op.exe": Install over v0.7.1, then Run. The co-op menu shows v0.7.2.
2. Uninstall, then Install again. Saves are kept.
3. The update check says you are up to date.

## Ladders

4. Solo: climb up, climb down, get on at the top and go back off, then climb from the bottom. The camera rises.
5. P1 climbs and P2 presses E on the same ladder: `LADDER ... copy 1 made`, both climb, both reach the end. Swap host and joiner.
6. One climbs up while the other climbs down the same ladder.
7. P1 stays on a ladder while P2 joins, and again while P2 respawns: P2 plays on (`not held by the cutscene wait`).
8. P1 stays on a ladder while P2 starts a scene: `ended from outside`, then `has controls back` after the scene. P1 can climb again and no climb model stays behind.
9. P2 disconnects on a ladder: the model goes and the ladder works for P1.
10. P1 stays on a ladder while P2 walks into a level change: both arrive and can move.

## Revive and weapons

11. P1 dies and P2 revives. In the second or two of the get-up, P1 presses jump, with the respawn delay already over. P1 respawns, moves, fires, and monsters attack (`REVIVE player=1 respawned with revive mode 3 left over`).
12. P1 is reviving P2 and a monster kills P1. P1 respawns on the timer, stands and fires.
13. Die with night vision on, respawn, and switch night vision off. No crash.
14. Night vision on, drop or lose the stored weapon through the inventory, switch night vision off. No crash, empty hands.
15. Queue a switch, drop that weapon, get knocked down and get up. The weapon in hand comes back.
