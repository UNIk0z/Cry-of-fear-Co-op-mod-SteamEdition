# v0.10 test run

Nothing in v0.10 has run in the game yet. Close Cry of Fear, open "Cry of Fear Co-op.exe", click Install, then Run Co-op Mod. Host log lines start with `[STORYCOOP_RC4]` (cryoffear\storycoop_logs, newest rc4-*.log). Every new host option starts off; test 1 checks that an untouched game plays like v0.9.

## Solo first (host alone)

1. Start Chapter 1 with every v0.10 option left off. Play a few minutes: no clock, no glow, no Simon, saving works. The sprays from the v0.9 Stability Fix still work (T at a wall, the hiss plays).
2. Launcher SETTINGS: the window shows Ping key G and Scare key J, Your colour, Black and white, Light with no torch (Game / Warm / Cold / Brighter) and Wider. Pick Green, Black and white, Warm, Wider. Save, start the mod: the picture is black and white on every map and stays so after a cutscene; the faint light around you (no torch out) is warmer and wider. Turn Black and white off again for the next tests.
3. Press G looking at a wall: a glow there for about 3 s and "<your name>: here (N m)" in green (`PING player=1 at ...`). Press G again at once: nothing (2 s wait). Press J: "Nobody to scare." (alone).
4. In the console: `dodgetoggle` (the game says dodge is off). Go through a level change: dodge stays off (`DODGE player=1 kept off`).
5. Create Server window, second page: "Run timer" = Chapter times. Start Chapter 1: at the chapter title card the log says `TIMER <chapter title> at 0:00:..`; 7 s after the card the screen shows "<chapter>: run 0:00:.." in grey. Set it to "Always on screen": a clock at the top right that is gone during cutscenes. Save with the recorder, quit, load that save: the clock goes on from the saved time, not 0.
6. "Harder with more players" on, alone: nothing changes (no `SCALE` line).
7. "Hardcore: no saves" on: the tape recorder says "saving is off on this server" and nothing is saved.
8. "Hardcore: no respawn while down" on: let a monster down you. No respawn after 60 s or after the auto respawn time; alone the game's own game over restarts the map. Fall off a ledge into a pit: the fall respawn (2 s) still works.
9. "Morphine trip time" 10: use morphine; the trip ends after about 10 s.
10. "No hint pop-ups" on: no "Press K" prompt, no objective pop-up after K, and the game's own hints are gone.
11. "Hunted" on, start Chapter 1 and play past the first maps (not the opening rooms): about 2.5 min into a story map, "Something is following you." (`HUNTED Book Simon appears at ...`). He walks toward you with the sledgehammer. Kill him (`HUNTED Book Simon is down`) or outrun him for 3 min (`gone (he never caught up)`). Start a cutscene while he is there: he is gone (`gone (a scene started)`). If the log says `HUNTED off on this map: models/booksimon.mdl is missing`, tell me which map. Leave a map while he is following you and come back to it: he is not there any more (nor after loading a tape save made while he was there).

## Two players

12. Player 2's SETTINGS colour Red. "Outline far teammates" on: walk apart more than about 10 m; each sees the other glowing (host: red partner; partner: the host's green). Close up the glow goes; in a cutscene it is gone; a downed player has none.
13. Player 2 presses G: both see the glow and "<name>: here (N m)" in red, each with his own distance. Player 2 presses J near the host: the host hears a laugh, player 2 reads "You scared <host>." Again at once: "Not yet." (`SCARE player=2 scared player=1`).
14. "Harder with more players" on with 2 players: `SCALE monster health x1.35 for 2 players`. A monster takes more hits than before; a boss does not. Go to the next map and back: the monsters left there are not tougher than before (no second `SCALE` for them).
15. Player 2 joins a running game for the first time while the host carries weapons (more than 15 s after the host is in): player 2 gets those weapons and, if far, is put beside the host (`CATCHUP player=2 joined late: N of the host's weapon(s) given`). He leaves and rejoins: nothing is given again. Two players who start a chapter together get no `CATCHUP` line.
16. Player 2 turns Black and white on: only his picture is grey; the host's stays in colour.
17. "No respawn while down" on: player 2 goes down and stays down until the host revives him; with both down the game over restarts the map. The host goes down, then player 2 leaves the game: the host can respawn again with Space within a minute.
18. Vote kick: player 2 types `votekick <host name>` in the console; the host is asked to vote (F1 yes, F2 no). Vote no.
19. Hunted with two players: Simon appears far from both; when he is killed he comes back about 4 min later.
