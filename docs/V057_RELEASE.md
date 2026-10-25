# Cry of Fear co-op v0.5.7 (2026-10-25)

v0.5.6 plus fixes from playtest 2 (31 clips, notes and logs in Katz's
"New folder", played on v0.5.5). Maps byte-identical; adapter only. Native
tests pass; **not run in the game, two players not tested.**

| Report (clip) | Cause (evidence) | Change |
|---|---|---|
| After a skip the camera stays in the cutscene, "Press R to skip" comes back, a snippet of the scene returns and stays (6, 7, 10, 11, 12, 17, 27, 29; 4 likely) | `CTriggerCamera::Use` only toggles its state (decomp triggers.cpp). The fast-forward had often switched the camera off already; the adapter then Used it again and switched it back ON for its full time. Logs: `SKIP ended dockam1 again` then `offered for dockam1` over and over; the same for menhorunge, hisskam2, kamera4, kamerahahe2. | A skipped camera is ended through its own end path: its return time is set to now, so the game's own think gives every player the view back and fires its targets. Never toggled. A camera already switching off is not offered again. Offsets from the hl.dll save table: wait 0x104, return time 0x108, state 0x124. |
| Doctor scene: not skippable, or only up to the choice (27) | The skip of `multits3` fired the yes/no at once (`DOCTOR weapon=1` followed). Both players' E/SPACE presses during the choice counted as skip votes (`SKIP vote (usercmd)`). | A scene that still has a yes/no choice to show is not offered, so the choice always appears; the scenes before and after it stay skippable. Only R counts as a skip vote now. |
| Hands "tweaking" after map changes (1) | After nearly every map change the held weapon was asked back every half second for 6 s (`EQUIP ... could not take ... back` on almost every map); the phone toggles up and down each time. | At most two asks, 1.5 s apart; the phone is never asked back. |
| Phone in the left hand (2) | Quick dual wield paired keys 1+2: phone + switchblade (`DUAL quick ... weapon_mobile+weapon_switchblade asked`). | The phone is never dual wielded. |
| Bag lost but inventory kept (25) | `bagremove` fires as s4_bridge starts, before any HUD is ready (`every player lost the bag (0 player(s))`), and the carried inventory was given back afterwards. | Every player on that map is cleared once their carried inventory has been applied (the game's own cof_clearitems, three slots). |
| Wheelchair for one player only (31) | Player 2 was still loading when `wmode` fired (`POSTPROCESS wmode applied to 1 player uses`). | The wheelchair stays on for the map; a player who arrives later gets one beside the others. |
| Train: the one who escapes leaves the other in the falling train | s4_bridge_e1: trigger `*109` -> `destroyall` (0.5 s) -> `multikuken2` drops the train. | When a player touches that trigger, everyone else is moved beside them before the train falls (`TRAIN` log line). |
| Invisible wall where the floor breaks (19, 30) | A start-off func_wall_toggle is switched on with the breaking floor: s4_down `spawnm2s` (multicome), s4_fsewer1 `blockstairs` (multistairs). | Those two walls are removed when the map starts (`FLOORWALL` log line), so the partner can drop down. |
| Sawer kills a player: no revive, crash; boss death should be a game over (9) | Host log 2026-10-23: `CRASH hl.dll+0xC7E8D` after a `third_person_interaction` think = `CHalfLifeMultiplay::DeathNotice`: the Sawer's grab kills with no inflictor and multiplayer's death notice reads NULL. | As soon as the grab starts, or a player dies while a boss is alive (Sawer, sewer boss, roof boss, Doctor, the book Simon), everyone sees a notice and the map restarts through the game-over path: this session's save of the map loads if there is one, otherwise everyone gets what they had on arrival. |
| Skip text over the health bars (screenshot) | Drawn at 86 % of the screen height. | Drawn at 72 %. |

Not fixed (no cause found from the clips and logs; details in the reply):
the doctor-door and "whole area" invisibility (22, 28), fuses in the subway
(24), arrival spots (16, 18, 20), the door that locks behind the partner and
the death after it (14, 15), the door locked from the other side (21), the
fall black screen (13), the black box (23), the missing elevator scene (8),
client frame rate (26), two players on one ladder (the game's ladder holds one
climber) and waiting for every player to load (a feature request).

The tester's game also runs a third-party mod: both client crashes on
2026-10-24 are in `RebuiltSimonCoFManager.dll`, not in the co-op adapter.
