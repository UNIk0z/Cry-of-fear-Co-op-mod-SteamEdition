// Owned SDK engine fixture; no original game code is loaded by this test.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <map>
namespace rc4 {struct Edict{std::array<unsigned char,0x400> bytes{};};}
using namespace rc4;
namespace {
std::array<Edict,64> edicts{};std::array<unsigned char,0xa0> g{};
std::unordered_map<Edict*,std::vector<unsigned char>> memory;
std::vector<std::string> strings{"","player","s4_start_e0","tape_recorder"};
std::unordered_map<Edict*,std::map<std::string,std::string>> helpers;int itemGrants=0;bool switchStuck=false;std::string lastItem;
std::array<std::string,3> args{};int nextEntity=3,grants=0,moves=0,menuMessages=0;
GlobalSnapshot state;std::string gameDir;std::vector<std::string> logged;bool badLanding=false;
bool logged_line(std::initializer_list<const char*> parts){for(const auto& line:logged)if(std::all_of(parts.begin(),parts.end(),[&](const char* s){return line.find(s)!=std::string::npos;}))return true;return false;}
int idx(Edict* e){return static_cast<int>(e-edicts.data());}
Edict* at(int i){return i>=0&&i<64?&edicts[i]:nullptr;}
void* ev(Edict* e){return e->bytes.data()+0x80;}
void* priv(Edict* e){auto i=memory.find(e);return i==memory.end()?nullptr:i->second.data();}
void* alloc_(Edict* e,int n){memory[e]=std::vector<unsigned char>(n);return priv(e);}
void free_(Edict* e){memory.erase(e);}
const char* str_(int i){return i>=0&&i<static_cast<int>(strings.size())?strings[i].c_str():"";}
int intern(const char* s){for(std::size_t i=0;i<strings.size();++i)if(strings[i]==s)return static_cast<int>(i);strings.emplace_back(s);return static_cast<int>(strings.size()-1);}
void print_(const char* s){if(s)logged.emplace_back(s);}Edict* printedTo=nullptr;int printedType=-1;std::string printedText;std::vector<Edict*> savedTold;void client_print(Edict* e,int type,const char* text){printedTo=e;printedType=type;printedText=text?text:"";if(printedText.rfind("Game saved",0)==0)savedTold.push_back(e);}
Edict* soundFrom=nullptr;std::string soundSample;int soundCount=0;void save_sound(Edict* e,int,const char* sample,float,float,int,int){soundFrom=e;soundSample=sample?sample:"";++soundCount;}
std::vector<std::pair<Edict*,std::string>> played;void __cdecl stuff_(Edict* e,const char* format,...){char text[256]{};va_list a;va_start(a,format);std::vsnprintf(text,sizeof(text),format,a);va_end(a);played.emplace_back(e,text);}
const char* arg(int i){return i>=0&&i<3?args[i].c_str():"";}
int uid(Edict* e){return idx(e)+10;}
const char* auth(Edict*){return "STEAM_ID_LAN";}
int listen(){return 0;}
std::string coopMusic;void cvar_set(const char* n,const char* value){if(n&&!std::strcmp(n,"sv_coop_music"))coopMusic=value?value:"";}
void dir_(char* p){std::strcpy(p,gameDir.c_str());}
void begin_(int,int,const float*,Edict*){}void end_(){++menuMessages;}void byte_(int){}void string_(const char*){}
Edict* named(int id){assert(nextEntity<63);auto e=&edicts[nextEntity++];store(e,0,0);store(e,4,nextEntity);store(ev(e),0,id);allocate(e,0x100);return e;}
void remove_(Edict* e){release(e);store(e,0,1);}
int spawn_(Edict*){return 0;}
void key_(Edict* e,KeyValue* kv){helpers[e][kv->key]=kv->value;kv->handled=1;}
void give_weapon(Edict* actor,const std::string& name,int clip){auto w=named(intern(name.c_str()));allocate(w,0x240);auto p=priv(actor),data=priv(w);store(data,0x1e4,clip);store(data,0x1dc,name=="weapon_syringe"?-1:1);store(data,0x164,load<std::uint32_t>(p,0x2514));store(p,0x2514,reinterpret_cast<std::uint32_t>(data));store(p,0x252c,reinterpret_cast<std::uint32_t>(data));for(int i=0;i<6;++i)if(!load<int>(p,0x2200+i*4)){store(p,0x2200+i*4,intern(name.c_str()));break;}}
void use_(Edict* h,Edict* actor){const auto type=classname(h);const auto value=helpers[h].count("message")?helpers[h]["message"]:helpers[h]["target"];auto p=priv(actor);
 if(!std::strcmp(type,"cof_giveitems")&&helpers[h]["spawnflags"]=="1"){++itemGrants;lastItem=value;for(int i=0;i<6;++i)if(!load<int>(p,0x2200+i*4)){store(p,0x2200+i*4,intern(value.c_str()));break;}}
 else if(!std::strcmp(type,"cof_giveitems")){assert(value.rfind("inventoryitems/",0)!=0);++grants;give_weapon(actor,value,0);}
 else if(!std::strcmp(type,"cof_weaponswitch")){if(switchStuck)return;std::vector<std::pair<Edict*,void*>> list;assert(weapon_list(actor,list));for(const auto& item:list)if(value==classname(item.first))store(p,0x252c,reinterpret_cast<std::uint32_t>(item.second));}
 else if(!std::strcmp(type,"cof_teleport")){assert(value=="sc4_checkpoint_421_p"+std::to_string(idx(actor)));++moves;store(ev(actor),8,64.0f);store(ev(actor),12,0.0f);store(ev(actor),16,badLanding?500.0f:36.0f);}
 else assert(false);
}
Edict* find_(Edict* start,const char* key,const char*){return start?nullptr:std::strcmp(key,"targetname")==0?&edicts[61]:&edicts[62];}
template<class T>void field(int token,T value){std::uint16_t n=sizeof(value),t=static_cast<std::uint16_t>(token);auto add=[](const void* p,std::size_t len){auto a=static_cast<const unsigned char*>(p);state.bytes.insert(state.bytes.end(),a,a+len);};add(&n,2);add(&t,2);add(&value,sizeof(value));}
void fixture_globals(){state={};state.tokens.resize(1024);state.tokens[1]="GLOBAL";state.tokens[2]="m_listCount";state.tokens[3]="GENT";state.tokens[4]="name";state.tokens[5]="levelName";state.tokens[6]="state";field(1,1);field(2,1);field(3,3);std::array<char,64> n{};std::strcpy(n.data(),"sc4.event.c_start.wake.done");field(4,n);std::array<char,32> map{};std::strcpy(map.data(),"s4_start_e0");field(5,map);field(6,1);}
void save_(SaveData* d){assert(d->capacity>=static_cast<int>(state.bytes.size()));std::memcpy(d->base,state.bytes.data(),state.bytes.size());d->size=static_cast<int>(state.bytes.size());d->current=d->base+d->size;for(std::size_t i=0;i<1024;++i)if(!state.tokens[i].empty())d->tokens[i]=state.tokens[i].data();}
void restore_(SaveData* d){assert(d->capacity>0);state.bytes.assign(reinterpret_cast<unsigned char*>(d->base),reinterpret_cast<unsigned char*>(d->base)+d->capacity);state.tokens.clear();for(int i=0;i<1024;++i)state.tokens.emplace_back(d->tokens[i]?d->tokens[i]:"");}
void empty(){}
int packs=0,packAccept=1;int pack_(void* s,int,Edict*,Edict*,int,int,unsigned char*){++packs;if(s)store(s,0x3c,0x8u);return packAccept;}
void player(int index,float hp){auto e=&edicts[index];store(e,0,0);store(e,4,index+100);store(ev(e),0,1);store(ev(e),0x164,hp);store(ev(e),0x174,0);allocate(e,PlayerBytes);allocations.at(e).commands=true;store(priv(e),0x21f0,75.0f);store(priv(e),0x235c,1);store(priv(e),0x2360,2);store(priv(e),0x2364,3);store(priv(e),0x2368,4);}
}
// v0.3 map state: entities that write nothing (every one FCAP_DONT_SAVE).
void entity_save_(Edict*,SaveData*){}
int main(int argc,char** argv){
 assert(argc==2);gameDir=argv[1];globals=g.data();verified=true;store(g.data(),0x90,2);store(g.data(),0x94,64);store(g.data(),0x0c,2);
 engine[14]=reinterpret_cast<void*>(&find_);engine[22]=reinterpret_cast<void*>(&remove_);engine[23]=reinterpret_cast<void*>(&named);engine[63]=reinterpret_cast<void*>(&alloc_);engine[64]=reinterpret_cast<void*>(&priv);engine[65]=reinterpret_cast<void*>(&free_);engine[66]=reinterpret_cast<void*>(&str_);engine[67]=reinterpret_cast<void*>(&intern);engine[68]=reinterpret_cast<void*>(&ev);engine[71]=reinterpret_cast<void*>(&idx);engine[72]=reinterpret_cast<void*>(&at);engine[80]=reinterpret_cast<void*>(&client_print);engine[29]=reinterpret_cast<void*>(&save_sound);engine[41]=reinterpret_cast<void*>(&stuff_);engine[81]=reinterpret_cast<void*>(&print_);engine[83]=reinterpret_cast<void*>(&arg);engine[99]=reinterpret_cast<void*>(&dir_);engine[113]=reinterpret_cast<void*>(&uid);engine[115]=reinterpret_cast<void*>(&listen);engine[143]=reinterpret_cast<void*>(&auth);engine[60]=reinterpret_cast<void*>(&cvar_set);
 engine[46]=reinterpret_cast<void*>(&begin_);engine[47]=reinterpret_cast<void*>(&end_);engine[48]=reinterpret_cast<void*>(&byte_);engine[54]=reinterpret_cast<void*>(&string_);
 original[1]=reinterpret_cast<void*>(&spawn_);original[3]=reinterpret_cast<void*>(&use_);original[6]=reinterpret_cast<void*>(&key_);original[12]=reinterpret_cast<void*>(&save_);original[7]=reinterpret_cast<void*>(&entity_save_);original[13]=reinterpret_cast<void*>(&restore_);original[25]=reinterpret_cast<void*>(&empty);original[39]=reinterpret_cast<void*>(&pack_);
 for(auto& e:edicts)store(&e,0,1);player(1,83);player(2,41);store(&edicts[62],0,0);store(&edicts[62],4,999);store(ev(&edicts[62]),0,3);store(&edicts[61],0,0);store(&edicts[61],4,998);store(ev(&edicts[61]),0,intern("info_teleport_destination"));store(ev(&edicts[61]),8,64.0f);for(int i:{1,2,62}){store(ev(&edicts[i]),8,1778.6f);store(ev(&edicts[i]),12,-3259.05f);store(ev(&edicts[i]),16,-775.0f);}
 configure_puzzle({&scoped,&player_memory,&serial,&classname,&resolve,&solved});puzzle_on_frame(&edicts[1],priv(&edicts[1]),PlayerBytes);fixture_globals();give_weapon(&edicts[1],"weapon_glock",7);give_weapon(&edicts[2],"weapon_glock",3);store(priv(&edicts[1]),0x2544,28);store(priv(&edicts[2]),0x2544,9);
 // LAN/duplicate Steam IDs: the two-player roles resolve automatically.
 SavedPlayer p;assert(capture_player(&edicts[1],p)&&p.identity=="auto:host"&&player_identity(&edicts[2])=="auto:guest");
 // A second unidentified guest is ambiguous: nobody gets a guessed guest record.
 store(g.data(),0x90,60);player(60,50);assert(player_identity(&edicts[2]).empty()&&player_identity(&edicts[60]).empty()&&player_identity(&edicts[1])=="auto:host");
 allocations.erase(&edicts[60]);store(&edicts[60],0,1);store(g.data(),0x90,2);
 args={"sc_checkpoint_bind","11","host"};assert(checkpoint_command(&edicts[1],args[0].c_str()));args={"sc_checkpoint_bind","12","guest"};assert(checkpoint_command(&edicts[1],args[0].c_str()));
 assert(capture_player(&edicts[1],p)&&p.identity=="profile:host"&&p.weapons[0].clip==7&&p.ammo[1]==28);
 // Native action is an internal active placeholder, not a missing real weapon.
 const auto active=load<std::uint32_t>(priv(&edicts[1]),0x252c),head=load<std::uint32_t>(priv(&edicts[1]),0x2514);
 auto action=named(intern("weapon_action"));allocate(action,0x240);store(priv(action),0x164,head);store(priv(&edicts[1]),0x2514,reinterpret_cast<std::uint32_t>(priv(action)));store(priv(&edicts[1]),0x252c,reinterpret_cast<std::uint32_t>(priv(action)));
 assert(capture_player(&edicts[1],p)&&p.equipped.empty()&&p.weapons.size()==1);
 store(priv(&edicts[1]),0x2514,head);store(priv(&edicts[1]),0x252c,active);remove_(action);
 assert(near_recorder(&edicts[1],"Sewer Store")==0);
 saveLoadMessage=8;screenFadeMessage=9;const auto noFile=checkpoint_directory()/"slot1.cofsave";
 // v0.1 (user request): only the host saves. The partner's recorder Use saves
 // nothing and shows "Only host can save the game"; the fade is still dropped.
 message_begin(1,8,nullptr,&edicts[2]);write_byte(0);write_string("Storage Tunnel");message_end();message_begin(1,9,nullptr,&edicts[2]);write_short(512);write_short(128);write_short(5);write_byte(0);write_byte(0);write_byte(0);write_byte(255);message_end();checkpoint_start_frame();
 assert(!std::filesystem::exists(noFile)&&menuMessages==0&&printedTo==&edicts[2]&&printedText=="Only host can save the game"&&!saveChoice.actor&&logged_line({"SAVE refused for player 2: only the host can save"}));
 checkpoint_save(&edicts[2]);assert(!std::filesystem::exists(noFile)&&logged_line({"only the host can save the game"}));
 // v0.2 (user): standing at or looking at a recorder shows nothing; a whole
 // player frame next to the recorder prints no restriction to anyone.
 printedTo=nullptr;checkpoint_start_frame();checkpoint_start_frame();assert(printedTo==nullptr);
 // The host's recorder Use shows the three slots on the host's PC; nothing is saved or heard yet.
 message_begin(1,8,nullptr,&edicts[1]);write_byte(0);write_string("Storage Tunnel");message_end();checkpoint_start_frame();
 assert(printedTo==&edicts[1]&&printedType==1&&printedText=="SAVE GAME (host's PC)\n1: empty\n2: empty\n3: empty\nPress 1, 2 or 3\n"&&soundCount==0&&played.empty()&&saveChoice.actor==&edicts[1]&&!std::filesystem::exists(noFile));
 // Only the host's 1, 2 or 3 is taken; anything else passes or is ignored.
 args={"quicksel","2",""};assert(!save_choice_command(&edicts[2],"quicksel")&&!save_choice_command(&edicts[1],"weapontoggle"));
 args={"quicksel","4",""};assert(save_choice_command(&edicts[1],"quicksel")&&saveChoice.actor==&edicts[1]);
 args={"quicksel","1",""};savedTold.clear();assert(save_choice_command(&edicts[1],"quicksel")&&!saveChoice.actor);checkpoint_start_frame();assert(std::filesystem::exists(noFile));
 // Written on the host's PC with the host as its owner; both players are told
 // where it went and each hears the recorder's save sound once, on their own client.
 assert(soundCount==0&&(savedTold==std::vector<Edict*>{&edicts[1],&edicts[2]})&&printedText=="Game saved in slot 1\non the host's PC.\n");
 assert((played==std::vector<std::pair<Edict*,std::string>>{{&edicts[1],"play save/save_sequence.wav\n"},{&edicts[2],"play save/save_sequence.wav\n"}}));played.clear();
 Checkpoint saved;bool backup=false;assert(read_checkpoint(checkpoint_directory(),1,saved,backup)&&saved.players.size()==2&&saved.owner=="profile:host");std::reverse(saved.players.begin(),saved.players.end());assert(commit_checkpoint(checkpoint_directory(),1,saved));
 assert(saved.recorder==419);
 for(int i:{1,2,62}){store(ev(&edicts[i]),8,1735.85f);store(ev(&edicts[i]),12,-3429.36f);}
 // The list shows the used slot's date. Using the recorder again shows the
 // list again: every save needs a slot choice.
 message_begin(1,8,nullptr,&edicts[1]);write_byte(0);write_string("Sewer Store");message_end();checkpoint_start_frame();
 assert(saveChoice.actor==&edicts[1]&&printedTo==&edicts[1]&&printedText.find("1: empty")==std::string::npos&&printedText.find("2: empty\n3: empty")!=std::string::npos);
 message_begin(1,8,nullptr,&edicts[1]);write_byte(0);write_string("Sewer Store");message_end();checkpoint_start_frame();
 assert(read_checkpoint(checkpoint_directory(),1,saved,backup)&&saved.recorder==419&&saveChoice.actor==&edicts[1]&&played.empty());
 args={"quicksel","1",""};assert(save_choice_command(&edicts[1],"quicksel"));checkpoint_start_frame();
 assert(read_checkpoint(checkpoint_directory(),1,saved,backup)&&saved.recorder==421&&!saveChoice.actor&&played.size()==2);played.clear();
 // The list closes after 15 s, or when the player walks away; nothing is saved.
 message_begin(1,8,nullptr,&edicts[1]);write_byte(0);write_string("Sewer Store");message_end();checkpoint_start_frame();assert(saveChoice.actor);
 store(g.data(),0,16.f);checkpoint_start_frame();assert(!saveChoice.actor&&printedText.empty()&&logged_line({"SAVE slot list closed: no slot chosen"}));store(g.data(),0,0.f);
 message_begin(1,8,nullptr,&edicts[1]);write_byte(0);write_string("Sewer Store");message_end();checkpoint_start_frame();assert(saveChoice.actor);
 store(ev(&edicts[1]),8,1935.85f);checkpoint_start_frame();assert(!saveChoice.actor&&logged_line({"walked away"}));store(ev(&edicts[1]),8,1735.85f);
 checkpointSlot=3;std::memset(static_cast<unsigned char*>(priv(&edicts[1]))+JournalOffset,'X',0x120);
 checkpoint_save(&edicts[1]);assert(!std::filesystem::exists(checkpoint_directory()/"slot3.cofsave"));
 std::memset(static_cast<unsigned char*>(priv(&edicts[1]))+JournalOffset,0,0x120);checkpointSlot=1;
 // Refusals name the player and failed step; no identity/key is printed.
 assert(logged_line({"CHECKPOINT save player=1 health=83","weapons=1","equipped=weapon_glock"})&&logged_line({"CHECKPOINT save player=2 health=41"}));
 checkpointSlot=2;allocations.at(&edicts[2]).commands=false;logged.clear();checkpoint_save(&edicts[1]);allocations.at(&edicts[2]).commands=true;
 assert(logged_line({"Save refused: player 2","client HUD not ready"})&&!std::filesystem::exists(checkpoint_directory()/"slot2.cofsave"));
 for(const auto& line:logged)assert(line.find("profile:")==std::string::npos&&line.find("host")==std::string::npos);checkpointSlot=1;
 // Fresh owned engine: no old private pointers/weapon objects survive.
 allocations.clear();memory.clear();playerBank.clear();profileBindings.clear();player(1,100);player(2,100);args={"sc_checkpoint_load","1",""};store(g.data(),0x0c,intern("c_game_menu1"));checkpoint_world_ready();assert(!checkpointWorldStarted);checkpoint_prepare_command();assert(checkpointPrepared&&!loadingCheckpoint);store(g.data(),0,100.0f);store(g.data(),0x0c,2);checkpoint_world_ready();assert(loadingCheckpoint&&playerBank.size()==2);
 // v0.3 audit: before anyone is restored, the next level change carries the save's records.
 for(const auto& s:playerBank)assert(carryBank.count(s.identity)&&carryBank.at(s.identity).health==s.health);
 {int banked=0;for(int i:{1,2})banked+=carry_departing(&edicts[i])?1:0;assert(banked==0&&carryBank.size()==2);}
 // No console binding after the restart: saved profile keys are unknown now,
 // so the host gets the owner record and the single guest the other record.
 const auto bankSize=playerBank.size();checkpoint_capture_departing(&edicts[2]);assert(playerBank.size()==bankSize&&playerBank[1].health!=100);
 allocations.at(&edicts[1]).commands=false;checkpoint_player_frame(&edicts[1]);assert(grants==0);allocations.at(&edicts[1]).commands=true;
 store(ev(&edicts[2]),0x1a8,0x80);logged.clear();checkpoint_player_frame(&edicts[2]);assert(grants==0&&logged_line({"RESTORE WAIT","player controls frozen"}));store(ev(&edicts[2]),0x1a8,0);
 // A role-matched guest awaiting restore still blocks a save that would bank fresh state.
 requestedRecorder=421;logged.clear();checkpoint_save(&edicts[1]);assert(logged_line({"awaits checkpoint restoration"}));
 logged.clear();checkpoint_player_frame(&edicts[2]);checkpoint_player_frame(&edicts[1]);assert(grants==2&&moves==2);
 assert(logged_line({"guest record matched by role"})&&logged_line({"host record matched by role"}));
 assert(capture_player(&edicts[1],p)&&p.health==83&&p.weapons[0].clip==7&&p.ammo[1]==28);assert(capture_player(&edicts[2],p)&&p.health==41&&p.weapons[0].clip==3&&p.ammo[1]==9);
 checkpoint_player_frame(&edicts[1]);checkpoint_player_frame(&edicts[2]);assert(grants==2&&moves==2);
 assert(logged_line({"CHECKPOINT restored player=1 health=83","equipped=weapon_glock"})&&logged_line({"CHECKPOINT restored player=2 health=41"}));
 // Each restore failure reports its exact step for the next real-client log.
 const auto host=std::find_if(playerBank.begin(),playerBank.end(),[](const SavedPlayer& s){return s.identity=="profile:host";});assert(host!=playerBank.end());
 assert(!restore_player(&edicts[3],*host)&&checkpointFailure.find("slot")!=std::string::npos);
 badLanding=true;assert(!restore_player(&edicts[1],*host)&&checkpointFailure.find("landing-readback")!=std::string::npos&&checkpointFailure.find("dz=500.0")!=std::string::npos);badLanding=false;
 // Continue request file: honored only at the exact saved map, then removed.
 const auto request=checkpoint_directory()/"continue.request";
 auto fresh=[&](){allocations.clear();memory.clear();playerBank.clear();profileBindings.clear();restoredPlayers.clear();loadingCheckpoint=false;hostConfirmed=false;checkpointRestoreFailed=false;checkpointWorldStarted=false;checkpointPrepared=false;player(1,100);player(2,100);give_weapon(&edicts[1],"weapon_glock",0);give_weapon(&edicts[2],"weapon_glock",0);};
 auto put=[&](const char* text){std::ofstream(request,std::ios::binary)<<text;};
 fresh();put("COFCONT1 1 s4_start");store(g.data(),0x0c,2);logged.clear();checkpoint_world_ready();assert(!loadingCheckpoint&&!std::filesystem::exists(request)&&logged_line({"CONTINUE request for another map"}));
 fresh();put("COFCONT1 9 s4_start_e0");checkpoint_world_ready();assert(!loadingCheckpoint&&!std::filesystem::exists(request)&&logged_line({"CONTINUE request malformed"}));
 fresh();put("COFCONT1 1 s4_start_e0");store(g.data(),0x0c,intern("c_game_menu1"));checkpoint_world_ready();assert(std::filesystem::exists(request)&&!checkpointWorldStarted);
 store(g.data(),0x0c,2);logged.clear();checkpoint_world_ready();assert(loadingCheckpoint&&!std::filesystem::exists(request)&&logged_line({"PREPARE accepted from Continue request file"}));
 // Exact profile identities take precedence over roles.
 args={"sc_checkpoint_bind","11","host"};checkpoint_command(&edicts[1],args[0].c_str());args={"sc_checkpoint_bind","12","guest"};checkpoint_command(&edicts[1],args[0].c_str());
 const int movesBefore=moves;logged.clear();checkpoint_player_frame(&edicts[1]);checkpoint_player_frame(&edicts[2]);
 assert(moves==movesBefore+2&&!logged_line({"matched by role"})&&capture_player(&edicts[1],p)&&p.health==83&&capture_player(&edicts[2],p)&&p.health==41);
 // Card paths (user log): weapon cards are never granted as items, other
 // cards use the native item branch, and a pending weapon switch is soft.
 {auto cards=*std::find_if(playerBank.begin(),playerBank.end(),[](const SavedPlayer& s){return s.identity=="profile:host";});
  cards.inventory={"weapon_glock","inventoryitems/weapons/weapon_glock.txt","inventoryitems/apartmentkey.txt","","",""};
  const int grantsBefore=grants,itemsBefore=itemGrants,movesBefore2=moves;store(priv(&edicts[1]),0x252c,0u);switchStuck=true;logged.clear();
  assert(restore_player(&edicts[1],cards)&&grants==grantsBefore&&itemGrants==itemsBefore+1&&lastItem=="inventoryitems/apartmentkey.txt"&&moves==movesBefore2+1);
  assert(logged_line({"RESTORE soft differences player=1","equipped"}));switchStuck=false;}
 // coop-fixes-008: quick-select keys are saved and point at the slot that
 // holds the same card after a restore.
 {void* hp=priv(&edicts[1]);store(hp,0x3dc,1);std::memcpy(static_cast<char*>(hp)+0x204c,"weapon_glock",13);store(hp,0x3e0,7);std::memcpy(static_cast<char*>(hp)+0x20cc,"weapon_glock",13);
  SavedPlayer q;assert(capture_player(&edicts[1],q)&&q.quick[0].slot==1&&q.quick[0].weapon=="weapon_glock"&&q.quick[1].slot==0&&q.quick[1].weapon.empty()&&q.quick[2].weapon.empty());
  std::swap(q.inventory[0],q.inventory[2]);q.quick[0]={3,"weapon_glock"};q.quick[2]={3,"weapon_glock"};
  store(hp,0x3dc,0);std::memset(static_cast<char*>(hp)+0x204c,0,0x180);
  assert(restore_player(&edicts[1],q)&&load<int>(hp,0x3dc)==1&&load<int>(hp,0x3e0)==0&&load<int>(hp,0x3e4)==1&&std::strcmp(static_cast<char*>(hp)+0x204c,"weapon_glock")==0&&std::strcmp(static_cast<char*>(hp)+0x214c,"weapon_glock")==0&&!*(static_cast<char*>(hp)+0x20cc));}
 // coop-fixes-008: game over (every player dead) on the map of this session's
 // save restarts from that save; on another map the game's own restart stays.
 {fresh();store(g.data(),0x0c,2);sessionSave={1,"s4_start_e0"};store(ev(&edicts[1]),0x174,2);store(ev(&edicts[2]),0x174,2);logged.clear();checkpoint_game_over_check();
  assert(gameOverMap=="s4_start_e0"&&logged_line({"GAME OVER on s4_start_e0"}));
  checkpointWorldStarted=false;loadingCheckpoint=false;checkpoint_world_ready();assert(loadingCheckpoint&&gameOverMap.empty()&&logged_line({"GAME OVER restart of s4_start_e0: loading the last save (slot1)"}));
  fresh();store(g.data(),0x0c,2);store(ev(&edicts[2]),0x174,2);checkpoint_game_over_check();assert(gameOverMap.empty());   // one player alive: no game over
  store(ev(&edicts[1]),0x174,2);checkpoint_game_over_check();sessionSave={1,"s4_start"};logged.clear();checkpoint_world_ready();assert(!loadingCheckpoint&&logged_line({"no save of this map"}));
  store(ev(&edicts[1]),0x174,0);store(ev(&edicts[2]),0x174,0);}
 // coop-fixes-013: every player's inventory is carried across a map change.
 // First visit banks it; on the next map each player gets missing weapons,
 // clips, reserve, health and the phone screen back, keeping what the new
 // map already gave; the first recorder hint is skipped; a game over goes
 // back to what everyone had on arrival.
 {fresh();store(g.data(),0x0c,2);store(g.data(),0,300.f);carry_reset();
  auto clip=[](Edict* e){std::vector<std::pair<Edict*,void*>> list;assert(weapon_list(e,list));for(const auto& w:list)if(!std::strcmp(classname(w.first),"weapon_glock"))return load<int>(w.second,0x1e4);return -1;};
  auto setClip=[](Edict* e,int n){std::vector<std::pair<Edict*,void*>> list;assert(weapon_list(e,list));for(const auto& w:list)if(!std::strcmp(classname(w.first),"weapon_glock"))store(w.second,0x1e4,n);};
  setClip(&edicts[1],7);store(priv(&edicts[1]),0x2544,28);store(priv(&edicts[1]),0x5cc,3);store(ev(&edicts[1]),0x164,83.f);setClip(&edicts[2],3);
  carry_frame(&edicts[1]);carry_frame(&edicts[2]);
  assert(carryBank.size()==2&&carryBank.at("auto:host").phoneMessage==3&&carryBank.at("auto:guest").weapons[0].clip==3&&load<unsigned char>(priv(&edicts[1]),0x21d8)==1);
  for(int i:{1,2})carry_departing(&edicts[i]);gameOverMap.clear();carry_map_changed();
  // Next map: rebuilt players with nothing; the map already gave player 1 a shotgun.
  allocations.clear();memory.clear();player(1,100);player(2,100);give_weapon(&edicts[1],"weapon_shotgun",4);
  const int before=grants;logged.clear();carry_frame(&edicts[1]);carry_frame(&edicts[2]);
  SavedPlayer now;assert(grants==before+2&&capture_player(&edicts[1],now)&&now.health==83&&now.ammo[1]==28&&now.phoneMessage==3&&clip(&edicts[1])==7&&now.weapons.size()==2);
  assert(capture_player(&edicts[2],now)&&clip(&edicts[2])==3&&logged_line({"CARRY player=1 kept","phone=3"})&&logged_line({"CARRY player=2 kept"}));
  carry_frame(&edicts[1]);assert(grants==before+2);
  // Later in the map player 1 shoots (clip 2); every player dies: the restart
  // gives back what they carried in (clip 7).
  setClip(&edicts[1],2);store(g.data(),0,302.f);carry_frame(&edicts[1]);assert(carryBank.at("auto:host").weapons.size()==2);
  {const auto& w=carryBank.at("auto:host").weapons;assert(std::any_of(w.begin(),w.end(),[](const SavedWeapon& s){return s.name=="weapon_glock"&&s.clip==2;}));}
  gameOverMap="s4_start_e0";carry_map_changed();gameOverMap.clear();
  {const auto& w=carryBank.at("auto:host").weapons;assert(std::any_of(w.begin(),w.end(),[](const SavedWeapon& s){return s.name=="weapon_glock"&&s.clip==7;}));}
  // A save while the partner is busy (HUD not ready, a ladder, a scene)
  // keeps their last good record beside the saver instead of failing.
  carry_frame(&edicts[1]);carry_frame(&edicts[2]);requestedRecorder=421;checkpointSlot=3;std::filesystem::remove(checkpoint_directory()/"slot3.cofsave");
  allocations.at(&edicts[2]).commands=false;logged.clear();checkpoint_save(&edicts[1]);
  Checkpoint busy;bool busyBackup=false;assert(read_checkpoint(checkpoint_directory(),3,busy,busyBackup)&&busy.players.size()==2&&logged_line({"SAVE player 2 busy (client HUD not ready)"}));
  {const auto guest=std::find_if(busy.players.begin(),busy.players.end(),[](const SavedPlayer& s){return s.identity=="auto:guest";});const auto hostRecord=std::find_if(busy.players.begin(),busy.players.end(),[](const SavedPlayer& s){return s.identity=="auto:host";});
   assert(guest!=busy.players.end()&&hostRecord!=busy.players.end()&&guest->hasPose&&guest->origin==hostRecord->origin&&guest->weapons[0].clip==3);}
  // Without any record the save fails, and the saver is told on screen.
  carry_reset();std::filesystem::remove(checkpoint_directory()/"slot3.cofsave");logged.clear();checkpoint_save(&edicts[1]);
  assert(!std::filesystem::exists(checkpoint_directory()/"slot3.cofsave")&&printedTo==&edicts[1]&&printedType==1&&printedText=="Could not save:\nyour partner is not ready\n");
  allocations.at(&edicts[2]).commands=true;checkpointSlot=1;}
 // coop-fixes-013: a story weapon given by a scene reaches every living
 // player who lacks it; item cards and the wake-up's sc3_native_ fan-outs do not.
 {auto giver=named(intern("cof_giveitems"));store(ev(giver),0x1d8,intern("weapon_shotgun"));helpers[giver]["message"]="weapon_shotgun";
  int before=grants;logged.clear();give_lookup(nullptr,"targetname","giveitem1",giver);give_lookup(nullptr,"targetname","giveitem1",giver);give_flush();
  assert(grants==before+1&&logged_line({"STORYITEM weapon_shotgun also given to player=2"})&&!logged_line({"player=1"}));give_flush();assert(grants==before+1);
  auto again=named(intern("cof_giveitems"));store(ev(again),0x1d8,intern("weapon_branch"));helpers[again]["message"]="weapon_branch";
  give_lookup(nullptr,"targetname","sc3_native_9",again);give_flush();assert(grants==before+1);
  store(ev(again),0x1a4,1);give_lookup(nullptr,"targetname","givekey",again);give_flush();assert(grants==before+1);
  store(ev(again),0x1a4,0);store(ev(&edicts[2]),0x174,2);give_lookup(nullptr,"targetname","givebranch",again);give_flush();assert(grants==before+2);
  // v0.1: the dead player is owed it, given once alive with a ready HUD, never twice.
  assert(logged_line({"STORYITEM weapon_branch owed to player=2"})&&giveOwed.count("auto:guest")&&!owns_weapon(&edicts[2],"weapon_branch"));
  give_owed_frame(&edicts[2]);assert(grants==before+2);
  store(ev(&edicts[2]),0x174,0);give_owed_frame(&edicts[2]);assert(grants==before+3&&owns_weapon(&edicts[2],"weapon_branch")&&!giveOwed.count("auto:guest")&&logged_line({"STORYITEM weapon_branch given to player=2 after the revive"}));
  give_owed_frame(&edicts[2]);store(ev(&edicts[2]),0x174,2);give_lookup(nullptr,"targetname","givebranch",again);give_flush();store(ev(&edicts[2]),0x174,0);give_owed_frame(&edicts[2]);assert(grants==before+3);
  give_owed_reset();}
 // v0.1: a level change reads every player at once, a player frozen in a
 // scene included (a weapon the scene just gave is carried); a dead player
 // keeps their last good record.
 {carry_reset();store(g.data(),0x0c,2);logged.clear();
  store(ev(&edicts[1]),0x1a8,load<unsigned>(ev(&edicts[1]),0x1a8)|0x80u);SavedPlayer frozen;assert(!capture_player(&edicts[1],frozen)&&capture_player(&edicts[1],frozen,false));
  carry_frame(&edicts[2]);const auto guestBefore=carryBank.at("auto:guest");store(ev(&edicts[2]),0x174,2);
  carry_level_change("s4_next");
  assert(carryBank.count("auto:host")&&carryBank.at("auto:host").weapons.size()==frozen.weapons.size()&&carryBank.at("auto:guest").weapons.size()==guestBefore.weapons.size());
  assert(logged_line({"CARRY level change to s4_next: 1 player record(s) read now, 1 kept from before"}));
  store(ev(&edicts[2]),0x174,0);store(ev(&edicts[1]),0x1a8,load<unsigned>(ev(&edicts[1]),0x1a8)&~0x80u);carry_reset();}
 // coop-fixes-013: a client watching a scene camera is sent no player entity.
 {auto cam=named(intern("trigger_camera"));clientViews.clear();assert(!hide_player_from(&edicts[2],&edicts[1]));view_seen(cam,&edicts[1]);
  assert(hide_player_from(&edicts[2],&edicts[1])&&hide_player_from(&edicts[1],&edicts[1])&&!hide_player_from(&edicts[1],&edicts[2])&&!hide_player_from(cam,&edicts[1]));
  view_seen(&edicts[1],&edicts[1]);assert(!hide_player_from(&edicts[2],&edicts[1]));}
 // coop-fixes-014: a hidden player is still packed (native result kept) with EF_NODRAW added;
 // the client's own player is never dropped, so a client joining into a running camera gets it.
 {auto cam=named(intern("trigger_camera"));clientViews.clear();view_seen(cam,&edicts[1]);std::array<unsigned char,0x154> s{};const int before=packs;
  assert(full_pack(s.data(),2,&edicts[2],&edicts[1],0,1,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x88u);
  s={};assert(full_pack(s.data(),1,&edicts[1],&edicts[1],0,1,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x88u);
  s={};assert(full_pack(s.data(),1,&edicts[1],&edicts[2],0,1,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x8u);
  packAccept=0;s={};assert(full_pack(s.data(),2,&edicts[2],&edicts[1],0,1,nullptr)==0&&load<unsigned>(s.data(),0x3c)==0x8u);packAccept=1;
  view_seen(&edicts[1],&edicts[1]);s={};assert(full_pack(s.data(),2,&edicts[2],&edicts[1],0,1,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x8u&&packs==before+5);}
 // v0.2: staged actors are hidden by the maps (build-v02-maps.mjs); the adapter
 // keeps every non-player packet native in every view, Simon models included.
 {auto simon=named(intern("env_model"));store(ev(simon),0xb8,intern("models/cutscene/player3.mdl"));
  auto cam=named(intern("trigger_camera"));clientViews.clear();std::array<unsigned char,0x154> s{};
  assert(full_pack(s.data(),40,simon,&edicts[1],0,0,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x8u);
  view_seen(cam,&edicts[1]);s={};assert(full_pack(s.data(),40,simon,&edicts[1],0,0,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x8u);
  view_seen(&edicts[1],&edicts[1]);s={};assert(full_pack(s.data(),40,simon,&edicts[2],0,0,nullptr)==1&&load<unsigned>(s.data(),0x3c)==0x8u);
  clientViews.clear();}
 // v0.2: a converted door's music reaches every player at once (the native
 // multiplayer trigger_mp3 rule), without restarting it for a player who already
 // has that track; a silent room stops it for everyone; other names are refused.
 {coopMusic="old";for(int i:{1,2}){store(priv(&edicts[i]),0x2310,0);store(priv(&edicts[i]),0x2314,0);}
  args={"sc_door_music","aphall2.mp3","1"};door_music_command();
  for(int i:{1,2})assert(!std::strcmp(str_(load<int>(priv(&edicts[i]),0x2310)),"aphall2.mp3")&&load<int>(priv(&edicts[i]),0x2314)==1&&load<unsigned char>(priv(&edicts[i]),0x231c)==1);
  assert(coopMusic=="aphall2.mp3"&&logged_line({"MUSIC door aphall2.mp3 (loop): changed for 2 player(s), 0 already on it"}));
  for(int i:{1,2})store(priv(&edicts[i]),0x2314,0);door_music_command();
  for(int i:{1,2})assert(load<int>(priv(&edicts[i]),0x2314)==0);assert(logged_line({"changed for 0 player(s), 2 already on it"}));
  store(priv(&edicts[2]),0x2310,intern("other.mp3"));args={"sc_door_music","aphall2.mp3","0"};door_music_command();
  assert(load<int>(priv(&edicts[1]),0x2314)==0&&load<int>(priv(&edicts[2]),0x2314)==1&&load<unsigned char>(priv(&edicts[2]),0x231c)==0);
  args={"sc_door_music","-","2"};door_music_command();
  for(int i:{1,2})assert(load<int>(priv(&edicts[i]),0x2310)==0&&load<int>(priv(&edicts[i]),0x2314)==1&&load<float>(priv(&edicts[i]),0x2318)==1.f);   // no fade given: the game's 1 s
  assert(coopMusic.empty()&&logged_line({"MUSIC door silence: changed for 2 player(s)"}));
  args={"sc_door_music","../x.wav","0"};door_music_command();assert(logged_line({"MUSIC door request refused"})&&coopMusic.empty());
  for(int i:{1,2}){store(priv(&edicts[i]),0x2314,0);store(priv(&edicts[i]),0x2318,0.f);}args={"","",""};}
 // v0.1: converted co-op door buttons get the latch locked sound before their
 // native Spawn; other buttons keep their own keys.
 {auto door=named(intern("func_button"));store(ev(door),0x1cc,intern("cof_story_present228"));
  auto other=named(intern("func_button"));store(ev(other),0x1cc,intern("lift_go"));
  logged.clear();spawn(door);spawn(other);assert(helpers[door]["locked_sound"]=="12"&&!helpers[other].count("locked_sound"));
  story_door_report();assert(logged_line({"DOORS locked sound ready on 1 co-op door button(s)"}));}
 // v0.2 IDLE_DIAG (read only): a still partner's predicted field changed outside pmove
 // is named with the stage that changed it; changes inside pmove and the host are not traced.
 {idle_reset();const float t0=load<float>(g.data(),0);store(g.data(),0,300.f);std::array<unsigned char,0x34> cmd{};auto e=&edicts[2];const float z=load<float>(ev(e),0x180);
  for(int k=0;k<3;++k){++netFrame;store(ev(e),0x20,0.f);store(ev(e),0x24,0.f);store(ev(e),0x28,0.f);idle_command(e,cmd.data());idle_command(&edicts[1],cmd.data());idle_stage(e,1);idle_stage(e,2);
   store(ev(e),0x0c,load<float>(ev(e),0x0c)+0.5f);idle_stage(e,-1);store(ev(e),0x180,28.f+k);idle_stage(e,3);idle_stage(e,4);}
  store(g.data(),0,311.f);idle_command(e,cmd.data());idle_command(&edicts[1],cmd.data());
  assert(logged_line({"IDLE_DIAG player=2 field=view_ofs changed 3 time(s) at native PostThink"})&&!logged_line({"IDLE_DIAG player=2 field=origin"})&&!logged_line({"IDLE_DIAG player=1"}));
  // v0.5.6 NET_CADENCE: the 4th command came in the same host frame as the 3rd.
  assert(logged_line({"NET_CADENCE player=2 4 commands arrived in 3 host frames (up to 2 at once) in 11 s"})&&!logged_line({"NET_CADENCE player=1"}));
  store(ev(e),0x180,z);store(ev(e),0x0c,load<float>(ev(e),0x0c)-1.5f);store(g.data(),0,t0);idle_reset();}
 // v0.3 audit: the main menu map ends the game; a Continue prepared for the next map survives.
 {carryBank["auto:host"]=SavedPlayer{};mapStates["s4_apartment1"]=MapState{};checkpointRestoreFailed=true;gameOverMap="s4_start_e0";
  Checkpoint next;next.map="s4_start_e0";loadedCheckpoint=next;checkpointPrepared=true;checkpointSlot=2;
  store(g.data(),0x0c,intern("c_game_menu1"));menu_reset();
  assert(carryBank.empty()&&mapStates.empty()&&!checkpointRestoreFailed&&gameOverMap.empty()&&checkpointPrepared&&checkpointSlot==2&&loadedCheckpoint.map=="s4_start_e0");
  checkpointPrepared=false;loadedCheckpoint={};store(g.data(),0x0c,2);}
 std::puts("PASS owned checkpoint runtime: card-path inventory restore with soft equipped/teleport always applied, automatic host/guest identities without console binding, ambiguous-guest refusal, role-matched and exact-profile restores, save blocked while a role-matched guest awaits restore, Continue request file (exact map, menu boot, malformed, stale map), host-only recorder slot list (1/2/3, list again on reuse, timeout, walk away), partner shown 'Only host can save the game' while at a recorder, save sound on every client, native menu/fade interception, SDK reconstruction, readback, no repeated grants, reason-coded diagnostics without identities, inventory/phone carried across map changes with game-over revert, busy-partner saves, story weapons for every player, player models packed but not drawn (EF_NODRAW) in scene cameras, own player always sent, v0.1 carry read at the level change (frozen players included, dead keep their record), story weapons owed to a dead player, Simon actors drawn only in scene views. NOT Cry of Fear gameplay verification.");
}
