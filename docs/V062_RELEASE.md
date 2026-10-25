# Cry of Fear co-op v0.6.2 (2026-10-25)

v0.6.1 plus two joining fixes from the testers. Maps byte-identical. Native
tests pass; **not run in the game.**

| Report | Cause (evidence) | Change |
|---|---|---|
| Join box does nothing; console: `Server tried to send invalid command:"connect 11.11.11.11:2701"` | The game refuses a `connect` that a server sends to its client, and the join box sent it that way from the player's own menu server. | The join box puts `connect <IP>:<port>` into the player's own console, as if typed. |
| "LAN servers are restricted to local clients (class C)" | The host's server ran with `sv_lan 1` (testers had to type `sv_lan 0`). | Hosting sets `sv_lan 0` every time (the adapter's restart into the story map and the older host launcher). |