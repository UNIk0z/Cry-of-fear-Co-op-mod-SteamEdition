// Linux package (Katz, 2026-11-01: "always pack with linux"). The same
// package files as the Windows player folder (scripts/player-package.mjs),
// with linux/coop.sh and linux/coop-linux.py instead of the .exe and the
// PowerShell scripts. Cry of Fear runs through Steam Proton, so the co-op
// adapter is the same Windows DLL; coop-linux.py builds every file from the
// player's own game files and checks it against release.json, like Windows.
//   <OUT>/Cry of Fear Co-op Linux/  coop.sh, READ ME.txt, playermodels/ and files/
// Packed as a .tar.gz, never a .zip: launchers before v0.8.1 take the first .zip
// of a release as the Windows update (Katz 2026-11-01).
// Usage: linux-package.mjs CANDIDATE_PACKAGE NEW_OUTPUT
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from './bsp.mjs';
import {playerFiles} from './player-package.mjs';

export const LINUX_FOLDER='Cry of Fear Co-op Linux';
const linuxDir=path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))),'linux');
export function linuxReadMe(version){
 return [
  `CRY OF FEAR CO-OP FOR LINUX - Simon's story for 1 to 4 players (${version})`,
  `Co-op mod ${version} developed by Anas Sammak (koolkat).`,
  'Special thanks: Mesilll, Michal Eminov, caixa, Joah230.',
  '',
  'Cry of Fear runs on Linux through Steam Play (Proton). This package installs',
  'the same co-op mod as the Windows version, so Linux and Windows players can',
  'play together. Everyone needs the same version.',
  '',
  'NEEDED',
  '  Cry of Fear from Steam, started once through Proton (Steam > Cry of Fear >',
  '  Properties > Compatibility > use Steam Play), and Python 3.',
  '',
  'INSTALL (game closed)',
  '  Open a terminal in this folder and run:  bash coop.sh',
  '  Pick 1 (Install or update). It finds the game through Steam, sets aside any',
  '  older version of this mod and builds the co-op files from your own game',
  '  files. Your saves stay. If the game is somewhere Steam does not list, run:',
  '    bash coop.sh install --game "/path/to/steamapps/common/Cry of Fear/cryoffear"',
  '',
  'PLAYING',
  '  Run  bash coop.sh  and pick 2 (Run Co-op Mod), or run  bash coop.sh run.',
  '  Steam starts Cry of Fear with the co-op menu. If the game\'s own launcher',
  '  window opens, click LAUNCH. Then "Host Simon\'s Story" or "Join Simon\'s',
  '  Story" work as on Windows: the host picks a chapter or save and the number',
  '  of players; a friend types the host\'s IP address and port (empty = 27015).',
  '  Or open the console with the ~ key and type: connect 192.168.1.1:27015',
  '',
  'PLAYER MODEL',
  '  "Player Model" in the co-op menu picks your model and, after Done, your',
  '  hands. Your own .mdl files go in the "playermodels" folder here; they are',
  '  added the next time you run the mod.',
  '',
  'SAVING, SKIPPING, CO-OP KEYS',
  '  Only the host saves (tape recorder). Press R in a cutscene to vote to skip.',
  '  K: unstuck (out of a wall, or next to your partner). H: HUD on/off.',
  '  T: spray your logo. Make a picture your spray:  bash coop.sh spray picture.png',
  '  (an 8-bit PNG or a 24/32-bit BMP). Keys, field of view (cl_fovmultiplier)',
  '  and head bob (cl_bob) are in the game folder: cryoffear/storycoop_comfort.cfg.',
  '  The host\'s settings (revive time, auto respawn) are in the Create Server',
  '  window and in cryoffear/storycoop_server.cfg.',
  '',
  'UNINSTALL',
  '  bash coop.sh uninstall  (game closed). Co-op saves are kept. Files are',
  '  moved to "Cry of Fear - removed co-op files" next to the game folder.',
  '',
  'THIS MOD IS UNFINISHED, AND THE LINUX VERSION IS NEW',
  '  Expect bugs. Please report them, with your log, in our Discord:',
  '  https://discord.gg/hgEGSf9MFZ',
  '  Your log is in the game folder: cryoffear/storycoop_logs, the newest',
  '  rc4-<date>-<time>.log. Please also paste what coop.sh printed.',
  '',
 ].join('\n');
}
export function main(args){
 if(args.length!==2)throw Error('Usage: linux-package.mjs CANDIDATE_PACKAGE NEW_OUTPUT');
 const [candidate,destination]=args,{catalog,files}=playerFiles(candidate);
 if(fs.existsSync(destination))throw Error('Output already exists: '+destination);
 const root=path.join(destination,LINUX_FOLDER),data=path.join(root,'files');fs.mkdirSync(data,{recursive:true});
 // The PowerShell scripts are Windows-only; the Linux script replaces them.
 for(const [rel,pin] of files){
  if(rel.endsWith('.ps1'))continue;
  const bytes=fs.readFileSync(path.join(candidate,rel));if(pin&&sha256(bytes)!==pin)throw Error('Changed package file: '+rel);
  fs.mkdirSync(path.dirname(path.join(data,rel)),{recursive:true});fs.writeFileSync(path.join(data,rel),bytes,{flag:'wx'});
 }
 // LF only: a Windows checkout (core.autocrlf) has CRLF, and bash stops at the first $'\r'.
 const lf=name=>fs.readFileSync(path.join(linuxDir,name),'utf8').replace(/\r\n/g,'\n');
 fs.writeFileSync(path.join(data,'coop-linux.py'),lf('coop-linux.py'),{flag:'wx'});
 fs.writeFileSync(path.join(root,'coop.sh'),lf('coop.sh'),{flag:'wx'});fs.chmodSync(path.join(root,'coop.sh'),0o755);
 fs.writeFileSync(path.join(root,'READ ME.txt'),linuxReadMe(catalog.coopFixes.id),{flag:'wx'});
 fs.mkdirSync(path.join(root,'playermodels'));fs.writeFileSync(path.join(root,'playermodels','PUT PLAYER MODELS HERE.txt'),'Put your own player models (.mdl files made for the Cry of Fear player skeleton) in this folder,\nthen run the mod (bash coop.sh run). Each model is copied into the game once.\n',{flag:'wx'});
 const summary={root,version:catalog.coopFixes.id,files:files.filter(([r])=>!r.endsWith('.ps1')).length};
 console.log(JSON.stringify(summary));return summary;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){try{main(process.argv.slice(2));}catch(e){console.error(e.stack);process.exitCode=1;}}
