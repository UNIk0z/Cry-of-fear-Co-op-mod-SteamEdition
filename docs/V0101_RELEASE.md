# Cry of Fear co-op v0.10.1 (2026-11-06): review fixes

v0.10 plus the fixes of a full-code review (docs/V0101_REVIEW.md) and a boss review (docs/BOSS_REVIEW.md). Maps and settings files byte-identical to v0.10; adapter and launcher changed. Native tests pass. Nothing here has run in the game. The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md.

| Fix | What changes |
|---|---|
| No saves | With "Hardcore: no saves" on, the host's tape recorder says "Could not save: saving is off on this server" instead of opening the save window and playing "Saving... Saved" for a save that never happened. |
| Rejoin on the same map | A player who rejoins (the crash-rejoin offer) on the map he left gets his network tuning again (smooth movement for the others) and is shown with his own model, hands, subtitle language, colour and black and white at once, not the last player's in that slot. |
| Boss fights start with their health bar | Every boss stands in its map from the start (waiting for its scene), so a death or a fall anywhere on a boss map was a game over for everyone, long before the fight; on the Chapter 3 start map Mace stays there for good. The "a player died in the boss fight" game over now begins when the fight's health bar appears; a player already down then does not end it, and falls before it respawn as anywhere else. |
| Boss bar after a game over | A game over loads the fight's autosave, and the health bar never came back. The fight now goes on after any load with its bar (and with its game-over rule). |
| Boss bar for late players | A player who joins or rejoins during a fight gets the health bar. |
| Harder with more players | The counters the ending fights show in their bars (endings 1-4) keep their health, so those bars empty as in single player. |
| Hunted | Off on every map with a boss fight (ending 4's Book Simons are hidden until they appear, so Hunted could come there). |

Per-boss notes: docs/BOSS_REVIEW.md.
