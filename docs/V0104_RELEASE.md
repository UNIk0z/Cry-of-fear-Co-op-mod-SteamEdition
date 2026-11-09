# Cry of Fear co-op v0.10.4 (2026-11-09): drop ammo for a teammate

v0.10.3 plus Katz's request of 2026-11-09: "make it so that on a certain keybind players can drop ammo to their teammates". Maps are byte-identical to v0.10.3; the adapter, the launcher (SETTINGS has a sixth key), storycoop_comfort.cfg (one new bind), release.json, known-builds.json and the READ ME version changed. Native and Node tests pass. Nothing here has run in the game.

| Request | How it works |
|---|---|
| Drop ammo for a teammate on a key | **Q** (change it in SETTINGS in "Cry of Fear Co-op.exe") drops one box of the ammo of the gun you hold, in front of you. It is the game's own ammo box: anyone takes it with E, like a box found on the map, and its rounds come out of your spare ammo (not the loaded clip). One box is what a box of that ammo holds in single player: Glock 15, P345 8, shotgun 9, rifle 5, revolver 5, M16 20, VP70 18, G43 10, TMP and MP5 30. Your teammates see "<your name> dropped ammo for you". |
| Limits | Once a second. Not while down, in a cutscene, during the load wait or with nobody else in the game. Not while dual wielding (hold one gun). With less spare ammo than one box: "Not enough spare ammo". TMP and MP5 boxes only on maps that have one of those boxes (their model must already be loaded). A dropped box is not kept across a map change or a load. |
| Key for players who already have settings | A storycoop_comfort.cfg that is still the installed default is replaced once with the new one. A changed one is kept, and the mod binds Q at the co-op start when config.cfg does not already use Q for something else. |

Kept from v0.10.3: each player's own copy of every pickup (the maps' extra copies removed), doors that change the map wait for every living player (`sc_doorwait`, on by default), and all v0.10.2 fixes.

Not changed in this build:

- "Changing hands doesn't work" still needs the exact steps.

The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md (steps 40-44 cover this build).
