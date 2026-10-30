// Owned SDK fixture for state-runtime.hpp (v0.3 map state). The fake game DLL
// saves/restores a small record per entity through the real SAVERESTOREDATA /
// ENTITYTABLE layout, including an entity reference by table id and a time
// field; no original game code is loaded and no game is launched.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <map>
namespace rc4 {struct Edict {std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,48> entities{};
std::unordered_map<Edict*,std::vector<unsigned char>> mem;
std::array<unsigned char,0xa0> globalData{};
std::vector<std::string> strings{"","s4_test_e1"};
std::vector<std::string> lines;std::string gameDir;int nextFree=30;std::set<Edict*> frozen;
bool logged(std::initializer_list<const char*> parts){for(const auto& line:lines)if(std::all_of(parts.begin(),parts.end(),[&](const char* s){return line.find(s)!=std::string::npos;}))return true;return false;}
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<48;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<48&&load<int>(&entities[i],0)==0?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void print(const char* s){lines.emplace_back(s);}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:0x1d0;
 for(int i=start?index(start)+1:1;i<48;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0];
}
int relinks=0;void set_origin(Edict* e,const float* o){store(ev(e),8,o[0]);store(ev(e),12,o[1]);store(ev(e),16,o[2]);++relinks;}
Edict* make(int i,const char* type,const char* name,float x=0,float y=0,float z=0,const char* model=""){
 auto e=&entities[i];*e={};store(e,0,0);store(e,4,700+i);store(ev(e),0,intern(type));store(ev(e),0x1d0,intern(name));store(ev(e),0xb8,intern(model));
 store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,z);store(ev(e),0x164,100.f);mem[e]=std::vector<unsigned char>(0x100);store(mem[e].data(),4,ev(e));return e;
}
Edict* created(int type){auto e=&entities[nextFree++];assert(nextFree<48);*e={};store(e,0,0);store(e,4,900+nextFree);store(ev(e),0,type);mem[e]=std::vector<unsigned char>(0x100);store(mem[e].data(),4,ev(e));return e;}
void dir_(char* p){std::strcpy(p,gameDir.c_str());}
// Fake DispatchSave/DispatchRestore with the SDK rules the adapter relies on.
void dsave(Edict* e,SaveData* d){
 auto table=static_cast<SaveTable*>(d->table);auto t=table+d->currentIndex;assert(t->pent==e&&t->id==d->currentIndex);
 const std::string type=classname(e);
 if(type=="soundent"||frozen.count(e))return;                                      // FCAP_DONT_SAVE / frozen by a camera
 if(type.rfind("monster_",0)==0&&type!="monster_generic"&&load<float>(ev(e),0x164)<=0)return;   // dead monsters are not saved
 d->tokens[5]=const_cast<char*>("ENTVARS");t->location=d->size;
 auto put=[&](const void* p,int n){std::memcpy(d->current,p,n);d->current+=n;d->size+=n;};
 const float health=load<float>(ev(e),0x164);put(&health,4);
 Edict* enemy=load<Edict*>(ev(e),0x194);int ref=-1;if(enemy)for(int i=0;i<d->tableCount;++i)if(table[i].pent==enemy){ref=i;break;}put(&ref,4);
 float think=load<float>(ev(e),0x104);think=think!=0?think-d->time:0;put(&think,4);
 t->size=d->size-t->location;
}
int restores=0;
int drestore(Edict* e,SaveData* d,int global){
 assert(global==0&&d->tokens[5]&&!std::strcmp(d->tokens[5],"ENTVARS")&&d->size+12<=d->capacity);++restores;
 float health=0,think=0;int ref=-1;std::memcpy(&health,d->current,4);std::memcpy(&ref,d->current+4,4);std::memcpy(&think,d->current+8,4);d->current+=12;d->size+=12;
 Edict* enemy=nullptr;auto table=static_cast<SaveTable*>(d->table);for(int i=0;i<d->tableCount;++i)if(table[i].id==ref)enemy=table[i].pent;
 store(ev(e),0x164,health);store(ev(e),0x194,enemy);store(ev(e),0x104,think!=0?think+d->time:0.f);
 return std::strcmp(classname(e),"env_dead_global")==0?-1:0;
}
float& now(){return *reinterpret_cast<float*>(globalData.data());}
void set_map(const char* name){store(globals,0x0c,intern(name));}
// The served map: BSP lump order (index 0 worldspawn), then what Spawn created.
std::vector<Edict*> bspList;
void spawn_map(const char* name,bool extraStart){
 for(int i=3;i<48;++i){entities[i]={};store(&entities[i],0,1);}mem.clear();nextFree=30;set_map(name);
 bspList={&entities[0]};
 make(3,"item_key","ropeheh",191,240,425);store(ev(&entities[3]),0x1d8,intern("inventoryitems/rope.txt"));store(ev(&entities[3]),0x10c,1);
 make(4,"func_button","ropebutton",0,0,0,"*68");
 make(5,"monster_zombie","",10,20,30);
 make(6,"trigger_once","",0,0,0,"*70");store(ev(&entities[6]),0x1cc,intern("multibadkars2"));
 make(7,"trigger_once","",0,0,0,"*12");store(ev(&entities[7]),0x1cc,intern("sc_level_7"));        // physical exit
 make(8,"cof_changelevel","sc_level_7");
 make(9,"env_model","rope2",38,499,270,"models/items/rope2.mdl");store(ev(&entities[9]),0x164,0.f);
 make(10,"env_sprite","",5,5,5);make(11,"env_sprite","",5,5,5);                 // identical twins: ordinals
 make(12,"multisource","sc4_cd_142_gate");store(ev(&entities[12]),0x164,1.f);
 make(13,"func_wall","mover_child",0,0,0,"*9");make(14,"func_train","mover",0,0,0,"*8");
 int last=14;if(extraStart){make(15,"info_player_start","",1,2,3);last=15;}
 state_map_begin();
 const std::string worldKey=state_identity(&entities[0]);world_spawned(&entities[0],true,0,worldKey);
 for(int i=3;i<=last;++i){
  if(i==13){KeyValue kv{const_cast<char*>("func_wall"),const_cast<char*>("movewith"),const_cast<char*>("mover"),0};state_keyvalue(&entities[13],&kv);}
  const std::string key=state_identity(&entities[i]);world_spawned(&entities[i],false,0,key);bspList.push_back(&entities[i]);
 }
 make(20,"env_laser_sprite","");make(21,"soundent","");                         // created while the map loads
 make(22,"","");                                                                 // no classname yet: no identity, kept as served
}
}
int main(int argc,char** argv){
 assert(argc==2);gameDir=argv[1];
 globals=globalData.data();verified=true;store(globals,0x90,2);store(globals,0x94,48);now()=1.f;
 engine[14]=reinterpret_cast<void*>(&find_);engine[23]=reinterpret_cast<void*>(&created);engine[28]=reinterpret_cast<void*>(&set_origin);
 engine[64]=reinterpret_cast<void*>(&pd);engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);
 engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[81]=reinterpret_cast<void*>(&print);engine[99]=reinterpret_cast<void*>(&dir_);
 original[7]=reinterpret_cast<void*>(&dsave);original[8]=reinterpret_cast<void*>(&drestore);
 for(auto& e:entities)store(&e,0,1);store(&entities[0],0,0);store(ev(&entities[0]),0,intern("worldspawn"));
 for(int i=1;i<=2;++i){make(i,"player","");}
 // --- Identity and source map.
 assert(state_source("s4_apartment5_e1")=="s4_apartment5"&&state_source("s4_start_e0")=="s4_start"&&state_source("s4_start2")=="s4_start2"&&state_source("s4_x_e")=="s4_x_e"&&state_source("s4_a_eb")=="s4_a_eb");
 // --- First visit: nothing kept yet, the map stays as served.
 spawn_map("s4_test_e1",false);lines.clear();state_activate();
 assert(logged({"MAPSTATE first visit of s4_test"})&&spawnRecords[8].key!=spawnRecords[9].key&&keepKeys.count(spawnRecords[5].key)&&keepKeys.count(spawnRecords[6].key)&&keepKeys.count(spawnRecords[11].key)&&keepKeys.count(spawnRecords[12].key)&&!keepKeys.count(spawnRecords[4].key));
 assert(loadRuntime.size()==2&&loadRuntime[0].key=="rt:env_laser_sprite#0");
 // --- Play: rope taken, rope button killtargeted, zombie dead, bath trigger
 // fired, exit trigger fired, rope model shown, gate toggled, one sprite
 // removed, a monstermaker child alive with its enemy, a player's own monster.
 now()=50.f;
 store(ev(&entities[3]),0x10c,0);store(ev(&entities[3]),0x118,0x80u);
 store(&entities[4],0,1);
 store(ev(&entities[5]),0x164,0.f);store(ev(&entities[5]),0x174,2);
 store(&entities[6],0,1);store(&entities[7],0,1);
 store(ev(&entities[9]),0x164,7.f);store(ev(&entities[12]),0x164,2.f);store(&entities[11],0,1);
 auto child=make(25,"monster_hound","",1,1,1);store(ev(child),0x164,33.f);store(ev(child),0x194,&entities[9]);store(ev(child),0x104,52.5f);
 auto pet=make(26,"monster_snark","");store(ev(pet),0x19c,&entities[1]);
 auto frozenOne=make(27,"monster_alien","");frozen.insert(frozenOne);
 // v0.3 tester log: a quit/error/crash also ends the map; that is never kept.
 lines.clear();state_deactivate();assert(logged({"not kept: the map ended without a level change"})&&!mapStates.count("s4_test"));
 state_level_change("s4_other_e0");lines.clear();state_deactivate();
 assert(mapStates.count("s4_test")&&logged({"MAPSTATE kept s4_test (s4_test_e1): saved=","runtime-monsters=1"})&&logged({"MAPSTATE gone on s4_test: ","ropeheh","ropebutton"}));
 const auto& kept=mapStates.at("s4_test");
 auto removedKey=[&](int i){return std::find(kept.removed.begin(),kept.removed.end(),spawnRecords[i].key)!=kept.removed.end();};
 assert(removedKey(1)&&removedKey(2)&&removedKey(3)&&removedKey(4)&&removedKey(9)&&!removedKey(5)&&!removedKey(6)&&!removedKey(7));
 assert(std::none_of(kept.entities.begin(),kept.entities.end(),[](const StateEntity& x){return x.type=="monster_snark"||x.type=="monster_alien"||x.type=="soundent";}));
 // --- Return through the other entry (_e0, one more player start): the kept
 // state is read back onto the fresh map before ServerActivate.
 frozen.clear();now()=2.f;relinks=0;restores=0;
 spawn_map("s4_test_e0",true);
 // v0.4 load crash: a removed entity queued in Spirit's desired list (trigger_auto
 // Activate: LF_DODESIRED|LF_DESIRED_ACTION at CBaseEntity +0x60) must leave it
 // and must not be Activated (FL_DORMANT); a kept one keeps its flags.
 store(pd(&entities[6]),0x60,0x80u|0x800u|0x1000u);store(pd(&entities[7]),0x60,0x80u|0x800u);
 lines.clear();const int activated=state_activate_edicts(10);
 assert(load<unsigned>(pd(&entities[6]),0x60)==0x1000u&&(load<unsigned>(ev(&entities[6]),0x1a8)&0x80000000u)&&load<unsigned>(pd(&entities[7]),0x60)==(0x80u|0x800u)&&!(load<unsigned>(ev(&entities[7]),0x1a8)&0x80000000u));
 auto killed=[](int i){return (load<unsigned>(ev(&entities[i]),0x1a8)&0x40000000u)!=0&&load<int>(ev(&entities[i]),0x10c)==0&&load<int>(ev(&entities[i]),0x1d0)==0&&load<float>(ev(&entities[i]),0x104)==0;};
 assert(killed(3)&&killed(4)&&killed(5)&&killed(6)&&killed(11)&&!killed(7)&&!killed(8)&&!killed(10)&&!killed(15)&&!killed(22));
 assert(load<float>(ev(&entities[9]),0x164)==7.f&&load<float>(ev(&entities[12]),0x164)==2.f&&load<float>(ev(&entities[7]),0x164)==100.f);
 Edict* again=nullptr;for(int i=30;i<48;++i)if(load<int>(&entities[i],0)==0&&!std::strcmp(classname(&entities[i]),"monster_hound"))again=&entities[i];
 assert(again&&load<float>(ev(again),0x164)==33.f&&load<Edict*>(ev(again),0x194)==&entities[9]&&std::fabs(load<float>(ev(again),0x104)-4.5f)<0.001f);
 assert(logged({"MAPSTATE restored s4_test on s4_test_e0:","removed=5","recreated=1"})&&relinks==restores+5&&restores>0);
 // v0.7.1: ServerActivate must reach the recreated monster, a new edict past the engine's count.
 assert(activated==index(again)+1);
 // --- v0.3 crash audit: an alias that creates another set of load-time entities keeps them as served.
 {const auto keep=mapStates;mapStates.clear();spawn_map("s4_test_e1",false);state_activate();store(ev(&entities[20]),0x164,55.f);state_level_change("s4_other_e0");state_deactivate();
  spawn_map("s4_test_e2",false);make(23,"env_laser_sprite","");lines.clear();state_activate();   // one more sprite: "rt:" names mean other entities
  assert(logged({"load-time entities kept as served"})&&load<float>(ev(&entities[20]),0x164)==100.f&&load<float>(ev(&entities[23]),0x164)==100.f);
  spawn_map("s4_test_e2",false);lines.clear();state_activate();
  assert(!logged({"load-time entities kept as served"})&&load<float>(ev(&entities[20]),0x164)==55.f);
  {std::map<std::string,MapState> back;assert(state_decode(state_encode(mapStates,7u),7u,back)&&back.at("s4_test").layout==mapStates.at("s4_test").layout&&back.at("s4_test").layout!=0);}
  mapStates=keep;}
 // --- Game over: the dying map's state is not kept (the restart is fresh or the save).
 gameOverMap="s4_test_e0";state_level_change("s4_test_e0");lines.clear();const auto before=mapStates.at("s4_test").data;state_deactivate();assert(logged({"not kept: game over"})&&mapStates.at("s4_test").data==before);gameOverMap.clear();
 // --- Kill switch.
 {std::ofstream(std::filesystem::path(gameDir)/"storycoop_mapstate.off")<<"off";spawn_map("s4_test_e1",false);lines.clear();state_activate();assert(logged({"MAPSTATE off"})&&!killed(3));
  std::filesystem::remove(std::filesystem::path(gameDir)/"storycoop_mapstate.off");}
 // --- Slot file: round trip, CRC pairing with the checkpoint, backup fallback.
 {const auto bytes=state_encode(mapStates,0x1234u);std::map<std::string,MapState> back;assert(!bytes.empty()&&state_decode(bytes,0x1234u,back)&&back.size()==1);
  const auto& a=mapStates.at("s4_test");const auto& b=back.at("s4_test");assert(a.data==b.data&&a.tokens==b.tokens&&a.removed==b.removed&&a.entities.size()==b.entities.size()&&b.entities[0].key==a.entities[0].key);
  // A classless entity written by the first v0.3 builds no longer makes the file unreadable.
  {auto legacy=mapStates;auto& s=legacy.at("s4_test");s.entities.push_back({8000,"rt:#0","",0,4});std::map<std::string,MapState> got;
   assert(state_decode(state_encode(legacy,0x1234u),0x1234u,got)&&got.at("s4_test").entities.size()==s.entities.size()-1);}
  assert(!state_decode(bytes,0x1235u,back));auto bad=bytes;bad[bad.size()/2]^=1;assert(!state_decode(bad,0x1234u,back));auto cut=bytes;cut.pop_back();assert(!state_decode(cut,0x1234u,back));
  const auto dir=std::filesystem::path(gameDir)/"storycoop_saves";
  assert(state_commit(dir,2,bytes));const auto older=state_encode(mapStates,0u);assert(state_commit(dir,2,older));
  // Primary now names checkpoint CRC 0, the .bak names 0x1234.
  std::vector<unsigned char> file;assert(state_file(state_path(dir,2),file)&&state_decode(file,0u,back));
  assert(state_file(std::filesystem::path(state_path(dir,2).wstring()+L".bak"),file)&&state_decode(file,0x1234u,back));
  // An invalid (unencodable) checkpoint has CRC 0, so the primary matches it.
  Checkpoint none;assert(state_checkpoint_crc(none)==0);mapStates.clear();lines.clear();state_load_slot(2,none);assert(stateLoaded&&mapStates.count("s4_test")&&logged({"MAPSTATE slot2 loaded: 1 map(s)"}));
  // A slot without a matching file: the timeline is emptied (older save).
  lines.clear();state_load_slot(3,none);assert(!stateLoaded&&mapStates.empty()&&logged({"slot3 has no matching map-state file"}));}
 // --- Older saves: a taken key item nobody holds is left for the lock that still needs it.
 {spawn_map("s4_test_e1",false);lockRecords.clear();lockRecords[&entities[4]]={serial(&entities[4]),"func_button","rope"};
  Checkpoint old;SavedPlayer p;p.identity="profile:a";old.players.push_back(p);stateLoaded=false;
  assert(state_reoffer(spawnRecords[1],old));
  old.players[0].inventory[0]="inventoryitems/rope.txt";assert(!state_reoffer(spawnRecords[1],old));
  old.players[0].inventory[0].clear();stateLoaded=true;assert(!state_reoffer(spawnRecords[1],old));stateLoaded=false;
  store(&entities[4],4,1);assert(!state_reoffer(spawnRecords[1],old));
  assert(!state_reoffer(spawnRecords[2],old));}
 // --- Drowned (v0.3, coop-runtime.hpp): INDEXENT(1) during her own think is her enemy.
 {auto bitch=make(40,"monster_crazybitch","");mem[bitch]=std::vector<unsigned char>(0x400);store(pd(bitch),0x184,&entities[2]);store(pd(bitch),0x188,load<int>(&entities[2],4));
  store(ev(&entities[2]),0x164,80.f);store(ev(&entities[2]),0x174,0);lines.clear();
  assert(drowned_index(1,&entities[1])==&entities[1]);                          // not inside her think
  drowned_begin(bitch);assert(drowned_index(1,&entities[1])==&entities[2]&&drowned_index(2,&entities[2])==&entities[2]&&drowned_index(0,&entities[0])==&entities[0]);
  assert(logged({"DROWNED monster 40 attack victim player=2 (its enemy)"}));
  store(ev(&entities[2]),0x174,2);assert(drowned_index(1,&entities[1])==&entities[1]&&logged({"victim player=1 (no live player enemy"}));store(ev(&entities[2]),0x174,0);
  store(pd(bitch),0x188,1);assert(drowned_index(1,&entities[1])==&entities[1]);store(pd(bitch),0x188,load<int>(&entities[2],4));
  store(pd(bitch),0x184,&entities[9]);assert(drowned_index(1,&entities[1])==&entities[1]);   // enemy not a player
  drowned_end();store(pd(bitch),0x184,&entities[2]);assert(drowned_index(1,&entities[1])==&entities[1]);
  // Roof boss: during its think "the player" is its live player enemy; other monsters never.
  auto roof=make(42,"monster_roofboss","");mem[roof]=std::vector<unsigned char>(0x400);store(pd(roof),0x184,&entities[2]);store(pd(roof),0x188,load<int>(&entities[2],4));
  assert(roofboss_enemy(roof)==&entities[2]&&roofboss_enemy(bitch)==nullptr);
  // Acting player (inventory use, bar puzzle, fan-outs): only the first-player lookup is answered.
  Edict* out=nullptr;assert(!acting_override(nullptr,"classname","player",out));actingPlayer=&entities[2];
  assert(acting_override(nullptr,"classname","player",out)&&out==&entities[2]&&!acting_override(&entities[1],"classname","player",out)&&!acting_override(nullptr,"targetname","player",out)&&!acting_override(nullptr,"classname","monster_zombie",out));
  actingPlayer=nullptr;
  // Two-half items: the game's own card format (inventoryitems/leftplate.txt).
  const std::string card="\"map_name\"\r\n{\r\n\tdata\t\"leftplate\"\r\n}\r\n\r\n\"combined_with\"\r\n{\r\n\tdata\t\"rightplate\"\r\n}\r\n";
  assert(half_value(card,"map_name")=="leftplate"&&half_value(card,"combined_with")=="rightplate"&&half_value(card,"combined_result").empty());
  auto zombie=make(41,"monster_zombie","");drowned_begin(zombie);assert(drowned_index(1,&entities[1])==&entities[1]);drowned_end();drowned_reset();}
 // --- New campaign.
 mapStates["x"]={};stateLoaded=true;state_reset();assert(mapStates.empty()&&!stateLoaded);
 std::puts("PASS map state: BSP identity with ordinals, first visit as served, kept on level change (taken pickup, killtarget, dead monster, fired once-trigger, removed load-time entity), transitions/exits/movewith kept as served, player-owned and frozen monsters not kept, restore onto the other entry alias with removals, recreated play-time monster with remapped reference and rebased time, relink per restored entity, game over not kept, kill switch, slot file round trip/corruption/CRC pairing/.bak fallback, older-save timeline reset and rope re-offer rule, new campaign reset; Drowned victim = her live player enemy only inside her own think. NOT Cry of Fear gameplay verification.");
}
