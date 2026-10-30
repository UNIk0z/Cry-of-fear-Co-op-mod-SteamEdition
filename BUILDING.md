# Building and testing

Everything runs on Windows. You need your own Steam copy of Cry of Fear 1.6a:
the adapter is built against its `cryoffear\cl_dlls\hl.dll`, and the maps are
rebuilt from its original BSPs. Nothing from the game goes into this repository.

## Tools

- Visual Studio 2022 with the x86 MSVC toolset **14.34.31933** and Windows SDK
  **10.0.26100.0**. `native/rc4/build.ps1` and `native/rc4/test.ps1` contain
  these paths at the top; change them if your versions differ.
- Node.js 20 or newer (scripts and tests; no npm packages are needed).
- Windows PowerShell 5.1 (installer, launchers, build scripts). Keep `.ps1`
  files ASCII: PowerShell 5.1 reads BOM-less UTF-8 as ANSI.

## Build the adapter

```
powershell -NoProfile -ExecutionPolicy Bypass -File native/rc4/build.ps1 -NativeDll "<game>\cryoffear\cl_dlls\hl.dll" -OutputDirectory <new empty folder>
```

The script refuses any other hl.dll (SHA256 pinned) and any existing output
folder. Output: `storycoop_rc4.dll` and `build-report.json` (source hashes).
Two builds of the same source are not byte-identical (the compiler and linker
stamp them), and the source hashes change with line endings (CRLF or LF). The
released v0.5.1 adapter has SHA256 `a1181dbf...`.

## Native tests

```
powershell -NoProfile -ExecutionPolicy Bypass -File native/rc4/test.ps1 -Adapter <build folder>\storycoop_rc4.dll -OutputDirectory <new empty folder>
```

It builds the 15 fixture programs from `native/rc4/test-*.cpp` and runs them.
No game DLL is loaded and the game is not started. Expect
`PASS all 15 native fixture suites`.

## Node tests

```
npm test
```

Tests that need a built package or map folder skip unless you set its
environment variable. For example, `COF_V04_MAPS` is the v0.4 map build that
`tests/v05.test.mjs` uses. Each test file names its variables at the top.

## Maps and packages

- Each release's map pass takes the previous release's map build as input.
  For example: `node scripts/build-v05-maps.mjs <v0.4 map build> <game>\cryoffear\maps <new folder>`.
  Each map is reconstructed from the original BSP and its plan, checked byte
  for byte, then edited.
- `scripts/package-state-rc4.mjs <map build> <rc2 package> <adapter dll> <new folder> <commit> <previous package> v05`
  makes the developer package. `scripts/player-package.mjs <package> <new folder>`
  makes the player folder that is zipped for release. docs/V05_RELEASE.md and
  the earlier V0x_RELEASE.md files list each release's inputs.

## Running a build

Either install a player package (Install.cmd), or copy your
`storycoop_rc4.dll` to `<game>\cryoffear\cl_dlls\` and start
`cof.exe -game cryoffear -dll cl_dlls/storycoop_rc4.dll -num_edicts 4096 -heapsize 1536000`
with the co-op maps installed. Each session's adapter log is written to
`cryoffear\storycoop_logs`.

## Rules the code keeps

- No detours and no calls into game code except `CBasePlayer::DualWield`.
- Never fan out a native `trigger_camera` Use per player (it already handles
  every client).
- Opening maps (`cof_campaign_story4`, `s4_nightmare`) stay passive.
- A Cry of Fear decompilation can help with functions, structs and offsets.
  Do not copy its code into this repository.
