// v0.10.3 (Claude, 2026-11-08): one of each pickup and the door wait. The map audit
// runs when COF_SERVED_MAPS (the served map build, claude-s25-maps-001) and
// COF_ORIGINAL_MAPS (the game's original maps) are set; the rest is static.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
import {mapBase,main as audit} from '../scripts/audit-v0103.mjs';
import {V09_DEFAULTS} from '../scripts/package-state-rc4.mjs';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');
// The adapter's table: {"apartment1",967},...
const firstCopies=()=>Object.fromEntries([...read('native/rc4/coop-runtime.hpp').match(/const FirstCopy FirstCopies\[\]=\{([\s\S]*?)\};/)[1].matchAll(/\{"([a-z0-9_]+)",(\d+)\}/g)].map(m=>[m[1],Number(m[2])]));

test('map names reduce to the base the adapter looks up (coop-runtime.hpp map_base)',()=>{
 assert.equal(mapBase('s4_apartment1_e3'),'apartment1');assert.equal(mapBase('s4_start'),'start');assert.equal(mapBase('s4_start2'),'start2');
 assert.equal(mapBase('s4_ending4b'),'ending4b');assert.equal(mapBase('cof_campaign_story4'),'cof_campaign_story4');
});

test('the door wait is a host option, on by default, in the Create Server window and the server cfg',()=>{
 const setup=read('scripts/menu/sc_setup.scr');assert.match(setup,/"sc_doorwait"\s*\{\s*"Doors that change the map wait for everyone"\s*\{ BOOL \}\s*\{ "1" \}\s*\}/);
 assert.match(read('scripts/menu/storycoop_server.cfg'),/\n\/\/ sc_doorwait 1\r?\n/);
 assert.match(read('native/rc4/qol2-runtime.hpp'),/cvar_t doorwaitCvar\{const_cast<char\*>\("sc_doorwait"\),const_cast<char\*>\("1"\),0,1\.f,nullptr\}/);
 // An older Create Server file is replaced once so the option shows up, on Windows and on Linux.
 assert.match(read('scripts/install-story-package.ps1'),/-match 'sc_doorwait'/);assert.match(read('linux/coop-linux.py'),/'sc_doorwait' in text/);
 // The v0.10.2 server cfg (LF as shipped, and CRLF) is an untouched default that gets the new line.
 assert.ok(V09_DEFAULTS['storycoop_server.cfg'].includes('115a40b5513abd457ce4f40c3e31d412f00dabf600dfe18cd619630c7d203bc8'));
});

test('no per-player pickup copies are left in the adapter',()=>{
 const src=['coop-runtime.hpp','qol2-runtime.hpp','adapter.cpp','state-runtime.hpp'].map(f=>read('native/rc4/'+f)).join('\n');
 for(const gone of ['item_copies_frame','pickup_hidden','pickup_group','pickup_unblocking','per_player_pickup','pickupCopies'])assert.ok(!src.includes(gone),gone);
 assert.ok(src.includes('pickup_extras_frame();team_cards_frame();'));
});

test('the served maps: every appended pickup is a copy the adapter removes, and every door that changes the map is covered',{skip:!(process.env.COF_SERVED_MAPS&&process.env.COF_ORIGINAL_MAPS)},()=>{
 const r=audit([process.env.COF_SERVED_MAPS,process.env.COF_ORIGINAL_MAPS]);
 assert.deepEqual(r.problems,[]);
 assert.deepEqual(firstCopies(),r.firstCopy);
 assert.equal(r.totals.maps,194);assert.equal(r.totals.copies,1288);
 assert.equal(r.totals.nativeDoors,298);assert.equal(r.totals.plainDoors,203);
 assert.ok(r.maps.s4_village3_e0.plainDoors.some(d=>d.target==='endingcheck'));
});
