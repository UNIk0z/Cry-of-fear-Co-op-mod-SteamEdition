// Owned SDK fixture for world-runtime.hpp and campaign-wide checkpoints; no
// original game code is loaded and no game is launched.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <map>
namespace rc4 {struct Edict {std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,32> entities{};
std::unordered_map<Edict*,std::vector<unsigned char>> mem;
std::array<unsigned char,0xa0> globalData{};
std::vector<std::string> strings{"","s2_lake_e0","player"};
std::unordered_map<Edict*,std::map<std::string,std::string>> helpers;
std::vector<std::string> lines;std::string gameDir;GlobalSnapshot state;int teleports=0,nextFree=20;
bool logged(std::initializer_list<const char*> parts){for(const auto& line:lines)if(std::all_of(parts.begin(),parts.end(),[&](const char* s){return line.find(s)!=std::string::npos;}))return true;return false;}
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<32;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<32?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void free_(Edict* e){mem.erase(e);}
void print(const char* s){lines.emplace_back(s);}
std::vector<std::pair<Edict*,Edict*>> touches;void touch_(Edict* e,Edict* other){touches.push_back({e,other});}
void client_print(Edict*,int,const char* s){lines.emplace_back(std::string("CLIENT:")+s);}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:0x1d0;
 for(int i=start?index(start)+1:1;i<32;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0];
}
void set_origin(Edict* e,const float* o){store(ev(e),8,o[0]);store(ev(e),12,o[1]);store(ev(e),16,o[2]);}
std::array<float,3> origin(Edict* e){return {load<float>(ev(e),8),load<float>(ev(e),12),load<float>(ev(e),16)};}
// World: floor plane z=0 (a standing origin is z>=36), a wall at x>1000 and
// a lake (water) at y>5000. Live players are 32x32x72 boxes.
bool solid(const float* p,bool monsters,Edict* skip){
 if(p[0]>1000||p[2]<36)return true;
 if(monsters)for(int i=1;i<=2;++i){auto e=&entities[i];if(e==skip||load<int>(e,0))continue;const auto o=origin(e);if(std::fabs(o[0]-p[0])<32&&std::fabs(o[1]-p[1])<32&&std::fabs(o[2]-p[2])<72)return true;}
 return false;
}
void hull(const float* a,const float* b,int noMonsters,int hullNumber,Edict* skip,Trace* t){
 assert(hullNumber==1);*t={};
 if(solid(a,!noMonsters,skip)){t->startSolid=1;t->allSolid=solid(b,!noMonsters,skip);return;}
 t->fraction=1;
 if(b[2]<a[2]){if(b[2]<36){t->fraction=(a[2]-36)/(a[2]-b[2]);}return;}
 if((a[0]<=1000)!=(b[0]<=1000))t->fraction=0.5f;
}
int contents(const float* p){return p[1]>5000?-3:-1;}
Edict* created(int type){auto e=&entities[nextFree++];assert(nextFree<32);*e={};store(e,0,0);store(e,4,500+nextFree);store(ev(e),0,type);return e;}
void remove_(Edict* e){release(e);store(e,0,1);helpers.erase(e);}
int spawn_(Edict*){return 0;}
void key_(Edict* e,KeyValue* kv){
 helpers[e][kv->key]=kv->value;kv->handled=1;
 if(!std::strcmp(kv->key,"targetname"))store(ev(e),0x1d0,intern(kv->value));
 if(!std::strcmp(kv->key,"origin")){float x=0,y=0,z=0;assert(std::sscanf(kv->value,"%f %f %f",&x,&y,&z)==3);store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,z);}
 if(!std::strcmp(kv->key,"angles")){float x=0,y=0,z=0;assert(std::sscanf(kv->value,"%f %f %f",&x,&y,&z)==3);store(ev(e),0x54,y);}
}
// Native cof_teleport: target origin + mins.z(-36) + 1, angles copied.
int cameraToggles=0;std::vector<std::string> fired;
void use_(Edict* h,Edict* actor){
 if(!std::strcmp(classname(h),"trigger_camera")){store(ev(h),0x248,load<int>(ev(h),0x248)?0:1);++cameraToggles;return;}   // v0.4 skip: native toggle
 if(!std::strcmp(classname(h),"multi_manager")){fired.push_back(std::string("use ")+targetname(h));return;}   // v0.5 prologue skip
 assert(!std::strcmp(classname(h),"cof_teleport")&&helpers[h]["target"]=="sc4_coop_landing");
 auto marker=find_next(nullptr,"targetname","sc4_coop_landing");assert(marker&&!std::strcmp(classname(marker),"info_teleport_destination"));
 store(ev(actor),8,load<float>(ev(marker),8));store(ev(actor),12,load<float>(ev(marker),12));store(ev(actor),16,load<float>(ev(marker),16)+37.f);store(ev(actor),0x54,load<float>(ev(marker),0x54));++teleports;
}
template<class T>void field(int token,T value){std::uint16_t n=sizeof(value),t=static_cast<std::uint16_t>(token);auto add=[](const void* p,std::size_t len){auto a=static_cast<const unsigned char*>(p);state.bytes.insert(state.bytes.end(),a,a+len);};add(&n,2);add(&t,2);add(&value,sizeof(value));}
void minimal_globals(){state={};state.tokens.resize(1024);state.tokens[1]="GLOBAL";state.tokens[2]="m_listCount";field(1,1);field(2,0);}
void save_(SaveData* d){std::memcpy(d->base,state.bytes.data(),state.bytes.size());d->size=static_cast<int>(state.bytes.size());d->current=d->base+d->size;for(std::size_t i=0;i<1024;++i)if(!state.tokens[i].empty())d->tokens[i]=state.tokens[i].data();}
void restore_(SaveData* d){state.bytes.assign(reinterpret_cast<unsigned char*>(d->base),reinterpret_cast<unsigned char*>(d->base)+d->capacity);state.tokens.clear();for(int i=0;i<1024;++i)state.tokens.emplace_back(d->tokens[i]?d->tokens[i]:"");}
void empty(){}
// Fake CMultiManager::ManagerThink (v0.4 skip): fires every target whose delay has passed, in order.
void think_(Edict* e){
 if(std::strcmp(classname(e),"multi_manager"))return;void* p=pd(e);const float t=load<float>(globalData.data(),0)-load<float>(p,0xe8);int i=load<int>(p,0xe4);
 while(i<load<int>(p,0xe0)&&load<float>(p,0x12c+i*4)<=t)fired.emplace_back(str(load<int>(p,0xec+4*i++)));
 store(p,0xe4,i);if(i>=load<int>(p,0xe0)){store(p,0xdc,0);store(ev(e),0x104,0.f);}
}
void make(int i,const char* type,const char* name,int serialValue);
Edict* manager(int slot,const char* name,std::initializer_list<std::pair<const char*,float>> list,int index,float start){
 make(slot,"multi_manager",name,600+slot);auto e=&entities[slot];allocate(e,0x190);store(pd(e),4,ev(e));store(pd(e),0xdc,2);store(pd(e),0xe0,static_cast<int>(list.size()));store(pd(e),0xe4,index);store(pd(e),0xe8,start);
 int i=0;for(const auto& [t,d]:list){store(pd(e),0xec+i*4,intern(t));store(pd(e),0x12c+i*4,d);++i;}store(ev(e),0x104,start+1.f);return e;
}
int uid(Edict* e){return index(e)+10;}
const char* auth(Edict*){return "STEAM_ID_LAN";}
int listen(){return 0;}
void dir_(char* p){std::strcpy(p,gameDir.c_str());}
std::array<std::string,3> argv_{};const char* arg(int i){return i>=0&&i<3?argv_[i].c_str():"";}
std::vector<std::string> wire;
void begin_(int dest,int type,const float*,Edict* e){wire.push_back("begin "+std::to_string(dest)+" "+std::to_string(type)+" "+std::to_string(index(e)));}void end_(){wire.emplace_back("end");}void byte_(int n){wire.push_back("byte "+std::to_string(n));}void short_(int n){wire.push_back("short "+std::to_string(n));}void string_(const char* s){wire.push_back(std::string("string ")+(s?s:""));}
void make(int i,const char* type,const char* name,int serialValue){auto e=&entities[i];*e={};store(e,0,0);store(e,4,serialValue);store(ev(e),0,intern(type));store(ev(e),0x1d0,intern(name));}
void place(Edict* e,float x,float y,float z){store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,z);}
void player(int i){make(i,"player","",100+i);allocate(&entities[i],PlayerBytes);allocations.at(&entities[i]).commands=true;store(ev(&entities[i]),0x164,90.f);store(pd(&entities[i]),0x21f0,50.f);}
// The served map, in BSP lump order (index 0 worldspawn). Edict slots differ
// from BSP indices exactly as in the engine (players occupy edicts 1..2).
void spawn_map(){
 for(int i=3;i<20;++i){entities[i]={};store(&entities[i],0,1);}
 make(3,"item_key","",31);place(&entities[3],200,0,36);store(ev(&entities[3]),0x10c,1);
 make(4,"weapon_glock","",32);store(ev(&entities[4]),0x1cc,intern("pickup_scare"));store(ev(&entities[4]),0x10c,1);
 make(5,"inter_door","key_door",33);allocate(&entities[5],0x1b0);store(pd(&entities[5]),0x190,intern("The door is locked."));
 make(6,"inter_door","door_b",34);allocate(&entities[6],0x1b0);store(pd(&entities[6]),0x190,intern("Locked."));
 make(7,"cof_interdooronoff","",35);store(ev(&entities[7]),0x1d8,intern("door_b"));
 make(8,"tape_recorder","",36);place(&entities[8],0,0,20);
 make(9,"ammo_glock","",37);store(ev(&entities[9]),0x118,0x80u);
 make(10,"boat","",38);make(11,"boat_exit","",39);place(&entities[11],500,0,36);
 world_spawned(&entities[0],true,0);for(int i=3;i<=11;++i)world_spawned(&entities[i],false,0);
}
}
// v0.3 map state: entities that write nothing (every one FCAP_DONT_SAVE).
void entity_save_(Edict*,SaveData*){}
int main(int argc,char** argv){
 assert(argc==2);gameDir=argv[1];
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,32);
 engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&created);engine[28]=reinterpret_cast<void*>(&set_origin);
 engine[34]=reinterpret_cast<void*>(&hull);engine[45]=reinterpret_cast<void*>(&contents);
 engine[46]=reinterpret_cast<void*>(&begin_);engine[47]=reinterpret_cast<void*>(&end_);engine[48]=reinterpret_cast<void*>(&byte_);engine[50]=reinterpret_cast<void*>(&short_);engine[54]=reinterpret_cast<void*>(&string_);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[65]=reinterpret_cast<void*>(&free_);engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);
 engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[80]=reinterpret_cast<void*>(&client_print);engine[81]=reinterpret_cast<void*>(&print);engine[83]=reinterpret_cast<void*>(&arg);
 engine[99]=reinterpret_cast<void*>(&dir_);engine[113]=reinterpret_cast<void*>(&uid);engine[115]=reinterpret_cast<void*>(&listen);engine[143]=reinterpret_cast<void*>(&auth);
 original[2]=reinterpret_cast<void*>(&think_);original[1]=reinterpret_cast<void*>(&spawn_);original[3]=reinterpret_cast<void*>(&use_);original[6]=reinterpret_cast<void*>(&key_);original[12]=reinterpret_cast<void*>(&save_);original[7]=reinterpret_cast<void*>(&entity_save_);original[13]=reinterpret_cast<void*>(&restore_);original[25]=reinterpret_cast<void*>(&empty);
 configure_puzzle({&scoped,&player_memory,&serial,&classname,&resolve,&solved});
 for(auto& e:entities)store(&e,0,1);store(&entities[0],0,0);store(ev(&entities[0]),0,intern("worldspawn"));
 // --- Map names: every RC2/RC4 alias except the passive nightmare.
 assert(checkpoint_map("s2_lake_e0")&&checkpoint_map("s4_apartment1_e3")&&checkpoint_map("s4_start_e0")&&legacy_checkpoint_map("s4_start_e0"));
 assert(!checkpoint_map("s4_nightmare")&&!checkpoint_map("c_lake")&&!checkpoint_map("S2_LAKE")&&!checkpoint_map("s2_lake e0")&&!legacy_checkpoint_map("s2_lake_e0"));
 // --- Registry and world state. A pickup with an authored target, a pickup
 // hidden at spawn and a story-toggled door are never touched on load.
 player(1);player(2);place(&entities[1],0,40,36);place(&entities[2],-3000,0,36);
 spawn_map();world_activate();assert(spawnRecords.size()==10&&spawnRecords[1].pickup&&!spawnRecords[2].pickup&&!spawnRecords[7].pickup&&spawnRecords[3].locked&&spawnRecords[4].locked&&toggledDoors.count("door_b"));
 assert(spawn_index(&entities[8])==6);
 store(&entities[3],0,1);store(&entities[4],0,1);store(&entities[9],0,1);store(pd(&entities[5]),0x190,0);store(pd(&entities[6]),0x190,0);
 Checkpoint captured;world_capture(captured);assert(captured.removed.size()==1&&captured.removed[0].first==1&&captured.removed[0].second=="item_key"&&captured.unlocked==std::vector<int>{3});
 // A pickup taken into an inventory (unsolid + no draw) counts as taken too.
 store(&entities[3],0,0);store(ev(&entities[3]),0x10c,0);store(ev(&entities[3]),0x118,0x80u);world_capture(captured);assert(captured.removed.size()==1);
 spawn_map();world_activate();lines.clear();world_apply(captured);
 assert((load<unsigned>(ev(&entities[3]),0x1a8)&0x40000000u)&&load<int>(ev(&entities[3]),0x10c)==0&&(load<unsigned>(ev(&entities[3]),0x118)&0x80u));
 assert(load<int>(pd(&entities[5]),0x190)==0&&load<int>(pd(&entities[6]),0x190)!=0&&logged({"WORLD restored removed-pickups=1 unlocked-doors=1 skipped=0"}));
 // Changed map contents at that index are skipped, never removed blindly.
 spawn_map();make(3,"item_battery","",31);spawnRecords[1].type="item_battery";lines.clear();world_apply(captured);assert(!(load<unsigned>(ev(&entities[3]),0x1a8)&0x40000000u)&&logged({"skipped=1"}));
 // --- v0.5 RC-2048: the deathmatch cvar reads 0 while a story map's entities
 // load (the engine keeps spawnflags-2048 entities) and is restored at
 // ServerActivate; saves keep indexing the other entities as v0.4 did.
 {static std::array<unsigned char,20> cvar{};store(cvar.data(),12,1.f);
  engine[116]=reinterpret_cast<void*>(+[](const char* n)->void*{return std::strcmp(n,"deathmatch")?nullptr:cvar.data();});
  deathmatch_open();assert(load<float>(cvar.data(),12)==0.f);
  spawn_map();make(12,"func_button","lighter_button",40);
  world_spawned(&entities[0],true,0);world_spawned(&entities[12],false,0,{},true);for(int i=3;i<=11;++i)world_spawned(&entities[i],false,0);
  assert(spawnRecords[1].dm&&legacy_index(1)==-1&&legacy_index(2)==1&&record_index(1)==2&&spawn_index(&entities[8])==6);
  lines.clear();deathmatch_close();assert(load<float>(cvar.data(),12)==1.f&&logged({"SPAWNFLAGS kept 1 entity"}));
  deathmatch_close();assert(load<float>(cvar.data(),12)==1.f);   // closing twice changes nothing
  world_activate();lines.clear();world_apply(captured);
  assert((load<unsigned>(ev(&entities[3]),0x1a8)&0x40000000u)&&!(load<unsigned>(ev(&entities[12]),0x1a8)&0x40000000u)&&logged({"removed-pickups=1"}));
  Checkpoint again;world_capture(again);assert(again.removed.size()==1&&again.removed[0].first==1);
  engine[116]=nullptr;entities[12]={};store(&entities[12],0,1);}
 // --- Safe landings: occupied anchor -> ring; never through a wall or into water.
 std::array<float,3> at{};place(&entities[2],0,0,36);assert(find_landing({0,0,36},&entities[1],at)&&!(std::fabs(at[0])<32&&std::fabs(at[1])<32));
 assert(find_landing({990,0,36},&entities[1],at)&&at[0]<=1000);
 assert(find_landing({0,4990,36},&entities[1],at)&&at[1]<=5000);
 assert(!find_landing({2000,0,36},&entities[1],at));
 // --- Boat: the rower docks; the partner far away joins beside them.
 spawn_map();place(&entities[2],-3000,0,36);place(&entities[1],0,0,36);lines.clear();
 auto p1=pd(&entities[1]);store(p1,0x2500,8);store(p1,0x380,reinterpret_cast<std::uint32_t>(&entities[10]));store(p1,0x384,serial(&entities[10]));
 boat_frame(&entities[1]);assert(logged({"BOAT embarked player=1"})&&logged({"CLIENT:","partner is rowing"}));
 store(p1,0x2500,0);store(p1,0x380,0u);place(&entities[1],500,0,36);teleports=0;boat_frame(&entities[1]);
 assert(teleports==1&&logged({"BOAT docked player=1; partners moved=1 failed=0"}));const auto p2=origin(&entities[2]);assert(std::fabs(p2[0]-500)<=150&&std::fabs(p2[1])<=150&&p2[2]==36);
 assert(find_next(nullptr,"targetname","sc4_coop_landing")==nullptr);
 // A partner already on the dock is not moved; leaving away from a dock moves nobody.
 store(p1,0x2500,8);store(p1,0x380,reinterpret_cast<std::uint32_t>(&entities[10]));boat_frame(&entities[1]);store(p1,0x2500,0);teleports=0;boat_frame(&entities[1]);assert(teleports==0);
 store(p1,0x2500,8);boat_frame(&entities[1]);store(p1,0x2500,0);place(&entities[1],-800,0,36);place(&entities[2],-3000,0,36);lines.clear();boat_frame(&entities[1]);assert(teleports==0&&logged({"away from a dock"}));
 // --- Saving at ANY recorder of a later alias, with poses and world state.
 spawn_map();world_activate();minimal_globals();saveLoadMessage=8;screenFadeMessage=9;saveWindowEnabled=false;   // the key list path (the v0.7 window: test-checkpoint-runtime)
 place(&entities[1],40,0,36);store(ev(&entities[1]),0x54,90.f);place(&entities[2],-200,300,36);store(&entities[3],0,1);
 message_begin(1,8,nullptr,&entities[1]);write_byte(0);write_string("Lake");message_end();checkpoint_start_frame();assert(saveChoice.recorder==6&&logged({"CLIENT:SAVE GAME"}));
 save_choice_commit(1);assert(requestedRecorder==6);checkpoint_start_frame();   // the host pressed 1
 Checkpoint saved;bool backup=false;assert(read_checkpoint(checkpoint_directory(),1,saved,backup)&&!backup);
 assert(saved.map=="s2_lake_e0"&&saved.recorder==6&&saved.players.size()==2&&saved.removed.size()==1&&saved.players[0].hasPose&&saved.players[0].origin[0]==40&&saved.players[0].angles[1]==90.f);
 // A save on a recorder-less spot of this alias is refused, nothing written.
 place(&entities[1],900,0,36);std::filesystem::remove(checkpoint_directory()/"slot1.cofsave");lines.clear();
 message_begin(1,8,nullptr,&entities[1]);write_byte(0);write_string("Lake");message_end();checkpoint_start_frame();assert(!std::filesystem::exists(checkpoint_directory()/"slot1.cofsave")&&logged({"Recorder checkpoint unavailable here"}));
 assert(commit_checkpoint(checkpoint_directory(),1,saved));
 // --- Continue: fresh world, saved pickups removed, both players land at
 // their own saved spots (the guest's spot taken -> nearest safe spot).
 allocations.clear();mem.clear();playerBank.clear();restoredPlayers.clear();loadingCheckpoint=false;checkpointRestoreFailed=false;playerRestoreFailed=false;
 player(1);player(2);place(&entities[1],0,-2000,36);place(&entities[2],-200,300,36);spawn_map();
 Checkpoint loaded;assert(read_checkpoint(checkpoint_directory(),1,loaded,backup));loaded.players[1].origin={60,10,36};assert(checkpoint_stage(loaded,false,1)&&loadingCheckpoint);
 lines.clear();checkpoint_world_activate();assert(logged({"WORLD restored removed-pickups=1"})&&(load<unsigned>(ev(&entities[3]),0x1a8)&0x40000000u));
 teleports=0;checkpoint_player_frame(&entities[1]);checkpoint_player_frame(&entities[2]);
 assert(teleports==2&&logged({"CHECKPOINT restored player=1"})&&logged({"CHECKPOINT restored player=2"}));
 const auto host=origin(&entities[1]);assert(host[0]==40&&host[1]==0&&host[2]==36&&load<float>(ev(&entities[1]),0x54)==90.f);
 const auto guest=origin(&entities[2]);assert(!(std::fabs(guest[0]-host[0])<32&&std::fabs(guest[1]-host[1])<32)&&std::fabs(guest[0]-60)<=150&&std::fabs(guest[1]-10)<=150);
 // The first recorder Use after Continue saves (no first-encounter line again).
 assert(load<unsigned char>(pd(&entities[1]),0x21d8)==1&&load<unsigned char>(pd(&entities[2]),0x21d8)==1);
 // --- Format: a non-c_start save without poses is never written.
 auto bad=saved;bad.players[1].hasPose=false;assert(encode_checkpoint(bad).empty());
 bad=saved;bad.recorder=0;assert(encode_checkpoint(bad).empty());bad=saved;bad.map="s4_nightmare";assert(encode_checkpoint(bad).empty());
 bad=saved;bad.removed.push_back({1,"item_key"});assert(encode_checkpoint(bad).empty());
 // --- Shared keypad code: a spawn default is never adopted; a native change
 // on ANY player (activator-only/first-player generators) reaches everyone,
 // also a later joiner; checkpoint schema 3+ carries it; a new campaign clears it.
 keypad_reset();store(pd(&entities[1]),0x2040,9999);store(pd(&entities[2]),0x2040,1234);keypad_frame();
 assert(!keypadShared.have&&load<int>(pd(&entities[2]),0x2040)==1234);
 store(pd(&entities[2]),0x2040,2795);store(pd(&entities[2]),0x2044,1);lines.clear();keypad_frame();
 assert(keypadShared.have&&keypadShared.code==2795&&load<int>(pd(&entities[1]),0x2040)==2795&&load<int>(pd(&entities[1]),0x2044)==1&&logged({"KEYPAD shared code=2795 type=1 from player=2"}));
 keypad_map_changed();store(pd(&entities[1]),0x2040,9999);store(pd(&entities[1]),0x2044,0);keypad_frame();assert(load<int>(pd(&entities[1]),0x2040)==2795&&keypadShared.code==2795);
 store(pd(&entities[1]),0x2040,5247);store(pd(&entities[1]),0x2044,2);keypad_frame();assert(load<int>(pd(&entities[2]),0x2040)==5247&&load<int>(pd(&entities[2]),0x2044)==2);
 {auto withKeypad=saved;keypad_capture(withKeypad);assert(withKeypad.haveKeypad&&withKeypad.keypadCode==5247);const auto bytes=encode_checkpoint(withKeypad);assert(!bytes.empty()&&bytes[8]==8);
  Checkpoint back;assert(decode_checkpoint(bytes,back)&&back.haveKeypad&&back.keypadCode==5247&&back.keypadType==2);keypad_reset();keypad_load(back);assert(keypadShared.have&&keypadShared.code==5247);
  auto wrong=withKeypad;wrong.keypadCode=10000;assert(encode_checkpoint(wrong).empty());}
 keypad_reset();assert(!keypadShared.have);
 // --- Story text: a ProFont subtitle sent to the first player right after a
 // first-player lookup reaches every player; a text to the acting player does not.
 proFontMessage=31;wire.clear();lines.clear();firstPlayerLookup=false;find(nullptr,"classname","player");assert(firstPlayerLookup);
 message_begin(8,31,nullptr,&entities[1]);write_byte(5);write_string("Simon: where am I?");message_end();
 assert((wire==std::vector<std::string>{"begin 8 31 1","byte 5","string Simon: where am I?","end","begin 8 31 2","byte 5","string Simon: where am I?","end"}));assert(logged({"STORYTEXT"}));
 firstPlayerLookup=false;wire.clear();message_begin(8,31,nullptr,&entities[2]);write_string("It is locked.");message_end();assert(wire.size()==3);
 // --- Objectives (v0.5): the text the native code sets on any player reaches every
 // player, with or without a custom hints file; a rebuilt player gets it again.
 {objective_session_reset();store(pd(&entities[1]),0x1e90,0);store(pd(&entities[2]),0x1e90,0);store(pd(&entities[1]),0x420,0);store(pd(&entities[2]),0x420,0);objective_share();   // first sight
  store(pd(&entities[1]),0x1e90,intern("OBJ_FIND_THE_KEY"));lines.clear();objective_share();
  assert(!std::strcmp(str(load<int>(pd(&entities[2]),0x1e90)),"OBJ_FIND_THE_KEY")&&logged({"OBJECTIVE OBJ_FIND_THE_KEY from player=1"})&&load<int>(pd(&entities[2]),0x420)==0);
  store(pd(&entities[1]),0x1e90,intern("OBJ_ESCAPE"));store(pd(&entities[1]),0x420,intern("txtfiles/custom.txt"));objective_share();
  assert(!std::strcmp(str(load<int>(pd(&entities[2]),0x1e90)),"OBJ_ESCAPE")&&!std::strcmp(str(load<int>(pd(&entities[2]),0x420)),"txtfiles/custom.txt"));
  objective_reset();store(pd(&entities[2]),0x1e90,0);objective_share();objective_share();assert(!std::strcmp(str(load<int>(pd(&entities[2]),0x1e90)),"OBJ_ESCAPE"));   // new map, rebuilt player
  objective_session_reset();}
 // --- coop-fixes-013 "stuck": typed in chat, the player is moved to a clear
 // spot beside the other living player; once per 15 s; other chat untouched.
 nextFree=20;place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);store(globalData.data(),0,50.f);lines.clear();teleports=0;
 argv_={"say","hello",""};stuck_command(&entities[2],"say");assert(teleports==0&&!logged({"STUCK"}));
 argv_={"say","stuck",""};stuck_command(&entities[2],"say");assert(teleports==1&&logged({"STUCK player=2 moved beside the partner"})&&logged({"CLIENT:Moved next to your partner."}));
 {const auto o=origin(&entities[2]);assert(std::fabs(o[0])<=150&&std::fabs(o[1])<=150&&!(std::fabs(o[0])<32&&std::fabs(o[1])<32));}
 place(&entities[2],-3000,0,36);argv_={"say_team","/Stuck",""};stuck_command(&entities[2],"say_team");assert(teleports==1&&logged({"CLIENT:Wait a moment"}));
 store(globalData.data(),0,70.f);stuck_command(&entities[2],"say_team");assert(teleports==2);
 store(ev(&entities[2]),0x164,0.f);store(globalData.data(),0,90.f);lines.clear();stuck_command(&entities[2],"say_team");assert(teleports==2&&logged({"CLIENT:Can't move you right now."}));store(ev(&entities[2]),0x164,90.f);
 argv_={};
 // --- coop-fixes-013 nightmare camera rooms: after a right photo (multispotN)
 // the partner far from the photographer is moved beside them; a near one stays.
 store(globalData.data(),0x0c,intern("s4_nightmare"));make(12,"multi_manager","multispot2",40);place(&entities[12],20,0,51);
 nextFree=20;place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);teleports=0;lines.clear();
 regroup_lookup(nullptr,"targetname","wrong1",&entities[12]);regroup_frame();assert(teleports==0);
 regroup_lookup(nullptr,"targetname","multispot2",&entities[12]);regroup_frame();assert(teleports==1&&logged({"REGROUP multispot2: player=2 moved beside player=1"})&&logged({"CLIENT:Moved to your partner"}));
 regroup_lookup(nullptr,"targetname","multispot2",&entities[12]);regroup_frame();assert(teleports==1);
 store(globalData.data(),0x0c,1);regroup_lookup(nullptr,"targetname","multispot2",&entities[12]);assert(!nightmareRegroup.pending);store(&entities[12],0,1);
 // --- v0.2: a scene teleport of one player (s4_apartment1 elevator trigger_cofteleport) brings the
 // partner who watches the same scene camera; two players put on one point are separated.
 {const int mapBefore=load<int>(globals,0x0c);store(globals,0x0c,intern("s4_apartment1_e0"));
  nextFree=20;place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);teleports=0;lines.clear();lastPlaces.clear();cameraLeases.clear();store(globalData.data(),0,200.f);
  auto cam=created(intern("trigger_camera"));allocate(cam,0x154);store(ev(cam),0x248,1);
  CameraLease lease;lease.owner=camera_ref(cam);lease.view=camera_ref(cam);for(int i:{1,2}){lease.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=lease;}
  scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);assert(teleports==0);
  place(&entities[1],-600,0,36);scene_teleport_frame(&entities[1]);
  assert(teleports==1&&logged({"SCENE_TELEPORT player=1"})&&logged({"partners moved=1 aside=0 failed=0"})&&std::fabs(origin(&entities[2])[0]+600)<200);
  scene_teleport_frame(&entities[2]);assert(teleports==1);   // our own move is not followed back
  store(globalData.data(),0,210.f);place(&entities[1],0,0,36);place(&entities[2],40,0,36);scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);
  place(&entities[1],-600,0,36);place(&entities[2],-600,0,36);lines.clear();scene_teleport_frame(&entities[1]);
  {const auto a=origin(&entities[1]),b=origin(&entities[2]);assert(teleports==2&&logged({"aside=1"})&&(std::fabs(a[0]-b[0])>=32||std::fabs(a[1]-b[1])>=32));}
  // Without a scene camera (door relocation, arrival) nobody follows.
  cameraLeases.clear();store(globalData.data(),0,220.f);scene_teleport_frame(&entities[2]);place(&entities[2],-3000,0,36);scene_teleport_frame(&entities[2]);assert(teleports==2);
  // --- v0.4: s4_start_e0 multiscene3 (Katz's dark box): staging teleport while the camera
  // runs, exit teleport in the frame the camera ends; a return long after the cameras
  // (subway2st3); the follow ends with the grace; the activator leaving sends the partner home.
  {nextFree=21;lastPlaces.clear();sceneSeen.clear();sceneFollowers.clear();cameraLeases.clear();teleports=0;lines.clear();store(ev(cam),0x248,1);
   CameraLease l2;l2.owner=camera_ref(cam);l2.view=camera_ref(cam);for(int i:{1,2}){l2.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l2;}
   store(globalData.data(),0,300.f);place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);
   place(&entities[1],-600,0,36);scene_teleport_frame(&entities[1]);assert(teleports==1&&sceneFollowers.count(&entities[2]));
   store(globalData.data(),0,315.f);scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);
   store(ev(cam),0x248,0);store(globalData.data(),0,315.4f);place(&entities[1],500,0,36);lines.clear();scene_teleport_frame(&entities[1]);
   assert(teleports==2&&std::fabs(origin(&entities[2])[0]-500)<200&&logged({"SCENE_TELEPORT player=1 moved as its scene ended,","partners moved=1"}));
   store(globalData.data(),0,331.f);scene_teleport_frame(&entities[2]);place(&entities[1],-1500,0,36);lines.clear();scene_teleport_frame(&entities[1]);
   assert(teleports==3&&std::fabs(origin(&entities[2])[0]+1500)<200&&logged({"moved after its scene,","partners moved=1"}));
   store(globalData.data(),0,400.f);scene_teleport_frame(&entities[2]);assert(!sceneFollowers.count(&entities[2]));
   place(&entities[1],0,0,36);scene_teleport_frame(&entities[1]);assert(teleports==3);
   // A partner who walked away from where the scene put them is free again.
   nextFree=21;store(ev(cam),0x248,1);store(globalData.data(),0,500.f);lastPlaces.clear();place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);
   place(&entities[1],-600,0,36);scene_teleport_frame(&entities[1]);assert(teleports==4);
   {auto o=origin(&entities[2]);place(&entities[2],o[0],o[1]+150,36);}store(globalData.data(),0,501.f);scene_teleport_frame(&entities[2]);assert(!sceneFollowers.count(&entities[2]));
   // The activator leaves the game before the exit: the partner goes back where they stood.
   store(globalData.data(),0,600.f);lastPlaces.clear();sceneFollowers.clear();place(&entities[1],0,0,36);place(&entities[2],-3000,0,36);scene_teleport_frame(&entities[1]);scene_teleport_frame(&entities[2]);
   place(&entities[1],-600,0,36);scene_teleport_frame(&entities[1]);assert(teleports==5);
   store(&entities[1],0,1);lines.clear();scene_teleport_frame(&entities[2]);store(&entities[1],0,0);
   assert(teleports==6&&std::fabs(origin(&entities[2])[0]+3000)<200&&logged({"left; back where they stood before the scene"})&&!sceneFollowers.count(&entities[2]));
   sceneSeen.clear();sceneFollowers.clear();cameraLeases.clear();}
  // --- v0.4 vote to skip: SPACE (IN_JUMP) votes; N/N fast-forwards the running manager (remaining
  // targets in order) and ends the camera with one toggle; a scene with a time-based target is not offered.
  {store(ev(cam),0x1d0,intern("ckamera1"));store(ev(cam),0x248,1);store(pd(cam),0x124,1);store(pd(cam),0x104,5.f);store(pd(cam),0x108,716.f);make(13,"trigger_cofmobile","sms3hehe",613);make(14,"cof_teleport","2teleport",614);
   store(globalData.data(),0,700.f);auto mm=manager(15,"multiscene3",{{"ckamera1",0.5f},{"sms3hehe",6.5f},{"2teleport",15.4f}},1,699.5f);
   CameraLease l3;l3.owner=camera_ref(cam);l3.view=camera_ref(cam);for(int i:{1,2}){l3.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l3;}
   lines.clear();wire.clear();fired.clear();cameraToggles=0;skipScene={};skip_frame();
   assert(logged({"SKIP offered for multiscene3 (1 manager(s))"})&&std::count(wire.begin(),wire.end(),std::string("begin 1 23 1"))==1&&std::count(wire.begin(),wire.end(),std::string("begin 1 23 2"))==1);
   std::array<unsigned char,0x34> cmd{};auto press=[&](int i,bool down){unsigned short b=down?0x2000:0;std::memcpy(cmd.data()+0x1e,&b,2);skip_command(&entities[i],cmd.data());};
   press(1,true);press(1,true);press(1,false);press(1,true);assert(skipScene.votes.size()==1&&logged({"SKIP vote (usercmd) player=1 on multiscene3: 1/2"}));
   skip_frame();assert(fired.empty()&&load<int>(ev(cam),0x248)==1);                 // 1/2: nothing yet
   // v0.5.5: R in the player's own game arrives as the client command "sc_skip".
   assert(!skip_client_command(&entities[2],"other")&&skip_client_command(&entities[2],"sc_skip")&&logged({"SKIP vote (R) player=2 on multiscene3: 2/2"}));skip_frame();
   assert((fired==std::vector<std::string>{"sms3hehe","2teleport"})&&cameraToggles==0&&load<float>(pd(cam),0x108)<700.f&&load<int>(pd(mm),0xdc)==0);   // v0.5.7: ended by its return time, never toggled
   assert(logged({"SKIP multiscene3 jumped"})&&logged({"s to the scene's end: 2 target(s) fired in order, 1 camera(s) ended"})&&!skipScene.active);
   // v0.7 (Playtest 3 clip 11): a timer after the scene (the lever doors closing 41 s later) keeps its time.
   store(ev(cam),0x248,1);store(pd(cam),0x108,704.f);make(16,"func_door","doorshut",616);mm=manager(15,"multilever",{{"ckamera1",0.f},{"sms3hehe",1.f},{"doorshut",41.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();press(1,false);press(1,true);press(2,false);press(2,true);skip_frame();
   assert((fired==std::vector<std::string>{"sms3hehe"})&&load<int>(pd(mm),0xe4)==2&&std::fabs(load<float>(pd(mm),0xe8)+41.f-(700.f+36.f))<0.2f&&!skipScene.active);
   // v0.5.5: a scene that still moves a train is offered too; the train is started in order and runs on.
   store(ev(cam),0x248,1);store(pd(cam),0x108,716.f);store(&entities[16],0,1);make(16,"func_train","tbtrain",616);mm=manager(15,"multitscene2",{{"ckamera1",0.f},{"tbtrain",3.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();assert(logged({"SKIP offered for multitscene2 (1 manager(s)); runs on after a skip: tbtrain (func_train)"}));
   press(1,false);press(1,true);press(2,false);press(2,true);skip_frame();assert((fired==std::vector<std::string>{"tbtrain"})&&!skipScene.active);
   store(ev(cam),0x248,1);skipScene={};skipAfter={};
   // v0.5.7: a camera already switched off (m_state 0) is ending: not offered, never toggled back on.
   store(pd(cam),0x124,0);lines.clear();cameraToggles=0;skip_frame();assert(!skipScene.active&&cameraToggles==0&&!logged({"SKIP offered"}));store(pd(cam),0x124,1);
   // v0.5.7: a scene that still shows a yes/no choice is not offered, so the choice always comes up.
   make(17,"cof_yesno","yesno",617);mm=manager(15,"multits3",{{"ckamera1",0.f},{"yesno",2.f}},1,699.f);lines.clear();skipScene={};skip_frame();
   assert(skipScene.active&&!skipScene.allowed&&logged({"SKIP not offered for multits3: its yes/no choice (yesno) comes up first"}));skipScene={};
   // v0.7.1 (Katz: "skip didnt work mid cutscene"): between two cameras the scene goes on, votes stay and R counts.
   auto r=[&](int i){return skip_client_command(&entities[i],"sc_skip");};
   make(16,"func_door","doorshut",616);store(ev(cam),0x248,1);store(pd(cam),0x108,701.f);
   mm=manager(15,"multigap",{{"ckamera1",0.f},{"ckamera1",3.f},{"sms3hehe",3.5f},{"doorshut",41.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();assert(skipScene.allowed&&!skipScene.everyone);
   r(1);assert(skipScene.votes.size()==1);
   store(ev(cam),0x248,0);skip_frame();assert(skipScene.active&&skipScene.everyone&&skipScene.votes.size()==1&&fired.empty());
   r(2);assert(logged({"SKIP vote (R) player=2 on multigap: 2/2"}));skip_frame();
   assert(!skipScene.active&&std::count(fired.begin(),fired.end(),std::string("sms3hehe"))==1&&std::count(fired.begin(),fired.end(),std::string("doorshut"))==0);
   // The last camera is over: the prompt goes after a moment, and R there never fires the timers left.
   store(ev(cam),0x248,1);store(pd(cam),0x108,701.f);mm=manager(15,"multilast",{{"ckamera1",0.f},{"doorshut",41.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();
   store(ev(cam),0x248,0);skip_frame();assert(skipScene.active&&skipScene.everyone);r(1);r(2);skip_frame();assert(fired.empty());
   store(globalData.data(),0,700.6f);skip_frame();assert(!skipScene.active&&fired.empty());store(globalData.data(),0,700.f);
   // A wait longer than a moment is play, where R is reload: no gap.
   store(ev(cam),0x248,1);mm=manager(15,"multilong",{{"ckamera1",0.f},{"ckamera1",30.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();store(ev(cam),0x248,0);store(globalData.data(),0,700.6f);skip_frame();assert(!skipScene.active);store(globalData.data(),0,700.f);
   // A camera the scene's list does not know joins the scene with its votes.
   store(ev(cam),0x248,1);store(pd(cam),0x108,701.f);mm=manager(15,"multichain",{{"ckamera1",0.f},{"doorshut",41.f}},1,699.f);
   auto chained=created(intern("trigger_camera"));store(ev(chained),0x1d0,intern("chainedcam"));allocate(chained,0x154);store(ev(chained),0x248,1);store(pd(chained),0x124,1);store(pd(chained),0x108,703.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();r(1);assert(skipScene.votes.size()==1);
   CameraLease l4;l4.owner=camera_ref(chained);l4.view=camera_ref(chained);for(int i:{1,2}){l4.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l4;}
   skip_frame();assert(skipScene.active&&skipScene.votes.size()==1&&skipScene.names.count("ckamera1")&&skipScene.names.count("chainedcam"));
   r(2);skip_frame();assert(!skipScene.active&&load<float>(pd(chained),0x108)<700.f&&fired.empty());
   // A player the camera does not hold does not block the others.
   store(ev(chained),0x248,1);store(pd(chained),0x108,703.f);mm=manager(15,"multione",{{"chainedcam",0.f},{"doorshut",41.f}},1,699.f);cameraLeases.erase(&entities[2]);clientViews.erase(&entities[2]);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();r(2);assert(skipScene.votes.empty());r(1);assert(logged({"SKIP vote (R) player=1 on multione: 1/1"}));skip_frame();assert(!skipScene.active);
   store(ev(chained),0x248,0);skipScene={};skipAfter={};CameraLease l5;l5.owner=camera_ref(cam);l5.view=camera_ref(cam);for(int i:{1,2}){l5.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l5;}
   release(chained);store(chained,0,1);
   // v0.7.1, second review: a held camera no scene owns (the wardrobe) is not offered: R could not end it.
   store(pd(mm),0xdc,0);store(ev(cam),0x248,1);store(pd(cam),0x104,666.f);lines.clear();skipScene={};skipAfter={};skip_frame();
   assert(skipScene.active&&!skipScene.allowed&&logged({"SKIP not offered for ckamera1: a held camera"}));store(pd(cam),0x104,5.f);
   // The stats camera stays unskippable when a skippable scene runs straight into it.
   mm=manager(15,"multiintostats",{{"ckamera1",0.f},{"doorshut",41.f}},1,699.f);lines.clear();skipScene={};skipAfter={};skip_frame();assert(skipScene.allowed);
   auto stats=created(intern("trigger_camera"));store(ev(stats),0x1d0,intern("statskamera1"));allocate(stats,0x154);store(ev(stats),0x248,1);store(pd(stats),0x124,1);store(pd(stats),0x104,666.f);
   CameraLease l6;l6.owner=camera_ref(stats);l6.view=camera_ref(stats);for(int i:{1,2}){l6.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l6;}
   skip_frame();assert(skipScene.active&&!skipScene.allowed&&logged({"the stats camera"}));
   release(stats);store(stats,0,1);for(int i:{1,2}){l5.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l5;}
   // A scene on a model camera (cof_mdlcutscene): no lease, every player counts, it ends by its own end path, and all its timers run out.
   store(ev(cam),0x248,0);cameraLeases.clear();
   auto stamp=created(intern("cof_mdlcutscene"));store(ev(stamp),0x1d0,intern("demdlcam"));allocate(stamp,0x3f4);store(ev(stamp),0x19c,&entities[1]);store(ev(stamp),0x104,702.f);store(ev(stamp),0x1a4,55);store(pd(stamp),0x3ec,3);store(pd(stamp),0x3f0,2);store(ev(stamp),0x1cc,intern("nextmdl"));
   auto nextModel=created(intern("cof_mdlcutscene"));store(ev(nextModel),0x1d0,intern("nextmdl"));allocate(nextModel,0x3f4);store(ev(nextModel),0x1a4,23);store(pd(nextModel),0x3ec,2);store(pd(nextModel),0x3f0,1);
   mm=manager(15,"multistamp",{{"demdlcam",0.f},{"doorshut",11.5f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skipAfter={};skip_frame();assert(!skipScene.active);   // no player has a model camera yet
   store(pd(&entities[1]),0x5ac,1);skip_frame();assert(skipScene.allowed&&skipScene.everyone&&logged({"SKIP offered for multistamp (1 manager(s))"}));
   r(1);assert(logged({"SKIP vote (R) player=1 on multistamp: 1/2"}));skip_frame();assert(fired.empty()&&load<int>(pd(stamp),0x3f0)==2);
   r(2);skip_frame();assert(!skipScene.active&&load<int>(pd(stamp),0x3f0)==4&&std::fabs(load<float>(ev(stamp),0x104)-700.f)<0.01f&&std::count(fired.begin(),fired.end(),std::string("doorshut"))==1);
   // The model camera it fires at its end belongs to the skipped scene: ended at once.
   store(ev(stamp),0x104,0.f);store(ev(nextModel),0x19c,&entities[1]);store(ev(nextModel),0x104,700.1f);skip_frame();assert(load<int>(pd(nextModel),0x3f0)==3&&logged({"SKIP ended nextmdl again"}));
   store(ev(nextModel),0x104,0.f);store(pd(&entities[1]),0x5ac,0);skipAfter={};
   // A trigger_camera scene with a model camera still to come also runs to its end (the train scene at 7 s, return at 23 s).
   store(ev(cam),0x248,1);store(pd(cam),0x108,707.f);for(int i:{1,2}){l5.playerSerial=serial(&entities[i]);cameraLeases[&entities[i]]=l5;}
   store(ev(stamp),0x19c,static_cast<void*>(nullptr));mm=manager(15,"multitscene2",{{"ckamera1",0.f},{"demdlcam",7.f},{"doorshut",23.f}},1,699.f);
   lines.clear();fired.clear();skipScene={};skip_frame();r(1);r(2);skip_frame();assert(!skipScene.active&&std::count(fired.begin(),fired.end(),std::string("doorshut"))==1);
   for(auto m:{stamp,nextModel}){release(m);store(m,0,1);}skipAfter={};
   // Scene over (camera off for everyone): the offer is gone.
   store(ev(cam),0x248,0);skip_frame();assert(!skipScene.active);
   for(int i:{13,14,15,16,17}){if(allocations.count(&entities[i]))release(&entities[i]);store(&entities[i],0,1);}cameraLeases.clear();skipScene={};skipJump.clear();}
  store(ev(cam),0x248,0);release(cam);store(cam,0,1);lastPlaces.clear();store(globals,0x0c,mapBefore);}
 // --- v0.5: a player who dies in a deadly trigger_hurt (fall zone) is moved, dead,
 // beside the nearest living player, once; others are untouched.
 {place(&entities[1],0,40,36);place(&entities[2],0,0,-480);store(ev(&entities[1]),0x164,90.f);store(ev(&entities[1]),0x174,0);
  make(13,"trigger_hurt","",713);store(ev(&entities[13]),0x1e4,1000.f);
  const float lo[3]={-300,-300,-600},hi[3]={300,300,-400};for(int k=0;k<3;++k){store(ev(&entities[13]),0xc4+k*4,lo[k]);store(ev(&entities[13]),0xd0+k*4,hi[k]);}
  fallWatch.clear();fall_frame(&entities[2]);assert(origin(&entities[2])[2]==-480.f);   // alive: nothing
  store(ev(&entities[2]),0x174,2);store(ev(&entities[2]),0x164,0.f);lines.clear();fall_frame(&entities[2]);
  assert(logged({"FALL player=2 died in a fall zone; body moved beside player=1"})&&origin(&entities[2])[2]>=36.f&&origin(&entities[1])[2]==36.f);
  place(&entities[2],0,0,-480);lines.clear();fall_frame(&entities[2]);assert(lines.empty()&&origin(&entities[2])[2]==-480.f);   // once per death
  store(ev(&entities[2]),0x174,0);store(ev(&entities[2]),0x164,90.f);place(&entities[2],-3000,0,36);fallWatch.clear();store(&entities[13],0,1);}
 // --- v0.5: the prologue's ready and final counts wait for the players actually in the game.
 {const int mapBefore=load<int>(globals,0x0c);store(globals,0x0c,intern("cof_campaign_story4"));
  make(14,"game_counter","sc3_ready_count",714);store(ev(&entities[14]),0x164,2.f);make(15,"game_counter","sc4_final_count",715);store(ev(&entities[15]),0x164,2.f);
  store(&entities[2],0,1);lines.clear();partyLimitLogged=-1;party_counters_frame();
  assert(load<float>(ev(&entities[14]),0x164)==1.f&&load<float>(ev(&entities[15]),0x164)==1.f&&logged({"PROLOGUE ready and final counts wait for 1 player(s)"}));
  store(&entities[2],0,0);party_counters_frame();assert(load<float>(ev(&entities[14]),0x164)==2.f);
  store(globals,0x0c,mapBefore);store(&entities[14],0,1);store(&entities[15],0,1);}
 // --- v0.5: the prologue can be skipped (SPACE or E from every player) through its own end manager.
 {const int mapBefore=load<int>(globals,0x0c);store(globals,0x0c,intern("cof_campaign_story4"));make(16,"multi_manager","kunitei2",716);
  std::array<unsigned char,0x34> cmd{};auto press=[&](int i,unsigned short b){std::memcpy(cmd.data()+0x1e,&b,2);prologue_skip_command(&entities[i],cmd.data());};
  prologueSkip={};skipJump.clear();openingTimeline=-1;lines.clear();prologue_skip_frame();press(1,2);assert(!prologueSkip.offered&&prologueSkip.votes.empty());   // intro not running yet
  openingTimeline=5;prologue_skip_frame();assert(logged({"SKIP offered for the prologue"}));
  fired.clear();press(1,0x2000);press(1,0x2000);prologue_skip_frame();assert(fired.empty()&&logged({"SKIP vote player=1 on the prologue: 1/2"}));
  press(2,0x2000);prologue_skip_frame();assert((fired==std::vector<std::string>{"use kunitei2"})&&prologueSkip.done&&logged({"SKIP prologue: kunitei2 started"}));
  press(2,0);press(2,2);prologue_skip_frame();assert(fired.size()==1);   // once
  prologueSkip={};skipJump.clear();openingTimeline=-1;store(&entities[16],0,1);store(globals,0x0c,mapBefore);}
 // --- v0.5: a second syringe inside the first one's window starts the morphine trip for that
 // player only; limping follows health below 25 through the "low" physics key.
 {static std::map<std::string,std::string> phys;
  engine[90]=reinterpret_cast<void*>(+[](std::int32_t lo,std::int32_t)->std::int32_t{return lo;});
  engine[119]=reinterpret_cast<void*>(+[](const Edict*,const char* k)->const char*{auto it=phys.find(k);return it==phys.end()?"":it->second.c_str();});
  engine[120]=reinterpret_cast<void*>(+[](const Edict*,const char* k,const char* val){phys[k]=val;});
  concussMessage=77;screenFadeMessage=78;morphineWatch.clear();store(globalData.data(),0,100.f);void* mp1=pd(&entities[1]);void* mp2=pd(&entities[2]);
  store(mp1,0x2274,0.f);store(mp2,0x2274,0.f);morphine_frame(&entities[1]);morphine_frame(&entities[2]);
  store(mp1,0x2274,130.f);morphine_frame(&entities[1]);assert(!load<unsigned char>(mp1,0x2278));   // first shot: no trip
  store(globalData.data(),0,105.f);store(mp1,0x2274,135.f);wire.clear();lines.clear();morphine_frame(&entities[1]);morphine_frame(&entities[2]);
  assert(load<unsigned char>(mp1,0x2278)==1&&load<float>(mp1,0x227c)==135.f&&load<int>(mp1,0x1ebc)==1&&!load<unsigned char>(mp2,0x2278)&&logged({"MORPHINE player=1"}));
  assert(std::find(wire.begin(),wire.end(),"begin 1 77 1")!=wire.end()&&std::find(wire.begin(),wire.end(),"begin 1 78 1")!=wire.end());
  store(mp1,0x2278,(unsigned char)0);store(mp1,0x1ebc,0);
  store(ev(&entities[1]),0x164,20.f);limp_frame(&entities[1]);assert(phys["low"]=="1");store(ev(&entities[1]),0x164,90.f);limp_frame(&entities[1]);assert(phys["low"]=="0");
  engine[90]=nullptr;engine[119]=nullptr;engine[120]=nullptr;concussMessage=0;screenFadeMessage=0;morphineWatch.clear();}
 // --- v0.7.1: the elevator scene and the door unlock wait at the single-player arrival (s4_apartmentsick_e0 *10, s4_city *47).
 {const int mapBefore=load<int>(globals,0x0c);store(globals,0x0c,intern("s4_apartment3_e0"));nextFree=25;lines.clear();touches.clear();pendingArrival={};loadWait={};
  original[4]=reinterpret_cast<void*>(&touch_);
  for(int i:{1,2}){place(&entities[i],-1204,2449,-18);allocations[&entities[i]].commands=1;allocations[&entities[i]].hudDue=0;carryDone[&entities[i]]=serial(&entities[i]);}
  arrival_level_change("s4_apartmentsick_e0");assert(pendingArrival.a&&!std::strcmp(pendingArrival.a->from,"apartment3"));
  store(globals,0x0c,intern("s4_apartmentsick_e0"));
  auto once=created(intern("trigger_once"));store(ev(once),0xc4,-1251.f);store(ev(once),0xc8,2403.f);store(ev(once),0xcc,-38.f);store(ev(once),0xd0,-1155.f);store(ev(once),0xd4,2496.f);store(ev(once),0xd8,134.f);store(ev(once),0x1cc,intern("multi_hiss_1"));
  auto away=created(intern("trigger_once"));store(ev(away),0xc4,5000.f);store(ev(away),0xd0,5100.f);store(ev(away),0x1cc,intern("not_here"));
  loadWait.armed=true;arrival_frame(&entities[1]);assert(touches.empty()&&pendingArrival.placed.empty());   // the partner is still loading
  loadWait={};arrival_frame(&entities[1]);assert(touches.size()==1&&touches[0].first==once&&touches[0].second==&entities[1]&&logged({"ARRIVAL fires","multi_hiss_1"})&&logged({"ARRIVAL player=1 from apartment3"}));
  arrival_frame(&entities[2]);assert(touches.size()==1&&logged({"ARRIVAL player=2 from apartment3"}));
  pendingArrival={};store(once,0,1);store(away,0,1);store(globals,0x0c,mapBefore);original[4]=nullptr;}
 // --- v0.5: the 10th byte of "Credits" (IsMultiplayer) reads 0 on story maps.
 {creditsMessage=99;wire.clear();message_begin(1,99,nullptr,&entities[1]);for(int i=1;i<=12;++i)write_byte(1);message_end();
  assert(wire[10]=="byte 0"&&wire[9]=="byte 1"&&wire[11]=="byte 1");
  wire.clear();message_begin(1,98,nullptr,&entities[1]);for(int i=1;i<=12;++i)write_byte(1);message_end();assert(wire[10]=="byte 1");creditsMessage=0;}
 std::puts("PASS world/campaign checkpoint fixtures: saves at any RC2/RC4 recorder (BSP-indexed), per-player poses, removed pickups and key-unlocked doors restored after load with authored-target/hidden/story-toggled exclusions, safe TraceHull landings (occupied, wall, water), tape reminder set on restore, boat partner gathered at the dock only, stuck command and nightmare photo regroup. NOT Cry of Fear gameplay verification.");
}
