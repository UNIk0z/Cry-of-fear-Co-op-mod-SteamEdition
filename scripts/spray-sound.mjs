// Original spray-can hiss; generated locally instead of distributing another game's sound.
export function spraySound() {
  const rate = 22050, count = Math.round(rate * 0.55);
  const wav = Buffer.alloc(44 + count * 2);
  wav.write('RIFF'); wav.writeUInt32LE(wav.length - 8, 4); wav.write('WAVEfmt ', 8);
  wav.writeUInt32LE(16, 16); wav.writeUInt16LE(1, 20); wav.writeUInt16LE(1, 22);
  wav.writeUInt32LE(rate, 24); wav.writeUInt32LE(rate * 2, 28); wav.writeUInt16LE(2, 32);
  wav.writeUInt16LE(16, 34); wav.write('data', 36); wav.writeUInt32LE(count * 2, 40);
  let seed = 0x201, low = 0;
  for (let i = 0; i < count; i++) {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    const noise = seed / 0x80000000 - 1;
    low += 0.16 * (noise - low);
    const envelope = Math.min(1, i / (rate * 0.025), (count - i) / (rate * 0.12));
    wav.writeInt16LE(Math.round((noise - low) * envelope * 10500), 44 + i * 2);
  }
  return wav;
}
