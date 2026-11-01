# v0.8 test run

Nothing in v0.8 has run in the game yet. Close Cry of Fear on both PCs. Every player opens "Cry of Fear Co-op.exe", clicks Install, then Run Co-op Mod. Host log lines start with `[STORYCOOP_RC4]`.

## Solo first

1. Host alone with 2 slots: each pickup shows once, and taking it leaves nothing behind (`ITEMS ... pickup(s) per player`).
2. Change maps: no "You got" lines. Save at a recorder: one sound.
3. Pick up M16 ammo past 3 magazines.
4. Player Model box: Done opens Hands. On the next map the log says `MODEL player=1 reports model ...`. The model, and the hands if you picked some, show in game.
5. `changelevel s4_apartmentsick_e0`: enter the Sawer fight (`AUTOSAVE boss fight ...: saved`). Die: red text, fade, restart at the start of the fight. Kill him, skip the scene: the next map loads (`BOSS scene over`).
6. Skip a scene with a voice line: no subtitle stays, and the view is not zoomed afterwards (`SKIP ... zoom(s) returned`).
7. s4_college1dark exit to s4_hamn3_e0: you land at 374 662 71 (`ARRIVAL ... placed`).
8. Ending wheelchair: the speed stays at the cap (`WHEELCHAIR speed ... held`).

## Two players

9. A takes an ammo box: it is gone for A only, and B takes his own.
10. A picks up a key: B gets it too and sees "You got the <item>" (`KEYITEM ... also given`). A uses it: B loses it. Fill B's inventory first: nothing drops (`no free slot`). A drops it: B keeps his.
11. Both choose models, then join: both show their own model on the host and the joiner.
12. Train crash: no weapons on the floor, 3 empty slots. One gets out: the other is moved beside him.
13. s4_subwaysick2_e0, doctor and police scene: both arrive at the station (`STORYTELEPORT 2teleportion moved player=N; partners moved=1`), and the maps do not swap.
14. One player starts a cutscene with the other far away: at its end both stand together (`SCENE over`).
15. One dies in a boss fight: game over for both, and both restart from the fight's start with what they had then.
16. Save the game, load it, and go back to a map you left: what each player took stays taken for him alone.
