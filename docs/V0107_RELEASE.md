# Cry of Fear co-op v0.10.7 (2026-11-14): code review fixes

v0.10.6 plus the fixes from a static code review of v0.10.6 (the handoff `HANDOFF_v0.10.7.md`, outside the repository). Maps are byte-identical to v0.10.6. **Nothing here has run in the game.** Every item is covered by a fixture that fails on v0.10.6 (`main` at `1382fd2`) and passes now.

| Item | Cause | Change |
|---|---|---|
| F1. Players carried through a transit teleport stood inside each other | v0.10.6 `transit_touch` calls the teleport's own Use for each other player; CCofTeleport::Use puts every activator exactly on its target | Each carried player is moved to a free spot beside the first one, outside the triggers that lead out, with the teleport's yaw |
| F2. Windows Defender taking the installed adapter gave no steps | The v0.10.6 guard covered blocked reads of package files only | Blocked writes, an adapter gone after Install, at Run Co-op Mod, at Join and in the update check give the same steps; the steps name both folders to exclude |
| F3. A blocked optional file turned a later error into a "Defender blocked" report | The block mark stayed set after an optional file was skipped | The mark is cleared when an optional file is skipped, with one plain line |
| F4. A second level change in the same frame overwrote the arrival and the load wait | The engine runs only the first ChangeLevel on a map; the adapter's hook counted every call | Only the first call is counted until the next map; player triggers are shut for 1 s after it |
| F5. A movewith group with an incomplete save record | A child restored without its parent's record gets `m_activated` and a null `m_pMoveWith`, and never rides its train again | Such a group stays as served whole, with a log line |
| Also: Get-FileHash dropped the antivirus code | Windows PowerShell 5.1's Get-FileHash catches the read error and writes a new one without it | host, join, update and setup hash through `Get-CofSha` (find-cof.ps1) |

## F1. Transit carry put every player on the same spot

Cause: `transit_touch` (coop-runtime.hpp, v0.10.6) carries every other ready player standing in a transit trigger_multiple through the same cof_teleport by calling its native Use. CCofTeleport::Use (decomp effects.cpp 7131) sets its activator's origin to the target, so all carried players landed on one point in the same frame. Player collision is on unless the host turns it off (`sc_collision` "1", session-runtime.hpp; `collision_off()`). The v0.10.6 fixture asserted the overlap (player 2 at x = -410, as player 1). The review counted 329 such triggers on the served maps.

Change (coop-runtime.hpp `transit_touch`): after the native Use, each carried player is moved with `arrival_landing` (the v0.6 arrival search: rings of 48-480 units, a clear hull, a floor, a straight line from the anchor) around the first player, avoiding every trigger that leads out (`exit_volumes`; at the stad the destination is next to the door and its trigger). The teleport's yaw is kept. With collision off nothing is moved. When no spot is found the player stays on the destination and the TRANSIT line says so (`no clear spot beside him: left on the destination`); otherwise it ends with `, beside him`.

Test: test-qol2.cpp, v0.10.6 transit block, now with a world that has a floor at the destination, players that are solid to a hull trace, and an exit trigger beside the destination: player 2 ends 32-100 units from player 1, not inside the exit, with the teleport's yaw (`0 90.00 0`), and the log ends `, beside him`. With `sc_collision 0` both stand on the destination. On v0.10.6 sources the fixture fails at the "beside him, not in him" assertion.

## F2. Defender taking the installed adapter

Cause: the tester's launcher.log (docs/V0106_RELEASE.md section 1) had "Already current: 236 file(s)" and no adapter in `cl_dlls`, so an earlier install's `cryoffear\cl_dlls\storycoop_rc4.dll` had most likely been removed by the antivirus. The v0.10.6 steps excluded only the extracted folder, so the next install writes the DLL into the game folder again, and:
- a blocked write (`Write-NewPatchFile`, `[IO.File]::Move` / `Replace`) threw the raw IOException: the trap only looked at `$script:CoopBlocked`, which only reads set;
- removed just after the write: setup printed `Not as expected: cl_dlls\storycoop_rc4.dll` and "Verification failed after install.";
- removed later: Run Co-op Mod said the mod is not installed; join-coop.ps1 read the adapter with raw `ReadAllBytes`; update-coop.ps1 hashed it inside a `catch {}` that swallowed everything.
- Found while fixing this: Windows PowerShell 5.1's Get-FileHash is a script function that catches the read error and calls `Write-Error` with a new message (checked on this PC: `(Get-Command Get-FileHash).Definition`), so the HResult is gone and the v0.10.6 check in host-story-coop.ps1 (`Test-CofBlocked` on a Get-FileHash error) could never fire.

Changes (scripts only, ASCII):
- install-story-package.ps1: the trap also treats any terminating error with ERROR_VIRUS_INFECTED / ERROR_VIRUS_DELETED in its exception chain as blocked, and names `$script:CoopTarget`, the game file being written (set for each installed item, the menu and the chapter-order rewrite). Exit code 3 and `COOP-BLOCKED <path>` as before; the message then says the installer stopped at that file and Install again finishes it.
- find-cof.ps1: `Get-CofSha` (hash through `[IO.File]::OpenRead`, the steps on a blocked read), `Test-CofAdapterGone` / `Assert-CofAdapter` (the co-op maps are installed and the adapter is not: the steps, then stop).
- setup-coop.ps1: hashes through `Get-CofSha`; the post-install `-VerifyOnly` run passes a `COOP-BLOCKED` line on to the steps; a co-op DLL listed as missing after the install (the adapter first) prints the steps instead of `Not as expected`.
- host-story-coop.ps1 (Run Co-op Mod and the RC4 launches): always loads find-cof.ps1; `Assert-CofAdapter` before the old "not installed" line; the adapter hash through `Get-CofSha`.
- join-coop.ps1: `Assert-CofAdapter`, hashes through `Get-CofSha`.
- update-coop.ps1: hash through `Get-CofSha`; maps without the adapter print the steps.
- `Write-CofBlocked` step 2 and READ ME.txt: exclude two folders, the extracted "Cry of Fear Co-op" folder and the game's `cryoffear\cl_dlls` folder. HELP line 2 says "exclude the co-op folder and cl_dlls".

This does not stop Defender. The lasting fix is still a false-positive submission (https://www.microsoft.com/wdsi/filesubmission) or code signing.

Test: tests/v0107.test.mjs runs the guard with a blocked write (a MethodInvocationException around an IOException with HResult 0x800700E1: exit 3, `COOP-BLOCKED` with the game path; a sharing violation stays a normal error), `Assert-CofAdapter` against fake game folders (maps without the adapter: the steps with both folders, three HELP lines under 110 characters, then stop; with the adapter, or without the maps: nothing), `Get-CofSha` against Node's SHA256, and join-coop.ps1 itself on a fake game folder (stops with the steps before "Starting Cry of Fear"). Host and setup are checked statically (running them looks for Steam or installs); host's check comes after `-MenuOnly` sets `$NativeRC4`, so Run Co-op Mod is covered.

## F3. A swallowed optional read left the blocked mark set

Cause: `Read-CoopBytes` sets `$script:CoopBlocked` and rethrows. The review named four sites. Checked one by one: the two XOR-delta branches (the replaced menu title / client.dll and the built CO-OP title) swallow the error but then throw "Generated extra mismatch", so reporting the block there is right; the outfit textures are read outside any try, so a block there ends the install with the steps at once. The one real case is the optional Simon player model: both its reads are inside a try/catch, and an optional mismatch then `continue`s, so the mark stayed set and any later, unrelated `throw` exited with code 3 naming the model file.

Change: `Skip-CoopBlocked` (inside the read guard block) prints `Optional file skipped: the antivirus blocked <path>` and clears the mark; the optional Simon skip calls it.

Test: tests/v0107.test.mjs: a blocked read swallowed by a try/catch, `Skip-CoopBlocked`, then an unrelated `throw`: exit code 1 with that error, no `COOP-BLOCKED` (v0.10.6: exit code 3 naming the skipped file).

## F4. A second ChangeLevel in the same frame

Cause: the engine ignores every `pfnChangeLevel` after the first on a map (decomp `src/engine_hw/pr_cmds.c` PF_changelevel_I keeps `last_spawncount`). The adapter's hook (`adapter.cpp` `change_level`) ran `carry_level_change` -> `load_wait_level_change` and `arrival_level_change(map)` on each call, so a second call reset `pendingArrival` for a map the engine will not load and logged the wrong CARRY map. Since v0.10.6 the load wait is only pending until ServerDeactivate, so for the rest of the old map's last frame another player's move could fire a second exit or a story `trigger_once` on a map that is about to end (a fired trigger_once is then saved as used).

Change (coop-runtime.hpp `level_change_first` / `level_change_closing`, adapter.cpp `change_level`): the first call on a map is remembered until `coop_map_changed` (ServerDeactivate); later calls only go on to the engine (which ignores them) and log `CARRY level change to <map> ignored: the engine runs only the first one on a map (<first map>)` (at most 8 lines). `arrival_touch_blocked` shuts every player trigger touch (not trigger_hurt) for 1 s of server time after the first call; the limit keeps a change the engine never runs from shutting the triggers for good.

Test: test-qol2.cpp, v0.10.7 block: on s4_buscity_e0, a change to s4_city_e1 and then to s4_park_e0 in the same frame keep the s4_city_e1 arrival and CARRY line and log the second as ignored (v0.10.6: the arrival is reset); a story trigger_once touched right after does not fire, and does 1.5 s later; on the next map its first change counts again.

## F5. Restoring a movewith group all or nothing

Decomp check (Spirit `src/dlls/cbase.cpp`): `CBaseEntity::Activate` returns before `InitMoveWith` when `m_activated` is set (line 571-574), and `m_SaveData` saves `m_MoveWith`, `m_pMoveWith`, `m_pChildMoveWith`, `m_pSiblingMoveWith` and `m_activated` (831-839). So a child restored from the save whose parent has no record in the same table comes back activated, with a null `m_pMoveWith`, and is never linked again: a button that no longer rides its train. The review's reading is right.

When a member has no record: DispatchSave writes nothing for FCAP_DONT_SAVE classes (temporary env_sprite / env_beam, CShower, CPlatTrigger, gibs, corpses, the spray can) and for monsters that are dead or frozen by a camera (cbase.cpp 281-288); a key can also be missing on another alias of the map. Audit of the served maps (`work/claude-s25-maps-001`, every entity with a "movewith" key or named by one): env_model 189, func_train 105, trigger_hurt 39, env_particle 36, env_sprite 35, ambient_generic 30, env_dynlight 24, func_wall 24, func_door 24, func_illusionary 23, func_button 19, inter_door 7, trigger_once 5, func_tracktrain 1; no monster and no temporary sprite or beam. So on a v0.10.6+ save of these maps the guard should not fire; it is a guard against a silent failure.

Change (state-runtime.hpp `state_activate`): before anything is restored, the fresh entities are grouped by `state_group`. A group with a live member that has no usable record (none, or one of another class) and is not in the removed list, while the save has an entry for some other member, stays as served whole (its keys join `keepKeys`, counted as kept-as-served) and the log has `MAPSTATE group <name> kept as served (incomplete record)`. A group with no entry at all (a v0.10.5 save) is as served anyway, without a line, and `state_repair_trains` still handles it.

Test: test-mapstate.cpp, v0.10.6 block: the stad save without the train's record keeps the button as served (health 100 as spawned, not the saved 43) and logs the line; the complete 2trainbutton group is restored as before; the v0.10.5-style save logs no such line.

## Not changed, still unproven from v0.10.6

- Whether the load-wait and transit changes end the tester's "random teleports". Lines to ask for in the host log: `LOADWAIT`, `TRANSIT`, `ARRIVAL`, `STORYTELEPORT`, `CARRY level change`.
- Movewith groups restored in the real hl.dll (s4_stad, s4_subway2st1, s4_college2 first).
- A real Defender block against the new messages.
- Old saves: s4_sewer3's key button is not repaired; a save in which the fuse button was already used can give one more fuse.

## Not reviewed line by line

checkpoint-runtime.hpp, session-runtime.hpp, qol*-runtime.hpp, story-runtime.hpp and CoopExtras.cs were read only where these fixes touch them. checkpoint-runtime.hpp is the next file to review.

The v0.10 features are in docs/V010_RELEASE.md and tested with docs/V010_TEST.md; steps 63-69 cover this build.
