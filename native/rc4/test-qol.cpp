// Owned SDK fixture for qol-runtime.hpp (v0.9 QoL pack): server cfg, per-player subtitles, unstuck,
// down notice and revive groan, objective reminder, team HUD switch and busy tags, the client-side team
// line, HUD key, bag report, comfort settings and the session file. No game code is loaded and no game
// is launched; the real client.dll bytes are checked by the gamebytes suite.
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
std::vector<std::string> strings{"","s4_start","player","c_game_menu1"};
std::unordered_map<Edict*,std::map<std::string,std::string>> helpers;
std::vector<std::string> lines,serverCommands,argv,precached,sounds;std::string gameDir;int nextFree=20;
std::map<int,std::string> stuffed;std::map<std::string,float> cvars{{"cof_subtitlelanguage",1.f},{"hud_draw",1.f}};std::vector<std::pair<std::string,float>> cvarSets;
struct Sent {int dest=0,type=0;Edict* to=nullptr;std::vector<int> bytes;std::vector<std::string> texts;};
std::vector<Sent> sent;Sent building;
bool logged(const char* part){for(const auto& l:lines)if(l.find(part)!=std::string::npos)return true;return false;}
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
const char* str(int n){return n>=0&&n<static_cast<int>(strings.size())?strings[n].c_str():"";}
int index(Edict* e){for(int i=0;i<32;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<32?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void free_(Edict* e){mem.erase(e);}
void print(const char* s){lines.emplace_back(s);}
void client_print(Edict* e,int,const char* s){lines.emplace_back("CLIENT"+std::to_string(index(e))+":"+s);}
void server_command(const char* s){serverCommands.emplace_back(s);}
void __cdecl stuff_(Edict* e,const char* format,...){char text[1024]{};va_list a;va_start(a,format);std::vsnprintf(text,sizeof(text),format,a);va_end(a);stuffed[index(e)]+=text;}
const char* arg(int i){return i>=0&&i<static_cast<int>(argv.size())?argv[i].c_str():"";}
void dir_(char* out){std::strcpy(out,gameDir.c_str());}
int listen(){return 0;}
float cvar_get(const char* n){auto it=cvars.find(n);return it==cvars.end()?0.f:it->second;}
void cvar_set(const char* n,float v){cvars[n]=v;cvarSets.emplace_back(n,v);}
int precache_(const char* s){precached.emplace_back(s);return static_cast<int>(precached.size());}
void sound_(Edict* e,int channel,const char* s,float,float,int,int){sounds.push_back(std::to_string(index(e))+":"+std::to_string(channel)+":"+s);}
std::int32_t random_(std::int32_t low,std::int32_t){return low;}
char* info_buffer(Edict* e){return reinterpret_cast<char*>(e);}
char* info_value(char* buffer,char* key){static std::string out;out=std::strcmp(key,"name")?"":index(reinterpret_cast<Edict*>(buffer))==1?"Katz":"Bob";return const_cast<char*>(out.c_str());}
void begin_(int dest,int type,const float*,Edict* e){building={};building.dest=dest;building.type=type;building.to=e;}
void byte_(int v){building.bytes.push_back(v);}
void string_(const char* s){building.texts.emplace_back(s?s:"");}
void end_(){sent.push_back(building);building={};}
// World: floor z=0 (a standing origin is at z>=36), a wall at x>1000, a pillar at x 480..520, y -20..20.
bool solid(const float* p){return p[0]>1000||p[2]<36||(p[0]>464&&p[0]<536&&p[1]>-36&&p[1]<36);}
void hull(const float* a,const float* b,int,int hullNumber,Edict*,Trace* t){
 assert(hullNumber==1||hullNumber==3);*t={};
 if(solid(a)){t->startSolid=1;t->allSolid=solid(b);return;}t->fraction=1;
 if(b[2]<a[2]){if(b[2]<36)t->fraction=(a[2]-36)/(a[2]-b[2]);return;}
 for(float s=0.f;s<=1.f;s+=0.05f){const float p[3]={a[0]+(b[0]-a[0])*s,a[1]+(b[1]-a[1])*s,a[2]};if(solid(p)){t->fraction=s;return;}}
}
int contents(const float*){return -1;}
Edict* find_(Edict* start,const char* key,const char* value){
 const int field=std::strcmp(key,"classname")==0?0:0x1d0;
 for(int i=start?index(start)+1:1;i<32;++i)if(load<int>(&entities[i],0)==0&&std::strcmp(str(load<int>(ev(&entities[i]),field)),value)==0)return &entities[i];
 return &entities[0];
}
Edict* created(int type){auto e=&entities[nextFree++];assert(nextFree<32);*e={};store(e,0,0);store(e,4,500+nextFree);store(ev(e),0,type);return e;}
void remove_(Edict* e){release(e);store(e,0,1);helpers.erase(e);--nextFree;}
int spawn_(Edict*){return 0;}
void key_(Edict* e,KeyValue* kv){
 helpers[e][kv->key]=kv->value;kv->handled=1;
 if(!std::strcmp(kv->key,"targetname"))store(ev(e),0x1d0,intern(kv->value));
 if(!std::strcmp(kv->key,"origin")){float x=0,y=0,z=0;assert(std::sscanf(kv->value,"%f %f %f",&x,&y,&z)==3);store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,z);}
}
int teleports=0;
void use_(Edict* h,Edict* actor){
 assert(!std::strcmp(classname(h),"cof_teleport")&&helpers[h]["target"]=="sc4_coop_landing");
 auto marker=find_next(nullptr,"targetname","sc4_coop_landing");assert(marker);
 store(ev(actor),8,load<float>(ev(marker),8));store(ev(actor),12,load<float>(ev(marker),12));store(ev(actor),16,load<float>(ev(marker),16)+37.f);++teleports;
}
void player(int i,float x,float y){
 auto e=&entities[i];*e={};store(e,0,0);store(e,4,100+i);store(ev(e),0,intern("player"));store(ev(e),8,x);store(ev(e),12,y);store(ev(e),16,36.f);store(ev(e),0x164,100.f);
 allocate(e,PlayerBytes);auto& a=allocations.at(e);a.commands=true;a.hudDue=0;
}
void now(float t){store(globals,0,t);}
const Sent* last_to(int type,Edict* e){for(auto it=sent.rbegin();it!=sent.rend();++it)if(it->type==type&&it->to==e)return &*it;return nullptr;}
void write(const std::filesystem::path& p,const std::string& text){std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f<<text;}
std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return std::string((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());}
bool bagVisible=false;
bool __fastcall fake_visible(void*,void*){return bagVisible;}
}
int main(int argc,char** argv_){
 assert(argc==2);gameDir=argv_[1];
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,4);store(globals,0x94,32);now(5.f);
 engine[1]=reinterpret_cast<void*>(&precache_);engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&created);
 engine[29]=reinterpret_cast<void*>(&sound_);engine[34]=reinterpret_cast<void*>(&hull);engine[39]=reinterpret_cast<void*>(&server_command);engine[41]=reinterpret_cast<void*>(&stuff_);engine[45]=reinterpret_cast<void*>(&contents);
 engine[46]=reinterpret_cast<void*>(&begin_);engine[47]=reinterpret_cast<void*>(&end_);engine[48]=reinterpret_cast<void*>(&byte_);engine[54]=reinterpret_cast<void*>(&string_);
 engine[57]=reinterpret_cast<void*>(&cvar_get);engine[59]=reinterpret_cast<void*>(&cvar_set);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[65]=reinterpret_cast<void*>(&free_);engine[66]=reinterpret_cast<void*>(&str);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);
 engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);engine[80]=reinterpret_cast<void*>(&client_print);engine[81]=reinterpret_cast<void*>(&print);engine[83]=reinterpret_cast<void*>(&arg);
 engine[90]=reinterpret_cast<void*>(&random_);engine[99]=reinterpret_cast<void*>(&dir_);engine[106]=reinterpret_cast<void*>(&info_buffer);engine[107]=reinterpret_cast<void*>(&info_value);engine[115]=reinterpret_cast<void*>(&listen);
 original[1]=reinterpret_cast<void*>(&spawn_);original[3]=reinterpret_cast<void*>(&use_);original[6]=reinterpret_cast<void*>(&key_);
 for(auto& e:entities)store(&e,0,1);store(&entities[0],0,0);store(ev(&entities[0]),0,intern("worldspawn"));
 proFontMessage=77;objectivesMessage=78;noteDoc=79;controlHintMessage=80;
 const std::filesystem::path dir=gameDir;auto p1=&entities[1],p2=&entities[2];
 player(1,0,0);player(2,200,0);

 // --- 1. storycoop_server.cfg runs at the start of each story map, only if it is there.
 server_cfg_exec();assert(serverCommands.empty()&&logged("SERVER storycoop_server.cfg not found"));
 write(dir/"storycoop_server.cfg","// sv_coopcprtime 5\n");server_cfg_exec();assert(serverCommands.back()=="exec storycoop_server.cfg\n");
 store(globals,0x0c,3);serverCommands.clear();server_cfg_exec();assert(serverCommands.empty());store(globals,0x0c,1);   // never on the menu map

 // --- 9. Subtitles in each player's own language.
 assert(subtitle_shown("Hello there*rest")=="Hello there"&&subtitle_shown("*star first")=="*star first"&&subtitle_shown(std::string(300,'a')).empty());
 assert(language_file(1,"hints.txt")=="txtfiles/hints.txt"&&language_file(0,"hints.txt")=="txtfiles/hints.txt"&&language_file(3,"hints.txt")=="txtfiles/languages/french/hints.txt"&&language_file(7,"x")=="txtfiles/languages/swedish/x");
 write(dir/"txtfiles"/"subtitles.txt","Hello there*colour\nSecond line\nx\nThird line\n");
 write(dir/"txtfiles"/"languages"/"french"/"subtitles.txt","Bonjour*couleur\nDeuxieme ligne\ny\n");
 // A joiner reports his own language with sc_mdl (third word); the host's own report is not used.
 argv={"sc_mdl","simon","auto","3"};assert(model_report_command(p2,"sc_mdl")&&playerLanguage[2]==3&&logged("LANGUAGE player=2 reads subtitles in 3"));
 assert(model_report_command(p1,"sc_mdl")&&!playerLanguage.count(1));
 argv={"sc_mdl","simon","auto"};assert(model_report_command(p2,"sc_mdl")&&playerLanguage[2]==3);   // an older game: unchanged
 std::string keep;
 assert(std::string(subtitle_for(p2,"Hello there",keep))=="Bonjour"&&std::string(subtitle_for(p2,"Second line",keep))=="Deuxieme ligne");
 assert(std::string(subtitle_for(p1,"Hello there",keep))=="Hello there");             // the host reads the host's language
 assert(std::string(subtitle_for(p2,"Third line",keep))=="Third line");               // no French line 4: as sent
 assert(std::string(subtitle_for(p2,"Picked up a key",keep))=="Picked up a key");     // not a subtitle line
 // Through the real message hooks: a ProFont line to the joiner, and the Objectives hints file.
 message_begin(8,77,nullptr,p2);write_byte(5);write_string("Hello there");message_end();assert(sent.back().to==p2&&sent.back().texts.at(0)=="Bonjour");
 message_begin(8,77,nullptr,p1);write_byte(5);write_string("Hello there");message_end();assert(sent.back().texts.at(0)=="Hello there");
 message_begin(1,78,nullptr,p2);write_string("objective_key");write_string("txtfiles/hints.txt");write_byte(0);message_end();
 assert(sent.back().texts.at(0)=="objective_key"&&sent.back().texts.at(1)=="txtfiles/languages/french/hints.txt");
 message_begin(1,78,nullptr,p2);write_string("objective_key");write_string("maps/my_hints.txt");write_byte(0);message_end();assert(sent.back().texts.at(1)=="maps/my_hints.txt");   // a map's own file stays
 // "subtitleset" from a joiner sets only his own game; from the host it runs as always.
 argv={"subtitleset","4"};assert(language_command(p2,"subtitleset")&&playerLanguage[2]==4&&stuffed[2]=="cof_subtitlelanguage 4\n");
 argv={"subtitleset","6"};assert(!language_command(p1,"subtitleset")&&hostLanguage==6);
 // The host's value is put back while a map starts, never later.
 hostLanguage=-1;cvars["cof_subtitlelanguage"]=2.f;language_activate();assert(hostLanguage==2);cvars["cof_subtitlelanguage"]=1.f;language_frame();assert(cvars["cof_subtitlelanguage"]==2.f);
 now(30.f);cvars["cof_subtitlelanguage"]=5.f;language_frame();assert(cvars["cof_subtitlelanguage"]==5.f&&hostLanguage==5);   // later: the host's own choice is followed
 store(globals,0x0c,3);language_activate();now(5.f);cvars["cof_subtitlelanguage"]=6.f;language_frame();assert(hostLanguage==6);store(globals,0x0c,1);   // the menu map: no windowcvars["cof_subtitlelanguage"]=1.f;hostLanguage=1;now(5.f);
 playerLanguage[2]=3;

 // --- 4./5. Down notice to the others, revive groan by model, objective after a respawn.
 groans_precache();assert(precached.size()==6&&precached[0]=="player/vocal/LowHealth/lhealth1.wav"&&precached[5]=="player/player_out_of_breath.wav");
 objectiveText="hint_find_key";objectiveFile.clear();modelStates[2].want="police2";
 down_frame(p2);sent.clear();
 store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);down_frame(p2);
 assert(sent.size()==1&&sent[0].to==p1&&sent[0].type==77&&sent[0].bytes==std::vector<int>{1}&&sent[0].texts==std::vector<std::string>{"Bob is down"});
 down_frame(p2);assert(sent.size()==1);                                                   // once per fall
 store(ev(p2),PlayerMode,3.f);down_frame(p2);assert(sounds.size()==1&&sounds[0]=="2:2:player/police/LowHealth/lhealth1.wav");
 store(ev(p2),0x174,0);store(ev(p2),0x164,30.f);store(ev(p2),PlayerMode,0.f);down_frame(p2);assert(sent.size()==1);   // revived: no reminder
 store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);down_frame(p2);store(ev(p2),0x174,0);store(ev(p2),0x164,100.f);down_frame(p2);   // respawned
 {const Sent* o=last_to(78,p2);assert(o&&o->texts==std::vector<std::string>({"hint_find_key","txtfiles/languages/french/hints.txt"})&&o->bytes==std::vector<int>{1});}
 modelStates[1].want="simon_camo";sounds.clear();revive_groan(p1);assert(sounds[0]=="1:2:player/vocal/LowHealth/lhealth1.wav");
 assert(groan_for(p2)==std::string("player/police/LowHealth/lhealth1.wav"));

 // --- 6. A player first ready more than 15 s into a map is reminded 3 s later; one who came at the start is not.
 qol_map_changed();sent.clear();now(10.f);objective_late_frame(p1);now(30.f);objective_late_frame(p2);assert(!last_to(78,p2));
 now(33.5f);objective_late_frame(p2);assert(last_to(78,p2));objective_late_frame(p1);assert(!last_to(78,p1));
 sent.clear();now(40.f);objective_late_frame(p2);assert(sent.empty());                    // once

 // --- 3. Unstuck: refusals, inside a pillar, beside the partner, alone, cooldown.
 qol_map_changed();now(50.f);lines.clear();
 store(ev(p1),0x174,2);argv={"sc_unstuck"};assert(qol_client_command(p1,"sc_unstuck")&&logged("CLIENT1:Unstuck is off while you are down."));store(ev(p1),0x174,0);
 store(ev(p1),8,500.f);store(ev(p1),12,0.f);                                                // inside the pillar
 assert(hull_stuck(p1,{500.f,0.f,36.f}));teleports=0;sent.clear();
 assert(qol_client_command(p1,"sc_unstuck")&&teleports==1&&!solid(std::array<float,3>{load<float>(ev(p1),8),load<float>(ev(p1),12),load<float>(ev(p1),16)}.data()));
 assert(logged("Moved out of the wall.")&&logged("UNSTUCK player=1 at 500 0 36 (inside something): out of the wall")&&last_to(78,p1));
 lines.clear();assert(qol_client_command(p1,"sc_unstuck")&&logged("CLIENT1:Unstuck again in 10 s.")&&teleports==1);
 now(61.f);store(ev(p1),8,-800.f);store(ev(p1),12,0.f);store(ev(p2),8,200.f);store(ev(p2),12,0.f);
 assert(qol_client_command(p1,"sc_unstuck")&&teleports==2&&logged("beside your partner"));
 {const float dx=load<float>(ev(p1),8)-200.f,dy=load<float>(ev(p1),12);assert(dx*dx+dy*dy<250.f*250.f);}
 now(72.f);store(p2,0,1);store(ev(p1),8,-800.f);store(ev(p1),12,0.f);                     // alone now
 assert(qol_client_command(p1,"sc_unstuck")&&teleports==3&&logged("to a clear spot nearby"));
 {const float dx=load<float>(ev(p1),8)+800.f,dy=load<float>(ev(p1),12);assert(dx*dx+dy*dy>=47.f*47.f);}
 store(p2,0,0);now(83.f);loadWait.armed=true;lines.clear();assert(qol_client_command(p1,"sc_unstuck")&&logged("Wait until everyone has loaded."));loadWait.armed=false;
 store(&entities[5],0,0);store(&entities[5],4,9);view_seen(&entities[5],p1);lines.clear();assert(qol_client_command(p1,"sc_unstuck")&&logged("Unstuck is off during a scene."));
 clientViews.erase(p1);store(&entities[5],0,1);
 // The key prompt: once, 8 s after the player is ready, from the installed hints file.
 sent.clear();now(100.f);unstuck_hint_frame(p1);assert(sent.empty());now(108.5f);unstuck_hint_frame(p1);
 {const Sent* h=last_to(80,p1);assert(h&&h->texts==std::vector<std::string>({"storycoop_unstuck","txtfiles/storycoop_hints.txt"}));}
 sent.clear();now(200.f);unstuck_hint_frame(p1);assert(sent.empty());

 // --- 7. Team HUD switch and busy tags, sent only when they change.
 qol_map_changed();stuffed.clear();now(300.f);store<int>(pd(p2),PlayerUsingComputerOrPhone,2);
 team_hud_frame();assert(stuffed[1]=="sc_teamhud 1\nsc_busy 1 0\nsc_busy 2 1\nsc_busy 3 0\nsc_busy 4 0\n"&&stuffed[2]==stuffed[1]);
 stuffed.clear();now(300.1f);team_hud_frame();assert(stuffed.empty());                    // 0.2 s between looks
 now(300.3f);team_hud_frame();assert(stuffed.empty());                                    // nothing changed
 argv={"sc_bag","1"};assert(qol_client_command(p1,"sc_bag"));now(300.6f);team_hud_frame();assert(stuffed[1]=="sc_busy 1 3\n"&&stuffed[2]=="sc_busy 1 3\n");
 stuffed.clear();busy_note_opened(1,79,p1);now(300.9f);team_hud_frame();assert(stuffed[1]=="sc_busy 1 2\n");
 stuffed.clear();assert(!qol_client_command(p1,"closedocument"));now(301.2f);team_hud_frame();assert(stuffed[1]=="sc_busy 1 3\n");
 stuffed.clear();store<int>(pd(p2),PlayerUsingComputerOrPhone,1);store(&entities[5],0,0);view_seen(&entities[5],p2);now(301.5f);team_hud_frame();
 assert(stuffed[2]=="sc_teamhud 0\nsc_busy 2 4\n"&&stuffed[1]=="sc_busy 2 4\n");          // the black box: hidden for him only
 stuffed.clear();argv={"sc_bag","0"};qol_client_command(p1,"sc_bag");store(ev(p2),0x174,2);store(ev(p2),0x164,0.f);now(301.8f);team_hud_frame();
 assert(stuffed[1]=="sc_busy 1 0\nsc_busy 2 0\n");                                        // a downed player is not busy
 store(ev(p2),0x174,0);store(ev(p2),0x164,100.f);clientViews.erase(p2);store(&entities[5],0,1);
 assert(qol_client_command(p1,"sc_teamhud")&&qol_client_command(p1,"sc_busy"));             // an older game's echo is dropped

 // --- Client side: the team line and the list switch in a fake client image.
 {
  std::vector<unsigned char> image(0x600000);clientImage=image.data();clientImageSize=image.size();const std::uint32_t base=reinterpret_cast<std::uint32_t>(clientImage);
  assert(team_line_shipped(ClientPreferredBase).size()==28&&team_line_ours(ClientPreferredBase,0x20000000).size()==28);
  const auto shipped=team_line_shipped(base);std::memcpy(image.data()+0x3C807,shipped.data(),shipped.size());
  store<std::uint32_t>(image.data(),0x3C66D,TeamFour-ClientPreferredBase+base);
  const char* name="Katz";store<const char*>(image.data(),0x556CC4,name);store<short>(image.data(),0x556CC8,40);store<int>(image.data(),0x18D66C,85);
  team_line_patch();assert(teamLinePatched&&std::equal(image.begin()+0x3C807,image.begin()+0x3C807+28,team_line_ours(base,reinterpret_cast<std::uint32_t>(&teamRows[0])).begin()));
  // push [ebx+distance] with ebx = row 0 of g_PlayerInfoList reads teamRows[0].line.
  assert(base+0x556CC4+load<std::uint32_t>(image.data(),0x3C807+10)==reinterpret_cast<std::uint32_t>(&teamRows[0].line));
  assert(load<std::uint32_t>(image.data(),0x3C807+15)==base+0x13C71B);
  assert(std::string(teamRows[0].line)=="Katz - Health 85 - Ping 40");
  argv={"sc_busy","1","1"};team_busy_command();argv={"sc_busy","40","1"};team_busy_command();argv={"sc_busy","2","9"};team_busy_command();
  team_rows_update();assert(std::string(teamLines[0])=="Katz - Health 85 - Ping 40 (phone)"&&teamTags[2]==0&&std::string(teamLines[1])==" - Health 0 - Ping 0");
  argv={"sc_teamhud","1"};team_hud_command();assert(load<std::uint32_t>(image.data(),0x3C66D)==base+0x14231D&&teamHudServer);
  argv={"sc_teamhud","0"};team_hud_command();assert(load<std::uint32_t>(image.data(),0x3C66D)==base+0x146894);
  argv={"sc_teamhud","1"};team_hud_command();cvarSets.clear();hud_toggle_command();assert(hudHidden&&cvars["hud_draw"]==0.f&&load<std::uint32_t>(image.data(),0x3C66D)==base+0x146894);
  hud_toggle_command();assert(!hudHidden&&cvars["hud_draw"]==1.f&&load<std::uint32_t>(image.data(),0x3C66D)==base+0x14231D);
  team_hud_reset();assert(load<std::uint32_t>(image.data(),0x3C66D)==base+0x14231C&&!teamHudServer&&teamTags[1]==0);
  store<std::uint32_t>(image.data(),0x3C66D,0x12345678);argv={"sc_teamhud","1"};team_hud_command();assert(load<std::uint32_t>(image.data(),0x3C66D)==0x12345678);   // unknown bytes: untouched
  // The bag panel: gViewPort -> +0x1404 -> isVisible (vtable +0x28), reported once per change to a v0.9 host only.
  void* table[16]{};table[10]=reinterpret_cast<void*>(&fake_visible);void* panel[1]={table};std::vector<unsigned char> viewport(0x1500);store<void*>(viewport.data(),0x1404,panel);
  store<void*>(image.data(),0x1B2600,viewport.data());serverCommands.clear();teamHudServer=false;bagVisible=true;bag_poll();assert(serverCommands.empty());
  teamHudServer=true;bag_poll();bag_poll();assert(serverCommands==std::vector<std::string>{"cmd sc_bag 1\n"});bagVisible=false;bag_poll();assert(serverCommands.back()=="cmd sc_bag 0\n");
  teamLinePatched=false;clientImage=nullptr;clientImageSize=0;
 }
 // --- 8. Comfort settings: once, 4 s after start, and a HUD left hidden comes back.
 serverCommands.clear();processStart=GetTickCount64();comfort_frame();assert(serverCommands.empty()&&!comfortDone);
 processStart=GetTickCount64()-5000;cvars["hud_draw"]=0.f;comfort_frame();assert(comfortDone&&cvars["hud_draw"]==1.f&&serverCommands.empty());   // no file yet: nothing run
 write(dir/"storycoop_comfort.cfg","bind \"k\" \"sc_unstuck\"\n");comfortDone=false;comfort_frame();assert((serverCommands==std::vector<std::string>{"exec storycoop_comfort.cfg\n","bind \"q\" \"sc_dropammo\"\n"}));
 comfort_frame();assert(serverCommands.size()==2);
 // v0.10.4: a comfort file without the drop key gets Q, unless config.cfg binds Q to something else.
 serverCommands.clear();assert(!comfort_drop_key("bind \"q\" \"sc_dropammo\"\n","")&&!comfort_drop_key("bind \"l\" \"sc_dropammo\"\n","")&&serverCommands.empty());
 assert(!comfort_drop_key("bind \"j\" \"sc_scare\"\n","bind \"Q\" \"+lean\"\n")&&serverCommands.empty());
 assert(comfort_drop_key("bind \"j\" \"sc_scare\"\n","bind \"q\" \"sc_dropammo\"\n")&&serverCommands.back()=="bind \"q\" \"sc_dropammo\"\n");
 // --- 11. The session file the launcher reads.
 session_join_started("1.2.3.4:27015");assert(read(dir/"storycoop_session.txt")=="joining 1.2.3.4:27015\n");
 localServerActive=false;session_joined_report();assert(read(dir/"storycoop_session.txt")=="joined 1.2.3.4:27015\n");
 localServerActive=true;session_joined_report();assert(read(dir/"storycoop_session.txt")=="joined 1.2.3.4:27015\n");   // the host's own report changes nothing
 session_state("local");assert(read(dir/"storycoop_session.txt")=="local 1.2.3.4:27015\n");session_closed();assert(read(dir/"storycoop_session.txt")=="closed 1.2.3.4:27015\n");
 assert(command_line_target("cof.exe -game cryoffear +connect 26.1.2.3:27016 +name x")=="26.1.2.3:27016"&&command_line_target("x +connect host.example")=="host.example:27015");
 assert(command_line_target("cof.exe -game cryoffear").empty()&&command_line_target("x +connect bad;name:1").empty()&&command_line_target("x +connect 1.2.3.4:99999").empty());
 std::puts("PASS v0.9 QoL fixture: server cfg, subtitle language, unstuck, down/revive, objective reminder, team HUD, comfort, session file");
 return 0;
}
