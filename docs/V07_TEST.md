# v0.7 test run

Solo first, then two players. Send the host's `cryoffear/storycoop_logs/rc4-*.log`.

1. "Run Co-op Mod.cmd": the menu shows Host, Join, Player Model, Options, Quit, with the credit line "v0.7". The log has no `MENU co-op menu off`. **Quit closes the game.** Starting the game normally shows the game's own menu.
2. Player Model: the box shows a preview, the name and the number; nothing can be typed. Next goes through Simon, the 11 outfits and your own models and back to Simon; Done closes it; opening it again shows the same model. Join still opens the join box with its own picture and fields afterwards.
3. Put a `.mdl` in `playermodels`, start "Run Co-op Mod.cmd": "Player model added: <name>"; Next reaches it with "No preview".
4. Choose an outfit, host, and have a friend join: the friend sees you in it (`MODEL player=1 now simon_...`); your own hands use its sleeves if it has them; each of you in a different outfit stays different. A model only the joiner has: Simon, and a short notice to that player.
5. Use a tape recorder as host and double-click slot 2: "Saving progress..." then "Saved"; Load-Save-Slot-2 loads it (`Checkpoint slot2 COMMITTED`, no "refused"). A single-player Load Game still shows your own saves.
6. University: enter the dark college, leave through the emergency door, come back. The faceless enemies move, attack and die when shot (`MAPSTATE restored ... recreated=N`). Then revisit another map with spawned enemies (city, subway).
7. Code-locked elevator: enter the code, use the elevator door. The scene of Simon entering and closing the door plays for every player, then you continue where the elevator stops (`ARRIVAL player=1 from apartment3`, `ARRIVAL fires the trigger ... multi_hiss_1`).
8. City, door to the bus map: come back from the bus map, then go back to that door. It is unlocked (`ARRIVAL fires the trigger ... untriggerlock`).
9. Repeat the v0.6.3 checks (docs/V063_TEST.md 1, 3-9).
