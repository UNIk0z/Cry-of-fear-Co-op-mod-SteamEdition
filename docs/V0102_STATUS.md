# v0.10.2 status (2026-11-07, Claude)

Packed from commit 4abe186 in this worktree (first build 9af2134, kept in outputs/superseded/v0102-a). Against v0.10.1 only the launcher, READ ME, storycoop_server.cfg, find-cof.ps1 (coop-linux.py on Linux), the adapter, release.json and known-builds.json differ. An LF export was tried first, but it changed the line endings of 13 unchanged files, so it was discarded. storycoop_comfort.cfg, storycoop_hints.txt and storycoop_server.cfg are LF in the working tree, as v0.10.1 shipped them. Nothing has run in the game.

## Evidence used

- Videos 1-6 (frames read through a local range server and a browser):
  - Clip 1 is the apartment corridor at a co-op door.
  - Clip 3 shows the player pushed at a tape recorder (RebuiltSimon speed 30-200 with no input).
  - Clip 4 is a black ladder screen ("I can't use this while Joah230 is using it"), then an arrival in the street facing a wall.
  - Clip 5 shows "Please wait until everyone is ready!", a dark area, then back in the street.
- The second player's logs (`logs/`, 2026-10-31 to 11-05):
  - The only CRASH is s4_roofs2_e0 at hl.dll+0xC8706 (touch roofboss_head, after the multikill2 skip).
  - The fuse sequence is in rc4-20261103-223623.log.
  - RebuiltSimon 2.5.1 is loaded through CoF Manager (qconsole-20261103).
- The host logs of the 2026-11-06 session were not supplied.

## Fixes and where they are

| # | Fix | Code | Fixture (test-qol2.cpp, v0.10.2 block) |
|---|---|---|---|
| 1 | Co-op level-change doors open for anyone | qol2 `door_ready_after_think`, adapter `think` | iuser1 set on trigger_coop_changelevel only |
| 2 | Ownerless touch (Carcass head) | qol2 `orphan_owner_touch`, adapter `dispatch_touch` | owner kept while alive, world once freed |
| 3 | Pickup copies push nobody | coop `pickup_group`, qol2 `pickup_unblocking`, adapter `full_pack` | groupinfo bit, state solid 0 |
| 4 | Key inside a door | qol2 `use_trace_redirect`/`trace_line` (engine slot 31, only in PreThink) | door to key, world untouched, carried key ignored |
| 5 | Phone dies for everyone | qol2 `phone_frame` | cof_phonedisable repeated with acting_override; a battery fixes one phone |
| 6 | Fuse sockets | coop `card_locks_left` in `team_cards_frame` | m_isLocked +0x180 (hl.dll save table 102116C0), removed socket |
| 7 | Story teleport facing | coop `move_beside_yaw`, `story_gather(...,yaw)`, StoryAnchor/ClimbFollow yaw | destination yaw vs leader's |
| 8 | sc_noteleports | qol2 `teleports_off`, coop scene/story/late moves | nobody gathered with it on |
| 9 | RebuiltSimon | qol2 `rs_guard` (boot, every map end) | not in a fixture (needs hw.dll); hw.dll SHA = SprayHwSha, 0250E07C written only by SV_ExecuteClientMessage and the bot path (4 writes) |

## RebuiltSimon (github.com/cd-n0/RebuiltSimon, HEAD fca8c87, read only)

- `Features/Cheats/disable_stamina.c` runs from its HUD_Frame hook whenever the client is in game. It writes `((simon_t*)(*gsv_player)->pvPrivateData)->infinite_stamina`, and it checks only the address of sv_player, never its value.
- hw.dll's sv_player (0250E07C) is set in SV_ExecuteClientMessage and never cleared. Host_ShutdownServer frees all private data (host.c 778), and the edict array is hunk memory that the next map reuses.
- So on a joining player's game the write goes through freed or reused memory, or through NULL when the game started straight into a server. That matches the recorded crash in RebuiltSimonCoFManager.dll+0x3C6B right after connecting (docs/V08_STATUS.md). The binary was not available, so the offset itself is not mapped.
- Other hooks checked and left alone:
  - Host_ValidSave: off by default (`rs_cof_save_prevention 1`).
  - PM_Unduck / PM_PreventMegaBunnyJumping: hl.dll signatures, our backend is the stock hl.dll.
  - The NOP at hl.dll+0xEB203 (noclip in PlayerDeathThink).
  - noclip, god and notarget are client commands forwarded to the server.
  - None of the adapter's client patches overlap the hooked prologues (patch_apply refuses unexpected bytes).
- Not fixed: `get_stamina()` (HUD stamina) reads a pointer at hw.dll+0x7E0544 and caches it. On a joining player the number it shows may be stale. It is a read, not a write.

## Virus scan (Windows Defender, MpCmdRun -Scan -ScanType 3 -DisableRemediation)

Signatures 1.463.176.0 (updated 2026-11-07 10:52), engine 1.1.26100.4, real-time protection on. The packages were scanned again after each repack, the last time after the harbour loop rebuild (commit 4abe186).

| File | Result |
|---|---|
| outputs/Cry of Fear Co-op v0.10.2.zip | no threats |
| outputs/Cry of Fear Co-op v0.10.2 Linux.tar.gz | no threats |
| outputs/coop-v0102-player (folder: launcher exe, scripts, adapter) | no threats |
| outputs/coop-v0102-linux (folder) | no threats |
| outputs/v0102-candidate (folder) | no threats |
| work/claude-s47-native-004/storycoop_rc4.dll (the shipped adapter) | no threats |
| outputs/Cry of Fear Co-op v0.10.1.zip (for comparison) | no threats |

`Trojan:Win32/Bearfoos.A!ml` is a machine-learning verdict, not a signature. It usually comes from cloud protection when a downloaded file is first opened. Likely triggers in this mod:

- The launcher is unsigned, new and low-prevalence, and every build has a new hash.
- It downloads a ZIP from GitHub, extracts it, and starts `powershell -ExecutionPolicy Bypass -File ...` with no window (CoopLauncher.cs 508-511).
- It writes into another program's folder.
- The adapter patches code in client.dll and hw.dll memory (VirtualProtect), hashes other DLLs and writes minidumps.

A clean local scan does not guarantee no flag on a tester's PC. The lasting fixes are a code-signing certificate, or submitting the exe and the DLL as a false positive at https://www.microsoft.com/wdsi/filesubmission (Katz).

## Open

- Clip 5 map-change loop: fixed from the second player's log of 2026-11-03 (s4_hamn2_e0 <-> s4_hamn3_e1; the co-op spawns stand in trigger_once *51). Code: coop-runtime `arrival_touch_blocked`, `exitsArrivedIn`; fixture in the v0.10.2 block. Video 5 itself has no host log, so it is the same symptom, not proven to be the same spot.
- "Changing hands doesn't work" means the weapon hand (Katz). Not found:
  - CoF has no hand-switch command.
  - The dual wield order works through the adapter (DualWield A,B puts A left and B right; a light always goes right, GiveDualWeapon).
  - The one dual wield in the logs (host, slots 4,3 = M16 + Glock) is refused by the game itself, because the M16 cannot be dual wielded.
  - Needs the exact steps.
- Ammo dropping to give teammates: a new feature, not done.

`setup-coop -CheckOnly` on a v0.9 install (nothing changed) gives the same plan as v0.10.1: "Found co-op v0.9. Updating it to v0.10.2", 248 files current, set aside cl_dlls\storycoop_rc4.dll and txtfiles\storycoop_hints.txt.
