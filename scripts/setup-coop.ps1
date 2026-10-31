[CmdletBinding()]
param([string]$GameDirectory,[switch]$CheckOnly,[switch]$Uninstall)
# One-step install, update and uninstall, for the host's PC and the friend's PC
# alike ("Install.cmd" / "Uninstall.cmd"). The version is release.json's.
# 1. Finds Cry of Fear through Steam (or -GameDirectory ...\cryoffear) and
#    names the co-op version installed now (by the adapter's SHA256).
# 2. Sets aside every file an earlier co-op build left behind: older RC4 maps,
#    adapters and menu files, the RC1/RC2/RC3 maps and the early test maps.
#    Files are only MOVED, never deleted, to "Cry of Fear - removed co-op
#    files\<time>" next to the game folder; a file is touched only when its
#    SHA256 is one this project produced (or, for the early test maps and
#    the old RC2/RC3 intro maps cof_campaign_story2/3, its exact name). An unknown file in a co-op path stops everything first.
# 3. Installs this package with its own guarded installer and verifies it.
# -Uninstall (coop-fixes-012): sets aside this build's files as well, puts the
# game's own menu title back and installs nothing. Unknown files are left alone.
# Saves (storycoop_saves), logs and every original game file stay in place.
$ErrorActionPreference='Stop'
function Get-Sha([string]$Path) {
    $digest=[Security.Cryptography.SHA256]::Create(); $stream=[IO.File]::OpenRead($Path)
    try { return ([BitConverter]::ToString($digest.ComputeHash($stream))).Replace('-','').ToLowerInvariant() } finally { $stream.Dispose(); $digest.Dispose() }
}
. (Join-Path $PSScriptRoot 'find-cof.ps1')
Write-CofStep 2 'Finding Cry of Fear'
if (-not $GameDirectory) { $GameDirectory=Find-CofGameDirectory }
if (-not $GameDirectory) { throw 'Cry of Fear was not found in any Steam library. Run again with -GameDirectory "<...>\steamapps\common\Cry of Fear\cryoffear".' }
$game=[IO.Path]::GetFullPath($GameDirectory).TrimEnd('\')
if (-not [IO.File]::Exists((Join-Path $game 'cl_dlls\hl.dll')) -or -not [IO.Directory]::Exists((Join-Path $game 'maps'))) { throw "Not a Cry of Fear game folder: $game" }
if (Get-Process -Name 'cof','cryoffear','hl' -ErrorAction SilentlyContinue) { throw 'Close Cry of Fear first (every copy, including one running in Sandboxie). Nothing changed.' }
Write-CofStep 8 'Checking the co-op files'
Write-Output "Cry of Fear: $game"
Write-CofAddonWarning $game
# v0.3: a protected game folder needs administrator rights to install into.
if (-not $CheckOnly -and (-not (Test-CofWritable $game) -or -not (Test-CofWritable (Split-Path $game -Parent)))) {
    if (Test-CofAdmin) { throw "Windows does not allow writing to $game even as administrator. Nothing changed." }
    $again=@(); if ($Uninstall) { $again+='-Uninstall' }; if ($PSBoundParameters.ContainsKey('GameDirectory')) { $again+=@('-GameDirectory',("'"+$GameDirectory.Replace("'","''")+"'")) }
    Invoke-CofElevated $PSCommandPath $again 'The Cry of Fear folder is protected by Windows (for example under Program Files), so installing needs administrator rights.'
}
$catalog=Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'release.json') | ConvertFrom-Json
$known=Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'known-builds.json') | ConvertFrom-Json
$version=[string]$catalog.coopFixes.id
# Expected files of this package.
$expected=@{}
foreach ($m in $catalog.maps) { $expected['maps\'+$m.map+'.bsp']=$m.outputSha256 }
$expected['cl_dlls\storycoop_rc4.dll']=$catalog.runtimeAdapter.sha256
foreach ($x in @($catalog.extras | Where-Object { $null -ne $_ })) { $expected[$x.target.Replace('/','\')]=$x.sha256 }
# coop-fixes-010: a game file this package replaces (the CO-OP menu title) is
# expected to hold the game's own version first; the installer keeps it.
$originals=@{}
foreach ($x in @($catalog.extras | Where-Object { $null -ne $_ -and $null -ne $_.replaces })) { $originals[$x.target.Replace('/','\')]=$x.replaces.sha256 }
# Every hash an earlier build of this project installed, by path.
$older=@{}
function Add-Known([string]$rel,[string]$sha) { if (-not $sha) { return }; if (-not $older.ContainsKey($rel)) { $older[$rel]=New-Object 'System.Collections.Generic.HashSet[string]' }; [void]$older[$rel].Add($sha) }
foreach ($b in $known.builds) {
    foreach ($m in @($b.maps)) { Add-Known ('maps\'+$m.map+'.bsp') $m.sha256 }
    if ($b.adapter) { Add-Known 'cl_dlls\storycoop_rc4.dll' $b.adapter }
    # A replaced game file is never set aside (remove.cmd restores the original).
    foreach ($x in @($b.extras | Where-Object { $null -ne $_ -and -not $_.replaces })) { Add-Known $x.target.Replace('/','\') $x.sha256 }
}
# coop-fixes-012: which co-op version is installed now (the adapter tells them apart).
$adapterPath=Join-Path $game 'cl_dlls\storycoop_rc4.dll'; $installed=$null
if ([IO.File]::Exists($adapterPath)) {
    $sha=Get-Sha $adapterPath
    if ($sha -ceq $catalog.runtimeAdapter.sha256) { $installed=$version }
    else { $match=@($known.builds | Where-Object { $_.adapter -ceq $sha }); $installed=if ($match.Count) { [string]$match[0].id } else { 'an unknown version' } }
}
if ($Uninstall) { Write-Output $(if ($installed) { "Installed now: co-op $installed. Uninstalling." } else { 'No co-op adapter installed; setting aside any co-op files left over.' }) }
elseif (-not $installed) { Write-Output "No co-op version installed yet. Installing $version." }
elseif ($installed -eq $version) { Write-Output "Co-op $version is already installed. Checking every file." }
else { Write-Output "Found co-op $installed. Updating it to $version." }
$otherVersion=New-Object System.Collections.Generic.List[string]; $otherSha=@{}; $moves=New-Object System.Collections.Generic.List[string]; $unknown=New-Object System.Collections.Generic.List[string]; $kept=0
# v0.5.1: sc_setup.scr is rewritten by the game's Create Server window (last choices); any content is ours.
$mutable=@{}; foreach ($x in @($catalog.extras | Where-Object { $null -ne $_ -and $_.mutable })) { $mutable[$x.target.Replace('/','\')]=$true }
foreach ($rel in @($expected.Keys | Sort-Object)) {
    $path=Join-Path $game $rel; if (-not [IO.File]::Exists($path)) { continue }
    if ($mutable.ContainsKey($rel)) { if ($Uninstall) { $moves.Add($rel) } else { $kept++ }; continue }
    if ($Uninstall -and $originals.ContainsKey($rel)) { continue }   # the title is put back below
    $sha=Get-Sha $path
    if ($sha -ceq $expected[$rel] -or ($originals.ContainsKey($rel) -and $sha -ceq $originals[$rel])) { if ($Uninstall) { $moves.Add($rel) } else { $kept++ } }
    elseif ($older.ContainsKey($rel) -and $older[$rel].Contains($sha)) { $moves.Add($rel) }
    # coop-fixes-013: a game file this package would replace (title, client.dll)
    # in another game version is left alone; the installer skips that extra.
    elseif ($originals.ContainsKey($rel)) { $otherVersion.Add($rel); $otherSha[$rel]=$sha }
    else { $unknown.Add($rel) }
}
if ($unknown.Count -and -not $Uninstall) { $unknown | ForEach-Object { Write-Output "Unknown file (not from any co-op build): $(Join-Path $game $_)" }; throw 'Unknown files sit where the co-op files go. Nothing changed. Move them away yourself, or ask for help.' }
# Leftovers this package does not use.
$skipped=New-Object System.Collections.Generic.List[string]
foreach ($rel in $unknown) { $skipped.Add($rel) }
foreach ($rel in $otherVersion) { $skipped.Add($rel) }
foreach ($rel in @($older.Keys | Sort-Object)) {
    if ($expected.ContainsKey($rel)) { continue }
    $path=Join-Path $game $rel; if (-not [IO.File]::Exists($path)) { continue }
    if ($older[$rel].Contains((Get-Sha $path))) { $moves.Add($rel) } else { $skipped.Add($rel) }
}
foreach ($name in @($known.experimentMaps)) { $rel='maps\'+$name+'.bsp'; if ([IO.File]::Exists((Join-Path $game $rel)) -and -not $moves.Contains($rel)) { $moves.Add($rel); [void]$skipped.Remove($rel) } }
# Node graphs the game built for maps that are being set aside.
foreach ($rel in @($moves.ToArray())) {
    if ($rel -notmatch '^maps\\([^\\]+)\.bsp$' -or ($expected.ContainsKey($rel) -and -not $Uninstall)) { continue }
    foreach ($ext in '.nod','.nrp') { $g='maps\graphs\'+$Matches[1]+$ext; if ([IO.File]::Exists((Join-Path $game $g))) { $moves.Add($g) } }
}
# Uninstall: the CO-OP title goes aside and the game's own title (kept beside
# it as .storycoop-original) goes back. A title that is already the game's
# own only loses the spare copy.
$titles=New-Object System.Collections.Generic.List[string]
if ($Uninstall) {
    foreach ($rel in @($originals.Keys)) {
        $path=Join-Path $game $rel; $keptCopy=$rel+'.storycoop-original'
        if (-not [IO.File]::Exists((Join-Path $game $keptCopy)) -or (Get-Sha (Join-Path $game $keptCopy)) -cne $originals[$rel]) { continue }
        $current=if ([IO.File]::Exists($path)) { Get-Sha $path } else { $null }
        if ($current -ceq $originals[$rel]) { $moves.Add($keptCopy) }
        elseif ($null -eq $current -or $current -ceq $expected[$rel]) { $titles.Add($rel); if ($current) { $moves.Add($rel) } }
        else { $skipped.Add($rel) }
    }
}
# v0.1: no game file is replaced any more. coop-fixes-010..014 replaced
# gfx\vgui\menu_logo.tga (CO-OP title) and cl_dlls\client.dll (main menu) and
# kept each original beside it as .storycoop-original. On install and on
# uninstall the co-op copy goes aside and the original goes back; a file that
# is already the original only loses the spare copy. A file in any other
# state is left alone and named.
foreach ($r in @($known.replacedFiles | Where-Object { $null -ne $_ })) {
    $rel=$r.target.Replace('/','\'); if ($originals.ContainsKey($rel)) { continue }
    $path=Join-Path $game $rel; $keptCopy=$rel+'.storycoop-original'; $keptPath=Join-Path $game $keptCopy
    $current=if ([IO.File]::Exists($path)) { Get-Sha $path } else { $null }
    $keptSha=if ([IO.File]::Exists($keptPath)) { Get-Sha $keptPath } else { $null }
    if ($null -eq $keptSha) { if ($current -and @($r.coopSha256) -contains $current) { $skipped.Add($rel+' (co-op copy without the kept original)') }; continue }
    if ($keptSha -cne $r.originalSha256) { $skipped.Add($keptCopy); continue }
    if ($current -ceq $r.originalSha256) { $moves.Add($keptCopy) }
    elseif ($null -eq $current -or @($r.coopSha256) -contains $current) { $titles.Add($rel); if ($current) { $moves.Add($rel) } }
    else { $skipped.Add($rel) }
}
# RC1 "Host Coop Game" main-menu entry: put the game's own menu back.
$menuRestore=$null
$menuPath=Join-Path $game 'resource\GameMenu.res'; $menuBackup=$menuPath+'.storycoop-rc1-backup'
if ([IO.File]::Exists($menuBackup) -and [IO.File]::Exists($menuPath)) {
    $backupSha=Get-Sha $menuBackup; $menuSha=Get-Sha $menuPath
    $ours=@($known.menus | Where-Object { $_.sourceSha256 -ceq $backupSha })
    if ($ours.Count -and ($menuSha -ceq $ours[0].outputSha256 -or $menuSha -ceq $backupSha)) {
        if ($menuSha -cne $backupSha) { $moves.Add('resource\GameMenu.res'); $menuRestore=$menuBackup }
        $moves.Add('resource\GameMenu.res.storycoop-rc1-backup')
    } else { $skipped.Add('resource\GameMenu.res') }
}
if (-not $Uninstall) { Write-Output "Already current: $kept file(s). Old co-op files to set aside: $($moves.Count)." }
foreach ($s in $skipped) { Write-Output "Left in place (not from this project, or changed since): $s" }
if ($CheckOnly) { $moves | ForEach-Object { Write-Output "  would set aside $_" }; foreach ($t in $titles) { Write-Output "  would put the game's own $t back" }; Write-Output 'Check only: nothing changed.'; return }
if ($moves.Count) {
    $stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
    $aside=Join-Path (Split-Path (Split-Path $game)) ('Cry of Fear - removed co-op files\'+$stamp)
    [void][IO.Directory]::CreateDirectory($aside)
    $menuBytes=$null; if ($menuRestore) { $menuBytes=[IO.File]::ReadAllBytes($menuRestore) }
    $manifest=New-Object System.Collections.Generic.List[string]
    foreach ($rel in $moves) {
        Write-CofStep $(if ($Uninstall) { 20+[int](70*$manifest.Count/$moves.Count) } else { 30+[int](15*$manifest.Count/$moves.Count) }) ('Setting aside co-op files ('+($manifest.Count+1)+' of '+$moves.Count+')')
        $target=Join-Path $aside $rel; [void][IO.Directory]::CreateDirectory((Split-Path $target))
        [IO.File]::Move((Join-Path $game $rel),$target); $manifest.Add($rel)
    }
    if ($menuBytes) { [IO.File]::WriteAllBytes($menuPath,$menuBytes); Write-Output 'Restored the game''s own main menu (removed the old "Host Coop Game" entry).' }
    $header=@(('Moved here from '+$game+' by setup-coop.ps1 ('+$version+$(if ($Uninstall) { ', uninstall' } else { '' })+').'),'Nothing in this folder is needed to play. Delete the folder whenever you like.','')
    [IO.File]::WriteAllLines((Join-Path $aside 'MOVED FILES.txt'),[string[]]($header+$manifest.ToArray()))
    Write-Output "Set aside $($moves.Count) co-op file(s) in: $aside"
    Write-Output 'Delete that folder yourself once you are happy (it frees the disk space).'
}
foreach ($rel in $titles) { [IO.File]::Move((Join-Path $game ($rel+'.storycoop-original')),(Join-Path $game $rel)); Write-Output "Put the game's own $rel back." }
if ($Uninstall) { Write-CofStep 100 'Done'; Write-Output 'DONE: the co-op mod is uninstalled. Your co-op saves (storycoop_saves) and logs were kept; "Install.cmd" puts it back.'; return }
$installer=Join-Path $PSScriptRoot 'install-story-package.ps1'
Write-CofStep 45 'Installing the co-op files (a few minutes the first time)'
Write-Output 'Installing: the co-op maps are built from your own game files (a few minutes the first time). Please wait...'
$stepDone=0; $stepTotal=[Math]::Max(1,$expected.Count)
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -MapsDirectory (Join-Path $game 'maps') | ForEach-Object { if ($_ -match '^(Installed|Already installed and verified)[: ]') { $stepDone=[Math]::Min($stepDone+1,$stepTotal); Write-CofStep (45+[int](45*$stepDone/$stepTotal)) ('Installing co-op files ('+$stepDone+' of '+$stepTotal+')') }; $_ } | Where-Object { $_ -notmatch '^(Installed|Already installed and verified)[: ]|^EXPERIMENTAL: |^RC4 experimental adapter|^Preflighting |^Main menu: ' }
if ($LASTEXITCODE -ne 0) { throw 'The package installer stopped (see above). Old files were set aside; nothing else changed.' }
Write-CofStep 92 'Checking the installed files'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $installer -MapsDirectory (Join-Path $game 'maps') -VerifyOnly | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Verification failed after install.' }
$bad=@($expected.Keys | Where-Object { -not [IO.File]::Exists((Join-Path $game $_)) -or (Get-Sha (Join-Path $game $_)) -cne $expected[$_] })
$simon=$catalog.extras | Where-Object { $_.target -eq 'models/player/simon/simon.mdl' }
$bad=@($bad | Where-Object { -not ($simon -and $_ -eq 'models\player\simon\simon.mdl' -and $simon.optional) })
# An optional replaced file that stayed the game's own version is fine.
$bad=@($bad | Where-Object { -not ($originals.ContainsKey($_) -and [IO.File]::Exists((Join-Path $game $_)) -and (Get-Sha (Join-Path $game $_)) -ceq $originals[$_]) })
# coop-fixes-013: so is one that was another version before install and was left untouched.
$bad=@($bad | Where-Object { -not ($otherSha.ContainsKey($_) -and [IO.File]::Exists((Join-Path $game $_)) -and (Get-Sha (Join-Path $game $_)) -ceq $otherSha[$_]) })
# v0.1: the CO-OP title (built from this PC's own title) is optional too.
$built=@($catalog.extras | Where-Object { $null -ne $_ -and $null -ne $_.deltaFrom } | ForEach-Object { $_.target.Replace('/','\') })
$bad=@($bad | Where-Object { -not ($built -contains $_ -and -not [IO.File]::Exists((Join-Path $game $_))) })
if ($bad.Count) { $bad | ForEach-Object { Write-Output "Not as expected: $_" }; throw 'Verification failed after install.' }
Write-CofStep 100 'Done'
if ($originals.Count) { Write-Output "DONE: co-op $version installed and verified ($($expected.Count) files). Saves were not touched; of the game's own files only the menu title and the main menu (cl_dlls\client.dll) are replaced; Uninstall puts the originals back." }
else { Write-Output "DONE: co-op $version installed and verified ($($expected.Count) files). Saves and the game's own files were not touched: starting Cry of Fear normally shows the game's own menu; ""Run Co-op Mod"" shows the co-op one." }
