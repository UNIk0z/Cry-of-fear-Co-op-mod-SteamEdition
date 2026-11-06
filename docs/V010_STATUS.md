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

## Engine use

No detours and no new calls into game code: engine functions, the entity API exports (Spawn, KeyValue, Use as before), entity variables, two known player fields (dodge flag, respawn timers) and messages. Options are read every 0.5 s; storycoop_server.cfg runs at each map start.
