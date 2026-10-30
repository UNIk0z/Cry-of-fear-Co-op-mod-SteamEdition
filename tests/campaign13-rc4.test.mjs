import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from '../scripts/bsp.mjs';
import {CAMPAIGN13_PARENT} from '../scripts/build-campaign4-rc4.mjs';
import {CLIENT_TARGET,CLIENT_ORIGINAL_SHA256,clientPatches,patchClient} from '../scripts/client-menu-patch.mjs';
import {applyDelta} from '../scripts/menu-logo.mjs';
import {PLAYER_FOLDER,PLAYER_SCRIPTS,main as playerPackage} from '../scripts/player-package.mjs';
const repo=fileURLToPath(new URL('..',import.meta.url));
const pkg=process.env.COF_CAMPAIGN13_PACKAGE,previous=process.env.COF_CAMPAIGN12_PACKAGE,build=process.env.COF_CAMPAIGN13_BUILD,game=process.env.COF_GAME_DIRECTORY;
const ready=!!(pkg&&previous&&fs.existsSync(path.join(pkg,'release.json'))&&fs.existsSync(path.join(previous,'release.json')));
const read=(...p)=>fs.readFileSync(path.join(...p));
// The 2026-10-13 rebuild: the hall key and basement key doors on every alias
// of their maps, plus the prologue (it resets the two new door states).
const CHANGED=['cof_campaign_story4',...[0,1,2,3,4,5,6,7].map(i=>'s4_apartment1_e'+i),...[0,1,2].map(i=>'s4_apartment4_e'+i)].sort();

test('coop-fixes-013 package: only the key-door maps change; new adapter, client.dll menu and button extras',{skip:!ready},()=>{
 const before=read(previous,'release.json');assert.equal(sha256(before),CAMPAIGN13_PARENT);
 const now=JSON.parse(read(pkg,'release.json')),old=JSON.parse(before);
 assert.equal(now.coopFixes.id,'coop-fixes-013');assert.deepEqual([...now.coopFixes.changedMaps].sort(),CHANGED);
 assert.equal(now.maps.length,old.maps.length);
 for(const m of now.maps){const o=old.maps.find(x=>x.map===m.map);assert.ok(o,m.map);assert.equal(m.outputSha256!==o.outputSha256,CHANGED.includes(m.map),m.map);
  assert.equal(sha256(read(pkg,'plans',m.map+'.json')),m.planSha256);}
 assert.notEqual(now.runtimeAdapter.sha256,old.runtimeAdapter.sha256);assert.equal(sha256(read(pkg,'native','storycoop_rc4.dll')),now.runtimeAdapter.sha256);
 assert.ok(now.nativeSource.some(s=>s.file==='coop-runtime.hpp'));
 const client=now.extras.find(x=>x.target===CLIENT_TARGET);
 assert.ok(client&&client.file==='extras/client.coop-delta'&&client.optional&&client.replaces.sha256===CLIENT_ORIGINAL_SHA256&&client.replaces.fileBytes===client.fileBytes);
 assert.equal(sha256(read(pkg,client.file)),client.deltaSha256);
 for(const name of ['sc_host','sc_host_mo','sc_join','sc_join_mo']){const x=now.extras.find(e=>e.target==='gfx/vgui/menu/'+name+'.tga');assert.ok(x&&x.file==='extras/'+name+'.tga'&&!x.replaces,name);assert.equal(sha256(read(pkg,x.file)),x.sha256);assert.ok(read(pkg,x.file).equals(read(repo,'scripts','menu',name+'.tga')));}
 // Unchanged from 012: the title delta, the menu entries and Simon's model.
 for(const target of ['gfx/vgui/menu_logo.tga','maps/storycoop_new.custom','maps/storycoop_slot1.custom','models/player/simon/simon.mdl'])assert.deepEqual(now.extras.find(x=>x.target===target),old.extras.find(x=>x.target===target),target);
 const known=JSON.parse(read(pkg,'known-builds.json'));assert.equal(known.builds[0].catalogSha256,CAMPAIGN13_PARENT);
 for(const f of PLAYER_SCRIPTS)assert.ok(read(pkg,f).equals(read(repo,'scripts',f)),f);
 // Rebuilt on this PC from its own client.dll, the delta gives exactly the patched file.
 if(game&&fs.existsSync(path.join(game,'cl_dlls','client.dll'))){
  const kept=path.join(game,'cl_dlls','client.dll.storycoop-original'),original=read(fs.existsSync(kept)?kept:path.join(game,'cl_dlls','client.dll'));
  const rebuilt=applyDelta(original,read(pkg,client.file));assert.equal(sha256(rebuilt),client.sha256);assert.ok(rebuilt.equals(patchClient(original)));
 }
});

test('client.dll menu patch: 50 checked sites, same length, no overlaps; refuses another client.dll',()=>{
 const patches=clientPatches();assert.equal(patches.length,50);
 const spans=patches.map(p=>[p.va,p.va+p.from.length]).sort((a,b)=>a[0]-b[0]);for(let i=1;i<spans.length;i++)assert.ok(spans[i][0]>=spans[i-1][1]);
 for(const p of patches)assert.equal(p.from.length,p.to.length);
 assert.throws(()=>patchClient(Buffer.alloc(1754112)),/not the expected version/);
 // Slot images point at files that exist in the game or in this package.
 const images=patches.filter(p=>p.why.startsWith('menu image ')).map(p=>p.why.slice(11));
 assert.deepEqual(images,['gfx/vgui/menu/sc_host.tga','gfx/vgui/menu/sc_host_mo.tga','gfx/vgui/menu/sc_join.tga','gfx/vgui/menu/sc_join_mo.tga','gfx/vgui/menu/options.tga','gfx/vgui/menu/options_mo.tga','gfx/vgui/menu/quit.tga','gfx/vgui/menu/quit_mo.tga']);
});

test('campaign13 build: script-given hall/basement keys convert their doors',{skip:!(build&&fs.existsSync(path.join(build,'state-report.json')))},()=>{
 const info=JSON.parse(read(build,'state-report.json'));assert.equal(info.campaign13,true);assert.equal(info.campaign13PreviousCatalogSha256,CAMPAIGN13_PARENT);assert.deepEqual([...info.campaign13ChangedMaps].sort(),CHANGED);
 for(const [map,entity,key] of [['s4_apartment1_e0',36,'hallkey'],['s4_apartment4_e0',190,'basementkey']]){const d=JSON.parse(read(build,map+'.json')).doorDecisions.find(x=>x.entity===entity);assert.equal(d.status,'CONVERTED_KEY_DOOR',map);assert.equal(d.key,key);}
});

test('013 adapter source: carry, story weapons, camera hiding, stuck, regroup, credit, net tune',()=>{
 const src=f=>fs.readFileSync(path.join(repo,'native','rc4',f),'utf8');const coop=src('coop-runtime.hpp'),session=src('session-runtime.hpp'),story=src('story-runtime.hpp'),adapter=src('adapter.cpp');
 for(const s of ['void carry_frame(','void give_lookup(','bool hide_player_from(','void stuck_command(','void regroup_frame('])assert.ok(coop.includes(s),s);
 assert.ok(adapter.includes('#include "coop-runtime.hpp"')&&adapter.includes('hide_player_from(e,host)')&&/carry_frame\(e\);[^\n]*?(give_owed_frame\(e\);)?rejoin_frame\(e\);/.test(adapter));
 // v0.1 renamed the thanks line (tests/v01.test.mjs checks the exact text).
 assert.ok(session.includes('developed by Anas Sammak (koolkat)\\nSpecial thanks'));
 assert.ok(story.includes('rate 100000\\ncl_updaterate 100\\ncl_cmdrate 100\\nex_interp 0.06\\n'));
});

test('013 player folder: READ ME names the new menu, stuck, Steam names and the credits; ships the menu files',{skip:!ready},()=>{
 const out=fs.mkdtempSync(path.join(os.tmpdir(),'coop13-player-'));fs.rmdirSync(out);
 try{
  const summary=playerPackage([pkg,out]),root=path.join(out,PLAYER_FOLDER),text=read(root,'READ ME.txt').toString('latin1');
  assert.equal(summary.version,'coop-fixes-013');
  for(const s of ['Anas Sammak (koolkat)','Special thanks: Mesilll, Michal Eminov, caixa','"Host Simon\'s Story"','type: stuck','Steam name','pick a save slot'])assert.ok(text.includes(s),s);
  for(const f of ['extras/client.coop-delta','extras/sc_host.tga','extras/sc_join_mo.tga'])assert.ok(fs.existsSync(path.join(root,'files',f)),f);
 }finally{fs.rmSync(out,{recursive:true,force:true});}
});
