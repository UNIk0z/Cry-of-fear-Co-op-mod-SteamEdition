// Owned SDK fixture for ladder-runtime.hpp; no original game code is loaded.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
namespace rc4 {struct Edict {std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,32> entities{};
std::unordered_map<Edict*,std::vector<unsigned char>> mem;
std::array<unsigned char,0xa0> globalData{};
std::vector<std::string> strings{"","s4_start_e0","c_start","player"};
std::unordered_map<Edict*,std::vector<std::pair<std::string,std::string>>> keys;int nextFree=20;std::vector<std::string> lines;
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<32;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<32?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void free_(Edict* e){mem.erase(e);}
void print(const char* s){lines.emplace_back(s);}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:std::strcmp(key,"target")==0?0x1cc:0x1d0;
 for(int i=start?index(start)+1:1;i<32;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0]; // real engine: world edict, not null, when nothing matches
}
Edict* create(int name){auto e=&entities[nextFree++];*e={};store(e,0,0);store(e,4,500+nextFree);store(ev(e),0,name);return e;}
void remove_(Edict* e){release(e);store(e,0,1);}
// The game's KeyValue: entvars fields only (origin x, model, targetname) plus a record of every key.
void kv(Edict* e,KeyValue* data){
 if(!std::strcmp(data->key,"targetname"))store(ev(e),0x1d0,intern(data->value));
 if(!std::strcmp(data->key,"model"))store(ev(e),0xb8,intern(data->value));
 if(!std::strcmp(data->key,"origin"))store(ev(e),8,static_cast<float>(std::atof(data->value)));
 keys[e].emplace_back(data->key,data->value);data->handled=1;
}
int spawn_(Edict* e){allocate(e,std::strcmp(classname(e),"cof_ladder_manager")?0x200:0x1b4);return 0;}
Edict* make(int i,const char* type,const char* name,const char* target,int bytes,float x){auto e=&entities[i];*e={};store(e,0,0);store(e,4,100+i);store(ev(e),0,intern(type));store(ev(e),0x1d0,intern(name));store(ev(e),0x1cc,intern(target));store(ev(e),8,x);allocate(e,bytes);return e;}
// Map load: the engine passes every BSP key of the manager through the DLL's KeyValue.
Edict* manager(int i,const char* name,float x,const char* model){
 auto e=make(i,"cof_ladder_manager",name,"",0x1b4,x);store(ev(e),0xb8,intern(model));
 char ox[32]{};std::snprintf(ox,sizeof(ox),"%.0f 0 0",x);
 for(const auto& [k,v]:std::vector<std::pair<const char*,const char*>>{{"classname","cof_ladder_manager"},{"targetname",name},{"origin",ox},{"model",model},{"animation1","climb_on"},{"animation2","climb_loop"},{"topent","ladtop"}}){KeyValue d{const_cast<char*>("cof_ladder_manager"),const_cast<char*>(k),const_cast<char*>(v),0};ladder_keyvalue(e,&d);}
 return e;
}
void player(int i){auto e=&entities[i];*e={};store(e,0,0);store(e,4,200+i);store(ev(e),0,3);allocate(e,PlayerBytes);}
Edict* camera(int i,float x,const char* model){auto c=make(i,"camera_model","","",0x200,x);store(ev(c),0xb8,intern(model));return c;}
void climb(Edict* m,int who,Edict* cam){store(pd(m),0x114,reinterpret_cast<std::uint32_t>(pd(&entities[who])));store(pd(&entities[who]),0x5ac,cam?reinterpret_cast<std::uint32_t>(pd(cam)):0u);}
std::uint32_t slot(Edict* m){return load<std::uint32_t>(pd(m),0x114);}
bool logged(const char* part){for(const auto& l:lines)if(l.find(part)!=std::string::npos)return true;return false;}
Edict* lookup(const char* name){Edict* out=nullptr;return ladder_find_override(nullptr,"targetname",name,out)?out:&entities[0];}
}
int main(){
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,32);
 engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&create);engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[65]=reinterpret_cast<void*>(&free_);
 engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[81]=reinterpret_cast<void*>(&print);
 original[1]=reinterpret_cast<void*>(&spawn_);original[6]=reinterpret_cast<void*>(&kv);
 for(auto& e:entities)store(&e,0,1);
 player(1);player(2);
 // Playtest 3 layout: three ladders side by side with the same climb model (nerdclimb14/15/16).
 auto a=manager(3,"nerdclimb14",100.f,"models/ladder_climb.mdl");make(4,"cof_ladder_manager_use","","nerdclimb14",0x184,100.f);
 auto b=manager(5,"nerdclimb15",300.f,"models/ladder_climb.mdl");make(6,"cof_ladder_manager_use","","nerdclimb15",0x184,300.f);
 auto c=manager(7,"nerdclimb16",2000.f,"models/ladder_climb.mdl");make(8,"cof_ladder_manager_use","","nerdclimb16",0x184,2000.f);
 auto camA=camera(9,100.f,"models/ladder_climb.mdl");auto camB=camera(10,300.f,"models/ladder_climb.mdl");
 ladder_activate();assert(ladders.size()==3&&logged("LADDER 3 ladder(s)"));
 // A free ladder: the use brush finds its own manager.
 assert(lookup("nerdclimb14")==&entities[0]);
 // P1 climbs ladder A. Its neighbour B, with a stale slot of P1 from an earlier climb, stays free.
 climb(b,1,nullptr);climb(a,1,camA);ladder_frame();
 assert(logged("LADDER nerdclimb14 climbed by player=1")&&!logged("nerdclimb15 climbed")&&(load<unsigned>(ev(camA),0x118)&0x20u));
 assert(lookup("nerdclimb15")==&entities[0]);
 // P2 presses E on A while P1 climbs it: a copy of A with A's keys and its own name.
 auto copy=lookup("nerdclimb14");assert(copy&&copy!=&entities[0]&&copy!=a&&ladders[0].copies.size()==1&&logged("LADDER nerdclimb14 copy 1 made"));
 assert(!std::strcmp(targetname(copy),"nerdclimb14_sc4copy1")&&load<float>(ev(copy),8)==100.f&&!std::strcmp(str(load<int>(ev(copy),0xb8)),"models/ladder_climb.mdl"));
 {bool anim=false,top=false;for(const auto& [k,v]:keys[copy]){anim|=k=="animation2"&&v=="climb_loop";top|=k=="topent"&&v=="ladtop";assert(k!="classname");}assert(anim&&top);}
 // P2 climbs the copy: both climb at once; with two player slots there is no third climber.
 auto camC=camera(11,100.f,"models/ladder_climb.mdl");store(ev(camC),16,96.f);climb(copy,2,camC);ladder_frame();
 assert(logged("LADDER nerdclimb14_sc4copy1 climbed by player=2"));
 {Edict* out=&entities[0];assert(ladder_find_override(nullptr,"targetname","nerdclimb14",out)&&out==nullptr);}
 // P1's climb ends: A is free again (its slot cleared), the copy stays busy.
 store(pd(&entities[1]),0x5ac,0u);ladder_frame();assert(slot(a)==0&&logged("LADDER nerdclimb14 free again (player=1)"));
 assert(lookup("nerdclimb14")==&entities[0]);
 // A camera_model with another model (a cutscene) does not hold the copy; P2 leaving clears its slot.
 store(ev(camC),0xb8,intern("models/mdl_camera.mdl"));ladder_frame();assert(logged("LADDER nerdclimb14_sc4copy1 free again (player=2)"));
 store(ev(camC),0xb8,intern("models/ladder_climb.mdl"));ladder_frame();store(&entities[2],0,1);ladder_frame();assert(slot(copy)==0);
 // Once a copy exists it is reused, never a second one for the same slot.
 player(2);climb(a,2,camB);store(ev(camB),8,100.f);ladder_frame();assert(lookup("nerdclimb14")==copy&&ladders[0].copies.size()==1);
 // Far ladder C and other lookups are untouched; a key other than targetname is never redirected.
 {Edict* out=nullptr;assert(!ladder_find_override(nullptr,"target","nerdclimb14",out)&&!ladder_find_override(a,"targetname","nerdclimb14",out));}
 (void)c;
 // Map change clears everything; unscoped maps never set up ladders.
 ladder_map_reset();assert(ladders.empty()&&ladderKeys.empty());store(globals,0x0c,2);player(1);ladder_activate();assert(ladders.empty());
 std::puts("PASS ladder fixtures: every player climbs at once (a copy of a busy manager with its recorded keys), neighbours with stale slots stay free, ended climbs and departed climbers release their slot, no climber beyond the player slots, map reset and unscoped maps untouched");
}
