# Cry of Fear co-op v0.7 (2026-10-30)

v0.6.3 (the Playtest 3 fixes, Unlock extras, Doctor's shot for everyone, the save
window with five slots) plus the player model menu and the fixes below. Maps
byte-identical; adapter, menu and installer only. Native tests pass, and every menu
patch passes against the real client.dll and GameUI.dll.

**Run in the game by Claude, solo** (window 800x600, Install then the real launcher):
the menu rows and credit, the model box with every model, its previews and the saved
choice, and an outfit on a story map (`MODEL player=1 is simon_hellokitty`). The installed
outfit model was also drawn from its own file by the preview renderer. **Not run:** two
players (a partner seeing the outfit), the wearer's own hands, the enemy, elevator and
door fixes in play. Quit: its handler was read in the disassembly, and the console quit
closes the game; clicking it on a clean launch is for the test run below.

| Item | Cause (evidence) | Change |
|---|---|---|
| Co-op menu missing (host log: `MENU co-op menu off: unexpected bytes at 101442B4`) | The 5-slot save work patched the name `%ssaveinfo%i.cof` two bytes late. The menu patch applies all or nothing, so one wrong address turned the whole menu off in v0.6.3. | Address corrected (0x101442B2). New suite `gamebytes` checks every patch list against the real game files, read as bytes. |
| University enemies do not move and cannot be killed (clips 7, 8) | They are monsters recreated when a map is revisited (host log: `recreated=7` on s4_college1dark_e2). They are new entities past the count the engine gave ServerActivate, so the game never called their Activate. Activate links a restored entity into Spirit's assist list (cbase.cpp, movewith.cpp) and a monster only thinks from that list: no AI, and a killed monster never reached its death schedule. | ServerActivate now covers the recreated entities. This applies to every revisited map with spawned monsters (the same log shows them on subway1st1, city, park2, park3, stad, subway2st1, asylum1). |
| Code-locked elevator scene missing (clip 2) | Not in s4_apartment3: the door leads to c_apartmentsick, and single player lands inside `trigger_once *10` (`multi_hiss_1`: cameras hisskam1/2, Simon entering and closing the door), then falls into `trigger_cofteleport *9`. That level change had no co-op spawn, so players started at the map's default start, on the far side of the teleport. | The level change lands as in single player (`ARRIVAL` line), so the scene plays. |
| Door opened from the other side stays locked (clip 16) | s4_city `bclockeddoor`, beside the SB AB van: `trigger_once *47` at the arrival from the bus map fires `untriggerlock`. The arrival was dropped in that build (no `ARRIVAL` line after buscity -> s4_city_e1; race fixed in v0.6.3). | One-shot triggers at the exact arrival spot now fire even if the landing has to move aside (`ARRIVAL fires the trigger`). Arrivals wait until every player has loaded, so a scene there starts for everyone. |
| Save window | The native save sequence makes the host "not a target" before the co-op save ran one frame later, so the save would have been refused. | The co-op save runs on the double-click itself. |

An audit of every original level change against the co-op spawns found no other
scene or unlock left behind at an arrival spot.

## Player model menu

The main menu reads Host, Join, Player Model, Options, Quit. Player Model opens the
game's computer screen in its logged-on state, which has no text fields, so nothing can
be typed. Each model has its own picture with a rendered preview (`sc_mb<n>.tga`, XOR
deltas rebuilt from the player's own file; `scripts/menu/make-modelbox.py`) and the panel
names it. Next shows the next model and keeps it; Done or the red power button closes
the box. The choice is kept in `storycoop_model.txt` and sent with every start
(`setinfo sc_model`).

The models: Simon, his 11 outfits, then the player's own. Install builds each outfit from
Simon's model and the game's own costume textures (`scripts/simon-model.mjs`, the same
steps in the installer's C#; byte-identical, checked at install). Player models name the
hoodie, jeans and face textures differently, because the client paints those names in the
LOCAL player's outfit, which would dress everyone alike. The wearer's own client gets the
game's clothes state, for their hands and the story's Simon.

`.mdl` files in the mod's `playermodels` folder are copied to
`models/player/<name>/<name>.mdl` by "Run Co-op Mod.cmd" (never replacing one already
there; a `<name>T.mdl` goes beside it). They show "No preview". The host shows a player
with their chosen model when the host has that model too (`MODEL player=N now <name>`);
otherwise Simon and a notice to that player. The host's own models, with their texture
files, are offered for download to joiners; every PC builds the outfits itself.

Still open: crawler (s4_subtunnel2), crouching on a save station. A custom hoodie
(models/costumes/custom_hoodie.tga, the game's twelfth clothes state) is not in the box.
