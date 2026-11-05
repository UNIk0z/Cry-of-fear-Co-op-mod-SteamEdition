[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$NativeDll,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$pin='0036b91c01e92ed205513f52563053a55a66a32d257efb5f476b8f1f3dde0e63'
if ((Get-FileHash -LiteralPath $NativeDll -Algorithm SHA256).Hash.ToLowerInvariant() -cne $pin) { throw 'Unsupported native source DLL; not building.' }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Use a fresh private build directory.' }
$sourcePins=@{}
foreach ($sourceName in @('adapter.cpp','abi.hpp','journal.cpp','journal.hpp','puzzle.cpp','puzzle.hpp','checkpoint.hpp','checkpoint.cpp','checkpoint-runtime.hpp','camera-runtime.hpp','note-layout.hpp','keydoor-runtime.hpp','ladder-runtime.hpp','pp-runtime.hpp','world-runtime.hpp','session-runtime.hpp','story-runtime.hpp','coop-runtime.hpp','state-runtime.hpp','client-menu.hpp','qol-runtime.hpp')) {
 $sourcePins[$sourceName]=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $sourceName) -Algorithm SHA256).Hash.ToLowerInvariant()
}
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $out | Out-Null
$compilerRoot='C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.34.31933'
$kit='C:\Program Files (x86)\Windows Kits\10'
$kitVersion='10.0.26100.0'
$cl=Join-Path $compilerRoot 'bin\Hostx64\x86\cl.exe'
$dump=Join-Path $compilerRoot 'bin\Hostx64\x86\dumpbin.exe'
$ml=Join-Path $compilerRoot 'bin\Hostx64\x86\ml.exe'
foreach ($required in @($cl,$dump,$ml)) { if (-not (Test-Path -LiteralPath $required)) { throw "Missing build tool $required" } }
$exports=@(& $dump /nologo /exports $NativeDll | ForEach-Object {
 if ($_ -match '^\s*(\d+)\s+[a-fA-F0-9]+\s+[a-fA-F0-9]{8}\s+(\S+)\s*$') { @{ordinal=[int]$Matches[1];name=$Matches[2]} }
})
if ($LASTEXITCODE -ne 0 -or $exports.Count -ne 846) { throw 'Native export interface changed.' }
$def=@('LIBRARY storycoop_rc4','EXPORTS')
$asm=@('.386','.model flat','EXTERN _rc4_exports:DWORD','.code')
foreach ($export in $exports) {
 $name=$export.name
 if ($name -ceq 'GiveFnptrsToDll') { $def+='GiveFnptrsToDll=_GiveFnptrsToDll@8 @1' }
 elseif ($name -cin @('GetEntityAPI','GetEntityAPI2')) { $def+=($name+' @'+$export.ordinal) }
 else {
  # Publish the original decorated COFF symbol directly. DEF aliases to C
  # stubs cause LINK to strip the original C++ name at its first @ character.
  $stub=$name; if (-not $name.StartsWith('?')) { $stub='_'+$name }
  if ($stub -cnotmatch '^[A-Za-z_?$@][A-Za-z_0-9?$@]*$') { throw 'Unsupported MASM export identifier.' }
  $def+=($name+' @'+$export.ordinal)
  $asm+=('PUBLIC '+$stub)
  $asm+=($stub+' PROC')
  $asm+=('jmp DWORD PTR [_rc4_exports+'+(($export.ordinal-1)*4)+']')
  $asm+=($stub+' ENDP')
 }
}
$asm+='END'
[IO.File]::WriteAllLines((Join-Path $out 'adapter.def'),$def,[Text.Encoding]::ASCII)
[IO.File]::WriteAllLines((Join-Path $out 'exports.asm'),$asm,[Text.Encoding]::ASCII)
& $ml /nologo /c /coff ('/Fo'+(Join-Path $out 'exports.obj')) (Join-Path $out 'exports.asm')
if ($LASTEXITCODE -ne 0) { throw 'SDK export forwarding assembly failed.' }
$compileArgs=@('/nologo','/std:c++17','/W4','/WX','/EHsc','/MT','/LD',('/I'+(Join-Path $compilerRoot 'include')))
foreach ($part in @('ucrt','um','shared')) { $compileArgs+=('/I'+(Join-Path $kit "Include\$kitVersion\$part")) }
$compileArgs+=@((Join-Path $PSScriptRoot 'adapter.cpp'),(Join-Path $PSScriptRoot 'journal.cpp'),(Join-Path $PSScriptRoot 'puzzle.cpp'),(Join-Path $PSScriptRoot 'checkpoint.cpp'),('/Fo'+$out+'\'),('/Fe'+(Join-Path $out 'storycoop_rc4.dll')),'/link','/WX',(Join-Path $out 'exports.obj'),('/DEF:'+(Join-Path $out 'adapter.def')),('/LIBPATH:'+(Join-Path $compilerRoot 'lib\x86')),('/LIBPATH:'+(Join-Path $kit "Lib\$kitVersion\ucrt\x86")),('/LIBPATH:'+(Join-Path $kit "Lib\$kitVersion\um\x86")),'bcrypt.lib')
& $cl @compileArgs
if ($LASTEXITCODE -ne 0) { throw 'Native adapter compilation failed.' }
$artifact=Join-Path $out 'storycoop_rc4.dll'
$outputExports=@(& $dump /nologo /exports $artifact)
$outputExports | Set-Content -LiteralPath (Join-Path $out 'exports.txt') -Encoding ascii
$actual=@($outputExports | ForEach-Object {
 if ($_ -match '^\s*(\d+)\s+[a-fA-F0-9]+\s+([a-fA-F0-9]{8})\s+(\S+)\s*$') { @{ordinal=[int]$Matches[1];rva=[Convert]::ToInt32($Matches[2],16);name=$Matches[3]} }
})
if ($LASTEXITCODE -ne 0 -or $actual.Count -ne $exports.Count) { throw 'Forwarding export count mismatch.' }
foreach ($expected in $exports) {
 if (@($actual | Where-Object { $_.name -ceq $expected.name -and $_.ordinal -eq $expected.ordinal }).Count -ne 1) { throw ('Forwarding export name/ordinal mismatch: '+$expected.name) }
}
$image=[IO.File]::ReadAllBytes($artifact); $nt=[BitConverter]::ToInt32($image,0x3c)
$sectionTable=$nt+24+[BitConverter]::ToUInt16($image,($nt+20)); $sections=[BitConverter]::ToUInt16($image,($nt+6))
$slotBase=$null; $jumpCount=0
foreach ($entry in $actual | Where-Object { $_.name -cnotin @('GiveFnptrsToDll','GetEntityAPI','GetEntityAPI2') }) {
 $offset=-1
 for ($i=0;$i -lt $sections;$i++) {
  $at=$sectionTable+$i*40; $va=[BitConverter]::ToUInt32($image,($at+12)); $length=[BitConverter]::ToUInt32($image,($at+16))
  if ($entry.rva -ge $va -and $entry.rva+6 -le $va+$length) { $offset=[BitConverter]::ToUInt32($image,($at+20))+$entry.rva-$va; break }
 }
 if ($offset -lt 0 -or $image[$offset] -ne 0xff -or $image[$offset+1] -ne 0x25) { throw ('Forwarding stub is not a direct indirect JMP: '+$entry.name) }
 $base=[long][BitConverter]::ToUInt32($image,($offset+2))-($entry.ordinal-1)*4
 if ($null -eq $slotBase) { $slotBase=$base } elseif ($slotBase -ne $base) { throw ('Forwarding jump slot differs: '+$entry.name) }
 $jumpCount++
}
if ($jumpCount -ne 843) { throw 'Forwarding jump interface changed.' }
foreach ($sourceName in $sourcePins.Keys) {
 if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $sourceName) -Algorithm SHA256).Hash.ToLowerInvariant() -cne $sourcePins[$sourceName]) { throw 'Native source changed during build; do not release this artifact.' }
}
$report=@{schemaVersion=1;artifact=$artifact;sha256=(Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash.ToLowerInvariant();bytes=$image.Length;nativeBackendSha256=$pin;architecture='x86';interfaceVersion=140;exports=846;exactNamesAndOrdinals=$true;jumpStubs=$jumpCount;registerAndStackPreserving=$true;nativeBackendLoaded=$false;sources=$sourcePins;status='COMPILED_NOT_GAMEPLAY_VERIFIED'}
$report | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'build-report.json') -Encoding utf8
$report | ConvertTo-Json -Compress
