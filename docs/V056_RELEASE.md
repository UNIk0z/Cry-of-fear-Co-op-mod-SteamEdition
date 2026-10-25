# Cry of Fear co-op v0.5.6 (2026-10-24)

v0.5.5 with the partner jitter fixed (testers 2026-10-24: "the connecting
player has a small constant jittering ... like you would have shivering
hands"; a second tester: "comes and goes, sometimes for one, sometimes for
both"). Maps byte-identical; adapter only. Native tests pass; **not run in
the game, two players not tested.**

| Report | Cause (evidence) | Change |
|---|---|---|
| The joining player shakes slightly all the time | The joining player's game sent its moves at the engine default `cl_cmdrate 30` (CL_MAIN.C; the mod never set it). The server moves a remote player only when one of those packets arrives (CL_SendCmd waits `1/cl_cmdrate` between packets; the host's own moves go over loopback every frame). The host runs at `fps_max 72` (every host log) and sends a picture of the world every frame, so the partner moved in one of every 2-3 frames and stood still in the others: small steps, also in the turning of the body and arms. Earlier IDLE_DIAG logs had already shown that the server writes nothing to a still partner, so it was not a server-side correction. | The joining player is now asked once per map for `cl_cmdrate 100`, next to the existing `rate 100000`, `cl_updaterate 100` and `ex_interp 0.06`. Their game then sends one packet every frame (their frame rate is the real limit). |
| Measuring it | Nothing in the log showed how often the partner's moves arrived. | New host log line every 10 s for each joined player: `NET_CADENCE player=2 N commands arrived in F host frames (up to M at once)`. Before the fix F is about 300 in 10 s (30 a second); after it F should be close to the host's frame rate (about 720 at 72 fps) and M mostly 1 or 2. |

What may still show: if the host's frame rate drops, or the network delivers
packets in bursts (VPNs, Wi-Fi), a player can still step now and then. The
NET_CADENCE line tells which side it is.

The setting stays in the joining player's own config like the earlier rate
settings (`cl_cmdrate 30` restores the default).
