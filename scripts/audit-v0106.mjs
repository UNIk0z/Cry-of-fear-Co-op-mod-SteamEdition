// v0.10.6 map audit (Claude, 2026-11-13). Reads the served co-op maps and lists the two patterns behind
// the s4_stad <-> s4_subway1st1 playtest report (docs/V0106_RELEASE.md):
// 1. Arrival rooms. Co-op spawns (cof_coop_spawn1..4) standing in or within 64 units of a trigger that
//    leads to another level (relays, counters, managers and trigger_command followed, as coop-runtime.hpp
//    exit_reaches), and spawns standing in a trigger_multiple whose cof_teleport carries the player into the
//    map (the transit room: with "wait" > 0 only the first player is carried; coop-runtime.hpp transit_touch
//    now carries everyone standing in it).
// 2. Movewith groups and trains. Every func_train / func_tracktrain and every movewith parent, with its
//    children and what fires it (one-shot: trigger_once, a func_button with wait -1). v0.3-v0.10.5 kept every
//    movewith group as served, so a save has no record of them (state-runtime.hpp state_group); a group
//    with a member that leads out is still kept as served whole.
// Usage: audit-v0106.mjs SERVED_MAPS_DIR [OUTPUT_JSON]
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {decodeBsp} from './bsp.mjs';

const v=(e,k)=>e.pairs.find(p=>p[0]===k)?.[1];
const cls=e=>v(e,'classname')??'';
const xyz=s=>{const p=(s??'').trim().split(/\s+/).map(Number);return p.length===3&&p.every(Number.isFinite)?p:null;};
const STANDARD=new Set(['origin','angles','classname','targetname','spawnflags','wait','maxwait','mode','triggerstate','m_iszThreadName','m_iszLocusThread']);
export function brushModels(bytes,bsp){
 const l=bsp.lumps[14],out=[];
 for(let i=0;i<Math.floor(l.length/64);i++){const at=l.offset+i*64;out.push([0,4,8,12,16,20].map(o=>bytes.readFloatLE(at+o)));}
 return out;
}
const bounds=(e,brushes)=>{const m=v(e,'model');if(!/^\*\d+$/.test(m??''))return null;const b=brushes[Number(m.slice(1))];if(!b)return null;const o=xyz(v(e,'origin'))??[0,0,0];return [b[0]+o[0],b[1]+o[1],b[2]+o[2],b[3]+o[0],b[4]+o[1],b[5]+o[2]];};
const named=(entities,name)=>entities.filter(x=>v(x,'targetname')===name);
const managerTargets=e=>e.pairs.filter(([k])=>!STANDARD.has(k)).map(([k])=>k.replace(/#.*$/,''));
function reachesExit(entities,name,depth=0,seen=new Set()){
 if(depth>6||!name||seen.has(name))return false;seen.add(name);
 for(const e of named(entities,name)){
  const c=cls(e);
  if(c==='cof_changelevel'||c==='trigger_changelevel')return true;
  if(c==='trigger_command'&&/changelevel/.test(v(e,'netname')??''))return true;
  if((c==='trigger_relay'||c==='game_counter')&&reachesExit(entities,v(e,'target'),depth+1,seen))return true;
  if(c==='multi_manager'&&managerTargets(e).some(t=>reachesExit(entities,t,depth+1,seen)))return true;
 }
 return false;
}
// Who fires NAME: direct targets, and through trigger_relay / multi_manager (up to 3 steps).
function firers(entities,name,depth=0,seen=new Set()){
 if(depth>3||seen.has(name))return [];seen.add(name);const out=[];
 for(const e of entities){
  const c=cls(e),direct=v(e,'target')===name||v(e,'TriggerTarget')===name||(c==='multi_manager'&&managerTargets(e).includes(name));
  if(!direct)continue;
  const label=c+(v(e,'model')?'('+v(e,'model')+')':'')+(v(e,'targetname')?' '+v(e,'targetname'):'');
  const oneShot=c==='trigger_once'||(c==='func_button'&&Number(v(e,'wait'))===-1)||(/^monster_/.test(c)&&v(e,'TriggerTarget')===name);
  if((c==='multi_manager'||c==='trigger_relay')&&v(e,'targetname')){const up=firers(entities,v(e,'targetname'),depth+1,seen);out.push({by:label,oneShot:up.some(u=>u.oneShot),via:up});}
  else out.push({by:label,oneShot});
 }
 return out;
}
const flat=list=>list.flatMap(f=>f.via?[f.by+' <- ['+flat(f.via).join(', ')+']']:[f.by+(f.oneShot?' (one-shot)':'')]);
// A player hull (32x32x72 around the origin) against a box, with SLACK units to spare.
const near=(p,b,slack)=>[0,1,2].every(k=>p[k]+(k===2?36:16)+slack>=b[k]&&p[k]-(k===2?36:16)-slack<=b[3+k]);
export function auditEntities(entities,brushes){
 const exits=entities.filter(e=>(cls(e)==='trigger_once'||cls(e)==='trigger_multiple')&&reachesExit(entities,v(e,'target'))||cls(e)==='trigger_changelevel')
  .map(e=>({e,b:bounds(e,brushes)})).filter(x=>x.b);
 const transits=entities.filter(e=>cls(e)==='trigger_multiple'&&!reachesExit(entities,v(e,'target'))&&named(entities,v(e,'target')).some(t=>cls(t)==='cof_teleport'&&!v(t,'message')))
  .map(e=>({e,b:bounds(e,brushes)})).filter(x=>x.b);
 const spawns=[];
 for(const s of entities.filter(e=>/^cof_coop_spawn\d$/.test(cls(e)))){
  const o=xyz(v(s,'origin'));if(!o)continue;
  const inExit=exits.filter(x=>near(o,x.b,8)).map(x=>v(x.e,'model')),nearExit=exits.filter(x=>!near(o,x.b,8)&&near(o,x.b,64)).map(x=>v(x.e,'model'));
  const inTransit=transits.filter(x=>near(o,x.b,0)).map(x=>v(x.e,'model')+' wait '+(v(x.e,'wait')??'0.2')+' -> '+v(x.e,'target'));
  if(inExit.length||nearExit.length||inTransit.length)spawns.push({spawn:cls(s),origin:v(s,'origin'),angles:v(s,'angles')??'0 0 0',inExit,nearExit,inTransit});
 }
 const parents=new Set(entities.map(e=>v(e,'movewith')).filter(Boolean));
 for(const t of entities)if(/^func_(train|tracktrain)$/.test(cls(t))&&v(t,'targetname'))parents.add(v(t,'targetname'));
 const groups=[];
 for(const name of parents){
  const self=named(entities,name),kids=entities.filter(e=>v(e,'movewith')===name);
  const members=[...self,...kids],leadsOut=members.some(m=>reachesExit(entities,v(m,'target')));
  const by=firers(entities,name);
  groups.push({name,classes:self.map(cls),children:kids.map(k=>cls(k)+(v(k,'model')?'('+v(k,'model')+')':'')+(v(k,'target')?'->'+v(k,'target'):'')),
   firedBy:flat(by),oneShot:by.some(f=>f.oneShot),usable:kids.some(k=>/^(func_button|inter_door|func_rot_button)$/.test(cls(k))),
   missingInV0105Saves:kids.length>0,keptAsServedNow:kids.length>0&&leadsOut});
 }
 return {spawns,groups};
}
export function auditServedMaps(dir){
 const out={};
 for(const f of fs.readdirSync(dir).filter(f=>f.endsWith('.bsp')).sort()){
  const bytes=fs.readFileSync(path.join(dir,f)),bsp=decodeBsp(bytes);
  const r=auditEntities(bsp.entities,brushModels(bytes,bsp));
  if(r.spawns.length||r.groups.length)out[f.replace(/\.bsp$/,'')]=r;
 }
 return out;
}
export function main(args){
 if(args.length<1)throw Error('Usage: audit-v0106.mjs SERVED_MAPS_DIR [OUTPUT_JSON]');
 const out=auditServedMaps(args[0]);
 if(args[1])fs.writeFileSync(args[1],JSON.stringify(out,null,1),{flag:'wx'});
 for(const [map,r] of Object.entries(out)){
  for(const s of r.spawns)console.log(`${map}\t${s.spawn} ${s.origin} (facing ${s.angles})${s.inExit.length?' IN EXIT '+s.inExit.join(','):''}${s.nearExit.length?' near exit '+s.nearExit.join(','):''}${s.inTransit.length?' in transit '+s.inTransit.join(','):''}`);
  for(const g of r.groups.filter(g=>g.usable||g.oneShot))console.log(`${map}\tgroup ${g.name} [${g.classes.join(',')}] children ${g.children.join(' ')} fired by ${g.firedBy.join('; ')||'-'}${g.keptAsServedNow?' (kept as served: a member leads out)':''}`);
 }
 return out;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){try{main(process.argv.slice(2));}catch(e){console.error(e.stack);process.exitCode=1;}}
