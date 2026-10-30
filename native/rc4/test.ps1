[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Adapter,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Fresh test directory required.' }
$out=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $out | Out-Null
$vc='C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.34.31933'
$kit='C:\Program Files (x86)\Windows Kits\10'
$common=@('/nologo','/std:c++17','/W4','/WX','/EHsc','/MT',('/I'+$vc+'\include'))
foreach ($part in @('ucrt','um','shared')) { $common+=('/I'+$kit+'\Include\10.0.26100.0\'+$part) }
$link=@('/link',('/LIBPATH:'+$vc+'\lib\x86'),('/LIBPATH:'+$kit+'\Lib\10.0.26100.0\ucrt\x86'),('/LIBPATH:'+$kit+'\Lib\10.0.26100.0\um\x86'),'bcrypt.lib')
$specs=@(
 @{name='journal'; sources=@('test-journal.cpp','journal.cpp');extra=@()},
 @{name='puzzle';sources=@('puzzle.cpp');extra=@('/DRC4_PUZZLE_TEST')},
 @{name='adapter';sources=@('test-adapter.cpp','journal.cpp','puzzle.cpp','checkpoint.cpp');extra=@()},
 @{name='camera-notes';sources=@('test-camera-notes.cpp','journal.cpp','puzzle.cpp','checkpoint.cpp');extra=@()},
 @{name='probe';sources=@('test-probe.cpp');extra=@()}
 @{name='checkpoint';sources=@('test-checkpoint.cpp','checkpoint.cpp','journal.cpp');extra=@()}
 @{name='checkpoint-runtime';sources=@('test-checkpoint-runtime.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='keydoor';sources=@('test-keydoor.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='ladder';sources=@('test-ladder.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='postprocess';sources=@('test-pp.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='world';sources=@('test-world.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='session';sources=@('test-session.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='story';sources=@('test-story.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='mapstate';sources=@('test-mapstate.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
 @{name='gamebytes';sources=@('test-gamebytes.cpp','checkpoint.cpp','journal.cpp','puzzle.cpp');extra=@()}
)
foreach ($test in $specs) {
 $args=$common+@($test.extra)+@($test.sources | ForEach-Object { Join-Path $PSScriptRoot $_ })+@(('/Fo'+$out+'\'),('/Fe'+$out+'\'+$test.name+'-tests.exe'))+$link
 & ($vc+'\bin\Hostx64\x86\cl.exe') @args
 if ($LASTEXITCODE -ne 0) { throw "Fixture compile failed: $($test.name)" }
 if ($test.name -eq 'probe') {
   $isolated=Join-Path $out 'bootstrap';New-Item -ItemType Directory -Path $isolated | Out-Null
   $probeArtifact=Join-Path $isolated 'storycoop_rc4.dll';Copy-Item -LiteralPath $Adapter -Destination $probeArtifact
   & ($out+'\probe-tests.exe') $probeArtifact
 } elseif ($test.name -eq 'checkpoint') { & ($out+'\'+$test.name+'-tests.exe') (Join-Path $out 'save-fixture') }
 elseif ($test.name -eq 'checkpoint-runtime') { $runtimeFixture=Join-Path $out 'runtime-fixture';New-Item -ItemType Directory -Path $runtimeFixture | Out-Null; & ($out+'\'+$test.name+'-tests.exe') $runtimeFixture }
 elseif ($test.name -eq 'story') { $storyFixture=Join-Path $out 'story-fixture';New-Item -ItemType Directory -Path $storyFixture | Out-Null; & ($out+'\'+$test.name+'-tests.exe') $storyFixture }
 elseif ($test.name -eq 'session') { $sessionFixture=Join-Path $out 'session-fixture';New-Item -ItemType Directory -Path $sessionFixture | Out-Null; & ($out+'\'+$test.name+'-tests.exe') $sessionFixture }
 elseif ($test.name -eq 'mapstate') { $mapFixture=Join-Path $out 'mapstate-fixture';New-Item -ItemType Directory -Path $mapFixture | Out-Null; & ($out+'\'+$test.name+'-tests.exe') $mapFixture }
 elseif ($test.name -eq 'world') { $worldFixture=Join-Path $out 'world-fixture';New-Item -ItemType Directory -Path $worldFixture | Out-Null; & ($out+'\'+$test.name+'-tests.exe') $worldFixture }
 elseif ($test.name -eq 'gamebytes') { $gameFolder=if ($env:COF_GAME_DIRECTORY) { $env:COF_GAME_DIRECTORY } else { 'D:\SteamLibrary\steamapps\common\Cry of Fear\cryoffear' }; & ($out+'\'+$test.name+'-tests.exe') $gameFolder }
 else { & ($out+'\'+$test.name+'-tests.exe') }
 if ($LASTEXITCODE -ne 0) { throw "Fixture failed: $($test.name)" }
}
Write-Output 'PASS all 15 native fixture suites. No Cry of Fear DLL loaded or game launched.'
