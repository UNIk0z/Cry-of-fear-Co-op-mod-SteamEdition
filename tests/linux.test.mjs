// Linux package (Katz 2026-11-01): coop.sh + coop-linux.py, packed beside the Windows ZIP.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {spawnSync} from 'node:child_process';
import {linuxReadMe,LINUX_FOLDER} from '../scripts/linux-package.mjs';
test('Linux READ ME names the version, Proton, the commands and the Discord',()=>{
 const t=linuxReadMe('v0.8');
 assert.ok(t.includes('(v0.8)')&&t.includes('Proton')&&t.includes('bash coop.sh')&&t.includes('https://discord.gg/hgEGSf9MFZ')&&!t.includes('\r'));
 assert.equal(LINUX_FOLDER,'Cry of Fear Co-op Linux');
});
test('Linux scripts parse',{skip:process.platform==='win32'&&'needs bash and python3'},()=>{
 assert.equal(spawnSync('bash',['-n','linux/coop.sh']).status,0);
 const py=spawnSync('python3',['-c','import ast,sys;ast.parse(open(sys.argv[1]).read())','linux/coop-linux.py']);
 assert.equal(py.status,0,String(py.stderr));
 assert.ok(fs.readFileSync('linux/coop-linux.py','utf8').includes("NATIVE_HL_SHA = '0036b91c01e92ed205513f52563053a55a66a32d257efb5f476b8f1f3dde0e63'"));
});
