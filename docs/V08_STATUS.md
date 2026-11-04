# Cry of Fear co-op v0.8 status (Claude, 2026-11-01)

Nothing here has run in the game, and two players were not tested: Katz said on 2026-11-01 not to launch it ("first u cant"). "Fixed" below means fixed in code. Evidence is the logs, the clips, the decompile, the disassembly and the tests.

Package: W/outputs/Cry of Fear Co-op v0.8.zip (232 entries, 2,789,214 bytes, SHA256 5d23fc53fe927073079633022ea752610b5bd1856e538a602d221ab1ffefcd45). Catalog 5ed046bd..., adapter 61bde91a... (W/work/claude-s41-native-004), launcher 7df9b109... (0.8.0.0), package commit 9a3e75b. W = the project's private work folder.

Checked without the game:
- All 15 native fixture suites pass (W/work/claude-s41-tests-009.log), including every byte patch against the real client.dll, GameUI.dll and hl.dll. Node tests: 64 pass, 0 fail, 68 skipped because they need package builds (W/work/claude-s41-node-001.log, run before the package commit).
- Against v0.7.2, the ZIP changes only the launcher, READ ME, known-builds.json, release.json and the adapter.
- setup-coop -CheckOnly on D: "Found co-op v0.7.2. Updating it to v0.8", 1 file set aside.
- From the decompile: the engine lets "sc_report" through a server's stuffed text (it refuses only alias and connect), and the mod's console commands stay registered while a game is connected to someone else's server.

Not verified: a real install, uninstall or update through the launcher. It has only been tested with fake scripts, as in v0.7.2.

Evidence: Katz's Playtest 3000 folder (17 clips, more.txt, the host's and the partner's logs).

| # | Item | Status | What changed | Cause or limit |
|---|---|---|---|---|
| 1 | Per-player pickups | Fixed, not run in game (fixture passes) | Each map pickup is one entity per player slot on the same spot, except notes, key items, the phone, the knife and the P345. Only its owner is sent it, can touch it, or finds it with E. Which slots took what is kept per map and in slotN.cofmap version 3. | v0.6.1 copies stood side by side for everyone. |
| 2 | Key items for the team | Fixed, not run in game (fixture passes) | A key card one player gets goes to all. No free slot: not given, nothing dropped, not offered again. Used by one: taken from all. A drop leaves the others' copy. Counted, so two fuses stay two. The "<player> picked up" line is gone. | Each receiver sees the game's own "You got the <item>", not literally "You picked up <item>". That is the single-player text. |
| 3 | Save sound twice | Fixed, not run in game (fixture passes) | After a save-window save, only a player over 1000 units from the host gets the extra "play". | Native PreSave plays the sound too since v0.7. |
| 4 | Train crash | Fixed, not run in game | Nothing carried comes back on the map where the bag is lost; only health stays. Restores never give more than fits. Weapon cards with no weapon behind them are removed after 2 looks a second apart. Everyone still on the train is moved to the player who got out. | The game drops what does not fit: 5 weapons into 3 slots. The partner was under 160 units away, so he was skipped. |
| 5 | Ammo cap | Fixed, not run in game | 11 checked byte changes in hl.dll, one for iMaxAmmo1 and one per ammo box class. All sites were read in the disassembly of the pinned DLL. | Multiplayer branches gave 3 clips. |
| 6 | c_college1dark exit | Fixed, not run in game | The exit to s4_hamn3_e0 lands at 374 662 71, or the nearest clear spot. | The other exit, to s4_hamn3_e3, has its own co-op spawns and is unchanged. Tell me if that is the door you meant. |
| 7 | Player models | Host model fixed, not run in game. Crash not done. | Each game now reports its model to the server by client command at every map (`sc_report`, `sc_mdl`). | Host model: both logs show "Info string length exceeded" at setinfo. Crash: both crash logs are at RebuiltSimonCoFManager.dll+0x3C6B on Caixa's PC, right after connecting. That is the add-on, so it is left alone under the standing rule. I did not find why the third attempt worked. |
| 8 | Hand model | Fixed, not run in game (fixture passes) | Done on the Player Model box opens a Hands page in the same panel: same as the model, Simon, or an outfit. Kept in storycoop_hands.txt. | No new menu button and no custom hand models. The page reuses the player model pictures, so it does not show the hands themselves. |
| 9 | Zoom and subtitles | Fixed, not run in game (zoom fixture passes) | Skip prints no subtitles, stops running subtitle lists and clears the line on screen. A zoom in a skipped scene never starts, and a running one returns. A player whose view is still zoomed when a scene ends is put back to 90 FOV. | Zoom after a skip: pp_activate renames each cof_camerazoom, so skip never found it (every log says "0 zoom(s) returned"), and the zoom then started after the scene. A zoom inside a scene you watch is the game's own and stays. |
| 10 | Bosses | Fixed, not run in game | Sawer: a second after the death scene, the exit runs. Game over: text and fade for 2.5 s, then restart. A boss bar starting saves the fight in hidden slot 6 once nobody is held by a scene, and a game over loads it. | Sawer's shaft is unlit and depends on a falling wall; I did not find why it failed. Sewer, Roof and Doctor have no such wall: their death scenes use story teleports, covered by item 11. A grab still restarts at once with no delay, because of the DeathNotice crash. |
| 11 | Everyone follows | Fixed, not run in game | Every story cof_teleport fired by name moves the others beside whoever it moved, and a dead player's body too. A player still loading is moved when ready. Scene end moves anyone over 200 units from the triggering player. | Not on s4_nightmare (passive map), and not for teleports behind a trigger_multiple, which are passages each player walks. Clip 7 itself was not traced to its map. |
| 12 | "Picked up" on map load | Fixed, not run in game (fixture passes) | Inventory messages are silent, and "You got" / "Picked up" lines blanked, until a player's inventory is back. The item-get sound is held back too. | |
| 13 | Doctor and police loop | Fixed, not run in game (fixture passes) | Item 11's teleport rule. Exit triggers also do nothing for a player's first 3 s on a map, and no trigger fires during the load wait. Anyone who ends up in that corridor with *40 already used is sent to the station by the map's own teleport. | Katz's spot, s4_subwaysick2_e0 to s4_subway2st1_e2 at -236 -1588 -143, is where trigger *40's teleport lands the first player (destination -237 -1589 -208). All four co-op spawns stand in *40, 16 units from exit *37. The other player stayed in the dark corridor and walked into *37, and s4_subwaysick2 *82 came straight back. After that *40 was used, so nobody could reach the station. Host log 2026-10-31 shows the loop. |
| 14 | "Zoom" | Investigated; speed cap fixed, not run in game | The wheelchair's speed cap now holds every frame. | Clip 17 is the host's own wheelchair on the final map (ping 0, HUD speed 287 against a cap of 220), not ping or prediction. The map was inferred from the clip's health and hands. If 220 still feels too fast, the cap needs lowering. |

## Code review 2026-11-04 (Claude, not run in game)

A read of the adapter, the launcher and the install scripts for conflicts between features, checked against the decompile. There are no Linux scripts in the repository. Fixed in code (fixture cases in test-world.cpp; adapter build W/work/claude-s42-native-002, suites W/work/claude-s42-tests-002.log):

| Area | Problem | Fix |
|---|---|---|
| Key items (item 2) | Two players taking the map's two fuses in the same 0.1 s ended with one fuse between them: the receiver's whole inventory was read again after the give, so his own pickup was never seen. | What the code gives or takes is added to the known counts. |
| Key items | A use took the card from the others down to the user's new count. A partner who held two fuses (the other one had no free slot for the second) lost both when one was spent: a socket left without a fuse. | A use takes as many as were used from each other player. A pickup adds to what they hold. A key picked up again after a drop is the same key (counted on the floor), so nobody gets a second copy. |
| Key items vs fall respawn and rejoin | A player given his inventory back (restore_player) never got the key items the others found while he was dead or away. The comment promised it. | A restore makes him new to the team: once settled he gets what the others hold, and his own restored cards are not shared as pickups. |
| Two-half items vs key items | The halves (number plates, scissors) are key items too. The v0.3 half move (one player's half given to the partner, cleared from him) looked like a use and could take the half from both players. | The move is recorded as neither a pickup nor a use. |
| Load wait (v0.6) | A wait armed by a level change was kept by the main menu and a new game. After quitting while a partner was still loading, the next game's first map held a lone host frozen for 60 s with every trigger shut. | reset() clears it (new game, main menu, Continue). |
| Pickups (item 1) | When a slot's copy could not be made (edict limit), that slot had nothing at that spot. | The map's own pickup is then everybody's, first come first served, as before v0.8. |
| Launcher | A .part file left by a launcher killed mid-download made the next update fail once ("file exists"). | The download overwrites it. The launcher binary is not rebuilt; the next package does that. |

Checked and left as they are: per-player copies are never written to the map state, so a revisit does not double them. cof_clearitems takes one card per Use (ClearOutSlotNamed, all=0). subtitle_main lookups are null-checked by the game. A skipped scene's zoom is returned through the fan-out counter. The boss autosave (slot 6) is refused while a load is still restoring someone, and is tried again every 2 s.

Ladder climber (Katz: "fix that limit"): a player climbing when a story teleport or a scene end moved the others was moved too, then put back at the ladder's end when the climb finished (LadderThink sets his origin). He now stays on the ladder and is moved beside the same player once the climb is over and his controls are back (within 60 s). Known limit, not changed: R typed in the chat box during a scene counts as a skip vote.

## Tests for Katz to run

Full list: docs/V08_TEST.md. Solo first:

1. Host alone with 2 slots: each pickup shows once; taking it leaves nothing behind.
2. Change maps: no "You got" lines. Save at a recorder: one sound.
3. Fill up on M16 ammo past 3 magazines.
4. `changelevel s4_apartmentsick_e0`, kill Sawer, skip the scene: the next map loads. Die in the fight: red text, fade, restart.
5. Player Model box: Done opens Hands. Log line `MODEL player=1 reports model ...` on the next map.
