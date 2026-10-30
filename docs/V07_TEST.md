# v0.7 test run

Solo first, then two players. Send the host's `cryoffear/storycoop_logs/rc4-*.log`.

1. Main menu: Host, Join, Player Model, Options, Quit; each button does what it says.
2. Player Model: the box lists the models; type one, Log on. Start a game: you look like that model to your partner if the host has it (`MODEL player=... now ...`).
3. Put a `.mdl` in `playermodels`, start "Run Co-op Mod.cmd": "Player model added: <name>"; it shows in the box.
4. Use a tape recorder as host and double-click slot 2: "Saving progress..." then "Saved"; Load-Save-Slot-2 loads it (`Checkpoint slot2 COMMITTED` in the log, no "refused").
5. Repeat the v0.6.3 checks (docs/V063_TEST.md 1, 3-9).
