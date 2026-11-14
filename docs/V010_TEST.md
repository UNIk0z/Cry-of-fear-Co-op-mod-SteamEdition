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

## v0.10.1 fixes

20. Solo: "Hardcore: no saves" on, use a tape recorder as the host: the screen says "Could not save: saving is off on this server", no save window opens and nothing is saved (`Save refused: saving is off on this server`).
21. Two players: player 2 picks another model (Player Model) and Red as his colour, joins, then closes his game with Task Manager and clicks REJOIN in his launcher while the host stays on the same map. Back in, he has his own model and colour at once (a ping shows his red) and the host log has a new `NET tuned player=2` line for that map.
22. Bosses (s4_apartmentsick, Sawer): two players; before the arena, player 2 lets a monster or a fall down him: no game over, the host can revive him (`BOSS fight starts` comes only when the Sawer's bar appears). In the fight, player 2 dies: game over, the map restarts at the fight (autosave), the bar is shown again (`BOSS fight goes on after the load`) and a death is a game over again.
23. During a boss fight, player 2 rejoins (or a third player joins): he sees the boss bar (`BOSS health bar shown to player=...`).
24. "Harder with more players" on, ending 1-3 strangle or ending 4: the bar empties at the end as in single player. "Hunted" on: the log says `HUNTED off on this map: a boss fight is here` on every boss map.

## v0.10.2 fixes

25. Two players, "Doors that change the map wait for everyone" off in the Create Server window (`sc_doorwait 0`), a door that changes the map (for example the apartment corridor of clip 1): player 2 stays far away, the host uses the door. No "Please wait until everyone is ready!": both fade out and arrive on the next map (`DOORS a co-op level-change door opens for any player`).
26. Carcass (s4_roofs2): two players; skip the scene with R while a head is in the air, or let a head hit a player after the fight: no crash (`TOUCH roofboss_head hit a player after its owner was gone` if it happened).
27. Two players at a tape recorder with cassette tapes on the floor: walk over and around the spot where the other player's tape lies. Nothing pushes you; E still takes your own tape.
28. A story teleport (for example s4_subway2st1 arriving from s4_subwaysick2, or the elevator scene): the partner arrives facing the same way as the player the story moved, not sideways.
29. Metro (s4_subway2st1): each player carries a fuse. Player 2 puts his into one socket: the host keeps his fuse (`KEYITEM inventoryitems/fuse.txt used by player=2: 1 more lock(s) for it on this map, the others keep theirs`) and puts it into the other socket; the door opens.
30. Drop a fuse in a doorway (or find one inside a door or cabinet), look at it and press E: you take the fuse, not the door (`USE player=N takes inventoryitems/fuse.txt from inside func_door...`).
31. s4_subway1st1, where the story switches the phone off (cof_phonedisable): the phone of player 2 dies too (`PHONE died for player=1: also for player=2`).
32. Player 2 with RebuiltSimon in cryoffear\addons joins the host: no crash after connecting, and the RebuiltSimon HUD works. In storycoop_server.cfg remove the // in front of `sc_noteleports` and set it to 1: after a scene the others stay where they were.
33. s4_hamn3 to s4_hamn2 (the way back from the college): both arrive and stay on s4_hamn2. Walking back into that exit still takes you to s4_hamn3 (`ARRIVAL player=N left the exit he arrived in`).

## v0.10.3 (docs/V0103_RELEASE.md)

34. Solo, s4_apartment1 (Chapter 1): every ammo box and syringe spot shows one item, not a square of four. The host log has `ITEMS N extra pickup copies of the map removed: one of each pickup for each player` on each map with copies.
35. Two players, same spots: each sees one item. Player 2 takes it: it is gone on his screen, the host still sees his own and takes it too.
36. Two players, "Doors that change the map wait for everyone" on (the default). The apartment corridor door to the next map: the host uses it while player 2 is a corridor away. The host gets "Please wait until everyone is ready!" and nothing happens. Player 2 comes to the door: the door works and both arrive on the next map.
37. A plain door that changes the map, for example s4_college2 to s4_college1 (doorch1), s4_hamn3 to the college (doortc1) or s4_subway1st1 to the tunnel (lockedd1): the same wait (`DOORS player=1 waits at the door to ...: 1 of 2 living player(s) there`). A locked one still says it is locked until the key is used.
38. Player 2 is downed far away: the host's door works without him. Alone (solo or the other player left): no door ever waits.
39. The ending door of s4_village3 (endingcheck): it waits for both players too, then the ending map loads.

## v0.10.4 (docs/V0104_RELEASE.md)

40. Two players, the host holds the Glock with spare ammo and presses Q: a 9mm box lands in front of him, his spare ammo drops by 15 (the clip stays), he sees "You dropped 15 rounds." and player 2 sees "<host> dropped ammo for you". Player 2 takes it with E and gets 15 rounds (`AMMO player=1 dropped 15 rounds (ammo_glock)`).
41. Q again within a second: nothing. With less than one box of spare ammo: "Not enough spare ammo: a box is 15." Dual wielding: "Hold one gun to drop its ammo." Alone: "Nobody to drop ammo for."
42. The shotgun, the M16 and the revolver each drop their own box (9, 20 and 5 rounds).
43. SETTINGS in "Cry of Fear Co-op.exe" shows a sixth key, "Drop ammo key" (Q); changing it and saving moves the drop to that key.
44. A player who kept his own storycoop_comfort.cfg from v0.10.3: Q still drops ammo after Install (`COMFORT drop ammo bound to Q` in his log), unless his config.cfg uses Q for something else.

## v0.10.5 (docs/V0105_RELEASE.md)

45. Main menu, Player Model: the first page shows Simon and his outfits as before. Done opens the page titled "Hands": its picture is an arm in a sleeve, not the player model. Next goes through Same as the model, Simon, Leatherhoff, ModDB, Hello Kitty, Camo, HLC, Awesome, Sick Simon, Twitcher (10), and the picture changes with each.
46. Solo: pick Camo hands, start Chapter 1. Your first-person sleeves are camouflage with every weapon (knife, phone, Glock). The host log has `MENU co-op main menu set up in memory` (not "menu off").
47. Pick Simon hands with a Camo player model: the sleeves are Simon's grey ones. Pick "Same as the model" with the Hello Kitty model: Hello Kitty sleeves.
48. Two players, player 2 picks Twitcher hands: his sleeves are Twitcher's on his screen; the host's sleeves stay what the host picked.
49. Two players, any locked door opened with a key item both players carry: after player 1 uses the key, player 2's copy of it is taken too (`KEYITEM ... used by player=1: taken from 1 other player(s)`). The metro fuse sockets still keep the second fuse until both sockets are filled.
50. An older choice of Afraid of Monsters hands (storycoop_hands.txt "simon_aom") shows as Awesome on the Hands page.
51. Hand skin: copy cryoffear\models\costumes\Camo\Camo_Sleeve.tga to the mod's `handskins\test\sleeve.tga` and click Run Co-op Mod: the start shows `Hand skin added: test (sleeve)`. On the Hands page the last entry is "Hands: test (your own skin)" with the arm in camouflage (gfx\vgui\sc_mbhc<n>.tga).
52. Solo with that skin: your first-person sleeves are camouflage with every weapon; the host log has `MODEL player=1 hand skin test (parts 1)`. Change it, run the mod again: `Hand skin added` again and the new picture.
53. Two players, player 2 has a skin the host does not have: player 2 sees his skin, the host sees his own hands. Player 2 changes to Twitcher hands: the log has `hand skin off` and his sleeves are Twitcher's.
54. A skin folder whose name has a space, "Red Jacket", is skipped with `Hand skin Red Jacket skipped`; a file that is not a 24-bit TGA is not listed.

## v0.10.6 (docs/V0106_RELEASE.md)

55. Install with a blocked file, without Defender: in a fresh extraction of the v0.10.6 ZIP rename `files\native\storycoop_rc4.dll` to `storycoop_rc4.dll.off` and click Install. The launcher says "Windows Defender blocked storycoop_rc4.dll (a false alarm). Allow it in Windows Security > Protection history, exclude the co-op folder, extract the ZIP again and click Install." and launcher.log lists the four steps; nothing new is in the game folder. Rename the file back and click Install: it installs.
56. Two players on s4_stad with the stad door to the subway open (the fuse placed). One of you goes through it: the host log has `CARRY level change to s4_subway1st1_e0`, then `MAPSTATE kept s4_stad` with no `LOADWAIT released` line between them, then on the subway `LOADWAIT s4_subway1st1_e0: waiting for 2 player(s)`. Nobody can move until both are in.
57. Same arrival: when the wait ends, both players appear on the subway side of the door together, nobody stays in the dark room (`TRANSIT player=N carried by teleportst11_2 with player=M`). Walk away from the door into the subway: you stay on the subway.
58. Use the subway's door back to the stad: both arrive on the stad side together (`TRANSIT ... teleportst11_1`), nobody in a dark room, no trip back to the subway unless someone uses the stad door again.
59. The tester's slot1 (or any v0.10.5 save made after the stad fuse was placed and the snowy trigger at the stadium entrance passed): load it on s4_stad. The host log has `MAPSTATE repaired 1trainbutton: starts at 1stop2`, and E at the fuse box gives the fuse. Leave the map and come back: no `repaired` line, the fuse box stays empty, no second fuse.
60. A new game: place the stad fuse, come back later in the snow, take the fuse at the fuse box, leave and return: the box stays empty. On s4_subway2st1 (the metro) the two take-back buttons are still at their sockets after leaving and coming back.
61. Other arrivals into a transit room, for example s4_hamn2 from s4_hamn, s4_park3, s4_buscity2: both players come out into the map together.
62. Enter s4_subway1st1 from the tunnel (s4_subtunnel1) while carrying a fuse: you keep it (no `Chapter 4 fuse(s) left behind`).

## v0.10.7 (docs/V0107_RELEASE.md)

63. Solo or two players: any arrival through a transit room (s4_stad <-> s4_subway1st1 first, then s4_hamn2 from s4_hamn, s4_park3, s4_buscity2). Both players come out side by side, not inside each other, and nobody is standing in the door or trigger that leads back. The host log has `TRANSIT player=N carried by ... , beside him`. If it says `no clear spot beside him`, note the map and where you stood.
64. Same arrival with the host option collision off (sc_collision 0): both come out on the same spot, as in v0.10.6, and can walk apart.
65. Two players walk into two different exits at the same moment (for example both doors of a room, or one through a door while the other steps into a level-change trigger). The game goes to one map; the host log has one `CARRY level change to <map>:` line and, if the second was asked for, `CARRY level change to <other> ignored`. On the new map both arrive at the right place.
66. Install with the adapter removed after Install, without Defender: install v0.10.7, close the launcher, delete `cryoffear\cl_dlls\storycoop_rc4.dll` by hand, then click Run Co-op Mod. The launcher shows the Defender HELP lines ("exclude the co-op folder and cl_dlls") instead of "not installed"; launcher.log has the four steps with both folders. Click Install: it puts the adapter back.
67. Same with Join (delete the adapter, Join Simon's Story): the same steps, the game does not start.
68. The update check (Check for updates) with the adapter deleted: it says "the co-op maps, but not the co-op adapter" and prints the steps.
69. A save made with v0.10.6 on s4_stad (fuse placed and taken back once): load it with v0.10.7. The host log has no `MAPSTATE group ... kept as served (incomplete record)` line, and the fuse box stays as it was left. If such a line appears on any map, send the log.
