# Cry of Fear co-op v0.10.3 (2026-11-09): one of each pickup per player, doors wait for the team

v0.10.2 plus Katz's two playtest requests of 2026-11-08 (the pickups clarified on 2026-11-09). Maps are byte-identical to v0.10.2; the adapter, the Create Server options (sc_setup.scr), storycoop_server.cfg (one new commented line), the installers' check for an old Create Server file, the launcher and the READ ME version changed. Native and Node tests pass. Nothing here has run in the game.

| Report | Cause found | What changes |
|---|---|---|
| "Why there is 4 syringes in each syringe spawn and they are apparently separate for each player"; each player must have his own copy of every item, taken on his own screen, as in Serious Sam (Katz, 2026-11-09) | The maps themselves carry up to three extra copies of every plain ammo and weapon pickup, in a small square beside it (an early co-op map build, 1288 copies on the 194 maps). Since v0.8 every pickup is one item per player, and those extra copies were copied per player too: four of everything for every player. | When a map starts, the maps' extra copies are removed before the copies per player are made, and they are never copied. Each player now sees one item at each spot, his own: what he takes is gone on his screen only, and the others still take theirs. The maps were not rebuilt. |
| "the player that runs ahead of you can just go through the map change door even though you're a corridor away ... bring back wait for players to be ready, and make it a setting for the host ... on by default, and also make sure all the needed doors have it in all levels" | v0.10.2 opened the game's co-op doors for anyone. Also, 71 doors that change the map never waited, not even before v0.10.2: locked, named or special doors (and the ending door) that the map build left as plain doors. Such a door moves only its user into the level-change trigger, and the map changes for everybody. | New host option **"Doors that change the map wait for everyone"** (`sc_doorwait`, on by default), in the Create Server window and storycoop_server.cfg. On: every door that leads to another map opens only when every living player stands at it; the others get "Please wait until everyone is ready!". A downed player is not waited for, and a player alone never waits. Off: v0.10.2's behaviour, the door opens for whoever uses it. |

Coverage (scripts/audit-v0103.mjs over all 194 served maps, docs/V0103_STATUS.md): 298 game co-op doors (102 different ones) wait through the game's own ready zone again; 203 plain doors that change the map (71 different ones, the ending door included) wait through the adapter, with the same zone around the door. Locked doors keep the game's own locked message and key. Walking into a level-change area (no door) and scripted level changes after a scene are not changed.

Kept from v0.10.2: the map-change loop fix (an exit a player arrives in stays shut for him until he has left it), the Carcass crash fix, pickup copies that push nobody, the key inside a door, fuse sockets, the phone dying for everyone, story teleport facing, sc_noteleports and the RebuiltSimon guard. Keys and fuses stay shared by the team.

Not changed in this build:

- "Changing hands doesn't work" still needs the exact steps.
- Ammo dropping to give teammates is a new feature and is not in this build.

The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md (steps 34-39 cover this build).

The first v0.10.3 build of 2026-11-08 (outputs/superseded/v0103-a, never to be installed) had also removed the copies per player, so a pickup was shared by the team, and it made the phone battery a team item. Katz meant one copy for each player; that build is superseded.
