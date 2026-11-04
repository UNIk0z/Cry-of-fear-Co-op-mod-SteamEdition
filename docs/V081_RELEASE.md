# Cry of Fear co-op v0.8.1 (2026-11-04)

v0.8 plus the fixes from a review of the whole source. Maps byte-identical; adapter and launcher. Native tests pass. Nothing here has run in the game. Details: docs/V08_STATUS.md, "Code review 2026-11-04".

| Problem | Change |
|---|---|
| Two players picking up the map's two fuses at the same moment ended up with one fuse between them | Each pickup counts: both end up with two. |
| A partner who had no room for the second fuse lost both when the other player used one, leaving a socket with no fuse | A use takes only as many as were used. |
| A key dropped and picked up again gave everyone a second copy | The same key is counted once. |
| After a fall respawn or a rejoin, a player did not get the key items the others found meanwhile | He gets them once he is back in. |
| Moving a half (number plate, scissors) to the partner could take the half from both players | A half move is neither a pickup nor a use. |
| After quitting while a partner was still loading, the next game froze a lone host for 60 s with every trigger shut | A new game and the main menu clear that wait. |
| A pickup whose copy for a slot could not be made (entity limit) was missing for that player | The map's own one is then shared, first come first served. |
| A player on a ladder when a story teleport or a scene end moved the others was put back at the ladder's end | He stays on the ladder and is moved to the team once his climb is over. |
| Launcher update took the Linux package | The launcher never takes the Linux file, and the Linux package is a .tar.gz. |
| Launcher update failed once after a download was cut off | The leftover file is overwritten. |