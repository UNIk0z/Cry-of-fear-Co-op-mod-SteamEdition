# Cry of Fear co-op v0.8 (2026-11-01)

v0.7.2 plus these changes. Maps byte-identical; adapter only. Native tests pass. Nothing here has run in the game. Per-item status: docs/V08_STATUS.md.

| Report | Cause | Change |
|---|---|---|
| "5 billion of everything": items duplicated by player count | Copies stood side by side for everyone | Every pickup except notes and key items is one per player, on the same spot. It disappears only for the player who takes it. Alone, you see each one once. |
| Key items | One per team, with a "<player> picked up" line | Everyone gets a key item when one player does, if they have room. Nothing drops. When it is used, it goes from everyone. You see the game's "You got the <item>". |
| Save sound twice | The game's own save sequence plays it, and the mod played it again | Once. |
| Weapons under the players after the train crash; gun icons that did nothing | 5 carried weapons given back into 3 slots: the game drops what does not fit | Nothing carried comes back on that map. A restore never gives more than fits. A weapon icon with no weapon behind it is removed. |
| Train: not moved to the player who got out | A partner still in the carriage was under 160 units away, so he was skipped | Everyone not already safe beside that player is moved. |
| Ammo limit | Multiplayer branches gave 3 clips | Same limits as single player. |
| c_college1dark exit | Landed by the old landmark | The exit to s4_hamn3_e0 lands at 374 662 71. |
| Host had no player model | "Info string length exceeded": the model never reached the server | Each game reports its model to the server at every map. The joiner crash in the logs is inside the RebuiltSimon add-on and is not changed. |
| Hand models | New | Done on the Player Model box opens a Hands page: same as the model, Simon, or an outfit. |
| Cameras still zoom in | Skip could not find the zoom and started it after the scene | A zoom in a skipped scene never starts, and a running one returns. Any zoom left behind is reset when a scene ends. |
| Subtitles after a skip | Skip fired every subtitle at once | Skip prints none and clears the line. |
| Black box after Sawer; bosses | The shaft relies on a falling wall | A second after the death scene, the way out runs. The start of each boss fight is saved by itself. A player's death shows game over and a fade, then restarts from there. |
| Players left behind by cutscenes (clips 6, 7, 8) | The map's teleport moves only the player who triggered it | Every story teleport brings the others along, and a scene's end brings everyone to the player who started it. |
| Loop after the doctor and police scene, s4_subwaysick2_e0 to s4_subway2st1_e2 | Only the first player was moved to the station; the other walked back through the exit beside the corridor, and the map's one-shot trigger was then used up | Both go to the station. Exits do nothing for your first 3 s on a map, no trigger fires until everyone has loaded, and anyone left in that corridor is sent on. |
| "Picked up" text on every map load | Inventory coming back counted as new pickups | Silent until your inventory is back. |
| "Zoom" (clip 17) | The host's own wheelchair on the last map, ping 0, so not network. Its speed cap ran only 10 times a second | The cap holds every frame. |
