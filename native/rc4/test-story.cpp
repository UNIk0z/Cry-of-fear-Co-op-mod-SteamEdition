// Owned SDK fixture for story-runtime.hpp (coop-fixes-006): dual wield
// command, third-person pose while dual wielding, ending flags and story
// wounds across map changes and checkpoints. No game code is loaded and no
// game is launched; the DualWield call is a local stand-in.
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
std::vector<std::string> lines,argv;std::string gameDir;
std::vector<std::array<int,3>> messages;
const char* strings(int n){switch(n){case 1:return "s4_start";case 2:return "player";case 3:return "c_game_menu1";case 4:return "weapon_dualweapon";case 5:return "models/player.mdl";case 6:return "weapon_switchblade";case 7:return "weapon_mobile";case 8:return "cof_giveitems";case 9:return "sc3_native_497";case 10:return "c_game_menu1";case 11:return "cof_campaign_story4";case 12:return "inventory/weapon_switchblade.txt";case 13:return "inventory/weapon_mobile.txt";case 14:return "inventory/weapon_glock.txt";}return "";}
int index(Edict* e){for(int i=0;i<10;++i)if(e==&entities[i])return i;return -1;}
Edict* byindex(int i){return i>=0&&i<10?&entities[i]:nullptr;}
void* ev(Edict* e){return e->data.data()+0x80;}
void* pd(Edict* e){auto it=mem.find(e);return it==mem.end()?nullptr:it->second.data();}
void* alloc(Edict* e,int n){mem[e]=std::vector<unsigned char>(n);return mem[e].data();}
void print(const char* s){lines.emplace_back(s);}
void client_print(Edict*,int,const char* s){lines.emplace_back(std::string("CLIENT:")+s);}
const char* arg(int i){return i>=0&&i<static_cast<int>(argv.size())?argv[i].c_str():"";}
void game_dir(char* out){std::strcpy(out,gameDir.c_str());}
int user(Edict* e){return index(e);}
int message_id(const char*,int){return 77;}
int pendingDestination=0,pendingType=0,pendingTarget=0;
void begin(int destination,int type,const float*,Edict* e){pendingDestination=destination;pendingType=type;pendingTarget=index(e);}
void byte_(int value){messages.push_back({pendingType,pendingTarget,value});}
void short_(int value){messages.push_back({pendingType,pendingTarget,value});}
void end(){}
Edict* fake_find(Edict* start,const char*,const char* value){return !start&&std::strcmp(value,"sc3_native_497")==0?&entities[5]:nullptr;}
int uses=0;
void give_weapon(Edict* player,int slot,int cls){auto w=&entities[slot];entities[slot]={};store(w,4,10+slot);store(ev(w),0,cls);allocate(w,0x300);store(pd(player),0x2514,reinterpret_cast<std::uint32_t>(pd(w)));}
void __cdecl fake_use(Edict* used,Edict* other){assert(used==&entities[5]);++uses;give_weapon(other,other==&entities[1]?6:7,7);}
void __cdecl native_noop(Edict*){}
void __cdecl free_private(Edict*){}
std::vector<std::string> stuffed;
void __cdecl stuff_(Edict* e,const char* format,...){va_list a;va_start(a,format);const char* text=va_arg(a,const char*);va_end(a);stuffed.push_back(std::to_string(index(e))+":"+text);}
int listen(){return 0;}
bool logged(const char* part){for(const auto& l:lines)if(l.find(part)!=std::string::npos)return true;return false;}
// __fastcall(ecx,edx,stack...) has the __thiscall(ecx,stack...) layout.
void* dualSelf=nullptr;int dualA=0,dualB=0,dualCalls=0,dualWeightSeen=-1;
void __fastcall fake_dual(void* self,void*,int a,int b){dualSelf=self;dualA=a;dualB=b;++dualCalls;dualWeightSeen=dualWeightOverride?*dualWeightOverride:-1;store(self,0x1e70,2);store(self,0x1e74,6);}
void reset_players(){
 allocations.clear();mem.clear();lines.clear();pose_reset();
 for(int i=0;i<10;++i){entities[i]={};store(&entities[i],4,10+i);store(ev(&entities[i]),0,2);}
 for(int i=1;i<=2;++i){allocate(&entities[i],PlayerBytes);store(ev(&entities[i]),0x164,100.f);store(ev(&entities[i]),0xb8,5);allocations.at(&entities[i]).commands=true;}
}
void* give_dual(Edict* player){
 auto w=&entities[3];store(ev(w),0,4);allocate(w,0x300);store(pd(player),0x252c,reinterpret_cast<std::uint32_t>(pd(w)));return pd(w);
}
// A studio model with the sequences the pose needs (version 10 layout).
void write_model(){
 struct Seq{const char* name;float fps;int flags,frames;};
 const Seq seqs[]={{"pro",30,0,10},{"idle",30,1,31},{"switchblade_idle",15,1,16},{"switchblade_attack1",30,0,21},{"pistol_idle",30,1,11},{"pistol_shoot",30,0,7},{"cpr_idle",30,1,30}};
 std::vector<unsigned char> b(200+sizeof(seqs)/sizeof(seqs[0])*176);std::memcpy(b.data(),"IDST",4);store(b.data(),4,10);store(b.data(),164,static_cast<int>(sizeof(seqs)/sizeof(seqs[0])));store(b.data(),168,200);
 for(std::size_t i=0;i<sizeof(seqs)/sizeof(seqs[0]);++i){auto s=b.data()+200+i*176;std::memcpy(s,seqs[i].name,std::strlen(seqs[i].name));store(s,32,seqs[i].fps);store(s,36,seqs[i].flags);store(s,56,seqs[i].frames);}
 std::filesystem::create_directories(std::filesystem::path(gameDir)/"models");std::ofstream f(std::filesystem::path(gameDir)/"models"/"player.mdl",std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));
}
}
int main(int argc,char** argv_){
 assert(argc==2);gameDir=argv_[1];
 globals=globalData.data();verified=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,10);store(globals,0,5.f);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&pd);engine[66]=reinterpret_cast<void*>(&strings);engine[68]=reinterpret_cast<void*>(&ev);engine[71]=reinterpret_cast<void*>(&index);engine[72]=reinterpret_cast<void*>(&byindex);
 engine[81]=reinterpret_cast<void*>(&print);engine[80]=reinterpret_cast<void*>(&client_print);engine[83]=reinterpret_cast<void*>(&arg);engine[99]=reinterpret_cast<void*>(&game_dir);engine[113]=reinterpret_cast<void*>(&user);
 engine[75]=reinterpret_cast<void*>(&message_id);engine[46]=reinterpret_cast<void*>(&begin);engine[47]=reinterpret_cast<void*>(&end);engine[48]=reinterpret_cast<void*>(&byte_);engine[50]=reinterpret_cast<void*>(&short_);
 auto e=&entities[1],other=&entities[2];

 // Dual wield: in co-op the command reaches DualWield for the sender.
 reset_players();dualWieldTarget=reinterpret_cast<DualWieldCall>(&fake_dual);
 int dualWeight=20;dualWeightOverride=&dualWeight;
 argv={"inventorydualwield","1","3"};assert(dual_command(e,"inventorydualwield"));
 assert(dualCalls==1&&dualSelf==pd(e)&&dualA==1&&dualB==3&&logged("DUAL player=1 slots=1,3 hands=2,6 holding=other"));
 // coop-fixes-008: the dual weapon outweighs any held item during the call only.
 assert(dualWeightSeen==1000000&&dualWeight==20);
 // Equal slots go to the native check (it answers with its own sound).
 argv={"inventorydualwield","2","2"};assert(dual_command(e,"inventorydualwield")&&dualCalls==2&&dualA==2&&dualB==2);
 // Slots outside 1..6 never reach it (DualWield indexes before its check).
 for(const auto& bad:std::vector<std::vector<std::string>>{{"inventorydualwield","7","1"},{"inventorydualwield","0","1"},{"inventorydualwield","12","1"},{"inventorydualwield","1",""},{"inventorydualwield"}}){argv=bad;assert(dual_command(e,"inventorydualwield"));}
 assert(dualCalls==2&&logged("DUAL refused player=1 slots=0,1"));
 // A dead player is refused; other commands and other maps stay native.
 argv={"inventorydualwield","1","3"};store(ev(e),0x174,2);assert(dual_command(e,"inventorydualwield")&&dualCalls==2);store(ev(e),0x174,0);
 assert(!dual_command(e,"inventorydrop"));store(globals,0x0c,3);assert(!dual_command(e,"inventorydualwield")&&dualCalls==2);store(globals,0x0c,1);
 // Without the pinned backend and no stand-in there is nothing to call.
 dualWieldTarget=nullptr;assert(!dual_target());assert(dual_command(e,"inventorydualwield")&&dualCalls==2);dualWieldTarget=reinterpret_cast<DualWieldCall>(&fake_dual);
 // v0.5 D5: two quick-slot keys within 0.25 s dual wield their weapons (newer key first).
 {auto p=pd(e);store(p,0x2200,12);store(p,0x2204,14);std::strcpy(static_cast<char*>(p)+0x204c,"weapon_switchblade");std::strcpy(static_cast<char*>(p)+0x20cc,"weapon_glock");
  store(globals,0,5.f);quick_dual_command(e,"quicksel","1");assert(dualCalls==2);
  store(globals,0,5.1f);quick_dual_command(e,"quicksel","2");assert(dualCalls==3&&dualA==2&&dualB==1&&logged("DUAL quick player=1 keys 1+2 weapon_glock+weapon_switchblade asked"));
  store(globals,0,5.15f);quick_dual_command(e,"quicksel","1");assert(dualCalls==3);   // a third key starts over
  store(globals,0,6.f);quick_dual_command(e,"quicksel","1");store(globals,0,6.5f);quick_dual_command(e,"quicksel","2");assert(dualCalls==3);   // too slow
  store(globals,0,7.f);quick_dual_command(e,"quicksel","2");store(globals,0,7.1f);quick_dual_command(e,"quicksel","2");assert(dualCalls==3);   // the same key twice
  store(globals,0,8.f);quick_dual_command(e,"quicksel","3");store(globals,0,8.1f);quick_dual_command(e,"quicksel","1");assert(dualCalls==3);   // an empty quick slot
  // v0.5.7 (playtest: the phone went into the left hand): the phone is never dual wielded.
  store(p,0x2204,13);std::strcpy(static_cast<char*>(p)+0x20cc,"weapon_mobile");store(globals,0,9.f);quick_dual_command(e,"quicksel","1");store(globals,0,9.05f);quick_dual_command(e,"quicksel","2");assert(dualCalls==3);
  store(globals,0,5.f);quickPresses.clear();}
 // v0.5 D4: after a load the inventory screen is told each restored quick key, one second later.
 {auto p=pd(e);messages.clear();qslotUpdateMessage=55;store(p,0x3dc,2);store(p,0x3e0,0);store(p,0x3e4,4);std::strcpy(static_cast<char*>(p)+0x214c,"weapon_mobile");
  quick_notice_want(e);quick_notice_frame(e);assert(messages.empty());store(globals,0,6.1f);quick_notice_frame(e);
  assert((messages==std::vector<std::array<int,3>>{{55,1,1},{55,1,2},{55,1,3},{55,1,4}})&&logged("QUICKKEYS player=1 2 shown"));
  quick_notice_frame(e);assert(messages.size()==4);qslotUpdateMessage=0;store(globals,0,5.f);}

 // Hand choice: the weapon, never the light; with two weapons the right.
 assert(pose_item(2,6)==6&&pose_item(6,2)==6&&pose_item(3,6)==6&&pose_item(6,3)==3&&pose_item(2,13)==2&&pose_item(15,0)==0);
 assert(!std::strcmp(weapon_idle(6),"switchblade_idle")&&!std::strcmp(weapon_idle(19),"vp70_idle")&&!weapon_idle(15)&&!weapon_idle(1)&&!weapon_idle(28)&&light_item(2)&&light_item(13)&&!light_item(6));

 // Model sequences are read by name from the model file, bounds-checked.
 write_model();const auto& model=model_sequences("models/player.mdl");
 assert(model.valid&&model.find("switchblade_idle")==2&&model.find("missing")==-1&&attack_sequence(model,"switchblade_idle")==3&&attack_sequence(model,"pistol_idle")==5&&attack_sequence(model,"radio_idle")==-1);
 assert(!model_sequences("../models/player.mdl").valid&&!model_sequences("models/../x.mdl").valid&&!model_sequences("models/none.mdl").valid);

 // Pose: knife + phone shows the knife idle, smoothly against native resets.
 reset_players();auto p=pd(e);auto weapon=give_dual(e);store(p,0x1e70,2);store(p,0x1e74,6);store(ev(e),0x12c,1);store(globals,0,10.f);
 pose_frame(e);
 assert(load<int>(ev(e),0x12c)==2&&load<float>(ev(e),0x134)==0.f&&load<float>(ev(e),0x138)==10.f&&load<float>(ev(e),0x13c)==1.f&&load<int>(p,0xfc)==1&&load<float>(p,0x4f4)==0.f&&logged("POSE player=1 dual hands=2,6 shows switchblade_idle"));
 assert(std::fabs(load<float>(p,0xec)-256.f*15.f/15.f)<0.01f);
 // animtime stays "now" (the client moves the model only when it changes);
 // the frame carries on from the idle's start.
 store(globals,0,10.5f);store(ev(e),0x12c,1);store(ev(e),0x134,0.f);store(ev(e),0x138,10.5f);pose_frame(e);assert(load<int>(ev(e),0x12c)==2&&load<float>(ev(e),0x138)==10.5f&&std::fabs(load<float>(ev(e),0x134)-128.f)<0.01f);
 store(globals,0,11.25f);store(ev(e),0x12c,1);pose_frame(e);assert(load<float>(ev(e),0x138)==11.25f&&std::fabs(load<float>(ev(e),0x134)-64.f)<0.01f);
 store(globals,0,11.f);store(ev(e),0x12c,1);store(ev(e),0x138,11.f);pose_frame(e);assert(load<int>(ev(e),0x12c)==2&&load<float>(ev(e),0x138)==11.f);
 // An attack press that moved the dual weapon's next attack plays the knife attack once.
 store(weapon,0x1d0,11.5f);store(p,0x22b0,1u);pose_frame(e);
 assert(load<int>(ev(e),0x12c)==3&&load<float>(ev(e),0x138)==11.f&&std::fabs(load<float>(p,0x4f4)-(11.f+21.f/30.f))<0.001f&&logged("dual attack switchblade_attack1"));
 store(globals,0,11.2f);pose_frame(e);assert(load<int>(ev(e),0x12c)==3);
 // The press alone (nothing fired) or a timer move without a press does nothing.
 store(globals,0,12.f);store(ev(e),0x12c,1);store(p,0x22b0,1u);pose_frame(e);assert(load<int>(ev(e),0x12c)==2&&load<float>(ev(e),0x138)==12.f);
 store(p,0x22b0,0u);store(weapon,0x1d0,13.f);pose_frame(e);assert(load<int>(ev(e),0x12c)==2);
 // A native animation (revive) is left alone; a non-dual weapon is native.
 store(ev(e),0x12c,6);pose_frame(e);assert(load<int>(ev(e),0x12c)==6);
 store(ev(e),0x12c,1);store(p,0x252c,0u);pose_frame(e);assert(load<int>(ev(e),0x12c)==1);
 // Two weapons: the right hand. Outside co-op nothing changes.
 give_dual(e);store(p,0x1e70,3);store(p,0x1e74,6);pose_frame(e);assert(load<int>(ev(e),0x12c)==2);
 store(p,0x1e70,6);store(p,0x1e74,3);store(ev(e),0x12c,1);pose_frame(e);assert(load<int>(ev(e),0x12c)==4);
 store(globals,0x0c,3);store(ev(e),0x12c,1);pose_frame(e);assert(load<int>(ev(e),0x12c)==1);store(globals,0x0c,1);

 // A save while dual wielding keeps the two weapons and holds the weapon hand.
 {reset_players();p=pd(e);profileBindings[1]="profile:host";
  auto add=[&](int slot,int string,int id){auto w=&entities[slot];store(ev(w),0,string);allocate(w,0x300);store(pd(w),0x168,id);return pd(w);};
  auto knife=add(4,6,6),phone=add(5,7,2),dual=add(3,4,15);
  store(p,0x2514,reinterpret_cast<std::uint32_t>(knife));store(knife,0x164,reinterpret_cast<std::uint32_t>(phone));store(phone,0x164,reinterpret_cast<std::uint32_t>(dual));
  store(p,0x252c,reinterpret_cast<std::uint32_t>(dual));store(p,0x1e70,2);store(p,0x1e74,6);store(p,0x60c,1);store<unsigned char>(p,0x434,1);
  SavedPlayer saved;assert(capture_player(e,saved));
  assert(saved.weapons.size()==2&&saved.equipped=="weapon_switchblade"&&saved.healthCap==1&&saved.nightVision);
  profileBindings.clear();}

 // Ending flags: any player's flag is shared and survives map changes.
 reset_players();story_reset();lines.clear();store(globals,0,20.f);
 store<unsigned char>(pd(other),0x4ce,1);story_frame();
 assert(endingShared==2&&load<unsigned char>(pd(e),0x4ce)==1&&!load<unsigned char>(pd(e),0x4cd)&&logged("ENDING flags=2 (roof boss 0, P345 given 1, package 0) from player=2"));
 store<unsigned char>(pd(e),0x4cd,1);story_frame();assert(endingShared==3&&load<unsigned char>(pd(other),0x4cd)==1);
 reset_players();story_map_changed();story_frame();
 assert(load<unsigned char>(pd(e),0x4cd)==1&&load<unsigned char>(pd(e),0x4ce)==1&&!load<unsigned char>(pd(e),0x4cf)&&load<unsigned char>(pd(other),0x4ce)==1);
 // Checkpoint schema 4 carries them; a new campaign clears them.
 {Checkpoint value;story_capture(value);assert(value.endingFlags==3);story_reset();assert(endingShared==0);story_load(value);assert(endingShared==3);
  reset_players();story_reset();story_frame();assert(!load<unsigned char>(pd(e),0x4cd));}

 // v0.7: the Doctor's shot (one player's health cap) becomes every player's; night vision stays per player.
 reset_players();story_reset();messages.clear();register_message("HealthCap",1);assert(healthCapMessage==77);
 profileBindings[1]="profile:host";profileBindings[2]="profile:guest";store(globals,0,30.f);
 story_frame();store(pd(e),0x60c,2);store<unsigned char>(pd(other),0x434,1);story_frame();
 assert(load<int>(pd(other),0x60c)==2&&capShared==2&&messages.size()==1&&messages[0]==(std::array<int,3>{{77,2,2}})&&logged("STORY health cap 2 from player=1, now every player's"));
 // A level change rebuilds both players without it: each gets it back once his HUD is ready.
 reset_players();story_map_changed();messages.clear();store(globals,0,40.f);allocations.at(e).hudDue=50.f;
 story_frame();assert(load<int>(pd(e),0x60c)==0&&load<int>(pd(other),0x60c)==2&&messages.size()==1);
 store(globals,0,51.f);story_frame();
 assert(load<int>(pd(e),0x60c)==2&&load<unsigned char>(pd(other),0x434)==1&&!load<unsigned char>(pd(e),0x434)&&messages.size()==2&&messages[1]==(std::array<int,3>{{77,1,2}}));
 // The story clearing it on one player (impulse 0) clears it for everyone.
 store(pd(e),0x60c,0);story_frame();assert(load<int>(pd(other),0x60c)==0&&capShared==0&&messages.size()==3);
 // A checkpoint keeps it.
 {Checkpoint v;SavedPlayer s1;s1.identity="profile:host";s1.healthCap=1;v.players.push_back(s1);story_load(v);assert(capShared==1);story_reset();}
 profileBindings.clear();

 // Checkpoint schema 4+ round trip with the new fields; schema 3 still loads.
 {Checkpoint v;v.map="s4_start2";v.owner="auto:host";v.recorder=12;v.globals.tokens.resize(1024);v.globals.tokens[1]="GLOBAL";v.globals.tokens[2]="m_listCount";
  auto field=[&](int token,int value){const std::uint16_t size=4,t=static_cast<std::uint16_t>(token);auto add=[&](const void* q,std::size_t n){auto a=static_cast<const unsigned char*>(q);v.globals.bytes.insert(v.globals.bytes.end(),a,a+n);};add(&size,2);add(&t,2);add(&value,4);};field(1,1);field(2,0);
  SavedPlayer s;s.identity=v.owner;s.health=90;s.stamina=50;s.hasPose=true;s.healthCap=2;s.nightVision=true;v.players.push_back(s);v.endingFlags=5;
  auto bytes=encode_checkpoint(v);assert(!bytes.empty()&&load<std::uint32_t>(bytes.data(),8)==8);Checkpoint loaded;assert(decode_checkpoint(bytes,loaded)&&loaded.endingFlags==5&&loaded.players[0].healthCap==2&&loaded.players[0].nightVision);
  auto bad=v;bad.endingFlags=8;assert(encode_checkpoint(bad).empty());bad=v;bad.players[0].healthCap=3;assert(encode_checkpoint(bad).empty());
  // A schema 3 file (no appendix) decodes with zero story state.
  auto old=bytes;old.resize(old.size()-12-18-8-6-32);   // schema 4, 5, 6, 7 and 8 appendices (one player, no music, no dual)
 store(old.data(),8,3u);store(old.data(),12,static_cast<std::uint32_t>(old.size()));
  std::uint32_t c=0xffffffff;for(std::size_t i=20;i<old.size();++i){c^=old[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<int>(c&1)));}store(old.data(),16,~c);
  assert(decode_checkpoint(old,loaded)&&loaded.endingFlags==0&&loaded.players[0].healthCap==0&&!loaded.players[0].nightVision);}
 // coop-fixes-007 credits: one CoFLogo per player per 4 s; a later one waits,
 // a repeat is dropped; outside the campaign the message is untouched.
 reset_players();messages.clear();logo_reset();cofLogoMessage=88;store(globals,0,100.f);
 auto logo=[&](Edict* to,int value){message_begin(1,88,nullptr,to);write_byte(value);message_end();};
 logo(e,5);assert(messages.size()==1&&messages[0]==(std::array<int,3>{{88,1,5}})&&logged("CREDIT player=1 logo=5 at 100.0"));
 store(globals,0,101.f);logo(e,6);assert(messages.size()==1&&logged("CREDIT player=1 logo=6 held"));
 store(globals,0,101.5f);logo(e,6);assert(messages.size()==1&&logged("CREDIT player=1 logo=6 repeat dropped"));
 store(globals,0,103.f);logo_frame();assert(messages.size()==1);
 store(globals,0,104.1f);logo_frame();assert(messages.size()==2&&messages[1]==(std::array<int,3>{{88,1,6}}));
 store(globals,0,104.2f);logo(e,6);assert(messages.size()==2);
 logo(other,5);assert(messages.size()==3&&messages[2]==(std::array<int,3>{{88,2,5}}));
 store(globals,0x0c,10);logo(e,1);logo(e,2);assert(messages.size()==5);store(globals,0x0c,1);
 // v0.5 C4: on the opening map every player gets each intro logo the first player gets.
 logo_reset();messages.clear();store(globals,0x0c,11);store(globals,0,200.f);logo(e,9);
 assert(messages.size()==2&&messages[0]==(std::array<int,3>{{88,1,9}})&&messages[1]==(std::array<int,3>{{88,2,9}})&&logged("CREDIT player=2 logo=9 at 200.0"));store(globals,0x0c,1);
 logo_reset();cofLogoMessage=0;

 // coop-fixes-008 phone light: ScoreInfo goes to each player separately; the
 // own entry and the entry whose plain spot shares the viewer's textured spot
 // key lose only the light bits.
 reset_players();messages.clear();score_reset();scoreInfoMessage=66;
 auto score=[&](int who,int frags){message_begin(2,66,nullptr,nullptr);write_byte(who);write_short(frags);write_short(3);write_short(0);write_short(1);message_end();};
 auto sent=[&](int to,int who,int frags){for(std::size_t i=0;i+4<messages.size();++i)if(messages[i][0]==66&&messages[i][1]==to&&messages[i][2]==who&&messages[i+1][2]==frags&&messages[i+2][2]==3&&messages[i+4][2]==1)return true;return false;};
 store(pd(e),0x5d0,1u);store(pd(other),0x5d0,0u);score(1,1);
 assert(messages.size()==10&&sent(1,1,0)&&sent(2,1,1));
 // Both phone lights on: the partner keeps its own textured spot.
 messages.clear();store(pd(other),0x5d0,1u);score(2,1);
 assert(sent(2,1,0)&&sent(1,2,1)&&sent(2,2,0)&&messages.size()==15&&logged("LIGHT player=2 gets player 1's light flags 1 without the plain spot"));
 messages.clear();score_sync();assert(messages.empty());
 // The partner's light goes off: the host's plain spot is shown again.
 store(pd(other),0x5d0,0u);score_sync();assert(messages.size()==5&&sent(2,1,1));
 // Other maps: untouched broadcast.
 messages.clear();store(globals,0x0c,10);score(1,1);assert(messages.size()==5&&messages[0][1]==-1);store(globals,0x0c,1);
 score_reset();scoreInfoMessage=0;

 // coop-fixes-007 rejoin: a player kicked while joining gets the wake-up
 // fan-out they missed; a player who had played gets their own state back.
 engine[14]=reinterpret_cast<void*>(&fake_find);original[3]=reinterpret_cast<void*>(&fake_use);original[16]=reinterpret_cast<void*>(&native_noop);engine[65]=reinterpret_cast<void*>(&free_private);
 store(globals,0,5.f);reset_players();rejoin_reset();checkpoint_reset();profileBindings[1]="profile:host";profileBindings[2]="profile:guest";
 {auto g=&entities[5];entities[5]={};store(g,4,15);store(ev(g),0,8);store(ev(g),0x1d8,7);}
 store(globals,0,20.f);assert(find(nullptr,"targetname","sc3_native_497")==&entities[5]&&fanouts.size()==1&&logged("REJOIN fan-out give weapon_mobile recorded at 20.0"));
 find(nullptr,"targetname","sc3_native_497");assert(fanouts.size()==1);
 // Present when it fired: nothing is replayed.
 rejoin_frame(e);rejoin_frame(other);assert(uses==0);
 // P2 kicked before the HUD settled: no record to keep, nothing banked.
 allocations.at(other).commands=false;checkpoint_disconnect(other);release(other);assert(departedPlayers.empty()&&playerBank.empty());
 store(globals,0,25.f);allocate(other,PlayerBytes);store(ev(other),0x164,100.f);allocations.at(other).commands=true;
 rejoin_frame(other);assert(uses==1&&owns_weapon(other,"weapon_mobile")&&logged("REJOIN player=2 missed fan-out: gave weapon_mobile"));
 rejoin_frame(other);assert(uses==1);
 // Already owned on a later rejoin: not given twice.
 checkpoint_disconnect(other);release(other);allocate(other,PlayerBytes);store(ev(other),0x164,100.f);allocations.at(other).commands=true;give_weapon(other,7,7);
 assert(logged("REJOIN player=2 left; kept their state (weapons=1)"));
 rejoin_frame(other);assert(uses==1);
 // P1 had played: their own state is kept and restored without a landing.
 // v0.3 (user log 141912): frozen at a level change, an older save record exists: their own state is kept, never the save's.
 {SavedPlayer old;old.identity="profile:host";playerBank={old};}store(ev(e),0x1a8,load<unsigned>(ev(e),0x1a8)|0x80u);
 give_weapon(e,6,6);checkpoint_disconnect(e);assert(departedPlayers.count("profile:host")&&departedPlayers["profile:host"].weapons.size()==1&&departedPlayers["profile:host"].hasPose&&logged("REJOIN player=1 left; kept their state (weapons=1)"));
 playerBank.clear();store(ev(e),0x1a8,load<unsigned>(ev(e),0x1a8)&~0x80u);
 release(e);store(globals,0,30.f);allocate(e,PlayerBytes);store(ev(e),0x164,100.f);allocations.at(e).commands=true;
 // A host-assigned profile is dropped on disconnect; the host binds it again.
 profileBindings[1]="profile:host";rejoin_frame(e);assert(departedPlayers.count("profile:host")==0&&(logged("REJOIN player=1 inventory restored")||logged("REJOIN player=1 restore incomplete")));
 // A checkpoint restore still pending keeps priority over a departure.
 loadingCheckpoint=true;restoredPlayers.clear();checkpoint_disconnect(other);assert(departedPlayers.empty());loadingCheckpoint=false;
 // A new map forgets the fan-outs; a new campaign forgets departures.
 departedPlayers["profile:x"].hasPose=true;rejoin_map_changed();assert(fanouts.empty()&&!departedPlayers["profile:x"].hasPose);rejoin_reset();profileBindings.clear();original[3]=nullptr;original[16]=nullptr;engine[14]=nullptr;

 // coop-fixes-007 smoother movers: each client once per campaign map.
 engine[41]=reinterpret_cast<void*>(&stuff_);engine[115]=reinterpret_cast<void*>(&listen);reset_players();netTuned.clear();
 net_tune(e);net_tune(e);net_tune(other);
 assert(stuffed.size()==2&&stuffed[0]=="1:cl_updaterate 100\nex_interp 0.01\n"&&stuffed[1]=="2:rate 100000\ncl_updaterate 100\ncl_cmdrate 100\nex_interp 0.06\n"&&logged("NET tuned player=1"));
 netTuned.clear();store(globals,0x0c,10);net_tune(e);assert(stuffed.size()==2);store(globals,0x0c,1);engine[41]=nullptr;
 std::puts("PASS story runtime: co-op dual wield command (slot guard, dead/other maps refused), weapon-hand pose and attack while dual wielding (no anim lock held, native animations kept), dual-wield save holds the weapon hand, shared ending flags across map changes and saves, per-player story wound/night vision kept once per map, checkpoint schema 4 and schema 3 compatibility; 007: credits spaced per player, rejoin restore and missed fan-out replay, client interpolation tuning");
}
