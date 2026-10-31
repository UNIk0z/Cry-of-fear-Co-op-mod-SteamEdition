# coop-fixes-008: locate ...\steamapps\common\Cry of Fear\cryoffear on any PC
# from Steam's own registry entry and library list. Dot-sourced; read only.
# Launcher (Katz, 2026-10-30): the .exe runs these scripts without a console
# window and sets COF_LAUNCHER=1; a step line then moves its progress bar.
# Written straight to stdout so no function result changes; the .cmd files
# never show it.
function Write-CofStep([int]$Percent,[string]$Text) {
    if ($env:COF_LAUNCHER -eq '1') { [Console]::Out.WriteLine('##cof-progress '+$Percent+' '+$Text); [Console]::Out.Flush() }
}
function Find-CofGameDirectory {
    $roots=New-Object System.Collections.Generic.List[string]
    foreach ($key in @(@('HKCU:\Software\Valve\Steam','SteamPath'),@('HKLM:\SOFTWARE\WOW6432Node\Valve\Steam','InstallPath'),@('HKLM:\SOFTWARE\Valve\Steam','InstallPath'))) {
        try { $value=(Get-ItemProperty -LiteralPath $key[0] -Name $key[1] -ErrorAction Stop).($key[1]); if ($value) { $roots.Add(([string]$value).Replace('/','\')) } } catch {}
    }
    $libraries=New-Object System.Collections.Generic.List[string]
    foreach ($root in $roots) {
        $libraries.Add($root)
        $vdf=Join-Path $root 'steamapps\libraryfolders.vdf'
        if ([IO.File]::Exists($vdf)) { foreach ($m in [regex]::Matches([IO.File]::ReadAllText($vdf),'"path"\s+"([^"]+)"')) { $libraries.Add($m.Groups[1].Value.Replace('\\','\')) } }
    }
    $libraries.Add('D:\SteamLibrary')
    # v0.5 ("not installed" reports): a library can hold an old copy of the game
    # Steam no longer uses. The library with Steam's own install record for Cry
    # of Fear (steamapps\appmanifest_223710.acf) wins; otherwise the first copy.
    $first=$null
    foreach ($library in $libraries) {
        $candidate=Join-Path $library 'steamapps\common\Cry of Fear\cryoffear'
        if (-not [IO.File]::Exists((Join-Path $candidate 'cl_dlls\hl.dll'))) { continue }
        if ([IO.File]::Exists((Join-Path $library 'steamapps\appmanifest_223710.acf'))) { return [IO.Path]::GetFullPath($candidate) }
        if (-not $first) { $first=[IO.Path]::GetFullPath($candidate) }
    }
    return $first
}
# coop-fixes-013 (user: names showed as "Unknown"): Cry of Fear keeps the
# player name from its own config.cfg (name "Unknown"). The launchers pass the
# Steam display name of the account that signed in last (Steam's own
# config\loginusers.vdf, MostRecent 1) as +name; the game then saves it in
# config.cfg itself. Only the display name is read; nothing is written.
function Get-SteamPersonaName {
    try {
        $root=[string](Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam' -Name SteamPath -ErrorAction Stop).SteamPath
        $vdf=Join-Path $root.Replace('/','\') 'config\loginusers.vdf'
        if (-not [IO.File]::Exists($vdf)) { return $null }
        $text=[IO.File]::ReadAllText($vdf,[Text.Encoding]::UTF8)
        $chosen=$null
        foreach ($block in [regex]::Matches($text,'"\d{17}"\s*\{([^{}]*)\}')) {
            $body=$block.Groups[1].Value
            $persona=[regex]::Match($body,'"PersonaName"\s+"([^"]*)"')
            if (-not $persona.Success) { continue }
            if ([regex]::IsMatch($body,'"MostRecent"\s+"1"') -or -not $chosen) { $chosen=$persona.Groups[1].Value }
        }
        if (-not $chosen) { return $null }
        # Letters, digits, spaces and common punctuation only; never quotes,
        # semicolons or backslashes (they would end the console command).
        $clean=(-join ($chosen.ToCharArray() | Where-Object { [int]$_ -ge 32 -and [int]$_ -lt 0x2000 -and '";\%'.IndexOf($_) -lt 0 })).Trim()
        if ($clean.Length -gt 31) { $clean=$clean.Substring(0,31).Trim() }
        if ($clean) { return $clean } else { return $null }
    } catch { return $null }
}
function Find-SteamExe {
    try { $path=Join-Path ([string](Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam' -Name SteamPath -ErrorAction Stop).SteamPath) 'steam.exe'; if ([IO.File]::Exists($path)) { return $path } } catch {}
    foreach ($key in 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam','HKLM:\SOFTWARE\Valve\Steam') { try { $path=Join-Path ([string](Get-ItemProperty -LiteralPath $key -Name InstallPath -ErrorAction Stop).InstallPath) 'steam.exe'; if ([IO.File]::Exists($path)) { return $path } } catch {} }
    return $null
}
# v0.3 (tester: the commands only worked "as administrator"). Two Windows
# causes: a game folder Windows protects (for example under Program Files)
# cannot be written by a normal user, and a Steam that runs as administrator
# ignores -applaunch requests from a normal window. The commands check both
# and, when needed, run themselves again as administrator (one Windows
# prompt) instead of failing half way.
function Test-CofAdmin {
    return ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}
function Test-CofWritable([string]$Directory) {
    try { $probe=Join-Path $Directory ('.coop-write-test-'+[guid]::NewGuid().ToString('N')); [IO.File]::WriteAllText($probe,'x'); [IO.File]::Delete($probe); return $true } catch { return $false }
}
function Test-SteamElevated {
    foreach ($s in @(Get-Process -Name 'steam' -ErrorAction SilentlyContinue)) { try { [void]$s.MainModule } catch { return $true } }
    return $false
}
function Invoke-CofElevated([string]$Script,[string[]]$Arguments,[string]$Why) {
    Write-Output $Why
    Write-Output 'Windows will now ask once for administrator permission; this command continues in a new window.'
    $command="& '"+$Script.Replace("'","''")+"' "+($Arguments -join ' ')+"; Write-Host ''; Read-Host 'Press Enter to close this window'"
    try { $child=Start-Process -FilePath 'powershell.exe' -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-Command',$command) -Verb RunAs -Wait -PassThru }
    catch { throw 'Administrator permission was refused. Run this command again and choose Yes, or right-click it and pick "Run as administrator".' }
    exit $child.ExitCode
}

# v0.4 (Katz 2026-10-18: the launcher must open the co-op game at once).
# Steam -applaunch 223710 starts the game's own CoFLaunchApp.exe, which waits
# for its LAUNCH button (seen on this PC with the v0.4 launcher). That window
# writes steam_appid.txt (223710) in the game root and starts
# "cof.exe -game cryoffear <arguments>" there. With Steam running, do exactly
# that; otherwise (or without cof.exe) fall back to Steam -applaunch.
function Start-CofGame([string]$GameDirectory,[string[]]$Arguments) {
    $root=Split-Path ([IO.Path]::GetFullPath($GameDirectory)) -Parent
    $exe=Join-Path $root 'cof.exe'
    if ([IO.File]::Exists($exe) -and (Get-Process -Name 'steam' -ErrorAction SilentlyContinue)) {
        $appId=Join-Path $root 'steam_appid.txt'
        if (-not [IO.File]::Exists($appId)) { [IO.File]::WriteAllText($appId,'223710') }
        Start-Process -FilePath $exe -WorkingDirectory $root -ArgumentList (@('-game','cryoffear')+$Arguments)
        return 'direct'
    }
    $steam=Find-SteamExe; if (-not $steam) { throw 'Steam was not found.' }
    Start-Process -FilePath $steam -ArgumentList (@('-applaunch','223710')+$Arguments) -WindowStyle Hidden
    return 'steam'
}
# v0.5 (playtest crash dumps): both testers ran the third-party add-on
# "RebuiltSimon" (cryoffear\addons\RebuiltSimonCoFManager.dll, fastrun.dll).
# Every co-op CRASH line in their logs was inside RebuiltSimonCoFManager.dll, and
# it changes movement speed. It is not part of Cry of Fear or of this mod; the
# setup and the launchers name it and tell the player to move it out for co-op.
# Nothing is moved or deleted here.
function Get-CofAddons([string]$GameDirectory) {
    $dir=Join-Path $GameDirectory 'addons'
    if (-not [IO.Directory]::Exists($dir)) { return @() }
    return @(Get-ChildItem -LiteralPath $dir -Filter '*.dll' -File -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
}
function Write-CofAddonWarning([string]$GameDirectory) {
    $addons=@(Get-CofAddons $GameDirectory)
    if (-not $addons.Count) { return }
    Write-Warning ('Cry of Fear add-ons found in '+(Join-Path $GameDirectory 'addons')+': '+($addons -join ', ')+'.')
    if (@($addons | Where-Object { $_ -match '^(RebuiltSimonCoFManager|fastrun)\.dll$' }).Count) { Write-Warning 'RebuiltSimon crashed the game in co-op playtests (every crash report pointed into RebuiltSimonCoFManager.dll) and changes how fast you move.' }
    Write-Warning 'For co-op, close the game and move the "addons" folder somewhere else (for example to your Desktop). Put it back afterwards for single player.'
}