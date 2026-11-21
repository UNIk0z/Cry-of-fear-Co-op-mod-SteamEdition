// v0.10.8 (Claude, 2026-11-21): Katz's requests (docs/V0108_RELEASE.md). The adapter changes are covered by the native
// fixtures (test-qol, test-qol2, test-session, test-mapstate, test-story, test-ladder, test-world, test-checkpoint-runtime);
// these are the package-side ones: default keys, the settings files replaced once, the READ ME and the launchers.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {readMe} from '../scripts/player-package.mjs';
import {V09_DEFAULTS} from '../scripts/package-state-rc4.mjs';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');
const sha=(s)=>crypto.createHash('sha256').update(s).digest('hex');
const ascii=s=>/^[\x09\x0a\x0d\x20-\x7e]*$/.test(s);

test('keys: picture mode on ], ping on H, no scare (comfort file, launcher, READ ME)',()=>{
 const comfort=read('scripts/menu/storycoop_comfort.cfg');
 assert.ok(comfort.includes('bind "]" "sc_hud"')&&comfort.includes('bind "h" "sc_ping"')&&!comfort.includes('sc_scare')&&!comfort.includes('bind "g"'));
 const launcher=read('launcher/CoopExtras.cs');
 assert.ok(launcher.includes('public string Unstuck = "k", Hud = "]", Spray = "t", Ping = "h", Drop = "q";')&&!launcher.includes('sc_scare')&&!launcher.includes('Scare'));
 assert.ok(launcher.includes('if (s.Hud == "h" && s.Ping == "g") { s.Hud = "]"; s.Ping = "h"; }'));   // a file with the old defaults
 assert.ok(launcher.includes('new Regex("^([a-z0-9]|\\\\[|\\\\]|f('));                                   // [ and ] are key names
 const keys=readMe('v0.10.8').split(/\r?\n/).filter(l=>/^  [\]HKTQJG] /.test(l));
 assert.ok(keys.some(l=>l.startsWith('  ]  Picture mode'))&&keys.some(l=>l.startsWith('  H  Ping'))&&!keys.some(l=>/^  [JG] /.test(l)));
 assert.ok(!read('scripts/linux-package.mjs').includes('scare'));
});

test('settings files: the v0.10.4-v0.10.7 comfort file and the v0.10 server file are replaced once when unchanged',()=>{
 const G=V09_DEFAULTS['storycoop_comfort.cfg'],S=V09_DEFAULTS['storycoop_server.cfg'];
 for(const h of ['3f718e0a40821d37b93ae7fa4f9beb1a0dbf24c1d1aeca33f54e50707f2c7b70','461c1d1e43cb439d9bdb8c6697bf57f8b5928721f2380534c94d5edb142af198'])assert.ok(G.includes(h),h);
 for(const h of ['d1dc2465be3785b19e7ec1a63cd38e50f013bc506fe5725a62ebe2f8d9b890c2','8c36315d32ef2dec7f0ee1d1563384f12655907cbcd615e56baeab2c9a13afab'])assert.ok(S.includes(h),h);
 for(const f of ['storycoop_comfort.cfg','storycoop_server.cfg']){const now=read('scripts/menu/'+f);assert.ok(!V09_DEFAULTS[f].includes(sha(now))&&!V09_DEFAULTS[f].includes(sha(now.replace(/\r\n/g,'\n'))),f+' is not its own old default');}
 const server=read('scripts/menu/storycoop_server.cfg');
 assert.ok(server.includes('a respawn keeps his items')&&!server.includes('loses the items'));
});

test('player model: no "+setinfo sc_model" at launch (it filled the userinfo and the model was lost)',()=>{
 assert.ok(!/setinfo['", ]+sc_model['", ]+\$chosen/.test(read('scripts/host-story-coop.ps1')));
 assert.ok(!read('linux/coop-linux.py').includes("'sc_model', chosen"));
 assert.ok(read('native/rc4/client-menu.hpp').includes('("setinfo sc_model \\"\\"\\n")'));
 for(const f of ['scripts/host-story-coop.ps1','scripts/menu/storycoop_server.cfg'])assert.ok(ascii(read(f)),f);
});
