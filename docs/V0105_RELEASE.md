# Cry of Fear co-op v0.10.5 (2026-11-09): hand models, review fixes

v0.10.4 plus Katz's request of 2026-11-09: "hand models dont work people reported ... first its simon model with simon model skin display and then its hand model selection with hand model skins display", and a review of the code. Maps are byte-identical to v0.10.4. The adapter, the Player Model box pictures (nine new ones), release.json, known-builds.json and the READ ME version changed. Nothing here has run in the game.

| Request | What was wrong | Fix |
|---|---|---|
| Chosen hands never showed | In multiplayer the game draws the first-person sleeve with its police-sleeve code (decomp gl_studiomodel.cpp, "if (bMultiplayer)" at client 100668B9), which never reads the clothes state the adapter sends. Only the single-player branch paints the costume sleeve. The choice was saved and sent, but Simon's own sleeves were always drawn. | One checked 2-byte jump in the in-memory client.dll (100668EE "push 0; call" -> "jmp 10066917"): the view model's sleeve takes the single-player path, the chosen costume's sleeve or Simon's own. Every other model keeps its own textures. |
| Hands page showed the player model | The Hands page reused the Player Model pictures. | The Hands page has its own pictures, `gfx/vgui/sc_mbh<state>.tga`, titled "Hands": the first-person arm in each costume sleeve (models/weapons/v_actions.mdl), rebuilt at install from the game's computer screen like the model pictures. |
| Hands list | Afraid of Monsters, Black Metal and Team Psykskallar have no sleeve; the game draws them with the Awesome sleeve, so three entries looked the same. The list also needed the outfit player model files. | The list is Same as the model, Simon, Leatherhoff, ModDB, Hello Kitty, Camo, HLC, Awesome, Sick Simon, Twitcher (sleeves are game files). An older saved choice of one of the three is Awesome. "Same as the model" names the sleeve it gives. |
| Plain Simon's hands | The game's own message after a spawn says "clothes on, state 0" when the host has unlocked any outfit; with the new sleeve path that asks for a sleeve file that does not exist (decomp LoadSleeveTexture). | The adapter now also sends plain Simon's hands (clothes off), every 5 s like an outfit. |

## Review

Read again: every change since the v0.10.1 full review (v0.10.2 to v0.10.4: adapter.cpp, coop-runtime.hpp, qol-runtime.hpp, qol2-runtime.hpp, the launcher, find-cof.ps1, install-story-package.ps1, coop-linux.py, the menu files) and the model/hands code (client-menu.hpp, session-runtime.hpp), checked against the decompile and the pinned hl.dll / client.dll.

| Where | What broke | Fix |
|---|---|---|
| coop-runtime.hpp card_locks_left (v0.10.2 fuse sockets) | The locks come from the map's "lockedby" keys and stay listed after a key opens them. Only buttons were checked for "still locked", so every door or valve counted as a lock left for good: after a key was used on a door, the other players kept that key forever ("KEYITEM ... more lock(s) for it on this map, the others keep theirs"). | A door, co-op door or valve counts only while its m_sLockedBy is set (inter_door +0x190, inter_door_coop +0x114, func_valve +0x180, from the pinned hl.dll's save tables; UnlockDoor clears it). |

Checked, not changed: door wait (solo and a downed partner never wait), the exit-arrival guard (s4_hamn2), the map's extra pickup copies, the phone for everyone, the Carcass owner fix, the RebuiltSimon guard (set at the server's end only), drop ammo (ammo never respawns in co-op: CHalfLifeMultiplay::AmmoShouldRespawn is NO), the comfort Q key, the installers' sc_doorwait replace rule.

Kept from v0.10.4: Q drops ammo, each player's own copy of every pickup, doors that change the map wait for every living player (`sc_doorwait`, on by default), and all v0.10.2 fixes.

The v0.10 features are described in docs/V010_RELEASE.md and tested with docs/V010_TEST.md (steps 45-50 cover this build).
