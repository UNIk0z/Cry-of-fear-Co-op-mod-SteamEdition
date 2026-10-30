// Included inside the adapter namespace, before checkpoint-runtime.hpp.
// Campaign-wide pieces shared by checkpoints and the boat (2026-10-10 user
// request: saves, pickups and the late boat section must work everywhere).
// SDK calls only: engine TraceHull34/PointContents45/SetOrigin28 and the
// native cof_teleport through the existing helper path.
struct KeyValue {char* classname;char* key;char* value;int handled;};
bool helper_use_pairs(const char* type,const std::vector<std::pair<std::string,std::string>>& pairs,Edict* actor);
// --- Map entity registry ------------------------------------------------------
// The engine dispatches DLL Spawn once per BSP entity, in lump order, starting
// with worldspawn; entities created by game code never pass this wrapper. The
// position in this list is therefore the BSP entity index of the served map.
// key: the v0.3 map-state identity, read before the native Spawn (state-runtime.hpp).
// dm: spawnflags 2048, an entity v0.4 and older never had (see deathmatch_open).
struct SpawnRecord {Edict* e=nullptr;int serial=-1;std::string type,key;bool pickup=false,locked=false,dm=false;};
std::vector<SpawnRecord> spawnRecords;
// --- Entities the engine dropped in co-op (v0.5) ---------------------------------
// Playtest logs (v0.4): every item lock on a button failed (lighter, saw, spade,
// screwdriver, fuse, rope, ladder) although the player stood within the native
// 64-unit sphere of CBasePlayer::InventoryUse (decomp player.cpp); key doors
// worked. The engine's map loader (hw.dll ED_LoadFromFile, "%i entities
// inhibited") frees every BSP entity with spawnflags 2048 (Quake's
// NOT_DEATHMATCH) while the deathmatch cvar is set, and co-op runs with
// deathmatch 1 (maxplayers > 1; InstallGameRules needs it for the multiplayer
// rules). CoF uses 2048 on func_button for "unlocking presses the button"
// (CBaseButton::UnlockDoor): all 86 item-lock buttons on 15 base maps, plus 8
// trigger_cameras on s4_subway2st3, never existed in co-op. On story maps the
// cvar's value reads 0 from the moment worldspawn has spawned (the game rules
// are installed by then) until ServerActivate, so the loader keeps them as in
// single player. Only the cvar's value is touched: no "changed" broadcast, and
// gpGlobals->deathmatch (game code) stays 1.
struct DeathmatchHold {void* cvar=nullptr;float value=0;bool held=false;};
DeathmatchHold deathmatchHold;int dmSpawned=0;
void deathmatch_open(){
 if(deathmatchHold.held||!engine[116])return;void* c=eng<void*(__cdecl*)(const char*)>(116)("deathmatch");if(!c)return;
 deathmatchHold={c,load<float>(c,12),true};if(deathmatchHold.value!=0)store(c,12,0.f);
}
void deathmatch_close(){
 if(!deathmatchHold.held)return;store(deathmatchHold.cvar,12,deathmatchHold.value);deathmatchHold={};
 if(dmSpawned){char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SPAWNFLAGS kept %d entit%s with flag 2048 (the engine drops them in multiplayer)\n",dmSpawned,dmSpawned==1?"y":"ies");log(line);}
 dmSpawned=0;
}
// --- Difficulty and the host's setup of this playthrough (v0.5) -------------------
// The menu's difficulty page (client Difficulty_guess.cpp) sends "skillset N";
// on the menu server that sets the "difficulty" cvar (1..3, 4 nightmare;
// decomp client.cpp) and the cvar survives the restart into the co-op server.
// A save keeps it (checkpoint schema 8) and a load puts it back, whatever the
// menu page chose (D1). Nightmare's stamina rules read the player's own
// m_bNightmare (player+0x1EE0 in the pinned hl.dll's save table), which every
// co-op player is rebuilt without; nightmare_frame (coop-runtime) sets it.
SessionOptions sessionOptions;
int difficulty_now(){if(!engine[57])return 0;const float d=eng<float(__cdecl*)(const char*)>(57)("difficulty");return d>=1&&d<=4?static_cast<int>(d):0;}
// Saves index BSP entities by their position in spawnRecords. Older saves were
// written without the dm entities, so the saved index counts only the others.
int legacy_index(std::size_t i){if(i<1||i>=spawnRecords.size()||spawnRecords[i].dm)return -1;int n=0;for(std::size_t k=1;k<i;++k)if(spawnRecords[k].dm)++n;return static_cast<int>(i)-n;}
int record_index(int legacy){if(legacy<1)return -1;int seen=0;for(std::size_t i=1;i<spawnRecords.size();++i)if(!spawnRecords[i].dm&&++seen==legacy)return static_cast<int>(i);return -1;}
std::set<std::string> toggledDoors;
// Pickups without an authored pickup target. A pickup that fires a target is
// left in place on load: picking it up again replays its story event.
bool pickup_class(const std::string& t){return (t.rfind("item_",0)==0&&t!="item_padlock")||t.rfind("weapon_",0)==0||t.rfind("ammo_",0)==0;}
// inter_door private 0x1B0 allocation, lockedby at +0x190 (keydoor-runtime).
int door_locked(Edict* door){
 auto it=allocations.find(door);
 if(it==allocations.end()||it->second.serial!=serial(door)||it->second.bytes!=0x1b0||std::strcmp(classname(door),"inter_door")||eng<void*(__cdecl*)(Edict*)>(64)(door)!=it->second.memory)return -1;
 return load<int>(it->second.memory,0x190)!=0?1:0;
}
void world_spawned(Edict* e,bool world,int result,const std::string& key={},bool dm=false){
 if(world){spawnRecords.clear();toggledDoors.clear();}
 SpawnRecord r;r.dm=dm&&!world;if(r.dm&&result==0)++dmSpawned;
 if(result==0&&live(e)){
  r.e=e;r.serial=serial(e);r.type=classname(e);r.key=key;auto p=vars(e);
  r.pickup=p&&pickup_class(r.type)&&!load<int>(p,0x1cc)&&!(load<unsigned>(p,0x118)&0x80u);
  r.locked=r.type=="inter_door"&&door_locked(e)==1;
 }
 spawnRecords.push_back(r);
}
int spawn_index(Edict* e){for(std::size_t i=1;i<spawnRecords.size();++i)if(spawnRecords[i].e==e&&spawnRecords[i].serial==serial(e))return legacy_index(i);return -1;}
// Doors named by a cof_interdooronoff (entvars message +0x1D8) are toggled by
// authored story logic; restoring their lock here could invert a later toggle.
void world_activate(){
 toggledDoors.clear();Edict* e=nullptr;
 for(int n=0;n<512;++n){e=find_next(e,"classname","cof_interdooronoff");if(!e)break;if(auto p=vars(e)){const char* name=string_at(load<int>(p,0x1d8));if(name&&*name)toggledDoors.insert(name);}}
}
bool world_taken(const SpawnRecord& r){
 if(serial(r.e)!=r.serial||r.type!=classname(r.e))return true;
 auto p=vars(r.e);return p&&load<int>(p,0x10c)==0&&(load<unsigned>(p,0x118)&0x80u);
}
void world_capture(Checkpoint& value){
 value.removed.clear();value.unlocked.clear();
 for(std::size_t i=1;i<spawnRecords.size();++i){
  const auto& r=spawnRecords[i];if(!r.e||r.dm)continue;const int index=legacy_index(i);
  if(r.pickup&&world_taken(r)&&value.removed.size()<WorldRemovedLimit)value.removed.push_back({index,r.type});
  if(r.locked&&serial(r.e)==r.serial&&!toggledDoors.count(targetname(r.e))&&door_locked(r.e)==0&&value.unlocked.size()<WorldUnlockedLimit)value.unlocked.push_back(index);
 }
}
// Same removal as a native killtarget: hidden and unsolid now, no pending
// think, FL_KILLME frees it at the end of the next physics frame; no pickup
// touch can run in between.
// v0.4 (v0.3 load crash, s4_start_e0 slot 1, debugger 2026-10-18: EIP 0x70 in
// CheckDesiredList, hl 100C4120 -> ApplyDesiredSettings 100C3E50): Spirit's
// assist/desired list links live entities through CBaseEntity +0x3C
// (m_pAssistLink) and keeps them while m_iLFlags (+0x60) has LF_DOASSIST or
// LF_DODESIRED. trigger_auto::Activate queues UTIL_DesiredAction until the
// first PlayerPostThink, so a removed fire-once trigger_auto was freed while
// still linked and the first PostThink called DesiredAction on freed memory.
// FL_DORMANT makes the game's ServerActivate skip Activate (client.cpp), and
// with the assist bits cleared the game's own CheckAssistList (StartFrame)
// unlinks the entity before the engine frees it later in that frame: the
// order a trigger_auto relies on when it removes itself.
constexpr unsigned FlKillMe=0x40000000u,FlDormant=0x80000000u,LfAssistBits=0x20u|0x80u|0x100u|0x200u|0x400u|0x800u;
void entity_retire(Edict* e){
 auto p=vars(e);if(!p)return;assist_unlink(e);
 store(p,0x10c,0);store(p,0x118,load<unsigned>(p,0x118)|0x80u);store(p,0x1d0,0);store(p,0x104,0.f);store(p,0x1a8,load<unsigned>(p,0x1a8)|FlKillMe|FlDormant);
 void* d=engine[64]?eng<void*(__cdecl*)(Edict*)>(64)(e):nullptr;   // CBaseEntity: vtable, then pev at +4
 if(d&&load<void*>(d,4)==p)store(d,0x60,load<unsigned>(d,0x60)&~LfAssistBits);
 const std::array<float,3> at{load<float>(p,8),load<float>(p,12),load<float>(p,16)};eng<void(__cdecl*)(Edict*,const float*)>(28)(e,at.data());
}
bool state_reoffer(const SpawnRecord& r,const Checkpoint& value);
void world_apply(const Checkpoint& value){
 int removed=0,unlocked=0,skipped=0;
 for(const auto& [saved,type]:value.removed){
  const int index=record_index(saved);if(index<1){++skipped;continue;}
  const auto& r=spawnRecords[index];auto p=vars(r.e);
  if(!r.e||!p||serial(r.e)!=r.serial||r.type!=type||r.type!=classname(r.e)){++skipped;continue;}
  if(state_reoffer(r,value)){++skipped;continue;}
  entity_retire(r.e);++removed;
 }
 for(int saved:value.unlocked){
  const int index=record_index(saved);if(index<1){++skipped;continue;}
  const auto& r=spawnRecords[index];
  if(!r.e||serial(r.e)!=r.serial||toggledDoors.count(targetname(r.e))||door_locked(r.e)!=1){++skipped;continue;}
  store(allocations.at(r.e).memory,0x190,0);++unlocked;
 }
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP] WORLD restored removed-pickups=%d unlocked-doors=%d skipped=%d\n",removed,unlocked,skipped);log(line);
}
// --- Shared keypad code (coop-fixes-004) ---------------------------------------
// Keypad codes live on each player: +0x2040 code (9999 = none, set 100E5996),
// +0x2044 keypad type. cof_keypad2 (100A6FE0) draws "279%i" with RANDOM_LONG
// into the ACTIVATOR only; cof_keypad4 (100A7120) and the "%i7%i9" generator
// (100A8EB0) write the FIRST player only; cof_addcodenote (100A6790) prints the
// first player's code into a note the shared journal hands to everyone; the
// check compares the user's OWN +0x2040. Map changes respawn players with the
// default. One session code: any native change on any player is adopted (a
// player's spawn default never is), written to every player each frame, saved
// in checkpoint schema 3 and cleared for a new campaign.
struct KeypadCode {bool have=false;int code=9999,type=0;};
KeypadCode keypadShared;
std::unordered_map<Edict*,std::pair<int,std::pair<int,int>>> keypadSeen;
void keypad_reset(){keypadShared={};keypadSeen.clear();}
void keypad_map_changed(){keypadSeen.clear();}
void keypad_frame(){
 if(!scoped()||!globals)return;
 const int count=load<int>(globals,0x90);
 for(int i=1;i<=count;++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  const std::pair<int,int> now{load<int>(p,0x2040),load<int>(p,0x2044)};auto it=keypadSeen.find(e);
  if(it==keypadSeen.end()||it->second.first!=serial(e)){keypadSeen[e]={serial(e),now};continue;}
  if(now!=it->second.second&&now.first>=0&&now.first<=9999&&now.second>=0&&now.second<=16){
   keypadShared={true,now.first,now.second};it->second.second=now;
   char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] KEYPAD shared code=%d type=%d from player=%d\n",now.first,now.second,i);log(line);
  }
 }
 if(!keypadShared.have)return;
 for(int i=1;i<=count;++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  if(load<int>(p,0x2040)!=keypadShared.code||load<int>(p,0x2044)!=keypadShared.type){store(p,0x2040,keypadShared.code);store(p,0x2044,keypadShared.type);}
  keypadSeen[e]={serial(e),{keypadShared.code,keypadShared.type}};
 }
}
void keypad_capture(Checkpoint& value){value.haveKeypad=keypadShared.have;value.keypadCode=keypadShared.code;value.keypadType=keypadShared.type;}
void keypad_load(const Checkpoint& value){keypadShared={value.haveKeypad,value.keypadCode,value.keypadType};keypadSeen.clear();}
// --- Safe landings ------------------------------------------------------------
struct Trace {int allSolid=0,startSolid=0,inOpen=0,inWater=0;float fraction=0;float end[3]{};float planeDist=0;float normal[3]{};Edict* hit=nullptr;int hitgroup=0;};
static_assert(sizeof(Trace)==56,"SDK TraceResult ABI");
Trace hull_trace(const std::array<float,3>& a,const std::array<float,3>& b,bool monsters,Edict* skip){Trace t{};eng<void(__cdecl*)(const float*,const float*,int,int,Edict*,Trace*)>(34)(a.data(),b.data(),monsters?0:1,1,skip,&t);return t;}
// A standing player hull fits at `at`, in open air (not water/solid), with a
// floor below and a straight walkable line from the anchor (never through a
// wall into the next room).
bool landing_clear(const std::array<float,3>& anchor,const std::array<float,3>& at,Edict* skip,bool worldFloor=false){
 if(!engine[34]||!engine[45])return false;
 const auto here=hull_trace(at,at,true,skip);if(here.allSolid||here.startSolid)return false;
 if(eng<int(__cdecl*)(const float*)>(45)(at.data())!=-1)return false;
 const auto floor=hull_trace(at,{at[0],at[1],at[2]-160},false,skip);if(floor.allSolid||floor.fraction>=1.0f)return false;if(worldFloor&&floor.hit&&eng<int(__cdecl*)(Edict*)>(71)(floor.hit)!=0)return false;
 if(at!=anchor){const auto path=hull_trace({anchor[0],anchor[1],at[2]},at,false,skip);if(path.allSolid||path.startSolid||path.fraction<1.0f)return false;}
 return true;
}
bool find_landing(const std::array<float,3>& anchor,Edict* skip,std::array<float,3>& out,bool worldFloor=false){
 // v0.5 (logs: "SCENE_TELEPORT ... failed=1" on s4_asylum1_e1 and s4_roofs_e0,
 // "REGROUP multispotea ... could not reach"): two more rings, 16 directions on the wide ones.
 for(float radius:{0.f,48.f,96.f,144.f,192.f,240.f}){
  const int steps=radius==0?1:radius<=96.f?8:16;
  for(int k=0;k<steps;++k){
   const float angle=static_cast<float>(k)*6.2831853f/static_cast<float>(steps);
   for(float lift:{0.f,18.f,36.f}){
    const std::array<float,3> at{anchor[0]+radius*std::cos(angle),anchor[1]+radius*std::sin(angle),anchor[2]+lift};
    if(landing_clear(anchor,at,skip,worldFloor)){out=at;return true;}
   }
  }
 }
 return false;
}
// Native cof_teleport (100A7990) puts the player origin at target+mins.z+1,
// i.e. 37 units above the target, zeroes velocity and copies the angles.
bool teleport_to(Edict* e,const std::array<float,3>& at,float yaw){
 if(!engine[23]||!engine[67]||!engine[22]||!original[1]||!original[6])return false;
 Edict* marker=eng<Edict*(__cdecl*)(int)>(23)(eng<int(__cdecl*)(const char*)>(67)("info_teleport_destination"));if(!live(marker))return false;
 char origin[96]{},angles[48]{};std::snprintf(origin,sizeof(origin),"%.2f %.2f %.2f",at[0],at[1],at[2]-37.0f);std::snprintf(angles,sizeof(angles),"0 %.2f 0",yaw);
 bool okay=true;
 for(const auto& [key,value]:std::vector<std::pair<const char*,const char*>>{{"targetname","sc4_coop_landing"},{"origin",origin},{"angles",angles}}){KeyValue data{const_cast<char*>("info_teleport_destination"),const_cast<char*>(key),const_cast<char*>(value),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(marker,&data);}
 okay=native<int(__cdecl*)(Edict*)>(1)(marker)==0&&helper_use_pairs("cof_teleport",{{"target","sc4_coop_landing"}},e);
 if(live(marker)){assist_unlink(marker);eng<void(__cdecl*)(Edict*)>(22)(marker);}
 return okay;
}
// --- Boat (c_lake, c_cof_lake) --------------------------------------------------
// CBoat::BoatUse (10022690) takes ONE driver; Embark hides that player and
// Disembark (10022750) puts only the driver on the boat_exit spot. The other
// players stayed on the far shore with no way across. When a driver leaves the
// boat next to a boat_exit, bring every other living player beside them.
std::unordered_map<Edict*,int> boatDrivers;
void boat_reset(){boatDrivers.clear();}
void boat_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;if(!scoped()||!player_memory(e,p,n))return;
 auto boat=camera_handle(p,0x380);
 const bool aboard=(load<int>(p,0x2500)&8)&&boat&&std::strcmp(classname(boat),"boat")==0;
 auto known=boatDrivers.find(e);
 if(aboard){
  if(known==boatDrivers.end()||known->second!=serial(e)){boatDrivers[e]=serial(e);char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BOAT embarked player=%d\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
   for(int i=1;i<=load<int>(globals,0x90);++i){auto other=eng<Edict*(__cdecl*)(int)>(72)(i);if(other!=e&&live(other)&&std::strcmp(classname(other),"player")==0&&engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(other,1,"Your partner is rowing.\nYou join them at the dock.\n");}}   // v0.5: one line was cut off (playtest "text is bugged")
  return;
 }
 if(known==boatDrivers.end())return;
 const bool same=known->second==serial(e);boatDrivers.erase(known);if(!same)return;
 auto v=vars(e);if(!v||load<int>(v,0x174)!=0)return;
 const std::array<float,3> anchor{load<float>(v,8),load<float>(v,12),load<float>(v,16)};
 Edict* exit=nullptr;Edict* found=nullptr;
 for(int k=0;k<16;++k){exit=find_next(exit,"classname","boat_exit");if(!exit)break;auto q=vars(exit);if(!q)continue;const float dx=load<float>(q,8)-anchor[0],dy=load<float>(q,12)-anchor[1];if(dx*dx+dy*dy<=192*192){found=exit;break;}}
 if(!found){log("[STORYCOOP_RC4] BOAT disembarked away from a dock; nobody moved\n");return;}
 int moved=0,failed=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto other=eng<Edict*(__cdecl*)(int)>(72)(i);auto q=vars(other);
  if(other==e||!q||std::strcmp(classname(other),"player")||load<int>(q,0x174)!=0||load<float>(q,0x164)<=0)continue;
  const float dx=load<float>(q,8)-anchor[0],dy=load<float>(q,12)-anchor[1],dz=load<float>(q,16)-anchor[2];
  if(dx*dx+dy*dy+dz*dz<=128*128)continue;   // v0.7 (Playtest 3 clip 19, "partners moved=0"): the far shore can be under 400 units away
  std::array<float,3> at{};if(find_landing(anchor,other,at)&&teleport_to(other,at,load<float>(v,0x54)))++moved;else ++failed;
 }
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BOAT docked player=%d; partners moved=%d failed=%d\n",eng<int(__cdecl*)(Edict*)>(71)(e),moved,failed);log(line);
}
// --- Shared objectives (coop-fixes-004, v0.5) -------------------------------------
// cof_objective Use (decomp effects.cpp) sets the FIRST player's objective text
// m_sObjectiveText (+0x1E90, the key in txtfiles/hints.txt) and, when it names
// one, a custom hints file m_sCustomObjectives (+0x420, cleared once sent).
// UpdateClientData sends the text to that player when it differs from what the
// client has, and B shows it. coop-fixes-004 shared only objectives with a
// custom file (+0x420), so most never reached the others; and every map change
// rebuilt the players without one (playtest: B showed nothing for clients).
// v0.5: the text any player's native code sets is the session's objective;
// every player whose text differs gets it (late joiners and new maps too). It
// is kept across maps and in saves (checkpoint schema 8).
std::string objectiveText,objectiveFile;int objectiveAlloc=0,objectiveFileAlloc=0;
std::unordered_map<Edict*,std::pair<int,int>> objectiveSeen;
void objective_reset(){objectiveAlloc=0;objectiveFileAlloc=0;objectiveSeen.clear();}   // map change: string handles are per map
void objective_session_reset(){objectiveText.clear();objectiveFile.clear();objective_reset();}
bool objective_text_ok(const char* s){return s&&*s&&std::strlen(s)<=120;}
void objective_share(){
 if(!scoped()||!globals||!engine[67])return;
 const int count=load<int>(globals,0x90);
 for(int i=1;i<=count;++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  const int value=load<int>(p,0x1e90);auto& seen=objectiveSeen[e];if(seen.first!=serial(e)){seen={serial(e),value};continue;}
  if(value&&value!=seen.second&&value!=objectiveAlloc){const char* s=string_at(value);
   if(objective_text_ok(s)&&objectiveText!=s){objectiveText=s;const int f=load<int>(p,0x420);const char* fs=f?string_at(f):nullptr;objectiveFile=objective_text_ok(fs)?fs:"";objectiveAlloc=value;objectiveFileAlloc=objectiveFile.empty()?0:f;
    char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] OBJECTIVE %s%s%s from player=%d, shown to everyone\n",objectiveText.c_str(),objectiveFile.empty()?"":" in ",objectiveFile.c_str(),i);log(line);}}
  seen.second=value;
 }
 if(objectiveText.empty())return;
 if(!objectiveAlloc){objectiveAlloc=eng<int(__cdecl*)(const char*)>(67)(objectiveText.c_str());objectiveFileAlloc=objectiveFile.empty()?0:eng<int(__cdecl*)(const char*)>(67)(objectiveFile.c_str());}
 for(int i=1;i<=count;++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  const int value=load<int>(p,0x1e90);const char* s=value?string_at(value):nullptr;if(s&&objectiveText==s)continue;
  store(p,0x1e90,objectiveAlloc);if(objectiveFileAlloc)store(p,0x420,objectiveFileAlloc);objectiveSeen[e]={serial(e),objectiveAlloc};
 }
}