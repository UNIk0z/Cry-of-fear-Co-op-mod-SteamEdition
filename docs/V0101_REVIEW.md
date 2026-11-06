# v0.10.1: full-code review (Claude, 2026-11-06)

Katz: "review the whole code from v0.10 ... and fix any potential issues". The shipped code as of v0.10 was read file by file and checked against the decompile (cof-decompiled) and the game's own files, read-only. Covered: native/rc4/adapter.cpp, session-, checkpoint-, coop-, story-, world-, state-, qol-, qol2-, camera- and ladder-runtime, client-menu.hpp; launcher/CoopLauncher.cs and CoopExtras.cs; scripts/setup-coop.ps1 and install-story-package.ps1 (upgrade, uninstall, mutable settings files); linux/coop-linux.py (the same paths). Skipped as already covered by their own fixture suites and reviews: spray-runtime (Codex, v0.9 Stability Fix), keydoor/pp-runtime, puzzle.cpp, journal.cpp, checkpoint.cpp (its decoder throws on every out-of-range read). RebuiltSimon crashes were not looked at (Katz).

The eight bugs of the v0.10 review pass are in v0.10 already (docs/V010_STATUS.md, "Review fixes").

## Fixed in v0.10.1

The second pass (Katz: "review the code from the start ... focus especially on bosses") is docs/BOSS_REVIEW.md: five boss fixes. That pass also read keydoor-, pp- and spray-runtime line by line (no change: the pp renames survive a save through their suffix strip; spray reads every pointer through spray_readable) and the v0.5 map build (its fall-zone change leaves the Doctor's scene death, a cof_killplayer fired by a manager, as in single player). puzzle.cpp, journal.cpp and checkpoint.cpp stay covered by their fixture suites.


| Where | What broke | Fix |
|---|---|---|
| native/rc4/checkpoint-runtime.hpp recorder_end | With "Hardcore: no saves" (sv_coop_nosave 1) the host's tape recorder still opened the single-player save window; a double click played the native "Saving progress... Saved" sequence over a save the adapter refused. | The recorder says "Could not save: saving is off on this server" and the window never opens. Fixture: test-qol2. |
| native/rc4/story-runtime.hpp net_tune (netTuned), via qol2-runtime.hpp qol2_disconnect | netTuned asks each player slot once per map for its network tuning and "sc_report". It was cleared only at a map change, so a player who rejoined the same slot on the same map (the usual path after the crash-rejoin offer) got neither: no cl_cmdrate 100 / rate (the "shivering" partner of v0.5.6 again), and his model, hands, subtitle language, colour and black and white were unknown until the next map. | The slot is cleared when its player leaves. Fixture: test-qol2. |
| native/rc4/session-runtime.hpp modelReports, playerLanguage, modelStates | Kept per slot after the player left: the next player in that slot was drawn with the last player's model and read subtitles in his language until his own report came (for good, with an older game that sends none). | Cleared with the slot on disconnect (same change). |

## Checked, not changed

| Where | Finding | Why not changed |
|---|---|---|
| Hunted (qol2) and CoF_BookSimonSledgehammer.cpp | His anim events 2 and 3 kill with Killed() and no lethal TakeDamage; in co-op that reaches DeathNotice with a NULL inflictor (the v0.5.7 Sawer crash). | booksimon.mdl (read-only) has only event 1 (a normal 30 damage hit) in every attack sequence, so the kill events never fire. |
| scripts/setup-coop.ps1 uninstall | The ".storycoop-old" copies of the settings files (v0.7 sc_setup.scr, v0.10 storycoop_server.cfg / storycoop_comfort.cfg) stay in the game folder after an uninstall. | They are the player's own earlier settings, a few hundred bytes; deleting or moving them is not worth any risk to user files. |
| qol2 scale_frame | A map's cof_bosshealthbar can make any monster a boss; if its bar starts before the 0.5 s scaling tick, that bar's maximum is the unscaled health for the fight. | Display only; the story's bosses are excluded from scaling, and changing it needs a game run to see which maps do this. |
| adapter.cpp client_command | "dodgetoggle" on the opening maps (not story-scoped) is not remembered. | The opening is passive by design; the choice made on any story map is kept. |
| qol-runtime / spray (Codex) | The client timer writes spray lines to the engine console. | Harmless; noted in V010_STATUS. |
| CoopLauncher.cs | Update check (0.10.1 > 0.10 > 0.9, numeric), Linux asset never taken, ZIP paths kept inside the new folder, rejoin target validated before it reaches PowerShell. | Correct as is. |
| install-story-package.ps1 / coop-linux.py | Mutable settings: $setupReplace is only read for mutable extras, which always set it first; Linux resets it each loop; untouched v0.9 defaults are replaced once with the old file kept. | Correct as is. |
