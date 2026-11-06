# Boss review (Claude, 2026-11-06, v0.10.1)

Katz: "review the code from the start ... focus especially on bosses". Every fight with a boss health bar (cof_bosshealthbar) in the installed co-op maps (entity lumps read-only from the game folder), traced from what starts it to what fires on the boss's death, against the decompile (cof-decompiled/src/dlls) and the adapter's boss code (coop-runtime.hpp boss section, qol2-runtime.hpp). Nothing was run in the game.

## The fights

| Map | Boss (class, name) | Start | Damage | On death | Co-op risks found | Verdict |
|---|---|---|---|---|---|---|
| s4_apartmentsick_e0/e1 (Ch. 2) | Sawer (monster_bosschainsaw "theboss") | multisawersc fires the bar "bossbar" | bullets only (x0.3), any damage while his eye is open (skin 1) | multikilled1: scene, then cof_triggeron lazzerro -> teleportusus; the adapter runs the exit changekuk (v0.8 BossExits) | His grab spawns third_person_interaction and kills with Killed() (the v0.5.7 DeathNotice crash): the adapter already restarts the map at the grab. Placed and alive from the map load. | OK after fixes 1-3 |
| s4_sewer3_e0/e1 (Ch. 3) | Mace (monster_sewerboss "dsasdaeas", wait for script) | multisecsene fires "bossbar2" | only electricity (DMG_SHOCK, the map's water) | whenIdie: scene, the converted doors re-opened, cof_teleport of the activator (the v0.8 story-teleport follow brings the others) | Chapter 3 starts on s4_sewer3_e1 with Mace still placed and alive: every death there was a game over. | OK after fixes 1-3 |
| s4_roofs2_e0/e1 (Ch. 4) | Carcass (monster_roofboss "carcassboss", wait for script) | multistartitall fires "bossbarh" | normal; its heads are roofboss_head (not monster_*, never scaled) | multikill: cof_ending, scene, killtarget, cof_teleport (followed) | First-player lookups in its think: the adapter's roofboss_enemy (v0.3) answers with its live enemy. Falls on the roofs before the fight: game over instead of the fall respawn. | OK after fixes 1-3 |
| s4_attic_e0/e1 (Ch. 7) | Doctor (monster_doctorboss "doctorboss") | multidadoc fires "doctorbar" (its yes/no keeps the scene unskippable, v0.5.7) | through the cof_doctorshoot boxes | multidscene: weapon trigger, scene, doors | Aims at the first player only (the host) and reads the weapon choice from him (shared for all since v0.6.3); his death fires its target with the host as activator. The "you die" zone (toncedeath -> cof_killplayer) kills every player with the player as killer: no DeathNotice crash, a game over as in single player. Chapter 7's start removes him (killtarget doctorboss). | OK |
| s4_ending4b (ending 4) | 3 hiding Book Simons (monster_booksimon frags 1) and the sledgehammer Simon "simon" (1000 health); the bar shows monster_generic "justhealth" (250) | trigger_once fires "bar" | each hiding Simon passes up to 60 (after x0.3) to justhealth, then hides; the sledgehammer Simon passes all of it | justhealth dies: fuckoff (teleport, credits, stop wheelchair) | The Book Simons are hidden (EF_NODRAW) until they appear, so the boss check saw no boss and Hunted could spawn. "Harder with more players" scaled justhealth (up to 512): no softlock (the sledgehammer Simon passes up to 1000, he is never scaled), but the bar's numbers were off. | OK after fixes 4-5 |
| s4_ending1c/2c/3c (endings 1-3) | the strangle (cof_strangle "strangle"); the bar shows monster_generic "fagnerd" (100) | multibossscene fires "bar" | each press does 25 to fagnerd and 25 to the strangled player | the player's own health reaches 1: multisdeath | Ends on the player's health, not the bar's, so no softlock; scaled, fagnerd's bar never emptied. No boss class alive: the boss rules stay off. | OK after fix 4 |

Not boss fights (no bar), checked for the same patterns: the Drowned (monster_crazybitch; the adapter's v0.3 victim fix; a partner can shoot her, scaling only makes it harder), the Tallers of s4_buscity2 (their stamp kills with the player as killer: no crash; multiplayer branches in their code), the strangle's first-player lookup (the adapter's v0.5 strangle_activator).

## Fixed in v0.10.1

1. **A death anywhere on a boss map was a game over** (coop-runtime.hpp boss_frame, fall_frame). Every boss is placed in its map and alive from the load (waiting for its script), and the v0.5.7 rule "a player dies while a boss is alive: game over" used boss_alive() alone: any death or fall on the map, long before the fight, ended the game for everyone (no revive, no fall respawn), and on the Chapter 3 start map Mace is placed for good. Now the fight begins with its health bar (boss_bar_lookup); a player already down at that moment does not end it until he is up and falls again (bossDownBefore). Falls before the bar get the normal fall respawn.
2. **After a game over inside a fight, the fight lost its bar** (and, with fix 1 alone, would also lose its game-over rule). The boss autosave is taken just after the bar fires; a game over loads it and the map state restores the bar as used (pev->iuser1 1, saved), so nothing fires it again. A used bar whose boss is alive now carries the fight on after any load (boss_bar_resume) and is shown to everyone again.
3. **A player who joins or rejoins during a fight had no bar** (the bar is per-player state, set for the players present when it fired). He is shown it with the bar's own Use, which sends it to everyone with the current values (boss_bar_late).
4. **"Harder with more players" scaled the counters the ending bars show** (qol2 scale_frame): monster_generic and any bar's target keep their health.
5. **Hunted could spawn in ending 4** (Book Simons hidden until they appear): Hunted is off on every map with a boss bar.

Fixtures: test-qol2 ("bosses"): no game over before the bar, a player down before the bar, revive then death = game over, the bar for a late player once, the fight going on after a load, a dead boss = no fight, scaling and Hunted on a boss map.

## Checked, unchanged

| Topic | Reason |
|---|---|
| Boss autosave while a partner is down | boss_save_frame waits (up to 30 s) until every player is up and free; it could save with the partner's last record instead, but a save mid-revive is not clearly better. |
| Players outside the arena | Each fight starts with a scene; the v0.8 scene-end gather and story-teleport follow put everyone beside the player who started it, and unstuck (K) still works. |
| Late join catch-up during a fight | The joiner gets the host's weapons and lands beside the host (inside the arena), now with the bar. |
| Hardcore no saves / no respawn | No saves: no boss autosave (skipped once, v0.10 review); a game over restarts the map with what everyone had on arrival. No respawn: a death in the fight is a game over anyway. |
| Timer, outlines, ping, scare | No interaction with the boss code. |
