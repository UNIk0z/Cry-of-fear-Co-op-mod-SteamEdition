// Included inside the adapter namespace. User report: screen post-processing
// (first enemy blur/zoom) reached only the player who triggered it. Native
// evidence (pinned hl.dll): cof_blur Use 100A5BB0 toggles ONLY the activator's
// player+0x1EBC; cof_camerazoom Use 100A5F30 and cof_greyfade Use 100A41D0
// message only the activator; cof_playerbreathetoggle Use 100A5EB0 writes only
// the activator's fields. (cof_blackandwhite already loops every client.)
// At activation each such entity keeps its class/keys under a private name and
// a native game_counter takes the authored name, so every authored trigger and
// timing still fires it. When its count changes the adapter uses the real
// effect entity once for EVERY connected player, through the public DLL Use.
// v0.3 (campaign audit of the decomp's first-player lookups): cof_lookat (s4_village3,
// turns only the first player's view) and cof_stopwheelchair (s4_ending4b, only
// the first player leaves the wheelchair) use the same fan-out; during each
// player's Use, "the player" (the first-player lookup) is that player.
// cof_phonedisable (s4_subway1st1, the phone battery dies) is NOT fanned out:
// the wiki and the maps have ONE replacement battery, so a second dead phone
// could never come back.
// v0.5 (playtest: "when my camera got locked onto the house, caixas camera also
// got locked onto the house", s4_village3): cof_lookat turns the view of the
// player who walked into its trigger only, as in single player. That player is
// the last one a trigger touched (DispatchTouch) up to half a second before the
// counter moved; with no such touch, the player nearest the effect entity.
struct PostProcess {Edict* counter=nullptr;int counterSerial=-1;Edict* actor=nullptr;int actorSerial=-1;float last=0;std::string name;bool own=false;};
std::vector<PostProcess> postProcesses;
Edict* lastToucher=nullptr;int lastToucherSerial=-1;float lastTouchTime=-100.f;
void pp_touch(Edict* trigger,Edict* other){
 if(!globals||!live(other)||std::strcmp(classname(other),"player")||std::strncmp(classname(trigger),"trigger_",8))return;
 lastToucher=other;lastToucherSerial=serial(other);lastTouchTime=load<float>(globals,0);
}
Edict* pp_activator(const PostProcess& pp){
 const float now=load<float>(globals,0);
 if(live(lastToucher)&&serial(lastToucher)==lastToucherSerial&&now>=lastTouchTime&&now-lastTouchTime<=0.5f)return lastToucher;
 auto a=vars(pp.actor);Edict* best=nullptr;float d0=0;if(!a)return nullptr;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);auto v=vars(e);if(!v||std::strcmp(classname(e),"player"))continue;
  float d=0;for(int k=0;k<3;++k){const float x=load<float>(v,8+k*4)-load<float>(a,8+k*4);d+=x*x;}if(!best||d<d0){best=e;d0=d;}}
 return best;
}
// v0.5.7 (playtest 2 clip on s4_ending4b: "other player can just walk here";
// log: "POSTPROCESS wmode applied to 1 player uses" while player 2 was still
// loading the map): the wheelchair was given only to the players already in
// the game. The mode now stays on for the map (until cof_stopwheelchair), and
// each player who arrives later gets their own chair once their HUD is ready
// and their carried inventory has been applied (the chair empties the hands).
bool carry_settled(Edict* e);   // coop-runtime.hpp
struct WheelchairOn {Edict* mode=nullptr;int serial=-1;std::set<std::pair<Edict*,int>> riders;};
WheelchairOn wheelchairOn;
void pp_reset(){postProcesses.clear();wheelchairOn={};}
// v0.5 (C3, s4_ending4b): cof_wheelchairmode (decomp effects.cpp) starts the
// wheelchair for its activator only and puts that wheelchair on its "message"
// point. With the fan-out every player gets one; the second, third... wheelchair
// (player+0x390 m_pWheelchair, EHANDLE) is moved 48 units aside each, to a clear
// spot, so the chairs do not stand inside each other.
void wheelchair_aside(Edict* player,Edict* mode,int k){
 void* p=nullptr;std::size_t n=0;auto mv=vars(mode);if(!player_memory(player,p,n)||!mv)return;
 auto chair=resolve(load<std::uint32_t>(p,0x390),load<int>(p,0x394));auto cv=vars(chair);if(!cv)return;
 const float yaw=load<float>(mv,0x54)*0.01745329f;const std::array<float,3> want{load<float>(cv,8)-std::sin(yaw)*48.f*k,load<float>(cv,12)+std::cos(yaw)*48.f*k,load<float>(cv,16)};
 std::array<float,3> at{};if(!find_landing(want,chair,at))return;eng<void(__cdecl*)(Edict*,const float*)>(28)(chair,at.data());
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] WHEELCHAIR player=%d gets a wheelchair beside the first\n",eng<int(__cdecl*)(Edict*)>(71)(player));log(line);
}
// v0.5 (F3, rain for every player): a named rain_modify (rain or snow switched
// on or off by a trigger: s4_city, s4_city2, s4_hamn*, s4_park*, s4_stad,
// s4_start2, s4_trainride) returns at once while deathmatch is set ("Rain error:
// only static rain in multiplayer", decomp effects.cpp CRainModify::Use), and in
// single player it writes player 1 only. Constant rain (no name) already reaches
// every player at spawn. The fan-out's counter now writes the same fields for
// EVERY player: CRainModify Rain_Drips/windX/windY/randX/randY/fadeTime at
// +0xDC..+0xF0, player Rain_dripsPerSecond..Rain_nextFadeUpdate at
// +0x27DC..+0x2808 (pinned save tables) and Rain_needsUpdate +0x280C (the int
// after them in player.h; hl.dll sets it with "mov [reg+280Ch],1").
void rain_apply(Edict* modify){
 if(!live(modify)||std::strcmp(classname(modify),"rain_modify")||!engine[64]||!globals)return;
 void* mp=eng<void*(__cdecl*)(Edict*)>(64)(modify);if(!mp||load<void*>(mp,4)!=vars(modify))return;
 const int drips=load<int>(mp,0xdc);const float wx=load<float>(mp,0xe0),wy=load<float>(mp,0xe4),rx=load<float>(mp,0xe8),ry=load<float>(mp,0xec),fade=load<float>(mp,0xf0),now=load<float>(globals,0);
 int players=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!live(e)||std::strcmp(classname(e),"player")||!player_memory(e,p,n)||n<0x2810)continue;
  if(fade>0){store(p,0x27f0,drips);store(p,0x27f4,wx);store(p,0x27f8,wy);store(p,0x27fc,rx);store(p,0x2800,ry);store(p,0x2804,now+fade);store(p,0x2808,now+1.f);}
  else {store(p,0x27dc,drips);store(p,0x27e0,wx);store(p,0x27e4,wy);store(p,0x27e8,rx);store(p,0x27ec,ry);store(p,0x280c,1);}
  ++players;}
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] RAIN %d drops/s%s for %d player(s)\n",drips,fade>0?" (fading in)":"",players);log(line);
}
void pp_activate(){
 pp_reset();if(!scoped()||!engine[23]||!engine[67]||!original[1]||!original[3]||!original[6])return;
 std::vector<Edict*> actors;
 for(const char* type:{"cof_blur","cof_camerazoom","cof_greyfade","cof_playerbreathetoggle","cof_lookat","cof_stopwheelchair","cof_wheelchairmode","rain_modify"}){
  Edict* e=nullptr;for(int n=0;n<256;++n){e=find_next(e,"classname",type);if(!e)break;if(live(e)&&*targetname(e))actors.push_back(e);}
 }
 for(auto actor:actors){
  // v0.3 audit: a revisited map's restored effect entity carries the private
  // name it had when the map was left ("X__sc4pp"); the counter must take the
  // authored name X again, or nothing the map fires reaches the effect.
  std::string name=targetname(actor);while(name.size()>7&&!name.compare(name.size()-7,7,"__sc4pp"))name.resize(name.size()-7);auto av=vars(actor);if(!av)continue;
  const int id=eng<int(__cdecl*)(const char*)>(67)("game_counter");auto counter=eng<Edict*(__cdecl*)(int)>(23)(id);if(!live(counter))continue;
  bool okay=true;
  for(const auto& [k,value]:std::initializer_list<std::pair<const char*,const char*>>{{"targetname",name.c_str()},{"health","1000000000"},{"frags","0"}}){
   KeyValue data{const_cast<char*>("game_counter"),const_cast<char*>(k),const_cast<char*>(value),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(counter,&data);okay=okay&&data.handled!=0;
  }
  if(!okay||native<int(__cdecl*)(Edict*)>(1)(counter)!=0){if(live(counter)){assist_unlink(counter);eng<void(__cdecl*)(Edict*)>(22)(counter);}continue;}
  // Only now take the authored name away from the effect entity.
  store(av,0x1d0,eng<int(__cdecl*)(const char*)>(67)((name+"__sc4pp").c_str()));
  PostProcess pp;pp.counter=counter;pp.counterSerial=serial(counter);pp.actor=actor;pp.actorSerial=serial(actor);pp.last=load<float>(vars(counter),0x168);pp.name=name;pp.own=!std::strcmp(classname(actor),"cof_lookat");postProcesses.push_back(pp);
 }
 if(!postProcesses.empty()){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] POSTPROCESS fan-out active for %u effects\n",static_cast<unsigned>(postProcesses.size()));log(line);}
}
void pp_frame(){
 if(postProcesses.empty()||!globals)return;
 for(auto& pp:postProcesses){
  if(serial(pp.counter)!=pp.counterSerial||serial(pp.actor)!=pp.actorSerial)continue;
  // CoF entvars frags is +0x168 (health +0x164 is pinned; frags follows it).
  const float now=load<float>(vars(pp.counter),0x168);if(now==pp.last)continue;
  const int uses=(std::min)(8,static_cast<int>(std::fabs(now-pp.last)+0.5f));pp.last=now;
  if(!std::strcmp(classname(pp.actor),"rain_modify")){if(serial(pp.actor)==pp.actorSerial)rain_apply(pp.actor);continue;}
  int players=0;Edict* only=pp.own?pp_activator(pp):nullptr;
  for(int u=0;u<uses;++u)for(int i=1;i<=load<int>(globals,0x90);++i){
   auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!live(e)||std::strcmp(classname(e),"player")||(only&&e!=only))continue;
   if(serial(pp.actor)!=pp.actorSerial)break;
   actingPlayer=e;native<void(__cdecl*)(Edict*,Edict*)>(3)(pp.actor,e);actingPlayer=nullptr;
   if(!std::strcmp(classname(pp.actor),"cof_wheelchairmode")){if(players>0)wheelchair_aside(e,pp.actor,players);wheelchairOn.riders.insert({e,serial(e)});}
   ++players;
  }
  if(!std::strcmp(classname(pp.actor),"cof_wheelchairmode")){wheelchairOn.mode=pp.actor;wheelchairOn.serial=pp.actorSerial;}
  if(!std::strcmp(classname(pp.actor),"cof_stopwheelchair"))wheelchairOn={};
  char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] POSTPROCESS %s applied to %d player uses\n",pp.name.c_str(),players);log(line);
 }
 // Late arrivals get their chair too (one Use each, beside the others).
 if(!live(wheelchairOn.mode)||serial(wheelchairOn.mode)!=wheelchairOn.serial)return;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!live(e)||std::strcmp(classname(e),"player")||!client_ready(e)||!carry_settled(e)||!view_alive(e)||wheelchairOn.riders.count({e,serial(e)}))continue;
  const int k=static_cast<int>(wheelchairOn.riders.size());wheelchairOn.riders.insert({e,serial(e)});
  actingPlayer=e;native<void(__cdecl*)(Edict*,Edict*)>(3)(wheelchairOn.mode,e);actingPlayer=nullptr;if(k>0)wheelchair_aside(e,wheelchairOn.mode,k);
  char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] WHEELCHAIR player=%d arrived later: wheelchair given\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
 }
}
