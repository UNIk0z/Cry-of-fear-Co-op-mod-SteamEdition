# Cry of Fear co-op v0.9 (2026-11-06): the QoL pack

v0.8.1 plus the quality-of-life list Katz picked (docs/COOP_QOL_PROPOSAL.md). Maps byte-identical; adapter, launcher, install files. Native tests pass. Nothing here has run in the game. Per-item status: docs/V09_STATUS.md.

| Feature | What it does |
|---|---|
| Unstuck (K) | Inside a wall or a prop: out to the nearest spot where you fit. Otherwise: next to the nearest partner. Alone: to a clear spot nearby. Once every 10 s; not while down, in a cutscene, on a ladder or while players load. The game's own key prompt says "Stuck? Press K to get free" once per game. |
| Revive options | The Create Server window has "Revive time (seconds)" (default 5) and "Auto respawn while down (seconds)" (default 384), the game's own settings. |
| Server settings file | `cryoffear\storycoop_server.cfg` on the host runs at the start of every story map. Every line starts switched off (`//`). |
| "<player> is down" | The other players see it in the game's white notice text. |
| Revive groan | A revived player groans or gasps in his own voice as he gets up (Simon's voice, or the police voice for police models). |
| Objective reminder | A player who joins a running map, respawns or uses unstuck sees the current objective at the top of the screen. B still shows it any time. |
| Team list | The game's own co-op list (name, health, ping, syringe icon) shows on every story map and hides while a cutscene holds you. It adds what a player is busy with: (phone), (reading), (bag), (puzzle). |
| Comfort settings | SETTINGS in the launcher: field of view, head bob (off, low, default) and the keys for unstuck, HUD on/off and spray. |
| HUD key (H) | Hides the whole HUD and the co-op texts for screenshots; press again to bring it back. |
| Subtitles in your language | Each player reads cutscene subtitles and objectives in the language set in his own game. A joiner's language choice no longer changes the host's. |
| Spray | SPRAY in the launcher turns a picture into your spray logo. T sprays it in the game. Linux: `bash coop.sh spray picture.png`. |
| Rejoin | If the game closes in a crash while you are joined to someone, and that host still answers, the launcher asks "Rejoin <host>?". |
