# v0.10 status (Claude, 2026-11-06)

Base: Codex's v0.9 Stability Fix (repo 2ce7faf, public clone c333601). Nothing in v0.10 has run in the game; Katz tests docs/V010_TEST.md (solo first). Native: 18 fixture suites pass (new: test-qol2.cpp). New source: native/rc4/qol2-runtime.hpp.

## Codex's spray work since v0.9 (reviewed)

- Sprays were invisible because CoF's own world renderer never draws the engine's decal pool. Codex draws each received player logo from the public SDK draw callback instead, and plays the game's shared spray hiss after an accepted decal. Both installers allow and check that sound file.
- FPS: only each player's newest spray is kept, with its projected geometry cached.
- Crash and fade/ESC tint: the first spray build used shifted client API slots (it called the screen-fade setter and the shake routine). Fixed to entity 53, time 54, level 74, with an engine-table test.
- Found nothing that blocks building on it. Small note: the client timer prints some spray lines to the engine console; harmless, but noisy in a console log.

v0.10 keeps all of it unchanged; V09_PARENT in the packager is the Stability Fix catalog (cdc747ae...), so the installer upgrades it.

## Per feature (docs/COOP_QOL_PROPOSAL.md numbers)

| # | Feature | Status |
|---|---|---|
| 16 | Hunted: Book Simon stalks the team | Done, experimental, off by default. Uses `monster_booksimonsledgehammer`, not `monster_booksimon`: the latter switches the global Simon mode (co-op doors, music for everyone, boss bar). Spawned with the game's own Spawn/KeyValue, removed with a trigger_relay killtarget. Unknown until played: how well he paths on each map. |
| 17 | Run timer and chapter times | Done (`sc_timer` 0/1/2). Own clock, not the game's survival timer (that one counts down). Kept in saves. |
| 14 | End stats | Partial: the game's end-of-game stats screen already shows (since v0.5). Per-chapter kill/down/revive cards skipped: crediting a kill to a player needs the game's damage code, which the adapter only sees through a detour. |
| 18 | Scare | Done (J). |
| 19 | Hardcore | Done: no saves (`sv_coop_nosave`, also refuses the co-op recorder), no respawn while down (`sc_norespawn`), morphine trip time, no hints (`sv_disablehints`, also our own pop-ups). |
| 20 | Vote kick | Checked in the decompile (multiplay_gamerules.cpp, `votekick` and `sv_votekick_time`); README and storycoop_server.cfg lines. Not run on a story map yet (test 18). |
| 21 | Dodge kept | Done. |
| 22 | Late-join catch-up | Done: weapons the host carries, moved beside the host if far. No extra "joined" notice (the game already prints one). |
| 8 | Ping | Done (G). |
| 9 | Player-count scaling | Done (`sc_scale`), monster health x(1 + 0.35 per extra player); bosses excluded so boss bars stay right. |
| 10, 30 | Outline, teammate colour | Done (`sc_outline`; colour in SETTINGS). Colour is used for the ping and the outline only, not for spray tint or names. |
| 26 | Own light colour and size | Done in the launcher SETTINGS (client cvars `cof_reducedlightcolour1-3`, `cof_reducedlightradius`). |
| 35 | Black and white | Done (per player, SETTINGS). |
| 11 | Spectate while down | Skipped: it needs SET_VIEW on the downed player, and cutscene cameras use the same view. Getting that wrong leaves a player looking through the wrong camera; that cannot be made safe without playing scenes. |
| 25 | Torch tint and beam | Skipped: the colour is a constant in the client and the beam is a texture; changing them means patching client code and shipping new textures, unsafe without a game run. #26 gives a safe light option instead. |
| 31 | Host music | Skipped: `sv_coop_music` plays on every spawn and would cover the story music; there is no lobby to play it in. |
| 23 | Commentary as tips | Skipped: needs written and recorded content per spot. |
| 33 | Outfits per player | Skipped: the Player Model picker already covers outfits. |
| 34 | Hand textures | Skipped: needs texture files and overlaps the existing hands patch; not low risk. |

Rejected by Katz and not built: regroup vote (#2), voice callouts (#41), ready-up lobby (#43), look-at names in scenes.

## Review fixes (2026-11-06, before release)

A second pass over the v0.10 diff, checked against the decompile. All in the v0.10 packages now:

| Where | What was wrong | Fix |
|---|---|---|
| state-runtime.hpp state_capture | The map state keeps every live runtime `monster_*`. A hunting Simon alive at a level change or a tape save came back on the revisit or load, untracked: never sent away by scenes, a second one spawned beside him. | The hunter (`sc10_hunter`) is never kept. |
| qol2 scale_frame, state_activate | A revisit or a save load restores monsters with their saved (already scaled) health, and the scaled list starts empty each map: x1.35 again on every revisit. | Monsters the map state restores count as scaled. |
| qol2 noir_frame / BlackWhite hook | The BlackWhite byte is a strength (client gl_postprocess.cpp: grey = byte / 255). We sent 1: black and white was invisible, and the story's own grey scenes were cut to 1/255 for that player. | 255. |
| qol2 norespawn_frame | With "no respawn while down", a downed player whose last living teammate left could never respawn: the game ends the map when the last living player dies, not when one leaves. | The hold applies only while a teammate is alive to revive him. |
| qol2 ping_command | TE_GLOWSPRITE's first byte is life in 0.1 s: the glow lasted 0.6 s. Ping also ran on the opening maps with the previous map's sprite index, and its text went over the skip prompt of a teammate in a cutscene. | Life 3 s; story maps only; the sprite index is cleared at each map change; no text to a player in a scene. |
| qol2 catch_up_frame | Every player's carry record exists once settled, so catch-up gave the host's weapons to any player with none on his first map, including players who started the game together. | Only a player ready more than 15 s after the first one (the objective reminder's "joined late"). |
| qol2 run_timer_title | A game over restarts the map and its chapter card fires again: a second split, and the chapter time started over. | The same card twice is not a new chapter. |
| coop-runtime boss_save_frame | With no saves, the boss autosave was refused and retried every 2 s for 30 s (log noise). | Skipped once with one log line. |

Checked and fine: entvars/entity_state offsets (CoF adds 4 bytes after light_level, so v_angle 0x74 and health 0x164 are right), glow shell 19 drawn by the client, Book Simon precaches only his model (we precache it), keeps the health keyvalue and gets AI from DispatchSpawn like a monstermaker's monster; his `message` damage relay finds nothing with an empty name; the game's own game over covers a solo host who goes down; the cof_reducedlight defaults (0.23 0.23 0.26, 60); installer upgrade/uninstall paths and Linux parity; launcher version compare.

## Engine use

No detours and no new calls into game code: engine functions, the entity API exports (Spawn, KeyValue, Use as before), entity variables, two known player fields (dodge flag, respawn timers) and messages. Options are read every 0.5 s; storycoop_server.cfg runs at each map start.
