import test from 'node:test';
import assert from 'node:assert/strict';
import {OUTFITS,buildOutfitModel} from '../scripts/simon-model.mjs';

// A model with one texture of 2 x 2 pixels, as Simon's after its renames.
function fixture(textureName){
 const b=Buffer.alloc(400+4+768);b.write('IDST',0,'latin1');b.writeInt32LE(1,180);b.writeInt32LE(200,184);
 b.write(textureName,200,'latin1');b.writeInt32LE(2,200+68);b.writeInt32LE(2,200+72);b.writeInt32LE(400,200+76);
 return b;
}
// Top-down rows in BGR.
function tga(rows,top=true){
 const pixels=rows.flat(),b=Buffer.alloc(18+pixels.length*3);b[2]=2;b.writeUInt16LE(2,12);b.writeUInt16LE(rows.length,14);b[16]=24;b[17]=top?0x20:0;
 pixels.forEach(([r,g,bl],i)=>{b[18+i*3]=bl;b[19+i*3]=g;b[20+i*3]=r;});return b;
}

test('v0.7 outfits: the client clothes order, safe names',()=>{
 assert.equal(OUTFITS.length,11);
 for(const [name,folder] of OUTFITS){assert.match(name,/^simon_[a-z]+$/);assert.match(folder,/^[A-Za-z_]+$/);}
 assert.deepEqual(OUTFITS.map(o=>o[1]).slice(0,4),['Leatherhoff','ModDB','Hello_Kitty','AoM']);
 assert.equal(OUTFITS[10][1],'Twitcher');
});

test('v0.7 outfit model: costume pixels and palette in place, the model name set, the base untouched',()=>{
 const base=fixture('hoodie_coop.bmp'),red=[250,10,10],blue=[10,10,250];
 const out=buildOutfitModel(base,'simon_test',[{part:'hoodie',bytes:tga([[red,blue],[blue,red]])}]);
 assert.equal(out.toString('latin1',8,8+14),'simon_test.mdl');assert.equal(base[400],0);assert.equal(out.length,base.length);
 const at=i=>[out[404+out[400+i]*3],out[405+out[400+i]*3],out[406+out[400+i]*3]];
 assert.deepEqual(at(0),red);assert.deepEqual(at(1),blue);assert.deepEqual(at(2),blue);assert.deepEqual(at(3),red);
 // A bottom-up file gives the same rows.
 const flipped=buildOutfitModel(base,'simon_test',[{part:'hoodie',bytes:tga([[blue,red],[red,blue]],false)}]);assert.deepEqual(flipped,out);
});

test('v0.7 outfit model: a wrong size or a missing target is refused',()=>{
 const base=fixture('hoodie_coop.bmp'),px=[1,2,3];
 assert.throws(()=>buildOutfitModel(base,'x',[{part:'hoodie',bytes:tga([[px]])}]));
 assert.throws(()=>buildOutfitModel(base,'x',[{part:'jeans',bytes:tga([[px,px],[px,px]])}]));
});
