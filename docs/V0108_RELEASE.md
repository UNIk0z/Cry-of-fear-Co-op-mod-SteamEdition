# Cry of Fear co-op v0.10.8 (2026-11-21): Katz's requests

v0.10.7 plus Katz's requests of 2026-11-21. Maps are byte-identical to v0.10.7. **Nothing here has run in the game.** There was no new two-player log: the only new host log on this PC (rc4-20261120-151417.log) is menus only. Every cause below comes from reading the decompiled code and the pinned binaries, and each change has a fixture.

| Request | Cause found | Change |
|---|---|---|
| "the host's lighting sometimes breaks" | (a) The client's ResetHUD/InitHUD set its Maglite state to 0, and the server sends "Maglite" only when it changes. (b) A switched `light` came back on a revisit with its saved state but the style its spawnflags give. Not proven to be what Katz saw. | (a) After every ResetHUD/InitHUD the light state is sent again. (b) A restored switchable light gets its saved style. |
| Stuck in each other after a door that changes the map (collision on) | A placement (door, arrival, teleport) put two solid players on one spot; neither could walk out | Two players inside each other for 0.5 s: the higher slot is moved to a clear spot |
| Ladders stuck for the second player | The climb reads its camera's button bits, set only by edge commands that the client sends once it knows it is on the camera; one manager's queued drop was a global shared by every climb | The camera's forward/back bits follow the keys held; each manager keeps its own queued drop |
| Remove the scare; picture mode on `]`; ping on H | Request | Scare removed (an old J bind does nothing); new default keys; old default binds moved over |
| Enemy hitboxes (the crawler and others) | A bullet only tests a monster's hitboxes after crossing its abs box, which is its small move hull | A monster's abs box covers what it plays; hitboxes 12% larger |
| Inventory sometimes not loaded after dying twice | At a game-over restart the rejoin path kept each dead player's own state (empty after a respawn), and it won over the arrival record and over a loaded save | A game over keeps no departure records |
| An autosave on each map change | Request | Every map arrival is saved into the hidden slot 6 (as the boss autosave); a game over loads the newest save of the map |
| 30 s respawn, weapons kept, no "(lose weapons)", the text gone once up | Native: 60 s, the respawn strips everything, the notice repeats every 12 s | Respawn unlocks 30 s after going down, items come back, own notice, cleared once up |
| Joiners see the host in their own skin | A full userinfo loses "model"; the other games then draw the base Simon model, which the client paints with its own outfit | The no-longer-used `sc_model` key is removed before the model is set, and is no longer added at launch |

## 1. The host's lighting

Two ways the adapter (or the game under co-op) can leave a light wrong, both fixed; neither is proven to be the one Katz saw. Lines to ask for: `LIGHT player=N HUD reset: light state sent again` and `MAPSTATE N switched light(s) set to their saved style`.

(a) Maglite. Client ResetHUD and InitHUD (decomp hud_msg.cpp 405, 453) set `m_iMaglite` to 0. The server sends the 1-byte "Maglite" message only when `m_iFlashlight` (+0x1EA8) differs from `m_iClientFlashlight` (player.cpp 6784), and ForceClientDllUpdate (8442) resets the FlashFlags copy but not that one. So after a respawn, a revive, a map start or a "fullupdate", a Maglite that was on stayed on in the game and dark on that player's screen until it was switched off and on. Now every ResetHUD/InitHUD sent to a player (the message hook already sees them) marks him; in his next PreThink the adapter forces FlashFlags again (+0x5D4 = -1, as the scene code does) and, if his Maglite is on, sends the Maglite byte itself (session-runtime.hpp `light_resend`; `Maglite` id from RegUserMsg).

(b) Map lights. Spirit's `light` (decomp lights.cpp) keeps `m_iState` and `m_iszCurrentStyle` in its save fields, but `CLight::Restore` does not call LIGHT_STYLE. Single player gets the engine's style table back from the save file; the adapter's map state (v0.3) does not. A light switched on (or off) on an earlier visit came back with the right state and the style its spawnflags give, until something toggled it. After its restore each light with a switchable style (32-60) now gets its saved style through the engine's LightStyle (state-runtime.hpp `state_light_style`): offsets from the pinned hl.dll, `CLight::SetStyle` at 100B7620 (`m_iStyle` +0x64, `m_iszCurrentStyle` +0xFC), engine slot 43.

Checked and left: the v0.8 per-viewer ScoreInfo filter (story-runtime.hpp) matches the client's spot keys (decomp gl_studiomodel.cpp DrawPlayerFlashlights keys -i, view.cpp SetupFlashlight -1 and -2, Maglite -8, flashlight hands -50 and the player's index); the scene hold/release of the lights (camera-runtime.hpp) restores what it took.

Tests: test-session.cpp (a ResetHUD sends the Maglite byte once and forces FlashFlags; Maglite off: FlashFlags only); test-mapstate.cpp (a switched light gets its style; a global style, an unchangeable one, no style and another class do not).

## 2. Stuck in each other (collision on)

With collision on two solid players cannot move into each other, so boxes that overlap are always a placement. `player_overlap_frame` (coop-runtime.hpp) runs every server frame. Two living players count as inside each other when their abs boxes overlap by more than 4 units on every axis: SV_LinkEdict pads each box by 1, so players side by side overlap by 2. Players in a scene, on a ladder, frozen or noclipping are skipped. After 0.5 s the one in the higher slot is moved with `find_landing` (the hull trace sees the other player) and `teleport_to`, at most once a second. Log: `COLLISION player=N was inside player=M: moved to a clear spot beside`, or `no clear spot found (K: unstuck)`. Off with collision off.

Test: test-world.cpp (moved after 0.5 s, not before; side by side: nothing; collision off: nothing).

## 3. Ladders

(a) Input. A climb steps on IN_FORWARD / IN_BACK in its camera's `pev->button` (decomp CoF_Ladder.cpp 431-440). Those bits are set only by the client commands forwardpressed/forwardreleased and backpressed/backreleased (client.cpp 1441-1468). The client sends them only once `pAttachCamEnt` is set (input.cpp 512), so a press before the AttachCam arrived, or a release after the view left it, never reached the camera. For the player whose AttachCam came late (the remote one) the climb stood still, or went on by itself. The client's usercmd always carries the keys held (CL_ButtonBits), so while a player climbs, his camera's two bits now follow his `pev->button` (+0x184) every frame (coop-runtime.hpp `ladder_input_frame`).

(b) The queued drop. `StartNextAnimation` keeps one queued 48-unit camera drop in a global bool, `wasqueuedtoreduceorigin` (CoF_Ladder.cpp 32, 570-577; hl.dll 10222E4C, written at 1004F4BA). Every ladder manager shares it. Since v0.7 two players climb at once on two managers (a copy for the second), so one climb used up the other's drop and its camera got out of step with the animation. Each manager now has its own copy: put in place around its Think and kept after it, with 0 between thinks (ladder-runtime.hpp `LadderQueueScope`, wrapped around DispatchThink). A Use outside a Think runs with 0.

Tests: test-ladder.cpp (the bits follow the keys held; not climbing: nothing; a drop kept for its manager only; other entities untouched).

## 4. Keys: no scare, picture mode `]`, ping H

- The scare (`sc_scare`, J) is gone. An old bind is taken by the adapter and does nothing.
- Defaults: picture mode (HUD on/off, `sc_hud`) on `]`, ping on H.
- `storycoop_comfort.cfg` as v0.10.4-v0.10.7 installed it (both line-ending forms) is replaced once by Install.
- A changed comfort file that still has the old defaults (H HUD, G ping) gets the new binds when the adapter runs it, and every key bound to `sc_scare` is unbound. A key that config.cfg still binds to `sc_hud` or `sc_ping` and the comfort file no longer uses is unbound too (`comfort_v0108_keys`).
- Launcher SETTINGS has five keys and accepts `[` and `]`. A file with the old defaults shows the new ones.
- In the game's own config `]` is `invnext`; co-op replaces it, and SETTINGS says so.

Tests: test-qol.cpp (`comfort_v0108_keys`: old defaults moved over, the scare and an old G ping unbound, a player's own keys kept), test-qol2.cpp (the scare does nothing), tests/v0108.test.mjs.

## 5. Enemy hitboxes

A bullet is a point trace. The engine clips it against a monster's model hitboxes (decomp r_studio.c SV_HullForStudioModel: a zero-size trace takes R_StudioHull), but only after the trace crossed the monster's abs box (world.c SV_ClipToLinks). The game's SetAbsBox makes that box from the movement hull. So any part of a body that its animation puts outside that hull (a crawler's head and arms ahead of its low hull, a lunge, a long reach) could not be hit at all. Now the DLL's SetAbsBox (slot 9) is wrapped (coop-runtime.hpp `monster_abs_box`):
- A living, solid monster's abs box also covers the box of the sequence it plays (mstudioseqdesc_t bbmin/bbmax, turned with its yaw, times its scale). The box is only ever made larger; the move hull, collisions and pathing do not change.
- Each hitbox of a monster's model grows by 12% of its size on every side (at least 2 units), once per loaded model (the model data is in memory; files are not changed). Log: `HITBOX N hitbox(es) of <model> grown for co-op`.

Not done: lag compensation for monsters. The engine rewinds players only, and a joiner sees monsters about 60 ms in the past (ex_interp) plus his ping. If moving crawlers are still hard to hit for the joiner and not for the host, that is the next step.

Tests: test-qol2.cpp (the box covers the turned sequence box and never shrinks; hitboxes grow once; dead monsters and players untouched).

## 6. Dying: inventories, the map-change autosave, the respawn

(a) Game over (Katz: "when we died after 2 times there is a chance that the inventory doesnt get loaded"). At a game over the game restarts the map, and the clients drop and reconnect. `rejoin_departing` (story-runtime.hpp) kept each dead player's own state: empty after a native respawn, the second death. That record then won over the arrival record that the restart gives back (`carry_map_changed`), and over a loaded save, because `rejoin_frame` restores it after the checkpoint restore. Now a game over keeps no departure records: they are not taken while `gameOverMap` is set, and they are dropped at ServerDeactivate. The save, or the arrival record, decides.

(b) The map-change autosave (`arrival_save_frame`, coop-runtime.hpp). Once everyone has arrived on a map, it is saved into the hidden slot 6, as the boss fight is (v0.8); no menu lists it. "Everyone has arrived" means: in, carried inventory applied, alive, not in a scene, the load wait over, 3 s settled. A game over then loads the newest save of the map (this one, or a tape save made later), so everyone gets back what they had when they came in. It is skipped:
- when this session already has a save of the map (loaded or made, it is newer);
- with saving off (sv_coop_nosave);
- during a boss fight;
- on the s4_start maps, which save only at their two recorders.

Log: `AUTOSAVE map arrival on <map>: saved`.

(c) The respawn (Katz: "remove the (respawn and lose weapons) and instead add a 30 second timer ... weapons would stay and make sure the respawn text disappears if revived or respawned"). Natively (decomp player.cpp MultiplayerKilled 2464, PreThink 3998-4035) jump respawns after 60 s. The client notice "Press <jump> to respawn (lose weapons) or wait to be revived" (hud_msg.cpp 735) repeats every 12 s, and the respawn strips everything. Now (`respawn_unlock_frame`, qol2-runtime.hpp):
- Jump respawns 30 s after going down (+0x3D4).
- The native notice is held off (+0x3D0). Our own notice says "You can respawn in N s, or wait to be revived." at 0, 10 and 20 s, then "Press jump to respawn (you keep your items), or wait to be revived." every 12 s.
- The player comes back with his last good record (the fall respawn's FallBack, coop-runtime.hpp), only when the respawn emptied him; a revive keeps his items by itself.
- Once he is up (revived or respawned) the notice is cleared (an empty ProFont).

Hardcore (sc_norespawn) keeps its rules and says "Wait for a teammate to revive you (hardcore: no respawn)." The automatic respawn (sv_auto_respawn_time) also keeps the items. storycoop_server.cfg says so; the v0.10 file is replaced once when unchanged.

Tests: test-story.cpp (no departure during a game over, none kept after it), test-checkpoint-runtime.cpp (the arrival save after 3 s settled, not before, not again once the map has a save, not on s4_start), test-qol2.cpp (30 s, the countdown, the native notice held, hardcore, the notice cleared once up).

## 7. Skins: joiners see their own skin on the host

The engine draws each player with his userinfo "model" (decomp r_studio.c `studioapi_SetupPlayerModel`). When that key is empty or the file is missing it draws the entity's own model, models/player.mdl (Simon). The client paints that model's hoodie_grey, jeans and head_face textures with its OWN outfit (gl_studiomodel.cpp 1628), so each player saw the other in his own skin.

The installed outfit models avoid those texture names (checked on this PC: hoodie_coop, jeanss, head_coop), so the model being missing is the cause. v0.8 recorded "Info string length exceeded" at every setinfo on both PCs. A full userinfo loses "model" altogether, because the engine removes the old key before it refuses the new value. The launchers and the model box add an `sc_model` key the server has not needed since v0.8 (the sc_report answer counts). Changes:
- Before setting "model", the server removes `sc_model` from that player's userinfo (session-runtime.hpp `model_frame`).
- Install and Run no longer pass `+setinfo sc_model` (host-story-coop.ps1, coop-linux.py).
- The model box clears it.
- After three failed tries the log gives the userinfo length (`MODEL player=N userinfo is L of 255 characters`).

Not proven: no two-player log of this case on this PC. If it is still seen, that line and `MODEL player=N still ...` tell why.

Test: test-session.cpp (sc_model removed, then the model set; not again once gone), tests/v0108.test.mjs.

## Not changed

Monster lag compensation (above). The v0.10.6/v0.10.7 items still not played: docs/V010_TEST.md steps 55-69.

The v0.10 features are in docs/V010_RELEASE.md and tested with docs/V010_TEST.md; steps 70-81 cover this build.
