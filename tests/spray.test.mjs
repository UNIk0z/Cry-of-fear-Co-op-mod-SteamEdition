import test from 'node:test';
import assert from 'node:assert/strict';
import {spraySound} from '../scripts/spray-sound.mjs';

test('original spray hiss is valid deterministic mono PCM with audible samples and faded edges', () => {
  const wav = spraySound();
  assert.deepEqual(wav, spraySound());
  assert.equal(wav.toString('ascii',0,4),'RIFF');
  assert.equal(wav.toString('ascii',8,16),'WAVEfmt ');
  assert.equal(wav.readUInt32LE(4),wav.length-8);
  assert.equal(wav.readUInt16LE(20),1);
  assert.equal(wav.readUInt16LE(22),1);
  assert.equal(wav.readUInt32LE(24),22050);
  assert.equal(wav.readUInt16LE(34),16);
  assert.equal(wav.readUInt32LE(40),wav.length-44);
  assert.equal(wav.readInt16LE(44),0);
  let square = 0;
  for(let i=44;i<wav.length;i+=2) square += wav.readInt16LE(i)**2;
  assert.ok(Math.sqrt(square/((wav.length-44)/2))>1000);
  assert.ok(Math.abs(wav.readInt16LE(wav.length-2))<10);
});
