[CmdletBinding()]
param([switch]$NoDownload,[string]$ReleaseApi='https://api.github.com/repos/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/latest',[int]$TimeoutSeconds=15)
# v0.5 (Katz: installing and updating are separate; Check for Updates).
# Asks the project's GitHub releases page for the newest version and compares it
# with this folder's version (release.json). A newer one can be downloaded to
# the Downloads folder; nothing is ever run, extracted or installed by this
# script, and saves, settings and the game are not touched. A failed check
# (no internet, timeout, GitHub down, unexpected answer) says so and never
# claims the mod is up to date.
$ErrorActionPreference='Stop'
function Get-VersionParts([string]$Text) {
    $m=[regex]::Match([string]$Text,'(?i)\bv?(\d+)\.(\d+)(?:\.(\d+))?')
    if (-not $m.Success) { return $null }
    return @([int]$m.Groups[1].Value,[int]$m.Groups[2].Value,$(if ($m.Groups[3].Success) { [int]$m.Groups[3].Value } else { 0 }))
}
function Compare-Version($a,$b) { for ($i=0;$i -lt 3;$i++) { if ($a[$i] -ne $b[$i]) { return [Math]::Sign($a[$i]-$b[$i]) } }; return 0 }
$catalogPath=Join-Path $PSScriptRoot 'release.json'
if (-not [IO.File]::Exists($catalogPath)) { throw 'This folder is incomplete (files\release.json is missing). Extract the whole ZIP again.' }
$current=[string](Get-Content -Raw -LiteralPath $catalogPath | ConvertFrom-Json).coopFixes.id
$currentParts=Get-VersionParts $current
Write-Output "This folder has co-op $current."
# Which version the game on this PC runs (the adapter's SHA256), when it can be told.
try {
    . (Join-Path $PSScriptRoot 'find-cof.ps1'); $game=Find-CofGameDirectory
    $adapter=if ($game) { Join-Path $game 'cl_dlls\storycoop_rc4.dll' } else { $null }
    if ($adapter -and [IO.File]::Exists($adapter)) {
        $sha=(Get-FileHash -LiteralPath $adapter -Algorithm SHA256).Hash.ToLowerInvariant()
        $catalog=Get-Content -Raw -LiteralPath $catalogPath | ConvertFrom-Json
        if ($sha -ceq $catalog.runtimeAdapter.sha256) { Write-Output "Installed in the game: co-op $current." }
        else { Write-Output 'Installed in the game: a different co-op version. Click Install in "Cry of Fear Co-op.exe" in this folder to install this one.' }
    } elseif ($game) { Write-Output 'Installed in the game: none yet. Click Install in "Cry of Fear Co-op.exe" in this folder.' }
} catch {}
Write-Output 'Checking GitHub for a newer version...'
try {
    [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
    $release=Invoke-RestMethod -Uri $ReleaseApi -TimeoutSec $TimeoutSeconds -UseBasicParsing -Headers @{'User-Agent'='cof-coop-updater';'Accept'='application/vnd.github+json'}
} catch {
    Write-Output ''
    Write-Output ('COULD NOT CHECK FOR UPDATES: '+$_.Exception.Message)
    Write-Output 'Check your internet connection and try again later. Nothing was changed.'
    Write-Output 'Releases page: https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases'
    exit 2
}
$latest=[string]$release.tag_name; $latestParts=Get-VersionParts $latest
if (-not $latestParts -or -not $currentParts) {
    Write-Output "COULD NOT CHECK FOR UPDATES: unexpected version names ('$latest' on GitHub, '$current' here). Nothing was changed."
    exit 2
}
$order=Compare-Version $latestParts $currentParts
if ($order -le 0) { Write-Output "You have the newest version ($current; newest on GitHub: $latest)."; exit 0 }
Write-Output ''
Write-Output "A NEWER VERSION IS OUT: $latest (you have $current)."
Write-Output ('Release page: '+[string]$release.html_url)
# Only a .zip attached to this project's own release, served by GitHub.
$asset=@($release.assets | Where-Object { [string]$_.name -match '\.zip$' -and [string]$_.browser_download_url -like 'https://github.com/UNIk0z/Cry-of-fear-Co-op-mod-SteamEdition/releases/download/*' }) | Select-Object -First 1
if (-not $asset) { Write-Output 'Download it from the release page.'; exit 0 }
if ($NoDownload) { Write-Output ('Download: '+[string]$asset.browser_download_url); exit 0 }
$answer=Read-Host ('Download '+[string]$asset.name+' ('+[Math]::Round([double]$asset.size/1MB,1)+' MB) to your Downloads folder now? (Y/N)')
if ($answer -notmatch '^(y|yes)$') { Write-Output 'Not downloaded. Nothing was changed.'; exit 0 }
$downloads=Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Downloads'
if (-not [IO.Directory]::Exists($downloads)) { $downloads=[Environment]::GetFolderPath('Desktop') }
$name=[IO.Path]::GetFileName([string]$asset.name) -replace '[^A-Za-z0-9 ._()-]','_'
$target=Join-Path $downloads $name; $n=1
while ([IO.File]::Exists($target)) { $target=Join-Path $downloads ([IO.Path]::GetFileNameWithoutExtension($name)+" ($n)"+[IO.Path]::GetExtension($name)); $n++ }
$partial=$target+'.part'
try {
    Invoke-WebRequest -Uri ([string]$asset.browser_download_url) -OutFile $partial -TimeoutSec 300 -UseBasicParsing -Headers @{'User-Agent'='cof-coop-updater'}
    if ((Get-Item -LiteralPath $partial).Length -ne [long]$asset.size) { throw 'the download is incomplete' }
    Move-Item -LiteralPath $partial -Destination $target
} catch {
    if ([IO.File]::Exists($partial)) { Remove-Item -LiteralPath $partial -Force }
    Write-Output ('DOWNLOAD FAILED: '+$_.Exception.Message+'. Get it from the release page instead.'); exit 3
}
Write-Output "Downloaded: $target"
Write-Output 'Next: close Cry of Fear, extract that ZIP to a new folder, open "Cry of Fear Co-op.exe" there and click Install. Your saves and settings stay as they are.'
