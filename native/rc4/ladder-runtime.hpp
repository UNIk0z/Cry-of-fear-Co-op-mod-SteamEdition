// Included inside the adapter namespace. Native evidence (pinned hl.dll):
// cof_ladder_manager_use Use (1004FE80, allocation 0x184) asks its master
// string +0x17C (UTIL_IsMasterTriggered 10119060) before firing its manager.
// CCoFLadderManager Use (1004FB50, allocation 0x1B4) stores ONE climber
// CBasePlayer* at +0x114 with no occupancy check, freezes him and gives him a
// camera_model view (player +0x5AC). LadderThink (1004ED40) ends the climb on
// its timer, deletes that camera_model and clears player +0x5AC, but never
// clears +0x114. A second Use overwrites the slot and strands the first
// climber frozen.
// v0.2 (user: P2 stuck in the ladder animation going down, ladder jitter):
// the climb camera moves 48 units per step and ladders reach 528 units
// (c_subway2st3). Its steps get EF_NOINTERP: a remote client otherwise slid
// the view between the old and the new step while the restarted sequence
// already played (the host's own client sees the server state at once).
// v0.7 (Playtest 3 clip 1 "ladder no worky worky", Katz: "make it so that we
// are able to use it at the same time or fix teleport"; host log 2026-10-28:
// nerdclimb14, 15 and 16 "reserved by player=1" at once, and never a "waiting
// player(s) moved" line):
//  - the old busy test took any camera_model with the ladder's model within
//    1024 units, so the stale slots of neighbouring ladders that player had
//    climbed before all looked busy and their use brushes were shut;
//  - the E press never reached the old queue: CBasePlayer::PlayerUse calls the
//    brush's Use directly, not through the engine's DispatchUse.
// Now every player can climb the same ladder at once. The use brushes fire
// their manager by name (CCoFLadderManagerUse::Use -> FireTargets, decomp
// CoF_Ladder.cpp). While a manager is busy, that name lookup (the engine's
// FindEntityByString, adapter find hook) returns a free copy of the manager
// instead: the same keyvalues as recorded at map load, its own targetname,
// made the first time it is needed (up to one copy per extra player slot),
// so each climber runs a climb of his own. A manager is busy only while its
// climber's camera_model stands straight above or below it (LadderThink
// moves the camera in z only) with the ladder's model. The slot of a climb
// that has ended (camera gone, climber left) is cleared, so it never holds
// the ladder again. Copies are made during play and are not part of the
// saved map state; a later visit makes them again when needed.
struct LadderCopy {Edict* e=nullptr;int serial=-1;std::string name;int climber=0;};
struct Ladder {Edict* manager=nullptr;int serial=-1;std::string name;int climber=0;std::vector<LadderCopy> copies;};
std::vector<Ladder> ladders;
bool move_beside(Edict* who,Edict* anchor);bool camera_view(Edict* host);Edict* lease_camera(Edict* e);   // coop-runtime.hpp
struct LadderKeys {int serial=-1;std::vector<std::pair<std::string,std::string>> keys;};
std::unordered_map<Edict*,LadderKeys> ladderKeys;
void ladder_reset(){ladders.clear();}
// ServerDeactivate: the next map records its own managers.
void ladder_map_reset(){ladder_reset();ladderKeys.clear();}
// KeyValue wrapper: every key a cof_ladder_manager is spawned with.
void ladder_keyvalue(Edict* e,const KeyValue* kv){
 if(!kv||!kv->classname||!kv->key||!kv->value||std::strcmp(kv->classname,"cof_ladder_manager"))return;
 auto& r=ladderKeys[e];if(r.serial!=serial(e))r={serial(e),{}};if(r.keys.size()<96)r.keys.emplace_back(kv->key,kv->value);
}
void* ladder_memory(Edict* e,std::size_t bytes,const char* type){
 auto it=allocations.find(e);
 if(it==allocations.end()||it->second.serial!=serial(e)||it->second.bytes!=bytes||std::strcmp(classname(e),type)||eng<void*(__cdecl*)(Edict*)>(64)(e)!=it->second.memory)return nullptr;
 return it->second.memory;
}
// The climber slot is a raw CBasePlayer*; resolve it only to a live tracked
// player. Returns his index while he climbs at this manager, else 0. A slot
// whose climb has ended (no camera) or whose player is gone is cleared.
int ladder_climber(Edict* manager,Edict** cameraOut=nullptr){
 auto m=ladder_memory(manager,0x1b4,"cof_ladder_manager");if(!m||!globals)return 0;
 const auto slot=load<std::uint32_t>(m,0x114);if(!slot)return 0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;
  if(!player_memory(e,p,n)||reinterpret_cast<std::uint32_t>(p)!=slot)continue;
  const auto camera=load<std::uint32_t>(p,0x5ac);if(!camera){store<std::uint32_t>(m,0x114,0);return 0;}
  for(const auto& a:allocations){
   if(reinterpret_cast<std::uint32_t>(a.second.memory)!=camera)continue;
   auto cv=vars(a.first),mv=vars(manager);if(!cv||!mv||a.second.serial!=serial(a.first))return i;
   if(std::strcmp(classname(a.first),"camera_model"))return 0;
   const float dx=load<float>(cv,8)-load<float>(mv,8),dy=load<float>(cv,12)-load<float>(mv,12),dz=load<float>(cv,16)-load<float>(mv,16);
   const int cm=load<int>(cv,0xb8),mm=load<int>(mv,0xb8);const bool ownModel=cm==mm||(cm&&mm&&!_stricmp(string_at(cm),string_at(mm)));
   if(!ownModel||dx*dx+dy*dy>4.f||dz*dz>1024.f*1024.f)return 0;
   if(cameraOut)*cameraOut=a.first;return i;
  }
  return i; // untracked camera handle: stay busy, never strand a climber
 }
 store<std::uint32_t>(m,0x114,0);return 0;   // the climber left the game
}
void ladder_activate(){
 ladder_reset();if(!scoped()||!engine[23]||!engine[67]||!original[1]||!original[6])return;
 Edict* e=nullptr;
 for(int n=0;n<256;++n){
  e=find_next(e,"classname","cof_ladder_manager");if(!e)break;
  if(!live(e)||!ladder_memory(e,0x1b4,"cof_ladder_manager"))continue;
  Ladder l;l.manager=e;l.serial=serial(e);l.name=targetname(e);if(l.name.empty())continue;
  if(find_next(nullptr,"target",l.name.c_str()))ladders.push_back(l);
 }
 if(ladders.empty())return;
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER %u ladder(s): every player can climb at the same time\n",static_cast<unsigned>(ladders.size()));log(line);
}
Edict* ladder_copy(Ladder& l){
 auto rec=ladderKeys.find(l.manager);if(rec==ladderKeys.end()||rec->second.serial!=l.serial||!engine[22]||!engine[23]||!engine[67])return nullptr;
 Edict* c=eng<Edict*(__cdecl*)(int)>(23)(eng<int(__cdecl*)(const char*)>(67)("cof_ladder_manager"));if(!live(c))return nullptr;
 const std::string name=l.name+"_sc4copy"+std::to_string(l.copies.size()+1);
 auto put=[&](const std::string& k,const std::string& v){KeyValue data{const_cast<char*>("cof_ladder_manager"),const_cast<char*>(k.c_str()),const_cast<char*>(v.c_str()),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(c,&data);};
 for(const auto& [k,v]:rec->second.keys)if(k!="classname"&&k!="targetname")put(k,v);
 put("targetname",name);
 if(native<int(__cdecl*)(Edict*)>(1)(c)!=0||!ladder_memory(c,0x1b4,"cof_ladder_manager")){if(live(c)){assist_unlink(c);eng<void(__cdecl*)(Edict*)>(22)(c);}return nullptr;}
 l.copies.push_back({c,serial(c),name,0});
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER %s copy %u made (a second climber at once)\n",l.name.c_str(),static_cast<unsigned>(l.copies.size()));log(line);
 return c;
}
// From the find hook: the first lookup of a busy ladder's name gives a free
// copy (none free and no room for another: nothing, so nobody is stranded).
bool ladder_find_override(Edict* start,const char* key,const char* value,Edict*& out){
 if(start||ladders.empty()||!key||!value||std::strcmp(key,"targetname")||!globals)return false;
 for(auto& l:ladders){
  if(l.name!=value||serial(l.manager)!=l.serial)continue;
  if(!ladder_climber(l.manager))return false;
  for(const auto& c:l.copies)if(serial(c.e)==c.serial&&!ladder_climber(c.e)){out=c.e;return true;}
  const int slots=(std::min)(4,load<int>(globals,0x90));
  out=static_cast<int>(l.copies.size())+1<slots?ladder_copy(l):nullptr;return true;
 }
 return false;
}
void ladder_watch(Edict* manager,const std::string& name,int& known){
 Edict* camera=nullptr;const int climber=ladder_climber(manager,&camera);
 if(camera){auto cv=vars(camera);if(cv&&!(load<unsigned>(cv,0x118)&0x20u))store(cv,0x118,load<unsigned>(cv,0x118)|0x20u);}
 if(climber==known)return;
 char line[160]{};std::snprintf(line,sizeof(line),climber?"[STORYCOOP_RC4] LADDER %s climbed by player=%d\n":"[STORYCOOP_RC4] LADDER %s free again (player=%d)\n",name.c_str(),climber?climber:known);log(line);
 known=climber;
}
void ladder_frame(){
 for(auto& l:ladders){
  if(serial(l.manager)!=l.serial)continue;
  ladder_watch(l.manager,l.name,l.climber);
  for(auto& c:l.copies)if(serial(c.e)==c.serial)ladder_watch(c.e,c.name,c.climber);
 }
}
