# Map state — v0.10.6 update (2026-11-13)

slotN.cofmap is unchanged (version 3). Movewith groups (a train or door and the
entities riding on it) are now saved and restored whole instead of kept as served
(state-runtime.hpp state_group); a group with a member that leads out stays as
served whole. Files from v0.10.5 and older have no record of these groups: a
func_train whose one-shot trigger chain was used on the kept visit is put at the
corner where that trigger left it (state_repair_trains, log `MAPSTATE repaired`).
See V0106_RELEASE.md. Not run in game.

# Save system — coop-fixes-004 update

Checkpoint schema 3 (COFCKP01 version 3) appends the shared keypad code
(player+0x2040 code, +0x2044 type) after the schema-2 world lists; schemas 1 and
2 still load and the PowerShell reader accepts 1..3. Converted doors that are
unlocked once keep their state in persistent env_global records (carried by the
native global stream); doors locked and unlocked in sequence are per-visit like
the native door. A new campaign (prologue) resets every persistent door state.

# Save system — coop-fixes-003 update

Checkpoints are accepted at any tape recorder on every scoped s2_/s4_ alias
except s4_nightmare (native/rc4/checkpoint.cpp checkpoint_map). File schema 2
(COFCKP01 version 2) appends per-player poses, taken pickups (BSP index +
class) and key-unlocked doors; schema 1 files still load. c_start aliases keep
recorders 419/421 and the authored sc4_checkpoint_* landing markers. Elsewhere
players land at their saved spot or the nearest safe spot (TraceHull). Restored
players get the recorder reminder flag (player+0x21D8). The Continue launcher
accepts any installed alias named by the save.

# Save system entrypoint

Canonical implementation and limitations: [RC4_CHECKPOINT.md](RC4_CHECKPOINT.md).
Latest test was INTERRUPTED by the cinematic softlock, not a save failure.
Serialization and owned runtime fixtures pass; a full two-client process restart
restore remains required. Source: native/rc4/checkpoint.hpp, checkpoint.cpp and
checkpoint-runtime.hpp. Do not maintain a second schema or duplicate implementation.
coop-fixes-001: no console binding is needed. Identities are automatic (Steam,
or auto:host/auto:guest), restore can match by role, and the Continue request
file plus -condebug are added; see the top of RC4_CHECKPOINT.md.
campaign-doors-001 adds reason-coded refusal, WAIT and FAILED lines plus per-slot
save/restore summaries (see the top of RC4_CHECKPOINT.md). In a failed Continue,
copy the whole `[STORYCOOP]` line: the text in parentheses names the failed step.
