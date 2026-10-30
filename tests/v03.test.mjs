import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from '../scripts/bsp.mjs';
import {V03_PARENT} from '../scripts/package-state-rc4.mjs';
import {auditMap,sourceOf} from '../scripts/audit-v03.mjs';
import {readMe} from '../scripts/player-package.mjs';
const repo=fileURLToPath(new URL('..',import.meta.url));
const src=f=>fs.readFileSync(path.join(repo,'native','rc4',f),'utf8');
const pkg=process.env.COF_V03_PACKAGE,previous=process.env.COF_V02_PACKAGE;

test('v0.3 adapter: tape card saves, Drowned victim, map state wired at spawn/activate/deactivate/save/load',()=>{
 const format=src('checkpoint.cpp'),adapter=src('adapter.cpp'),coop=src('coop-runtime.hpp'),state=src('state-runtime.hpp'),ckp=src('checkpoint-runtime.hpp');
 assert.ok(format.includes("(c>='A'&&c<='Z')")&&format.includes('if(!n.empty()&&!card(n))return false;')&&format.includes('bool weapon(const std::string& s){return resource(s)'));
 assert.ok(adapter.includes('forwardEngine[72]=reinterpret_cast<void*>(&entity_of_index);')&&adapter.includes('drowned_begin(e);')&&adapter.includes('if(bar)actingPlayer=nullptr;drowned_end();'));
 assert.ok(coop.includes('load<Edict*>(p,0x184)')&&coop.includes('"monster_crazybitch"'));
 assert.ok(adapter.includes('native<void(__cdecl*)(Edict*,int,int)>(21)(list,state_activate_edicts(count),clients)')&&adapter.includes('checkpoint_game_over_check();state_deactivate();')&&adapter.includes('stateKey=state_identity(e);'));
 assert.ok(ckp.includes('state_save_slot(dir,checkpointSlot,value,why)')&&ckp.indexOf('state_save_slot(dir')<ckp.indexOf('!commit_checkpoint(dir,checkpointSlot,value)')&&ckp.includes('state_load_slot(slot,value);'));
 assert.ok(state.includes('native<void(__cdecl*)(Edict*,SaveData*)>(7)')&&state.includes('native<int(__cdecl*)(Edict*,SaveData*,int)>(8)')&&state.includes('storycoop_mapstate.off')&&state.includes('"cof_changelevel"'));
 // No per-map special case in the shared handler.
 assert.ok(!/apartment|ropeheh|Videotape/.test(state.replace(/^\/\/.*$/gm,'')));
 assert.ok(src('session-runtime.hpp').match(/MenuCredit="Co-op mod v0\.[4-9] developed by Anas Sammak \(koolkat\)/));
 // Follow-up (user host log 141912 and the first-player audit).
 const story=src('story-runtime.hpp'),pp=src('pp-runtime.hpp');
 assert.ok(story.includes('capture_player(e,p)||capture_player(e,p,false)')&&story.includes('carry_record(e)')&&story.includes(`"the last save's record"`));
 assert.ok(adapter.includes('if(acting_override(start,key,value,out))return out;')&&adapter.includes('actingPlayer=e;')&&adapter.includes('"boilercheck"')&&coop.includes('"monster_roofboss"'));
 assert.ok(pp.includes('"cof_playerbreathetoggle","cof_lookat","cof_stopwheelchair"')&&!pp.includes('"cof_phonedisable",')&&coop.includes('void half_frame()')&&adapter.includes('half_frame();')&&pp.includes('actingPlayer=e;native<void(__cdecl*)(Edict*,Edict*)>(3)(pp.actor,e);actingPlayer=nullptr;'));
 assert.ok(src('build.ps1').includes("'state-runtime.hpp'")&&src('test.ps1').includes("name='mapstate'"));
});

test('v0.3 coverage audit: families, exits and movewith per map',()=>{
 const e=pairs=>({pairs:Object.entries(pairs)});
 const r=auditMap([e({classname:'item_key',message:'inventoryitems/rope.txt'}),e({classname:'func_button',lockedby:'rope'}),e({classname:'monster_crazybitch'}),e({classname:'monster_generic'}),
  e({classname:'trigger_once',target:'sc_level_1'}),e({classname:'trigger_once',target:'scare'}),e({classname:'cof_changelevel',targetname:'sc_level_1',message:'s4_apartment3_e2'}),
  e({classname:'func_wall',movewith:'lift'}),e({classname:'func_train',targetname:'lift'}),e({classname:'multisource'}),e({classname:'trigger_auto',spawnflags:'1'})]);
 assert.equal(r.pickups,1);assert.deepEqual(r.keyItems,['rope.txt']);assert.deepEqual(r.locks,['rope']);assert.equal(r.monsters,1);assert.equal(r.drowned,1);
 assert.equal(r.onceTriggers,1);assert.equal(r.autoOnce,1);assert.equal(r.gates,1);assert.equal(r.moveWith,2);assert.equal(r.keptAsServed,2);assert.deepEqual(r.destinations,['s4_apartment3_e2']);
 assert.equal(sourceOf('s4_apartment5_e1'),'s4_apartment5');assert.equal(sourceOf('s4_start2'),'s4_start2');
 assert.ok(fs.readFileSync(path.join(repo,'docs','V03_COVERAGE.md'),'utf8').includes('80 source maps, 193 aliases'));
});

test('v0.3 READ ME names the version and map state',()=>{const t=readMe('v0.3');assert.ok(t.includes('Co-op mod v0.3 developed by Anas Sammak (koolkat).')&&t.includes('Maps you go back to stay as you left them'));});

test('v0.3 package: v0.2 parent, maps identical, new adapter',{skip:!(pkg&&previous&&fs.existsSync(path.join(pkg,'release.json')))},()=>{
 const before=fs.readFileSync(path.join(previous,'release.json'));assert.equal(sha256(before),V03_PARENT);
 const now=JSON.parse(fs.readFileSync(path.join(pkg,'release.json'))),old=JSON.parse(before);
 assert.equal(now.coopFixes.id,'v0.3');assert.equal(now.coopFixes.parentCatalog,V03_PARENT);assert.equal(now.coopFixes.mapsByteIdentical,true);assert.deepEqual(now.maps,old.maps);
 assert.notEqual(now.runtimeAdapter.sha256,old.runtimeAdapter.sha256);assert.equal(sha256(fs.readFileSync(path.join(pkg,'native','storycoop_rc4.dll'))),now.runtimeAdapter.sha256);
 assert.ok(now.nativeSource.some(s=>s.file==='state-runtime.hpp'));
 const known=JSON.parse(fs.readFileSync(path.join(pkg,'known-builds.json')));assert.equal(known.builds[0].catalogSha256,V03_PARENT);
});

test('public build: one "Run Co-op Mod.cmd" (v0.4: double-click hosts, "join [address]" joins), READ ME with logs and the Discord placeholder, GitHub README',async()=>{
 const {PLAYER_COMMANDS}=await import('../scripts/player-package.mjs');
 assert.deepEqual(Object.keys(PLAYER_COMMANDS).sort(),['Check for Updates.cmd','Install.cmd','Run Co-op Mod.cmd','Uninstall.cmd']);   // v0.5: install and update are separate
 const run=PLAYER_COMMANDS['Run Co-op Mod.cmd'];
 assert.ok(!/choice|errorlevel 2|Press H/.test(run)&&run.includes('set "cofFiles=%~dp0files"')&&run.includes('if /i "%~1"=="join" goto join'));
 assert.ok(run.includes(String.raw`-File "%cofFiles%\host-story-coop.ps1" -MenuOnly`)&&run.includes(String.raw`-File "%cofFiles%\join-coop.ps1" -HostAddress "%~2"`)&&run.includes(String.raw`if not exist "%cofFiles%\host-story-coop.ps1" goto missing`)&&run.endsWith('exit /b %cofExit%\r\n'));
 // The first command a double-click reaches is the host launch.
 const lines=run.split('\r\n');assert.ok(lines.findIndex(l=>l.includes('host-story-coop.ps1" -MenuOnly'))<lines.indexOf(':join'));
 const t=readMe('v0.3');assert.ok(t.includes('https://discord.gg/hgEGSf9MFZ')&&t.includes(String.raw`cryoffear\storycoop_logs`)&&t.includes('UNFINISHED')&&t.includes('connect 192.168.1.1:27015')&&!/Host a game|Join a game/.test(t));
 const gh=fs.readFileSync(path.join(repo,'docs','README.md'),'utf8');
 assert.ok(gh.includes('https://discord.gg/hgEGSf9MFZ')&&gh.includes('storycoop_logs')&&gh.includes('Run Co-op Mod.cmd')&&gh.includes('unfinished')&&gh.includes('connect 192.168.1.1:27015'));
 for(const f of ['setup-coop.ps1','install-story-package.ps1','host-story-coop.ps1','join-coop.ps1'])assert.ok(!/"Host a game|"Join a game|""Host a game/.test(fs.readFileSync(path.join(repo,'scripts',f),'utf8')),f);
});

test('v0.3 tester fixes: map kept only on a real level change; commands elevate for a protected folder or an admin Steam',()=>{
 const adapter=src('adapter.cpp'),state=src('state-runtime.hpp'),rd=f=>fs.readFileSync(path.join(repo,'scripts',f),'utf8');
 assert.ok(adapter.includes('carry_level_change(map);state_level_change(map);')&&adapter.includes('forwardEngine[39]=reinterpret_cast<void*>(&server_command_hook);')&&state.includes('MAPSTATE not kept: the map ended without a level change'));
 assert.ok(rd('find-cof.ps1').includes('function Invoke-CofElevated')&&rd('find-cof.ps1').includes('-Verb RunAs'));
 assert.ok(rd('setup-coop.ps1').includes('Test-CofWritable $game')&&rd('join-coop.ps1').includes('Test-SteamElevated')&&rd('host-story-coop.ps1').includes('Test-SteamElevated'));
});
