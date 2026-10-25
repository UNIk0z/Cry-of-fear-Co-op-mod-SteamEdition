// Included inside the adapter namespace. Native evidence (pinned hl.dll):
// cof_ladder_manager_use Use (1004FE80, allocation 0x184) asks its master
// string +0x17C (UTIL_IsMasterTriggered 10119060) before firing its manager.
// CCoFLadderManager Use (1004FB50, allocation 0x1B4) stores ONE climber
// CBasePlayer* at +0x114 with no occupancy check, freezes him and gives him a
// camera_model view (player +0x5AC). LadderThink (1004ED40) ends the climb on
// its timer, deletes that camera_model and clears player +0x5AC, but never
// clears +0x114. A second Use overwrites the slot and strands the first
// climber frozen. While a climber still owns a camera_model at that ladder,
// its use brushes get a never-triggered master; the original is restored when
// the climb ends, the climber disconnects or the map changes. Per ladder only.
// v0.2 (user: P2 stuck in the ladder animation going down, ladder jitter):
// the climb camera was only counted within 384 units of the manager, but
// LadderThink moves it 48 units per step and ladders reach 528 units
// (c_subway2st3); mid-climb the lock dropped, the other player's E took the
// single climber slot and the first climber stayed frozen on the ladder. The
// camera now counts while it carries this ladder's model (Use: SET_MODEL with
// the manager's model) within 1024 units. Its 48-unit steps also get
// EF_NOINTERP: a remote client otherwise slid the view between the old and
// the new step while the restarted sequence already played (the host's own
// client sees the server state at once).
struct LadderUse {Edict* brush=nullptr;int serial=-1;std::uint32_t original=0;};
struct Ladder {Edict* manager=nullptr;int serial=-1;std::string name;std::vector<LadderUse> uses;int climber=0;std::vector<std::pair<Edict*,int>> waiting;};
std::vector<Ladder> ladders;
bool move_beside(Edict* who,Edict* anchor);bool camera_view(Edict* host);Edict* lease_camera(Edict* e);   // coop-runtime.hpp
Edict* ladderGate=nullptr;int ladderGateSerial=-1;std::uint32_t ladderGateName=0;
void ladder_reset(){ladders.clear();ladderGate=nullptr;ladderGateSerial=-1;ladderGateName=0;}
void* ladder_memory(Edict* e,std::size_t bytes,const char* type){
 auto it=allocations.find(e);
 if(it==allocations.end()||it->second.serial!=serial(e)||it->second.bytes!=bytes||std::strcmp(classname(e),type)||eng<void*(__cdecl*)(Edict*)>(64)(e)!=it->second.memory)return nullptr;
 return it->second.memory;
}
// The climber slot is a raw CBasePlayer*; resolve it only to a live tracked player.
int ladder_climber(const Ladder& l,Edict** cameraOut=nullptr){
 auto m=ladder_memory(l.manager,0x1b4,"cof_ladder_manager");if(!m)return 0;
 const auto slot=load<std::uint32_t>(m,0x114);if(!slot||!globals)return 0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;
  if(!player_memory(e,p,n)||reinterpret_cast<std::uint32_t>(p)!=slot)continue;
  const auto camera=load<std::uint32_t>(p,0x5ac);if(!camera)return 0;
  // v0.6 (Katz 2026-10-25: ladders "show as busy when the last person who climbed
  // them is in a cutscene"): the native slot +0x114 is never cleared, and a scene
  // can hand that player another view. A player held by a scene camera, or
  // whose view entity is not a camera_model, is not climbing.
  if(camera_view(e)||lease_camera(e))return 0;
  // A player keeps this ladder only while his camera_model stands at it.
  for(const auto& a:allocations){
   if(reinterpret_cast<std::uint32_t>(a.second.memory)!=camera)continue;
   auto cv=vars(a.first),mv=vars(l.manager);if(!cv||!mv||a.second.serial!=serial(a.first))return i;
   if(std::strcmp(classname(a.first),"camera_model"))return 0;
   const float dx=load<float>(cv,8)-load<float>(mv,8),dy=load<float>(cv,12)-load<float>(mv,12),dz=load<float>(cv,16)-load<float>(mv,16);
   const int cm=load<int>(cv,0xb8),mm=load<int>(mv,0xb8);const bool ownModel=cm==mm||(cm&&mm&&!_stricmp(string_at(cm),string_at(mm)));
   if(!ownModel||dx*dx+dy*dy+dz*dz>1024.f*1024.f)return 0;
   if(cameraOut)*cameraOut=a.first;return i;
  }
  return i; // untracked camera handle: stay reserved, never strand a climber
 }
 return 0;
}
void ladder_apply(Ladder& l,bool reserved){
 for(auto& u:l.uses){auto m=ladder_memory(u.brush,0x184,"cof_ladder_manager_use");if(m&&serial(u.brush)==u.serial)store(m,0x17c,reserved?ladderGateName:u.original);}
}
void ladder_activate(){
 ladder_reset();if(!scoped()||!engine[23]||!engine[67]||!original[1]||!original[6])return;
 Edict* e=nullptr;
 for(int n=0;n<256;++n){
  e=find_next(e,"classname","cof_ladder_manager");if(!e)break;
  if(!live(e)||!ladder_memory(e,0x1b4,"cof_ladder_manager"))continue;
  Ladder l;l.manager=e;l.serial=serial(e);l.name=targetname(e);if(l.name.empty())continue;
  Edict* u=nullptr;
  for(int k=0;k<64;++k){
   u=find_next(u,"target",l.name.c_str());if(!u)break;
   auto m=live(u)?ladder_memory(u,0x184,"cof_ladder_manager_use"):nullptr;if(!m)continue;
   l.uses.push_back({u,serial(u),load<std::uint32_t>(m,0x17c)});
  }
  if(!l.uses.empty())ladders.push_back(l);
 }
 if(ladders.empty())return;
 // One authored-style never-true master: no inputs and a global that is never set.
 const int id=eng<int(__cdecl*)(const char*)>(67)("multisource");auto gate=eng<Edict*(__cdecl*)(int)>(23)(id);if(!live(gate)){ladders.clear();return;}
 for(const auto& [k,value]:std::initializer_list<std::pair<const char*,const char*>>{{"targetname","sc4_ladder_reserved"},{"globalstate","sc4.ladder.reserved.never"}}){
  KeyValue data{const_cast<char*>("multisource"),const_cast<char*>(k),const_cast<char*>(value),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(gate,&data);
 }
 if(native<int(__cdecl*)(Edict*)>(1)(gate)!=0){assist_unlink(gate);eng<void(__cdecl*)(Edict*)>(22)(gate);ladders.clear();return;}
 ladderGate=gate;ladderGateSerial=serial(gate);ladderGateName=static_cast<std::uint32_t>(eng<int(__cdecl*)(const char*)>(67)("sc4_ladder_reserved"));
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LADDER reservation active for %u ladders\n",static_cast<unsigned>(ladders.size()));log(line);
}
void ladder_frame(){
 if(ladders.empty()||serial(ladderGate)!=ladderGateSerial)return;
 for(auto& l:ladders){
  if(serial(l.manager)!=l.serial)continue;
  Edict* camera=nullptr;const int climber=ladder_climber(l,&camera);
  if(camera){auto cv=vars(camera);if(cv&&!(load<unsigned>(cv,0x118)&0x20u))store(cv,0x118,load<unsigned>(cv,0x118)|0x20u);}
  if(climber==l.climber)continue;
  ladder_apply(l,climber!=0);
  char line[160]{};std::snprintf(line,sizeof(line),climber?"[STORYCOOP_RC4] LADDER %s reserved by player=%d\n":"[STORYCOOP_RC4] LADDER %s released (player=%d)\n",l.name.c_str(),climber?climber:l.climber);log(line);
  // v0.6: the players who pressed E on it during the climb follow the climber.
  if(!climber&&l.climber){Edict* c=eng<Edict*(__cdecl*)(int)>(72)(l.climber);int moved=0;
   if(live(c)&&view_alive(c))for(const auto& [w,ws]:l.waiting)if(live(w)&&serial(w)==ws&&w!=c&&view_alive(w)&&move_beside(w,c))++moved;
   if(!l.waiting.empty()){char follow[160]{};std::snprintf(follow,sizeof(follow),"[STORYCOOP_RC4] LADDER %s: %d waiting player(s) moved beside player=%d at the end of the climb\n",l.name.c_str(),moved,l.climber);log(follow);}
   l.waiting.clear();}
  l.climber=climber;
 }
}
// v0.6 (playtest note: "only 1 person being able to use it at a time is very time
// consuming"): the game's ladder manager holds one climber (header above), so a
// second climber still cannot start; a player who presses E on a ladder that is
// in use is queued and moved beside the climber as soon as that climb ends.
void ladder_use_attempt(Edict* brush,Edict* player){
 if(ladders.empty()||!live(player)||std::strcmp(classname(player),"player")||!engine[80])return;
 for(auto& l:ladders){if(!l.climber)continue;
  if(std::none_of(l.uses.begin(),l.uses.end(),[&](const LadderUse& u){return u.brush==brush&&serial(brush)==u.serial;}))continue;
  if(eng<Edict*(__cdecl*)(int)>(72)(l.climber)==player)return;
  if(std::any_of(l.waiting.begin(),l.waiting.end(),[&](const std::pair<Edict*,int>& w){return w.first==player&&w.second==serial(player);}))return;
  l.waiting.push_back({player,serial(player)});eng<void(__cdecl*)(Edict*,int,const char*)>(80)(player,1,"Your partner is on this ladder.\nYou follow when they get off.\n");return;}
}
