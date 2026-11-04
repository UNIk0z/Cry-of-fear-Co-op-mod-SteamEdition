# v0.8.1 test run

Nothing in v0.8.1 has run in the game yet. Run docs/V08_TEST.md first; these are the new checks. Host log lines start with `[STORYCOOP_RC4]`.

1. Two players, a map with two fuses (or any two copies of one key item): both pick theirs up at the same moment. Each inventory shows two (`KEYITEM ... also given to`).
2. One player drops a key item and picks it up again: the partner still has exactly one.
3. A player falls to his death (fall zone) while the partner picks up a key item: after the respawn he has it too.
4. A player is on a ladder when the other walks into a story teleport: the climber finishes the climb and is then moved beside the other (`STORYTELEPORT player=2 off the ladder`).
5. Quit to the menu while the partner is still loading, then start a new game alone: no "Waiting for players" freeze.
6. Launcher: Check for Updates on a v0.8 install offers v0.8.1 and installs the Windows ZIP, not the Linux file.