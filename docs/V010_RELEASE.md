# Cry of Fear co-op v0.10 (2026-11-06): host options and team tools

v0.9 with Codex's spray fixes (the v0.9 Stability Fix build) plus the rest of the list in docs/COOP_QOL_PROPOSAL.md. Maps byte-identical; adapter, launcher, install files. Native tests pass. Nothing here has run in the game. Per-item status and what was left out: docs/V010_STATUS.md.

Everything that changes the game is a host option and is off unless the host turns it on (Create Server window, second page, or `cryoffear\storycoop_server.cfg`).

| Feature | What it does |
|---|---|
| Run timer (host) | `sc_timer` 1: 7 s after each chapter title card everyone sees "<chapter>: run 0:42:13 (last chapter 0:12:05)", and the total at the credits. 2: also a clock at the top right. Counts story-map time only; a save keeps it (slotN.cofrun beside the save). A new game or a chapter start begins at 0. |
| Outline far teammates (host) | `sc_outline 1`: a teammate more than about 10 m away glows in his colour. Not during cutscenes, not on a downed player. |
| Harder with more players (host) | `sc_scale 1`: monster health x1.35 for 2 players, x1.70 for 3, x2.05 for 4. Bosses and the hunted Simon keep theirs. Alone: no change. |
| Hunted (host, experimental) | `sc_hunted 1`: about 2.5 min into a story map, Book Simon with the sledgehammer appears behind the team, at a spot it walked through earlier, far away and out of sight ("Something is following you."). He has 60 health. He leaves when a cutscene starts or if he does not catch up within 3 min, and comes back 4 min later. Never on the opening maps, with a boss alive, or while players load. |
| Hardcore (host) | "No saves" (`sv_coop_nosave 1`, the game's own setting) and "No respawn while down" (`sc_norespawn 1`: a downed player waits for a revive; with everyone down the game's own game over restarts the map; a player left down with nobody alive to revive him gets the normal respawn back). |
| Morphine trip time (host) | `sv_morphinetriptime` (the game's own setting, default 35 s). |
| No hint pop-ups (host) | `sv_disablehints 1` (the game's own setting) also turns off the co-op objective reminder and the "Press K" prompt. |
| Ping (G) | Marks the spot you look at with a glow; everyone reads "<name>: here (12 m)" in your colour. Once every 2 s. |
| Scare (J) | Plays one of the game's laughs at the teammate closest to you. Once every 20 s. |
| Your colour | SETTINGS in the launcher (or `sc_color 0-8`): the colour of your ping and your outline. Automatic gives players 1-4 red, blue, green and yellow. |
| Black and white | SETTINGS (or `sc_noir 1`): your own picture is black and white, on every map. Only your game. |
| Light with no torch | SETTINGS: colour (game, warm, cold, brighter) and size of the faint light around you when no torch is on. Only your game. |
| Dodge choice kept | `dodgetoggle` used to switch back on at every map; your choice now stays. |
| Late joiners catch up | A player who joins a game in progress for the first time gets the weapons the host carries (as many as fit) and is put beside the host if far away. |
| Vote kick | The game's own: type `votekick <name>` in the console (~); the others press F1 (yes) or F2 (no). `sv_votekick_time` in storycoop_server.cfg. |
| Launcher 0.10 | SETTINGS has the ping and scare keys, colour, black and white and the light. A settings file you never changed is updated once (the old one is kept as .storycoop-old). |
