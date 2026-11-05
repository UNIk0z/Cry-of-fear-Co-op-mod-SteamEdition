# v0.9 test run

Nothing in v0.9 has run in the game yet. Close Cry of Fear, open "Cry of Fear Co-op.exe", click Install, then Run Co-op Mod. Host log lines start with `[STORYCOOP_RC4]` (cryoffear\storycoop_logs, newest rc4-*.log).

## Solo first (host alone)

1. Launcher: SETTINGS opens. Set field of view to 100, head bob Off, leave K / H / T. Save. Start the mod. Log: `COMFORT storycoop_comfort.cfg run`. The view is wider and does not bob when you walk.
2. Host Simon's Story: the Create Server window shows "Revive time (seconds)" 5 and "Auto respawn while down (seconds)" 384 under Unlock extras. Start Chapter 1.
3. On the first story map: `SERVER storycoop_server.cfg run on s4_...`. About 8 s in, the game's hint at the top says "Stuck? Press K to get free (co-op)." (`UNSTUCK key prompt shown to player=1`).
4. The co-op list at the right (your name - Health - Ping) shows. Dial on the phone: "(phone)" after your name; put it away: the tag goes. (The bag and a note cover the screen, so their tags are for the partner: test 16.)
5. Start a cutscene: the list is gone while the camera holds you, and comes back after.
6. Press K in open space: "Moved to a clear spot nearby." and the objective shows at the top (`UNSTUCK ... to a clear spot nearby`, `OBJECTIVE reminder`). Press K again at once: "Unstuck again in 10 s." Press K in a cutscene: "Unstuck is off during a scene." On a ladder: "Unstuck is off on a ladder."
7. Walk into a corner or prop until you are stuck, press K: you are put free (`(inside something): out of the wall`).
8. Press H: the whole HUD and the co-op texts go; press H again: they come back.
9. Launcher SPRAY: choose a picture, SAVE AS MY SPRAY. Start the mod, press T at a wall: your picture is painted there.
10. Change the game's subtitle language in the main menu, start a game: subtitles are in that language, and stay so on the next map (no `LANGUAGE ... set back` unless the game changed it while the map started).

## Two players

11. Joiner sets another subtitle language than the host before joining: in a cutscene each sees subtitles in his own language (`LANGUAGE player=2 reads subtitles in N`). B shows the objective in his language too.
12. Player 2 goes down: player 1 sees "<name> is down" in white (`DOWN player=2 is down; 1 other player(s) told`). Revive him: as he gets up you hear his groan (`REVIVE player=2 gets up: ...`).
13. Let a downed player wait for the auto respawn (set "Auto respawn" low in the Create Server window, e.g. 20): he respawns and sees the objective.
14. A friend joins a map more than 15 s after it started: 3 s after his HUD is ready he sees the objective.
15. Player 2 in another room presses K: he lands next to player 1 ("Moved beside your partner.").
16. While player 2 reads a note, has his bag open, dials the phone or types a keypad code, player 1's list shows "(reading)", "(bag)", "(phone)" or "(puzzle)" after his name, and nothing once he is done.
17. Rejoin: the joiner's game crashes or is closed with Task Manager while joined (host still running) with the launcher open: it asks "The game closed. Rejoin <host>?". REJOIN starts the game and joins. A normal quit asks nothing.
18. Edit cryoffear\storycoop_server.cfg on the host: remove the // before `sv_coopcprtime 5` and change it to 2. A revive then takes 2 s.
