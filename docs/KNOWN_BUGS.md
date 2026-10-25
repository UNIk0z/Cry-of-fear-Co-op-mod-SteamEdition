# Known bugs entrypoint

Canonical issue history: [BUG_TRACKER.md](BUG_TRACKER.md), newest section first.

**v0.5 (2026-10-23, Claude):** item locks missing in co-op (spawnflags 2048), the park level-change loop, the s4_start black box for the second player and the solo prologue are fixed in code (not run in game). Still open: Sawer first encounter/M4 (A5), bridge carriage (B3), two apartment exits (B5), leaving the ending house (B6). RebuiltSimon add-on crashes are not ours (V05_CHECKLIST RC-ADDON).

**v0.4 (2026-10-18, Claude):** the revisit/load crash below is FIXED (root cause: map state removed fire-once trigger_autos before ServerActivate; their Activate queued UTIL_DesiredAction and the engine freed them while still in Spirit's assist list; first PlayerPostThink called freed memory, EIP 0x70; Steam's crash handler then hung the game). P2 dark box after s4_start_e0 multiscene3 FIXED (exit teleport in the frame the camera ends was not followed). Details: docs/V04_AUDIT_HANDOFF.md. Open: two-player runtime verification.

**Revisit crash (tester, 2026-10-17, NOT reproduced locally):** every run ends right after the host's player enters a map revisited through another alias (s4_start2 -> s4_start_e0; earlier s4_apartment1_e0 -> s4_start_e2), after "MAPSTATE restored". No error text. The 21:00 host log still lists "rt:#1", which only builds before a6a70087 can write, so that host ran an older adapter. Hardening in f353e88d: load-time ("rt:") entities are matched only when the alias creates the same set (layout CRC per visit, cofmap v2), so a wrong entity is never restored or freed. Crash report: an unhandled exception now writes "[STORYCOOP_RC4] CRASH code=... at module+0x... map=... last=[...] stack=[...]" and storycoop_logs/crash-*.dmp. Also fixed: post-process effects on revisited maps (restored "X__sc4pp" name). Ruled out: identity mismatch between s4_start and s4_start_e0 (1143 identical, 0 conflicting), function-name save (adapter wraps Name/FunctionFromName), journal on arrival (worked in 013-era logs), CSoundEnt kill (always rt:#0).

**v0.3 level-change audit (2026-10-17, Claude, gameplay NOT verified):** fixed (1) session state (kept maps, carried inventories, failed-restore flag) leaked into a game not started from the mod's New Game/Load in the same process: c_game_menu1 now resets it, keeping a prepared Continue (adapter.cpp menu_reset); (2) a player not yet restored from a loaded save lost the save's inventory if the map was left first: the save's records seed the carry (carry_seed); (3) a classless runtime entity made slotN.cofmap unreadable: skipped at load, dropped when decoding older files (state-runtime.hpp). Still open: the tester crash at the s4_apartment1_e0 exit (trigger_once *204 -> cof_changelevel sc_level_643 -> native SetPlayerVariables; no CARRY line, and ServerDeactivate ran, so an engine error rather than a hard fault): need the tester's qconsole.log / error text.

**v0.2 playtest (2026-10-16), fixes in v0.3, gameplay NOT verified:** "Cannot overwrite savefile" (uppercase Videotape card failed validation), rope/tape/pickups/enemies/triggers reset on revisit (maps loaded fresh; now per-map DispatchSave/Restore state), Drowned always attacked player 1 (INDEXENT(1)). Open: movewith groups and transition entities keep served state; v0.2 saves have no map state (rope re-offer rule only). See docs/V03_RELEASE.md.

**v0.1 playtest (2026-10-16), fixes in v0.2, gameplay NOT verified:** staged cutscene actors idle before scenes (map start chains never ran in co-op), elevator scene teleported one player only, converted doors silent (cvar only), P2 ladder hijack/jitter, recorder line on sight. Playtest 2 (2026-10-24, v0.5.5): see docs/V057_RELEASE.md for the fixed and still open items. Open: P2 idle camera jitter (IDLE_DIAG added; v0.5.6: partner cl_cmdrate 30 found and raised to 100, see docs/V056_RELEASE.md). See docs/V02_RELEASE.md.

**coop-fixes-013 P2 crash (2026-10-15, fixed in coop-fixes-014, gameplay NOT verified):**
P2 joined s4_nightmare while scene camera 271 was running and crashed:
access violation reading 0x60 at client.dll+0x5F5C8, in R_DrawBrushModel
(client 0x1005F570 per the decomp reference) with e->model == NULL. 013's full_pack
returned 0 for EVERY player while a client watched a trigger_camera, including that
client's own player. The client renderer (RendererUpdateEntityList) picks brush
entities whose messagenum equals the LOCAL player's, so a stale local messagenum
breaks the brush list. 014 sends players again and sets EF_NODRAW (0x80) at
entity_state+0x3C instead (hl.dll 10018A89/8F copies entvars effects there).
The same cause may explain the reported missing/invisible world objects during
scene cameras (Bug A, not confirmed). Installed maps all match 013 (194/194), and
the 012->013 entity diffs only add the key-door bridge entities and two env_global
resets. No model refs or render fields were lost.
Current candidate coop-fixes-001: [RC4_COOP_FIXES.md](RC4_COOP_FIXES.md). The ladder
softlock and the black box have confirmed root causes and fixes. The train,
saves and the code drift have hardening plus new evidence lines. P2 jitter is
diagnostics only.
Current candidate campaign-doors-001: [RC4_CAMPAIGN_DOORS.md](RC4_CAMPAIGN_DOORS.md) lists its
limits. Apartment return routes are not overlaid, converted locked buttons are
silent, and the native door visibility cause is still unknown.
Current candidate camera and note changes: [RC4_CAMERA_NOTES.md](RC4_CAMERA_NOTES.md).
Actual two-client camera recovery, note rendering and full-restart save restoration
remain pending. Preserve user-confirmed working doors, puzzle, sounds and notes.
