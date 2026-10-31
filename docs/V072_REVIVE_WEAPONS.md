# Revive and dropped weapons for v0.7.2 (Claude, 2026-10-31)

Katz passed on two entries from the changelog of a CoF co-op port on Xash3D and asked whether this mod has them. Both come from the game's own code, so they were checked against the decompile and the pinned hl.dll. Adapter only: `native/rc4/coop-runtime.hpp`. Native fixtures pass. Not packaged, not run in game.

| Their entry | Here | Cause | Change |
|---|---|---|---|
| Respawn during a revive: guns never fire again that life, monsters ignore you. | **Yes.** The fall respawn adds to it, because it puts the body beside the partner and respawns it 2 s later. The player's move keys stop too. | `RevivePlayer` (100F18FD) sets `pev->fuser3` to 3 for the get-up, and only `MultiplayerPlayerDeathThink` clears it. The player is still dead during that animation, so the jump key or the respawn timer can run first, and `Spawn` never writes `fuser3` (disassembly). Weapons need `fuser3 <= 0`. `FVisible` and `Look` hide a player at 3. The client drops the move keys while `vuser4.x`, a copy of `fuser3`, is set. | `respawn_mode_clear` runs each frame from `fall_frame`. An alive player at 3 goes to 0. |
| Same trap, not in their entry: a reviver killed during CPR. | **Yes**, from the code. | They respawn with `fuser3` 2 and lie on the floor with no weapons. | 2 goes to 0 on the first frame after a death. A knockdown keeps its 2. `m_bWaitingToFall` is cleared as in `RevivePlayer`. |
| Crash after dropping a weapon: a later switch grabs the one on the floor. | **Partly fixed already.** `deploy_guard` (v0.5.7) covered `m_pNextItem` and `m_pLastItem`. Two pointers were still open. | `m_pStoredNightVisionWeapon` (+0x3F0) is deployed with no check when night vision goes away, and `m_pStoredActionItem` (+0x253C) is read for its classname. A drop, `RemoveAllItems` and `Spawn` all leave them set, so a death with night vision on left a freed pointer too. | `deploy_guard` clears both when the weapon is no longer in the player's item lists. |
| Getting up after a knockdown. | **Behaviour fix.** It could not crash here, but it emptied the hand. | The get-up queues a re-deploy of `m_pNextItem`. With that weapon gone, the guard queued nothing, and the game then set the active item to nothing. | The queue now takes the weapon in hand if the player still owns it. |

As in their port, nobody has seen the stored-pointer crash happen here. It comes from reading the code.

## Test

Nothing here has run in the game. Host log lines: `REVIVE player=N respawned with revive mode 3 left over`, `WEAPON player=N: a stored weapon was no longer his`.

1. P1 dies and P2 revives. In the second or two of the get-up, P1 presses jump, with the respawn delay already over. P1 respawns, moves, fires, and monsters attack.
2. P1 is reviving P2 and a monster kills P1. P1 respawns on the timer, stands and fires.
3. Die with night vision on, respawn, and switch night vision off. No crash.
4. Night vision on, drop or lose the stored weapon through the inventory, switch night vision off. No crash, empty hands.
5. Queue a switch, drop that weapon, get knocked down and get up. The weapon in hand comes back.
