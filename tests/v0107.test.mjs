// v0.10.7 (Claude, 2026-11-14): fixes from the code review of v0.10.6 (docs/V0107_RELEASE.md). The installer's
// guard runs for real in Windows PowerShell with a fake blocked write and a blocked optional read; find-cof.ps1's
// adapter check runs against a fake game folder, and join-coop.ps1 runs up to its check. Host and setup are
// static checks (running them would look for Steam or install). The adapter changes are covered by
// native/rc4/test-qol2.cpp (F1, F4) and test-mapstate.cpp (F5).
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {readMe} from '../scripts/player-package.mjs';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');
const ascii=s=>/^[\x09\x0a\x0d\x20-\x7e]*$/.test(s);
const ps=file=>spawnSync('powershell.exe',['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',file],{encoding:'utf8'});

function guard(body){
 const script=read('scripts/install-story-package.ps1');
 const block=script.slice(script.indexOf('# BEGIN read guard'),script.indexOf('# END read guard'));
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-guard7-'));fs.mkdirSync(path.join(dir,'native'));
 fs.writeFileSync(path.join(dir,'native','storycoop_rc4.dll'),Buffer.from([1,2,3]));
 const file=path.join(dir,'guard.ps1');fs.writeFileSync(file,"$ErrorActionPreference='Stop'\r\n"+block+'\r\n'+body+'\r\n');
 const r=ps(file);fs.rmSync(dir,{recursive:true,force:true});return r;
}
const virus="[Management.Automation.MethodInvocationException]::new('Exception calling \"Move\"',[IO.IOException]::new('virus',-2147024671))";
const blockedRead="$script:CoopRead={ param([string]$Path) throw [IO.IOException]::new('virus',-2147024671) }";

test('F2 installer: a write the antivirus stops names the game file being written, exit code 3',()=>{
 const r=guard("$script:CoopTarget='C:\\Games\\Cry of Fear\\cryoffear\\cl_dlls\\storycoop_rc4.dll'\r\nthrow "+virus);
 assert.equal(r.status,3,r.stderr);
 assert.match(r.stdout,/^COOP-BLOCKED C:\\Games\\Cry of Fear\\cryoffear\\cl_dlls\\storycoop_rc4\.dll\r?$/m);
 assert.ok(r.stdout.includes('Install again finishes it.')&&ascii(r.stdout)&&r.stderr.trim()==='');
 // v0.10.6: the raw error record, exit code 1.
 const plain=guard("$script:CoopTarget='C:\\x.bsp'\r\nthrow [IO.IOException]::new('sharing violation',-2147024864)");
 assert.equal(plain.status,1);assert.ok(!plain.stdout.includes('COOP-BLOCKED'));   // other write errors stay as they were
 const script=read('scripts/install-story-package.ps1');
 assert.ok(script.includes('foreach ($item in $prepared) {\n    $script:CoopTarget=$item.path')||script.includes('foreach ($item in $prepared) {\r\n    $script:CoopTarget=$item.path'));
 assert.ok(script.includes('$script:CoopTarget=$menuPath')&&script.includes('$script:CoopTarget=$entryPaths[$i]'));
});

test('F3 installer: an optional file whose read was blocked and skipped does not turn a later error into a block',()=>{
 const r=guard(blockedRead+"\r\ntry { $x=(Read-CoopBytes (Join-Path $PSScriptRoot 'native\\storycoop_rc4.dll')) } catch { $x=$null }\r\nSkip-CoopBlocked\r\nthrow 'Unknown/changed native menu retained.'");
 assert.equal(r.status,1);assert.ok(!r.stdout.includes('COOP-BLOCKED'));                 // v0.10.6: exit code 3 naming the skipped file
 assert.match(r.stdout,/^Optional file skipped: the antivirus blocked .*native\\storycoop_rc4\.dll\r?$/m);
 assert.ok(r.stderr.includes('Unknown/changed native menu retained.'));
 // The optional Simon model is the one optional item whose reads sit in a try/catch.
 assert.ok(read('scripts/install-story-package.ps1').includes("if ($extra.optional) { Skip-CoopBlocked; Write-Output 'Simon player model skipped: the model built"));
});

function fakeGame(withMaps,withAdapter){
 const game=fs.mkdtempSync(path.join(os.tmpdir(),'cof-game7-'));fs.mkdirSync(path.join(game,'maps'));fs.mkdirSync(path.join(game,'cl_dlls'));
 if(withMaps)fs.writeFileSync(path.join(game,'maps','cof_campaign_story4.bsp'),'map');
 if(withAdapter)fs.writeFileSync(path.join(game,'cl_dlls','storycoop_rc4.dll'),'dll');
 return game;
}

test('F2 find-cof.ps1: the co-op maps without the adapter give the steps; the steps name both folders',()=>{
 const run=(game)=>{const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-assert7-'));const file=path.join(dir,'a.ps1');
  fs.writeFileSync(file,"$ErrorActionPreference='Stop'\r\n. '"+path.join(repo,'scripts','find-cof.ps1')+"'\r\nAssert-CofAdapter '"+game+"'\r\nWrite-Output 'passed'\r\n");
  const r=ps(file);fs.rmSync(dir,{recursive:true,force:true});return r;};
 let game=fakeGame(true,false);let r=run(game);
 assert.equal(r.status,1);assert.ok(!r.stdout.includes('passed')&&ascii(r.stdout));
 assert.ok(r.stdout.includes('blocked this file: '+path.join(game,'cl_dlls','storycoop_rc4.dll')));
 assert.ok(r.stdout.includes('the extracted "Cry of Fear Co-op" folder, and the game\'s cryoffear\\cl_dlls'));
 const help=r.stdout.split(/\r?\n/).filter(l=>l.startsWith('HELP: '));assert.equal(help.length,3);assert.ok(help.every(l=>l.length<=110)&&help[1].includes('cl_dlls'));
 assert.ok(r.stderr.includes('storycoop_rc4.dll is missing from cryoffear\\cl_dlls although the co-op maps are installed'));
 fs.rmSync(game,{recursive:true,force:true});
 for(const [maps,adapter] of [[true,true],[false,false]]){game=fakeGame(maps,adapter);r=run(game);assert.equal(r.status,0,r.stderr);assert.ok(r.stdout.includes('passed')&&!r.stdout.includes('HELP:'));fs.rmSync(game,{recursive:true,force:true});}
});

test('F2 find-cof.ps1: Get-CofSha is the file SHA256 (no Get-FileHash, which drops the antivirus code)',()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-sha7-'));const data=path.join(dir,'x.bin');fs.writeFileSync(data,Buffer.from('cry of fear'));
 const file=path.join(dir,'s.ps1');fs.writeFileSync(file,". '"+path.join(repo,'scripts','find-cof.ps1')+"'\r\nWrite-Output (Get-CofSha '"+data+"')\r\n");
 const r=ps(file);fs.rmSync(dir,{recursive:true,force:true});
 assert.equal(r.status,0,r.stderr);assert.equal(r.stdout.trim(),crypto.createHash('sha256').update('cry of fear').digest('hex'));
 for(const f of ['scripts/host-story-coop.ps1','scripts/join-coop.ps1','scripts/update-coop.ps1','scripts/setup-coop.ps1']){
  const s=read(f);assert.ok(s.includes('Get-CofSha'),f);assert.ok(!/Get-FileHash -LiteralPath \$adapter/.test(s)&&!s.includes('ReadAllBytes($dll)'),f);}
});

test('F2 join: the co-op maps without the adapter stop Join with the steps',()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-join7-'));
 for(const f of ['join-coop.ps1','find-cof.ps1'])fs.copyFileSync(path.join(repo,'scripts',f),path.join(dir,f));
 fs.writeFileSync(path.join(dir,'release.json'),JSON.stringify({maps:[{map:'cof_campaign_story4',outputSha256:'0'.repeat(64)}],runtimeAdapter:{sha256:'1'.repeat(64)},coopFixes:{id:'v0.10.7'}}));
 const game=fakeGame(true,false);
 const r=spawnSync('powershell.exe',['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',path.join(dir,'join-coop.ps1'),'-HostAddress','127.0.0.1','-GameDirectory',game],{encoding:'utf8'});
 fs.rmSync(dir,{recursive:true,force:true});fs.rmSync(game,{recursive:true,force:true});
 assert.equal(r.status,1);assert.ok(r.stdout.includes('Protection history')&&r.stdout.includes('HELP: Windows Defender blocked storycoop_rc4.dll (a false alarm).'));
 assert.ok(r.stderr.includes('most likely removed it')&&!r.stdout.includes('Starting Cry of Fear'));   // v0.10.6: "The co-op mod is not installed"
});

test('F2 Run Co-op Mod, update check and setup: the same steps for an adapter removed after Install',()=>{
 const host=read('scripts/host-story-coop.ps1');
 assert.ok(host.includes('if ($NativeRC4 -and $cof) { Assert-CofAdapter $GameDirectory }'));
 assert.ok(host.indexOf('Assert-CofAdapter $GameDirectory')<host.indexOf("The co-op mod is not installed in '"));   // before the old "not installed" line
 assert.ok(host.indexOf('if ($MenuOnly) { $NativeRC4=$true }')<host.indexOf('Assert-CofAdapter $GameDirectory'));   // Run Co-op Mod (-MenuOnly) is covered
 assert.ok(read('scripts/update-coop.ps1').includes("elseif ($game -and (Test-CofAdapterGone $game)) {"));
 const setup=read('scripts/setup-coop.ps1');
 assert.ok(setup.includes("-VerifyOnly | ForEach-Object { if ($_ -match '^COOP-BLOCKED (.+)$') { $blocked=$Matches[1] } }"));
 assert.ok(setup.includes("$gone=@($bad | Where-Object { $_ -match '\\.dll$' -and -not [IO.File]::Exists((Join-Path $game $_)) }"));
 assert.ok(setup.indexOf('Write-CofBlocked (Join-Path $game $gone[0])')<setup.indexOf('Not as expected: $_'));   // v0.10.6: "Not as expected: cl_dlls\storycoop_rc4.dll"
 for(const f of ['scripts/install-story-package.ps1','scripts/setup-coop.ps1','scripts/find-cof.ps1','scripts/host-story-coop.ps1','scripts/join-coop.ps1','scripts/update-coop.ps1'])assert.ok(ascii(read(f)),f+' must stay ASCII for Windows PowerShell 5.1');
});

test('F2 READ ME: both folders are excluded, nothing is called fixed',()=>{
 const text=readMe('v0.10.7');const part=text.slice(text.indexOf('IF WINDOWS DEFENDER'),text.indexOf('PLAYING'));
 assert.ok(part.includes('Folder, twice: pick this "Cry of Fear Co-op" folder,')&&part.includes("the game's cryoffear\\cl_dlls folder."));
 assert.ok(ascii(text)&&!/fixed/i.test(part));
});
