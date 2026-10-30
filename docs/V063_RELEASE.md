# Cry of Fear co-op v0.6.3 (2026-10-30)

v0.6.2 plus the Playtest 3 fixes and the first v0.7 items. Maps byte-identical;
adapter, Create Server options and installer only. Native tests pass; **not
run in the game.**

| Report (clip) | Cause (evidence) | Change |
|---|---|---|
| Host crash after the train crash (log, crash-20261028-214041.dmp) | `CTmp::Deploy` with no owner: a queued weapon switch (player +0x2538) kept a weapon that was taken away or dropped (hl.dll 100EDFB4). | A queued or last weapon the player no longer owns is taken off the switch (`WEAPON` log line). |
| Partner crash when joining (crash-20261028-204940.dmp) | The connect ran while the closing join box still played a sound from the menu server (S_StartDynamicSound). | The connect waits 0.6 s. |
| Ladder (1) | Stale climber slots made neighbouring ladders look busy; the E press never reached the old queue. | Every player can climb at the same time: a busy ladder hands the next climber their own copy of its manager (`LADDER ... copy made`). |
| Skip: voices, zoom (4); lever door closed again (11) | The skip fired every remaining target at once, including the lever's 41 s door close and the scene's voice lines; the zoom returned seconds later. | A skip jumps to the end of the scene: later timers keep their time, the scene's voices stop and the zoom returns at once. |
| Roof fall stays black (3) | `fadefall` (stay faded out, for everyone) on s4_roofs. | That fade only covers the faller; the faller gets a reveal when they are back. |
| Invisible partner after doors (5, 7) | The game hides a player going through a door (`b_GoingThroughADoor`) and never shows them again. | Shown again when the door ends (`DOORHIDE`). |
| Map change loop (14), wrong arrival (9) | Arrivals were put beside the landmark, inside the trigger back (s4_down), or on a landmark 512 units off (college -> harbour); one arrival was dropped. | Arrivals as in single player (position relative to the landmark), never inside a level-change trigger. |
| Story-locked door | The v0.6 rule needed a touch within one second; it never fired. | When a story lock closes a door, or a player presses E on it, players left outside are moved to the partner beyond it (`DOORLOCK`). |
| Train (17), boat (19) | Landing beside the escaped player put partners on the train; the far shore was within the boat's 400-unit radius. | Train landing only on the world floor; boat brings everyone more than 128 units away. |
| Two doctor's pistols (18) | Pickup copies included the P345. | P345 (and story weapons) never copied. |
| Collision on but players pass through | The game makes touching players non-solid. | With collision on, players block each other (as in Counter-Strike). |
| Red "Picked up" text stays | The client's message strip sticks at a level change. | Cleared after each HUD reset. |
| Fuse (15) | One quest item per team (Katz). | Others are told who picked it up ("Joah230 picked up the fuse"). |
| Credits | | Joah230. |

v0.7 items: the Doctor's shot lowers max health for every player; "Unlock
extras" in the Create Server window opens the Chapter 1 secret room with every
unlockable (wardrobe removed); the host's recorder opens the single-player save
window with five co-op slots (labels in SAVE/coopinfoN.cof, single-player saves
untouched), Load-Save-Slot-4 and 5 in Create Server.

Still open: elevator scene, crawler, invincible enemies, crouching on a save
station, a door opened from the other side, player model menu.
