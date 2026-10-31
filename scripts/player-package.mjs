// coop-fixes-012 (user request): the folder players get. The full candidate
// package (rollbacks, source, reports, QA) stays the developer's; this copies
// only what installing, hosting, joining and uninstalling need into
//   <OUT>/Cry of Fear Co-op/  Cry of Fear Co-op.exe, READ ME.txt, playermodels/ and files/
// and checks every copied file against the candidate's release.json.
// Usage: player-package.mjs CANDIDATE_PACKAGE NEW_OUTPUT
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {sha256} from './bsp.mjs';

export const PLAYER_FOLDER='Cry of Fear Co-op';
// Launcher (Katz, 2026-10-30): one themed .exe, built by launcher/build-launcher.ps1.
// v0.7.2 (Katz: "we dont need cmd files anymore"): it is the only way in, so a
// package without it is refused.
export const LAUNCHER_EXE='Cry of Fear Co-op.exe';
const launcherBuild=path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))),'launcher','out',LAUNCHER_EXE);
export const PLAYER_SCRIPTS=['setup-coop.ps1','install-story-package.ps1','host-story-coop.ps1','join-coop.ps1','find-cof.ps1','update-coop.ps1'];
export function readMe(version){
 return [
  `CRY OF FEAR CO-OP - Simon's story for 1 to 4 players (${version})`,
  `Co-op mod ${version} developed by Anas Sammak (koolkat).`,
  'Special thanks: Mesilll, Michal Eminov, caixa, Joah230.',
  '',
  'Everything starts from "Cry of Fear Co-op.exe" in this folder. Its buttons are',
  'Install, Run Co-op Mod, Check for Updates (it also installs a newer version)',
  'and Uninstall. Hosting and joining are in the game\'s co-op menu. The program is',
  'not signed, so Windows may warn about it: click More info, then Run anyway.',
  '',
  'BOTH PLAYERS',
  '  1. Close Cry of Fear.',
  '  2. Open "Cry of Fear Co-op.exe" and click Install. It finds the game through Steam, sets aside any',
  '     older version of this mod and installs this one. Your saves stay.',
  '     Check for Updates tells you when a newer version is out and installs it.',
  '     If you use the RebuiltSimon add-on (cryoffear\\addons), move that',
  '     folder out while playing co-op: it crashed the game in our tests.',
  '',
  'PLAYING',
  '  3. Both players click Run Co-op Mod. Cry of Fear opens with',
  '     the co-op main menu (Steam may ask once to allow the launch options).',
  '  4. The HOST clicks "Host Simon\'s Story". The game\'s Create Server window',
  '     opens. In its list pick where to start (Chapter-0-Prologue up to',
  '     Chapter-7-Only-Safe-at-Home) or a save (Load-Save-Slot-1 to 5).',
  '     Set the number of players (2 to 4) and, on the second page, the',
  '     difficulty and Player collision (on: you bump into each other; off:',
  '     you walk through each other) and Unlock extras (the Chapter 1 secret',
  '     room with every unlockable). Click Start. A save keeps its own',
  '     players, collision and difficulty. You can also play alone.',
  '     Give your friends your address (for example your Radmin VPN address).',
  '  5. A FRIEND clicks "Join Simon\'s Story". A box opens: type the host\'s',
  '     IP address, then the port (leave it empty for 27015), and click Join.',
  '     Or open the game console with the ~ key and type connect, the IP and',
  '     :27015, for example:',
  '        connect 192.168.1.1:27015',
  '',
  'PLAYER MODEL',
  '  Click "Player Model" in the co-op menu: Simon, his outfits and any models you',
  '  added, with a preview. Next shows the next one, Done keeps the one shown, and',
  '  your choice is kept for the next start. To add your own model, put its .mdl',
  '  file (made for the Cry of Fear player skeleton) in the',
  '  "playermodels" folder next to "Cry of Fear Co-op.exe" and start the mod again.',
  '  The others see your model when the host has it too (the host can put the',
  '  same file in their playermodels folder; joining players download it).',
  '',
  'Run Co-op Mod uses your Steam name as your player name. Starting Cry of Fear',
  'the normal way shows the game\'s own menu; only Run Co-op Mod starts it with',
  'the co-op menu.',
  '',
  'SAVING',
  '  Only the host can save. The host uses a tape recorder: the game\'s save',
  '  window opens; double-click one of the five slots. Everyone hears the',
  '  recorder when the save is done. If the friend uses a recorder, they see',
  '  "Only host can save the game". Saves are kept on the host\'s PC. On harder',
  '  difficulties the game itself needs a cassette tape to save.',
  '  Maps you go back to stay as you left them (taken items, dead enemies,',
  '  opened doors), and a save keeps that too.',
  '',
  'SKIPPING A CUTSCENE',
  '  During a cutscene press R to vote to skip ("Press R to skip 1/2" shows',
  '  bottom-left). When every player has pressed it, the scene',
  '  jumps to its end: everything it still does (teleports, items, music)',
  '  still happens. The opening (prologue) can be skipped the same way.',
  '  Only scenes with a yes/no choice still to come, and the stats screen at the',
  '  end of the game, cannot be skipped.',
  '',
  'STUCK?',
  '  Press Y (chat) and type: stuck',
  '  You are moved next to the other player.',
  '',
  'UNINSTALL',
  '  Close the game and click Uninstall. Co-op saves are kept. Files are',
  '  moved to "Cry of Fear - removed co-op files" next to the game folder,',
  '  never deleted. The game\'s own main menu and title come back.',
  '',
  'THIS MOD IS UNFINISHED',
  '  Expect bugs. Please report them, with your log, in our Discord:',
  '  https://discord.gg/hgEGSf9MFZ',
  '',
  'HOW TO FIND YOUR LOG',
  '  1. Open the game folder: in Steam, right-click Cry of Fear > Manage >',
  '     Browse local files.',
  '  2. Open the folder cryoffear\\storycoop_logs.',
  '  3. Take the NEWEST rc4-<date>-<time>.log (sort by "Date modified"). The',
  '     host\'s log matters most; send the friend\'s too if you can.',
  '  4. Post it in the Discord with the map or place, what you did and what',
  '     went wrong.',
  '',
 ].join('\r\n');
}
// Every file the four commands use, with the SHA256 release.json pins (null:
// a script, which the candidate package itself carries).
export function playerFiles(candidate){
 const catalog=JSON.parse(fs.readFileSync(path.join(candidate,'release.json'),'utf8').replace(/^﻿/,''));
 if(!catalog.coopFixes?.id)throw Error('Not a co-op candidate package');
 const files=[['release.json',null],['known-builds.json',null],...PLAYER_SCRIPTS.map(f=>[f,null]),[path.join('native',catalog.runtimeAdapter.file),catalog.runtimeAdapter.sha256]];
 for(const m of catalog.maps)files.push([path.join('plans',m.map+'.json'),m.planSha256]);
 for(const x of catalog.extras??[])if(x.file)files.push([x.file,x.replaces||x.deltaFrom?x.deltaSha256:x.sha256]);
 return {catalog,files};
}
export function main(args){
 if(args.length!==2)throw Error('Usage: player-package.mjs CANDIDATE_PACKAGE NEW_OUTPUT');
 const [candidate,destination]=args,{catalog,files}=playerFiles(candidate);
 if(fs.existsSync(destination))throw Error('Output already exists: '+destination);
 if(!fs.existsSync(launcherBuild))throw Error('Build the launcher first: launcher/build-launcher.ps1 -Version x.y.z');
 const root=path.join(destination,PLAYER_FOLDER),data=path.join(root,'files');fs.mkdirSync(data,{recursive:true});
 for(const [rel,pin] of files){
  const bytes=fs.readFileSync(path.join(candidate,rel));if(pin&&sha256(bytes)!==pin)throw Error('Changed package file: '+rel);
  fs.mkdirSync(path.dirname(path.join(data,rel)),{recursive:true});fs.writeFileSync(path.join(data,rel),bytes,{flag:'wx'});
 }
 fs.copyFileSync(launcherBuild,path.join(root,LAUNCHER_EXE),fs.constants.COPYFILE_EXCL);
 fs.writeFileSync(path.join(root,'READ ME.txt'),readMe(catalog.coopFixes.id),{flag:'wx'});
 // v0.7: custom player models go here (host-story-coop.ps1 copies them into the game).
 fs.mkdirSync(path.join(root,'playermodels'));fs.writeFileSync(path.join(root,'playermodels','PUT PLAYER MODELS HERE.txt'),'Put your own player models (.mdl files made for the Cry of Fear player skeleton) in this folder,\r\nthen click Run Co-op Mod. Each model shows up in the Player Model box of the co-op menu.\r\nUninstall leaves this folder alone.\r\n',{flag:'wx'});
 const summary={root,version:catalog.coopFixes.id,files:files.length,catalogSha256:sha256(fs.readFileSync(path.join(candidate,'release.json')))};
 console.log(JSON.stringify(summary));return summary;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){try{main(process.argv.slice(2));}catch(e){console.error(e.stack);process.exitCode=1;}}
