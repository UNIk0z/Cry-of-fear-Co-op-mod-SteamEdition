# Ladder audit for v0.7.2 (Claude, 2026-10-31)

Tester report: "if player 2 uses a ladder there is a softlock potential". No log came with it, so the whole ladder path was read against the decompile (`src/dlls/CoF_Ladder.cpp`, `subs.cpp`, `multiplay_gamerules.cpp`, `effects.cpp`, `triggers.cpp`, `cl_dll/input.cpp`, `hud_msg.cpp`) and the pinned hl.dll. Adapter only: `native/rc4/ladder-runtime.hpp`, two calls in `adapter.cpp`. Native fixtures pass. Not packaged, not run in game.

## Fixed

| Case | Cause | Change |
|---|---|---|
| **Softlock.** One player climbs, another presses E on the same ladder: the first stays frozen on the camera for good. | The busy ladder hands out a copy, but `FireTargets` repeats the name lookup after the entity it just fired. The engine reuses the lowest free edict, so a copy can sit before the original. That next lookup found the busy original, and its Use took the climber slot. | The lookup after a copy now ends there. |
| **Softlock.** A player who joins or respawns while anyone is on a ladder gets a black screen, no controls and "Please wait until cutscene has finished." forever. | `CHalfLifeMultiplay::PlayerSpawn` (100C948D) treats any player's camera (+0x5AC) as a cutscene and sets `m_bCoopWaitingForCutsceneToEnd` (+0x3C8). Only the end of a `cof_mdlcutscene` clears it. | `ladder_wait_frame` runs before that player's first think and clears the flag when only climbs hold a camera. A real scene keeps the native wait. |
| **Softlock.** A scene removes a climber's camera and does not take control: frozen for good. `trigger_camera` flag 2048 does this. | The ladder's freeze had no owner left, and the adapter had already cleared the climber slot. | The seat stops its think and queues the player. Once no scene holds them, frozen and notarget come off (`LADDER player=N has controls back`). |
| A `cof_mdlcutscene` starts during a climb. | It replaces the climber's camera. LadderThink kept running on the new one: W stepped the scene's camera and the last step deleted it. The old camera stayed in the world. | The seat stops as soon as the camera is no longer its own, and removes the old camera. |
| Godmode stayed on after any of those scenes. | Only LadderThink's normal end clears it. | Cleared when the climb is ended from outside. |
| A scene's end unfroze a climber mid-climb. | `EnableControl(TRUE)` goes to every player. | A climber is frozen again each frame while the climb camera is theirs. |
| The next climb ended at once, or its camera did not rise. | The manager keeps per-climb bytes: +0xDD, +0x121, +0x1A8 (a native bug, solo too: get on at the top, go back off, climb from the bottom). | Each climb starts clean (`ladder_fresh`). Offsets read from the disassembly at 1004ED40-1004FE80. |
| A climber disconnects: a frozen climb model stays on the ladder until the map changes. | Nothing removed the camera. | `ladder_left` from the disconnect hook frees the seat and removes the camera. |

## Read and left alone

| Case | Finding |
|---|---|
| Host alone | The lookup finds a free ladder and the game's own climb runs. New code only cleans the manager bytes and holds the freeze during the climb. |
| Host or joiner climbing | Same server path for both. The client sends `forwardpressed`/`backpressed` while attached to a camera; the adapter's command hook passes them on. A key held before the camera attaches sends nothing until pressed again, in the base game too. |
| Two players pressing E | The use brush has a 1 s lockout (`m_flNextUse`), so the second press inside that second does nothing and works when repeated. |
| Mount | The base game refuses a player who is not on the ground, or who holds a lit flare. |
| Dismount onto another player | The player is placed without a check. CoF keeps a player non-solid while their box overlaps another's, including with collision on (`collision_hold`), so they separate. |
| Map change during a climb | Player memory is rebuilt, the carry record holds no flags, and the client clears its camera in `VidInit`. A saved manager restores with no climber because the player is not in the save table, and `ladder_fresh` covers its leftover bytes. |
| Save during a climb | The climber's partner saves with their last good record (`carry_record`). A frozen player cannot open the recorder. Climb cameras and copies are not in the map state. |
| Death on a ladder | Godmode blocks damage. A respawned player keeps the climb camera and can finish the climb. |

## Test

Nothing here has run in the game. Host log lines start with `[STORYCOOP_RC4] LADDER`.

1. Solo: climb up, climb down, get on at the top and go back off, then climb from the bottom. The camera rises.
2. P1 climbs and P2 presses E on the same ladder: `copy 1 made`, both climb, both reach the end. Swap host and joiner.
3. One climbs up while the other climbs down the same ladder.
4. P1 stays on a ladder while P2 joins, and again while P2 respawns: P2 plays on with `not held by the cutscene wait`.
5. P1 stays on a ladder while P2 starts a scene: `ended from outside`, then `has controls back` after the scene. P1 can climb again and no climb model stays behind.
6. P2 disconnects on a ladder: the model goes and the ladder works for P1.
7. P1 stays on a ladder while P2 walks into a level change: both arrive and can move.
