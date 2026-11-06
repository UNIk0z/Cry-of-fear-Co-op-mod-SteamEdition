# Cry of Fear co-op v0.10.1 (2026-11-06): review fixes

v0.10 plus the fixes of a full-code review (docs/V0101_REVIEW.md). Maps and settings files byte-identical to v0.10; adapter and launcher changed. Native tests pass. Nothing here has run in the game. The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md.

| Fix | What changes |
|---|---|
| No saves | With "Hardcore: no saves" on, the host's tape recorder says "Could not save: saving is off on this server" instead of opening the save window and playing "Saving... Saved" for a save that never happened. |
| Rejoin on the same map | A player who rejoins (the crash-rejoin offer) on the map he left gets his network tuning again (smooth movement for the others) and is shown with his own model, hands, subtitle language, colour and black and white at once, not the last player's in that slot. |
