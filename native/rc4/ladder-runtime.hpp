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
// v0.7.2 (tester: "if player 2 uses a ladder there is a softlock potential"):
//  - FireTargets (subs.cpp) repeats the lookup starting after the entity it just
//    used. ED_Alloc reuses the lowest free edict, so a copy can stand before the
//    original: that next lookup found the busy original and its Use took the slot
//    from its climber, who stayed frozen. The lookup after a copy now ends there.
//  - PlayerSpawn (100C948D) takes ANY player's +0x5AC for a running cutscene and
//    sets m_bCoopWaitingForCutsceneToEnd (+0x3C8), which only a cof_mdlcutscene's
//    end clears: whoever joined or respawned during a climb stayed black and
//    frozen ("Please wait until cutscene has finished."). ladder_wait_frame runs
//    before the first PostThink that would act on it.
//  - A scene can end a climb from outside: cof_mdlcutscene replaces +0x5AC
//    (effects.cpp), trigger_camera flag 2048 removes it (triggers.cpp). LadderThink
//    ran on, stepping and even deleting the scene's camera; the old camera stayed
//    in the world and godmode stayed on, and a scene that did not take control
//    never gave it back. The seat now stops its think, removes its camera and
//    hands the player back when no scene holds them (ladder_ended, release list).
//    A manager whose think has stopped by itself (nextthink 0) ended normally.
//  - The manager keeps per-climb bytes after a climb: m_bEnableControl +0xDD (the
//    next climb ended at once), m_bOriginReductionQueuedPrevious +0x121 and
//    m_bGotOnButNotDescended +0x1A8 (the next climb's camera did not rise after a
//    player got on at the top and went back off). Each climb starts clean.
constexpr std::size_t LadderSlot=0x114,LadderEnableControl=0xdd,LadderQueuedPrevious=0x121,LadderGotOn=0x1a8,PlayerScene=0x5ac,PlayerCutsceneWait=0x3c8;
constexpr unsigned LadderGodmode=0x40u,LadderNotarget=0x80u,LadderFrozen=0x1000u;
struct LadderSeat {Edict* e=nullptr;int serial=-1;std::string name;int climber=0;unsigned stamp=0;Edict* camera=nullptr;int cameraSerial=-1;};
struct Ladder {LadderSeat seat;std::vector<LadderSeat> copies;};
std::vector<Ladder> ladders;
std::unordered_map<Edict*,unsigned> ladderRelease;   // player -> allocation stamp: frozen by a climb a scene ended
bool move_beside(Edict* who,Edict* anchor);bool camera_view(Edict* host);Edict* lease_camera(Edict* e);   // coop-runtime.hpp
struct LadderKeys {int serial=-1;std::vector<std::pair<std::string,std::string>> keys;};
std::unordered_map<Edict*,LadderKeys> ladderKeys;
void ladder_reset(){ladders.clear();ladderRelease.clear();}
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
// The entity behind a raw CBaseEntity* (player +0x5AC), or null when it no longer exists.
Edict* ladder_tracked(std::uint32_t handle){
 if(!handle)return nullptr;
 for(const auto& a:allocations)if(reinterpret_cast<std::uint32_t>(a.second.memory)==handle)return a.second.serial==serial(a.first)?a.first:nullptr;
 return nullptr;
}
bool ladder_holds(Edict* manager,Edict* camera){
 auto cv=vars(camera),mv=vars(manager);if(!cv||!mv||std::strcmp(classname(camera),"camera_model"))return false;
 const float dx=load<float>(cv,8)-load<float>(mv,8),dy=load<float>(cv,12)-load<float>(mv,12),dz=load<float>(cv,16)-load<float>(mv,16);
 const int cm=load<int>(cv,0xb8),mm=load<int>(mv,0xb8);const bool ownModel=cm==mm||(cm&&mm&&!_stricmp(string_at(cm),string_at(mm)));
 return ownModel&&dx*dx+dy*dy<=4.f&&dz*dz<=1024.f*1024.f;
}
// A camera of a climb, not of a scene. A copy stands where its original stands.
bool ladder_is_camera(Edict* camera){for(const auto& l:ladders)if(serial(l.seat.e)==l.seat.serial&&ladder_holds(l.seat.e,camera))return true;return false;}
// The climber slot is a raw CBasePlayer*; resolve it only to a live tracked
// player. Returns his index while he climbs at this manager, else 0. A slot
// whose climb has ended (no camera of this ladder) or whose player is gone is
// cleared, so LadderThink never touches a scene's camera.
int ladder_climber(Edict* manager,Edict** cameraOut=nullptr){
 auto m=ladder_memory(manager,0x1b4,"cof_ladder_manager");if(!m||!globals)return 0;
 const auto slot=load<std::uint32_t>(m,LadderSlot);if(!slot)return 0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;
  if(!player_memory(e,p,n)||reinterpret_cast<std::uint32_t>(p)!=slot)continue;
  auto camera=ladder_tracked(load<std::uint32_t>(p,PlayerScene));
  if(!camera||!ladder_holds(manager,camera)){store<std::uint32_t>(m,LadderSlot,0);return 0;}
  if(cameraOut)*cameraOut=camera;return i;
 }
 store<std::uint32_t>(m,LadderSlot,0);return 0;   // the climber left the game
}
void ladder_activate(){
 ladder_reset();if(!scoped()||!engine[23]||!engine[67]||!original[1]||!original[6])return;
 Edict* e=nullptr;
 for(int n=0;n<256;++n){
  e=find_next(e,"classname","cof_ladder_manager");if(!e)break;
  if(!live(e)||!ladder_memory(e,0x1b4,"cof_ladder_manager"))continue;
  Ladder l;l.seat.e=e;l.seat.serial=serial(e);l.seat.name=targetname(e);if(l.seat.name.empty())continue;
  if(find_next(nullptr,"target",l.seat.name.c_str()))ladders.push_back(l);
 }
 if(ladders.empty())return;
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER %u ladder(s): every player can climb at the same time\n",static_cast<unsigned>(ladders.size()));log(line);
}
Edict* ladder_copy(Ladder& l){
 auto rec=ladderKeys.find(l.seat.e);if(rec==ladderKeys.end()||rec->second.serial!=l.seat.serial||!engine[22]||!engine[23]||!engine[67])return nullptr;
 Edict* c=eng<Edict*(__cdecl*)(int)>(23)(eng<int(__cdecl*)(const char*)>(67)("cof_ladder_manager"));if(!live(c))return nullptr;
 const std::string name=l.seat.name+"_sc4copy"+std::to_string(l.copies.size()+1);
 auto put=[&](const std::string& k,const std::string& v){KeyValue data{const_cast<char*>("cof_ladder_manager"),const_cast<char*>(k.c_str()),const_cast<char*>(v.c_str()),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(c,&data);};
 for(const auto& [k,v]:rec->second.keys)if(k!="classname"&&k!="targetname")put(k,v);
 put("targetname",name);
 if(native<int(__cdecl*)(Edict*)>(1)(c)!=0||!ladder_memory(c,0x1b4,"cof_ladder_manager")){if(live(c)){assist_unlink(c);eng<void(__cdecl*)(Edict*)>(22)(c);}return nullptr;}
 LadderSeat seat;seat.e=c;seat.serial=serial(c);seat.name=name;l.copies.push_back(seat);
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER %s copy %u made (a second climber at once)\n",l.seat.name.c_str(),static_cast<unsigned>(l.copies.size()));log(line);
 return c;
}
// A climb ended. Nothing to do when LadderThink ended it itself (its think has
// stopped, and it gave the player back). Otherwise something else took the
// camera: stop the think, remove the climb's camera if it is still in the
// world, and give back what the climb took once no scene holds the player.
void ladder_ended(LadderSeat& s){
 auto mv=vars(s.e);auto m=ladder_memory(s.e,0x1b4,"cof_ladder_manager");Edict* camera=s.camera;const int camSerial=s.cameraSerial;s.camera=nullptr;
 if(!mv||!m||load<float>(mv,0x104)<=0)return;
 store<unsigned char>(m,LadderEnableControl,0);store(mv,0x104,0.f);
 auto who=eng<Edict*(__cdecl*)(int)>(72)(s.climber);void* p=nullptr;std::size_t n=0;Edict* now=nullptr;auto a=allocations.find(who);
 if(live(who)&&player_memory(who,p,n)&&a!=allocations.end()&&a->second.stamp==s.stamp){
  now=ladder_tracked(load<std::uint32_t>(p,PlayerScene));auto v=vars(who);store(v,0x1a8,load<unsigned>(v,0x1a8)&~LadderGodmode);ladderRelease[who]=s.stamp;}
 if(camera&&camera!=now&&live(camera)&&serial(camera)==camSerial)entity_retire(camera);
 char line[170]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER %s climb of player=%d ended from outside (a scene or a departure)\n",s.name.c_str(),s.climber);log(line);
}
void ladder_watch(LadderSeat& s){
 Edict* camera=nullptr;const int climber=ladder_climber(s.e,&camera);
 if(camera){
  auto cv=vars(camera);if(cv&&!(load<unsigned>(cv,0x118)&0x20u))store(cv,0x118,load<unsigned>(cv,0x118)|0x20u);
  s.camera=camera;s.cameraSerial=serial(camera);
  // A scene's end gives every player control back, a climber too.
  auto who=eng<Edict*(__cdecl*)(int)>(72)(climber);auto a=allocations.find(who);auto pv=vars(who);if(a!=allocations.end())s.stamp=a->second.stamp;
  if(pv&&!(load<unsigned>(pv,0x1a8)&LadderFrozen))store(pv,0x1a8,load<unsigned>(pv,0x1a8)|LadderFrozen);
 }
 if(climber==s.climber)return;
 if(s.climber&&!climber)ladder_ended(s);
 char line[160]{};std::snprintf(line,sizeof(line),climber?"[STORYCOOP_RC4] LADDER %s climbed by player=%d\n":"[STORYCOOP_RC4] LADDER %s free again (player=%d)\n",s.name.c_str(),climber?climber:s.climber);log(line);
 s.climber=climber;
}
// What the next climb must not inherit from the last one on this manager.
void ladder_fresh(Edict* manager){
 auto m=ladder_memory(manager,0x1b4,"cof_ladder_manager");if(!m)return;
 store<unsigned char>(m,LadderEnableControl,0);store<unsigned char>(m,LadderQueuedPrevious,0);store<unsigned char>(m,LadderGotOn,0);
}
// From the find hook: the first lookup of a busy ladder's name gives a free
// copy (none free and no room for another: nothing, so nobody is stranded), and
// the lookup after a copy ends there, whatever edict the copy got.
bool ladder_find_override(Edict* start,const char* key,const char* value,Edict*& out){
 if(ladders.empty()||!key||!value||std::strcmp(key,"targetname")||!globals)return false;
 for(auto& l:ladders){
  if(l.seat.name!=value||serial(l.seat.e)!=l.seat.serial)continue;
  if(start){for(const auto& c:l.copies)if(c.e==start){out=nullptr;return true;}continue;}
  ladder_watch(l.seat);
  if(!l.seat.climber){ladder_fresh(l.seat.e);return false;}
  for(auto& c:l.copies)if(serial(c.e)==c.serial){ladder_watch(c);if(!c.climber){ladder_fresh(c.e);out=c.e;return true;}}
  const int slots=(std::min)(4,load<int>(globals,0x90));
  out=static_cast<int>(l.copies.size())+1<slots?ladder_copy(l):nullptr;return true;
 }
 return false;
}
void ladder_release_frame(){
 for(auto it=ladderRelease.begin();it!=ladderRelease.end();){
  auto e=it->first;void* p=nullptr;std::size_t n=0;auto v=vars(e);auto a=allocations.find(e);
  if(!v||!live(e)||!player_memory(e,p,n)||a==allocations.end()||a->second.stamp!=it->second){it=ladderRelease.erase(it);continue;}
  if(ladder_tracked(load<std::uint32_t>(p,PlayerScene))||camera_view(e)||lease_camera(e)){++it;continue;}   // the scene's end does this, or comes first
  store(v,0x1a8,load<unsigned>(v,0x1a8)&~(LadderFrozen|LadderNotarget));
  char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER player=%d has controls back\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
  it=ladderRelease.erase(it);
 }
}
void ladder_frame(){
 for(auto& l:ladders){
  if(serial(l.seat.e)!=l.seat.serial)continue;
  ladder_watch(l.seat);
  for(auto& c:l.copies)if(serial(c.e)==c.serial)ladder_watch(c);
 }
 ladder_release_frame();
}
// Before the player's think: a climb is no cutscene to wait for. Native rule
// otherwise: a camera with iuser4 1 (+0x254) never counts, any other does.
void ladder_wait_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;if(ladders.empty()||!player_memory(e,p,n)||!load<unsigned char>(p,PlayerCutsceneWait))return;
 for(auto o:connected_players()){
  void* q=nullptr;if(!player_memory(o,q,n))continue;auto c=ladder_tracked(load<std::uint32_t>(q,PlayerScene));auto cv=c?vars(c):nullptr;
  if(cv&&load<int>(cv,0x254)!=1&&!ladder_is_camera(c))return;
 }
 store<unsigned char>(p,PlayerCutsceneWait,0);
 char line[130]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER player=%d not held by the cutscene wait (only a climb)\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
}
// A climber who leaves: the seat frees and the camera goes too.
void ladder_left(Edict* e){
 void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))return;const auto me=reinterpret_cast<std::uint32_t>(p);
 auto leave=[&](LadderSeat& s){auto m=ladder_memory(s.e,0x1b4,"cof_ladder_manager");if(!m||load<std::uint32_t>(m,LadderSlot)!=me)return;store<std::uint32_t>(m,LadderSlot,0);store<std::uint32_t>(p,PlayerScene,0);ladder_watch(s);};
 for(auto& l:ladders){if(serial(l.seat.e)==l.seat.serial)leave(l.seat);for(auto& c:l.copies)if(serial(c.e)==c.serial)leave(c);}
 ladderRelease.erase(e);
}
