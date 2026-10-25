// Included inside the adapter namespace after coop-runtime.hpp.
// --- Map state across revisits and saves (v0.3) ------------------------------------
// User (v0.2 playtests): going back to a map brought the rope and the tape
// back, re-ran enemy spawns and reset doors, the balcony climb lock and puzzle
// states. Single player keeps a saved copy of every level it left; co-op
// ChangeLevel reloads the BSP. This keeps the same copy with the game's own
// per-entity save code (decomp cbase.cpp DispatchSave/DispatchRestore,
// API140 slots 7/8, SDK SAVERESTOREDATA): when a campaign map ends, every
// entity is written into a buffer kept for its source map (the alias without
// _eN). On the next visit, before ServerActivate (as in a single-player level
// load), each saved entity is read back onto the entity with the same
// identity in the freshly spawned map:
//  - BSP entities: classname, targetname, model and spawn origin from the BSP,
//    read before their own Spawn, plus an ordinal for identical ones;
//  - entities other entities create while the map loads (sprites, bodyque):
//    classname and creation order ("rt:");
//  - monsters created during play (monstermaker, scripts): recreated by
//    classname and restored, as the engine does for a saved level ("new:").
// Gone at the end of the visit = gone again: taken pickups, dead monsters
// (DispatchSave skips them, as in single player), fired once-triggers and
// killtargets are removed with the native killtarget flags. Kept as served,
// never restored or removed: player starts, landmarks, level-change entities,
// the triggers that fire a cof_changelevel (every exit must keep working) and
// movewith parents/children (their link lists are rebuilt only by Spawn).
// A save writes slotN.cofmap beside slotN.cofsave, naming that checkpoint by
// CRC; loading a save replaces the whole kept timeline with that file (or with
// nothing for an older save) and a new campaign starts empty. Kill switch: the
// file cryoffear/storycoop_mapstate.off (checked at every map change).
struct SaveTable {int id=0;Edict* pent=nullptr;int location=0,size=0,flags=0,classname=0;};
static_assert(sizeof(SaveTable)==24,"SDK ENTITYTABLE ABI (x86)");
constexpr int StateTokens=0xfff;   // the engine's own token table size
constexpr std::size_t StateBuffer=24u<<20,StateFileLimit=192u<<20;
struct StateEntity {int id=0;std::string key,type;int location=0,size=0;};
struct MapState {std::string map;std::vector<unsigned char> data;std::vector<std::string> tokens;std::vector<StateEntity> entities;std::vector<std::string> removed;std::uint32_t layout=0;};
std::map<std::string,MapState> mapStates;
struct RuntimeRecord {Edict* e=nullptr;int serial=-1;std::string key;};
std::vector<RuntimeRecord> loadRuntime;
std::map<std::string,int> stateOrdinals;
std::set<Edict*> moveWithUsers;std::set<std::string> moveWithNames,keepKeys;
bool stateLoaded=false;   // the loaded save had a matching map-state file
std::uint32_t stateLayout=0;   // CRC of this map's load-time entity names (see state_activate)
std::uint32_t state_crc(const unsigned char* p,std::size_t n);
// v0.3 tester crash log: a map is kept only when it ends by a real level change
// (the engine ChangeLevel call or a "changelevel" server command). A quit, an
// error or a crash also runs ServerDeactivate; the world is then not a
// finished visit and is never kept.
std::string stateChangeTo;
void state_level_change(const char* map){stateChangeTo=map&&*map?map:"?";}
std::string state_source(const std::string& map){
 const auto at=map.rfind("_e");if(at==std::string::npos||at+2>=map.size())return map;
 for(std::size_t i=at+2;i<map.size();++i)if(map[i]<'0'||map[i]>'9')return map;
 return map.substr(0,at);
}
bool state_off(){
 char dir[MAX_PATH]{};if(!engine[99])return true;eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return true;
 const auto path=std::filesystem::path(dir)/L"storycoop_mapstate.off";return GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES;
}
bool state_scope(){return scoped()&&checkpoint_map(map_name());}
void state_reset(){mapStates.clear();stateLoaded=false;}
void state_map_begin(){stateOrdinals.clear();moveWithUsers.clear();moveWithNames.clear();keepKeys.clear();loadRuntime.clear();}
void state_keyvalue(Edict* e,const KeyValue* kv){
 if(!kv||!kv->key||!kv->value||!*kv->value||std::strcmp(kv->key,"movewith"))return;
 moveWithUsers.insert(e);moveWithNames.insert(kv->value);
}
// Before the native Spawn: entvars hold exactly the BSP keyvalues.
std::string state_identity(Edict* e){
 auto p=vars(e);if(!p)return {};const char* name=string_at(load<int>(p,0x1d0));const char* model=string_at(load<int>(p,0xb8));
 char key[240]{};std::snprintf(key,sizeof(key),"%.40s|%.60s|%.60s|%.0f %.0f %.0f",classname(e),name?name:"",model?model:"",load<float>(p,8),load<float>(p,12),load<float>(p,16));
 const int n=stateOrdinals[key]++;return std::string(key)+"#"+std::to_string(n);
}
std::set<std::string> state_exit_names(){
 std::set<std::string> out;Edict* e=nullptr;
 for(int n=0;n<256;++n){e=find_next(e,"classname","cof_changelevel");if(!e)break;const char* name=targetname(e);if(name&&*name)out.insert(name);}
 return out;
}
// Served transition entities and movewith groups always keep the map's own state.
bool state_keep(Edict* e,const std::string& type,const std::set<std::string>& exits){
 for(const char* t:{"worldspawn","player","info_player_start","info_player_deathmatch","info_player_coop","info_landmark","info_teleport_destination","trigger_changelevel","trigger_coop_changelevel","inter_door_coop_changelevel","inter_door_exit","cof_changelevel","cof_lobby_start"})if(type==t)return true;
 if(type.rfind("cof_coop_spawn",0)==0||moveWithUsers.count(e))return true;
 auto p=vars(e);if(!p)return false;
 const char* name=string_at(load<int>(p,0x1d0));if(name&&*name&&moveWithNames.count(name))return true;
 const char* target=string_at(load<int>(p,0x1cc));if(target&&*target&&exits.count(target))return true;
 if(type=="trigger_command"){const char* command=string_at(load<int>(p,0x1d4));if(command&&(std::strstr(command,"changelevel")||std::strstr(command,"map ")))return true;}
 return false;
}
bool state_client(Edict* e){if(!e||!live(e))return false;const int i=eng<int(__cdecl*)(Edict*)>(71)(e);return i>=1&&i<=load<int>(globals,0x90);}
bool state_dead_monster(Edict* e,const std::string& type){
 if(type.rfind("monster_",0)!=0||type=="monster_generic")return false;auto p=vars(e);
 return p&&(load<int>(p,0x174)!=0||load<float>(p,0x164)<=0);
}
bool state_doomed(Edict* e){auto p=vars(e);return !p||(load<unsigned>(p,0x1a8)&FlKillMe);}
// The native killtarget's visible effect, never a think (world-runtime.hpp
// entity_retire): it also leaves Spirit's assist list and is not Activated.
void state_kill(Edict* e){entity_retire(e);}
std::string state_label(const std::string& key){const auto a=key.find('|'),b=a==std::string::npos?a:key.find('|',a+1);if(b==std::string::npos)return key.substr(0,40);const std::string name=key.substr(a+1,b-a-1);return name.empty()?key.substr(0,a):name;}
bool state_capture(MapState& out,std::string& why){
 if(!original[7]||!globals||!engine[72]||!engine[67]){why="save callback unavailable";return false;}
 const int clients=load<int>(globals,0x90),maxEntities=load<int>(globals,0x94);
 struct Candidate {Edict* e;std::string key,type;bool fixed;};std::vector<Candidate> list;std::set<Edict*> known;
 out={};out.map=map_name();out.layout=stateLayout;
 for(std::size_t i=1;i<spawnRecords.size();++i){
  const auto& r=spawnRecords[i];if(!r.e||r.key.empty()||keepKeys.count(r.key))continue;
  const bool present=serial(r.e)==r.serial&&r.type==classname(r.e);if(present)known.insert(r.e);
  if(!present||state_doomed(r.e)||(r.pickup&&world_taken(r))){out.removed.push_back(r.key);continue;}
  list.push_back({r.e,r.key,r.type,true});
 }
 for(const auto& r:loadRuntime){
  if(keepKeys.count(r.key))continue;const bool present=serial(r.e)==r.serial;if(present)known.insert(r.e);
  if(!present||state_doomed(r.e)){out.removed.push_back(r.key);continue;}
  list.push_back({r.e,r.key,classname(r.e),true});
 }
 int created=0;
 for(int i=clients+1;i<maxEntities;++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!e||!live(e)||known.count(e)||state_doomed(e))continue;
  const std::string type=classname(e);auto p=vars(e);
  if(type.rfind("monster_",0)!=0||state_client(load<Edict*>(p,0x19c))||state_client(load<Edict*>(p,0x198)))continue;
  list.push_back({e,"new:"+type,type,false});++created;
 }
 std::vector<unsigned char> buffer(StateBuffer);std::vector<char*> tokens(StateTokens,nullptr);std::vector<SaveTable> table(list.size());
 for(std::size_t i=0;i<list.size();++i){table[i].id=static_cast<int>(i);table[i].pent=list[i].e;table[i].classname=eng<int(__cdecl*)(const char*)>(67)(list[i].type.c_str());}
 SaveData data;data.base=reinterpret_cast<char*>(buffer.data());data.current=data.base;data.capacity=static_cast<int>(buffer.size());data.tokenCount=StateTokens;data.tokens=tokens.data();
 data.tableCount=static_cast<int>(table.size());data.table=table.data();data.time=load<float>(globals,0);std::snprintf(data.map,sizeof(data.map),"%s",map_name());
 for(std::size_t i=0;i<list.size();++i){
  data.currentIndex=static_cast<int>(i);const int before=data.size;crumb("mapstate save",list[i].e);
  native<void(__cdecl*)(Edict*,SaveData*)>(7)(list[i].e,&data);
  if(data.size>=data.capacity){why="buffer full";return false;}
  if(data.size==before){if(list[i].fixed&&state_dead_monster(list[i].e,list[i].type))out.removed.push_back(list[i].key);continue;}
  out.entities.push_back({static_cast<int>(i),list[i].key,list[i].type,table[i].location,data.size-table[i].location});
 }
 if(data.size<0||data.current!=data.base+data.size){why="inconsistent save buffer";return false;}
 out.data.assign(buffer.begin(),buffer.begin()+data.size);
 out.tokens.resize(StateTokens);for(int i=0;i<StateTokens;++i)if(tokens[i]){std::size_t n=0;while(n<200&&tokens[i][n])++n;out.tokens[i].assign(tokens[i],n);}
 (void)created;return true;
}
void state_log_kept(const char* what,const MapState& s){
 int created=0;for(const auto& x:s.entities)if(x.key.rfind("new:",0)==0)++created;
 char line[260]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE %s %s (%s): saved=%u removed=%u runtime-monsters=%d bytes=%u\n",what,state_source(s.map).c_str(),s.map.c_str(),static_cast<unsigned>(s.entities.size()),static_cast<unsigned>(s.removed.size()),created,static_cast<unsigned>(s.data.size()));log(line);
 std::string names;for(const auto& k:s.removed){if(names.size()>600)break;names+=(names.empty()?"":", ")+state_label(k);}
 if(!names.empty()){std::string text="[STORYCOOP_RC4] MAPSTATE gone on "+state_source(s.map)+": "+names+"\n";log(text.c_str());}
}
void state_deactivate(){
 const std::string changeTo=stateChangeTo;stateChangeTo.clear();
 if(!state_scope()||state_off())return;
 if(changeTo.empty()){log("[STORYCOOP_RC4] MAPSTATE not kept: the map ended without a level change (quit, error or crash)\n");return;}
 if(!gameOverMap.empty()){log("[STORYCOOP_RC4] MAPSTATE not kept: game over restarts this map\n");return;}
 if(checkpointRestoreFailed){log("[STORYCOOP_RC4] MAPSTATE not kept: failed restore in this session\n");return;}
 MapState s;std::string why;
 if(!state_capture(s,why)){std::string text="[STORYCOOP_RC4] MAPSTATE NOT kept for "+std::string(map_name())+": "+why+"; the next visit loads the map fresh\n";log(text.c_str());mapStates.erase(state_source(map_name()));return;}
 state_log_kept("kept",s);mapStates[state_source(s.map)]=std::move(s);
}
// ServerActivate, before the native one: every entity has spawned and
// precaching is still open, exactly where a single-player level load restores.
void state_activate(){
 loadRuntime.clear();keepKeys.clear();stateChangeTo.clear();
 if(!state_scope()||!globals||!engine[72])return;
 const auto exits=state_exit_names();
 for(std::size_t i=1;i<spawnRecords.size();++i){const auto& r=spawnRecords[i];if(r.e&&!r.key.empty()&&serial(r.e)==r.serial&&state_keep(r.e,r.type,exits))keepKeys.insert(r.key);}
 std::set<Edict*> bsp;for(const auto& r:spawnRecords)if(r.e&&serial(r.e)==r.serial)bsp.insert(r.e);
 std::map<std::string,int> order;const int clients=load<int>(globals,0x90),maxEntities=load<int>(globals,0x94);
 for(int i=clients+1;i<maxEntities;++i){
  // v0.3 audit: an entity without a classname (tester log: "rt:#1" in the
  // MAPSTATE gone list) has no identity, and state_decode refused an empty type,
  // so one such saved entity made the whole slotN.cofmap unreadable. Kept as served.
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!e||!live(e)||bsp.count(e))continue;const std::string type=classname(e);if(type.empty())continue;
  RuntimeRecord r{e,serial(e),"rt:"+type+"#"+std::to_string(order[type]++)};if(state_keep(e,type,exits))keepKeys.insert(r.key);loadRuntime.push_back(r);
 }
 {std::string all;for(const auto& r:loadRuntime)all+=r.key+'\n';stateLayout=state_crc(reinterpret_cast<const unsigned char*>(all.data()),all.size());}
 if(state_off()){log("[STORYCOOP_RC4] MAPSTATE off (storycoop_mapstate.off): map loaded fresh\n");return;}
 const auto found=mapStates.find(state_source(map_name()));
 if(found==mapStates.end()){char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE first visit of %s in this timeline: map as served\n",state_source(map_name()).c_str());log(line);return;}
 const MapState& s=found->second;if(!original[8]||!engine[23]||!engine[67]){log("[STORYCOOP_RC4] MAPSTATE restore callback unavailable; map as served\n");return;}
 std::unordered_map<std::string,Edict*> fresh;
 for(std::size_t i=1;i<spawnRecords.size();++i){const auto& r=spawnRecords[i];if(r.e&&!r.key.empty()&&serial(r.e)==r.serial)fresh[r.key]=r.e;}
 // v0.3 crash audit: load-time entities are named by type and creation order.
 // Another alias of the map (s4_start -> s4_start_e0) can create a different
 // set, and the same name then means another entity: its saved state was
 // written into the wrong one, or a removal freed one that others still point
 // to. They are matched only when the kept visit had the same layout.
 const bool runtimeOk=s.layout?s.layout==stateLayout:s.map==map_name();
 if(runtimeOk){for(const auto& r:loadRuntime)if(serial(r.e)==r.serial)fresh[r.key]=r.e;}
 else log("[STORYCOOP_RC4] MAPSTATE load-time entities kept as served (this alias creates a different set)\n");
 int removed=0,restored=0,created=0,unmatched=0,kept=0,failed=0,deleted=0;
 for(const auto& key:s.removed){if(keepKeys.count(key)){++kept;continue;}auto it=fresh.find(key);if(it==fresh.end())continue;state_kill(it->second);fresh.erase(it);++removed;}
 std::vector<SaveTable> table(s.entities.size());
 for(std::size_t j=0;j<s.entities.size();++j){
  const auto& x=s.entities[j];auto& t=table[j];t.id=x.id;t.location=x.location;t.size=x.size;t.classname=eng<int(__cdecl*)(const char*)>(67)(x.type.c_str());
  if(x.key.rfind("new:",0)==0){t.pent=eng<Edict*(__cdecl*)(int)>(23)(t.classname);if(t.pent&&live(t.pent))++created;else {t.pent=nullptr;++failed;}continue;}
  if(keepKeys.count(x.key)){++kept;continue;}
  auto it=fresh.find(x.key);if(it==fresh.end()||x.type!=classname(it->second)){++unmatched;continue;}
  t.pent=it->second;
 }
 auto bytes=s.data;std::vector<std::string> names=s.tokens;std::vector<char*> tokens(StateTokens,nullptr);
 for(int i=0;i<StateTokens&&i<static_cast<int>(names.size());++i)if(!names[i].empty())tokens[i]=names[i].data();
 SaveData data;data.base=reinterpret_cast<char*>(bytes.data());data.capacity=static_cast<int>(bytes.size());data.tokenCount=StateTokens;data.tokens=tokens.data();
 data.tableCount=static_cast<int>(table.size());data.table=table.data();data.time=load<float>(globals,0);std::snprintf(data.map,sizeof(data.map),"%s",map_name());
 for(std::size_t j=0;j<table.size();++j){
  auto e=table[j].pent;if(!e)continue;
  data.currentIndex=static_cast<int>(j);data.size=table[j].location;data.current=data.base+table[j].location;crumb("mapstate restore",e);
  const int result=native<int(__cdecl*)(Edict*,SaveData*,int)>(8)(e,&data,0);
  if(result<0||!live(e)){if(live(e))state_kill(e);++deleted;continue;}
  if(auto p=vars(e)){const std::array<float,3> at{load<float>(p,8),load<float>(p,12),load<float>(p,16)};eng<void(__cdecl*)(Edict*,const float*)>(28)(e,at.data());}
  ++restored;
 }
 char line[300]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE restored %s on %s: restored=%d removed=%d recreated=%d kept-as-served=%d unmatched=%d create-failed=%d deleted-by-game=%d\n",state_source(map_name()).c_str(),map_name(),restored,removed,created,kept,unmatched,failed,deleted);log(line);crumb("mapstate done");
}
// --- slotN.cofmap ------------------------------------------------------------
// "COFMAP01", version 1, CRC of the checkpoint it belongs to, file size, CRC
// of the rest; then every kept map. Read only by this adapter.
std::uint32_t state_crc(const unsigned char* p,std::size_t n){std::uint32_t c=0xffffffff;for(std::size_t i=0;i<n;++i){c^=p[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<int>(c&1)));}return ~c;}
std::uint32_t state_checkpoint_crc(const Checkpoint& value){const auto b=encode_checkpoint(value);std::uint32_t c=0;if(b.size()>=20)std::memcpy(&c,b.data()+16,4);return c;}
struct StateWriter {
 std::vector<unsigned char> b;
 void u32(std::uint32_t v){const auto p=reinterpret_cast<const unsigned char*>(&v);b.insert(b.end(),p,p+4);}
 void str(const std::string& s){u32(static_cast<std::uint32_t>(s.size()));b.insert(b.end(),s.begin(),s.end());}
};
struct StateReader {
 const std::vector<unsigned char>& b;std::size_t at=0;bool ok=true;
 std::uint32_t u32(){if(b.size()-at<4){ok=false;at=b.size();return 0;}std::uint32_t v=0;std::memcpy(&v,b.data()+at,4);at+=4;return v;}
 std::string str(std::size_t max=255){const auto n=u32();if(!ok||n>max||n>b.size()-at){ok=false;at=b.size();return {};}std::string s(reinterpret_cast<const char*>(b.data()+at),n);at+=n;return s;}
};
std::vector<unsigned char> state_encode(const std::map<std::string,MapState>& states,std::uint32_t checkpoint){
 StateWriter w;const char magic[8]={'C','O','F','M','A','P','0','1'};w.b.insert(w.b.end(),magic,magic+8);w.u32(2);w.u32(checkpoint);w.u32(0);w.u32(0);
 w.u32(static_cast<std::uint32_t>(states.size()));
 for(const auto& [source,s]:states){
  w.str(source);w.str(s.map);w.u32(static_cast<std::uint32_t>(s.data.size()));w.b.insert(w.b.end(),s.data.begin(),s.data.end());
  std::uint32_t named=0;for(const auto& t:s.tokens)if(!t.empty())++named;w.u32(named);
  for(std::size_t i=0;i<s.tokens.size();++i)if(!s.tokens[i].empty()){w.u32(static_cast<std::uint32_t>(i));w.str(s.tokens[i]);}
  w.u32(static_cast<std::uint32_t>(s.entities.size()));for(const auto& x:s.entities){w.u32(static_cast<std::uint32_t>(x.id));w.str(x.key);w.str(x.type);w.u32(static_cast<std::uint32_t>(x.location));w.u32(static_cast<std::uint32_t>(x.size));}
  w.u32(static_cast<std::uint32_t>(s.removed.size()));for(const auto& k:s.removed)w.str(k);w.u32(s.layout);
 }
 if(w.b.size()>StateFileLimit)return {};
 const auto size=static_cast<std::uint32_t>(w.b.size()),sum=state_crc(w.b.data()+24,w.b.size()-24);std::memcpy(w.b.data()+16,&size,4);std::memcpy(w.b.data()+20,&sum,4);return w.b;
}
bool state_decode(const std::vector<unsigned char>& b,std::uint32_t checkpoint,std::map<std::string,MapState>& out){
 if(b.size()<28||b.size()>StateFileLimit||std::memcmp(b.data(),"COFMAP01",8))return false;
 StateReader r{b,8};const auto version=r.u32();if((version!=1&&version!=2)||r.u32()!=checkpoint||r.u32()!=b.size()||r.u32()!=state_crc(b.data()+24,b.size()-24))return false;
 std::map<std::string,MapState> states;const auto count=r.u32();if(count>512)return false;
 for(std::uint32_t m=0;m<count&&r.ok;++m){
  MapState s;const auto source=r.str(64);s.map=r.str(64);const auto n=r.u32();if(!r.ok||n>StateBuffer||n>b.size()-r.at)return false;s.data.assign(b.begin()+r.at,b.begin()+r.at+n);r.at+=n;
  s.tokens.resize(StateTokens);const auto named=r.u32();if(named>StateTokens)return false;
  for(std::uint32_t i=0;i<named&&r.ok;++i){const auto index=r.u32();auto t=r.str(200);if(index>=StateTokens||t.empty()||t.find('\0')!=std::string::npos)return false;s.tokens[index]=std::move(t);}
  const auto entities=r.u32();if(entities>8192)return false;std::set<int> ids;
  for(std::uint32_t i=0;i<entities&&r.ok;++i){StateEntity x;x.id=static_cast<int>(r.u32());x.key=r.str();x.type=r.str(63);x.location=static_cast<int>(r.u32());x.size=static_cast<int>(r.u32());
   if(x.id<0||x.id>8191||!ids.insert(x.id).second||x.location<0||x.size<=0||static_cast<std::size_t>(x.location)+static_cast<std::size_t>(x.size)>s.data.size())return false;
   if(!x.type.empty())s.entities.push_back(std::move(x));}   // classless (written by the first v0.3 builds): kept as served
  const auto removed=r.u32();if(removed>8192)return false;for(std::uint32_t i=0;i<removed&&r.ok;++i)s.removed.push_back(r.str());if(version==2)s.layout=r.u32();
  if(!r.ok||source.empty()||state_source(s.map)!=source||!states.emplace(source,std::move(s)).second)return false;
 }
 if(!r.ok||r.at!=b.size())return false;out=std::move(states);return true;
}
std::filesystem::path state_path(const std::filesystem::path& dir,int slot){return dir/(L"slot"+std::to_wstring(slot)+L".cofmap");}
bool state_file(const std::filesystem::path& p,std::vector<unsigned char>& out){
 const auto a=GetFileAttributesW(p.c_str());if(a==INVALID_FILE_ATTRIBUTES||(a&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))return false;
 std::ifstream f(p,std::ios::binary|std::ios::ate);const auto n=f.tellg();if(n<28||n>static_cast<std::streamoff>(StateFileLimit))return false;
 out.resize(static_cast<std::size_t>(n));f.seekg(0);f.read(reinterpret_cast<char*>(out.data()),n);return f.good();
}
// Atomic like commit_checkpoint: a new temp file, then ReplaceFile keeps the
// previous pair's file as .bak (it still matches the previous checkpoint).
bool state_commit(const std::filesystem::path& dir,int slot,const std::vector<unsigned char>& bytes){
 if(bytes.empty())return false;if(!CreateDirectoryW(dir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;
 const auto p=state_path(dir,slot),backup=std::filesystem::path(p.wstring()+L".bak"),tmp=std::filesystem::path(p.wstring()+L".tmp-"+std::to_wstring(GetCurrentProcessId()));
 HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool okay=WriteFile(h,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(h);CloseHandle(h);
 if(okay){std::vector<unsigned char> old;
  if(GetFileAttributesW(p.c_str())==INVALID_FILE_ATTRIBUTES)okay=MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
  else if(state_file(p,old))okay=ReplaceFileW(p.c_str(),tmp.c_str(),backup.c_str(),0,nullptr,nullptr)!=0;
  else okay=DeleteFileW(p.c_str())&&MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH)!=0;   // unreadable primary: the .bak stays
 }DeleteFileW(tmp.c_str());return okay;
}
bool state_save_slot(const std::filesystem::path& dir,int slot,const Checkpoint& value,std::string& why){
 if(state_off()){log("[STORYCOOP_RC4] MAPSTATE off: save has no map-state file\n");return true;}
 auto states=mapStates;
 if(state_scope()){MapState now;if(!state_capture(now,why))return false;state_log_kept("saved",now);states[state_source(now.map)]=std::move(now);}
 const auto bytes=state_encode(states,state_checkpoint_crc(value));if(bytes.empty()){why="too large";return false;}
 if(!state_commit(dir,slot,bytes)){why="file write failed";return false;}
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE slot%d written: %u map(s), %u bytes\n",slot,static_cast<unsigned>(states.size()),static_cast<unsigned>(bytes.size()));log(line);return true;
}
// Loading a save replaces the kept timeline: its own file, or nothing.
void state_load_slot(int slot,const Checkpoint& value){
 mapStates.clear();stateLoaded=false;if(state_off())return;
 auto dir=checkpoint_directory();const auto crc=state_checkpoint_crc(value);std::map<std::string,MapState> states;std::vector<unsigned char> bytes;const char* used=nullptr;
 if(!dir.empty()){if(state_file(state_path(dir,slot),bytes)&&state_decode(bytes,crc,states))used="";else if(state_file(std::filesystem::path(state_path(dir,slot).wstring()+L".bak"),bytes)&&state_decode(bytes,crc,states))used=" (backup)";}
 char line[200]{};
 if(used){mapStates=std::move(states);stateLoaded=true;std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE slot%d loaded%s: %u map(s) in this save's timeline\n",slot,used,static_cast<unsigned>(mapStates.size()));}
 else std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAPSTATE slot%d has no matching map-state file (save from v0.2 or earlier): maps load as served, old pickup/door lists apply\n",slot);
 log(line);
}
// Older saves (no map-state file) only list taken pickups. A taken key item
// that no saved player holds while this map still has a lock it opens was
// spent on a lock the old save could not record (v0.2: the balcony rope
// placed, ropebutton back after the load). Leaving the pickup in place lets the
// players use it on that lock again; every other taken pickup stays removed.
bool state_reoffer(const SpawnRecord& r,const Checkpoint& value){
 if(stateLoaded||r.type!="item_key")return false;auto p=vars(r.e);const char* card=p?string_at(load<int>(p,0x1d8)):nullptr;if(!card||!*card)return false;
 std::string base=card;base=base.substr(base.find_last_of('/')==std::string::npos?0:base.find_last_of('/')+1);if(base.size()>4&&_stricmp(base.c_str()+base.size()-4,".txt")==0)base.resize(base.size()-4);
 for(const auto& player:value.players)for(const auto& held:player.inventory)if(_stricmp(held.c_str(),card)==0)return false;
 for(const auto& [ent,lock]:lockRecords)if(serial(ent)==lock.serial&&_stricmp(lock.key.c_str(),base.c_str())==0){
  char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP] WORLD older save: %s left in place (no player holds it; %.40s still needs it)\n",card,targetname(ent));log(line);return true;
 }
 return false;
}
// v0.6.1: no saved state of this map in this timeline yet (item copies only then).
bool state_first_visit(){return state_off()||mapStates.find(state_source(map_name()))==mapStates.end();}
