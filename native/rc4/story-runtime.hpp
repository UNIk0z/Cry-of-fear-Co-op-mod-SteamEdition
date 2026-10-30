// Included in adapter.cpp after session-runtime.hpp (coop-fixes-006).
// User requests 2026-10-12: dual wield in co-op; while dual wielding the
// player model shows the weapon hand's animation (knife + phone looks like
// the knife); the ending choices are kept so the right ending plays.

// --- Dual wield ------------------------------------------------------------------
// The inventory's dual wield sends "inventorydualwield A B" in multiplayer too
// (client 10043D3B has no multiplayer check). The server handler (1001B6B9)
// returns when g_pGameRules->IsMultiplayer() (1001B6DB) and otherwise calls
// CBasePlayer::DualWield(A,B) (100DE990). DualWield, the weapon_dualweapon it
// gives (100E08B0, item id 15) and that weapon's code have no multiplayer
// branches, and the two component weapons stay owned, so the native co-op
// level change (1002A800) still carries them by inventory card. In co-op the
// adapter makes the same call for the player who sent the command. This is
// the project's only direct call into the game's code, made at the user's
// explicit request (2026-10-12); the backend is the SHA256-pinned hl.dll, so
// the address cannot drift. DualWield reads inventorySlot[A] (+0x21FC) before
// its own 1..6 check, so both slots must be single digits 1..6 first.
using DualWieldCall=void(__thiscall*)(void*,int,int);
DualWieldCall dualWieldTarget=nullptr; // unit fixtures substitute the call
int dualLines=0;
DualWieldCall dual_target(){if(dualWieldTarget)return dualWieldTarget;return verified&&backend?reinterpret_cast<DualWieldCall>(reinterpret_cast<unsigned char*>(backend)+0xDE990):nullptr;}
// CBasePlayerItem::ItemInfoArray (hl.dll 10231888, 0x2C per id; iFlags +0x24
// is read at 102318AC and iWeight +0x28 at 102318B0): weight of id 15.
constexpr std::uint32_t DualWeightRva=0x231888+15*0x2c+0x28;
int* dualWeightOverride=nullptr; // unit fixtures substitute the table
int* dual_weight(){if(dualWeightOverride)return dualWeightOverride;return verified&&backend?reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(backend)+DualWeightRva):nullptr;}
int inventory_slot(const char* s){return s&&s[0]>='1'&&s[0]<='6'&&!s[1]?s[0]-'0':0;}
bool dual_command(Edict* e,const char* command){
 if(!scoped()||!command||std::strcmp(command,"inventorydualwield"))return false;
 const int a=inventory_slot(eng<const char*(__cdecl*)(int)>(83)(1)),b=inventory_slot(eng<const char*(__cdecl*)(int)>(83)(2));
 void* p=nullptr;std::size_t n=0;auto v=vars(e);const auto call=dual_target();
 const bool alive=v&&load<int>(v,0x174)==0&&load<float>(v,0x164)>0;
 char line[160]{};const int index=live(e)?eng<int(__cdecl*)(Edict*)>(71)(e):-1;
 if(!a||!b||!call||!player_memory(e,p,n)||!alive)std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DUAL refused player=%d slots=%d,%d\n",index,a,b);
 else {
 // coop-fixes-008: the 007 test logged DUAL but nobody held two weapons.
 // DualWield gives weapon_dualweapon through AddPlayerItem, and the
 // multiplayer FShouldSwitchWeapon (100C8740) only switches to a new item
 // that weighs more than the held one (single player 100FC1F0 always
 // switches). The dual weapon's ItemInfo weight is raised for this call only.
  int* weight=dual_weight();const int savedWeight=weight?*weight:0;if(weight)*weight=1000000;
  call(p,a,b);if(weight)*weight=savedWeight;
  Edict* held=nullptr;void* heldMemory=nullptr;const auto bits=load<std::uint32_t>(p,0x252c);const bool dual=bits&&item_object(bits,held,heldMemory)&&std::strcmp(classname(held),"weapon_dualweapon")==0;
  std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DUAL player=%d slots=%d,%d hands=%d,%d holding=%s\n",index,a,b,load<int>(p,0x1e70),load<int>(p,0x1e74),dual?"dual":"other");}
 if(dualLines<100){++dualLines;log(line);}
 return true;
}

// v0.5: the same call for a restored dual wield (the two weapons' inventory slots).
int weapon_card_slot(void* p,const std::string& weapon);
bool dual_rebuild(Edict* e,void* p,const std::string& a,const std::string& b){
 const int sa=weapon_card_slot(p,a),sb=weapon_card_slot(p,b);const auto call=dual_target();auto v=vars(e);
 if(!sa||!sb||sa==sb||!call||!v||load<int>(v,0x174)!=0||load<float>(v,0x164)<=0)return false;
 int* weight=dual_weight();const int savedWeight=weight?*weight:0;if(weight)*weight=1000000;call(p,sa,sb);if(weight)*weight=savedWeight;return true;
}

// v0.5 (D5, "quick dual wield using two bound hotkeys"): in single player two
// quick slots pressed within 0.25 s dual wield their two weapons (decomp
// player.cpp QuickSelectItem -> CheckQuickDualWield, newer slot first); both
// return in multiplayer. After the native "quicksel N" (which equips slot N) the
// adapter makes the same DualWield call for the two quick-slot weapons, which
// checks light pairs and weapons that cannot be dual wielded itself.
// m_sQuickSel1..3 (char[128]) are at 0x204C/0x20CC/0x214C in the pinned hl.dll
// save table.
// v0.5.4 (Katz 2026-10-23: "keybind 3 not working (quick slot 3)"; log: pressing 3
// right after 2 to switch weapons made "DUAL quick ... keys 2+3" again and again):
// the keys must now come within 0.12 s, a press together rather than a quick switch.
constexpr float QuickDualWindow=0.12f;
constexpr std::size_t QuickSel1=0x204c;
struct QuickPress {int serial=-1,number=0;float at=-100;};
std::unordered_map<Edict*,QuickPress> quickPresses;
void quick_dual_command(Edict* e,const char* command,const std::string& arg){
 if(!scoped()||!command||std::strcmp(command,"quicksel")||!globals||!live(e))return;
 const int number=arg.size()==1&&arg[0]>='1'&&arg[0]<='3'?arg[0]-'0':0;if(!number)return;
 auto& q=quickPresses[e];const float now=load<float>(globals,0);
 const bool pair=q.serial==serial(e)&&q.number&&q.number!=number&&now>=q.at&&now-q.at<QuickDualWindow;const int last=q.number;q={serial(e),number,now};
 void* p=nullptr;std::size_t n=0;if(!pair||!player_memory(e,p,n)||n<QuickSel1+3*0x80)return;
 auto sel=[&](int k){const char* s=static_cast<const char*>(p)+QuickSel1+(k-1)*0x80;return std::string(s,strnlen(s,127));};
 // v0.5.7 (playtest clip "idk why it went in my left hand": "DUAL quick keys 1+2
 // weapon_mobile+weapon_switchblade asked"): the phone is never dual wielded.
 const auto a=sel(number),b=sel(last);if(a.rfind("weapon_",0)||b.rfind("weapon_",0)||a=="weapon_mobile"||b=="weapon_mobile")return;
 const bool done=dual_rebuild(e,p,a,b);if(done)q.number=0;
 if(dualLines<100){++dualLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DUAL quick player=%d keys %d+%d %s+%s %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),last,number,a.c_str(),b.c_str(),done?"asked":"refused");log(line);}
}

// --- Third-person animation while dual wielding --------------------------------------
// The upper-body sequence comes from the active item's id (100E03A0, table
// 100E04B4) and weapon_dualweapon is id 15, the empty-handed "idle", so a
// dual wielder looked unarmed to the partner and never attacked. After the
// native PostThink the adapter shows the weapon hand's own idle (the weapon,
// not the light; with two weapons the right hand, +0x1E74) and plays that
// weapon's attack when the dual weapon attacks (a fresh attack press that
// moved its next-attack time, +0x1D0/+0x1D4). The idle is re-applied every
// frame from an unchanged start time, so it plays smoothly against the
// native reset without holding the animation lock (+0x4F4), which also gates
// revive (100EFD25). coop-fixes-009: animtime must still be "now" with the
// frame advanced to match (as the native StudioFrameAdvance leaves them): the
// 008 test showed a dual wielder standing still for the partner and jumping
// only on an attack or a weapon change, because the client takes a new
// position for a studio entity only when its animtime changes. An attack sets that lock exactly as the native PlayAnim
// (100EA240) does. Sequences are found by name in the player's model file.
const char* weapon_idle(int id){
 static const char* const names[26]={"radio_idle","pistol_idle","shotgun_idle",nullptr,"switchblade_idle","pistol_idle","syringe_idle","m16_idle","branch_idle","rifle_idle","nightstick_idle",nullptr,"tmp_idle",nullptr,"branch_idle",nullptr,"sledgehammer_idle","vp70_idle",nullptr,"pistol_idle","radio_idle",nullptr,nullptr,nullptr,"m16_idle","mp5_idle"};
 return id>=2&&id<=27?names[id-2]:nullptr;
}
// Light sources (100E4BF0, table 100E4C20): ids 1, 2 (phone), 10 and 13.
bool light_item(int id){return id==1||id==2||id==10||id==13;}
int pose_item(int left,int right){
 if(weapon_idle(right)&&!light_item(right))return right;
 if(weapon_idle(left)&&!light_item(left))return left;
 return weapon_idle(right)?right:weapon_idle(left)?left:0;
}
struct SequenceInfo {std::string name;float fps=0,move[3]{};int flags=0,frames=0;};
struct ModelSequences {
 bool tried=false,valid=false;std::vector<SequenceInfo> list;
 int find(const std::string& name)const{for(std::size_t i=0;i<list.size();++i)if(list[i].name==name)return static_cast<int>(i);return -1;}
};
std::unordered_map<std::string,ModelSequences> modelSequences;
bool model_path(const char* s){
 if(!s||std::strncmp(s,"models/",7))return false;const std::string m(s);
 return m.size()<128&&m.size()>11&&m.compare(m.size()-4,4,".mdl")==0&&m.find("..")==m.npos&&std::all_of(m.begin(),m.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='/'||c=='.'||c=='-';});
}
// Studio header: "IDST", version 10, numseq +164, seqindex +168; each
// sequence is 176 bytes: label[32], fps +32, flags +36, numframes +56,
// linearmovement +76. Read once per model path, bounds-checked.
const ModelSequences& model_sequences(const char* model){
 static const ModelSequences none{};if(!model_path(model))return none;
 auto& m=modelSequences[model];if(m.tried)return m;m.tried=true;
 try{
  char dir[MAX_PATH]{};if(!engine[99])return m;eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return m;
  std::ifstream file(std::filesystem::path(dir)/model,std::ios::binary|std::ios::ate);if(!file)return m;
  const auto size=static_cast<std::streamoff>(file.tellg());if(size<176||size>(64<<20))return m;
  std::vector<unsigned char> b(static_cast<std::size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(b.data()),size);if(!file)return m;
  if(std::memcmp(b.data(),"IDST",4)||load<int>(b.data(),4)!=10)return m;
  const int count=load<int>(b.data(),164),at=load<int>(b.data(),168);
  if(count<1||count>2048||at<0||static_cast<std::size_t>(at)+static_cast<std::size_t>(count)*176>b.size())return m;
  for(int i=0;i<count;++i){const unsigned char* s=b.data()+at+i*176;SequenceInfo q;std::size_t n=0;while(n<32&&s[n])++n;q.name.assign(reinterpret_cast<const char*>(s),n);
   q.fps=load<float>(s,32);q.flags=load<int>(s,36);q.frames=load<int>(s,56);for(int k=0;k<3;++k)q.move[k]=load<float>(s,76+k*4);
   if(!std::isfinite(q.fps)||q.fps<0||q.fps>1000||q.frames<0||q.frames>100000)return m;m.list.push_back(std::move(q));}
  m.valid=true;
 }catch(...){m.list.clear();m.valid=false;}
 return m;
}
int attack_sequence(const ModelSequences& m,const char* idle){
 std::string base(idle);if(base.size()<6||base.compare(base.size()-5,5,"_idle"))return -1;base.resize(base.size()-5);
 for(const char* suffix:{"_shoot","_attack1","_attack"}){const int i=m.find(base+suffix);if(i>=0)return i;}return -1;
}
// The fields CBaseAnimating::ResetSequenceInfo (1000E9E0) and GetSequenceInfo
// (1000F290) set: entvars sequence/frame/animtime/framerate and the private
// frame rate +0xEC, ground speed +0xF0, last event check +0xF4, finished
// +0xF8 and looping +0xFC.
float sequence_duration(const SequenceInfo& s){return s.fps>0&&s.frames>0?s.frames/s.fps:0.f;}
void play_sequence(void* p,void* v,const SequenceInfo& s,int index,float start,float now){
 const float span=s.frames>1?static_cast<float>(s.frames-1):0.f,rate=span>0?256.f*s.fps/span:256.f;
 float frame=(std::max)(0.f,(now-start)*rate);frame=s.flags&1?std::fmod(frame,256.f):(std::min)(frame,255.f);
 store(v,0x12c,index);store(v,0x134,frame);store(v,0x138,now);store(v,0x13c,1.f);
 store(p,0xec,rate);store(p,0xf0,span>0?std::sqrt(s.move[0]*s.move[0]+s.move[1]*s.move[1]+s.move[2]*s.move[2])*s.fps/span:0.f);
 store(p,0xf4,now);store(p,0xf8,0);store(p,0xfc,s.flags&1);
}
struct PoseState {int serial=-1,sequence=-1,attack=-1;float since=0,attackUntil=0,primary=0,secondary=0;std::uint32_t item=0;};
std::unordered_map<Edict*,PoseState> poses;int poseLines=0;
void pose_reset(){poses.clear();poseLines=0;}
void pose_log(const char* line){if(poseLines<100){++poseLines;log(line);}}
void pose_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;auto v=vars(e);if(!scoped()||!globals||!v||!player_memory(e,p,n))return;
 auto& s=poses[e];if(s.serial!=serial(e)){s={};s.serial=serial(e);}
 Edict* item=nullptr;void* weapon=nullptr;const auto bits=load<std::uint32_t>(p,0x252c);
 if(!bits||!item_object(bits,item,weapon)||std::strcmp(classname(item),"weapon_dualweapon")||load<int>(v,0x174)!=0){s.item=0;s.sequence=-1;return;}
 const auto& m=model_sequences(string_at(load<int>(v,0xb8)));if(!m.valid)return;
 const int hand=pose_item(load<int>(p,0x1e70),load<int>(p,0x1e74));const char* idleName=weapon_idle(hand);
 const int idle=idleName?m.find(idleName):-1,generic=m.find("idle");if(idle<0||generic<0)return;
 const float now=load<float>(globals,0),primary=load<float>(weapon,0x1d0),secondary=load<float>(weapon,0x1d4);
 const bool attacked=s.item==bits&&(load<unsigned>(p,0x22b0)&(1u|0x800u))&&(primary>s.primary+0.01f||secondary>s.secondary+0.01f);
 s.item=bits;s.primary=primary;s.secondary=secondary;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e),current=load<int>(v,0x12c);
 if(attacked){const int attack=attack_sequence(m,idleName);if(attack>=0){
  const float duration=sequence_duration(m.list[attack]);play_sequence(p,v,m.list[attack],attack,now,now);store(p,0x4f4,now+duration);
  s.attack=attack;s.attackUntil=now+duration;s.sequence=-1;char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] POSE player=%d dual attack %s\n",index,m.list[attack].name.c_str());pose_log(line);return;}}
 if(current==s.attack&&now<s.attackUntil)return;
 // A native animation (revive, damage, ...) owns the body until it ends.
 if(current!=generic&&current!=idle&&current!=s.attack)return;
 if(s.sequence!=idle){s.sequence=idle;s.since=now;char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] POSE player=%d dual hands=%d,%d shows %s\n",index,load<int>(p,0x1e70),load<int>(p,0x1e74),idleName);pose_log(line);}
 play_sequence(p,v,m.list[idle],idle,s.since,now);
}

// --- Ending choices and story wounds across co-op map changes ------------------------
// endingcheck (the inter_door before the endings, 10048070) sends the door's
// user to ending_exit1..5 by that player's +0x4CD roof boss killed, +0x4CE
// P345 given and +0x4CF package delivered. cof_ending (100A3C80; c_roofs2,
// c_asylum1 iuser1 1, c_hamn3 iuser1 2) sets them on every connected player.
// The native co-op level change (cof_changelevel 100A1F70 -> 1002A480)
// carries health, stamina, stats, quick slots and weapons but not these, so
// from the next map on every player was back on ending 1. One shared value:
// any flag seen on any player is kept for the session, written to every
// player each frame, saved in checkpoint schema 4, cleared for a new campaign.
// The same level change dropped cof_maxhealthchange's story wound (+0x60C,
// 0..2, activator only, HUD message HealthCap) and the night-vision pickup
// (+0x434); those are kept per player identity and restored once per map.
int endingShared=0;
// v0.5 (C1): the weapon given to the doctor (s4_asylum1 choice, cof_doctorweaponset
// on the FIRST player, player+0x5B0) decides the s4_attic fight: CDoctorBoss waits
// for the first player's value before it fights and cof_doctorweapontrigger
// chooses the scene by it (decomp CoF_DoctorBoss.cpp, effects.cpp). The co-op
// level change rebuilt the players without it, so the doctor never fought. Shared
// like the ending flags: any value seen is kept, written to every player, saved.
int doctorShared=0;
struct StoryPlayer {int healthCap=0;bool nightVision=false;};
std::unordered_map<std::string,StoryPlayer> storyBank;
std::unordered_map<Edict*,int> storyApplied;
// v0.7 (docs/V07_TODO.md 5, Katz 2026-10-27): the Doctor's shot lowers the
// health cap of EVERY player. cof_maxhealthchange (decomp effects.cpp 9009) sets
// m_iHealthCap (+0x60C, 0..2; impulse 0 clears it) and the HealthCap HUD
// message on its activator only. Any change the game makes to one player's cap
// becomes the shared cap; every player, players who join later and every
// level change get it, and checkpoints save it, until the story clears it.
int capShared=0;std::unordered_map<Edict*,std::pair<int,int>> capSeen;   // serial, last value seen
void story_reset(){endingShared=0;doctorShared=0;capShared=0;storyBank.clear();storyApplied.clear();capSeen.clear();}
void story_map_changed(){storyApplied.clear();capSeen.clear();}
int ending_bits(void* p){return (load<unsigned char>(p,0x4cd)?1:0)|(load<unsigned char>(p,0x4ce)?2:0)|(load<unsigned char>(p,0x4cf)?4:0);}
int health_cap(void* p){const int cap=load<int>(p,0x60c);return cap>=0&&cap<=2?cap:0;}
void send_health_cap(Edict* e,int cap){
 if(healthCapMessage<=0||!engine[46]||!engine[47]||!engine[48]||!live(e))return;
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,healthCapMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(cap);eng<void(__cdecl*)()>(47)();
}
void story_adopt(){
 if(!scoped()||!globals)return;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  const int doctor=load<int>(p,0x5b0);if(doctor>=1&&doctor<=2&&doctor!=doctorShared){doctorShared=doctor;char d[120]{};std::snprintf(d,sizeof(d),"[STORYCOOP_RC4] DOCTOR weapon=%d (1 P345, 2 revolver) from player=%d, kept for everyone\n",doctor,i);log(d);}
  const int fresh=ending_bits(p)&~endingShared;if(!fresh)continue;endingShared|=fresh;
  char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] ENDING flags=%d (roof boss %d, P345 given %d, package %d) from player=%d\n",endingShared,endingShared&1,(endingShared>>1)&1,(endingShared>>2)&1,i);log(line);
 }
}
void story_frame(){
 if(!scoped()||!globals)return;story_adopt();
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  for(std::size_t bit=0;bit<3;++bit)if((endingShared>>bit)&1&&!load<unsigned char>(p,0x4cd+bit))store<unsigned char>(p,0x4cd+bit,1);
  if(doctorShared&&load<int>(p,0x5b0)!=doctorShared)store(p,0x5b0,doctorShared);
  // Wounds/night vision: restore the identity's record once per map when
  // the HUD is ready, then follow the native value.
  const auto identity=player_identity(e);if(identity.empty())continue;
  auto applied=storyApplied.find(e);
  if(applied==storyApplied.end()||applied->second!=serial(e)){
   if(!client_ready(e))continue;storyApplied[e]=serial(e);
   const auto saved=storyBank.find(identity);
   if(saved!=storyBank.end()){
    bool changed=false;
    if(saved->second.nightVision&&!load<unsigned char>(p,0x434)){store<unsigned char>(p,0x434,1);changed=true;}
    if(changed){char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] STORY player=%d kept health-cap=%d night-vision=%d\n",i,health_cap(p),load<unsigned char>(p,0x434)?1:0);log(line);}
   }
  }
  storyBank[identity]={health_cap(p),load<unsigned char>(p,0x434)!=0};
  auto& seen=capSeen[e];const int cap=health_cap(p);
  if(seen.first!=serial(e))seen={serial(e),cap};   // first look on this map: the rebuilt value is not a story change
  else if(cap!=seen.second){seen.second=cap;if(cap!=capShared){capShared=cap;char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] STORY health cap %d from player=%d, now every player's\n",cap,i);log(line);}}
 }
 for(auto& [e,seen]:capSeen){void* p=nullptr;std::size_t n=0;if(seen.first!=serial(e)||!player_memory(e,p,n)||health_cap(p)==capShared)continue;
  store(p,0x60c,capShared);send_health_cap(e,capShared);seen.second=capShared;}
}
void story_capture(Checkpoint& value){story_adopt();value.endingFlags=endingShared;value.doctorWeapon=doctorShared;}
void story_load(const Checkpoint& value){
 endingShared=value.endingFlags;doctorShared=value.doctorWeapon;storyBank.clear();storyApplied.clear();
 capShared=0;capSeen.clear();for(const auto& p:value.players){storyBank[p.identity]={p.healthCap,p.nightVision};capShared=(std::max)(capShared,p.healthCap);}
}

// --- Coming back after a kick (coop-fixes-007) -----------------------------------
// User report: P2 was kicked ("reliable overflow") while joining s4_start and
// came back with an empty inventory. A player who joins in the middle of a
// map gets neither the native level-change carry nor the wake-up scene's
// phone and knife: those are fan-outs (game_zone_player -> sc3_native_*
// cof_giveitems/cof_clearitems) that reach only the players present when they
// fire. Now a leaving player is remembered by identity (their own state if
// they had played this map, otherwise what they carried into it), restored
// without a teleport when they come back, and fan-outs that fired before
// their player entity existed are replayed: give only what is missing, clear
// only what is there.
struct Fanout {Edict* entity=nullptr;int serial=0;float time=0;bool give=true;std::string item;};
std::vector<Fanout> fanouts;
std::unordered_map<std::string,SavedPlayer> departedPlayers;
std::unordered_map<Edict*,unsigned> rejoinDone;   // player edict -> allocation stamp handled
int rejoinLines=0;
void rejoin_reset(){fanouts.clear();departedPlayers.clear();rejoinDone.clear();rejoinLines=0;}
// A pose belongs to the map it was taken on.
void rejoin_map_changed(){fanouts.clear();rejoinDone.clear();for(auto& d:departedPlayers)d.second.hasPose=false;}
void rejoin_log(const char* line){if(rejoinLines<100){++rejoinLines;log(line);}}
void fanout_lookup(Edict* start,const char* key,const char* value,Edict* found){
 if(start||!scoped()||!globals||!key||!value||std::strcmp(key,"targetname")||std::strncmp(value,"sc3_native_",11)||!live(found))return;
 const char* type=classname(found);const bool give=!std::strcmp(type,"cof_giveitems"),clear=!std::strcmp(type,"cof_clearitems");if(!give&&!clear)return;
 for(const auto& f:fanouts)if(f.entity==found&&f.serial==serial(found))return;
 auto v=vars(found);const char* item=v?string_at(load<int>(v,0x1d8)):nullptr;   // pev->message (save table 0x1D8)
 if(!item||!*item||std::strlen(item)>64||fanouts.size()>=64)return;
 fanouts.push_back({found,serial(found),load<float>(globals,0),give,item});
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REJOIN fan-out %s %s recorded at %.1f\n",give?"give":"clear",item,load<float>(globals,0));rejoin_log(line);
}
// Called from ClientDisconnect before the native callback.
void rejoin_departing(Edict* e){
 if(!scoped()||checkpointRestoreFailed||!live(e))return;
 // A checkpoint restore still pending for this player keeps priority.
 if(loadingCheckpoint){auto restored=restoredPlayers.find(e);if(restored==restoredPlayers.end()||restored->second!=serial(e))return;}
 const auto identity=player_identity(e);if(identity.empty())return;
 // v0.3 (user log rc4-20261016-141912): the clients drop and reconnect at a
 // level change while the players are frozen, so the strict capture failed and
 // the LAST SAVE's record was kept: the tape, a new weapon and health were
 // reverted at the next door. Now: their own state (frozen allowed), else the
 // record the level-change carry just read, and only then the save's record.
 SavedPlayer p;bool played=client_ready(e)&&(capture_player(e,p)||capture_player(e,p,false));const char* kept=played?"their state":nullptr;
 if(!played){if(const SavedPlayer* carried=carry_record(e)){p=*carried;kept="their level-change record";}}
 if(!kept){auto it=std::find_if(playerBank.begin(),playerBank.end(),[&](const SavedPlayer& s){return s.identity==identity;});if(it==playerBank.end())return;p=*it;kept="the last save's record";}
 if(!played)p.hasPose=false;
 // coop-fixes-008 (user: a rejoining player was left at the map's spawn, a
 // closed box): remember where they stood to land them there again.
 else if(auto v=vars(e)){p.hasPose=true;p.origin={load<float>(v,8),load<float>(v,12),load<float>(v,16)};p.angles={load<float>(v,0x50),load<float>(v,0x54),load<float>(v,0x58)};}
 departedPlayers[identity]=p;
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REJOIN player=%d left; kept %s (weapons=%u)\n",eng<int(__cdecl*)(Edict*)>(71)(e),kept,static_cast<unsigned>(p.weapons.size()));rejoin_log(line);
}
bool owns_weapon(Edict* e,const std::string& name){std::vector<std::pair<Edict*,void*>> list;if(!weapon_list(e,list))return true;for(const auto& w:list)if(name==classname(w.first))return true;return false;}
void rejoin_frame(Edict* e){
 if(!scoped()||!globals||checkpointRestoreFailed||!live(e)||std::strcmp(classname(e),"player"))return;
 auto a=allocations.find(e);if(a==allocations.end())return;
 auto done=rejoinDone.find(e);if(done!=rejoinDone.end()&&done->second==a->second.stamp)return;
 if(loadingCheckpoint){auto restored=restoredPlayers.find(e);if(restored==restoredPlayers.end()||restored->second!=serial(e))return;}
 const auto identity=player_identity(e);SavedPlayer current;
 if(identity.empty()||!client_ready(e)||!capture_player(e,current))return;
 rejoinDone[e]=a->second.stamp;const int index=eng<int(__cdecl*)(Edict*)>(71)(e);const float born=a->second.born;
 auto departed=departedPlayers.find(identity);
 if(departed!=departedPlayers.end()){
  const SavedPlayer saved=departed->second;departedPlayers.erase(departed);
  char line[200]{};
  if(restore_player(e,saved,saved.hasPose,true))std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REJOIN player=%d inventory restored (weapons=%u)%s\n",index,static_cast<unsigned>(saved.weapons.size()),saved.hasPose?", back where they left":"");
  else std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REJOIN player=%d restore incomplete: %.120s\n",index,checkpointFailure.c_str());
  rejoin_log(line);
 }
 for(const auto& f:fanouts){
  if(born<=f.time||!live(f.entity)||serial(f.entity)!=f.serial||!original[3])continue;
  const bool has=owns_weapon(e,f.item);if(f.give==has)continue;
  native<void(__cdecl*)(Edict*,Edict*)>(3)(f.entity,e);
  char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REJOIN player=%d missed fan-out: %s %s\n",index,f.give?"gave":"cleared",f.item.c_str());rejoin_log(line);
 }
}

// --- Credits (cof_logo) on every screen (coop-fixes-007) ---------------------------
// User report: a credit name stayed on P2's screen. The client's CoFLogo
// handler (client 10039560) builds a new label for every message and forgets
// the previous one without hiding it, so a label still showing (3 s + fade)
// when the next CoFLogo arrives stays on screen for good. Each player now gets
// at most one CoFLogo per 4 s: a later one waits, a repeat is dropped.
struct LogoState {float lastSent=-100;int lastValue=-1;std::vector<int> queue;};
std::unordered_map<Edict*,LogoState> logos;
struct LogoCapture {bool active=false;Edict* to=nullptr;int value=-1,writes=0;} logoCapture;
int logoLines=0;
void logo_reset(){logos.clear();logoCapture={};logoLines=0;}
void logo_line(const char* format,int player,int value,float at){if(logoLines>=60)return;++logoLines;char line[120]{};std::snprintf(line,sizeof(line),format,player,value,at);log(line);}
void logo_send(Edict* e,int value){
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,cofLogoMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(value);eng<void(__cdecl*)()>(47)();
 auto& s=logos[e];s.lastSent=load<float>(globals,0);s.lastValue=value;
 logo_line("[STORYCOOP_RC4] CREDIT player=%d logo=%d at %.1f\n",eng<int(__cdecl*)(Edict*)>(71)(e),value,s.lastSent);
}
bool logo_begin(int dest,int type,Edict* e){
 logoCapture={};if(cofLogoMessage<=0||type!=cofLogoMessage||dest!=1||!globals||!campaign_map()||!live(e))return false;
 logoCapture.active=true;logoCapture.to=e;return true;
}
bool logo_write(int value){if(!logoCapture.active)return false;logoCapture.value=value;++logoCapture.writes;return true;}
void logo_queue(Edict* to,int value){
 auto& s=logos[to];const float now=load<float>(globals,0);
 if(now<s.lastSent)s.lastSent=now-4.f;   // a new server time base
 const bool repeat=(value==s.lastValue&&now-s.lastSent<4.f)||std::find(s.queue.begin(),s.queue.end(),value)!=s.queue.end();
 if(repeat){logo_line("[STORYCOOP_RC4] CREDIT player=%d logo=%d repeat dropped at %.1f\n",eng<int(__cdecl*)(Edict*)>(71)(to),value,now);return;}
 if(s.queue.empty()&&now-s.lastSent>=4.f)logo_send(to,value);
 else if(s.queue.size()<8){s.queue.push_back(value);logo_line("[STORYCOOP_RC4] CREDIT player=%d logo=%d held (previous still showing) at %.1f\n",eng<int(__cdecl*)(Edict*)>(71)(to),value,now);}
}
// v0.5 (C4, "intro logos must display for everyone"): the 14 cof_logo of the
// opening (cof_campaign_story4, the only map with them) message their activator
// alone (decomp effects.cpp CCofLogo::Use, MSG_ONE), which cof_lobby_start makes
// the first player. On the opening maps every player now gets each logo.
bool logo_end(){
 if(!logoCapture.active)return false;const auto c=logoCapture;logoCapture={};
 if(c.writes!=1||c.value<0||c.value>255||!live(c.to))return true;
 logo_queue(c.to,c.value);
 if(opening())for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(e!=c.to&&live(e)&&!std::strcmp(classname(e),"player"))logo_queue(e,c.value);}
 return true;
}
void logo_frame(){
 if(!globals||logos.empty())return;const float now=load<float>(globals,0);
 for(auto it=logos.begin();it!=logos.end();){
  if(!live(it->first)){it=logos.erase(it);continue;}
  auto& s=it->second;if(now<s.lastSent)s.lastSent=now-4.f;
  if(!s.queue.empty()&&now-s.lastSent>=4.f){const int value=s.queue.front();s.queue.erase(s.queue.begin());logo_send(it->first,value);}
  ++it;
 }
}

// --- Phone light centre texture with two players (coop-fixes-008) ---------------
// User report: the lit centre texture of the phone/flashlight light sometimes
// vanished, and always when both players used the phone light. Client
// 100AEA10 draws the OWN light as a textured spot (gfx/flashlight.tga) under
// spot key -1 (bit 1; the phone glow, bit 2 without 4, under key -2). Client
// 10065260 then draws a plain, untextured spot for EVERY player whose
// scoreboard frags (ScoreInfo, frags = light flags, UpdateClientData 100EDB6E)
// has bit 1, 8 or 0x20, under key -(player index). Spot allocator 1005C0F0
// reuses a slot by key and clears it, so player 1's plain spot (the host's,
// and on the host its own duplicate) overwrote the textured one. The server
// now sends ScoreInfo to each player separately: a player's own entry carries
// no light bits (its textured spot is drawn anyway), and player 1's/2's light
// bits are withheld only from a player whose own spot uses key -1/-2 at that
// moment. Everything else in ScoreInfo is unchanged.
struct ScoreEntry {bool known=false;int frags=0,deaths=0,third=0,team=0;};
std::array<ScoreEntry,33> scoreTable{};std::map<std::pair<int,int>,std::array<int,4>> scoreSent;
struct ScoreCapture {bool active=false;int dest=0,to=0,bytes=0,shorts=0,index=0;std::array<int,4> values{};} scoreCapture;
int scoreInfoMessage=0,scoreLines=0;
void score_reset(){scoreTable={};scoreSent.clear();scoreCapture={};scoreLines=0;}
unsigned score_own_flags(Edict* e){void* p=nullptr;std::size_t n=0;return player_memory(e,p,n)?load<unsigned>(p,0x5d0)&0x3fu:0u;}
int score_seen(int viewer,unsigned own,int index,int frags){
 if(frags<0||!(frags&0x29))return frags;
 const bool hide=index==viewer||(index==1&&(own&1u))||(index==2&&(own&2u)&&!(own&4u));
 return hide?frags&~0x29:frags;
}
void score_sync(){
 if(scoreInfoMessage<=0||!globals||!scoped())return;
 for(int viewer=1;viewer<=load<int>(globals,0x90)&&viewer<33;++viewer){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(viewer);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n)){for(int i=1;i<33;++i)scoreSent.erase({viewer,i});continue;}
  const unsigned own=load<unsigned>(p,0x5d0)&0x3fu;
  for(int i=1;i<33;++i){const auto& t=scoreTable[i];if(!t.known)continue;
   const std::array<int,4> want{{score_seen(viewer,own,i,t.frags),t.deaths,t.third,t.team}};auto it=scoreSent.find({viewer,i});if(it!=scoreSent.end()&&it->second==want)continue;
   eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,scoreInfoMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(i);for(int v:want)eng<void(__cdecl*)(int)>(50)(v);eng<void(__cdecl*)()>(47)();
   if(want[0]!=t.frags&&scoreLines<40&&(it==scoreSent.end()||it->second[0]==t.frags)){++scoreLines;char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LIGHT player=%d gets player %d's light flags %d without the plain spot (own textured spot kept)\n",viewer,i,t.frags);log(line);}
   scoreSent[{viewer,i}]=want;}
 }
}
bool score_begin(int dest,int type,Edict* e){
 scoreCapture={};if(scoreInfoMessage<=0||type!=scoreInfoMessage||!globals||!scoped()||(dest!=0&&dest!=1&&dest!=2&&dest!=4))return false;
 int to=0;if(dest==1||dest==4){if(!live(e))return false;to=eng<int(__cdecl*)(Edict*)>(71)(e);}
 scoreCapture.active=true;scoreCapture.dest=dest;scoreCapture.to=to;return true;
}
bool score_write(int kind,int value){
 if(!scoreCapture.active)return false;auto& c=scoreCapture;
 if(kind==48&&c.bytes==0&&c.shorts==0){c.index=value;++c.bytes;}else if(kind==50&&c.bytes==1&&c.shorts<4){c.values[c.shorts++]=static_cast<short>(value);}else c.bytes=99;
 return true;
}
bool score_end(){
 if(!scoreCapture.active)return false;const auto c=scoreCapture;scoreCapture={};
 if(c.bytes!=1||c.shorts!=4||c.index<1||c.index>32){
  // Not the expected layout: deliver it unchanged.
  eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(c.dest,scoreInfoMessage,nullptr,c.to?eng<Edict*(__cdecl*)(int)>(72)(c.to):nullptr);if(c.bytes>=1&&c.bytes<99)eng<void(__cdecl*)(int)>(48)(c.index);for(int i=0;i<c.shorts;++i)eng<void(__cdecl*)(int)>(50)(c.values[i]);eng<void(__cdecl*)()>(47)();return true;}
 scoreTable[c.index]={true,c.values[0],c.values[1],c.values[2],c.values[3]};
 if(c.to)scoreSent.erase({c.to,c.index});else for(int v=1;v<33;++v)scoreSent.erase({v,c.index});
 score_sync();return true;
}

// --- Smoother moving trains and cars (coop-fixes-007) -----------------------------
// User report: the intro train lagged behind the paper effect it throws up,
// and the cars did too. The effect is drawn at once; brush entities are drawn
// ex_interp (0.1 s) in the past, from updates the host received only 20
// times a second (cl_updaterate 20 in every host log). Once per map each
// client is asked for 100 updates a second and a matching interpolation
// window: 0.01 s for the host (local), 0.03 s for the partner (network).
std::set<int> netTuned;
void net_tune(Edict* e){
 if(!campaign_map()||!live(e)||!engine[41]||std::strcmp(classname(e),"player"))return;const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 if(!netTuned.insert(index).second)return;const bool host=index==1&&engine[115]&&eng<int(__cdecl*)()>(115)()==0;
 // coop-fixes-013: the partner's camera and ladder views jittered while
 // standing still (user test over a VPN): their rate stayed 30000, too little
 // for 100 updates a second, and a 0.03 s window left no room for a late
 // packet. Partner: rate 100000 and a 0.06 s window.
 // v0.5.6 (testers: the connecting player jitters "like shivering hands"):
 // the partner still sent its moves at the default cl_cmdrate 30, and the
 // server moves a remote player only when a packet arrives, so everyone saw
 // the partner move in one of every 2-3 host frames (NET_CADENCE). Partner:
 // cl_cmdrate 100 (capped by their frame rate: one packet per frame). The
 // host's own commands go over loopback every frame (CL_MAIN.C CL_SendCmd).
 eng<void(__cdecl*)(Edict*,const char*,...)>(41)(e,"%s",host?"cl_updaterate 100\nex_interp 0.01\n":"rate 100000\ncl_updaterate 100\ncl_cmdrate 100\nex_interp 0.06\n");
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] NET tuned player=%d cl_updaterate=100 ex_interp=%s\n",index,host?"0.01":"0.06 rate=100000 cl_cmdrate=100");log(line);
}
