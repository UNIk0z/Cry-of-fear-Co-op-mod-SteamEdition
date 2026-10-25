# v0.5.6 test run

Both players: close the game, run "Install.cmd" from the v0.5.6 folder, then
"Run Co-op Mod.cmd". The host sends the newest
cryoffear\storycoop_logs\rc4-*.log.

1. **Host a chapter, friend joins.** Play for a minute side by side.
2. **Host watches the friend:** have them walk past you, strafe and turn on
   the spot. Before: small constant shaking. Expect smooth movement.
3. **Friend watches the host** the same way.
4. **Friend's console (~):** type `cl_cmdrate`. Expect `100`.
5. **Host log:** look for `NET tuned player=2 ... cl_cmdrate=100` and the
   `NET_CADENCE player=2` lines. Note the "host frames" numbers.
6. If it still shakes, say who shakes, for whom, and whether it is constant
   or comes and goes, and send the log.
