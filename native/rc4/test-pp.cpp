// Owned SDK fixture for pp-runtime.hpp; no original game code is loaded.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
namespace rc4 {struct Edict {std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,24> entities{};
std::unordered_map<Edict*,std::vector<unsigned char>> mem;
std::array<unsigned char,0xa0> globalData{};
std::vector<std::string> strings{"","s4_start_e0","c_start","player"};
std::vector<std::pair<Edict*,Edict*>> uses;int nextFree=12;std::vector<std::string> lines;
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<24;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<24?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void free_(Edict* e){mem.erase(e);}
void print(const char* s){lines.emplace_back(s);}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:0x1d0;
 for(int i=start?index(start)+1:1;i<24;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0];
}
Edict* create(int name){auto e=&entities[nextFree++];*e={};store(e,0,0);store(e,4,700+nextFree);store(ev(e),0,name);return e;}
void remove_(Edict* e){release(e);store(e,0,1);}
void kv(Edict* e,KeyValue* data){
 if(!std::strcmp(data->key,"targetname"))store(ev(e),0x1d0,intern(data->value));
 else if(!std::strcmp(data->key,"health"))store(ev(e),0x164,static_cast<float>(std::atof(data->value)));
 else if(!std::strcmp(data->key,"frags"))store(ev(e),0x168,static_cast<float>(std::atof(data->value)));
 data->handled=1;
}
int spawn_(Edict*){return 0;}
void use(Edict* used,Edict* other){uses.emplace_back(used,other);}
void make(int i,const char* type,const char* name){auto e=&entities[i];*e={};store(e,0,0);store(e,4,100+i);store(ev(e),0,intern(type));store(ev(e),0x1d0,intern(name));}
void fixture_reset(){pp_reset();uses.clear();lines.clear();nextFree=12;for(auto& e:entities){e={};store(&e,0,1);}make(1,"player","");make(2,"player","");}
void no_sound(Edict*,int,const char*,float,float,int,int){}void no_ambient(Edict*,float*,const char*,float,float,int,int){}int thinks=0;void count_think(Edict*){++thinks;}
Edict* counter_named(const char* name){for(int i=12;i<24;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),0x1d0)),name)==0)return &entities[i];return nullptr;}
}
int main(){
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,24);
 engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&create);engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[65]=reinterpret_cast<void*>(&free_);
 engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[81]=reinterpret_cast<void*>(&print);
 original[1]=reinterpret_cast<void*>(&spawn_);original[3]=reinterpret_cast<void*>(&use);original[6]=reinterpret_cast<void*>(&kv);
 fixture_reset();make(3,"cof_blur","blurrr");make(4,"cof_camerazoom","zoomin1");make(5,"cof_blur","");make(6,"cof_blackandwhite","white");
 pp_activate();
 // Authored names now belong to counters; the effect entities keep a private name.
 assert(postProcesses.size()==2&&std::string(targetname(&entities[3]))=="blurrr__sc4pp"&&std::string(targetname(&entities[4]))=="zoomin1__sc4pp");
 assert(std::string(targetname(&entities[5])).empty()&&std::string(targetname(&entities[6]))=="white");
 auto blur=counter_named("blurrr"),zoom=counter_named("zoomin1");assert(blur&&zoom&&std::strcmp(classname(blur),"game_counter")==0&&load<float>(ev(blur),0x164)>1e8f);
 // Nothing fires without an authored Use.
 pp_frame();assert(uses.empty());
 // One authored fire -> the real effect used once for EVERY connected player.
 store(ev(blur),0x168,1.f);pp_frame();assert(uses.size()==2&&uses[0]==std::make_pair(&entities[3],&entities[1])&&uses[1]==std::make_pair(&entities[3],&entities[2]));
 pp_frame();assert(uses.size()==2);
 // Two more fires in one frame (and an OFF fire counting down) each count.
 store(ev(zoom),0x168,2.f);pp_frame();assert(uses.size()==6);store(ev(zoom),0x168,1.f);pp_frame();assert(uses.size()==8);
 // A disconnected player is skipped.
 store(&entities[2],0,1);store(ev(blur),0x168,2.f);pp_frame();assert(uses.size()==9&&uses.back().second==&entities[1]);
 // v0.3 audit: a revisited map restores the private name; the counter takes the authored one again.
 fixture_reset();make(3,"cof_blur","blurrr__sc4pp");pp_activate();
 assert(postProcesses.size()==1&&counter_named("blurrr")&&std::string(targetname(&entities[3]))=="blurrr__sc4pp"&&postProcesses[0].name=="blurrr");
 // v0.5 F3: a switched rain_modify writes the rain fields of every player (no native Use).
 fixture_reset();make(7,"rain_modify","rain");for(int i=1;i<=2;++i)allocate(&entities[i],PlayerBytes);
 {auto r=static_cast<unsigned char*>(alloc(&entities[7],0x100));store(r,4,ev(&entities[7]));store(r,0xdc,800);store(r,0xe0,2.f);store(r,0xe8,1.5f);
  pp_activate();auto c=counter_named("rain");assert(c&&postProcesses.size()==1);store(ev(c),0x168,1.f);pp_frame();assert(uses.empty());
  for(int i=1;i<=2;++i){auto p=pd(&entities[i]);assert(load<int>(p,0x27dc)==800&&load<float>(p,0x27e0)==2.f&&load<float>(p,0x27e8)==1.5f&&load<int>(p,0x280c)==1&&load<float>(p,0x2804)==0.f);}
  // With a fade time the ideal fields and the fade end are written instead.
  store(globals,0,10.f);store(r,0xf0,4.f);store(r,0xdc,0);store(ev(c),0x168,2.f);pp_frame();
  for(int i=1;i<=2;++i){auto p=pd(&entities[i]);assert(load<int>(p,0x27f0)==0&&load<float>(p,0x2804)==14.f&&load<float>(p,0x2808)==11.f&&load<int>(p,0x27dc)==800);}
  bool logged=false;for(const auto& l:lines)logged=logged||l.find("RAIN 800 drops/s for 2 player(s)")!=std::string::npos;assert(logged);store(globals,0,0.f);}
 // v0.8: the effect has a private name, so a skip reaches it through the fan-out: a zoom of the skipped scene
 // never starts, a running one returns once and its later return is cancelled; another scene's zoom is left alone.
 fixture_reset();make(4,"cof_camerazoom","zoomin1");make(5,"cof_camerazoom","other");pp_activate();
 {engine[29]=reinterpret_cast<void*>(&no_sound);engine[30]=reinterpret_cast<void*>(&no_ambient);original[2]=reinterpret_cast<void*>(&count_think);
  auto z=counter_named("zoomin1"),o=counter_named("other");store(ev(&entities[4]),0x104,99.f);skipScene={};skipScene.names.insert("zoomin1");
  store(ev(z),0x168,1.f);store(ev(o),0x168,1.f);thinks=0;skip_stop_sounds();pp_frame();
  assert(thinks==1&&load<float>(ev(&entities[4]),0x104)==0.f&&uses.size()==2&&uses[0].first==&entities[5]&&uses[1].first==&entities[5]);
  skipScene={};uses.clear();store(ev(z),0x168,2.f);pp_frame();assert(uses.size()==2&&uses[0].first==&entities[4]);   // fired again later, in play: it runs
  engine[29]=nullptr;engine[30]=nullptr;original[2]=nullptr;}
 // Unscoped maps are left completely untouched.
 fixture_reset();store(globals,0x0c,2);make(3,"cof_blur","blurrr");pp_activate();assert(postProcesses.empty()&&std::string(targetname(&entities[3]))=="blurrr");
 std::puts("PASS post-process fan-out fixtures: authored name handed to a native counter, effect used once per connected player per fire, repeated/off fires counted, unnamed/unrelated effects and unscoped maps untouched, disconnected players skipped");
}
