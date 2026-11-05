# Co-op quality-of-life and customization proposal (Claude, 2026-11-05)

Ideas to make the co-op mod stand out, checked against the decompile (cof-decompiled/src) and the adapter source (native/rc4). Nothing here is built. Every item must work with the host alone and is tested solo first. Effort: S = a day or less, M = a few days, L = a week or more. Risk = chance of breaking what already works.

## What the game already gives us (from the decompile)

- **Revive tuning:** `sv_coopcprtime` (seconds to revive, default 5, player.cpp CPR code) and `sv_auto_respawn_time` (auto respawn while down, player.cpp RevivePlayer/PostThink) are real game cvars. Also `mp_friendlyfire`, `sv_disablehints`, `sv_morphinetriptime`, `sv_lobby_time`.
- **Look-at name and health:** the client still hooks `StatusText` / `StatusValue` and draws the status bar (cl_dll/statusbar.cpp, hud.cpp:962), and the server registers both messages (player.cpp:1254). Only `CBasePlayer::UpdateStatusBar` is commented out (player.cpp:3838, call at 9946). The adapter can send these two messages itself.
- **Difficulty data:** the 794 `sk_*` cvars are read by `RefreshSkillData`, which runs when the game rules are created each map (multiplay_gamerules.cpp:605). Values set before a map loads apply to that map.
- **Spectator code:** `StartObserver` exists (player.cpp:3448) and the client has the full HL spectator HUD (hud_spectator.cpp). Not used by CoF co-op.
- **Adapter tools we already have:** `hud_text` (on-screen text per player, used by the skip prompt), client commands like `sc_skip` / `sc_report`, "move beside a player" (story teleports, v0.8 item 11), "nearest clear spot" (v0.8 item 6), per-receiver control of what each player is sent (pickups, AddToFullPack), the in-memory co-op menu and the Player Model / Hands panel.

## Ranked list

| # | Feature | Value | Effort | Risk | Implementation sketch |
|---|---|---|---|---|---|
| 1 | **Unstuck command** (`sc_unstuck`, bindable) | High: softlocks were the top bug class; works solo | S | Low | On the client command, move the caller to the nearest clear spot (v0.8 item 6 code), once per 10 s, refused during scenes and on ladders. |
| 2 | **Regroup vote** (`sc_regroup`) | High: ends "partner stuck in the other room" without a restart | S | Low | Same vote flow as R skip; when it passes, move everyone beside the caller with the story-teleport helper. Solo: instant, does nothing harmful. |
| 3 | **Server config file** `storycoop_server.cfg` | High: base for every option below | S | Low | Installer drops a commented default file; adapter runs `exec storycoop_server.cfg` at each map start. Holds revive time, auto respawn, friendly fire, and the toggles below. Uninstall removes it. |
| 4 | **Revive options** | Medium-high | S (after #3) | Low | Just the real cvars in the cfg: `sv_coopcprtime`, `sv_auto_respawn_time`, plus our own `sc_revive_health` (health after a revive, set in the existing revive hook). |
| 5 | **Teammate name and health on look** | High: very visible, "feels like a real co-op mod" | S | Low | Every 0.2 s trace from each player's eyes; on a player hit, send `StatusText` "%p1  %i2%%" and `StatusValue` (index, health) the way the commented-out UpdateStatusBar did. Solo: nothing shows, nothing breaks. |
| 6 | **Downed alert** | High | S | Low | When a player goes down: `hud_text` to the others "Katz is down, 23 m behind you, hold E to revive", refreshed each second with the auto-respawn countdown. |
| 7 | **Team status line** | Medium | S | Low | Small `hud_text` line in a corner, on a free channel: each teammate's name, health and DOWN state. Cfg toggle `sc_teamhud 0/1`. Hidden during scenes. |
| 8 | **Ping / marker** (`sc_ping`, bindable) | High: standout feature in a dark game | M | Low-medium | Trace from the caller's eyes; send everyone a short glow sprite (TE_GLOWSPRITE with a sprite the game already ships) plus `hud_text` "Katz: here" with distance. Cooldown 2 s. Needs a check that the sprite exists in cryoffear/sprites. |
| 9 | **Player-count difficulty scaling** | Medium-high: 4 players currently trivialise fights | M | Medium | Cfg `sc_scale 0/1`. Before each map, scale monster health `sk_*` values by player count (e.g. +35% per extra player). Mid-map joins: also scale `pev->health` of monsters that spawn after. Off by default; solo = 1.0x, so solo is untouched. |
| 10 | **Teammate outline when far** | Medium | M | Medium | In AddToFullPack, give a teammate kRenderFxGlowShell with his chosen color only in the packet sent to other players, only past ~400 units and never during scenes (scene code already edits players in the pack, so needs care). Color picked on the Player Model panel. |
| 11 | **Spectate while down** | Medium | M | Medium-high | While dead and waiting, SET_VIEW to a living teammate, cycle with mouse1, reset on revive and at every scene start. Risk: scene cameras also use SET_VIEW; must never run during a scene. Solo: never active. |
| 12 | **Bindable co-op keys** | Medium | S | Low | Installer appends `bind` lines to a separate `storycoop_binds.cfg` (not config.cfg) and the adapter execs it: F2 unstuck, F3 regroup, MOUSE3 ping. Keys listed in READ ME. |
| 13 | **Co-op settings page in the menu** | High polish | L | Medium | New page next to Player Model in the existing in-memory panel, writing the cfg from #3 (host) and binds from #12. Do it last, after the cfg options exist. |
| 14 | **End-of-chapter stats** | Medium fun | M | Low | Count kills, downs, revives, deaths per player (existing hooks); show a `hud_text` card on the chapter title / ending map. |

## Suggested order

1. **Ready to build now (small, safe, solo-testable):** #3 config file, #1 unstuck, #2 regroup, #4 revive options, #5 look-at name/health, #6 downed alert. Together about one release (v0.9 "Co-op QoL pack").
2. Next: #8 ping, #7 team line, #12 binds.
3. Later, needs Katz's call: #9 difficulty scaling (changes balance), #10 outlines, #11 spectate, #13 settings page, #14 stats.

## Not recommended

- Through-wall player outlines: needs client render changes the wrapper cannot make safely.
- Voice/radio features: the game has a radio (dlls/CoF_Radio.cpp, cl_dll/vgui_radio.cpp) tied to story scripts; touching it risks story triggers.
- Steam invites/P2P: declined earlier, not revived.

## Open checks before building

- Does CoF honour `mp_friendlyfire` in co-op (PlayerRelationship / FPlayerCanTakeDamage)? Read before listing it in the cfg.
- Which `hud_text` channels are free (skip uses 3).
- Sprite for the ping: pick one already in the game's sprites folder.

## Katz's answers (2026-11-05)

- #1 Unstuck: yes, on the **K** key.
- #2 Regroup vote: not needed, unstuck covers it.
- #3 Server config file: yes.
- #4 Revive options: yes, as entries in the Create Server window (scripts/menu/sc_setup.scr, where sc_collision and sc_extras already live). The game cvars are `sv_coopcprtime` (default 5) and `sv_auto_respawn_time` (default 384), both FCVAR_ARCHIVE in game.cpp:48/50.
- #5 Name and health on look: problem in scenes, where players stand in the black box facing each other. Left out unless it can be hidden during scenes.
- #6 Downed alert: only the text "<player> is down", no distance or countdown.
- Nothing is built until Katz says so.

## More ideas found in the decompile (round 2, not built)

| # | Idea | What the game already has | Value | Effort | Risk |
|---|---|---|---|---|---|
| 15 | **Down text in the game's own style** (for #6) | `UTIL_ProMessage(player, text, ...)`, the white CoF notice the game uses for "Dodge enabled" (client.cpp:1383). | Looks native instead of plain HUD text | S | Low |
| 16 | **Hunted mode: Book Simon stalks the team** | Native co-op code spawns `monster_booksimon` at `info_simon_spawnpoint` or `trigger_booksimon` after the `simonmode` command (multiplay_gamerules.cpp:958, client.cpp:1629). Story maps have neither, so the adapter would place him far from the players. | Biggest "stand out" item: an optional horror mode no other CoF mod has | M-L | Medium-high (model precache, story scripts, solo balance) |
| 17 | **Co-op run timer and chapter times** | Survival mode drives a HUD timer with `gmsgTimerPlus` (multiplay_gamerules.cpp). Needs a check that the client draws it outside survival. | Speedrun/bragging value; show at each chapter title and the ending | M | Low-medium |
| 18 | **Scare button for friends** | `laugh 1-3` plays ambience/laugh1-3.wav at the player, but only for developers (client.cpp:1322). The adapter can play the same sounds on a teammate, with a cooldown. | Fun, very shareable on Discord | S | Low |
| 19 | **Hardcore options in the host box** | `sv_coop_nosave` (no saves), `sv_auto_respawn_time` (no auto respawn = must be revived), `sv_morphinetriptime` (35 s morphine trip), `sv_disablehints`. All real cvars in game.cpp. | Replay value, costs almost nothing once #3/#4 exist | S | Low |
| 20 | **Vote kick for public servers** | Native `votekick <name>` with `sv_votekick_time` (multiplay_gamerules.cpp:721) and its sounds. Just needs checking it works on our story maps and a READ ME line. | Needed once strangers join | S | Low |
| 21 | **Per-player dodge toggle remembered** | `dodgetoggle` exists per player (m_bDodgeDisabled), but is lost every map. The adapter's carry record could keep it. | Small polish | S | Low |
| 22 | **Late-join catch-up** | Partly done (key items, story teleports). Add: a joiner spawns beside the host with the team's story weapons, plus a "<player> joined" notice. | Fewer "where are you" moments | S-M | Low |
| 23 | **Developer commentary as co-op tips** | `CoF_DeveloperCommentary.cpp` is a ready node entity (sound + sprite on Use). Could hold short tips at hard co-op spots (the subway loop, ladders). | Novel, but needs audio/text per spot | M | Low |

Looked at and dropped: `mp_copsalwaysvisible`, `sv_fogradius` and `sv_joebiden` are registered but not read by the server code; `sv_custom_hoodie` is one value for the whole server, so it can't give each player their own outfit.

## Corrections from round 3

- **#5 already exists in the game.** CoF's own multiplayer code traces 384 units every 0.15 s and sends `LookingAt` with the player you aim at (player.cpp:5290). Nothing to build. If names show in the scene black box, the fix is to hold that message during scenes, not to add a new HUD.
- **#14 stats: the game has a native co-op stats screen.** The client hooks `CoopStats` and `BeatCoop` (cl_dll/CoopStats_guess.cpp:513-666): shots fired/hit/missed, accuracy, damage taken, time, melee and shooting kills, a grade from S to F, and its own `stats.mp3`. The adapter would only count the numbers and send that message at the ending (or at chapter ends). This makes #14 much cheaper (S-M) and it looks native.

## Customization options (round 3, not built)

Per player unless marked "host". Where the option lives: the Hands / Player Model panel already has pages, so the per-player ones could become a third page there; host ones go in the Create Server window (sc_setup.scr).

| # | Option | What the game already has | Value | Effort | Risk |
|---|---|---|---|---|---|
| 24 | **Spray logos** | Impulse 201 still paints the player's logo (player.cpp:8518, CSprayCan). The launcher could turn a PNG the player picks into the engine's tempdecal.wad. | Very visible in co-op, classic GoldSrc fun | S-M (launcher) | Low |
| 25 | **Flashlight tint and beam shape** | The flashlight is a dynamic light with a hard-coded colour (1.4, 1.4, 1.4) and the cookie texture gfx/flashlight2.tga (cl_dll/view.cpp:2695). Both can be changed in memory, the way the join box picture is (client-menu.hpp). Ship a few cookie TGAs: round, wide, cracked lens. | Personal look, good for screenshots | M | Low-medium (one more pinned client patch) |
| 26 | **Own ambient light colour and size** | The faint light around the player when no torch is on is client cvars `cof_reducedlightcolour1-3` and `cof_reducedlightradius` (view.cpp:2942). Only that player's game changes. | Lets each player tune darkness without breaking the mood for others | S | Low |
| 27 | **FOV slider** | Client cvar `default_fov` (hud.cpp). v0.8 forces 90 after a zoomed scene; it would use the player's own value instead. | Common request on PC | S | Low |
| 28 | **Head bob and comfort** | Client cvars `cl_bob`, `cl_bobup`, `cl_bobcycle` (view.cpp). Offer Off / Low / Default. | Motion-sickness option | S | Low |
| 29 | **Cinematic / screenshot mode key** | Client cvar `hud_draw 0/1`. A key that toggles it (and our hud_text). | Trailer and screenshot shots for Discord | S | Low |
| 30 | **Teammate colour** | Each player picks a colour on the model page; used for their outline (#10), ping (#8), spray tint and their name in our notices. | Ties the customization together | S once #8/#10 exist | Low |
| 31 | **Host: menu and lobby music** | Server cvar `sv_coop_music` plays a chosen MP3 to each player on spawn (multiplay_gamerules.cpp:1573). An option for the lobby / chapter start. | Small flavour | S | Low (must not cover story music) |
| 32 | **Per-player subtitle language** | `cof_subtitlelanguage` is per client but the save code can overwrite it (player.cpp:5430). Keep each player's own choice across maps. | Mixed-language groups | S | Low |
| 33 | **Outfits per player** | `setclothes` / `m_iUnlockedClothes` per player and the `Clothes` message exist, but `sv_custom_hoodie` is one value for everyone. Our Player Model picker already covers outfits; only worth it if the native outfit textures look better. | Low (already covered) | M | Medium |

Checked and left out: `goro` is read into a variable that nothing uses; `gl_*` graphics cvars are already in the game's own video options; `skybright*` changes the sky for one player only and could make maps look broken.

## Katz's picks so far (2026-11-05, nothing built yet)

- From round 1: #1 Unstuck on K, #3 server config file, #4 revive options in the Create Server window, #6 "<player> is down" text only.
- From round 2: liked the ideas (#15-23), kept on the list.
- From round 3: **comfort options** (#27 FOV slider, #28 head bob, #29 screenshot/HUD-off key), **#32 per-player subtitle language**, **#24 spray logos with their own setting** (a separate Spray page or launcher tab to pick the image, not mixed into the model page).

## More discoveries (round 4, not built)

The server registers about 150 HUD messages (player.cpp REG_USER_MSG list). These are the ones the story co-op doesn't use yet:

| # | Idea | What the game already has | Value | Effort | Risk |
|---|---|---|---|---|---|
| 34 | **Custom hand textures: sleeve, glove, skin** | The client takes `CustomSlv`, `CustomGlv` and `CustomFng` messages with a texture path and paints it on the view hands (hud_msg.cpp:1289-1338, gl_studiomodel.cpp:159-401). The server only ever sends empty ones (multiplay_gamerules.cpp:1237). We'd send each player their own choice. | Real hand customization, which v0.8 said wasn't possible. Fits the existing Hands page. | M (needs a few TGAs, plus a check of the expected size and format) | Low-medium |
| 35 | **Black-and-white filter** | Per-player `m_iBlackAndWhite` and the `BlackWhite` message (player.cpp:9392, hud_msg.cpp:882). | "Noir mode" for one player, good for videos | S | Low |
| 36 | **Native control hints for our keys** | `ControlHint` shows the game's own key prompt from hints.txt (player.cpp:14925). It could show "K: unstuck" or "R: skip" in the game's style instead of plain text. | Polish, and it's localized | S | Low (language files) |
| 37 | **Objective reminder** | `Objectives` sends the current objective text plus the language's hints.txt (player.cpp:6719-6763). Needs a check of which maps set it. A key or a notice could repeat it to a player who joins late. | Helps lost partners | S-M | Low |
| 38 | **Sound cues for team events** | `PlaySFX` plays a sound on one client (hud_msg.cpp:1340). It could play the game's own sounds for "teammate down" or "teammate revived" so the text isn't missed. | Feels native | S | Low |
| 39 | **Native join notice** | `InitHUD` already prints "%s has joined the game" (multiplay_gamerules.cpp:1249). Check whether the story maps show it. If they do, idea #22's notice is free. | Free | S (check only) | None |

Not worth it: the `DevDonator` flag (only marks the game's real developers and donators), `LensFlare` and `Blur` (visual effects for story scenes), `Timer` (survival only).

## Katz's picks (round 4)

- #36 native key prompts: yes, starting with "K: unstuck".
- #37 objective reminder: yes.
- #38 sounds: yes. Use a **groan or gasp as the player gets back up** after a revive, not a UI beep. The game ships these per voice: `player/vocal/LowHealth/lhealth1-5.wav` and `player/player_out_of_breath.wav` (Simon), plus `player/police/...` and `player/Doctor/...` sets (player.cpp precache list). Play the set that matches the revived player's model, at the player, so everyone nearby hears it.
- #39 join notice: the game already prints it. Dropped.

## More interesting discoveries (round 5, not built)

| # | Idea | What the game already has | Value | Effort | Risk |
|---|---|---|---|---|---|
| 40 | **Native team HUD: "Name - Health 85 - Ping 40"** | The client already draws a co-op player list at the right of the screen, with a "follow me" icon and a syringe icon per player, and colours developers and donators (cl_dll/HUDControl_guess.cpp:1973-2050). It also has the **look-at name label**. Both only show when the map name contains `cof_` (line 1974), so our story maps hide them. One pinned in-memory string change in client.dll (as with the join box) would turn on the game's own team HUD. It must be hidden during scenes (the black box problem from #5). | Very high: real native team HUD, replaces #7 | S-M | Low-medium |
| 41 | **Voice callouts (L4D style)** | `adlib 0/1/2` is a working multiplayer command: it plays a random "Follow me" (7 lines), "Help" (5) or "Look at this" (9) voice line from player/police/adlib/, with a 1 s cooldown. It sends `AdLib`, which lights the follow icon next to that player in #40's list (client.cpp:1571, HUDControl_guess.cpp:2000). The sounds are precached in every multiplayer game (client.cpp:2816). It only needs key binds, for example Z / X / C. | Very high, almost free: works with today's servers once bound | S | Low (only police voices; check they fit Simon) |
| 42 | **Syringe icon for teammates** | The same list shows who carries a syringe (`m_bSyringe_guess`, `Syringe` message). On with #40. | Know who can heal | Free with #40 | Low |

#40 and #41 together give the mod a team HUD and voice callouts that look and sound like they were always in the game.

## Katz's picks (round 5)

- #40 native team HUD: **yes**. #42 syringe icon comes with it.
- #41 voice callouts: no.

## Last QoL pass (round 6, not built)

| # | Idea | What the game already has | Value | Effort | Risk |
|---|---|---|---|---|---|
| 43 | **Ready-up before a chapter or save starts** | The native co-op lobby: `readylobby` command, per-player `m_bReady`, the `Lobby` HUD message and the `sv_lobby_time` timeout of 60 s (multiplay_gamerules.cpp:810-920, client.cpp:967). It could replace our silent load wait with the game's own "who is ready" screen. Solo: the host is ready at once. | Clear start, no friend left loading | M | Medium (touches the load wait) |
| 44 | **"Busy" tag** | `m_iUsingComputerOrPhone` (1 = computer, 2 = phone) and the inventory and note states are per player (client.cpp:1143). Show "(phone)", "(reading)" or "(bag)" after the name in the #40 team HUD, or as a short notice. | Ends the "why aren't you moving" moments | S | Low |
| 45 | **Rejoin last game** | Launcher only: remember the last IP:port from the join box and offer one "Rejoin" button after a crash or disconnect. | Fast recovery after a crash | S | Low |

## Final picked list (what to build when Katz says go)

1. **#3 Server config file** `storycoop_server.cfg`, run at each map start.
2. **#4 Revive options** in the Create Server window: revive time (`sv_coopcprtime`) and auto respawn (`sv_auto_respawn_time`).
3. **#1 Unstuck on K**, shown with the game's own key prompt (**#36**).
4. **#6 "<player> is down"** text, in the game's notice style (#15).
5. **#38 Revive groan**: the revived player's own low-health or out-of-breath voice, matching their model.
6. **#37 Objective reminder** for lost or late players.
7. **#40 Native team HUD** (name, health, ping, syringe icon) on story maps, hidden during scenes.
8. **Comfort options**: #27 FOV slider, #28 head bob, #29 HUD-off / screenshot key.
9. **#32 Per-player subtitle language** kept across maps.
10. **#24 Spray logos** with their own setting page.
11. **#34 Custom hand textures** (sleeve, glove, skin) on the Hands page. Not picked yet, but it's the strongest customization find.

Still open for Katz: #43-45 above, and round 2 (#16 Hunted mode, #17 run timer, #18 scare button, #19 hardcore options, #20 vote kick), which he liked but hasn't picked.
