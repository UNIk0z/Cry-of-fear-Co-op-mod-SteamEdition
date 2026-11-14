# Cry of Fear co-op v0.10.6 (2026-11-13): playtest fixes

v0.10.5 plus three reports from a two-player playtest of v0.10.5 (2026-11-10): the install stopped on a Windows Defender "virus" error, "random teleports" and a "black box" between s4_stad and s4_subway1st1, and the stad fuse could not be taken. Evidence: the tester's launcher.log, the host log rc4-20261110-173627.log and his slot1 save (copies outside the repository; not committed). Maps are byte-identical to v0.10.5. **Nothing here has run in the game.**

| Report | Cause | Change |
|---|---|---|
| Install stopped: "the file contains a virus or potentially unwanted software" | Windows Defender refused a read of a package file (a false alarm on the unsigned co-op adapter). The installer printed the raw .NET error in the PC's language, without the file name. | The installer names the blocked file and stops before writing anything (exit code 3); setup and the launcher show the steps; the READ ME explains them. Defender itself cannot be fixed in code. |
| Thrown back and forth between s4_stad and s4_subway1st1, a black box | (1) The load wait was let go on the old map, so the subway arrival was not held. (2) The transit room's trigger_multiple carries one player and then sleeps 2 s; the others stay in the dark room facing the trigger that leads back. The subway-to-stad trips were the co-op door beside the arrival point, used by a player. | (1) The wait starts counting on the new map only. (2) Everyone standing in such a trigger goes through its teleport with the first player. |
| The stad fuse cannot be taken | The take-back button rides on a train (movewith). v0.3-v0.10.5 never saved movewith groups, so on every visit the train was back at its first corner while the one-shot trigger that moves it stayed used. | Movewith groups are saved and restored whole. A v0.10.5 save gets the train put where its used trigger left it, so the tester's slot1 can go on. |

## 1. Windows Defender stopped the install

launcher.log: "No co-op version installed yet. Installing v0.10.5." and "Already current: 236 file(s)", then `install-story-package.ps1 : Exception calling "ReadAllBytes" with "1" argument(s)` with the Swedish text for ERROR_VIRUS_INFECTED, then setup-coop.ps1:174 "The package installer stopped". The record names no file: `[IO.File]::ReadAllBytes` does not put the path in that message.

Which file: the first binary of the package the installer reads is the adapter, `files\native\storycoop_rc4.dll` (v0.10.5 install-story-package.ps1 line 214; the reads before it are the JSON plans and the game's own maps). The rest of the log points the same way: 236 co-op files were already in place and current, but setup found no co-op adapter in `cl_dlls`, so an earlier install's adapter had most likely been taken away by the antivirus too. Defender has flagged this DLL before as Trojan:Win32/Bearfoos.A!ml, a machine-learning guess (docs/V0102_STATUS.md). Not proven: the log does not name the file.

Changes (scripts only; the guidance is plain ASCII so it survives the console code page):
- install-story-package.ps1: every file read goes through `Read-CoopBytes` (the "read guard" block at the top). A read that fails with ERROR_VIRUS_INFECTED (0x800700E1) or ERROR_VIRUS_DELETED (0x800700E2) anywhere in the exception chain, or a package file that is missing, is remembered; the script then prints `COOP-BLOCKED <path>` and a short explanation and exits with code 3 instead of the raw error record. A block swallowed by one of the installer's own try/catch blocks still ends this way. All package files are read before anything is written, so nothing is installed then. (The brief's "0x800700EA" is ERROR_MORE_DATA, not an antivirus code.)
- setup-coop.ps1: exit code 3 with the marker prints the steps (find-cof.ps1 `Write-CofBlocked`) and stops with a one-line ASCII message; its own hash reads (the installed adapter in `cl_dlls`) do the same.
- host-story-coop.ps1 (Run Co-op Mod): the hash read of the installed adapter does the same.
- Launcher (CoopLauncher.cs `FirstError`): when the scripts printed `HELP:` lines, those are shown instead of the first error record.
- READ ME.txt: "IF WINDOWS DEFENDER BLOCKS A FILE" with the steps: Windows Security > Protection history > Allow on device (or Restore), add the extracted "Cry of Fear Co-op" folder as an exclusion, extract the ZIP again, click Install again. It says false alarm; it does not say anything is fixed.
- Linux (coop-linux.py) has no Defender and is unchanged. The add-on warning (MadSimon.dll in `addons`) is unchanged: setup warns and goes on.

The lasting fix is outside the code: Katz submits the adapter DLL and the launcher exe as a false positive at https://www.microsoft.com/wdsi/filesubmission, or the files get a code-signing certificate.

Test: tests/v0106.test.mjs runs the read guard in Windows PowerShell with a fake read that throws the tester's IOException (HResult 0x800700E1), a block swallowed by a try/catch, a missing package file, a missing game file (stays a normal error) and a normal read; and find-cof.ps1's steps and HELP lines.

## 2. s4_stad <-> s4_subway1st1: what happened

The two maps share one room at the landmark `st11`, at the same coordinates, with a dark transit room below it:

| | s4_stad_e1 | s4_subway1st1_e0 |
|---|---|---|
| Co-op spawns | -428/-464, -529/-493, -491, facing west | -552/-516, -529/-493, -491, facing east |
| Trigger the spawns stand in | trigger_multiple *23, wait 2 -> cof_teleport teleportst11_1 -> -488 -150 -344 (beside the stad door and the fuse box) | trigger_multiple *79, wait 2 -> cof_teleport teleportst11_2 -> -410 -152 -344 |
| Way back, 24 units in front of spawns 2 and 4 | trigger_once *22 -> sc_level_288 -> s4_subway1st1_e0 | trigger_once *80 -> sc_level_368 -> s4_stad_e1 |
| Door to the other map | inter_door `shitdoor` (locked by the fuse box) -> st11_exit1, inside *22 | inter_door_coop_changelevel *1 -> s4_stad_e1, its zone sc_exit_5_ready covers the arrival point |

The host log, step by step:

1. s4_stad_e2 to the subway (log lines 134-137): `CARRY level change to s4_subway1st1_e0`, `DOORHIDE player=2 visible again after a door` (player 2 had used `shitdoor`), `LOADWAIT released (everyone is in), 0 player(s) were held`, and only then `MAPSTATE kept s4_stad`. The engine's ChangeLevel only queues a "changelevel" command (decomp pr_cmds.c PF_changelevel_I) that runs at the next frame's Cbuf_Execute (host.c _Host_Frame). A trigger touched from a player's move runs in SV_ReadPackets, and SV_Physics then calls StartFrame in the same frame (sv_main.c SV_Frame, sv_phys.c SV_Physics). `load_wait_frame` (coop-runtime.hpp) ran there, on the old map, where everyone was in, and let the wait go. So s4_subway1st1_e0 has no `LOADWAIT ... waiting` line: the host was not held, was carried out of the dark room at once, and stood alone on the subway side while player 2 was still loading.
2. Subway back to the stad, twice (lines 171-172 and 258-259): `VISIBLE player=1 to player=2: ... still not sent (effects 80)`, then `CARRY level change to s4_stad_e1`, with no `LOADWAIT released` line after it. EF_NODRAW (0x80) on a player in multiplayer comes only from b_GoingThroughADoor (decomp player.cpp 5629), which only doors set (CoF_InterDoor.cpp 336 for its user, CoF_InterDoorCoop.cpp 932 for every player). No StartFrame between the CARRY and the change means the change was asked for by an entity think, not a player's move. Both fit the co-op door *1: CInterDoorCoopChangeLevel::Use fades and freezes everyone and hides them, and its ChangeLevelThink calls ChangeLevel 2 s later (CoF_InterDoorCoop.cpp 879-952), to its message s4_stad_e1. So trigger *80 did not fire, and nothing restored from the save did it: *80 and sc_level_368 are kept as served (they lead out), and *79 was restored solid, with its touch and no pending wait (decoded from slot1.cofmap). A player pressed E at that door; the log does not say who or why. The door stands right behind where *79 puts arrivals, inside its own "everyone is ready" zone, the subway side of the room looks exactly like the stad side, and the fuse box the players could not use (report 3) is on the other side of that door. Single player has the same door in the same place.
3. Arriving on s4_stad_e1 (lines 185-203): here the wait worked (`waiting for 2 player(s)`, `1 player(s) were held`), so both players were let go together inside *23. CTriggerMultiple fires for the first player that touches it and then ignores every touch for its "wait", 2 s (decomp triggers.cpp 2866-2900), and CCofTeleport::Use moves its activator only (effects.cpp 7131). The second player stayed in the dark room, facing *22, 24 units away. `ARRIVAL player=2 left the exit he arrived in` (line 219) proves he touched an exit in his first seconds; *22 is the only one there. The v0.10.2 guard kept it shut while he stood in it: he was in a dark box. Later he went through `shitdoor` into *22; the guard's record let that touch through ("left ... works for him again") and the subway change followed at once (line 220, `DOORHIDE player=2`: he was the door's user).

The coordinator's suspects, checked: (a) the load wait let go on the old map: confirmed, above. (b1) a late joiner standing in the spawn box: his grace is his own, 3 s after his inventory is back; with (a) fixed he is held like everyone else. (b2) records keyed by edict surviving a level change: `exitsArrivedIn`, `exitTriggers` and `arrivalGrace` are cleared by `coop_map_changed` at ServerDeactivate; none survive. (b3) a re-armed *80 or a half-run sc_level_368 chain in the restored state: no, both are kept as served. (b4) *79 restored inert: no. (c) the host's EF_NODRAW: the co-op door's fade, the same event as the bounce, not a door-hide flag left from the stad.

Changes (coop-runtime.hpp, adapter.cpp):
- `load_wait_level_change` only marks the wait pending; `coop_map_changed` (ServerDeactivate of the old map) arms it, so it counts and lets go on the new map only. A change the engine never runs leaves nothing armed on the current map.
- `transit_touch` (called from `dispatch_touch` after the game's own touch): when a trigger_multiple that does not lead out carries its toucher away through a plain cof_teleport, every other ready player standing in that trigger goes through the same teleport (`TRANSIT player=N carried by ... with player=M`). The map's teleport behind a trigger_multiple is otherwise still each player's own passage, as before.

Not changed: the co-op door at the arrival point still works for whoever presses E once everyone stands in its zone, as the game and v0.10.3 (sc_doorwait) intend.

Test: test-qol2.cpp, v0.10.6 block: the CARRY, then one more old-map frame with both players in (v0.10.5: `LOADWAIT released`), then the new map holding the host until player 2 is in; both players in subway *79, the first touch carries both (v0.10.5: player 2 stays at -516); a player outside the trigger stays.

Audit (scripts/audit-v0106.mjs over the extracted campaign maps, 390 maps):
- Co-op spawns inside a trigger that leads out (the v0.10.2 guard's case): s4_avlopp_e0 (*47), s4_buscity_e1 (*40), s4_city_e2 (*46), s4_park2_e0 (*68), s4_stad_e2 (*18), s4_trainride2 (*195), s4_trainride_e1 (*204), s4_trainride_e2 (*222); 13 spawns.
- Spawns in a transit trigger_multiple with wait 1 or 2 (the case fixed above): s4_apartment3sick_e4, s4_asylum1_e2, s4_asylum1dark_e1, s4_buscity2_e0/e1/e2, s4_college1_e1, s4_college1dark_e1/e2, s4_college2_e0/e1, s4_forest7_e0/e1/e2, s4_hamn2_e0/e1, s4_hamn3_e1/e2/e3, s4_hamn_e0/e1, s4_park3_e0/e1, s4_stad_e0/e1/e2, s4_subway1st1_e0, s4_subway2st1_e3, s4_subway2st2_e0, s4_subway2st3_e0/e1, s4_subwaysick1_e0/e1, s4_subwaysick2_e0, s4_trainride_e0, s4_trainscene_e1 (36 triggers). 78 more transit triggers on 47 map families use the default wait 0.2.
- Spawns within 64 units of a trigger that leads out, with no transit trigger: 68 (map, trigger) pairs, the full list in the audit output. These are as served; the v0.10.2 guard covers the first seconds.

## 3. The stad fuse cannot be taken

Map (s4_stad_e1/e2): the fuse is placed with func_button `elbutton1` (*26, lockedby fuse) -> multi_manager `multiunlo` (unlocks `shitdoor`). It is taken back with an unnamed func_button *30, `"movewith" "1trainbutton"`, `"wait" "-1"`, target `multipoff2` (gives `inventoryitems/fuse.txt`, locks `shitdoor` again, hides the fuse model). `1trainbutton` is a func_train (*31) at path_corner `1stop1` (-645 60 -300); trigger_once *32 (master `mastersnow`) sends it to `1stop2` (-491 -113 -300), at the fuse box. The button is only at the fuse box after that one-shot trigger.

Save (slot1.cofmap, decoded with the adapter's own `state_decode` in a throwaway harness; 25 maps): s4_stad (s4_stad_e2) has 355 records and 22 removed; `trigger_once||*32|0 0 0#0` is removed (used), and there is no record of `1trainbutton` or of *30. The save's players hold no fuse.

Cause (state-runtime.hpp, v0.10.5 `state_keep`): every entity with a "movewith" key and every movewith parent was kept as served ("their link lists are rebuilt only by Spawn"), so it was never saved, never restored and never counted in the restore line (`kept-as-served` counts only keys that are in the file). On every load or revisit the train was back at 1stop1 while *32 stayed used: no click could take the fuse. A reinstall cannot help; it is in the save. The v0.5.7 notes already said "the take-fuse buttons ride on movewith parents".

Changes (state-runtime.hpp):
- Movewith groups are saved and restored whole (`state_group`). Spirit saves the links (m_pMoveWith, m_pChildMoveWith, m_pSiblingMoveWith) and m_activated (decomp cbase.cpp 831-839). The restore runs before the game's Activate, with the whole group in the save table, so the links point at the fresh entities and Activate skips InitMoveWith (cbase.cpp 571), as in a single-player load. A train in the middle of a move is restored with its think times rebased, as single player does. A group is kept as served as a whole when one of its members leads out (keeps every exit working).
- Saves from v0.10.5 and older (`state_repair_trains`): a func_train with no record whose trigger chain was used on the kept visit (a removed entity's target, directly or through trigger_relay / multi_manager, up to three steps) gets as its target the first later path_corner that waits for a retrigger (flag 1) or ends the path. The train's own PostSpawn in Activate (plats.cpp 1100) then puts it, and its movewith children (movewith.cpp UTIL_AssignOrigin), there, as the map does with every train's first corner. A path that loops is left alone. Log: `MAPSTATE repaired 1trainbutton: starts at 1stop2 ...` and `trains-repaired=N` in the restore line. This was chosen over re-arming *32: *32 has the master `mastersnow` (a multisource on the global state `statesnow`) and sits at the stadium entrance across the map, so re-arming depends on that global and on the players walking back, while moving the train is exactly what the trigger did.
- Limits of the repair: a train sent further by later triggers is only put at its first waiting corner; a train started by a monster's death trigger (s4_sewer3's key button, monster_sewerboss TriggerTarget) is not followed (that field is the monster's private data); in an old save in which the button was already used, the button is back once (one more fuse possible). After the first visit with v0.10.6 the group is in the save.
- The tester's slot1: in the harness the restore of its s4_stad part logs `MAPSTATE repaired 1trainbutton: starts at 1stop2` and the train's target is 1stop2. Not run in game.

The s4_subway1st1 fuse rule (coop-runtime.hpp `carry_frame`, v0.6): every "fuse" card was taken from players entering any map named s4_subway1st1*. The stad fuse is not carried in that way: taking it fires `lockshdoor`, which locks `shitdoor` again (CCofInterDoorOnOff::Use toggles the lock, decomp effects.cpp 6654), and that door is the only way from s4_stad into s4_subway1st1. The other way in is s4_subtunnel1 -> s4_subway1st1_e1, a backtrack on which a carried fuse would have been taken. In this log the rule never ran (every level change went through the reconnect path, `REJOIN ... inventory restored`, and there is no `CARRY player=` line), so it did not cause report 3. It is narrowed to what v0.6 wrote it for: the chapter's first entry, s4_subway1st1_e0 with no kept state of the map.

Test: test-mapstate.cpp, v0.10.6 block: the stad's train and button are saved and come back as they were left; a group with a member that leads out stays as served whole; a v0.10.5-style state (no group records, *32 used) puts 1trainbutton at 1stop2, s4_subway2st1's pair through its manager at 2stop2, and leaves a looping train alone; with *32 unused nothing moves.

Audit, movewith groups with a usable child, all without a record in v0.10.5 saves (none is kept as served under the new rule):
- s4_stad_e0/e1/e2: 1trainbutton, func_button *30 (fuse take-back) <- trigger_once *32.
- s4_subway2st1_e0..e4: 1trainbutton (*41) and 2trainbutton (*43), the metro fuse take-backs <- multi_manager multitrigshit <- trigger_once *57 kalle.
- s4_college2_e0/e1: 1trainbutton, func_button *5 -> multipoff <- multiscrew <- multiscrew2 <- func_button scbutton (screwdriver lock, wait -1).
- s4_city2_e0/e1: 1trainbutton, func_button *226 -> tavlaparkbus <- multiafsub <- trigger_once *227 (master masterpower).
- s4_apartment3sick_e0..e4 and s4_avlopp_e0/e1: 1trainbutton, a ladder inter_door (*139 / *95) <- multiladder1 <- func_button ladder1b (ladder lock, wait -1).
- s4_sewer3_e0/e1: 1trainbutton, func_button *156 (the key cut free with the saw) <- multi_manager whenIdie <- monster_sewerboss (death). Not covered by the old-save repair (see above).
Other movewith groups (scene trains, the hanging men, sliding doors, the ending props) have no usable child; they are now saved too. The full list is in the audit output.

## Other things in the log

- At each level change both players drop and reconnect (`REJOIN player=N left`, then `REJOIN ... inventory restored`), the host too; the inventory comes back through the rejoin path, not `carry_frame`.
- s4_stad_e1, the last 2 minutes: `NET_CADENCE player=2 ~600 commands arrived in 300-400 host frames` and `SERVER_FRAME last60s gaps>100ms=7 max=800ms`: the host's frame rate fell to about 30/s on that map. Not investigated.
- Loading the save logged `SERVER_FRAME ... max=800ms` once on s4_stad_e2 (load time).
- The model box was scrolled through 32 times in the menu (`MODEL chosen`); harmless.

The v0.10 features are in docs/V010_RELEASE.md and tested with docs/V010_TEST.md; steps 55-62 cover this build.
