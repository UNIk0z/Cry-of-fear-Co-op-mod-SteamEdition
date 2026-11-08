# Cry of Fear co-op v0.10.3 (2026-11-08): one of each pickup, doors wait for the team

v0.10.2 plus Katz's two playtest requests of 2026-11-08. Maps are byte-identical to v0.10.2; the adapter, the Create Server options (sc_setup.scr), storycoop_server.cfg (one new commented line), the installers' check for an old Create Server file, the launcher and the READ ME version changed. Native and Node tests pass. Nothing here has run in the game.

| Report | Cause found | What changes |
|---|---|---|
| "Why there is 4 syringes in each syringe spawn and they are apparently separate for each player ... should be returned to just 1 item" | Two things. The maps themselves carry up to three extra copies of every plain ammo and weapon pickup, in a small square beside it (an early co-op map build, 1288 copies on the 194 maps). And since v0.8 each of those was one item per player, shown to and taken by that player only. So every spawn had four syringes for each player. | When a map starts, the extra copies are removed, and nothing is copied per player any more. Every pickup is one shared item again, as in single player: whoever takes it first gets it. The maps were not rebuilt. |
| (same) | The phone battery was one per player too, and since v0.10.2 the phone dies for every player. With one battery, only one phone could ever work again. | The one phone battery of the campaign (s4_subway2st1) is handled like a key: whoever picks it up, every player gets one in the inventory, and using it does not take the others'. Tapes, the video tape, the Glock light and the night vision are one per spawn, for whoever takes them. Keys and fuses stay shared by the team as before. |
| "the player that runs ahead of you can just go through the map change door even though you're a corridor away ... bring back wait for players to be ready, and make it a setting for the host ... on by default, and also make sure all the needed doors have it in all levels" | v0.10.2 opened the game's co-op doors for anyone. Also, 71 doors that change the map never waited, not even before v0.10.2: locked, named or special doors (and the ending door) that the map build left as plain doors. Such a door moves only its user into the level-change trigger, and the map changes for everybody. | New host option **"Doors that change the map wait for everyone"** (`sc_doorwait`, on by default), in the Create Server window and storycoop_server.cfg. On: every door that leads to another map opens only when every living player stands at it; the others get "Please wait until everyone is ready!". A downed player is not waited for, and a player alone never waits. Off: v0.10.2's behaviour, the door opens for whoever uses it. |

Coverage (scripts/audit-v0103.mjs over all 194 served maps, docs/V0103_STATUS.md): 298 game co-op doors (102 different ones) wait through the game's own ready zone again; 203 plain doors that change the map (71 different ones, the ending door included) wait through the adapter, with the same zone around the door. Locked doors keep the game's own locked message and key. Walking into a level-change area (no door) and scripted level changes after a scene are not changed.

Kept from v0.10.2: the map-change loop fix (an exit a player arrives in stays shut for him until he has left it), the Carcass crash fix, the key inside a door, fuse sockets, the phone dying for everyone, story teleport facing, sc_noteleports and the RebuiltSimon guard. The v0.10.2 change that stopped pickup copies pushing players went away with the copies.

Not changed in this build:

- "Changing hands doesn't work" still needs the exact steps.
- Ammo dropping to give teammates is a new feature and is not in this build.

The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md (steps 34-40 cover this build).
