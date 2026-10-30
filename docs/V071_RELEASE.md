# Cry of Fear co-op v0.7.1 (2026-10-30)

v0.7 plus these fixes. Maps byte-identical; adapter only. Native tests pass.

| Report | Cause (evidence) | Change |
|---|---|---|
| A joining tester on s4_start: 1+2 for knife and phone does nothing, the TAB menu works | Not fixed in v0.7. A rule from v0.5.7 kept the phone out of the two-key path only (story-runtime.hpp); the TAB path never had it. Single player allows the pair (decomp player.cpp CheckQuickDualWield, GiveDualWeapon puts the light in the left hand). | The phone pairs again. |
| Same report, for any two weapons of a joining player | The host measures the time between the two keys. A joining player's game holds the second command until the first is acknowledged, so it arrives about one ping later, which can miss the 0.12 s limit. This comes from how the engine's network channel sends reliable commands; it was not measured. | The limit is 0.12 s plus the player's ping (engine GetPlayerStats), at most 0.3 s. The host's own limit stays 0.12 s. A near miss logs `DUAL quick ... apart, limit ...: a weapon switch`. |
| Found while reading the single-player code | The phone rule also kept a pair from touching a phone call. | No pair while the native key does nothing: frozen, on a ladder, fuser3 10, in or about to get a phone call. |
| Katz: "skip didnt work mid cutscene it must be always available at any point" | The real miss, found in the second review: **scenes on a model camera** (cof_mdlcutscene) use no trigger_camera, so R counted for nothing and the prompt was gone. 22 in the story maps: the Doctor's stomp (s4_attic demdlcam, right after skippable multidydoc; the 2026-10-28 log says `runs on after a skip: demdlcam`), the train scene from 7 s on (s4_subway2st3), the endings. | R counts on them too. They end by their own end path; what they fire at their end is ended at once for 2 s; a scene with one runs all its timers out. |
| Same report: other ways a press could be lost, from the code | A camera the scene's list did not know started over with no votes. A player the camera did not hold was needed for N/N but could not vote. A quick tap could fall between 50 ms key polls. R did not run at all in a game whose engine or GameUI build is not the pinned one, or if the menu hold patch failed; that player's missing vote then blocked everybody. In Katz's test window my wait script held back every queued command, R included, as it did Quit. | A new camera joins the scene with its votes. Only players the camera holds count. The poll reads the "pressed since last time" bit and no longer depends on the pause guard or the hold patch. |
| Audit of all 292 scenes with a camera in the story maps | Shots overlap by design (they end together), so a moment with no camera in the middle of a scene does not happen: the only gaps are 25 to 62 s of play on s4_bridge multikuken and 37 s on the s4_subway1st1 lever. The gap handling from the first fix is only a safety net; I overstated it to Katz and corrected that. | Kept: 0.5 s after a camera, up to 5 s when a camera is due. |
| Also found | The prologue prompt showed once for 3 s although R works for the whole prologue. My first fix let the stats camera become skippable when a scene ran straight into it. The wardrobe's held camera showed a prompt R cannot honour. | Prompt repeats. Stats camera stays out. A held camera no scene owns is not offered. |

Still not skippable: scenes with a yes/no choice still to come (Katz's wish, v0.5.7), the stats camera, the wardrobe.

**Not run in the game at all** (Katz, 2026-10-30: stop testing): neither fix, and a joining player's real ping. See V071_TEST.md.

The v0.5.7 clip ("idk why it went in my left hand") can come back: pressing 1 and 2 almost
together gives phone and knife, as in single player.
