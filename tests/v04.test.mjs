// v0.4 (Claude, 2026-10-18): the v0.3 audit fixes and the direct launcher.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from '../scripts/bsp.mjs';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const src=f=>fs.readFileSync(path.join(repo,'native','rc4',f),'utf8'),script=f=>fs.readFileSync(path.join(repo,'scripts',f),'utf8');

test('v0.4 removals leave the Spirit assist list and are not Activated',()=>{
 const world=src('world-runtime.hpp'),state=src('state-runtime.hpp'),adapter=src('adapter.cpp');
 assert.ok(world.includes('void entity_retire(Edict* e)')&&world.includes('FlKillMe|FlDormant')&&world.includes('store(d,0x60,load<unsigned>(d,0x60)&~LfAssistBits)')&&world.includes('if(d&&load<void*>(d,4)==p)'));
 assert.ok(state.includes('void state_kill(Edict* e){entity_retire(e);}')&&adapter.includes('constexpr std::size_t AssistLink=0x3c;'));
 // Every REMOVE_ENTITY (engine 22) the adapter issues is preceded by an unlink.
 for(const f of ['checkpoint-runtime.hpp','ladder-runtime.hpp','pp-runtime.hpp','world-runtime.hpp'])for(const line of src(f).split('\n'))if(line.includes('(22)('))assert.ok(line.includes('assist_unlink('),f+': '+line.trim().slice(0,80));
});

test('v0.4 scene teleports: exit after the camera, followers, return home; first-chance crash report; credit',()=>{
 const coop=src('coop-runtime.hpp');
 assert.ok(coop.includes('constexpr float SceneExitWindow=1.5f,SceneFollowGrace=30.f,SceneFollowRadius=96.f;')&&coop.includes('Edict* scene_watched(Edict* e,float now)')&&coop.includes('back where they stood before the scene'));
 assert.ok(coop.includes('sceneSeen.clear();sceneFollowers.clear();'));
 const session=src('session-runtime.hpp');
 assert.ok(session.includes('AddVectoredExceptionHandler(1,&crash_vectored)')&&session.match(/MenuCredit="Co-op mod v0\.[4-9](\.\d+)? developed by Anas Sammak \(koolkat\)/));
});

test('v0.4 vote to skip: SPACE votes, N/N fast-forwards the managers, allowlist, README',async()=>{
 const coop=src('coop-runtime.hpp'),adapter=src('adapter.cpp');
 assert.ok(coop.includes('constexpr int MmState=0xdc,MmCount=0xe0,MmIndex=0xe4,MmStart=0xe8,MmNames=0xec,MmDelays=0x12c,MmMode=0x178;')&&coop.includes('store(p,MmStart,load<float>(p,MmStart)-jump)')&&coop.includes('(buttons&0x2000u)!=0'));   // v0.5.5: R votes; v0.7: the managers jump to the scene's end
 const allow=coop.slice(coop.indexOf('bool skip_class('),coop.indexOf('void* manager_memory('));
 for(const c of ['func_train','scripted_sequence','cof_strangle','monstermaker','cof_credits'])assert.ok(!allow.includes('"'+c+'"'),c);
 assert.ok(adapter.includes('skip_frame();')&&adapter.includes('skip_command(e,command);'));
 const {readMe}=await import('../scripts/player-package.mjs');assert.ok(readMe('v0.4').includes('SKIPPING A CUTSCENE'));
 assert.ok(fs.readFileSync(path.join(repo,'docs','V04_RELEASE.md'),'utf8').includes('Vote to skip a cutscene'));
});

test('v0.4 launcher: double-click hosts and starts cof.exe directly while Steam runs',async()=>{
 const find=script('find-cof.ps1'),host=script('host-story-coop.ps1'),join=script('join-coop.ps1');
 assert.ok(find.includes('function Start-CofGame')&&find.includes("@('-game','cryoffear')+$Arguments")&&find.includes("'-applaunch','223710'")&&find.includes("WriteAllText($appId,'223710')"));
 assert.ok(host.includes('Start-CofGame $GameDirectory')&&join.includes('Start-CofGame $GameDirectory'));
 const {PLAYER_COMMANDS,readMe}=await import('../scripts/player-package.mjs');
 const run=PLAYER_COMMANDS['Run Co-op Mod.cmd'];assert.ok(!/choice/.test(run)&&run.includes('-MenuOnly'));
 const t=readMe('v0.4');assert.ok(t.includes('double-click "Run Co-op Mod.cmd"')&&t.includes('connect 192.168.1.1:27015')&&!/presses H|presses J/.test(t));
});

test('v0.4 package (set COF_V04_PACKAGE to the candidate folder)',{skip:!process.env.COF_V04_PACKAGE},()=>{
 const pkg=process.env.COF_V04_PACKAGE,now=JSON.parse(fs.readFileSync(path.join(pkg,'release.json')));
 assert.equal(now.coopFixes.id,'v0.4');assert.equal(now.hotfix.id,'v0.4');assert.equal(now.coopFixes.mapsByteIdentical,true);
 assert.equal(sha256(fs.readFileSync(path.join(pkg,'native','storycoop_rc4.dll'))),now.runtimeAdapter.sha256);
 const known=JSON.parse(fs.readFileSync(path.join(pkg,'known-builds.json')));
 assert.ok(known.builds.some(b=>b.adapter==='f353e88d486f893c2ca84efc9584215b8b7e431db83670e63cb3d3789375da23'));
 for(const f of ['README.md','TESTING.md'])assert.ok(fs.readFileSync(path.join(pkg,f),'utf8').includes('v0.4'),f);
});
