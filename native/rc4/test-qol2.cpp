// Owned SDK fixture for qol2-runtime.hpp (v0.10): host options, hardcore (no saves, no respawn),
// monster scaling, dodge kept, ping, scare, run timer, black and white, teammate outline, hunted mode.
// No game code is loaded and no game is launched.
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
std::vector<std::string> strings{"","s4_start","player","monster_zombie","monster_bosschainsaw"};
std::unordered_map<Edict*,std::map<std::string,std::string>> helpers;
std::vector<std::string> lines,serverCommands,argv,precached,models,sounds;std::string gameDir;int nextFree=40;
std::map<std::string,float> cvars;
struct Sent {int dest=0,type=0;Edict* to=nullptr;std::vector<int> bytes;std::vector<float> coords;std::vector<std::string> texts;};
std::vector<Sent> sent;Sent building;
bool logged(const char* part){for(const auto& l:lines)if(l.find(part)!=std::string::npos)return true;return false;}
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<48;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<48?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void free_(Edict* e){mem.erase(e);}
void print(const char* s){lines.emplace_back(s);}
void client_print(Edict* e,int,const char* s){lines.emplace_back("CLIENT"+std::to_string(index(e))+":"+s);}
void server_command(const char* s){serverCommands.emplace_back(s);}
void __cdecl stuff_(Edict*,const char*,...){}
const char* arg(int i){return i>=0&&i<static_cast<int>(argv.size())?argv[i].c_str():"";}
void dir_(char* out){std::strcpy(out,gameDir.c_str());}
int listen(){return 0;}
float cvar_get(const char* n){auto it=cvars.find(n);return it==cvars.end()?0.f:it->second;}
void cvar_register(cvar_t* c){cvars.emplace(c->name,c->value);}
int precache_model_(const char* s){models.emplace_back(s);return 70+static_cast<int>(models.size());}
int precache_(const char* s){precached.emplace_back(s);return static_cast<int>(precached.size());}
void sound_(Edict* e,int channel,const char* s,float,float,int,int){sounds.push_back(std::to_string(index(e))+":"+std::to_string(channel)+":"+s);}
std::int32_t random_(std::int32_t low,std::int32_t){return low;}
char* info_buffer(Edict* e){return reinterpret_cast<char*>(e);}
char* info_value(char* buffer,char* key){static std::string out;out=std::strcmp(key,"name")?"":index(reinterpret_cast<Edict*>(buffer))==1?"Katz":"Bob";return const_cast<char*>(out.c_str());}
void begin_(int dest,int type,const float*,Edict* e){building={};building.dest=dest;building.type=type;building.to=e;}
void byte_(int v){building.bytes.push_back(v);}
void coord_(float v){building.coords.push_back(v);}
void string_(const char* s){building.texts.emplace_back(s?s:"");}
void end_(){sent.push_back(building);building={};}
// World: floor z=0 (a standing origin is z>=36), a wall at x=600..640 (x 600-640 blocks sight and walking).
bool solid(const float* p){return p[2]<36||(p[0]>600&&p[0]<640);}
void hull(const float* a,const float* b,int,int,Edict*,Trace* t){
 *t={};if(solid(a)){t->startSolid=1;t->allSolid=solid(b);return;}t->fraction=1;
 if(b[2]<a[2]){if(b[2]<36)t->fraction=(a[2]-36)/(a[2]-b[2]);return;}
 for(float s=0.f;s<=1.f;s+=0.02f){const float p[3]={a[0]+(b[0]-a[0])*s,a[1]+(b[1]-a[1])*s,a[2]};if(solid(p)){t->fraction=s;return;}}
}
// Line traces: the wall at x 600..640 and a back wall at x=2000.
void line_(const float* a,const float* b,int,Edict*,Trace* t){
 *t={};t->fraction=1;for(int k=0;k<3;++k)t->end[k]=b[k];
 for(float s=0.f;s<=1.f;s+=0.001f){const float p[3]={a[0]+(b[0]-a[0])*s,a[1]+(b[1]-a[1])*s,a[2]+(b[2]-a[2])*s};if((p[0]>600&&p[0]<640)||p[0]>=2000||p[2]<0){t->fraction=s;for(int k=0;k<3;++k)t->end[k]=p[k];return;}}
}
int contents(const float*){return -1;}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:0x1d0;
 for(int i=start?index(start)+1:1;i<48;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0];
}
Edict* created(int type){auto e=&entities[nextFree++];assert(nextFree<48);*e={};store(e,0,0);store(e,4,500+nextFree);store(ev(e),0,type);return e;}
void remove_(Edict* e){release(e);store(e,0,1);helpers.erase(e);}
int spawns=0;
int spawn_(Edict* e){++spawns;if(!std::strcmp(classname(e),"monster_booksimonsledgehammer")){store(ev(e),0x164,static_cast<float>(std::atof(helpers[e]["health"].c_str())));store(ev(e),0x1a8,0x20u);}return 0;}
void key_(Edict* e,KeyValue* kv){
 helpers[e][kv->key]=kv->value;kv->handled=1;
 if(!std::strcmp(kv->key,"targetname"))store(ev(e),0x1d0,intern(kv->value));
 if(!std::strcmp(kv->key,"origin")){float x=0,y=0,z=0;assert(std::sscanf(kv->value,"%f %f %f",&x,&y,&z)==3);store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,z);}
}
std::vector<std::string> used;
void use_(Edict* h,Edict*){
 used.push_back(classname(h));
 if(!std::strcmp(classname(h),"trigger_relay")){auto victim=find_next(nullptr,"targetname",helpers[h]["killtarget"].c_str());if(victim)store(victim,0,1);}
}
void player(int i,float x,float y){
 auto e=&entities[i];*e={};store(e,0,0);store(e,4,100+i);store(ev(e),0,intern("player"));store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,36.f);store(ev(e),0x164,100.f);store(ev(e),0x1a8,0x200u);
 allocate(e,PlayerBytes);auto& a=allocations.at(e);a.commands=true;a.hudDue=0;
}
Edict* monster(int i,const char* cls,float health){auto e=&entities[i];*e={};store(e,0,0);store(e,4,300+i);store(ev(e),0,intern(cls));store(ev(e),0x164,health);store(ev(e),0x1b4,health);store(ev(e),0x170,1.f);store(ev(e),0x1a8,0x20u);return e;}
void now(float t){store(globals,0,t);}
void changelevel_(char*,char*){}
std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return std::string((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());}
const Sent* last_type(int type){for(auto it=sent.rbegin();it!=sent.rend();++it)if(it->type==type)return &*it;return nullptr;}
int count_type(int type){int n=0;for(const auto& s:sent)if(s.type==type)++n;return n;}
}
int main(int argc,char** argv_){
 assert(argc==2);gameDir=argv_[1];
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,4);store(globals,0x94,48);now(5.f);
 engine[0]=reinterpret_cast<void*>(&precache_model_);engine[1]=reinterpret_cast<void*>(&precache_);engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&created);
 engine[29]=reinterpret_cast<void*>(&sound_);engine[31]=reinterpret_cast<void*>(&line_);engine[34]=reinterpret_cast<void*>(&hull);engine[39]=reinterpret_cast<void*>(&server_command);engine[41]=reinterpret_cast<void*>(&stuff_);engine[45]=reinterpret_cast<void*>(&contents);
 engine[46]=reinterpret_cast<void*>(&begin_);engine[47]=reinterpret_cast<void*>(&end_);engine[48]=reinterpret_cast<void*>(&byte_);engine[50]=reinterpret_cast<void*>(&byte_);engine[53]=reinterpret_cast<void*>(&coord_);engine[54]=reinterpret_cast<void*>(&string_);
 engine[56]=reinterpret_cast<void*>(&cvar_register);engine[57]=reinterpret_cast<void*>(&cvar_get);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[65]=reinterpret_cast<void*>(&free_);engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);
 engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[80]=reinterpret_cast<void*>(&client_print);engine[81]=reinterpret_cast<void*>(&print);engine[83]=reinterpret_cast<void*>(&arg);
 engine[90]=reinterpret_cast<void*>(&random_);engine[99]=reinterpret_cast<void*>(&dir_);engine[106]=reinterpret_cast<void*>(&info_buffer);engine[107]=reinterpret_cast<void*>(&info_value);engine[115]=reinterpret_cast<void*>(&listen);
 original[1]=reinterpret_cast<void*>(&spawn_);original[3]=reinterpret_cast<void*>(&use_);original[6]=reinterpret_cast<void*>(&key_);
 for(auto& e:entities)store(&e,0,1);store(&entities[0],0,0);store(ev(&entities[0]),0,intern("worldspawn"));
 proFontMessage=77;blackWhiteMessage=81;chapterMessage=82;creditsMessage=83;
 const std::filesystem::path dir=gameDir;auto p1=&entities[1],p2=&entities[2];
 player(1,0,0);player(2,200,0);

 // --- Options: registered in every game, read twice a second, all off by default.
 qol2_register();for(const char* c:{"sc_hunted","sc_scale","sc_outline","sc_timer","sc_norespawn","sc_noir","sc_color"})assert(cvars.count(c)&&cvars[c]==0.f);
 host_options_frame();assert(!hostOptions.hunted&&!hostOptions.scale&&!hostOptions.outline&&!hostOptions.timer&&!hostOptions.norespawn&&!hints_off());
 cvars["sv_disablehints"]=1;now(5.6f);host_options_frame();assert(hints_off());cvars["sv_disablehints"]=0;now(6.2f);host_options_frame();

 // --- Each player's colour and black-and-white choice from sc_mdl (older games send neither).
 argv={"sc_mdl","simon","auto","1","3","1"};assert(model_report_command(p2,"sc_mdl")&&playerPrefs[2].colour==3&&playerPrefs[2].noir);
 argv={"sc_mdl","simon","auto"};assert(model_report_command(p2,"sc_mdl")&&playerPrefs[2].colour==3);
 assert((player_colour(2)==std::array<unsigned char,3>{235,220,50})&&(player_colour(1)==std::array<unsigned char,3>{230,40,40})&&(player_colour(4)==std::array<unsigned char,3>{235,220,50}));

 // --- #19 No saves: the co-op save is refused. No respawn: the native waits are pushed ahead.
 cvars["sv_coop_nosave"]=1;lines.clear();checkpoint_save(p1);assert(logged("saving is off on this server")&&logged("CLIENT1:Could not save:"));
 // v0.10.1: the host's recorder does not open the save window ("Saving... Saved") when saving is off.
 saveLoadMessage=90;lines.clear();sent.clear();assert(recorder_begin(1,90,nullptr,p1));recorder_write(48,0,0,nullptr);recorder_write(54,0,0,"Storage Tunnel");assert(recorder_end());
 assert(logged("CLIENT1:Could not save:")&&count_type(90)==0&&!saveWindow.actor);saveLoadMessage=0;cvars["sv_coop_nosave"]=0;
 cvars["sc_norespawn"]=1;now(6.8f);host_options_frame();store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);
 store(pd(p2),PlayerRespawnDelay,10.f);store(pd(p2),PlayerDeadNotify,10.f);store(pd(p2),PlayerAutoRevive,30.f);norespawn_frame(p2);
 assert(load<float>(pd(p2),PlayerRespawnDelay)>60.f&&load<float>(pd(p2),PlayerDeadNotify)>60.f&&load<float>(pd(p2),PlayerAutoRevive)>600.f);
 fallBacks[p2].serial=serial(p2);store(pd(p2),PlayerAutoRevive,8.f);norespawn_frame(p2);assert(load<float>(pd(p2),PlayerAutoRevive)==8.f);fallBacks.clear();   // a fall respawn keeps its 2 s
 store(p1,0,1);store(pd(p2),PlayerAutoRevive,8.f);norespawn_frame(p2);assert(load<float>(pd(p2),PlayerAutoRevive)==8.f);store(p1,0,0);   // nobody left to revive him: the game's own respawn
 store(ev(p2),0x174,0);store(ev(p2),0x164,100.f);cvars["sc_norespawn"]=0;now(7.4f);host_options_frame();

 // --- #9 Monster health by party size: off, one player, two players, bosses, once each.
 auto z=monster(10,"monster_zombie",100.f);auto boss=monster(11,"monster_bosschainsaw",1000.f);
 scale_frame();assert(load<float>(ev(z),0x164)==100.f);
 cvars["sc_scale"]=1;now(8.f);host_options_frame();store(p2,0,1);scale_frame();assert(load<float>(ev(z),0x164)==100.f);   // alone: x1.0
 store(p2,0,0);now(8.6f);scale_frame();assert(std::fabs(load<float>(ev(z),0x164)-135.f)<0.01f&&std::fabs(load<float>(ev(z),0x1b4)-135.f)<0.01f&&load<float>(ev(boss),0x164)==1000.f&&logged("SCALE monster health x1.35 for 2 players"));
 now(9.2f);scale_frame();assert(std::fabs(load<float>(ev(z),0x164)-135.f)<0.01f);                                    // once
 qol2_map_changed();host_options_frame();scale_restored(z);now(9.5f);scale_frame();assert(std::fabs(load<float>(ev(z),0x164)-135.f)<0.01f);   // a revisit or a load restores it scaled: not again
 assert(std::fabs(scale_factor(4)-2.05f)<0.001f&&scale_factor(9)==scale_factor(4));
 cvars["sc_scale"]=0;now(9.8f);host_options_frame();store(z,0,1);store(boss,0,1);

 // --- #21 Dodge kept per player across maps.
 store<unsigned char>(pd(p1),PlayerDodgeDisabled,1);qol2_after_command(p1,"dodgetoggle");assert(dodgeChoice["slot1"]==1);
 qol2_map_changed();player(1,0,0);assert(load<unsigned char>(pd(p1),PlayerDodgeDisabled)==0);dodge_frame(p1);assert(load<unsigned char>(pd(p1),PlayerDodgeDisabled)==1&&logged("DODGE player=1 kept off"));

 // --- #8 Ping: glow at the wall the player looks at, text to everyone with the distance, 2 s cooldown.
 ping_precache();assert(pingSprite==71&&models.back()=="sprites/glow01.spr");sent.clear();now(20.f);
 store(ev(p1),0x178,0.f);store(ev(p1),0x17c,0.f);store(ev(p1),0x180,28.f);store(ev(p1),0x74,0.f);store(ev(p1),0x78,0.f);   // eye 28 up, looking along +x
 assert(qol2_client_command(p1,"sc_ping"));
 {const Sent* g=nullptr;for(const auto& m:sent)if(m.type==23&&m.dest==2)g=&m;assert(g&&g->dest==2&&(g->bytes==std::vector<int>{23,71,30,30,220})&&g->coords.size()==3&&g->coords[0]>590.f&&g->coords[0]<601.f&&std::fabs(g->coords[2]-64.f)<0.5f);}   // 3 s
 {int texts=0;for(const auto& s:sent)if(s.type==23&&!s.texts.empty()){++texts;assert(s.texts[0].rfind("Katz: here (",0)==0);}assert(texts==2);}
 sent.clear();now(21.f);qol2_client_command(p1,"sc_ping");assert(sent.empty());   // cooldown
 now(23.f);store(ev(p1),0x174,2);qol2_client_command(p1,"sc_ping");assert(sent.empty());store(ev(p1),0x174,0);  // not while down
 sent.clear();now(25.f);store(&entities[5],0,0);store(&entities[5],4,9);view_seen(&entities[5],p2);assert(qol2_client_command(p1,"sc_ping"));   // a teammate in a scene gets no text
 {int texts=0;for(const auto& s:sent)if(s.type==23&&!s.texts.empty())++texts;assert(texts==1);}clientViews.erase(p2);store(&entities[5],0,1);

 // --- #18 Scare: a laugh at the nearest teammate, 20 s cooldown, alone nobody.
 sounds.clear();lines.clear();now(30.f);assert(qol2_client_command(p1,"sc_scare")&&sounds.size()==1&&sounds[0]=="2:0:ambience/laugh1.wav"&&logged("CLIENT1:You scared Bob."));
 now(35.f);qol2_client_command(p1,"sc_scare");assert(sounds.size()==1&&logged("CLIENT1:Not yet."));
 now(60.f);store(p2,0,1);lines.clear();qol2_client_command(p1,"sc_scare");assert(sounds.size()==1&&logged("CLIENT1:Nobody to scare."));store(p2,0,0);

 // --- #17 Run timer: counts story map time, splits at chapter cards, kept in a save.
 run_timer_set(0);now(100.f);run_timer_frame();now(100.5f);run_timer_frame();now(101.f);run_timer_frame();assert(std::fabs(runTimer.seconds-1.0)<0.01);
 now(1.f);run_timer_frame();assert(std::fabs(runTimer.seconds-1.0)<0.01);   // a new map starts its clock again: nothing lost or added
 runTimer.seconds=3725;cvars["sc_timer"]=1;host_options_frame();sent.clear();
 message_begin(1,82,nullptr,p1);write_string("Chapter 2");write_string("Who is that doctor?");message_end();
 message_begin(1,82,nullptr,p2);write_string("Chapter 2");write_string("Who is that doctor?");message_end();assert(runTimer.shown=="Chapter 2: run 1:02:05 (last chapter 1:02:05)");
 now(5.f);run_timer_frame();now(8.5f);run_timer_frame();{int shown=0;for(const auto& s:sent)if(s.type==23&&!s.texts.empty()&&s.texts[0]==runTimer.shown)++shown;assert(shown==2);}
 message_begin(1,82,nullptr,p1);write_string("Game over");message_end();assert(runTimer.title=="Chapter 2");
 now(70.f);message_begin(1,82,nullptr,p1);write_string("Chapter 2");message_end();assert(runTimer.splitDue<0&&runTimer.chapterStart==3725);   // the same card after a game over restart: no new split
 std::filesystem::create_directories(dir/"storycoop_saves");run_timer_save(2);assert(read(dir/"storycoop_saves"/"slot2.cofrun")=="3725"&&run_timer_saved(2)==3725&&run_timer_saved(3)==0);
 cvars["sc_timer"]=2;now(9.f);host_options_frame();sent.clear();now(9.5f);run_timer_frame();{const Sent* t=last_type(23);assert(t&&t->texts.size()==1&&t->texts[0]=="1:02:05");}
 cvars["sc_timer"]=0;now(10.f);host_options_frame();

 // --- #35 Black and white for the player who chose it: sent once per map, and kept on.
 sent.clear();noir_frame(p2);noir_frame(p2);noir_frame(p1);assert(count_type(81)==1&&last_type(81)->to==p2&&last_type(81)->bytes==std::vector<int>{255});   // full strength
 message_begin(1,81,nullptr,p2);write_byte(0);message_end();assert(last_type(81)->bytes==std::vector<int>{255});   // a scene's "off" stays on for him
 message_begin(1,81,nullptr,p1);write_byte(0);message_end();assert(last_type(81)->bytes==std::vector<int>{0});   // the others as the game says

 // --- #10/#30 Outline: far teammates only, in their colour, not in scenes.
 std::array<unsigned char,0x100> state{};assert(!outline_state(state.data(),p2,p1));
 cvars["sc_outline"]=1;now(11.f);host_options_frame();assert(!outline_state(state.data(),p2,p1));   // 200 units: too near
 store(ev(p2),8,900.f);assert(outline_state(state.data(),p2,p1)&&load<int>(state.data(),0x54)==19&&state[0x50]==235&&state[0x51]==220&&state[0x52]==50);
 store(&entities[5],0,0);store(&entities[5],4,9);view_seen(&entities[5],p1);state={};assert(!outline_state(state.data(),p2,p1));clientViews.erase(p1);store(&entities[5],0,1);
 store(ev(p2),8,200.f);cvars["sc_outline"]=0;now(11.6f);host_options_frame();

 // --- #16 Hunted: off by default; with the model, he comes from a walked spot out of sight, once.
 qol2_map_changed();qol2_world_spawned();assert(!hunt.precached);
 cvars["sc_hunted"]=1;now(12.f);host_options_frame();qol2_world_spawned();assert(!hunt.precached&&logged("models/booksimon.mdl is missing"));
 std::filesystem::create_directories(dir/"models");{std::ofstream(dir/"models"/"booksimon.mdl")<<"m";}
 qol2_world_spawned();assert(hunt.precached&&models.back()=="models/booksimon.mdl");
 // The team walked from x=-300 eastwards; a crumb behind the wall (x 1200) is out of sight.
 hunt.crumbs={{10.f,{1200.f,0.f,36.f}},{12.f,{-1200.f,0.f,36.f}}};now(100.f);hunt_frame();assert(!hunt.who);   // first visit: waits 150 s
 now(260.f);hunt_frame();assert(hunt.who&&live(hunt.who)&&std::fabs(load<float>(ev(hunt.who),8)-1200.f)<0.1f&&logged("HUNTED Book Simon appears at 1200 0 36"));
 {const Sent* t=last_type(77);assert(t&&t->texts==std::vector<std::string>{"Something is following you."});}
 assert(hunt_entity(hunt.who)&&!hunt_entity(z)&&!hunt_entity(p1));   // never kept in a map state or a save (state_capture)
 // A scene: he goes, and comes back no sooner than 4 min later.
 store(&entities[5],0,0);store(&entities[5],4,9);view_seen(&entities[5],p2);now(270.f);hunt_frame();assert(!hunt.who&&logged("HUNTED Book Simon gone (a scene started)"));clientViews.erase(p2);store(&entities[5],0,1);
 now(300.f);hunt_frame();assert(!hunt.who);
 // Killed: he comes back later.
 now(520.f);hunt.crumbs.push_back({400.f,{1300.f,40.f,36.f}});hunt_frame();assert(hunt.who);store(ev(hunt.who),0x164,0.f);now(530.f);hunt_frame();assert(!hunt.who&&logged("HUNTED Book Simon is down"));
 // Never seen near: he goes after 3 min.
 now(775.f);hunt_frame();assert(hunt.who);now(960.f);hunt_frame();assert(!hunt.who&&logged("he never caught up"));
 cvars["sc_hunted"]=0;

 // v0.10.1: a player who leaves takes his slot's records along, so the next one in that slot is asked again (sc_report).
 netTuned.insert(2);modelReports[2]={"simon_camo","auto"};playerLanguage[2]=3;playerPrefs[2].colour=5;qol2_disconnect(p2);
 assert(!netTuned.count(2)&&!modelReports.count(2)&&!playerLanguage.count(2)&&!playerPrefs.count(2));

 // --- v0.10.1 boss review (docs/BOSS_REVIEW.md): the fight starts with its bar.
 {nextFree=40;qol2_map_changed();coop_map_changed();engine[6]=reinterpret_cast<void*>(&changelevel_);player(1,0,0);player(2,200,0);
  auto saw=monster(20,"monster_bosschainsaw",1000.f);store(ev(saw),0x1d0,intern("theboss"));
  auto holder=monster(22,"monster_generic",100.f);store(ev(holder),0x1d0,intern("fagnerd"));
  auto bar=&entities[21];*bar={};store(bar,0,0);store(bar,4,421);store(ev(bar),0,intern("cof_bosshealthbar"));store(ev(bar),0x1d0,intern("bossbar"));store(ev(bar),0x1d8,intern("theboss"));
  // Monster health by party size: the bar's target and a monster_generic keep theirs.
  cvars["sc_scale"]=1;hostOptions.next=-1;host_options_frame();auto z2=monster(23,"monster_zombie",100.f);scale_frame();
  assert(load<float>(ev(holder),0x164)==100.f&&load<float>(ev(saw),0x164)==1000.f&&std::fabs(load<float>(ev(z2),0x164)-135.f)<0.01f);cvars["sc_scale"]=0;host_options_frame();
  // A death before the fight is not a game over; neither does a fall wait out the boss.
  store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);boss_frame();assert(!bossOver&&!boss_fight());
  boss_bar_lookup(nullptr,"targetname","bossbar",bar);assert(bossBarSeen&&boss_fight()&&logged("BOSS fight starts")&&bossDownBefore.size()==1);
  boss_frame();assert(!bossOver);   // down before the bar: the fight goes on
  store(ev(p2),0x174,0);store(ev(p2),0x164,50.f);boss_frame();assert(!bossOver&&bossDownBefore.empty());   // revived
  // A player who comes in during the fight is shown the bar (its own Use, which sends it to everyone).
  used.clear();player(3,300,0);boss_frame();assert(std::count(used.begin(),used.end(),std::string("cof_bosshealthbar"))==1&&logged("BOSS health bar shown to player=3"));
  boss_frame();assert(std::count(used.begin(),used.end(),std::string("cof_bosshealthbar"))==1);store(&entities[3],0,1);
  store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);boss_frame();assert(bossOver&&logged("BOSS player died"));
  // After a load inside the fight (autosave, game over) the used bar (iuser1 1) carries the fight on.
  coop_map_changed();store(ev(p2),0x174,0);store(ev(p2),0x164,100.f);store(ev(bar),0x248,1);boss_frame();assert(bossBarSeen&&logged("BOSS fight goes on after the load"));
  coop_map_changed();store(ev(saw),0x174,2);boss_frame();assert(!bossBarSeen);store(ev(saw),0x174,0);   // a dead boss: no fight
  // Hunted: never on a boss map.
  qol2_map_changed();cvars["sc_hunted"]=1;now(12.f);hostOptions.next=-1;host_options_frame();qol2_world_spawned();assert(hunt.precached);
  now(500.f);hunt.crumbs={{10.f,{1200.f,0.f,36.f}}};hunt_frame();now(800.f);hunt_frame();assert(!hunt.who&&hunt.bossMap&&logged("a boss fight is here"));cvars["sc_hunted"]=0;
  store(saw,0,1);store(holder,0,1);store(bar,0,1);store(z2,0,1);}

 std::puts("PASS v0.10 fixture: options, hardcore, scaling, dodge, ping, scare, run timer, black and white, outline, hunted, bosses");
 return 0;
}
