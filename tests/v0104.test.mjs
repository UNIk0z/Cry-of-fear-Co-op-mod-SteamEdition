// v0.10.4 (Claude, 2026-11-09): drop ammo for a teammate (sc_dropammo, Q). Static checks of the key wiring;
// the command itself is covered by native/rc4/test-qol2.cpp and test-qol.cpp.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';
import {V09_DEFAULTS} from '../scripts/package-state-rc4.mjs';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');

test('Q drops ammo in the installed comfort file, in SETTINGS and in the adapter',()=>{
 assert.match(read('scripts/menu/storycoop_comfort.cfg'),/\nbind "q" "sc_dropammo"\r?\n/);
 const launcher=read('launcher/CoopExtras.cs');
 assert.ok(launcher.includes('Drop = "q"')&&launcher.includes('"Drop ammo key"')&&launcher.includes('"sc_dropammo" }'));
 assert.ok(read('native/rc4/qol2-runtime.hpp').includes('if(!std::strcmp(command,"sc_dropammo"))return drop_ammo_command(e);'));
});

test('an untouched v0.10-v0.10.3 comfort file is replaced once, so it gets the new key',()=>{
 for(const sha of ['be1c7316e1863f5252b87d09ee0542dbc4c8aa1ed33bec070ed99761ec0ba923','cf894686a853e27f86d1de229f7144631e3b424db8d1cb790bd8192f484bb80e'])
  assert.ok(V09_DEFAULTS['storycoop_comfort.cfg'].includes(sha));
});
