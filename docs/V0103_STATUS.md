# v0.10.3 status (2026-11-08, Claude)

Katz's requests of 2026-11-08 (through the project chat): one item per pickup spawn ("4 syringes in each syringe spawn ... separate for each player"), and doors that change the map wait for the team again, as a host option on by default, on every door that needs it. Built from main c15d082 (v0.10.2). Nothing has run in the game.

## Where it is

| # | Change | Code | Fixture |
|---|---|---|---|
| 1 | The maps' pickup copies are removed when a story map starts | coop-runtime `FirstCopies`, `first_copy`, `same_pickup`, `pickup_extras_frame` (start_frame) | test-world v0.10.3 block |
| 2 | No per-player copies (v0.8 `item_copies_frame`, `pickup_hidden`, owners, v0.10.2 `pickup_group`/`pickup_unblocking` removed); the map state no longer writes per-slot masks (the field stays for old saves) | coop-runtime, adapter `full_pack`, qol2, state-runtime | test-world, test-mapstate (old masks still decode) |
| 3 | The phone battery is a team card that each player keeps when another uses his | coop-runtime `personal_card`, `kept_card`, `team_cards_frame` | test-world |
| 4 | Host option `sc_doorwait` (1 by default): Create Server window (sc_setup.scr), storycoop_server.cfg; installers replace an old sc_setup.scr without it once | qol2 `doorwaitCvar`, `host_options_frame`; install-story-package.ps1, coop-linux.py | test-qol2, tests/v0103.test.mjs |
| 5 | Game co-op doors: `door_ready_after_think` opens their zone only with sc_doorwait 0 or one living player | qol2 | test-qol2 v0.10.2 and v0.10.3 blocks |
| 6 | Plain doors into a level change wait: E (PlayerUse trace and FindEntityInSphere) does not find them while a living player is outside the door's zone box; the user gets the game's message | qol2 `door_wait_players`, `door_leads_out`, `door_everyone_at`, `door_wait_blocked`, `door_wait_trace`; adapter `find_in_sphere` | test-qol2 v0.10.3 block |

Decomp and pinned-binary facts used:

- CTriggerCoopChangelevel::InsideThink (triggers.cpp 6616) and CountConnectedLivePlayers (6571): alive and fuser4 (+0x264) 0.
- CInterDoorCoopChangeLevel::Use (CoF_InterDoorCoop.cpp 879): "Please wait until everyone is ready!", colour 2.
- CInterDoor::Use and EndSequence (CoF_InterDoor.cpp 224, 399): lock, disabled, useunlock, owner, next use; the exit origin + 37; "endingcheck" picks ending_exit1..5.
- CInterDoor save table in the pinned hl.dll at 10212400: m_bDisabled +0x180, m_sLockedBy +0x190, m_sUseUnlock +0x1A0, m_fNextUse +0x1A4.
- CBasePlayer::PlayerUse (player.cpp 3468): the 64-unit trace first, then FindEntityInSphere.
- CPhoneBattery (items.cpp 474): an inventory card, inventoryitems/mobile_battery.txt.
- Every classname on the served maps is exported by the pinned hl.dll, so the adapter's spawn list position is the BSP entity index.

## Map audit (scripts/audit-v0103.mjs; tests/v0103.test.mjs with COF_SERVED_MAPS and COF_ORIGINAL_MAPS)

194 served maps (the claude-s25 map build) against the original c_*.bsp. No problems: every pickup appended after the original map's entities is an ammo_/weapon_ copy of an earlier pickup, 1288 in all, and no original entity turned into a pickup. Game co-op doors: 298, every one with its zone. Plain doors that change the map: 203. Level-change triggers with no door in front (most are hidden boxes behind converted doors, the rest scripted ending and fall boxes or walk-in exits) and the scripted teleports into a level change after a scene are left as they were.

Plain doors that change the map (now waiting with sc_doorwait 1):

| Map | Entity | Exit | Door | Variants |
|---|---|---|---|---|
| apartment1 | 386 | exitdbl2b | locked (asddas) | 8 |
| apartment3sick | 462 | exhej1 | named ladder1door | 5 |
| apartment5 | 58 | exitdbl5 | named fittansman | 2 |
| asylum1 | 304 | aa1_do_2_2 | locked (lockedelec), named elecdoor | 3 |
| asylum1 | 572 | fda_exit2 | locked (asdasdas) | 3 |
| asylum1dark | 219 | fda_exit22 | locked (asdasdasd) | 2 |
| asylum1day | 215 | aa1_do_2_l | named elecdoor | 2 |
| asylumb | 20 | asb_do_22 | named doorchanges | 2 |
| attic | 4 | aa1_do_2_1 | named door2change | 2 |
| avlopp | 275 | exhej2 | named ladder1door | 2 |
| buscity2 | 253 | pa3_exit1 | locked (blockach1), named chdoor1 | 3 |
| buscity2 | 255 | pa3_exit2 | locked (blockach2), named chdoor2 | 3 |
| buscity | 41 | exlsel1 | - | 3 |
| buscity | 250 | exitbcfuck | named doorbclock | 3 |
| city | 206 | sw_exit1 | locked (asdsdasda) | 4 |
| city | 347 | d1_1_exit1 | locked (bclocky), named bclockeddoor | 4 |
| city | 353 | d2_1_exit1 | locked (citysewerkey) | 4 |
| college1dark | 339 | danmsase | locked (firekey) | 3 |
| college2 | 2 | oexd3_2 | named doorch1 | 2 |
| college2 | 25 | oexd3_2 | locked (adsjadjk), named blockingdoor | 2 |
| down | 2 | 3b_exit12 | - | 1 |
| ending1c | 769 | fo5_exit2 | locked (asdasdsd) | 1 |
| ending2c | 609 | fo5_exit2 | locked (asdadsas) | 1 |
| ending3c | 609 | fo5_exit2 | locked (asdasdasd) | 1 |
| forest4 | 13 | fo3_do_2_2 | locked (dssdasd) | 2 |
| forest4 | 17 | fo4_exit2 | named doorlocko | 2 |
| forest7 | 191 | fd7_exit22 | named exitfucka | 3 |
| forestday | 8 | fod_e2 | locked (asdasd) | 2 |
| fsewer1 | 91 | vi1_e1 | locked (fsewerkey) | 2 |
| hamn3 | 16 | hac_exit2 | named doortc1 | 4 |
| hamn3 | 154 | 2hac_exit22 | locked (tjenakungen), named 2doortc1 | 4 |
| park2 | 88 | sw_exit1kl | named 4dernn | 3 |
| park2 | 190 | sw_exit2h | named dernn | 3 |
| park2 | 361 | sw_exit2h | named dernn2, starts disabled | 3 |
| park2 | 369 | sw_exit1kl | named 4dernn2, starts disabled | 3 |
| park3 | 167 | sw_exit15 | locked (entrancekey) | 4 |
| park3 | 372 | pa3_exit12 | locked (blockadock), named chdoor12 | 4 |
| park3 | 374 | pa3_exit22 | locked (blockadock2), named chdoor22 | 4 |
| park3 | 662 | sw_exit2kl | named doorparkmus | 4 |
| park | 222 | sw_exit2ea | locked (gatekey1) | 2 |
| sewer2 | 55 | sw_exithaha2 | locked (dasdsasda) | 2 |
| stad | 1 | sw_exit1h | named doornen2 | 3 |
| stad | 138 | st11_exit1 | locked (gabriella), named shitdoor | 3 |
| stad | 495 | sw_exit1h | named doornen, starts disabled | 3 |
| subtunnel2 | 270 | 2exlsel1 | - | 2 |
| subway1st1 | 173 | s4_1_exit2 | named lockedd1 | 2 |
| subway2st1 | 27 | oense | locked (greger), named lockshit2 | 5 |
| subway2st1 | 721 | sw_exit1t | locked (gabriella2), named shitdoor | 5 |
| subway2st1 | 727 | sw_exit1t | locked (dasadsasd), named fuseddoor, starts disabled | 5 |
| subway2st2 | 121 | sw_exit2t | - | 4 |
| subway2st2 | 548 | sw_exit2n | - | 4 |
| subway2st2 | 592 | s2_exit1 | named removed | 4 |
| subway2st2 | 598 | e2_exit1 | named doorl, starts disabled | 4 |
| subway2st3 | 428 | oensa | - | 4 |
| village1 | 267 | vi1_e2 | locked (dsadasasddsa) | 2 |
| village1 | 285 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 286 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 287 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 288 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 289 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 290 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 291 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 292 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 293 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 294 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 295 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 296 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 297 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 298 | vi1_e2 | locked (dsdsa) | 2 |
| village1 | 311 | vi1_e2 | locked (dsdsa) | 2 |
| village3 | 287 | endingcheck | the ending door (ending_exit1..5) | 6 |

Game co-op doors per map (different doors): apartment1 5, apartment2 4, apartment3 4, apartment3sick 4, apartment4 3, apartment4sick 3, apartment5 1, asylum1 1, asylum1dark 1, asylum1day 1, asylum2 2, asylum2dark 2, avlopp 1, basement 1, bridge 1, buscity2 1, buscity 1, city2 2, city 1, college1 1, college1dark 2, ending1 1, ending1b 1, ending2 1, ending2b 1, ending3 1, ending3b 1, ending4 1, ending5 1, ending5b 1, ending5c 1, forest1 2, forest2 2, forest3 2, forest4 1, forest5 2, forest6 2, forest7 1, forestday 1, fsewer1 1, hamn2 2, hamn3 1, hamn 2, lake 2, park2 1, park 1, roofs2 1, roofs 1, sewer2 1, sewer3 2, sewer 2, stad 1, start 2, start2 1, subtunnel1 2, subtunnel2 1, subway1st1 1, subway2st1 2, subway2st3 2, subwaysick1 1, subwaysick2 1, trainride2 1, trainride 1, village1 1, village2 2, village3 1.

Pickup copies removed per map (all variants together): apartment1 104, apartment2 12, apartment3 10, apartment3sick 40, apartment4 27, apartment5 14, apartmentsick 24, asylum1 24, asylum1day 18, asylum2 32, asylumb 6, attic 52, avlopp 26, basement 18, bridge 34, buscity2 18, buscity 24, city2 28, city 36, down 6, ending1c 6, ending2c 6, ending3c 6, ending4b 6, ending5c 11, forest1 30, forest2 18, forest3 6, forest4 54, forest5 18, forest6 18, forestday 30, fsewer1 50, hamn2 18, hamn3 12, hamn 6, park2 36, park 6, roofs2 6, roofs 28, sewer2 12, sewer3 10, stad 9, start 33, subtunnel1 58, subtunnel2 12, subway1st1 24, subway2st1 45, subway2st3 24, subwaysick1 6, trainride2 3, village1 42, village2 50, village3 36.


## Tests

- Native: 18 suites PASS, adapter claude-s48-native-003 (SHA256 3bef00b65156d2e0f51d4f972861fc73bc372ecb5ef3caa2fb01433a59b26387).
- Node: 108 pass, 0 fail, 29 skipped (COF_SERVED_MAPS, COF_ORIGINAL_MAPS and the older map builds set; COF_V04_MAPS must stay unset, it is the v0.4 build).

## Not verified

- Nothing has run in the game. The door wait for plain doors depends on PlayerUse calling the engine's TraceLine and FindEntityInSphere through the adapter during PreThink, as the v0.10.2 key-inside-a-door fix does (also not played yet).
- The battery hand-out uses the game's cof_giveitems as the key items do (played since v0.8).

## Virus scan (Windows Defender, MpCmdRun -Scan -ScanType 3 -DisableRemediation)

Signatures 1.463.191.0 (updated 2026-11-08 09:38), engine 1.1.26100.4, real-time protection on.

| File | Result |
|---|---|
| outputs/Cry of Fear Co-op v0.10.3.zip | no threats |
| outputs/Cry of Fear Co-op v0.10.3 Linux.tar.gz | no threats |
| outputs/coop-v0103-player (folder) | no threats |
| outputs/coop-v0103-linux (folder) | no threats |
| outputs/v0103-candidate (folder) | no threats |
| work/claude-s48-native-003/storycoop_rc4.dll (the shipped adapter) | no threats |

A clean local scan does not guarantee that a tester's PC raises no machine-learning warning (docs/V0102_STATUS.md).

## Packages

Windows `Cry of Fear Co-op v0.10.3.zip` (SHA256 `95a5d9c6...`), Linux `Cry of Fear Co-op v0.10.3 Linux.tar.gz` (`45523bac...`), catalog `6ecec127...`, commit `fdf3943`. Against v0.10.2 only the launcher, READ ME, sc_setup.scr, storycoop_server.cfg, the installer script, the adapter, release.json and known-builds.json differ. `setup-coop -CheckOnly` on Katz's game: "Found co-op v0.9. Updating it to v0.10.3", 248 files current, set aside cl_dlls\storycoop_rc4.dll and txtfiles\storycoop_hints.txt (the same plan as v0.10.2).
