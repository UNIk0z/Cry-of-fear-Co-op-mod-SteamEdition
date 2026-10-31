[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Version,[string]$GameDirectory,[string]$OutFile)
# Builds "Cry of Fear Co-op.exe" (CoopLauncher.cs) with the C# compiler that
# ships with the .NET Framework 4.x on every Windows 10/11 PC. Plain build:
# no packer, no obfuscation, version info and manifest embedded.
# The icon is made here from the installed game's own cryoffear\coficon.ico
# (black and white), so no game file is kept in the repository.
# Usage: build-launcher.ps1 -Version 0.7.1   (output: launcher\out\)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$here=$PSScriptRoot
$numeric=($Version -replace '^[vV]','')
if ($numeric -notmatch '^\d+\.\d+(\.\d+)?$') { throw 'Version must look like 0.7.1' }
$parts=@($numeric.Split('.')); while ($parts.Count -lt 4) { $parts+='0' }; $fileVersion=$parts -join '.'
$obj=Join-Path $here 'obj'; $out=Join-Path $here 'out'
[void][IO.Directory]::CreateDirectory($obj); [void][IO.Directory]::CreateDirectory($out)
if (-not $OutFile) { $OutFile=Join-Path $out 'Cry of Fear Co-op.exe' }

# Black-and-white icon from the game's icon (16, 24, 32 and 48 px, PNG entries).
if (-not $GameDirectory) { . (Join-Path $here '..\scripts\find-cof.ps1'); $GameDirectory=Find-CofGameDirectory }
$source=Join-Path ([string]$GameDirectory) 'coficon.ico'
if (-not [IO.File]::Exists($source)) { throw ('Game icon not found: '+$source) }
$big=(New-Object Drawing.Icon($source,48,48)).ToBitmap()
$c=1.0/0.92; $o=-0.04*$c
$rows=@(@((0.299*$c),(0.299*$c),(0.299*$c),0,0),@((0.587*$c),(0.587*$c),(0.587*$c),0,0),@((0.114*$c),(0.114*$c),(0.114*$c),0,0),@(0,0,0,1,0),@($o,$o,$o,0,1))
$matrix=New-Object Drawing.Imaging.ColorMatrix
for ($r=0;$r -lt 5;$r++) { for ($k=0;$k -lt 5;$k++) { $matrix[$r,$k]=[single]$rows[$r][$k] } }
$attributes=New-Object Drawing.Imaging.ImageAttributes; $attributes.SetColorMatrix($matrix)
$images=@()
foreach ($size in 16,24,32,48) {
    $bmp=New-Object Drawing.Bitmap($size,$size,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g=[Drawing.Graphics]::FromImage($bmp); $g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic; $g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.DrawImage($big,(New-Object Drawing.Rectangle(0,0,$size,$size)),0,0,$big.Width,$big.Height,[Drawing.GraphicsUnit]::Pixel,$attributes); $g.Dispose()
    $ms=New-Object IO.MemoryStream; $bmp.Save($ms,[Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
    $images+=,@($size,$ms.ToArray())
}
$big.Dispose()
$ico=New-Object IO.MemoryStream; $w=New-Object IO.BinaryWriter($ico)
$w.Write([uint16]0); $w.Write([uint16]1); $w.Write([uint16]$images.Count)
$offset=6+16*$images.Count
foreach ($i in $images) { $w.Write([byte]$i[0]); $w.Write([byte]$i[0]); $w.Write([byte]0); $w.Write([byte]0); $w.Write([uint16]1); $w.Write([uint16]32); $w.Write([uint32]$i[1].Length); $w.Write([uint32]$offset); $offset+=$i[1].Length }
foreach ($i in $images) { $w.Write([byte[]]$i[1]) }
$w.Flush(); $iconPath=Join-Path $obj 'launcher.ico'; [IO.File]::WriteAllBytes($iconPath,$ico.ToArray()); $w.Dispose()

# Version information shown in the file's Properties > Details.
$info=@(
 'using System.Reflection;',
 '[assembly: AssemblyTitle("Cry of Fear Co-op Steam Edition launcher")]',
 '[assembly: AssemblyDescription("Installs, updates and starts the Cry of Fear co-op mod")]',
 '[assembly: AssemblyCompany("Anas Sammak (koolkat)")]',
 '[assembly: AssemblyProduct("Cry of Fear Co-op Steam Edition")]',
 '[assembly: AssemblyCopyright("Co-op mod by Anas Sammak (koolkat). Cry of Fear by Team Psykskallar.")]',
 ('[assembly: AssemblyVersion("'+$fileVersion+'")]'),
 ('[assembly: AssemblyFileVersion("'+$fileVersion+'")]'),
 ('[assembly: AssemblyInformationalVersion("'+$numeric+'")]'))
$infoPath=Join-Path $obj 'VersionInfo.cs'; [IO.File]::WriteAllLines($infoPath,$info)

$csc=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not [IO.File]::Exists($csc)) { $csc=Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
if (-not [IO.File]::Exists($csc)) { throw 'The .NET Framework 4 C# compiler was not found.' }
$cscArgs=@('/nologo','/target:winexe','/platform:anycpu','/optimize+','/utf8output',
 ('/out:'+$OutFile),('/win32icon:'+$iconPath),('/win32manifest:'+(Join-Path $here 'app.manifest')),
 '/r:System.dll','/r:System.Drawing.dll','/r:System.Windows.Forms.dll','/r:System.Net.Http.dll','/r:System.Web.Extensions.dll','/r:System.IO.Compression.dll','/r:System.IO.Compression.FileSystem.dll',
 (Join-Path $here 'CoopLauncher.cs'),$infoPath)
& $csc $cscArgs
if ($LASTEXITCODE -ne 0) { throw ('csc failed with exit code '+$LASTEXITCODE) }
$hash=(Get-FileHash -LiteralPath $OutFile -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Output ('Built '+$OutFile+' ('+(Get-Item -LiteralPath $OutFile).Length+' bytes, sha256 '+$hash+')')
