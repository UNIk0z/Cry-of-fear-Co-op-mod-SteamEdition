// Owned SDK fixture for session-runtime.hpp (coop-fixes-005): light policy,
// pause attribution, Simon model, main-menu host commands. No game code is
// loaded and no game is launched.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <map>
namespace rc4 {struct Edict {std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,10> entities{};
std::unordered_map<Edict*,std::vector<unsigned char>> mem;
std::array<unsigned char,0xa0> globalData{};
std::vector<std::string> lines,serverCommands,clientCommands,argv;std::string gameDir;
std::unordered_map<int,std::map<std::string,std::string>> info;
const char* strings(int n){switch(n){case 1:return "s4_start";case 2:return "player";case 3:return "c_game_menu1";case 4:return "cof_campaign_story4";case 5:return "police2";case 6:return "Police3";case 7:return "police22";case 8:return "c_start";case 9:return "c_intro";case 10:return "c_lake";case 11:return "c_cof_lake";case 12:return "Chapter-2-Who-is-that-Doctor";case 13:return "Load-Save-Slot-1";}return "";}
float collisionCvarValue=1.f,difficultyValue=0.f;float cvar_get(const char* n){if(!std::strcmp(n,"sc_collision"))return collisionCvarValue;if(!std::strcmp(n,"difficulty"))return difficultyValue;return 0.f;}
int map_valid(char* m){for(const char* v:{"s4_start","cof_campaign_story4","s4_lake_e0","s4_lake_e1"})if(!std::strcmp(m,v))return 1;return 0;}
int index(Edict* e){for(int i=0;i<10;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<10?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void print(const char* s){lines.emplace_back(s);}
void client_print(Edict*,int,const char* s){lines.emplace_back(std::string("CLIENT:")+s);}
void server_command(const char* s){serverCommands.emplace_back(s);}
void __cdecl client_command_(Edict*,const char* format,...){char text[512]{};va_list a;va_start(a,format);std::vsnprintf(text,sizeof(text),format,a);va_end(a);clientCommands.emplace_back(text);}
const char* arg(int i){return i>=0&&i<static_cast<int>(argv.size())?argv[i].c_str():"";}
void game_dir(char* out){std::strcpy(out,gameDir.c_str());}
char* info_buffer(Edict* e){return reinterpret_cast<char*>(e);}
char* info_value(char* buffer,char* key){auto& m=info[index(reinterpret_cast<Edict*>(buffer))];auto it=m.find(key);return const_cast<char*>(it==m.end()?"":it->second.c_str());}
int keySets=0;
std::vector<int> message;std::string messageText;int messageBegins=0;
void begin_(int dest,int type,const float*,Edict*){++messageBegins;message.clear();message.push_back(dest);message.push_back(type);}
void end_(){}
void int_(int v){message.push_back(v);}
void text_(const char* s){messageText=s;}
void set_client_key(int client,char* buffer,char* key,char* value){assert(reinterpret_cast<Edict*>(buffer)==&entities[client]);info[client][key]=value;++keySets;}
int alloc_string(const char* s){return std::strcmp(s,"simon")==0?555:1;}
bool logged(const char* part){for(const auto& l:lines)if(l.find(part)!=std::string::npos)return true;return false;}
template<class T>void field(GlobalSnapshot& g,int token,T v){const std::uint16_t size=sizeof(v),t=static_cast<std::uint16_t>(token);auto append=[&](const void* p,std::size_t n){auto a=static_cast<const unsigned char*>(p);g.bytes.insert(g.bytes.end(),a,a+n);};append(&size,2);append(&t,2);append(&v,sizeof(v));}
Checkpoint save_fixture(const char* map){Checkpoint v;v.map=map;v.owner="auto:host";v.recorder=12;v.globals.tokens.resize(1024);v.globals.tokens[1]="GLOBAL";v.globals.tokens[2]="m_listCount";field(v.globals,1,1);field(v.globals,2,0);
 SavedPlayer p;p.identity=v.owner;p.health=90;p.stamina=50;p.hasPose=true;v.players.push_back(p);return v;}
void reset_players(){
 allocations.clear();mem.clear();lines.clear();light_reset();
 for(int i=0;i<10;++i){entities[i]={};store(&entities[i],4,10+i);store(ev(&entities[i]),0,2);}
 for(int i=1;i<=2;++i){allocate(&entities[i],PlayerBytes);store(ev(&entities[i]),0x164,100.f);}
}
}
int main(int argc,char** argv_){
 assert(argc==2);gameDir=argv_[1];
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,10);store(globals,0,5.f);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[66]=reinterpret_cast<void*>(&strings);engine[68]=reinterpret_cast<void*>(&ev);engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);
 engine[81]=reinterpret_cast<void*>(&print);engine[80]=reinterpret_cast<void*>(&client_print);engine[39]=reinterpret_cast<void*>(&server_command);engine[41]=reinterpret_cast<void*>(&client_command_);engine[83]=reinterpret_cast<void*>(&arg);engine[57]=reinterpret_cast<void*>(&cvar_get);engine[99]=reinterpret_cast<void*>(&game_dir);
 engine[106]=reinterpret_cast<void*>(&info_buffer);engine[107]=reinterpret_cast<void*>(&info_value);engine[109]=reinterpret_cast<void*>(&set_client_key);engine[67]=reinterpret_cast<void*>(&alloc_string);
 engine[46]=reinterpret_cast<void*>(&begin_);engine[47]=reinterpret_cast<void*>(&end_);engine[48]=reinterpret_cast<void*>(&int_);engine[50]=reinterpret_cast<void*>(&int_);engine[54]=reinterpret_cast<void*>(&text_);
 auto e=&entities[1];auto p=pd(e);

 // Lights (coop-fixes-012, user request): the Maglite and the Glock light stay
 // on; only the police flashlight (F key, frags bit 0x20) is kept off.
 reset_players();p=pd(e);store(p,0x1ea8,1);store(p,0x5d0,1u|2u|4u|8u);store(ev(e),0x168,32.f+3.f);store(p,0x5d4,15);
 light_frame(e);
 assert(load<int>(p,0x1ea8)==1&&load<unsigned>(p,0x5d0)==(1u|2u|4u|8u)&&load<int>(p,0x5d4)==-1);
 // A stray 0x20 in FlashFlags is cleared with it, so the rewrite happens once.
 store(p,0x5d0,1u|0x20u);store(p,0x5d4,3);light_frame(e);assert(load<unsigned>(p,0x5d0)==1u&&load<int>(p,0x5d4)==-1);store(ev(e),0x168,1.f);store(p,0x5d4,1);light_frame(e);assert(load<int>(p,0x5d4)==1);
 assert(!logged("Maglite switched off")&&!logged("Glock light switched off")&&logged("LIGHT player=1 flags=f"));
 for(const auto& l:lines)assert(l.find("CLIENT:")==std::string::npos);
 assert(light_command(e,"flashlight")&&!light_command(e,"weapontoggle")&&logged("police flashlight refused")&&logged("CLIENT:No F flashlight in co-op."));
 // The phone light alone is never touched.
 reset_players();p=pd(e);store(p,0x5d0,2u);store(ev(e),0x168,2.f);store(p,0x5d4,2);light_frame(e);assert(load<unsigned>(p,0x5d0)==2u&&load<int>(p,0x5d4)==2);
 // Outside the co-op campaign the light policy is inert.
 store(globals,0x0c,3);store(p,0x1ea8,1);light_frame(e);assert(load<int>(p,0x1ea8)==1&&!light_command(e,"flashlight"));store(globals,0x0c,1);

 // Cutscene hold also forces the frags/ScoreInfo resync.
 reset_players();p=pd(e);camera_reset();store(p,0x5d4,7);cinematic_hold(e,p);assert(load<int>(p,0x5d4)==-1);

 // Simon model (coop-fixes-007): the world's per-player models name simon, so
 // the game's own ClientUserInfoChanged keeps it; at most 3 sets per map.
 reset_players();info.clear();info[1]["model"]="police1";info[2]["model"]="police1";
 std::vector<unsigned char> image(WorldPointerRva+16);backend=reinterpret_cast<HMODULE>(image.data());
 auto world=alloc(&entities[0],0x200);std::memcpy(image.data()+WorldPointerRva,&world,4);
 model_activate();assert(!simonModel&&load<int>(world,0xe0)==0);model_frame(e);assert(info[1]["model"]=="police1");
 std::filesystem::create_directories(std::filesystem::path(gameDir)/"models"/"player"/"simon");{std::ofstream f(std::filesystem::path(gameDir)/"models"/"player"/"simon"/"simon.mdl");f<<"IDST";}
 store(world,0xe4,77);   // the map's own model for player 2 wins
 model_activate();assert(simonModel&&load<int>(world,0xe0)==555&&load<int>(world,0xe4)==77&&load<int>(world,0xe8)==555&&load<int>(world,0xec)==555&&logged("world player models -> simon (3 of 4)"));
 store(globals,0,10.f);model_frame(e);model_frame(&entities[2]);assert(info[1]["model"]=="simon"&&info[2]["model"]=="police1"&&logged("MODEL player=1 now simon (was police1, try 1)")&&logged("MODEL player=2 keeps the map's model"));
 // Already simon: logged once, nothing set.
 store(globals,0,11.5f);model_frame(e);assert(keySets==1&&logged("MODEL player=1 is simon"));
 // coop-fixes-008: checked once a second, never more than 3 sets per map.
 for(int i=0;i<10;++i){info[1]["model"]="police1";model_frame(e);model_frame(e);store(globals,0,12.6f+i*1.1f);}
 assert(keySets==3&&logged("MODEL player=1 now simon (was police1, try 3)")&&logged("MODEL player=1 still police1 after 3 tries"));
 // An emptied world field is refilled for the player.
 store(world,0xe0,0);info[1]["model"]="police1";store(world,0xe4,0);model_activate();store(globals,0,30.f);store(world,0xe0,0);model_frame(e);model_frame(&entities[2]);
 assert(load<int>(world,0xe0)==555&&info[1]["model"]=="simon"&&info[2]["model"]=="simon"&&keySets==5);
 store(globals,0,5.f);
 // A world object that is not the pinned CWorld is never written.
 {void* other=nullptr;std::memcpy(image.data()+WorldPointerRva,&other,4);std::memset(world,0,0x200);model_activate();assert(load<int>(world,0xe0)==0&&logged("world object not recognized"));std::memcpy(image.data()+WorldPointerRva,&world,4);}
 // coop-fixes-009: the passive opening maps get Simon too; the main menu does not.
 std::memset(world,0,0x200);store(globals,0x0c,4);info[1]["model"]="police1";model_activate();store(globals,0,40.f);model_frame(e);
 assert(load<int>(world,0xe0)==555&&info[1]["model"]=="simon"&&logged("MODEL player=1 now simon"));
 std::memset(world,0,0x200);store(globals,0x0c,3);model_activate();assert(load<int>(world,0xe0)==0);store(globals,0x0c,1);store(globals,0,5.f);
 // coop-fixes-011: the worldspawn's police1..police4 (s4_nightmare) are the
 // game's default, replaced like empty fields; other named models still win.
 std::memset(world,0,0x200);store(globals,0x0c,4);store(world,0xe0,5);store(world,0xe4,6);store(world,0xe8,7);store(world,0xec,77);model_activate();
 assert(load<int>(world,0xe0)==555&&load<int>(world,0xe4)==555&&load<int>(world,0xe8)==7&&load<int>(world,0xec)==77&&logged("world player models -> simon (2 of 4)"));
 store(world,0xe0,5);info[1]["model"]="police1";store(globals,0,50.f);model_frame(e);assert(load<int>(world,0xe0)==555&&info[1]["model"]=="simon");
 store(globals,0x0c,1);store(globals,0,5.f);
 // Single player: inert.
 std::memset(world,0,0x200);store(globals,0x90,1);model_activate();assert(load<int>(world,0xe0)==0);store(globals,0x90,2);backend=nullptr;

 // Pause attribution, user scenario: the host pauses, the partner opens and
 // closes the menu (unpause) -> the host re-pauses; closing the host menu
 // resumes. Times are milliseconds.
 {PauseState s;assert(!pause_step(s,false,false,500));
  assert(!pause_step(s,false,true,1000));assert(!pause_step(s,true,true,1040)&&!s.othersOpen);
  assert(pause_step(s,false,true,9000)&&!s.othersOpen&&s.selfPending);
  assert(!pause_step(s,true,true,9030)&&!s.selfPending&&!s.othersOpen);
  assert(!pause_step(s,true,false,12000));assert(!pause_step(s,false,false,12020)&&!s.othersOpen);
  assert(!pause_step(s,false,false,12500));}
 // The host opened its menu but its own setpause is late: the guard helps after 250 ms.
 {PauseState s;assert(!pause_step(s,false,true,1000));assert(!pause_step(s,false,true,1200));assert(pause_step(s,false,true,1300));}
 // No re-pause while a new map is loading.
 {PauseState s;s.quietUntil=8000;s.othersOpen=true;assert(!pause_step(s,false,false,7000));assert(pause_step(s,false,false,8000));}
 // Partner pauses first, the host opens and closes its menu -> stays paused;
 // the partner's own unpause resumes.
 {PauseState s;assert(!pause_step(s,false,false,1000));assert(!pause_step(s,true,false,2000)&&s.othersOpen);
  assert(!pause_step(s,true,true,3000));assert(!pause_step(s,true,false,4000));assert(pause_step(s,false,false,4020)&&s.othersOpen);
  assert(!pause_step(s,true,false,4060)&&!s.selfPending&&s.othersOpen);assert(!pause_step(s,false,false,9000)&&!s.othersOpen);assert(!pause_step(s,false,false,9500));}
 // A re-pause is issued at most once per second while a menu stays open.
 {PauseState s;s.lastHost=true;s.hostOpenAt=0;assert(pause_step(s,false,true,5000));assert(!pause_step(s,false,true,5500));assert(pause_step(s,false,true,6001));}
 // coop-fixes-009: "*paused*" on every screen, re-sent each second, cleared on resume.
 {PauseNotice n;assert(!pause_notice_step(n,false,100));assert(!std::strcmp(pause_notice_step(n,true,1000),"*paused*"));assert(!pause_notice_step(n,true,1500));
  assert(!std::strcmp(pause_notice_step(n,true,2000),"*paused*"));assert(!std::strcmp(pause_notice_step(n,false,2100),""));assert(!pause_notice_step(n,false,2200));}
 reset_players();pauseNotice={};pause_notice(true,1000);assert(std::count(lines.begin(),lines.end(),std::string("CLIENT:*paused*"))==2);
 pause_notice(false,1100);assert(std::count(lines.begin(),lines.end(),std::string("CLIENT:"))==2&&logged("PAUSE resumed"));

 // coop-fixes-013: the main menu (c_game_menu1) shows the credit and thanks
 // bottom-left once the menu client's commands arrive (CmdStart), then every
 // 5 s without a fade-in; again after coming back from a game; never on a story map.
 reset_players();menu_credit_reset();menu_credit_active(&entities[1]);menu_credit_frame();assert(messageBegins==0);
 store(globals,0x0c,3);store(globals,0,20.f);menu_credit_frame();assert(messageBegins==0);
 menu_credit_active(&entities[2]);menu_credit_frame();assert(messageBegins==0);   // only the menu's own client
 menu_credit_active(&entities[1]);store(globals,0,20.4f);menu_credit_frame();assert(messageBegins==0);
 store(globals,0,20.6f);menu_credit_frame();assert(messageBegins==1&&messageText=="Co-op mod v0.7.2 developed by Anas Sammak (koolkat)\nSpecial thanks: Mesilll, Michal Eminov, caixa, Joah230"&&logged("MENU credit shown bottom-left"));
 assert((message==std::vector<int>{1,23,29,4,163,7454,0,200,200,200,180,200,200,200,180,256,256,7680}));
 store(globals,0,24.f);menu_credit_frame();assert(messageBegins==1);store(globals,0,25.7f);menu_credit_frame();assert(messageBegins==2&&message[15]==0);
 // Back from a game: a new map, no credit until that client is running again.
 menu_credit_reset();store(globals,0,1.f);menu_credit_frame();assert(messageBegins==2);
 menu_credit_active(&entities[1]);store(globals,0,1.6f);menu_credit_frame();assert(messageBegins==3&&message[15]==256);
 store(globals,0x0c,1);menu_credit_reset();

 // Main menu: the two Custom Campaign entries start a two-player server.
 lines.clear();serverCommands.clear();clientCommands.clear();store(globals,0x0c,3);
 argv={"campaign","sc4coop_new"};assert(!menu_command("campaign")&&menuMode==1&&logged("MENU host new co-op game"));
 stuff_command(e,"map %s","sc4coop_new");
 assert(clientCommands.empty()&&serverCommands.size()==1&&serverCommands[0]=="disconnect;maxplayers 2;mp_footsteps 1;sv_maxupdaterate 100;sv_lan 0;map cof_campaign_story4\n"&&menuMode==0&&!checkpointPrepared);
 // Unrelated stuffed commands pass through unchanged (formatted once).
 stuff_command(e,"map %s","c_intro");stuff_command(e,"%s","echo 100%");assert(clientCommands.size()==2&&clientCommands[0]=="map c_intro"&&clientCommands[1]=="echo 100%");
 // Without the menu selection the same text is not intercepted.
 stuff_command(e,"map sc4coop_new");assert(clientCommands.size()==3);
 // Continue with no save: back to the menu, the player is told.
 serverCommands.clear();argv={"campaign","sc4coop_continue"};menu_command("campaign");assert(menuMode==2);stuff_command(e,"map sc4coop_continue");
 assert(serverCommands.size()==1&&serverCommands[0]=="disconnect;maxplayers 1;deathmatch 0;map c_game_menu1\n"&&logged("CLIENT:No co-op save")&&!checkpointPrepared);
 // Continue picks the newest valid slot whose map is installed.
 const auto saves=std::filesystem::path(gameDir)/"storycoop_saves",maps=std::filesystem::path(gameDir)/"maps";std::filesystem::create_directories(saves);std::filesystem::create_directories(maps);
 assert(commit_checkpoint(saves,1,save_fixture("s4_start2")));{std::ofstream f(maps/"s4_start2.bsp");f<<"bsp";}
 assert(commit_checkpoint(saves,3,save_fixture("s4_missing")));
 serverCommands.clear();menuMode=2;checkpointWorldStarted=true;stuff_command(e,"map sc4coop_continue\n");
 assert(serverCommands.size()==1&&serverCommands[0]=="disconnect;maxplayers 2;mp_footsteps 1;sv_maxupdaterate 100;sv_lan 0;map s4_start2\n");
 assert(checkpointPrepared&&checkpointSlot==1&&!checkpointWorldStarted&&loadedCheckpoint.map=="s4_start2"&&logged("PREPARE accepted from the main menu: slot1 map=s4_start2"));
 // A newer slot wins.
 assert(commit_checkpoint(saves,2,save_fixture("s4_start2")));std::filesystem::last_write_time(saves/"slot2.cofsave",std::filesystem::last_write_time(saves/"slot1.cofsave")+std::chrono::seconds(30));
 serverCommands.clear();menuMode=2;stuff_command(e,"map sc4coop_continue");assert(checkpointSlot==2&&serverCommands.back().find("map s4_start2\n")!=std::string::npos);
 // coop-fixes-012: "Load Slot N" loads exactly that slot (slot 2 is newer; slot 1 is asked for).
 serverCommands.clear();argv={"campaign","sc4coop_slot1"};menu_command("campaign");assert(menuMode==11&&logged("MENU load co-op slot 1 selected"));stuff_command(e,"map sc4coop_slot1");
 assert(checkpointSlot==1&&checkpointPrepared&&serverCommands.size()==1&&serverCommands[0]=="disconnect;maxplayers 2;mp_footsteps 1;sv_maxupdaterate 100;sv_lan 0;map s4_start2\n");
 // A slot without a loadable save (here its map is not installed) goes back to the menu and says so.
 serverCommands.clear();checkpointPrepared=false;argv={"campaign","sc4coop_slot3"};menu_command("campaign");stuff_command(e,"map sc4coop_slot3");
 assert(serverCommands.size()==1&&serverCommands[0]=="disconnect;maxplayers 1;deathmatch 0;map c_game_menu1\n"&&logged("CLIENT:Save slot 3 is empty.")&&!checkpointPrepared);
 // The stuffed map must be the selected entry; unknown ids select nothing.
 clientCommands.clear();argv={"campaign","sc4coop_slot2"};menu_command("campaign");stuff_command(e,"map sc4coop_slot1");assert(clientCommands.size()==1&&menuMode==12);
 argv={"campaign","sc4coop_slot5"};menu_command("campaign");assert(menuMode==15);argv={"campaign","sc4coop_slot6"};menu_command("campaign");assert(menuMode==0);argv={"campaign","sc4coop_slot22"};menu_command("campaign");assert(menuMode==0);
 // v0.5: a single-player story map typed in a co-op game becomes its co-op version;
 // the menu, maps without one and a real single-player game (1 slot) are left alone.
 engine[110]=reinterpret_cast<void*>(&map_valid);serverCommands.clear();
 for(const auto& [token,expect]:std::vector<std::pair<int,std::string>>{{8,"changelevel s4_start\n"},{9,"changelevel cof_campaign_story4\n"},{10,"changelevel s4_lake_e0\n"}}){store(globals,0x0c,token);coop_version_redirect();assert(serverCommands.back()==expect);}
 assert(logged("MAP c_start is the single-player map; co-op continues on s4_start"));
 for(int token:{3,11,1,4}){store(globals,0x0c,token);coop_version_redirect();}assert(serverCommands.size()==3);
 store(globals,0x0c,8);store(globals,0x90,1);coop_version_redirect();assert(serverCommands.size()==3);store(globals,0x90,2);store(globals,0x0c,1);
 // v0.5.1 Host: an entry of the Create Server window restarts the server on its chapter with the window's options.
 {reset_players();serverCommands.clear();lines.clear();collisionCvarValue=0.f;difficultyValue=3.f;store(globals,0x90,3);store(globals,0x0c,12);story_entry_redirect();
  assert(serverCommands.back()=="disconnect;maxplayers 3;mp_footsteps 1;sv_maxupdaterate 100;sv_lan 0;difficulty 3;map s4_basement_e0\n"&&sessionOptions.have&&sessionOptions.maxPlayers==3&&sessionOptions.collision==0);
  assert(logged("HOST Chapter-2-Who-is-that-Doctor: players=3 collision=off extras=locked difficulty=3"));
  // v0.7 Unlock extras: each line's locked token turns into its unlocked token in the engine's buffer (same length).
  {std::string f="x\n\nl2\nz3,4lj)4b!9g7vgi4x5a\nl4\nl5\nl6\nye&ole1^0(fcanb3v5t4\n";for(int i=8;i<=14;++i)f+="l"+std::to_string(i)+"\n";f+="v9agjak+p)(,o3qb)aq4\r\n";
   std::string g=f;assert(extras_unlock(g.data(),static_cast<int>(g.size()))==3&&g.size()==f.size()&&g.find("z3,4rj)4b!9g7vgi4x5a")!=std::string::npos&&g.find("ye&0le1^0(fcanb3v5t4")!=std::string::npos&&g.find("v9agjak+p)(,03qb)aq4\r\n")!=std::string::npos);
   std::string h="z3,4lj)4b!9g7vgi4x5a\n";assert(extras_unlock(h.data(),static_cast<int>(h.size()))==0);   // line 1 holds no unlock
   KeyValue lock{const_cast<char*>("func_button"),const_cast<char*>("lockedby"),const_cast<char*>("sc5_extras_later"),0};extrasUnlocked=false;assert(!extras_keyvalue_dropped(&lock));extrasUnlocked=true;assert(extras_keyvalue_dropped(&lock));extrasUnlocked=false;}
  store(globals,0x90,1);story_entry_redirect();assert(sessionOptions.maxPlayers==2);   // a 1-slot window still makes a co-op server
  // A save entry loads that slot with its own saved settings.
  store(globals,0x90,2);store(globals,0x0c,13);story_entry_redirect();assert(checkpointPrepared&&checkpointSlot==1&&serverCommands.back().find("map s4_start2\n")!=std::string::npos);
  store(globals,0x0c,1);serverCommands.clear();story_entry_redirect();assert(serverCommands.empty());   // any other map: nothing
  // Collision off: the others are not solid during one player's move, and back after.
  sessionOptions.have=true;sessionOptions.collision=0;store(ev(&entities[1]),0x10c,3);store(ev(&entities[2]),0x10c,3);collision_pre(&entities[1]);assert(load<int>(ev(&entities[2]),0x10c)==0&&load<int>(ev(&entities[1]),0x10c)==3);
  collision_restore();assert(load<int>(ev(&entities[2]),0x10c)==3);sessionOptions=SessionOptions{};checkpointPrepared=false;difficultyValue=0.f;collisionCvarValue=1.f;}
 // v0.5.1 Join: the menu's Join opens the computer box with our texts; Join connects, Cancel puts the game's texts back.
 {reset_players();lines.clear();clientCommands.clear();messageBegins=0;store(globals,0x0c,3);computerMessage=91;
  std::vector<unsigned char> img(0x150000,0);for(const auto& jp:join_box_patches())std::copy(jp.from.begin(),jp.from.end(),img.begin()+(jp.va-0x10000000));clientImage=img.data();clientImageSize=img.size();
  const auto shipped=img;auto j=&entities[1];argv={"sc_joinbox"};assert(join_box_command(j,"sc_joinbox")&&messageBegins==1&&message[1]==91&&(std::vector<int>(message.begin()+2,message.end())==std::vector<int>{0,0,0,1}));
  assert(std::string(reinterpret_cast<const char*>(img.data()+0x1473CC))=="gfx/vgui/sc_joinbox.tga"&&std::string(reinterpret_cast<const char*>(img.data()+0x1474F4))=="Port (empty: 27015)");
  argv={"computercheck","Type","the"};assert(join_box_command(j,"computercheck")&&messageBegins==2&&message[3]==1&&logged("JOIN refused"));   // the prompt itself is no address
  argv={"computercheck","26.0.0.5","70000"};assert(join_box_command(j,"computercheck")&&messageBegins==3);
  argv={"computercheck","26.0.0.5",""};const auto sentBefore=serverCommands.size();assert(join_box_command(j,"computercheck")&&serverCommands.size()==sentBefore&&!pendingJoin.target.empty());   // v0.7: the connect waits 0.6 s
  join_connect_frame();assert(serverCommands.size()==sentBefore);pendingJoin.due=0;join_connect_frame();assert(serverCommands.back()=="connect 26.0.0.5:27015\n"&&img==shipped&&!joinBox.who&&pendingJoin.target.empty());
  argv={"sc_joinbox"};join_box_command(j,"sc_joinbox");argv={"computercheck","host.example:27020","Port"};join_box_command(j,"computercheck");pendingJoin.due=0;join_connect_frame();assert(serverCommands.back()=="connect host.example:27020\n");
  // v0.6.1: the raw text (pfnCmd_Args) wins over the tokens the engine cut at ':' and '('.
  {static std::string joinRaw;engine[82]=reinterpret_cast<void*>(+[]()->const char*{return joinRaw.c_str();});
   argv={"sc_joinbox"};join_box_command(j,"sc_joinbox");joinRaw="26.0.0.5:27016 Port (empty: 27015)";argv={"computercheck","26.0.0.5",":"};join_box_command(j,"computercheck");pendingJoin.due=0;join_connect_frame();assert(serverCommands.back()=="connect 26.0.0.5:27016\n");
   argv={"sc_joinbox"};join_box_command(j,"sc_joinbox");joinRaw="10.0.0.2 Port (empty: 27015)";argv={"computercheck","10.0.0.2","Port"};join_box_command(j,"computercheck");pendingJoin.due=0;join_connect_frame();assert(serverCommands.back()=="connect 10.0.0.2:27015\n");
   engine[82]=nullptr;}
  argv={"sc_joinbox"};join_box_command(j,"sc_joinbox");assert(img!=shipped);argv={"stopcomputer"};assert(join_box_command(j,"stopcomputer")&&img==shipped&&logged("JOIN box closed"));
  // The computer puzzle in the story is never taken.
  store(globals,0x0c,1);argv={"computercheck","a","b"};assert(!join_box_command(j,"computercheck"));clientImage=nullptr;clientImageSize=0;computerMessage=0;}
 // v0.7 Player Model: Next goes through the models, Done closes the box, the game's strings come back; nothing can be typed.
 {reset_players();lines.clear();serverCommands.clear();messageBegins=0;store(globals,0x0c,3);computerMessage=91;
  std::vector<unsigned char> img(0x150000,0);
  for(const auto& list:{join_box_patches(),model_box_patches()})for(const auto& mp:list)std::copy(mp.from.begin(),mp.from.end(),img.begin()+(mp.va-0x10000000));
  {const auto path=client_text(ModelBoxPictureShipped,ModelBoxPictureBytes);std::copy(path.begin(),path.end(),img.begin()+(ModelBoxPicture-0x10000000));}
  clientImage=img.data();clientImageSize=img.size();const auto shipped=img;auto j=&entities[1];
  const auto player=std::filesystem::path(gameDir)/"models"/"player";
  for(const char* name:{"Caps","police1","zed","no file","simon_aom","simon_twitcher"}){std::filesystem::create_directories(player/name);if(std::strcmp(name,"no file"))std::ofstream(player/name/(std::string(name)+".mdl"))<<"x";}
  std::filesystem::create_directories(player/L"café");std::ofstream(player/L"café"/L"café.mdl")<<"x";   // not a-z: skipped, no exception
  auto picture=[&]{return std::string(reinterpret_cast<const char*>(img.data()+0x147380));};
  auto saved=[&]{std::ifstream f(std::filesystem::path(gameDir)/"storycoop_model.txt",std::ios::binary);std::string s;f>>s;return s;};
  assert((local_player_models()==std::vector<std::string>{"simon","simon_aom","simon_twitcher","caps","zed"}));   // Simon, the outfits in their order, then the player's own
  assert(model_outfit("simon_aom")==4&&model_outfit("simon_twitcher")==11&&!model_outfit("simon")&&!model_outfit("zed")&&model_label("simon_aom")=="Afraid of Monsters"&&model_label("simon")=="Simon"&&model_label("zed")=="zed");
  assert(model_box_command(j,"sc_modelbox")&&messageBegins==1&&message[1]==91&&(std::vector<int>(message.begin()+2,message.end())==std::vector<int>{1,0,0,1}));   // logged on: no text fields
  assert(picture()=="gfx/vgui/sc_mb0.tga"&&messageText=="\nSimon\n\nModel 1 of 5\n\nNext: next model\nDone: keep this one"&&logged("MODEL box open: 5 model(s), current simon"));
  assert(img[0x3EF6A]==0x90&&img[0x3EF6B]==0x90&&std::string(reinterpret_cast<const char*>(img.data()+0x14736C))=="%s\n");   // OK is Next, the text is ours
  assert(img[0x3F201]==0x8B&&img[0x3F202]==0xF1&&img[0x3F208]==0x6A&&img[0x3F209]==0xFF&&img[0x3F215]==0x90&&img[0x3F216]==0x2C&&img[0x3F21A]==0xE9);   // setBgColor(0,0,0,255): no grey box
  assert(model_box_command(j,"computercheck")&&picture()=="gfx/vgui/sc_mb4.tga"&&messageText.rfind("\nAfraid of Monsters\n\nModel 2 of 5",0)==0&&saved()=="simon_aom"&&serverCommands.back()=="setinfo sc_model \"simon_aom\"\n"&&messageBegins==2);
  assert(model_box_command(j,"computercheck")&&picture()=="gfx/vgui/sc_mb11.tga"&&saved()=="simon_twitcher");
  assert(model_box_command(j,"computercheck")&&picture()=="gfx/vgui/sc_mbx.tga"&&messageText.rfind("\ncaps\n\nModel 4 of 5",0)==0&&saved()=="caps");   // no preview for the player's own
  assert(model_box_command(j,"computercheck")&&model_box_command(j,"computercheck")&&picture()=="gfx/vgui/sc_mb0.tga"&&saved()=="simon");   // wraps
  assert(model_box_command(j,"computercheck")&&saved()=="simon_aom");
  assert(model_box_command(j,"stopcomputer")&&img==shipped&&!modelBox.who&&logged("MODEL box closed"));
  assert(!model_box_command(j,"stopcomputer")&&!model_box_command(j,"computercheck"));   // the game's own again
  assert(model_box_command(j,"sc_modelbox")&&picture()=="gfx/vgui/sc_mb4.tga");   // kept for the next time
  assert(!model_box_command(&entities[2],"computercheck")&&picture()=="gfx/vgui/sc_mb4.tga");   // only the player who opened it
  // The join box while the model box is open (no stopcomputer came): the game's strings are back first.
  argv={"sc_joinbox"};assert(!model_box_command(j,"sc_joinbox")&&!modelBox.who&&img==shipped&&join_box_command(j,"sc_joinbox")&&joinBox.who==j&&logged("JOIN box open"));
  assert(model_box_command(j,"sc_modelbox")&&!joinBox.who&&picture()=="gfx/vgui/sc_mb4.tga");model_box_close(nullptr);assert(img==shipped);
  // Never on a story map: the computer puzzle stays the game's.
  store(globals,0x0c,1);assert(!model_box_command(j,"sc_modelbox")&&!model_box_command(j,"computercheck"));
  clientImage=nullptr;clientImageSize=0;computerMessage=0;store(globals,0x0c,3);
  // The wearer's own clothes state goes to their client only when it is ready, and again later.
  {clothesMessage=77;engine[51]=engine[48];ModelState cs;messageBegins=0;
   allocations.at(j).commands=true;allocations.at(j).hudDue=0;store(globals,0,50.f);
   model_clothes(j,cs,"simon",50.f);assert(messageBegins==0);   // plain Simon: the game's own state stays
   model_clothes(j,cs,"simon_aom",50.f);assert(messageBegins==1&&message[1]==77&&(std::vector<int>(message.begin()+2,message.end())==std::vector<int>{0,1,4,0}));
   model_clothes(j,cs,"simon_aom",51.f);assert(messageBegins==1);model_clothes(j,cs,"simon_aom",55.f);assert(messageBegins==2);
   model_clothes(j,cs,"simon",60.f);assert(messageBegins==3&&(std::vector<int>(message.begin()+2,message.end())==std::vector<int>{0,0,0,0}));
   clothesMessage=0;engine[51]=nullptr;}
  for(const auto& item:{player/"Caps",player/"police1",player/"zed",player/"no file",player/"simon_aom",player/"simon_twitcher",player/L"café"})std::filesystem::remove_all(item);std::filesystem::remove(std::filesystem::path(gameDir)/"storycoop_model.txt");}
 // Campaign maps are recognized for the pause guard; the 3D menu is not.
 store(globals,0x0c,3);assert(!campaign_map());store(globals,0x0c,4);assert(campaign_map());store(globals,0x0c,1);assert(campaign_map());
 // v0.1 Steam names: Steam IDs convert to 64-bit IDs; names are cleaned; with
 // no steam_api.dll in this fixture the launcher's sc_name is shown, kept
 // unchanged while it already matches, and checked again after 2 s.
 unsigned long long id64=0;assert(steam_id64("STEAM_0:1:12345",id64)&&id64==76561197960290419ull&&steam_id64("STEAM_1:0:7",id64)&&id64==76561197960265742ull);
 assert(!steam_id64("STEAM_ID_LAN",id64)&&!steam_id64("STEAM_0:2:5",id64)&&!steam_id64("STEAM_0:1:",id64)&&!steam_id64("STEAM_0:1:12a",id64));
 assert(clean_name("  Uni \"x\";\\%\n ")=="Uni x"&&clean_name("Unknown").empty()&&clean_name("[unknown]").empty()&&clean_name(std::string(40,'a').c_str()).size()==31);
 reset_players();store(globals,0x0c,1);keySets=0;info[2]["name"]="Unknown";info[2]["sc_name"]="Friend \"P2\"";store(globals,0,30.f);
 name_frame(&entities[2]);assert(keySets==1&&info[2]["name"]=="Friend P2"&&logged("NAME player=2 shown as Friend P2 (from the launcher)"));
 name_frame(&entities[2]);store(globals,0,32.5f);name_frame(&entities[2]);assert(keySets==1);
 info[2]["name"]="Unknown";store(globals,0,35.f);name_frame(&entities[2]);assert(keySets==2&&info[2]["name"]=="Friend P2");
 info[1].erase("sc_name");info[1]["name"]="Unknown";name_frame(&entities[1]);assert(info[1]["name"]=="Unknown");
 store(globals,0x0c,3);info[2]["name"]="Unknown";store(globals,0,40.f);name_frame(&entities[2]);assert(info[2]["name"]=="Unknown");store(globals,0x0c,1);
 // v0.1 menu isolation: the reviewed client.dll changes are made in memory.
 // Each site holds the game's bytes -> changed; absolute addresses follow
 // the loaded base; a second pass changes nothing; foreign bytes refuse all.
 for(const std::uint32_t loaded:{0x10000000u,0x23450000u}){
  std::vector<unsigned char> img(0x150000,static_cast<unsigned char>(0xcc));
  for(const auto& patch:client_patches()){const auto from=client_value(patch.from,patch.absolute,loaded);std::copy(from.begin(),from.end(),img.begin()+(patch.va-0x10000000));}
  std::string why;assert(client_menu_apply(img.data(),img.size(),loaded,why)==1);
  for(const auto& patch:client_patches()){const auto to=client_value(patch.to,patch.absolute,loaded);assert(std::equal(to.begin(),to.end(),img.begin()+(patch.va-0x10000000)));}
  const auto once=img;assert(client_menu_apply(img.data(),img.size(),loaded,why)==2&&img==once);
  assert(std::string(reinterpret_cast<const char*>(img.data()+0x14389C))=="gfx/vgui/sc_logo.tga"&&std::string(reinterpret_cast<const char*>(img.data()+0x143888),8)=="storyc*m");
  const std::uint32_t jump=img[0x2DF68]|img[0x2DF69]<<8|img[0x2DF6A]<<16|img[0x2DF6B]<<24;assert(jump==0x1002DDC1-0x10000000+loaded);
  // An older install's patched file (everything but the new title path) is completed.
  auto older=once;const auto logo=client_text("gfx/vgui/menu_logo.tga",23);std::copy(logo.begin(),logo.end(),older.begin()+0x14389C);
  assert(client_menu_apply(older.data(),older.size(),loaded,why)==1&&older==once);
  auto foreign=once;foreign[0x2DE46+1]=0x07;const auto before=foreign;assert(client_menu_apply(foreign.data(),foreign.size(),loaded,why)==0&&foreign==before&&why.find("1002DE46")!=std::string::npos);
 }
 assert(client_patches().size()==68);
 // Every adapter line is also kept in a per-session log beside the saves.
 {bool found=false;std::error_code error;for(const auto& item:std::filesystem::directory_iterator(std::filesystem::path(gameDir)/"storycoop_logs",error)){const auto n=item.path().filename().string();if(n.rfind("rc4-",0)==0){std::ifstream in(item.path());std::string all((std::istreambuf_iterator<char>(in)),{});found=all.find("MENU host new co-op game")!=std::string::npos;}}assert(found);}
 // v0.3 crash report: code, module+offset, map, the last steps and stack code addresses in one line, then a minidump.
 {reset_players();crumbAt=0;for(auto& c:crumbs)c={};store(globals,0x0c,1);crumb("think",&entities[1]);crumb("mapstate restore",&entities[2]);
  EXCEPTION_RECORD r{};r.ExceptionCode=EXCEPTION_ACCESS_VIOLATION;r.ExceptionAddress=reinterpret_cast<void*>(&reset_players);r.NumberParameters=2;r.ExceptionInformation[1]=0x24;
  volatile std::uintptr_t stack[4]={0,reinterpret_cast<std::uintptr_t>(&reset_players),1,2};CONTEXT c{};c.Esp=static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(&stack[0]));
  EXCEPTION_POINTERS x{&r,&c};char line[1400]{};crash_line(&x,line,sizeof(line));const std::string s=line;
  assert(s.find("CRASH code=C0000005 at session-tests.exe+0x")!=std::string::npos&&s.find("(read 0x00000024)")!=std::string::npos&&s.find("map=s4_start")!=std::string::npos);
  assert(s.find("last=[think 1 player; mapstate restore 2 player; ]")!=std::string::npos&&s.find("stack=[session-tests.exe+0x")!=std::string::npos);
  lines.clear();crashWritten=false;crashPrevious=nullptr;assert(crash_filter(&x)==EXCEPTION_CONTINUE_SEARCH&&crashWritten&&logged("CRASH code=C0000005"));
  lines.clear();assert(crash_filter(&x)==EXCEPTION_CONTINUE_SEARCH&&lines.empty());   // one report per process
  // v0.4: the first-chance (vectored) report; C++ throws and breakpoints are ignored.
  crashWritten=false;EXCEPTION_RECORD cpp{};cpp.ExceptionCode=0xE06D7363;EXCEPTION_POINTERS y{&cpp,&c};assert(crash_vectored(&y)==EXCEPTION_CONTINUE_SEARCH&&!crashWritten&&lines.empty());
  assert(crash_vectored(&x)==EXCEPTION_CONTINUE_SEARCH&&crashWritten&&logged("CRASH code=C0000005")&&logged("first chance"));
  crash_install();crash_install();assert(crashVectored);crash_uninstall();assert(!crashVectored);{auto f=SetUnhandledExceptionFilter(nullptr);assert(f!=&crash_filter);SetUnhandledExceptionFilter(f);}   // v0.4: gone before the DLL unloads
  bool dump=false;std::error_code error;for(const auto& item:std::filesystem::directory_iterator(std::filesystem::path(gameDir)/"storycoop_logs",error))if(item.path().extension()==".dmp"&&item.file_size()>0)dump=true;assert(dump);}
 std::puts("PASS session runtime: per-session adapter log, police flashlight (F) only light policy and frags resync, cutscene ScoreInfo resync, Simon model through the world player-model fields (one userinfo set per map), pause attribution (host/partner/self, rate limit), main-menu new game, newest-slot (older entry) and Load Slot 1..3 host commands, empty slot back to the menu, v0.1 credit, Steam/launcher display names, in-memory co-op menu (base-relative, idempotent, older install completed, foreign bytes refused)");
}
