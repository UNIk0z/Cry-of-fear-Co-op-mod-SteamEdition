<div align="center">

# Cry of Fear Co-op (Steam Edition)

**Play Simon's single-player story in Cry of Fear with your friends.**

**Not to be confused with COF Enhanced Coop Mod for Xash3D, this mod is for the latest Steam Version.**

![Status: early test build](https://img.shields.io/badge/status-early%20test%20build-orange?style=for-the-badge)
![Players: 4](https://img.shields.io/badge/players-4-555?style=for-the-badge)

[![Discord](https://img.shields.io/badge/Discord-report%20bugs-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/hgEGSf9MFZ)

</div>

## ⬇️ Download

### **[Click here to download Cry of Fear Co-op v0.7](https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/download/v0.7/Cry.of.Fear.Co-op.v0.7.zip)**

> [!IMPORTANT]
> **Don't use the green "Code" button or "Download ZIP".** That gives you the
> source of this page, not the mod.

1. Click the download link above. Your browser saves the mod's ZIP
   (**`Cry.of.Fear.Co-op`** and the version).
   (If it doesn't start, open the [latest release page](https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/latest), scroll down
   to **Assets** and click the **`Cry.of.Fear.Co-op`** ZIP.)
2. Right-click the ZIP and choose **Extract All**.
3. With Cry of Fear closed, run **`Install.cmd`** (every player).
4. To play, run **`Run Co-op Mod.cmd`**. See [Play](#2-play) below.

> [!WARNING]
> **This mod is unfinished,**
> up to four players, and you will run into bugs.
> Please report them on the Discord (see [Reporting bugs](#reporting-bugs)).

Cry of Fear's own co-op mode is a separate campaign. This mod lets up to four players
play **Simon's single-player story** together instead.

## How it works

| | |
|---|---|
| 🚪 **Story maps for everyone** | All players spawn together, and doors and level changes take everyone along. Cutscenes play for everyone, and story weapons and items are given to every player. |
| 🔢 **Shared progress** | Codes, notes and objectives are shared, so any player can enter a keypad code. |
| 🎬 **Start at any chapter** | Pick the Prologue, any chapter from 1 to 7 or a save. A chapter starts where its title appears, with the weapons Simon has found by then. |
| 🎒 **Same story rules** | Item puzzles (rope, lighter, saw, fuse, scissors...) work for every player. After the train crash every player loses the bag and has 3 slots, as in the story. The Doctor's shot lowers max health for every player. |
| 📦 **Items for every player** | The first time you enter a map, ammo, syringes and weapons get one copy for each extra player, placed side by side. Keys and story items are not copied, but the others are told who picked one up. You can carry up to 10 syringes. |
| 🧩 **Two-half items** | When the two halves of an item are picked up by different players, they go to one player so they can be combined. |
| 🗺️ **Maps keep their state** | A map you go back to stays as you left it: taken items, dead enemies, opened doors and solved puzzles. |
| 🪜 **Ladders for everyone** | Every player can climb the same ladder at the same time. |
| 🧍 **Player models** | Pick Simon, one of his 11 outfits or your own model in the *Player Model* box, which shows a preview. The others see your outfit too. Your own model shows for them when the host has the same file. |
| 🔓 **Unlock extras** | A host option that opens the Chapter 1 secret room with every unlockable. |
| 📼 **Saving** | Only the host can save. The host uses a tape recorder and picks one of the 5 slots in the game's save window. Saves are kept on the host's PC. |
| ✅ **Your game stays original** | The mod only runs when you start it with `Run Co-op Mod.cmd`. Starting Cry of Fear normally gives you the original game. |

## What you need

- **Cry of Fear** from Steam, on every PC.
- A way to reach each other's PC, for example **Radmin VPN** or a forwarded
  port **27015**.

## Quick start

The download has these files:

| File | When to use it |
|---|---|
| `Install.cmd` | Once per new version, with the game closed |
| `Check for Updates.cmd` | To see if a newer version is out (it can download it for you) |
| `Run Co-op Mod.cmd` | **Every time you play** |
| `Uninstall.cmd` | To remove the mod |
| `READ ME.txt` | The same instructions as this page |
| `playermodels` folder | To add your own player models (see [Play](#2-play)) |

### 1. Install (every player)

1. Download the ZIP from the link at the top and unzip it anywhere.
2. Close Cry of Fear and run **`Install.cmd`**. It finds the game
   through Steam, moves any older version of the mod aside and installs this one.

### 2. Play

Every player runs **`Run Co-op Mod.cmd`**.

- **Host:** click *Host Simon's Story*. The game's **Create Server** window
  opens: pick where to start (*Chapter-0-Prologue* up to
  *Chapter-7-Only-Safe-at-Home*) or a save (*Load-Save-Slot-1* to *5*), set the
  number of players (2 to 4), the difficulty, player collision and *Unlock
  extras*, and click **Start**. A chapter starts where its title card shows, with the weapons
  Simon has found by then. Give your friends your address (for
  example your Radmin VPN IP).
- **Friend:** click *Join Simon's Story*. A box opens: type the host's IP
  address, then the port (leave it empty for 27015), and click **Join**. Typing the
  address with its port, like `192.168.1.1:27015`, works too. You can
  also press **`~`** to open the console and type `connect`, then the host's
  IP address and `:27015`. For example:

  ```
  connect 192.168.1.1:27015
  ```
- **Player model (optional):** click *Player Model*. The box shows a preview of
  each model: Simon, his 11 outfits from the game, then your own. **Next** shows
  the next model and **Done** closes the box with the one you see. Your choice is
  kept for the next start. To add your own model, put its `.mdl` file (made for
  the Cry of Fear player skeleton) in the `playermodels` folder next to
  `Run Co-op Mod.cmd` and start the mod again; it shows "No preview" in the box.
  The others see your own model when the host has it too, so the host needs the
  same file in their `playermodels` folder. Players who join download it.

> [!TIP]
> **Skip a cutscene:** press **R** (the screen shows "Press R to skip"). When every player has pressed it, the scene
> jumps to its end. Yes/no choices, trains, boss fights and the endings cannot be skipped.
>
> **Stuck somewhere?** Press **Y** (chat) and type `stuck`. You are moved next
> to another player.

### 3. Uninstall

Close the game and run **`Uninstall.cmd`**. The mod's files are moved to
*Cry of Fear - removed co-op files*, next to the game folder. Nothing is
deleted, and your co-op saves are kept.

## Reporting bugs

**Join the Discord:** https://discord.gg/hgEGSf9MFZ

Every bug report needs the **log file**. That's how we can see what happened.

1. In Steam, right-click **Cry of Fear** > **Manage** > **Browse local files**.
2. Open the folder **`cryoffear\storycoop_logs`**.
3. Sort by **Date modified** and take the **newest** `rc4-<date>-<time>.log`
   from the session where the bug happened.
   - **The host's log is the most important one.**
   - If you can, also send the friend's log from the same session.
4. Post it in the Discord bug-report channel, with:
   - the place or map (open the console with `~` and type `status`; the map
     name is shown there);
   - what you did;
   - what went wrong, and a screenshot if you have one.

## Current Known limits

- Most chapter starts have been tried alone. Most of the campaign has not been
  played through in co-op yet.
- The endings (the strangle scenes and the final wheelchair boss) are still
  designed around player 1.
- Some particle effects may only show for the host.
- Three and four players have not been tested yet.
- Open bugs: a crawler enemy is missing in the subway, and crouching on a save
  station jitters.
- Fixed in v0.7 but not yet tried in a co-op session: the university enemies,
  the elevator scene after entering the code and the door opened from the other
  side. The outfits have only been tried alone so far.

## Credits

Co-op mod developed by **Anas Sammak (koolkat)**.
Special thanks: **Mesilll**, **Michal Eminov**, **caixa**, **Joah230**.

Cry of Fear is made by Team Psykskallar. This is an unofficial fan mod and
requires the original game from Steam.

## For developers

This ZIP is the mod's source code. [BUILDING.md](BUILDING.md) explains the tools,
the build and the tests; the release notes are in [docs](docs).
