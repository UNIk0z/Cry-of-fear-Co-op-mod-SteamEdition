// Included in adapter.cpp's guarded implementation namespace. These interfaces
// are SDK callbacks, never addresses/private vtable calls into the backend.
struct SaveLevel {char map[32]{},landmark[32]{};Edict* entity=nullptr;float origin[3]{};};
struct SaveData {
 char* base=nullptr;char* current=nullptr;int size=0,capacity=0,tokenSize=0,tokenCount=0;char** tokens=nullptr;
 int currentIndex=0,tableCount=0,connectionCount=0;void* table=nullptr;SaveLevel levels[16]{};
 int useLandmark=0;char landmark[20]{};float offset[3]{},time=0;char map[32]{};
};
static_assert(offsetof(SaveData,levels)==44&&sizeof(SaveData)==1396,"API140 save buffer ABI");
std::vector<SavedPlayer> playerBank;
// v0.3 map state (state-runtime.hpp): written beside the slot, read with it.
bool state_save_slot(const std::filesystem::path& dir,int slot,const Checkpoint& value,std::string& why);
void state_load_slot(int slot,const Checkpoint& value);
std::unordered_map<int,std::string> profileBindings;
std::unordered_map<Edict*,int> restoredPlayers;
Checkpoint loadedCheckpoint;bool loadingCheckpoint=false,hostConfirmed=false,checkpointRestoreFailed=false,playerRestoreFailed=false;
int checkpointSlot=1,saveLoadMessage=0,screenFadeMessage=0;
float checkpointMapTime=0;
bool checkpointPrepared=false,checkpointWorldStarted=false,checkpointPreparedBackup=false;
int requestedRecorder=0;
std::unordered_map<Edict*,std::string> checkpointWait;
// coop-fixes-008: the save this session last committed or loaded, and the
// map the game restarted because every player was dead (game over).
struct SessionSave {int slot=0;std::string map;} sessionSave;std::string gameOverMap;
std::vector<SavedPlayer>::iterator checkpoint_role_record(Edict* e);
void carry_seed(const std::vector<SavedPlayer>& saved);
int pose_item(int left,int right);void story_capture(Checkpoint& value);void story_load(const Checkpoint& value);void send_health_cap(Edict* e,int cap);void rejoin_departing(Edict* e);
void checkpoint_prepare_command();
void checkpoint_world_ready();
struct RecorderWrite {int callback=0,value=0;float real=0;std::string text;};
struct RecorderMessage {bool active=false,drop=false;int destination=0,type=0;Edict* actor=nullptr;std::vector<RecorderWrite> writes;} recorderMessage;
Edict* recorderFadeActor=nullptr;int recorderFadeSerial=-1;
Edict* recorderRequest=nullptr;int recorderRequestSerial=-1;
// coop-fixes-012: the open save slot list (see save_choice_begin).
struct SaveChoice {Edict* actor=nullptr;int serial=-1,recorder=0;float until=0,next=0;float origin[3]{};std::string text;};
SaveChoice saveChoice;
void checkpoint_reset(){sessionSave={};gameOverMap.clear();playerBank.clear();profileBindings.clear();restoredPlayers.clear();loadingCheckpoint=false;hostConfirmed=false;checkpointRestoreFailed=false;playerRestoreFailed=false;loadedCheckpoint={};checkpointSlot=1;checkpointPrepared=false;checkpointWorldStarted=false;checkpointWait.clear();requestedRecorder=0;recorderRequest=nullptr;recorderMessage={};recorderFadeActor=nullptr;saveChoice={};}
void checkpoint_notice(Edict* e,const char* text){log(text);if(live(e)&&engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,2,text);}
void checkpoint_center(Edict* e,const char* text,const char* line){log(line);if(live(e)&&engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);}
bool checkpoint_host(Edict* e){return live(e)&&eng<int(__cdecl*)(Edict*)>(71)(e)==1&&engine[115]&&eng<int(__cdecl*)()>(115)()==0;}
int user_id(Edict* e){return engine[113]?eng<int(__cdecl*)(Edict*)>(113)(e):0;}
std::string auth_id(Edict* e){if(!engine[143]||!live(e))return {};const char* id=eng<const char*(__cdecl*)(Edict*)>(143)(e);return id?id:"";}
std::string steam_identity(Edict* e){std::string s=auth_id(e);if(!checkpoint_identity(s)||s.rfind("STEAM_",0)!=0||s=="STEAM_ID_LAN"||s=="STEAM_ID_PENDING")return {};return s;}
std::vector<Edict*> connected_players(){std::vector<Edict*> out;if(!globals)return out;for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(live(e)&&std::strcmp(classname(e),"player")==0)out.push_back(e);}return out;}
bool unique_steam(Edict* e,const std::vector<Edict*>& players){
 const auto s=steam_identity(e);if(s.empty())return false;int n=0;
 for(auto q:players)if(!profileBindings.count(user_id(q))&&steam_identity(q)==s)++n;return n==1;
}
// Automatic identity, no console binding required:
// 1. host-assigned profile key (advanced fallback, unchanged);
// 2. a valid Steam ID that is unique among connected players (user evidence:
//    the Steam host and Sandboxie client exposed two distinct IDs);
// 3. when IDs are LAN/duplicate (one account in two clients) the listen-server
//    host is "auto:host" and ONE remaining unresolved player is "auto:guest".
// PENDING waits for Steam. More than one unresolved guest stays refused, so
// records are never merged or silently swapped.
std::string player_identity(Edict* e){
 auto binding=profileBindings.find(user_id(e));if(binding!=profileBindings.end())return binding->second;
 if(!live(e))return {};const auto players=connected_players();
 if(unique_steam(e,players))return steam_identity(e);
 if(auth_id(e)=="STEAM_ID_PENDING")return {};
 if(checkpoint_host(e))return "auto:host";
 int unresolved=0;for(auto q:players)if(!checkpoint_host(q)&&!profileBindings.count(user_id(q))&&!unique_steam(q,players))++unresolved;
 return unresolved==1?"auto:guest":std::string{};
}
std::filesystem::path checkpoint_directory(){char dir[MAX_PATH]{};if(!engine[99])return {};eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return {};return std::filesystem::path(dir)/L"storycoop_saves";}
bool global_capture(GlobalSnapshot& out){
 if(!original[12])return false;std::vector<unsigned char> bytes(32768);std::array<char*,1024> tokens{};SaveData data;
 data.base=reinterpret_cast<char*>(bytes.data());data.current=data.base;data.capacity=static_cast<int>(bytes.size());data.tokenCount=1024;data.tokens=tokens.data();
 native<void(__cdecl*)(SaveData*)>(12)(&data);
 if(data.size<=0||data.size>data.capacity||data.current!=data.base+data.size)return false;bytes.resize(data.size);out.bytes=std::move(bytes);out.tokens.clear();
 for(auto s:tokens){if(!s)out.tokens.emplace_back();else {std::size_t n=0;while(n<64&&s[n])++n;if(n==64)return false;out.tokens.emplace_back(s,n);}}
 return validate_globals(out);
}
bool global_restore(const GlobalSnapshot& input){
 if(!original[13]||!validate_globals(input))return false;auto bytes=input.bytes;auto names=input.tokens;std::array<char*,1024> tokens{};for(std::size_t i=0;i<tokens.size();++i)if(!names[i].empty())tokens[i]=names[i].data();
 SaveData data;data.base=reinterpret_cast<char*>(bytes.data());data.current=data.base;data.capacity=static_cast<int>(bytes.size());data.tokenCount=1024;data.tokens=tokens.data();
 native<void(__cdecl*)(SaveData*)>(13)(&data);
 GlobalSnapshot actual;return global_capture(actual)&&checkpoint_globals_equal(input,actual);
}
bool item_object(std::uint32_t bits,Edict*& entity,void*& memory){
 for(const auto& item:allocations){const auto& a=item.second;if(live(item.first)&&reinterpret_cast<std::uint32_t>(a.memory)==bits&&a.bytes>=0x1e8&&serial(item.first)==a.serial&&eng<void*(__cdecl*)(Edict*)>(64)(item.first)==a.memory){entity=item.first;memory=a.memory;return std::strncmp(classname(entity),"weapon_",7)==0;}}
 return false;
}
bool weapon_list(Edict* actor,std::vector<std::pair<Edict*,void*>>& list){
 void* p=nullptr;std::size_t n=0;if(!player_memory(actor,p,n))return false;std::set<std::uint32_t> visited;
 for(int k=0;k<6;++k){auto bits=load<std::uint32_t>(p,0x2514+k*4);while(bits){Edict* entity=nullptr;void* memory=nullptr;if(list.size()>=32||!visited.insert(bits).second||!item_object(bits,entity,memory))return false;
  list.emplace_back(entity,memory);bits=load<std::uint32_t>(memory,0x164);
 }}return true;
}
// Diagnostic reason for the latest capture/restore refusal. Real two-client
// tests must report WHICH step failed; conditions themselves are unchanged.
std::string checkpointFailure;
bool checkpoint_fail(std::string reason){checkpointFailure=std::move(reason);return false;}
// Offsets verified in this backend's entvars metadata. Its padlock Use/cleanup
// sets/clears80 at flags1A8 for control freeze; do not assume another SDK's bit.
// Quick-select keys (user report 2026-10-12: key binds lost on Continue and
// after a game-over restart). quickselset1..3 (10019A1C) store the inventory
// slot (+0x3DC/+0x3E0/+0x3E4) and copy that card's weapon classname into
// m_sQuickSel1..3 (+0x204C/+0x20CC/+0x214C, 128 bytes, save table 102174FC);
// the quicksel key (100F07F0) selects by that name. Only weapon_* names are
// accepted by the game, and anything else is stored as an empty key.
constexpr std::size_t QuickSlotOffset=0x3dc,QuickNameOffset=0x204c,QuickNameBytes=0x80;
void quick_capture(void* p,SavedPlayer& result){
 for(std::size_t k=0;k<3;++k){
  const int slot=load<int>(p,QuickSlotOffset+k*4);const char* name=static_cast<const char*>(p)+QuickNameOffset+k*QuickNameBytes;
  const void* end=std::memchr(name,0,QuickNameBytes);const std::string weapon=end?std::string(name,static_cast<const char*>(end)-name):std::string();
  const bool ok=slot>=1&&slot<=6&&weapon.size()>7&&weapon.size()<64&&weapon.rfind("weapon_",0)==0&&std::all_of(weapon.begin(),weapon.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';});
  result.quick[k]=ok?QuickSlot{slot,weapon}:QuickSlot{};
 }
}
// The inventory order after a restore can differ: point each key at the slot
// that now holds the card it pointed at when saved.
void quick_restore(void* p,const SavedPlayer& saved,const SavedPlayer& now){
 for(std::size_t k=0;k<3;++k){
  const auto& q=saved.quick[k];int slot=q.slot;
  if(q.slot>=1&&q.slot<=6){const auto& card=saved.inventory[q.slot-1];for(int i=0;i<6;++i)if(!card.empty()&&now.inventory[i]==card){slot=i+1;break;}}
  store<int>(p,QuickSlotOffset+k*4,q.weapon.empty()?0:slot);
  char* name=static_cast<char*>(p)+QuickNameOffset+k*QuickNameBytes;std::memset(name,0,QuickNameBytes);std::memcpy(name,q.weapon.data(),std::min<std::size_t>(q.weapon.size(),QuickNameBytes-1));
 }
}
// v0.5 (D4, "show the correct assigned bindings in the inventory"): the keys
// above are the server's; the inventory screen draws its own copy
// (client gHUD.m_szInventoryWeapon1..3), filled only by the player's own
// quickselset or the server's QSlotUpdate(key, slot) (decomp RECBInventory
// MsgQSlotUpdate; the native co-op level change sends it the same way,
// CoF_CoopLevelChangeManager.cpp). A load sends it one second after the
// restore, when the inventory cards have reached the client.
std::unordered_map<Edict*,std::pair<int,float>> quickNotices;
void quick_notice_want(Edict* e){if(globals&&live(e))quickNotices[e]={serial(e),load<float>(globals,0)+1.f};}
void quick_notice_frame(Edict* e){
 auto it=quickNotices.find(e);if(it==quickNotices.end()||!globals)return;
 if(it->second.first!=serial(e)){quickNotices.erase(it);return;}if(load<float>(globals,0)<it->second.second)return;quickNotices.erase(it);
 void* p=nullptr;std::size_t n=0;if(qslotUpdateMessage<=0||!player_memory(e,p,n))return;int sent=0;
 for(int k=0;k<3;++k){const int slot=load<int>(p,QuickSlotOffset+k*4);if(slot<1||slot>6||!static_cast<const char*>(p)[QuickNameOffset+k*QuickNameBytes])continue;
  eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,qslotUpdateMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(k+1);eng<void(__cdecl*)(int)>(48)(slot);eng<void(__cdecl*)()>(47)();++sent;}
 if(sent){char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] QUICKKEYS player=%d %d shown in the inventory\n",eng<int(__cdecl*)(Edict*)>(71)(e),sent);log(line);}
}
// strict=false (v0.1 map-change carry): the inventory of a player frozen in a
// scene or steering the boat (+0x2500 bit 8) is read as well. Only saves and
// restores need a free-moving player; a scene that gives a weapon and then
// changes the level must not lose that weapon.
bool capture_player(Edict* actor,SavedPlayer& result,bool strict=true){
 void* p=nullptr;std::size_t n=0;auto ev=vars(actor);if(!ev||!player_memory(actor,p,n))return checkpoint_fail("player memory unavailable");
 if(load<int>(ev,0x174)!=0||load<float>(ev,0x164)<=0)return checkpoint_fail("player dead or down");
 if(strict&&(load<int>(p,0x2500)&8))return checkpoint_fail("native player state 2500 bit8 set");
 if(strict&&(load<int>(ev,0x1a8)&0x80))return checkpoint_fail("player controls frozen");
 result={};result.identity=player_identity(actor);if(!checkpoint_identity(result.identity))return checkpoint_fail("no stable identity; host must run sc_checkpoint_bind USERID KEY");result.health=load<float>(ev,0x164);result.stamina=load<float>(p,0x21f0);
 std::memcpy(result.ammo.data(),static_cast<unsigned char*>(p)+0x2540,sizeof(result.ammo));std::memcpy(result.items.data(),static_cast<unsigned char*>(p)+0x22c8,sizeof(result.items));
 for(std::size_t i=0;i<6;++i){const char* s=string_at(load<int>(p,0x2200+i*4));if(!s)return checkpoint_fail("inventory name unavailable");std::size_t j=0;while(j<128&&s[j])++j;if(j==128)return checkpoint_fail("inventory name too long");result.inventory[i]=std::string(s,j);}
 result.tapes=load<int>(p,0x1ef4);result.battery=load<int>(p,0x22a8);result.usedSyringe=load<unsigned char>(p,0x226c)!=0;
 const int cap=load<int>(p,0x60c);result.healthCap=cap>=0&&cap<=2?cap:0;result.nightVision=load<unsigned char>(p,0x434)!=0;
 quick_capture(p,result);
 // Schema 6 (coop-fixes-013): the phone screen m_iPhoneMessage (+0x5CC, the
 // phone model body that CMobile re-syncs on deploy, 10052E90) and
 // m_iPhoneDisabled (+0x1F14). A rebuilt player showed the first SMS again.
 result.phoneMessage=load<int>(p,0x5cc);result.phoneDisabled=load<int>(p,0x1f14);
 std::vector<std::pair<Edict*,void*>> list;if(!weapon_list(actor,list))return checkpoint_fail("weapon list invalid or unrecognized object");
 const auto active=load<std::uint32_t>(p,0x252c);bool foundActive=active==0;
 // weapon_dualweapon (coop-fixes-006) only pairs two owned weapons: it is
 // not saved; a dual wielder is saved holding the weapon hand (pose_item).
 bool dual=false;
 for(const auto& item:list){const std::string name=classname(item.first);if(name=="weapon_action"||name=="weapon_dualweapon"){if(reinterpret_cast<std::uint32_t>(item.second)==active){foundActive=true;dual=name=="weapon_dualweapon";}continue;}result.weapons.push_back({name,load<int>(item.second,0x1e4),load<int>(item.second,0x1dc)});if(reinterpret_cast<std::uint32_t>(item.second)==active){result.equipped=name;foundActive=true;}}
 // v0.5 (schema 8): which two weapons the dual wield pairs (item ids at +0x1E70/+0x1E74).
 if(dual)for(const auto& item:list){const std::string name=classname(item.first);if(name=="weapon_action"||name=="weapon_dualweapon")continue;const int id=load<int>(item.second,0x168);if(id==load<int>(p,0x1e70)&&result.dualA.empty())result.dualA=name;else if(id==load<int>(p,0x1e74)&&result.dualB.empty())result.dualB=name;}
 if(dual){const int hand=pose_item(load<int>(p,0x1e70),load<int>(p,0x1e74));for(const auto& item:list){const std::string name=classname(item.first);if(name!="weapon_action"&&name!="weapon_dualweapon"&&(result.equipped.empty()||load<int>(item.second,0x168)==hand)&&(load<int>(item.second,0x168)==load<int>(p,0x1e70)||load<int>(item.second,0x168)==load<int>(p,0x1e74)))result.equipped=name;}}
 return foundActive||checkpoint_fail("active weapon is not in the owned weapon list");
}
// Non-identifying evidence line: slot/record index, never Steam IDs or keys.
void checkpoint_summary(const char* phase,int index,const SavedPlayer& p){
 int reserve=0;for(int a:p.ammo)reserve+=a;std::size_t items=0;for(const auto& s:p.inventory)if(!s.empty())++items;
 char line[260]{};std::snprintf(line,sizeof(line),"[STORYCOOP] CHECKPOINT %s player=%d health=%.0f stamina=%.0f weapons=%u inventory=%u reserve=%d equipped=%s\n",phase,index,p.health,p.stamina,static_cast<unsigned>(p.weapons.size()),static_cast<unsigned>(items),reserve,p.equipped.empty()?"-":p.equipped.c_str());log(line);
}
void bank_player(const SavedPlayer& value){auto it=std::find_if(playerBank.begin(),playerBank.end(),[&](const SavedPlayer& p){return p.identity==value.identity;});if(it==playerBank.end())playerBank.push_back(value);else *it=value;}
void checkpoint_capture_departing(Edict* e){if(!scoped()||checkpointRestoreFailed)return;if(loadingCheckpoint){auto restored=restoredPlayers.find(e);if(!hostConfirmed||restored==restoredPlayers.end()||restored->second!=serial(e))return;}SavedPlayer p;if(capture_player(e,p))bank_player(p);}
bool helper_use_pairs(const char* type,const std::vector<std::pair<std::string,std::string>>& pairs,Edict* actor){
 if(!engine[23]||!engine[67]||!engine[22]||!original[1]||!original[3]||!original[6])return false;
 int id=eng<int(__cdecl*)(const char*)>(67)(type);Edict* helper=eng<Edict*(__cdecl*)(int)>(23)(id);if(!live(helper))return false;
 bool okay=true;
 for(const auto& [key,value]:pairs){KeyValue data{const_cast<char*>(type),const_cast<char*>(key.c_str()),const_cast<char*>(value.c_str()),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(helper,&data);okay=okay&&data.handled!=0;}
 okay=okay&&native<int(__cdecl*)(Edict*)>(1)(helper)==0;if(okay)native<void(__cdecl*)(Edict*,Edict*)>(3)(helper,actor);
 if(live(helper)){assist_unlink(helper);eng<void(__cdecl*)(Edict*)>(22)(helper);}return okay;
}
bool helper_use(const char* type,const char* key,const std::string& value,Edict* actor){return helper_use_pairs(type,{{key,value}},actor);}
// Saved inventory entries are card resource paths (user log: native
// GiveNamedItem rejected "inventoryitems/weapons/weapon_mobile.txt"). Weapon
// cards arrive with their weapon. Other cards are granted by native
// cof_giveitems with spawnflag 1 (item branch 100A6C0D: add by card path,
// or drop an item_key when the inventory is full).
bool weapon_card(const std::string& n){return n.rfind("inventoryitems/weapons/",0)==0;}
// land=false (coop-fixes-007 rejoin): inventory only, the player stays where
// the native spawn put them.
// atPose (coop-fixes-008 rejoin): land at the player's own saved pose even on
// the c_start aliases.
// clearExtras=false (coop-fixes-013 map-change carry): weapons the new map
// already gave are kept; only missing ones are given.
void equip_want(Edict* e,const SavedPlayer& saved);   // coop-runtime.hpp: keeps trying the held weapon / dual wield
bool restore_player(Edict* e,const SavedPlayer& saved,bool land=true,bool atPose=false,bool clearExtras=true){
 const int slot=eng<int(__cdecl*)(Edict*)>(71)(e);
 // c_start aliases: authored per-slot markers. Elsewhere: the player's own
 // saved standing point, or the nearest safe spot around it (TraceHull).
 const bool legacy=land&&!atPose&&legacy_checkpoint_map(loadedCheckpoint.map);
 if(land&&legacy&&(slot<1||slot>2))return checkpoint_fail("player slot outside the two supported landings");
 if(land&&!legacy&&(slot<1||slot>4||!saved.hasPose))return checkpoint_fail("player slot or saved pose unavailable");
 const std::string marker=legacy?"sc4_checkpoint_"+std::to_string(loadedCheckpoint.recorder)+"_p"+std::to_string(slot):std::string("sc4_coop_landing");
 std::array<float,3> landing{};float yaw=0;
 if(!land){}
 else if(legacy){
  auto destination=find_next(nullptr,"targetname",marker.c_str());
  auto dest=vars(destination);if(!dest||std::strcmp(classname(destination),"info_teleport_destination"))return checkpoint_fail("landing marker "+marker+" missing from served map");
  landing={load<float>(dest,8),load<float>(dest,12),load<float>(dest,16)};
 }else{
  std::array<float,3> at{};if(!find_landing(saved.origin,e,at))return checkpoint_fail("no safe landing near the saved position");
  landing={at[0],at[1],at[2]-37.0f};yaw=saved.angles[1];
 }
 SavedPlayer current;if(!capture_player(e,current))return checkpoint_fail("initial capture: "+checkpointFailure);
 // Synchronize the fresh native inventory; every hard problem is collected
 // but the checkpoint landing is still applied afterwards (user test: one
 // equipped mismatch used to skip teleports and every later player).
 std::string hard,soft;
 auto sync=[&]{
  if(clearExtras)for(const auto& w:current.weapons)if(std::none_of(saved.weapons.begin(),saved.weapons.end(),[&](const SavedWeapon& s){return s.name==w.name;}))if(!helper_use("cof_clearitems","message",w.name,e)){hard+=" clear:"+w.name;}
  if(clearExtras)for(const auto& name:current.inventory)if(!name.empty()&&!weapon_card(name)&&std::find(saved.inventory.begin(),saved.inventory.end(),name)==saved.inventory.end())soft+=" extra-item:"+name;
  SavedPlayer after;if(!capture_player(e,after)){hard+=" after-clear:"+checkpointFailure;return;}
  for(const auto& w:saved.weapons)if(std::none_of(after.weapons.begin(),after.weapons.end(),[&](const SavedWeapon& s){return s.name==w.name;}))if(!helper_use("cof_giveitems","message",w.name,e))hard+=" give:"+w.name;
  SavedPlayer armed;if(!capture_player(e,armed)){hard+=" after-weapons:"+checkpointFailure;return;}
  for(const auto& name:saved.inventory)if(name.rfind("inventoryitems/",0)==0&&!weapon_card(name)&&std::find(armed.inventory.begin(),armed.inventory.end(),name)==armed.inventory.end())if(!helper_use_pairs("cof_giveitems",{{"message",name},{"spawnflags","1"}},e))soft+=" give-item:"+name;
  std::vector<std::pair<Edict*,void*>> list;if(!weapon_list(e,list)){hard+=" weapon-list";return;}
  for(const auto& w:saved.weapons){auto it=std::find_if(list.begin(),list.end(),[&](const auto& item){return w.name==classname(item.first);});if(it==list.end()){hard+=" absent:"+w.name;continue;}if(load<int>(it->second,0x1dc)!=w.ammoIndex){hard+=" ammo-index:"+w.name;continue;}store(it->second,0x1e4,w.clip);}
  void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n)){hard+=" memory";return;}auto ev=vars(e);store(ev,0x164,saved.health);store(p,0x21f0,saved.stamina);std::memcpy(static_cast<unsigned char*>(p)+0x2540,saved.ammo.data(),sizeof(saved.ammo));std::memcpy(static_cast<unsigned char*>(p)+0x22c8,saved.items.data(),sizeof(saved.items));store(p,0x1ef4,saved.tapes);store(p,0x22a8,saved.battery);store<unsigned char>(p,0x226c,saved.usedSyringe?1:0);
  // User report: after Continue the first recorder Use only printed the
  // first-encounter line and saved nothing. CTapeRecorder::TapeUse (1007C987)
  // shows that line while player+0x21D8 (m_bTapeReminder) is 0, sets it and
  // returns. A checkpoint is only ever made at a recorder: mark it seen.
  store<unsigned char>(p,0x21d8,1);
  // Schema 4 story wounds and night vision (zero in older saves).
  store(p,0x5cc,saved.phoneMessage);store(p,0x1f14,saved.phoneDisabled);
  store(p,0x60c,saved.healthCap);store<unsigned char>(p,0x434,saved.nightVision?1:0);if(saved.healthCap)send_health_cap(e,saved.healthCap);
  // Native weapon switching completes over later frames: equipped is soft.
  if(!saved.equipped.empty()&&!helper_use("cof_weaponswitch","message",saved.equipped,e))soft+=" switch:"+saved.equipped;
  {SavedPlayer slots;if(capture_player(e,slots)){void* qp=nullptr;std::size_t qn=0;if(player_memory(e,qp,qn)){quick_restore(qp,saved,slots);quick_notice_want(e);}}}
  SavedPlayer check;if(!capture_player(e,check)){hard+=" readback:"+checkpointFailure;return;}
  if(clearExtras&&check.weapons.size()!=saved.weapons.size())hard+=" weapons";if(check.phoneMessage!=saved.phoneMessage||check.phoneDisabled!=saved.phoneDisabled)soft+=" phone";if(check.health!=saved.health)hard+=" health";if(check.stamina!=saved.stamina)hard+=" stamina";if(check.ammo!=saved.ammo)hard+=" reserve";if(check.items!=saved.items)hard+=" counters";
  if(check.tapes!=saved.tapes)hard+=" tapes";if(check.healthCap!=saved.healthCap||check.nightVision!=saved.nightVision)soft+=" story";if(check.battery!=saved.battery)hard+=" battery";if(check.usedSyringe!=saved.usedSyringe)hard+=" syringe";
  for(const auto& w:saved.weapons)if(std::none_of(check.weapons.begin(),check.weapons.end(),[&](const auto& q){return q.name==w.name&&q.clip==w.clip&&q.ammoIndex==w.ammoIndex;}))hard+=" clip:"+w.name;
  if(check.equipped!=saved.equipped||!saved.dualA.empty())equip_want(e,saved);
  if(check.equipped!=saved.equipped)soft+=" equipped (retrying)";
  for(std::size_t k=0;k<3;++k)if(check.quick[k].weapon!=saved.quick[k].weapon)soft+=" quick"+std::to_string(k+1);
  std::vector<std::string> a,b;for(const auto& s:check.inventory)if(!s.empty())a.push_back(s);for(const auto& s:saved.inventory)if(!s.empty())b.push_back(s);
  std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(clearExtras?a!=b:!std::includes(a.begin(),a.end(),b.begin(),b.end()))soft+=" inventory";
 };
 sync();
 if(!land){}
 else if(!(legacy?helper_use("cof_teleport","target",marker,e):teleport_to(e,{landing[0],landing[1],landing[2]+37.0f},yaw)))hard+=" teleport:"+marker;
 else if(auto ev=vars(e)){
  const float dx=load<float>(ev,8)-landing[0],dy=load<float>(ev,12)-landing[1],dz=load<float>(ev,16)-landing[2];
  if(!(std::isfinite(dx)&&std::isfinite(dy)&&std::isfinite(dz)&&dx*dx+dy*dy<=16&&dz>=0&&dz<=72)){char where[120]{};std::snprintf(where,sizeof(where)," landing-readback dx=%.1f dy=%.1f dz=%.1f",dx,dy,dz);hard+=where;}
 }else hard+=" player-lost";
 if(!soft.empty()){const std::string line="[STORYCOOP] RESTORE soft differences player="+std::to_string(slot)+":"+soft+"\n";log(line.c_str());}
 return hard.empty()||checkpoint_fail(hard.substr(1));
}
// v0.1 (user request): only the host saves; the file is written on the
// host's PC and its owner record is the host's. Everyone is told where it went
// and hears the recorder's save sound once, on their own client ("play": the
// game's save/save_sequence.wav, which CTapeRecorder 100EF227 plays from the
// saving player). A cancelled or failed save plays nothing.
void checkpoint_saved_notice(int slot){
 char text[96]{};std::snprintf(text,sizeof(text),"Game saved in slot %d\non the host's PC.\n",slot);
 for(auto e:connected_players()){if(engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);if(engine[41])eng<void(__cdecl*)(Edict*,const char*,...)>(41)(e,"%s","play save/save_sequence.wav\n");}
}
// coop-fixes-013: a refused save is also shown on the saver's screen (the
// user saw nothing happen at a recorder); the full reason stays in the log.
void save_failed(Edict* actor,const char* full,const char* shown){checkpoint_notice(actor,full);if(live(actor)&&engine[80]){std::string text=std::string("Could not save:\n")+shown+"\n";eng<void(__cdecl*)(Edict*,int,const char*)>(80)(actor,1,text.c_str());}}
const SavedPlayer* carry_record(Edict* e);
void checkpoint_save(Edict* actor){
 Edict* host=globals&&engine[72]?eng<Edict*(__cdecl*)(int)>(72)(1):nullptr;
 if(!live(actor)||!checkpoint_host(host)){checkpoint_notice(actor,"[STORYCOOP] Save refused: the host is not ready.\n");return;}
 if(actor!=host){checkpoint_notice(actor,"[STORYCOOP] Save refused: only the host can save the game.\n");return;}
 const bool legacy=legacy_checkpoint_map(map_name());
 if(!checkpoint_map(map_name())||(legacy?(requestedRecorder!=419&&requestedRecorder!=421):(requestedRecorder<1||requestedRecorder>8191))||(loadingCheckpoint&&!hostConfirmed)||checkpointRestoreFailed||playerRestoreFailed){save_failed(actor,"[STORYCOOP] Checkpoint refused: unsupported map, pending restore or failed restoration.\n","not possible on this map now");return;}
 everyone(true);Checkpoint value;value.map=map_name();value.recorder=requestedRecorder;value.owner=player_identity(host);
 // v0.2: the music the host hears (player+0x2310 track, +0x231C loop), restored on load.
 {void* hp=nullptr;std::size_t hn=0;if(player_memory(host,hp,hn)){const int t=load<int>(hp,0x2310);const char* s=t?string_at(t):nullptr;std::string track=s?s:"";
  if(track.size()<=60&&std::all_of(track.begin(),track.end(),[](char c){return std::isalnum(static_cast<unsigned char>(c))||c=='_'||c=='-'||c=='.'||c=='/';})){value.music=track;value.musicLoop=load<unsigned char>(hp,0x231c)?1:0;}}}const std::string saverIdentity=player_identity(actor);
 if(!global_capture(value.globals)||(legacy&&checkpoint_global(value.globals,"sc4.event.c_start.wake.done")!=1)){checkpoint_notice(actor,"[STORYCOOP] Checkpoint refused: wake-up must finish; global stream must validate.\n");return;}
 if(legacy)for(const char* id:{"sc4.event.c_start.wake.active","sc4.event.c_start.blacker.active","sc4.puzzle.c_start.padlock.active"})if(checkpoint_global(value.globals,id)==1){checkpoint_notice(actor,"[STORYCOOP] Checkpoint refused during an active story event.\n");return;}
 std::set<std::string> connected;auto next=playerBank;int clients=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!live(e)||std::strcmp(classname(e),"player"))continue;SavedPlayer p;
  if(loadingCheckpoint&&(std::any_of(playerBank.begin(),playerBank.end(),[&](const SavedPlayer& saved){return saved.identity==player_identity(e);})||checkpoint_role_record(e)!=playerBank.end())){const auto record=restoredPlayers.find(e);if(record==restoredPlayers.end()||record->second!=serial(e)){checkpoint_notice(actor,"[STORYCOOP] Save refused while a connected player awaits checkpoint restoration.\n");return;}}
  auto allocation=allocations.find(e);
  if(allocation==allocations.end()||allocation->second.rejection){save_failed(actor,"[STORYCOOP] Save refused: native journal contains uncaptured/rejected state; existing checkpoint retained.\n","notes could not be read");return;}
  const char* refusal=!client_ready(e)?"client HUD not ready":!capture_player(e,p)?checkpointFailure.c_str():!connected.insert(p.identity).second?"duplicate identity; bind distinct profiles":nullptr;
  // coop-fixes-013: the saver's partner may be busy (ladder, scene, revive,
  // loading); their last good record is saved and lands beside the saver.
  if(refusal&&e!=actor&&std::strcmp(refusal,"duplicate identity; bind distinct profiles")){
   if(const SavedPlayer* last=carry_record(e)){
    char text[220]{};std::snprintf(text,sizeof(text),"[STORYCOOP] SAVE player %d busy (%s): their last good record is saved\n",i,refusal);log(text);
    SavedPlayer kept=*last;kept.hasPose=false;auto it=std::find_if(next.begin(),next.end(),[&](const SavedPlayer& s){return s.identity==kept.identity;});if(it==next.end())next.push_back(kept);else *it=kept;continue;
   }
  }
  if(refusal){char text[300]{};std::snprintf(text,sizeof(text),"[STORYCOOP] Save refused: player %d state/identity unavailable (%s). Host may bind distinct profiles with sc_checkpoint_bind USERID KEY.\n",i,refusal);save_failed(actor,text,e==actor?"you are busy right now":"your partner is not ready");return;}
  checkpoint_summary("save",i,p);
  if(auto ev=vars(e)){p.hasPose=true;p.origin={load<float>(ev,8),load<float>(ev,12),load<float>(ev,16)};p.angles={load<float>(ev,0x50),load<float>(ev,0x54),load<float>(ev,0x58)};}
  ++clients;auto it=std::find_if(next.begin(),next.end(),[&](const SavedPlayer& s){return s.identity==p.identity;});if(it==next.end())next.push_back(p);else *it=p;
 }
 if((legacy&&clients>2)||clients>4||next.size()>4){checkpoint_notice(actor,"[STORYCOOP] Save refused: too many active players for the safe landings, or more than four retained identity records.\n");return;}
 // Retained records of players who are not connected now carry a pose from
 // another map (or none): they land beside the player who committed the save.
 const auto saver=std::find_if(next.begin(),next.end(),[&](const SavedPlayer& s){return s.identity==saverIdentity;});
 if(value.owner.empty()||std::none_of(next.begin(),next.end(),[&](const SavedPlayer& s){return s.identity==value.owner;})){checkpoint_notice(actor,"[STORYCOOP] Save refused: the host's record is unavailable.\n");return;}
 if(saver==next.end()||!saver->hasPose){checkpoint_notice(actor,"[STORYCOOP] Save refused: the saving player's position is unavailable.\n");return;}
 const auto anchor=*saver;for(auto& s:next)if(!connected.count(s.identity)){s.hasPose=true;s.origin=anchor.origin;s.angles=anchor.angles;}
 value.difficulty=difficulty_now();value.options=sessionOptions;value.objective=objectiveText;value.objectiveFile=objectiveFile;
 value.players=next;world_capture(value);keypad_capture(value);story_capture(value);for(std::size_t i=0;i<journal.size();++i){value.notes.push_back(journal.at(i));value.metadata.push_back(journal.metadata(i));}
 value.haveDigits=puzzle_has_solution();value.digits=puzzle_solution();value.haveCredentials=puzzle_has_credentials();value.credentials=puzzle_credentials();auto dir=checkpoint_directory();
 // v0.3: the map-state file goes first; it names this checkpoint by CRC, so a
 // failed checkpoint commit leaves the previous pair (kept as .bak) matching.
 {std::string why;if(!dir.empty()&&!state_save_slot(dir,checkpointSlot,value,why)){const std::string full="[STORYCOOP] Checkpoint NOT saved: map state "+why+"; the previous save is kept.\n";save_failed(actor,full.c_str(),"the map state could not be written");return;}}
 if(dir.empty()||!commit_checkpoint(dir,checkpointSlot,value)){save_failed(actor,"[STORYCOOP] Checkpoint NOT saved: validation or atomic disk commit failed.\n","the save file could not be written");return;}
 playerBank=std::move(next);sessionSave={checkpointSlot,value.map};char text[160]{};std::snprintf(text,sizeof(text),"[STORYCOOP] Checkpoint slot%d COMMITTED map=%s recorder=%d (%u player records); restart restore still requires gameplay verification.\n",checkpointSlot,value.map.c_str(),value.recorder,static_cast<unsigned>(playerBank.size()));log(text);checkpoint_saved_notice(checkpointSlot);
}
int argument_number(int i){const char* s=eng<const char*(__cdecl*)(int)>(83)(i);if(!s||!*s)return -1;int n=0;for(;*s;++s){if(*s<'0'||*s>'9'||n>100000)return -1;n=n*10+(*s-'0');}return n;}
bool checkpoint_command(Edict* actor,const char* command){
 if(!command||std::strncmp(command,"sc_checkpoint_",14))return false;
 if(!checkpoint_host(actor)){checkpoint_notice(actor,"[STORYCOOP] Checkpoint controls belong to the host.\n");return true;}
 if(!std::strcmp(command,"sc_checkpoint_slot")){int slot=argument_number(1);if(slot>=1&&slot<=5){checkpointSlot=slot;checkpoint_notice(actor,"[STORYCOOP] Save slot selected. Use a tape recorder to commit.\n");}else checkpoint_notice(actor,"[STORYCOOP] Slot must be 1, 2 or 3.\n");}
 else if(!std::strcmp(command,"sc_checkpoint_bind")){int uid=argument_number(1);const char* key=eng<const char*(__cdecl*)(int)>(83)(2);std::string name=key?key:"";if(uid<=0||name.empty()||name.size()>48||!checkpoint_identity(name)||name.find(':')!=name.npos){checkpoint_notice(actor,"[STORYCOOP] Bind requires a current status USERID and a stable profile KEY (letters/digits/underscore/hyphen).\n");return true;}
  bool exists=false;for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(live(e)&&user_id(e)==uid)exists=true;}if(exists){profileBindings[uid]="profile:"+name;checkpoint_notice(actor,"[STORYCOOP] Host assigned profile. Use the same person/key on future joins.\n");}else checkpoint_notice(actor,"[STORYCOOP] No live participant has that USERID.\n");
 }else checkpoint_notice(actor,"[STORYCOOP] Restore is a fresh-launch server command; see Continue instructions.\n");
 return true;
}
bool checkpoint_stage(Checkpoint value,bool backup,int slot){
 if(value.map!=map_name()||(legacy_checkpoint_map(value.map)&&checkpoint_global(value.globals,"sc4.event.c_start.wake.done")!=1)){log("[STORYCOOP] LOAD REFUSED: checkpoint map/progression mismatch.\n");return false;}
 if(!global_restore(value.globals)||!puzzle_load_saved(value.haveDigits,value.digits,value.haveCredentials,value.credentials)){checkpointRestoreFailed=true;log("[STORYCOOP] LOAD FAILED: native shared-state restore did not validate. Restart session.\n");return false;}
 keypad_load(value);story_load(value);objective_session_reset();objectiveText=value.objective;objectiveFile=value.objectiveFile;
 journal.reset();for(std::size_t i=0;i<value.notes.size();++i)if(journal.grant(value.notes[i],value.metadata[i])){checkpointRestoreFailed=true;log("[STORYCOOP] LOAD FAILED: journal conflict; restart session.\n");return false;}
 state_load_slot(slot,value);
 // v0.3 audit: a player not restored yet (still joining) when the map is left
 // keeps the save's record for the next map (carry_seed); it used to be dropped.
 carry_reset();sessionSave={slot,value.map};loadedCheckpoint=std::move(value);playerBank=loadedCheckpoint.players;carry_seed(playerBank);loadingCheckpoint=true;hostConfirmed=false;restoredPlayers.clear();checkpointWait.clear();checkpointSlot=slot;
 log(backup?"[STORYCOOP] LOAD staged from valid BACKUP before map entities; waiting for host identity and native HUD.\n":"[STORYCOOP] LOAD staged before map entities; waiting for host identity and native HUD.\n");return true;
}
// Continue queues the fully validated payload BEFORE +map. Worldspawn consumes
// it before authored trigger_auto/monster/door entities initialize, irrespective
// of disk speed, sign-on duration or the engine's initial game-time value.
void checkpoint_prepare_command(){
 const int slot=argument_number(1);auto dir=checkpoint_directory();Checkpoint value;bool backup=false;
 if(checkpointWorldStarted||checkpointPrepared||dir.empty()||!read_checkpoint(dir,slot,value,backup)){log("[STORYCOOP] PREPARE REFUSED: launch Continue in a fresh process with a valid save.\n");return;}
 loadedCheckpoint=std::move(value);checkpointSlot=slot;checkpointPrepared=true;checkpointPreparedBackup=backup;
 log(backup?"[STORYCOOP] PREPARE accepted valid BACKUP; awaiting saved map.\n":"[STORYCOOP] PREPARE accepted; awaiting saved map.\n");
}
// Continue launcher request (storycoop_saves/continue.request: "COFCONT1 SLOT
// MAP"). It does not depend on when +commands run relative to GameInit. It is
// consumed and deleted at the first real map and honored only for that exact
// saved map within 15 minutes, so a stale request never loads into a new game.
bool checkpoint_continue_request(int& slot){
 auto dir=checkpoint_directory();if(dir.empty())return false;const auto path=dir/L"continue.request";
 std::error_code error,removed;if(!std::filesystem::is_regular_file(path,error))return false;
 const auto written=std::filesystem::last_write_time(path,error);const auto age=std::filesystem::file_time_type::clock::now()-written;
 std::ifstream file(path);char text[96]{};file.read(text,sizeof(text)-1);file.close();std::filesystem::remove(path,removed);
 // Strict "COFCONT1 <1-3> <map>" with a lowercase map token and nothing else.
 const std::string line(text);int requested=0;std::string map;
 if(line.size()>11&&line.compare(0,9,"COFCONT1 ")==0&&line[9]>='1'&&line[9]<='3'&&line[10]==' '){
  requested=line[9]-'0';map=line.substr(11);while(!map.empty()&&(map.back()=='\r'||map.back()=='\n'||map.back()==' '))map.pop_back();
  if(map.empty()||map.size()>39||!std::all_of(map.begin(),map.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';}))requested=0;
 }
 if(!requested){log("[STORYCOOP] CONTINUE request malformed; ignored and removed\n");return false;}
 if(map!=map_name()){log("[STORYCOOP] CONTINUE request for another map; ignored and removed\n");return false;}
 if(error||age>std::chrono::minutes(15)){log("[STORYCOOP] CONTINUE request stale; ignored and removed\n");return false;}
 slot=requested;return true;
}
void checkpoint_world_ready(){
 // The actual supplied log proves valve.rc first loads c_game_menu1 before
 // processing launch +commands. This boot menu must not close the prepare gate.
 if(std::strcmp(map_name(),"c_game_menu1")==0)return;
 checkpointWorldStarted=true;
 int requestSlot=0;const bool requested=checkpoint_continue_request(requestSlot);
 if(!checkpointPrepared&&requested){
  auto dir=checkpoint_directory();Checkpoint value;bool backup=false;
  if(read_checkpoint(dir,requestSlot,value,backup)){loadedCheckpoint=std::move(value);checkpointSlot=requestSlot;checkpointPrepared=true;checkpointPreparedBackup=backup;log("[STORYCOOP] PREPARE accepted from Continue request file\n");}
  else {checkpointRestoreFailed=true;log("[STORYCOOP] PREPARE REFUSED: Continue request slot has no valid checkpoint or backup\n");}
 }
 // Game over (both players dead): the game restarts the map by itself and
 // only carries what everyone had when they entered it. When this session's
 // save is on the restarted map, load it exactly like Continue: positions,
 // inventories, quick keys, taken pickups and opened doors.
 if(!checkpointPrepared&&!gameOverMap.empty()){
  const std::string over=gameOverMap;gameOverMap.clear();
  auto dir=checkpoint_directory();Checkpoint value;bool backup=false;char line[200]{};
  if(over==map_name()&&sessionSave.map==over&&!dir.empty()&&read_checkpoint(dir,sessionSave.slot,value,backup)&&value.map==over){
   loadedCheckpoint=std::move(value);checkpointSlot=sessionSave.slot;checkpointPrepared=true;checkpointPreparedBackup=backup;
   std::snprintf(line,sizeof(line),"[STORYCOOP] GAME OVER restart of %s: loading the last save (slot%d)\n",over.c_str(),checkpointSlot);
  }else std::snprintf(line,sizeof(line),"[STORYCOOP] GAME OVER restart of %s: no save of this map in this session; the game's own restart\n",over.c_str());
  log(line);
 }
 if(!checkpointPrepared)return;checkpointPrepared=false;
 auto value=std::move(loadedCheckpoint);if(!checkpoint_stage(std::move(value),checkpointPreparedBackup,checkpointSlot)){checkpointRestoreFailed=true;log("[STORYCOOP] Prepared checkpoint failed; stop this session.\n");}
}
void __cdecl checkpoint_load_command(){
 // Old launchers are explicitly rejected: loading after entity construction
 // could miss one-shot restoration gates and appear to succeed on a wrong world.
 log("[STORYCOOP] LOAD REFUSED: use the updated Continue launcher (prepare before map). No state changed.\n");
}
// Role fallback when no record has this player's exact identity. Host -> the
// owner record. Guest -> the single non-owner record that no connected player
// matches exactly, and only when this is the single such unmatched guest.
std::vector<SavedPlayer>::iterator checkpoint_role_record(Edict* e){
 const auto none=playerBank.end();
 if(checkpoint_host(e))return std::find_if(playerBank.begin(),playerBank.end(),[](const SavedPlayer& p){return p.identity==loadedCheckpoint.owner;});
 const auto players=connected_players();
 auto exact=[&](const std::string& id){for(auto q:players)if(player_identity(q)==id)return true;return false;};
 int guests=0;for(auto q:players)if(!checkpoint_host(q)&&!std::any_of(playerBank.begin(),playerBank.end(),[&](const SavedPlayer& p){return p.identity==player_identity(q);}))++guests;
 if(guests!=1)return none;
 auto found=none;int candidates=0;
 for(auto it=playerBank.begin();it!=playerBank.end();++it)if(it->identity!=loadedCheckpoint.owner&&!exact(it->identity)){found=it;++candidates;}
 return candidates==1?found:none;
}
void checkpoint_wait(Edict* e,const char* reason){auto& old=checkpointWait[e];if(old!=reason){old=reason;checkpoint_notice(e,reason);}}
void checkpoint_player_frame(Edict* e){
 if(!loadingCheckpoint||checkpointRestoreFailed||!checkpoint_map(map_name())||!live(e))return;auto restored=restoredPlayers.find(e);if(restored!=restoredPlayers.end()&&restored->second==serial(e))return;
 auto host=eng<Edict*(__cdecl*)(int)>(72)(1);const auto identity=player_identity(e);
 // The save file lives on the listen-server host's disk and its owner record
 // is, by construction, the host who saved. An exact identity match is
 // preferred; otherwise the host role selects the owner record.
 if(!hostConfirmed){if(checkpoint_host(host)){hostConfirmed=true;if(player_identity(host)!=loadedCheckpoint.owner)log("[STORYCOOP] RESTORE host matched by role (identity changed or unavailable)\n");}else {checkpoint_wait(e,"[STORYCOOP] RESTORE WAIT: listen-server host not ready.\n");return;}}
 if(identity.empty()){checkpoint_wait(e,"[STORYCOOP] RESTORE WAIT: stable player identity unavailable (Steam pending or several unidentified guests).\n");return;}int duplicates=0;for(int i=1;i<=load<int>(globals,0x90);++i){auto other=eng<Edict*(__cdecl*)(int)>(72)(i);if(live(other)&&identity==player_identity(other))++duplicates;}if(duplicates!=1){checkpoint_wait(e,"[STORYCOOP] RESTORE WAIT: duplicate identities; assign distinct saved profiles.\n");return;}
 auto it=std::find_if(playerBank.begin(),playerBank.end(),[&](const SavedPlayer& p){return p.identity==identity;});
 if(it==playerBank.end()){it=checkpoint_role_record(e);if(it!=playerBank.end())log(checkpoint_host(e)?"[STORYCOOP] RESTORE host record matched by role\n":"[STORYCOOP] RESTORE guest record matched by role (single unmatched guest and record)\n");}
 if(it==playerBank.end()){checkpoint_wait(e,"[STORYCOOP] RESTORE WAIT: no saved inventory matches this identity or role.\n");return;}
 // Mark before touching the inventory: a failed partial restore must never
 // retrigger grants every frame. Restart or inspect the explicit failure.
 SavedPlayer ready;if(!client_ready(e)||!capture_player(e,ready)){const std::string reason=std::string("[STORYCOOP] RESTORE WAIT: native player/HUD initialization or frozen/dead state (")+(client_ready(e)?checkpointFailure:std::string("client HUD not ready"))+").\n";checkpoint_wait(e,reason.c_str());return;}
 restoredPlayers[e]=serial(e);
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 if(restore_player(e,*it)){restore(e,true);if(!loadedCheckpoint.music.empty())door_music(loadedCheckpoint.music.c_str(),loadedCheckpoint.musicLoop,0.f);SavedPlayer now;if(capture_player(e,now))checkpoint_summary("restored",index,now);checkpoint_notice(e,"[STORYCOOP] Player checkpoint restored and inventory read back. Verify gameplay/HUD/visibility.\n");}
 else {playerRestoreFailed=true;const std::string text="[STORYCOOP] Player RESTORE FAILED ("+checkpointFailure+"); other players continue; saving stays blocked so the checkpoint is not overwritten.\n";checkpoint_notice(e,text.c_str());}
}
// --- Save slot choice (coop-fixes-012, user request) ---------------------------
// A tape recorder no longer saves at once: the player who used it sees the
// three slots on the host's PC and presses 1, 2 or 3. With the game's default
// binds those keys send "quicksel N" to the server (hl.dll 1001999B otherwise
// switches the quick weapon); it is taken only from that player while the list
// is up. The list closes after 15 s, or when the player walks away or dies:
// nothing is saved then. v0.1: only the host gets the list.
// "13 Oct 21:40" from the slot file (or its backup) when it holds a valid save.
std::string save_slot_label(const std::filesystem::path& dir,int slot){
 Checkpoint value;bool backup=false;if(dir.empty()||!read_checkpoint(dir,slot,value,backup))return "empty";
 auto file=dir/(L"slot"+std::to_wstring(slot)+L".cofsave");if(backup)file+=L".bak";
 WIN32_FILE_ATTRIBUTE_DATA data{};FILETIME local{};SYSTEMTIME t{};
 if(!GetFileAttributesExW(file.c_str(),GetFileExInfoStandard,&data)||!FileTimeToLocalFileTime(&data.ftLastWriteTime,&local)||!FileTimeToSystemTime(&local,&t)||t.wMonth<1||t.wMonth>12)return "saved";
 static const char* months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
 char text[32]{};std::snprintf(text,sizeof(text),"%u %s %02u:%02u",static_cast<unsigned>(t.wDay),months[t.wMonth-1],static_cast<unsigned>(t.wHour),static_cast<unsigned>(t.wMinute));return text;
}
// v0.1 (user request): the host picks the slot for every save; using the
// recorder again shows the list again instead of saving to the last slot.
std::string save_choice_text(){
 const auto dir=checkpoint_directory();std::string text="SAVE GAME (host's PC)\n";
 for(int slot=1;slot<=5;++slot)text+=std::to_string(slot)+": "+save_slot_label(dir,slot)+"\n";
 return text+"Press 1 to 5\n";
}
void save_choice_print(Edict* e,const char* text){if(live(e)&&engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);}
bool save_choice_open(){return saveChoice.actor&&live(saveChoice.actor)&&serial(saveChoice.actor)==saveChoice.serial;}
void save_choice_close(const char* line){if(save_choice_open())save_choice_print(saveChoice.actor,"");saveChoice={};if(line)log(line);}
void save_choice_commit(int slot){
 Edict* actor=saveChoice.actor;const int recorder=saveChoice.recorder;saveChoice={};
 checkpointSlot=slot;requestedRecorder=recorder;recorderRequest=actor;recorderRequestSerial=serial(actor);
 char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP] SAVE slot %d chosen by player %d\n",slot,eng<int(__cdecl*)(Edict*)>(71)(actor));log(line);
}
void save_choice_begin(Edict* actor,int recorder){
 if(!checkpoint_host(actor))return;
 auto v=vars(actor);if(!v||!globals)return;
 saveChoice={};saveChoice.actor=actor;saveChoice.serial=serial(actor);saveChoice.recorder=recorder;
 for(int k=0;k<3;++k)saveChoice.origin[k]=load<float>(v,8+k*4);
 const float now=load<float>(globals,0);saveChoice.until=now+15.f;saveChoice.next=now;
 try{saveChoice.text=save_choice_text();}catch(...){saveChoice.text="SAVE GAME (host's PC)\nPress 1, 2 or 3\n";}
 char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP] SAVE slot list shown to player %d (recorder %d)\n",eng<int(__cdecl*)(Edict*)>(71)(actor),recorder);log(line);
}
// "quicksel 1..3" from the choosing player while the list is up.
bool save_choice_command(Edict* e,const char* command){
 if(!command||std::strcmp(command,"quicksel")||!save_choice_open()||e!=saveChoice.actor)return false;
 const char* a=eng<const char*(__cdecl*)(int)>(83)(1);if(!a||a[0]<'1'||a[0]>'5'||a[1])return true;
 save_choice_commit(a[0]-'0');return true;
}
void save_choice_frame(){
 if(!saveChoice.actor||!globals)return;
 if(!save_choice_open()){saveChoice={};log("[STORYCOOP] SAVE slot list closed: the player left\n");return;}
 auto v=vars(saveChoice.actor);const float now=load<float>(globals,0);
 float d=0;if(v)for(int k=0;k<3;++k){const float x=load<float>(v,8+k*4)-saveChoice.origin[k];d+=x*x;}
 if(!v||load<int>(v,0x174)!=0||load<float>(v,0x164)<=0){save_choice_close("[STORYCOOP] SAVE slot list closed: the player died\n");return;}
 if(d>96.f*96.f){save_choice_close("[STORYCOOP] SAVE slot list closed: the player walked away\n");return;}
 if(now>=saveChoice.until){save_choice_close("[STORYCOOP] SAVE slot list closed: no slot chosen\n");return;}
 if(now>=saveChoice.next){saveChoice.next=now+1.f;save_choice_print(saveChoice.actor,saveChoice.text.c_str());}
}
// --- The single-player save window for the host (v0.7, docs/V07_TODO.md 4) -----------------
// Katz 2026-10-27: when the host uses a tape recorder, the game's own save
// window opens on the host's screen with its sound and fade, the host picks one
// of five boxes with a double click, then the save sequence plays as in single
// player; partners keep "Only host can save the game". The window (decomp
// cl_dll SaveLoad_guess.cpp) is opened by the recorder's own SaveLoad message,
// which is now passed on to the host. A double click sends "unfreezesave 1024
// <slot>" (server PreSave: sound, "Saving progress...", 3 s, "Saved", fade):
// the co-op checkpoint goes to that slot, and the window's later "savehack
// cofsave<slot>" (the single-player save itself, hud_redraw.cpp 310) is never
// run in co-op. The window's slot labels are its own files: in co-op launches
// the client reads and writes SAVE/coopinfo<slot>.cof instead of saveinfo
// (client-menu.hpp), so the single-player saves and labels are never touched.
struct SaveWindow {Edict* actor=nullptr;int serial=-1,recorder=0;float until=0;} saveWindow;
bool saveWindowEnabled=true;   // false: the v0.1 key list (fixtures)
bool save_window_open(Edict* actor,int recorder,const RecorderMessage& message);
bool save_window_open(Edict* actor,int recorder,const RecorderMessage& message){
 if(!saveWindowEnabled||saveLoadMessage<=0||!engine[46]||!engine[47]||!engine[48]||!engine[54]||!globals||!live(actor))return false;
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(message.destination,message.type,nullptr,actor);
 for(const auto& w:message.writes){if(w.callback==54)eng<void(__cdecl*)(const char*)>(54)(w.text.c_str());else eng<void(__cdecl*)(int)>(w.callback)(w.value);}
 eng<void(__cdecl*)()>(47)();
 saveWindow={actor,serial(actor),recorder,load<float>(globals,0)+120.f};
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP] SAVE window opened for player %d (recorder %d)\n",eng<int(__cdecl*)(Edict*)>(71)(actor),recorder);log(line);return true;
}
bool save_window_command(Edict* e,const char* command){
 if(!command||!scoped())return false;
 if(!std::strcmp(command,"savehack")){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP] SAVE single-player save from the window skipped (player %d)\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);return true;}
 if(std::strcmp(command,"unfreezesave")||!saveWindow.actor||e!=saveWindow.actor||serial(e)!=saveWindow.serial||!globals||load<float>(globals,0)>saveWindow.until)return false;
 const char* a=eng<const char*(__cdecl*)(int)>(83)(1);const char* b=eng<const char*(__cdecl*)(int)>(83)(2);
 if(!a||std::strcmp(a,"1024")||!b||b[0]<'1'||b[0]>'5'||b[1])return false;
 saveChoice={};saveChoice.actor=e;saveChoice.serial=serial(e);saveChoice.recorder=saveWindow.recorder;saveWindow={};
 save_choice_commit(b[0]-'0');
 // Saved now: the native PreSave below sets FL_NOTARGET, which a capture one frame later refuses.
 if(recorderRequest==e){recorderRequest=nullptr;checkpoint_save(e);}
 return false;   // the native PreSave runs its sequence as well
}
bool recorder_begin(int destination,int type,const float*,Edict* actor){
 recorderMessage={};
 if(recorderFadeActor){bool drop=type==screenFadeMessage&&actor==recorderFadeActor&&serial(actor)==recorderFadeSerial;recorderFadeActor=nullptr;if(drop){recorderMessage.active=true;recorderMessage.drop=true;return true;}}
 if(!scoped()||destination!=1||type!=saveLoadMessage||saveLoadMessage<=0||!live(actor)||std::strcmp(classname(actor),"player"))return false;
 recorderMessage.active=true;recorderMessage.destination=destination;recorderMessage.type=type;recorderMessage.actor=actor;return true;
}
bool recorder_write(int callback,int value,float real,const char* text){if(!recorderMessage.active)return false;if(!recorderMessage.drop)recorderMessage.writes.push_back({callback,value,real,text?text:""});return true;}
int near_recorder(Edict* actor,const std::string& title){
 if(!checkpoint_map(map_name()))return 0;auto p=vars(actor);if(!p)return 0;
 if(!legacy_checkpoint_map(map_name())){
  // Every other alias: the tape_recorder the player is using, identified by
  // its BSP entity index in the served map (the native Use range is ~64u).
  Edict* e=nullptr;
  for(int n=0;n<256;++n){e=find_next(e,"classname","tape_recorder");if(!e)break;auto q=vars(e);if(!q)continue;
   const float dx=load<float>(p,8)-load<float>(q,8),dy=load<float>(p,12)-load<float>(q,12),dz=load<float>(p,16)-load<float>(q,16);
   if(dx*dx+dy*dy+dz*dz<128*128){const int index=spawn_index(e);return index>0&&index<=8191?index:0;}}
  return 0;
 }
 struct Tape{int id;const char* title;float x,y,z;};
 for(const auto& tape:{Tape{419,"Storage Tunnel",1778.6f,-3259.05f,-775},Tape{421,"Sewer Store",1735.85f,-3429.36f,-775}}){
  if(title!=tape.title)continue;float dx=load<float>(p,8)-tape.x,dy=load<float>(p,12)-tape.y,dz=load<float>(p,16)-tape.z;
  if(dx*dx+dy*dy+dz*dz>=128*128)return 0;
  Edict* e=nullptr;for(int n=0;n<32;++n){e=find_next(e,"classname","tape_recorder");if(!e)break;auto q=vars(e);if(q&&std::fabs(load<float>(q,8)-tape.x)<1&&std::fabs(load<float>(q,12)-tape.y)<1&&std::fabs(load<float>(q,16)-tape.z)<1)return tape.id;}
 }return 0;
}
bool recorder_end(){
 if(!recorderMessage.active)return false;auto message=std::move(recorderMessage);recorderMessage={};if(message.drop)return true;
 if(message.writes.size()==2&&message.writes[0].callback==48&&message.writes[0].value==0&&message.writes[1].callback==54){
  // A partner's recorder Use saves nothing and shows "Only host can save the
  // game" to that partner only (v0.2: never from merely standing there).
  if(!checkpoint_host(message.actor)){char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP] SAVE refused for player %d: only the host can save\n",eng<int(__cdecl*)(Edict*)>(71)(message.actor));log(line);if(engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(message.actor,1,"Only host can save the game");}
  else if(const int recorder=near_recorder(message.actor,message.writes[1].text)){
   // v0.7 (docs/V07_TODO.md 4): the host gets the single-player save window
   // (five slots, its sound, fade and post process) instead of the key list.
   if(save_window_open(message.actor,recorder,message)){return true;}
   save_choice_begin(message.actor,recorder);}
  else checkpoint_notice(message.actor,"[STORYCOOP] Recorder checkpoint unavailable here; no save committed.\n");
  recorderFadeActor=message.actor;recorderFadeSerial=serial(message.actor);return true;
 }
 // Unrelated native SaveLoad UI retains its exact payload and behavior.
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(message.destination,message.type,nullptr,message.actor);
 for(const auto& w:message.writes){if(w.callback==54)eng<void(__cdecl*)(const char*)>(54)(w.text.c_str());else if(w.callback==52||w.callback==53)eng<void(__cdecl*)(float)>(w.callback)(w.real);else eng<void(__cdecl*)(int)>(w.callback)(w.value);}eng<void(__cdecl*)()>(47)();return true;
}
void __cdecl checkpoint_start_frame(){
 native<void(__cdecl*)()>(25)();recorderFadeActor=nullptr;
 if(recorderRequest){auto actor=recorderRequest;const int id=recorderRequestSerial;recorderRequest=nullptr;if(serial(actor)==id)checkpoint_save(actor);}
 save_choice_frame();
}
// After every map entity spawned (ServerActivate): remove the pickups that
// were taken and unlock the key doors that were open when the save was made.
void checkpoint_world_activate(){
 world_activate();
 if(loadingCheckpoint&&!checkpointRestoreFailed&&loadedCheckpoint.map==map_name())world_apply(loadedCheckpoint);
 // v0.2: PlayerSpawn plays sv_coop_music, so the saved track is set before anyone spawns.
 if(loadingCheckpoint&&!checkpointRestoreFailed&&loadedCheckpoint.map==map_name()&&!loadedCheckpoint.music.empty()&&engine[60])eng<void(__cdecl*)(const char*,const char*)>(60)("sv_coop_music",loadedCheckpoint.music.c_str());
}
// coop-fixes-007: a player kicked before their HUD settled (still loading)
// keeps the record they carried into the map; rejoin_departing remembers it.
// Called at the end of a map (ServerDeactivate): every connected player dead
// or down means the game's own game-over restart is changing the level.
// v0.5.7: a death in a boss fight ends the attempt for everyone (boss_frame in coop-runtime.hpp).
bool forcedGameOver=false;
void checkpoint_game_over_check(){
 gameOverMap.clear();const bool forced=forcedGameOver;forcedGameOver=false;if(!scoped()||!globals||!checkpoint_map(map_name()))return;int players=0,alive=0;
 if(forced){gameOverMap=map_name();char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP] GAME OVER on %s: a player died in a boss fight\n",gameOverMap.c_str());log(line);return;}
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);auto v=live(e)&&!std::strcmp(classname(e),"player")?vars(e):nullptr;if(!v)continue;++players;if(load<int>(v,0x174)==0&&load<float>(v,0x164)>0)++alive;}
 if(players&&!alive){gameOverMap=map_name();char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP] GAME OVER on %s: every player is dead\n",gameOverMap.c_str());log(line);}
}
void __cdecl checkpoint_disconnect(Edict* e){rejoin_departing(e);if(client_ready(e))checkpoint_capture_departing(e);profileBindings.erase(user_id(e));native<void(__cdecl*)(Edict*)>(16)(e);}
