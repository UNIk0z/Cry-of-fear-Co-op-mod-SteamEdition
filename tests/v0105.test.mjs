// v0.10.5 (Claude, 2026-11-09): hand models. Static checks; the menu itself is covered by
// native/rc4/test-session.cpp and the client bytes by native/rc4/test-gamebytes.cpp.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {fileURLToPath} from 'node:url';

const repo=fileURLToPath(new URL('..',import.meta.url));
const read=f=>fs.readFileSync(repo+f,'utf8');

test('the multiplayer sleeve call jumps to the clothes branch (one checked jump)',()=>{
 assert.ok(read('native/rc4/client-menu.hpp').includes('p.push_back({0x100668EE,{0x6a,0x00},{0xeb,0x27},false});'));
 assert.equal(0x10066917-(0x100668EE+2),0x27);
});

test('the Hands page has one picture per costume sleeve, made and packed under the installer\'s sc_mb names',()=>{
 const maker=read('scripts/menu/make-modelbox.py'),pack=read('scripts/package-state-rc4.mjs');
 for(const i of [0,1,2,3,5,6,9,10,11])assert.ok(maker.includes(`('sc_mbh${i}'`));
 assert.ok(pack.includes("[0,1,2,3,5,6,9,10,11].map(i=>'sc_mbh'+i)"));
 assert.match('gfx/vgui/sc_mbh11.tga',/^gfx\/vgui\/(sc_mb[a-z0-9]+)\.tga$/);   // install-story-package.ps1 delta rule
 assert.ok(read('native/rc4/client-menu.hpp').includes('"gfx/vgui/sc_mbh"+std::to_string(hands_state(look))+".tga"'));
});
