[CmdletBinding()]
param([string]$SteamExe,[switch]$ShowCommand,[switch]$NativeRC4,[switch]$MenuOnly,[ValidateSet('cof_campaign_story2','cof_campaign_story3','s3_start','cof_campaign_story4','s4_start','s4_start_e0','s4_start_e2')][string]$Map='cof_campaign_story2',[ValidateRange(0,3)][int]$ContinueSlot=0,[string]$GameDirectory='D:\SteamLibrary\steamapps\common\Cry of Fear\cryoffear')
$ErrorActionPreference='Stop'
# coop-fixes-008: on another PC the game lives elsewhere; find it through Steam.
# v0.5: Steam's own copy of the game first; the developer default only when Steam knows none
# (a stale D:\ copy used to win and the launcher reported "not installed").
if (-not $PSBoundParameters.ContainsKey('GameDirectory') -and [IO.File]::Exists((Join-Path $PSScriptRoot 'find-cof.ps1'))) {
    . (Join-Path $PSScriptRoot 'find-cof.ps1'); $found=Find-CofGameDirectory; if ($found) { $GameDirectory=$found }
}
$launchMap=$Map
# coop-fixes-005: open the game's real main menu with the co-op adapter loaded.
# Custom Campaign > "Simon's Story: ..." then hosts a two-player game (the
# adapter sets maxplayers 2 itself). No map, no slot.
if ($MenuOnly -and ($ContinueSlot -or $PSBoundParameters.ContainsKey('Map') -or $NativeRC4)) { throw 'MenuOnly takes no map, slot or -NativeRC4.' }
if ($ContinueSlot) {
    . (Join-Path $PSScriptRoot 'read-coop-checkpoint.ps1')
    $checkpoint=Read-CoopCheckpointSlot $GameDirectory $ContinueSlot
    if ($PSBoundParameters.ContainsKey('Map') -and $Map -cne $checkpoint.Map) { throw 'Explicit -Map differs from checkpoint. Omit -Map when continuing.' }
    # -Map keeps its launch whitelist; a checkpoint may name any RC2/RC4 alias.
    $launchMap=$checkpoint.Map; $NativeRC4=$true
    if (-not [IO.File]::Exists((Join-Path $GameDirectory ('maps\'+$launchMap+'.bsp')))) { throw ('Saved map '+$launchMap+' is not installed. Install the full RC2 campaign and this package first.') }
    if ($checkpoint.Backup) { Write-Warning 'Primary checkpoint unavailable; selecting its valid backup. The adapter will revalidate it.' }
}
if (-not $SteamExe) {
    $SteamExe=Join-Path ([string](Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam' -Name SteamPath).SteamPath) 'steam.exe'
}
if (-not [IO.File]::Exists($SteamExe)) { throw 'Steam.exe not found. Supply -SteamExe with its existing Windows path.' }
# Pinned Steam DLL worldspawn10121083 reads mp_footsteps for the lobby deadline,
# despite registering sv_lobby_time separately. <=1 disables its forced-start
# branch. Set before map initialization; changing it after load is too late.
$isRC4=($launchMap -cin @('cof_campaign_story4','s4_start','s4_start_e0','s4_start_e2')) -or [bool]$ContinueSlot
if (-not $MenuOnly -and [bool]$NativeRC4 -ne $isRC4) { throw 'RC4 aliases require -NativeRC4; the adapter is limited to the RC4 launcher.' }
if ($MenuOnly) { $NativeRC4=$true }
# -condebug logs the host console from boot into qconsole.log (evidence for
# Continue staging and adapter diagnostics without typing condebug).
$launchArguments=@('-applaunch','223710','-console','-dev','-condebug')
if ($MenuOnly) { $launchArguments=@('-applaunch','223710','-condebug') }
# coop-fixes-013: the Steam display name instead of config.cfg's "Unknown"
# (find-cof.ps1), and a listen server without VAC: client.dll's menu is patched
# by this mod.
$persona=$null
if ($MenuOnly -and [IO.File]::Exists((Join-Path $PSScriptRoot 'find-cof.ps1'))) { . (Join-Path $PSScriptRoot 'find-cof.ps1'); $persona=Get-SteamPersonaName }
# v0.1: the game kept config.cfg's "Unknown" despite +name; the host's adapter
# now shows Steam names itself and uses this sc_name userinfo key as a fallback.
if ($MenuOnly) { $launchArguments+='-insecure'; if ($persona) { $launchArguments+=@('+name',('"'+$persona+'"'),'+setinfo','sc_name',('"'+$persona+'"')) } }
# v0.7 (player model menu): custom models from the mod's "playermodels" folder
# (next to "Run Co-op Mod.cmd": name.mdl or name\name.mdl) are copied to
# cryoffear\models\player\<name>\<name>.mdl, the only place the game reads
# player models; a model already there is never replaced. The model picked in
# the game's Player Model box (cryoffear\storycoop_model.txt) is sent again.
if ($MenuOnly) {
    $modelSource=Join-Path (Split-Path -Parent $PSScriptRoot) 'playermodels'
    if ([IO.Directory]::Exists($modelSource)) {
        foreach ($file in @(Get-ChildItem -LiteralPath $modelSource -Recurse -Filter '*.mdl' -File -ErrorAction SilentlyContinue)) {
            $name=[IO.Path]::GetFileNameWithoutExtension($file.Name).ToLowerInvariant(); $owner=$name
            # A model's separate texture file (<name>T.mdl) goes beside it.
            if ($name.Length -gt 1 -and $name.EndsWith('t') -and [IO.File]::Exists((Join-Path $file.DirectoryName ($name.Substring(0,$name.Length-1)+'.mdl')))) { $owner=$name.Substring(0,$name.Length-1) }
            if ($owner -cnotmatch '^[a-z0-9_-]{1,32}$' -or $owner -cmatch '^(simon|police[1-4])$') { continue }
            $target=Join-Path $GameDirectory ('models\player\'+$owner+'\'+$name+'.mdl')
            if ([IO.File]::Exists($target)) { continue }
            try { [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)); [IO.File]::Copy($file.FullName,$target); Write-Output "Player model added: $name" } catch { Write-Output "Player model $name could not be copied: $($_.Exception.Message)" }
        }
    }
    $modelChoice=Join-Path $GameDirectory 'storycoop_model.txt'
    if ([IO.File]::Exists($modelChoice)) { $chosen=([IO.File]::ReadAllText($modelChoice)).Trim().ToLowerInvariant(); if ($chosen -cmatch '^[a-z0-9_-]{1,32}$') { $launchArguments+=@('+setinfo','sc_model',$chosen) } }
}
if ($NativeRC4) { $launchArguments+=@('-dll','cl_dlls/storycoop_rc4.dll') }
# The game's own liblist.gam asks for these (commandargs). The full RC4
# campaign overlay reaches 1,202 map entities (s4_sewer3); pass them explicitly
# so a Steam -applaunch never falls back to the engine default edict limit.
# v0.5: the same heap the game's own launcher asks for (decomp coflaunchapp
# Form1.cs: -basedir cryoffear -game cryoffear -hw -heapsize 1536000), not less.
if ($NativeRC4) { $launchArguments+=@('-num_edicts','4096','-heapsize','1536000','-hw') }
# User log: the server capped P2 at sv_maxupdaterate 30 although P2 asked for
# 100. Raise only the server-side cap; each client keeps its own settings.
if (-not $MenuOnly) {
$launchArguments+=@('+maxplayers','2','+mp_footsteps','1','+sv_maxupdaterate','100','+sv_lan','0')
if ($ContinueSlot) { $launchArguments+=@('+sc_checkpoint_prepare',[string]$ContinueSlot) }
$launchArguments+=@('+map',$launchMap)
}
if ($ShowCommand) { Write-Output ('"'+$SteamExe+'" '+($launchArguments -join ' ')); return }
if (Get-Process -Name 'cof','cryoffear' -ErrorAction SilentlyContinue) { throw 'Close Cry of Fear normally before launching this Steam session.' }
# v0.3: Steam running as administrator ignores a launch request from a normal window.
if ($MenuOnly -and (Get-Command Test-SteamElevated -ErrorAction SilentlyContinue) -and (Test-SteamElevated) -and -not (Test-CofAdmin)) { Invoke-CofElevated $PSCommandPath @('-MenuOnly') 'Steam is running as administrator, so the game must be started as administrator too (or restart Steam normally).' }
if ($NativeRC4 -and -not [IO.File]::Exists((Join-Path $GameDirectory 'cl_dlls\storycoop_rc4.dll'))) { throw ('The co-op mod is not installed in '+$GameDirectory+'. Run "Install.cmd" first (game closed), then start it again.') }
# coop-fixes-012: the launcher checks which co-op version this PC has. v0.5: a
# different installed version is named and still started (it was reported as
# "not installed" although the game ran it); Install.cmd from this folder updates it.
if ($MenuOnly -and [IO.File]::Exists((Join-Path $PSScriptRoot 'release.json'))) {
    $release=Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'release.json') | ConvertFrom-Json
    $sha=(Get-FileHash -LiteralPath (Join-Path $GameDirectory 'cl_dlls\storycoop_rc4.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sha -cne $release.runtimeAdapter.sha256) {
        $name='an older or unknown version'
        if ([IO.File]::Exists((Join-Path $PSScriptRoot 'known-builds.json'))) { $known=@((Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'known-builds.json') | ConvertFrom-Json).builds | Where-Object { $_.adapter -ceq $sha }); if ($known.Count) { $name='co-op '+[string]$known[0].id } }
        Write-Warning ('This PC runs '+$name+', not co-op '+$release.coopFixes.id+' from this folder. Starting it anyway. To update: close the game and run "Install.cmd" from this folder. Both players need the same version.')
    }
}
if (Get-Command Write-CofAddonWarning -ErrorAction SilentlyContinue) { Write-CofAddonWarning $GameDirectory }
# -condebug restarts qconsole.log each launch: keep the previous session's log
# (the last 20) so a later launch never destroys test evidence.
$hostLog=Join-Path (Split-Path ([IO.Path]::GetFullPath($GameDirectory))) 'qconsole.log'
if ([IO.File]::Exists($hostLog)) {
    $archive=Join-Path $GameDirectory 'storycoop_logs'; [void][IO.Directory]::CreateDirectory($archive)
    Copy-Item -LiteralPath $hostLog -Destination (Join-Path $archive ('qconsole-'+[IO.File]::GetLastWriteTime($hostLog).ToString('yyyyMMdd-HHmmss')+'.log')) -Force
    Get-ChildItem -LiteralPath $archive -Filter 'qconsole-*.log' | Sort-Object Name -Descending | Select-Object -Skip 20 | Remove-Item -Force
}
if ($ContinueSlot) { $null=Write-CoopContinueRequest $GameDirectory $ContinueSlot $launchMap }
elseif ($NativeRC4 -and -not $MenuOnly) { . (Join-Path $PSScriptRoot 'read-coop-checkpoint.ps1'); if ([IO.Directory]::Exists((Join-Path $GameDirectory 'storycoop_saves'))) { Remove-CoopContinueRequest $GameDirectory } }
# Candidate native Steam listen-server launch. Actual lobby/intro operation is
# awaiting gameplay. RC4 optionally loads our separate SDK forwarding adapter.
# v0.4: the co-op menu starts cof.exe directly (find-cof.ps1 Start-CofGame), so
# the game's own CoFLaunchApp window (resolution + LAUNCH) does not wait for a click.
if ($MenuOnly -and (Get-Command Start-CofGame -ErrorAction SilentlyContinue)) { $null=Start-CofGame $GameDirectory @($launchArguments | Select-Object -Skip 2) }
else { Start-Process -FilePath $SteamExe -ArgumentList $launchArguments -WindowStyle Hidden }
if ($MenuOnly) { Write-Output 'Starting Cry of Fear with the co-op mod. In the main menu choose "Host Simon''s Story", then "Simon''s Story: New Game" or "Load Slot 1/2/3". Your friend opens the same menu with "Run Co-op Mod.cmd" and types in the console (~): connect <your IP>:27015' }
else { Write-Output "Launch requested through Steam. Confirm status shows 2 max players and $launchMap; follow TESTING.md." }
