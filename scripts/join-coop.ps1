[CmdletBinding()]
param([string]$HostAddress,[string]$GameDirectory)
# The friend's side (J). Checks that this PC has the same co-op
# version as release.json (Install in the launcher first), then starts Cry of Fear
# through Steam and connects to the host's address.
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'find-cof.ps1')
if (-not $GameDirectory) { $GameDirectory=Find-CofGameDirectory }
if (-not $GameDirectory) { throw 'Cry of Fear was not found in any Steam library.' }
$catalog=Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'release.json') | ConvertFrom-Json
$first=$catalog.maps | Where-Object { $_.map -ceq 'cof_campaign_story4' }
$map=Join-Path $GameDirectory 'maps\cof_campaign_story4.bsp'
$digest=[Security.Cryptography.SHA256]::Create()
$ok=[IO.File]::Exists($map) -and ([BitConverter]::ToString($digest.ComputeHash([IO.File]::ReadAllBytes($map))).Replace('-','').ToLowerInvariant() -ceq $first.outputSha256)
# The maps can be identical between builds; the adapter tells the versions apart.
$dll=Join-Path $GameDirectory 'cl_dlls\storycoop_rc4.dll'
$ok=$ok -and [IO.File]::Exists($dll) -and ([BitConverter]::ToString($digest.ComputeHash([IO.File]::ReadAllBytes($dll))).Replace('-','').ToLowerInvariant() -ceq $catalog.runtimeAdapter.sha256)
$digest.Dispose()
if (-not [IO.File]::Exists($dll)) { throw ('The co-op mod is not installed in '+$GameDirectory+'. Close Cry of Fear and click Install in "Cry of Fear Co-op.exe" in this folder first.') }
if (-not $ok) { Write-Warning ('This PC has a different co-op version than this folder ('+$catalog.coopFixes.id+'). Joining anyway; if the game refuses, close it and click Install in "Cry of Fear Co-op.exe" in this folder. Both players need the same version.') }
Write-CofAddonWarning $GameDirectory
if (-not $HostAddress) { $HostAddress=Read-Host 'Host address (the IP your friend gives you, for example 26.171.21.225)' }
$HostAddress=$HostAddress.Trim()
if ($HostAddress -notmatch '^[A-Za-z0-9.\-]{1,253}(:[0-9]{1,5})?$') { throw 'That does not look like an address. Use something like 26.171.21.225 or 26.171.21.225:27015.' }
if ($HostAddress -notmatch ':') { $HostAddress+=':27015' }
if (Get-Process -Name 'cof','cryoffear','hl' -ErrorAction SilentlyContinue) { throw 'Cry of Fear is already running. Close it, or type "connect '+$HostAddress+'" in its console (~).' }
$steam=Find-SteamExe; if (-not $steam) { throw 'Steam was not found.' }
# v0.3: Steam running as administrator ignores a launch request from a normal window.
if ((Test-SteamElevated) -and -not (Test-CofAdmin)) { Invoke-CofElevated $PSCommandPath @('-HostAddress',("'"+$HostAddress.Replace("'","''")+"'")) 'Steam is running as administrator, so the game must be started as administrator too (or restart Steam normally).' }
# v0.1: the co-op adapter is loaded on the friend's side too (-dll), so the
# friend gets the co-op main menu and the player list; a normal launch of the
# game keeps the game's own menu. sc_name: see host-story-coop.ps1.
# v0.5: the same edict limit and heap as the host launch (host-story-coop.ps1).
$launch=@('-applaunch','223710','-dll','cl_dlls/storycoop_rc4.dll','-num_edicts','4096','-heapsize','1536000','-hw','-condebug')
$persona=Get-SteamPersonaName; if ($persona) { $launch+=@('+name',('"'+$persona+'"'),'+setinfo','sc_name',('"'+$persona+'"')) }
# v0.4: cof.exe directly when Steam runs (no CoFLaunchApp click), else Steam -applaunch.
$null=Start-CofGame $GameDirectory (@($launch | Select-Object -Skip 2)+@('+connect',$HostAddress))
if ($persona) { Write-Output "Your name in the game: $persona" }
Write-Output "Starting Cry of Fear and joining $HostAddress ..."
