# Cry of Fear co-op v0.6.1 (2026-10-25)

v0.6 plus three requests from Katz and the testers. Maps byte-identical;
adapter only. Native tests pass; **not run in the game, two players not
tested.**

| Request | Cause (evidence) | Change |
|---|---|---|
| Pickups for every player, side by side; weapons too except the scene Glock | New. | On the first visit of a map, each ammo pickup, syringe and weapon lying in the map gets one copy per extra player slot, on a 24-unit grid around it (48 when crowded). A spot is used only when it is empty, has floor right below and a clear line from the original, so nothing clips into walls or drops off ledges. Never copied: keys and story items, the phone, the knife, the Glock, and pickups that start an event. Revisits and loaded saves get no new copies (`ITEMS` log line). |
| Join box did not read the address correctly | The panel sends `computercheck <IP field> <port field>` (decomp Computer_guess.cpp); the engine cuts `:` `(` `)` into separate words, so `1.2.3.4:27015` or the untouched "Port (empty: 27015)" text gave the wrong port. | The raw text of the command is read instead: first word the address (an optional `:port` works), second word the port, empty or the prompt text means 27015. A refusal now logs exactly what the box sent. |
| More than one syringe | `CBasePlayerItem::iMaxAmmo1` (hl.dll 101202F0) returns 1 for weapon_syringe in multiplayer; single player uses the syringe's own 10. | That one value is raised to 10 when the map starts (checked byte patch of the pinned game DLL, `SYRINGE` log line). |
