import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from '../scripts/bsp.mjs';
import {CAMPAIGN12_PARENT} from '../scripts/package-state-rc4.mjs';
import {PLAYER_FOLDER,LAUNCHER_EXE,PLAYER_SCRIPTS,main as playerPackage} from '../scripts/player-package.mjs';
const repo=fileURLToPath(new URL('..',import.meta.url));
const pkg=process.env.COF_CAMPAIGN12_PACKAGE,previous=process.env.COF_CAMPAIGN11_PACKAGE;
const ready=!!(pkg&&previous&&fs.existsSync(path.join(pkg,'release.json'))&&fs.existsSync(path.join(previous,'release.json')));
const read=(...p)=>fs.readFileSync(path.join(...p));
const pairs=text=>Object.fromEntries([...text.matchAll(/"([^"]*)"\s+"([^"]*)"/g)].map(m=>[m[1],m[2]]));

test('coop-fixes-012 keeps the maps, Simon and the title; new adapter and Load Slot entries',{skip:!ready},()=>{
 const before=read(previous,'release.json');assert.equal(sha256(before),CAMPAIGN12_PARENT);
 const now=JSON.parse(read(pkg,'release.json')),old=JSON.parse(before);
 assert.equal(now.coopFixes.id,'coop-fixes-012');assert.deepEqual(now.coopFixes.changedMaps,[]);
 for(const m of now.maps){const o=old.maps.find(x=>x.map===m.map);assert.equal(m.outputSha256,o.outputSha256);assert.equal(m.planSha256,o.planSha256);}
 for(const kind of ['build','replaces'])assert.deepEqual(now.extras.find(x=>x[kind]),old.extras.find(x=>x[kind]));
 const entries=now.extras.filter(x=>x.target.endsWith('.custom')).map(x=>x.target).sort();
 assert.deepEqual(entries,['maps/storycoop_new.custom','maps/storycoop_slot1.custom','maps/storycoop_slot2.custom','maps/storycoop_slot3.custom']);
 for(const x of now.extras.filter(e=>e.target.endsWith('.custom'))){const bytes=read(pkg,x.file);assert.equal(sha256(bytes),x.sha256);
  const p=pairs(bytes.toString('latin1'));assert.ok(p.description.startsWith("Simon's Story: ")&&p.description.length<=26,p.description);assert.match(p.firstmap,/^sc4coop_(new|slot[1-3])$/);}
 assert.notEqual(now.runtimeAdapter.sha256,old.runtimeAdapter.sha256);assert.equal(sha256(read(pkg,'native','storycoop_rc4.dll')),now.runtimeAdapter.sha256);
 const known=JSON.parse(read(pkg,'known-builds.json'));assert.equal(known.builds[0].catalogSha256,CAMPAIGN12_PARENT);
 // The 011 Continue entry is a known older file, so the updater sets it aside.
 assert.ok(known.builds[0].extras.some(x=>x.target==='maps/storycoop_continue.custom'));
 // coop-fixes-013 changed the launcher/installer scripts: 012 must carry its own.
 for(const f of PLAYER_SCRIPTS)assert.ok(read(pkg,f).length>0,f);
});

test('the player folder holds the launcher, READ ME and only the files it uses',{skip:!ready},()=>{
 const out=fs.mkdtempSync(path.join(os.tmpdir(),'coop-player-'));fs.rmdirSync(out);
 try{
  const summary=playerPackage([pkg,out]),root=path.join(out,PLAYER_FOLDER);
  assert.deepEqual(fs.readdirSync(root).sort(),[LAUNCHER_EXE,'READ ME.txt','files','playermodels','handskins'].sort());
  const catalog=JSON.parse(read(root,'files','release.json'));assert.equal(summary.version,catalog.coopFixes.id);
  for(const m of catalog.maps)assert.equal(sha256(read(root,'files','plans',m.map+'.json')),m.planSha256);
  assert.equal(sha256(read(root,'files','native','storycoop_rc4.dll')),catalog.runtimeAdapter.sha256);
  for(const x of catalog.extras.filter(e=>e.file))assert.equal(sha256(read(root,'files',x.file)),x.replaces?x.deltaSha256:x.sha256);
  const all=[];const walk=d=>{for(const e of fs.readdirSync(d,{withFileTypes:true}))e.isDirectory()?walk(path.join(d,e.name)):all.push(path.relative(root,path.join(d,e.name)));};walk(root);
  for(const f of all)assert.doesNotMatch(f,/rollback|source|baseline|state-report|QA\.json|TESTING|CHECKPOINT|SOURCE|\.md$/i,f);
  assert.equal(all.length,3+summary.files);
  assert.match(read(root,'READ ME.txt').toString('latin1'),/click Install[\s\S]*Run Co-op Mod[\s\S]*Uninstall[\s\S]*storycoop_logs/);
  assert.throws(()=>playerPackage([pkg,out]),/already exists/);
 }finally{fs.rmSync(out,{recursive:true,force:true});}
});

test('012 adapter: slot list at the recorder, partner saves, Load Slot entries, only the F flashlight off',()=>{
 const checkpoint=fs.readFileSync(path.join(repo,'native','rc4','checkpoint-runtime.hpp'),'utf8'),session=fs.readFileSync(path.join(repo,'native','rc4','session-runtime.hpp'),'utf8');
 assert.ok(checkpoint.includes('save_choice_begin(message.actor,recorder)')&&!checkpoint.includes('Only the host can save the game'));
 assert.ok(checkpoint.includes('value.owner=player_identity(host)')&&checkpoint.includes("on the host's PC."));
 assert.ok(session.includes('"sc4coop_slot",12')&&!session.includes('Glock light switched off')&&!session.includes('Maglite switched off'));
 for(const t of session.matchAll(/light_notify\(e,\d+u,"([^"]*)"/g))for(const line of t[1].split(String.fromCharCode(92)+'n'))assert.ok(line.length<=26,line);
});
