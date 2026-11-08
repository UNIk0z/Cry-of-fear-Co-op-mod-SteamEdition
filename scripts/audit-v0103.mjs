// v0.10.3 map audit (Claude, 2026-11-08). Reads the served co-op maps (maps are
// unchanged since the claude-s25 build) and the original single-player maps, and
// records what the v0.10.3 adapter relies on:
// 1. Pickup copies. The connected-campaign build gave every plain ammo/weapon
//    pickup up to three copies, appended after the original map's entities
//    (build-v05-maps.mjs ammoSquares put them in a 2x2 square). The adapter
//    removes them when a story map starts (coop-runtime.hpp FirstCopies): every
//    served entity from the original map's entity count on that is an ammo_ or
//    weapon_ pickup with the keys of an earlier pickup is a copy.
// 2. Doors that change the map. A native co-op door (inter_door_coop_changelevel)
//    waits for its trigger_coop_changelevel zone. A plain inter_door whose exit
//    lies in a trigger that reaches a level change (relays, counters, managers
//    and trigger_command followed, as coop-runtime.hpp exit_reaches) moves its
//    user into that trigger; the adapter makes those wait too (qol2-runtime.hpp).
// Usage: audit-v0103.mjs SERVED_MAPS_DIR ORIGINAL_MAPS_DIR [OUTPUT_JSON]
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {decodeBsp} from './bsp.mjs';

const v=(e,k)=>e.pairs.find(p=>p[0]===k)?.[1];
const cls=e=>v(e,'classname')??'';
const xyz=s=>{const p=(s??'').trim().split(/\s+/).map(Number);return p.length===3&&p.every(Number.isFinite)?p:null;};
// coop-runtime.hpp map_base: s4_apartment1_e3 -> apartment1.
export const mapBase=m=>(m.startsWith('s4_')?m.slice(3):m).replace(/_e\d+$/,'');
const pickupKey=e=>JSON.stringify(e.pairs.filter(([k])=>k!=='origin'&&k!=='classname'));
const PICKUP=/^(ammo_|weapon_)/;

function models(bytes,bsp){
 const l=bsp.lumps[14],out=[];
 for(let i=0;i<Math.floor(l.length/64);i++){const at=l.offset+i*64;out.push([0,4,8,12,16,20].map(o=>bytes.readFloatLE(at+o)));}
 return out;
}
function bounds(e,brushes){
 const m=v(e,'model');if(!/^\*\d+$/.test(m??''))return null;const b=brushes[Number(m.slice(1))];if(!b)return null;
 const o=xyz(v(e,'origin'))??[0,0,0];return [b[0]+o[0],b[1]+o[1],b[2]+o[2],b[3]+o[0],b[4]+o[1],b[5]+o[2]];
}
const STANDARD=new Set(['origin','angles','classname','targetname','spawnflags','wait','maxwait','mode','triggerstate','m_iszThreadName','m_iszLocusThread']);
function reaches(entities,name,depth=0,seen=new Set()){
 if(depth>6||!name||seen.has(name))return false;seen.add(name);
 for(const e of entities.filter(x=>v(x,'targetname')===name)){
  const c=cls(e);
  if(c==='cof_changelevel'||c==='trigger_changelevel')return true;
  if(c==='trigger_command'&&/changelevel/.test(v(e,'netname')??''))return true;
  if((c==='trigger_relay'||c==='game_counter')&&reaches(entities,v(e,'target'),depth+1,seen))return true;
  if(c==='multi_manager')for(const [k] of e.pairs)if(!STANDARD.has(k)&&reaches(entities,k.replace(/#.*$/,''),depth+1,seen))return true;
 }
 return false;
}
// A player standing at origin p (hull 32x32x72) touches the volume (8 units of slack, coop-runtime.hpp outside_exits).
const touches=(p,b)=>[0,1,2].every(k=>p[k]+(k===2?36:16)+8>=b[k]&&p[k]-(k===2?36:16)-8<=b[3+k]);

export function auditServedMap(bytes,originalBytes){
 const bsp=decodeBsp(bytes),entities=bsp.entities,brushes=models(bytes,bsp);
 const original=decodeBsp(originalBytes).entities,firstCopy=original.length;
 // 1. Pickups.
 const originals=new Set(entities.slice(0,firstCopy).filter(e=>PICKUP.test(cls(e))).map(e=>cls(e)+pickupKey(e)));
 const appended=entities.slice(firstCopy).filter(e=>/^(ammo_|weapon_|item_)/.test(cls(e)));
 const copies=appended.filter(e=>PICKUP.test(cls(e))&&originals.has(cls(e)+pickupKey(e)));
 const problems=[];
 if(copies.length!==appended.length)problems.push(`${appended.length-copies.length} appended pickup(s) are not copies`);
 for(let i=0;i<firstCopy;i++)if(PICKUP.test(cls(entities[i]))&&cls(entities[i])!==cls(original[i]))problems.push(`entity ${i} became a pickup`);
 // 2. Doors that change the map.
 const exitVolumes=entities.filter(e=>(cls(e)==='trigger_once'||cls(e)==='trigger_multiple')&&reaches(entities,v(e,'target'))||cls(e)==='trigger_changelevel').map(e=>bounds(e,brushes)).filter(Boolean);
 const nativeDoors=[],plainDoors=[];
 for(const e of entities){
  if(cls(e)==='inter_door_coop_changelevel'){
   const zone=entities.find(x=>v(x,'targetname')===v(e,'target'));
   nativeDoors.push({entity:e.index,next:v(e,'message'),zone:zone&&cls(zone)==='trigger_coop_changelevel'&&!!bounds(zone,brushes)});
  }
  if(cls(e)!=='inter_door')continue;
  const target=v(e,'target');if(!target)continue;
  const names=target==='endingcheck'?[1,2,3,4,5].map(n=>'ending_exit'+n):[target];
  const out=names.some(n=>{const x=entities.find(y=>v(y,'targetname')===n);const o=x&&xyz(v(x,'origin'));return o&&exitVolumes.some(b=>touches([o[0],o[1],o[2]+37],b));});
  if(out)plainDoors.push({entity:e.index,target,name:v(e,'targetname')??null,lockedby:v(e,'lockedby')??null,spawnflags:Number(v(e,'spawnflags')??0)});
 }
 return {firstCopy,copies:copies.length,problems,nativeDoors,plainDoors};
}

export function main(args){
 if(args.length<2||args.length>3)throw Error('Usage: audit-v0103.mjs SERVED_MAPS_DIR ORIGINAL_MAPS_DIR [OUTPUT_JSON]');
 const [served,originals,output]=args,report={maps:{},firstCopy:{},problems:[]};
 for(const file of fs.readdirSync(served).filter(f=>f.endsWith('.bsp')).sort()){
  const map=file.slice(0,-4),info=JSON.parse(fs.readFileSync(path.join(served,map+'.json'),'utf8'));
  const r=auditServedMap(fs.readFileSync(path.join(served,file)),fs.readFileSync(path.join(originals,info.sourceMap+'.bsp')));
  report.maps[map]={source:info.sourceMap,...r};const base=mapBase(map);
  if(r.copies){if(report.firstCopy[base]!==undefined&&report.firstCopy[base]!==r.firstCopy)report.problems.push(`${base}: first copy ${report.firstCopy[base]} and ${r.firstCopy}`);report.firstCopy[base]=r.firstCopy;}
  for(const p of r.problems)report.problems.push(`${map}: ${p}`);
  for(const d of r.nativeDoors)if(!d.zone)report.problems.push(`${map}: co-op door ${d.entity} has no zone`);
 }
 const maps=Object.values(report.maps);
 report.totals={maps:maps.length,copies:maps.reduce((s,m)=>s+m.copies,0),nativeDoors:maps.reduce((s,m)=>s+m.nativeDoors.length,0),plainDoors:maps.reduce((s,m)=>s+m.plainDoors.length,0)};
 if(output)fs.writeFileSync(output,JSON.stringify(report,null,1));
 return report;
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const r=main(process.argv.slice(2));
 console.log(JSON.stringify({totals:r.totals,problems:r.problems,firstCopy:r.firstCopy}));
 if(r.problems.length)process.exitCode=1;
}
