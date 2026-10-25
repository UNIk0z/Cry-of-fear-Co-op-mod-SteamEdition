import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from '../scripts/bsp.mjs';
import {V01_PARENT,V01_LOGO_TARGET} from '../scripts/package-state-rc4.mjs';
import {PLAYER_SCRIPTS,PLAYER_FOLDER,readMe,main as playerPackage} from '../scripts/player-package.mjs';
import {clientPatches} from '../scripts/client-menu-patch.mjs';
import {SIMON_ACTOR_MODELS,auditEntities,groupBySource,markdown} from '../scripts/audit-v01.mjs';
const repo=fileURLToPath(new URL('..',import.meta.url));
const src=f=>fs.readFileSync(path.join(repo,f),'utf8');
const pkg=process.env.COF_V01_PACKAGE,previous=process.env.COF_CAMPAIGN14_PACKAGE;
const ready=!!(pkg&&previous&&fs.existsSync(path.join(pkg,'release.json'))&&fs.existsSync(path.join(previous,'release.json')));
const read=(...p)=>fs.readFileSync(path.join(...p));

test('v0.1 adapter source: no forced title-train send, exact credits, host-only saves, names, carry at ChangeLevel',()=>{
 const adapter=src('native/rc4/adapter.cpp'),coop=src('native/rc4/coop-runtime.hpp'),session=src('native/rc4/session-runtime.hpp'),checkpoint=src('native/rc4/checkpoint-runtime.hpp');
 for(const gone of ['check_visibility','cinematicPackVehicle','cinematicViews','VEHICLE_PACK retry'])assert.ok(!adapter.includes(gone),gone);
 assert.ok(['v0.1','v0.2','v0.3','v0.4','v0.5','v0.6'].some(n=>session.includes('constexpr const char* MenuCredit="Co-op mod '+n+' developed by Anas Sammak (koolkat)\\nSpecial thanks: Mesilll, Michal Eminov, caixa')));   // v0.2 since 2026-10-16
 assert.ok(!session.includes('"Co-op mod developed by Anas Sammak (koolkat)\\nSpecial thanks to Michael Eminov"'));
 assert.ok(coop.includes('constexpr const char* HostOnlySave="Only host can save the game";')||checkpoint.includes('"Only host can save the game"'));   // v0.2: shown on Use only
 assert.ok(checkpoint.includes('if(actor!=host){checkpoint_notice(actor,"[STORYCOOP] Save refused: only the host can save the game.\\n");return;}'));
 assert.ok(!checkpoint.includes('Use again: slot'));
 assert.ok(checkpoint.includes('"play save/save_sequence.wav\\n"'));
 assert.ok(adapter.includes('forwardEngine[6]=reinterpret_cast<void*>(&change_level);'));
 assert.ok(adapter.includes('const bool hide=player?hide_player_from(e,host):hide_actor_from(e,host);')||adapter.includes('const bool hide=player&&hide_player_from(e,host);'));   // v0.2: the maps hide staged actors
 assert.ok(coop.includes('SteamFriends')&&coop.includes('table[7]')&&coop.includes('"sc_name"'));
});

test('v0.1 Simon actor list: adapter and coverage audit name the same models',()=>{
 // v0.2 removed the adapter's Simon list (the maps hide staged actors at load); the audit list stays as the v0.1 record.
 const coop=src('native/rc4/coop-runtime.hpp');assert.ok(!coop.includes('simon_actor_model')&&SIMON_ACTOR_MODELS.length>0);
 assert.ok(!SIMON_ACTOR_MODELS.includes('simon_clothes.mdl'));
});

test('v0.1 in-memory menu: the adapter table is the reviewed client.dll patch plus the sc_logo title path',()=>{
 const table=src('native/rc4/client-menu.hpp'),js=clientPatches();
 const vas=[...table.matchAll(/0x(1[0-9A-Fa-f]{7})u?\b/g)].map(m=>parseInt(m[1],16)).filter(v=>v>=0x10001000&&v<0x10200000);
 // The ten click-id jump-table entries are written as 0x1002DF68+i*4 in the adapter.
 for(const p of js)assert.ok(p.why.startsWith('menu click id')?p.va>=0x1002DF68&&p.va<0x1002DF68+40&&vas.includes(0x1002DF68):vas.includes(p.va),p.why);
 assert.ok(table.includes('p.push_back({0x1002DF68+i*4,client_dword(was[i]),client_dword(now[i]),true});'));
 assert.ok(table.includes('Image{0x1014389C,"gfx/vgui/menu_logo.tga","gfx/vgui/sc_logo.tga",23}'));
 for(const p of js.filter(p=>p.from.length===26||p.from.length===29||p.from.length===27||p.from.length===30)){const was=p.from.toString('latin1').replace(/\0+$/,''),is=p.to.toString('latin1').replace(/\0+$/,'');assert.ok(table.includes(`"${was}","${is}",${p.from.length}`),was);}
 assert.equal(V01_LOGO_TARGET,'gfx/vgui/sc_logo.tga');
});

test('v0.1 launchers: Join loads the co-op adapter; both pass the Steam name as sc_name',()=>{
 const join=src('scripts/join-coop.ps1'),host=src('scripts/host-story-coop.ps1');
 assert.ok(join.includes("$launch=@('-applaunch','223710','-dll','cl_dlls/storycoop_rc4.dll'"));
 for(const s of [join,host])assert.ok(s.includes("'+setinfo','sc_name'"));
});

test('v0.1 READ ME: version, credits, host-only saving, base-game menu',()=>{
 const text=readMe('v0.1');
 for(const s of ['(v0.1)','developed by Anas Sammak (koolkat).','Special thanks: Mesilll, Michal Eminov, caixa, joah.','Only the host can save.','"Only host can save the','shows the game\'s own menu'])assert.ok(text.includes(s),s);
 assert.ok(!/Michael|Either player can save/.test(text));
});

test('v0.1 coverage audit counts the shared-fix targets of a map',()=>{
 const e=(pairs)=>({pairs:Object.entries(pairs)});
 const ents=[e({classname:'trigger_camera'}),e({classname:'env_model',model:'models/cutscene/player3.mdl'}),e({classname:'env_model',model:'models/cutscene/simon_clothes.mdl'}),
  e({classname:'cof_giveitems',message:'weapon_glock'}),e({classname:'cof_giveitems',message:'inventoryitems/hallkey.txt',spawnflags:'1'}),
  e({classname:'func_button',target:'cof_story_present12',master:'gate'}),e({classname:'func_button',target:'cof_story_present13'}),e({classname:'inter_door',spawnflags:'4'}),
  e({classname:'trigger_coop_changelevel'}),e({classname:'tape_recorder'}),e({classname:'env_global',globalstate:'sc4.event.x.done'})];
 const r=auditEntities(ents);
 assert.deepEqual(r,{cameras:1,simonActors:1,weaponGrants:['weapon_glock'],keyGrants:1,coopDoors:2,storyLockedDoors:1,keyBridges:0,nativeDisabledDoors:1,transitions:1,recorders:1,latches:1,endingEntities:0});
 const md=markdown(groupBySource([{map:'s4_apartment1_e0',sourceMap:'c_apartment1',...r},{map:'s4_apartment1_e1',sourceMap:'c_apartment1',...r}]),[{map:'a',...r}]);
 assert.ok(md.includes('| c_apartment1 | 2 | 1 / 1 | weapon_glock | 2 (1) / - / 1 | 1 | 1 | 1 / - | played in 013/014 tests'));
});

test('v0.1 package: adapter and package only over 014; no game file replaced; title rebuilt as sc_logo.tga',{skip:!ready},()=>{
 const before=read(previous,'release.json');assert.equal(sha256(before),V01_PARENT);
 const now=JSON.parse(read(pkg,'release.json')),old=JSON.parse(before);
 assert.equal(now.coopFixes.id,'v0.1');assert.equal(now.coopFixes.version,'v0.1');assert.equal(now.coopFixes.mapsByteIdentical,true);assert.deepEqual(now.coopFixes.changedMaps,[]);
 assert.deepEqual(now.maps,old.maps);for(const m of now.maps)assert.equal(sha256(read(pkg,'plans',m.map+'.json')),m.planSha256);
 assert.ok(!now.extras.some(x=>x.replaces),'no replaced game file');
 const logo=now.extras.find(x=>x.target==='gfx/vgui/sc_logo.tga'),oldLogo=old.extras.find(x=>x.target==='gfx/vgui/menu_logo.tga');
 assert.equal(logo.deltaFrom.target,'gfx/vgui/menu_logo.tga');assert.equal(logo.deltaFrom.sha256,oldLogo.replaces.sha256);assert.equal(logo.sha256,oldLogo.sha256);assert.equal(sha256(read(pkg,logo.file)),logo.deltaSha256);
 assert.deepEqual(now.extras.filter(x=>x.target.startsWith('maps/')).map(x=>x.target),['maps/storycoop_new.coopm','maps/storycoop_slot1.coopm','maps/storycoop_slot2.coopm','maps/storycoop_slot3.coopm']);
 for(const kind of ['new','slot1']){const a=now.extras.find(x=>x.target==='maps/storycoop_'+kind+'.coopm'),b=old.extras.find(x=>x.target==='maps/storycoop_'+kind+'.custom');assert.equal(a.sha256,b.sha256);}
 assert.ok(!now.extras.some(x=>x.target==='cl_dlls/client.dll'));
 assert.notEqual(now.runtimeAdapter.sha256,old.runtimeAdapter.sha256);assert.equal(sha256(read(pkg,'native','storycoop_rc4.dll')),now.runtimeAdapter.sha256);
 assert.equal(now.nativeSource.filter(f=>f.file==='client-menu.hpp').length,1);
 const known=JSON.parse(read(pkg,'known-builds.json'));assert.equal(known.builds[0].catalogSha256,V01_PARENT);
 const replaced=Object.fromEntries(known.replacedFiles.map(r=>[r.target,r]));
 assert.equal(replaced['gfx/vgui/menu_logo.tga'].originalSha256,oldLogo.replaces.sha256);assert.ok(replaced['gfx/vgui/menu_logo.tga'].coopSha256.includes(oldLogo.sha256));
 const oldClient=old.extras.find(x=>x.target==='cl_dlls/client.dll');assert.equal(replaced['cl_dlls/client.dll'].originalSha256,oldClient.replaces.sha256);assert.ok(replaced['cl_dlls/client.dll'].coopSha256.includes(oldClient.sha256));
 for(const f of PLAYER_SCRIPTS)assert.ok(read(pkg,f).equals(read(repo,'scripts',f)),f);
 const out=fs.mkdtempSync(path.join(os.tmpdir(),'coop-v01-player-'));fs.rmdirSync(out);
 try{const summary=playerPackage([pkg,out]);assert.equal(summary.version,'v0.1');for(const f of ['extras/sc_logo.coop-delta','extras/storycoop_new.coopm'])assert.ok(fs.existsSync(path.join(out,PLAYER_FOLDER,'files',f)),f);assert.ok(!fs.existsSync(path.join(out,PLAYER_FOLDER,'files','extras','client.coop-delta')));}
 finally{fs.rmSync(out,{recursive:true,force:true});}
});
