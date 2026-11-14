// v0.10.6 (Claude, 2026-11-13): playtester fixes. The installer's read guard runs for real in Windows
// PowerShell with a fake blocked read (an IOException with ERROR_VIRUS_INFECTED, the error the tester's
// launcher.log shows) and a package file taken away; the rest are static checks. The adapter changes are
// covered by native/rc4/test-qol2.cpp and test-mapstate.cpp; the map audit by scripts/audit-v0106.mjs.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {readMe} from '../scripts/player-package.mjs';
import {auditEntities} from '../scripts/audit-v0106.mjs';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');
const ascii=s=>/^[\x09\x0a\x0d\x20-\x7e]*$/.test(s);

function guard(body){
 const script=read('scripts/install-story-package.ps1');
 const block=script.slice(script.indexOf('# BEGIN read guard'),script.indexOf('# END read guard'));
 assert.ok(block.length>200);
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-guard-'));fs.mkdirSync(path.join(dir,'native'));
 fs.writeFileSync(path.join(dir,'native','storycoop_rc4.dll'),Buffer.from([1,2,3]));
 const file=path.join(dir,'guard.ps1');fs.writeFileSync(file,"$ErrorActionPreference='Stop'\r\n"+block+'\r\n'+body+'\r\n');
 const r=spawnSync('powershell.exe',['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',file],{encoding:'utf8'});
 fs.rmSync(dir,{recursive:true,force:true});
 return r;
}
const blocked="$script:CoopRead={ param([string]$Path) throw [IO.IOException]::new('Det gick inte att slutfora atgarden eftersom filen innehaller ett virus',-2147024671) }";

test('installer: a read Windows Defender blocks names the file and stops with exit code 3, in ASCII',()=>{
 const r=guard(blocked+"\r\n$adapterBytes=(Read-CoopBytes (Join-Path $PSScriptRoot 'native\\storycoop_rc4.dll'))\r\nWrite-Output 'not reached'");
 assert.equal(r.status,3,r.stderr);
 assert.match(r.stdout,/^COOP-BLOCKED .*native\\storycoop_rc4\.dll\r?$/m);
 assert.ok(r.stdout.includes('Nothing was installed.')&&!r.stdout.includes('not reached'));
 assert.ok(ascii(r.stdout)&&r.stderr.trim()==='',r.stderr);   // no raw error record (the tester's was in Swedish, as mojibake)
});

test('installer: a block swallowed by a try/catch still ends with the steps, not the next error',()=>{
 const r=guard(blocked+"\r\ntry { $x=(Read-CoopBytes (Join-Path $PSScriptRoot 'native\\storycoop_rc4.dll')) } catch { $x=$null }\r\nthrow 'Generated extra mismatch. Nothing installed.'");
 assert.equal(r.status,3);assert.match(r.stdout,/^COOP-BLOCKED /m);
});

test('installer: a package file an antivirus removed is named too; other errors stay as they were',()=>{
 let r=guard("$x=(Read-CoopBytes (Join-Path $PSScriptRoot 'native\\gone.dll'))");
 assert.equal(r.status,3);assert.match(r.stdout,/^COOP-BLOCKED .*native\\gone\.dll\r?$/m);
 r=guard("$x=(Read-CoopBytes 'Z:\\no\\such\\game\\file.bsp')");
 assert.notEqual(r.status,3);assert.equal(r.status,1);assert.ok(!r.stdout.includes('COOP-BLOCKED'));
 r=guard("$x=(Read-CoopBytes (Join-Path $PSScriptRoot 'native\\storycoop_rc4.dll'))\r\nWrite-Output ($x.GetType().Name+' '+$x.Length)");
 assert.equal(r.status,0);assert.match(r.stdout,/Byte\[\] 3/);   // an ordinary read is one byte array, not a list of bytes
});

test('installer: every file read goes through the guard; setup-coop and the launcher show the steps',()=>{
 const script=read('scripts/install-story-package.ps1');
 const after=script.slice(script.indexOf('# END read guard'));
 assert.ok(!after.includes('[IO.File]::ReadAllBytes(')&&after.includes('(Read-CoopBytes '));
 const setup=read('scripts/setup-coop.ps1');
 assert.ok(setup.includes("if ($LASTEXITCODE -eq 3 -and $blocked) { Write-CofBlocked $blocked;")&&setup.includes("'^COOP-BLOCKED (.+)$'"));
 assert.ok(read('scripts/find-cof.ps1').includes('if (Test-CofBlocked $_.Exception) { Write-CofBlocked $Path;')&&setup.includes('Get-CofSha $Path'));   // v0.10.7: one hash for setup, host, join and update
 assert.ok(read('scripts/host-story-coop.ps1').includes('Get-CofSha $adapterFile'));
 assert.ok(read('launcher/CoopLauncher.cs').includes('entry.StartsWith("HELP: ", StringComparison.Ordinal)'));
 for(const f of ['scripts/install-story-package.ps1','scripts/setup-coop.ps1','scripts/find-cof.ps1','scripts/host-story-coop.ps1'])assert.ok(ascii(read(f)),f+' must stay ASCII for Windows PowerShell 5.1');
});

test('find-cof.ps1 prints the steps and short HELP lines for the launcher',()=>{
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'cof-help-'));const file=path.join(dir,'help.ps1');
 fs.writeFileSync(file,". '"+path.join(repo,'scripts','find-cof.ps1')+"'\r\nWrite-CofBlocked 'C:\\Games\\Cry of Fear Co-op\\files\\native\\storycoop_rc4.dll'\r\n"+
  "Write-Output (Test-CofBlocked ([Management.Automation.MethodInvocationException]::new('x',[IO.IOException]::new('v',-2147024671))))\r\n"+
  "Write-Output (Test-CofBlocked ([IO.IOException]::new('v',-2147024670)))\r\nWrite-Output (Test-CofBlocked ([IO.IOException]::new('sharing',-2147024864)))\r\n");
 const r=spawnSync('powershell.exe',['-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',file],{encoding:'utf8'});
 fs.rmSync(dir,{recursive:true,force:true});
 assert.equal(r.status,0,r.stderr);assert.ok(ascii(r.stdout));
 const lines=r.stdout.split(/\r?\n/);
 assert.ok(lines.includes('Windows Defender (or another antivirus) blocked this file: C:\\Games\\Cry of Fear Co-op\\files\\native\\storycoop_rc4.dll'));
 assert.ok(lines.some(l=>l.includes('Protection history'))&&lines.some(l=>l.includes('Exclusions'))&&lines.some(l=>l.includes('Extract the ZIP again'))&&lines.some(l=>l.includes('Click Install again')));
 const help=lines.filter(l=>l.startsWith('HELP: '));assert.equal(help.length,3);
 assert.ok(help.every(l=>l.length<=110)&&help[0]==='HELP: Windows Defender blocked storycoop_rc4.dll (a false alarm).');
 assert.deepEqual(lines.filter(l=>l==='True'||l==='False'),['True','True','False']);
});

test('the player READ ME explains a Defender block as a false alarm, with the steps, and claims no fix',()=>{
 const text=readMe('v0.10.6');
 assert.ok(text.includes('IF WINDOWS DEFENDER BLOCKS A FILE')&&text.includes('This is a false alarm.')&&text.includes('Protection history')&&text.includes('Exclusions')&&text.includes('Extract the ZIP again'));
 assert.ok(ascii(text)&&!/fixed/i.test(text.slice(text.indexOf('IF WINDOWS DEFENDER'),text.indexOf('PLAYING'))));
});

test('audit: the stad/subway transit room and the stad fuse train are found',()=>{
 const e=(pairs)=>({pairs:Object.entries(pairs)});
 const entities=[e({classname:'worldspawn'}),
  e({classname:'trigger_multiple',model:'*1',wait:'2',target:'tele'}),e({classname:'cof_teleport',targetname:'tele',target:'tele_ld'}),
  e({classname:'trigger_once',model:'*2',target:'sc_level_1'}),e({classname:'cof_changelevel',targetname:'sc_level_1',message:'s4_stad_e1'}),
  e({classname:'cof_coop_spawn1',origin:'-552 -529 -491',angles:'0 0 0'}),e({classname:'cof_coop_spawn2',origin:'-516 -529 -491',angles:'0 0 0'}),
  e({classname:'func_train',targetname:'1trainbutton',model:'*3',target:'1stop1'}),e({classname:'func_button',model:'*4',movewith:'1trainbutton',target:'multipoff2',wait:'-1'}),
  e({classname:'trigger_once',model:'*5',target:'1trainbutton'})];
 const brushes=[[0,0,0,0,0,0],[-584,-573,-535,-504,-481,-404],[-476,-573,-535,-396,-481,-404],[-8,-8,-8,8,8,8],[-653,59,-305,-637,61,-295],[0,0,0,10,10,10]];
 const r=auditEntities(entities,brushes);
 assert.equal(r.spawns.length,2);assert.ok(r.spawns.every(s=>s.inTransit.length===1&&s.inTransit[0].startsWith('*1 wait 2')));
 assert.deepEqual(r.spawns.map(s=>s.nearExit),[['*2'],['*2']]);   // 60 and 24 units from the way back
 assert.ok(r.spawns.every(s=>s.inExit.length===0));
 const g=r.groups.find(x=>x.name==='1trainbutton');assert.ok(g.usable&&g.oneShot&&g.missingInV0105Saves&&!g.keptAsServedNow);
});
