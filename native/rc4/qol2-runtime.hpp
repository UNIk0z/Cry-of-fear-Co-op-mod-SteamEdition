// Included in adapter.cpp after spray-runtime.hpp (v0.10: the rest of docs/COOP_QOL_PROPOSAL.md that Katz
// did not reject). Gameplay changes are host options, off by default (Create Server window, sc_setup.scr,
// or storycoop_server.cfg). Every feature works for a host alone. Formats and offsets: the decompile
// (cof-decompiled/src) and the disassembly of the pinned hl.dll.

// --- Options -------------------------------------------------------------------------
// Host (registered in every game; the Create Server window or storycoop_server.cfg sets them):
cvar_t huntedCvar{const_cast<char*>("sc_hunted"),const_cast<char*>("0"),0,0.f,nullptr};
cvar_t scaleCvar{const_cast<char*>("sc_scale"),const_cast<char*>("0"),0,0.f,nullptr};
cvar_t outlineCvar{const_cast<char*>("sc_outline"),const_cast<char*>("0"),0,0.f,nullptr};
cvar_t timerCvar{const_cast<char*>("sc_timer"),const_cast<char*>("0"),0,0.f,nullptr};
cvar_t norespawnCvar{const_cast<char*>("sc_norespawn"),const_cast<char*>("0"),0,0.f,nullptr};
// Each player's own game (storycoop_comfort.cfg, SETTINGS in the launcher); sent to the host with sc_mdl.
cvar_t noirCvar{const_cast<char*>("sc_noir"),const_cast<char*>("0"),0,0.f,nullptr};
cvar_t colourCvar{const_cast<char*>("sc_color"),const_cast<char*>("0"),0,0.f,nullptr};
void __cdecl qol2_register(){if(!engine[56])return;for(cvar_t* c:{&huntedCvar,&scaleCvar,&outlineCvar,&timerCvar,&norespawnCvar,&noirCvar,&colourCvar})eng<void(__cdecl*)(cvar_t*)>(56)(c);}
float cvar_value(const char* name){return engine[57]?eng<float(__cdecl*)(const char*)>(57)(name):0.f;}
struct HostOptions {int hunted=0,scale=0,outline=0,timer=0,norespawn=0,nosave=0,nohints=0;float next=-1;} hostOptions;
void host_options_frame(){
 if(!globals)return;const float now=load<float>(globals,0);if(hostOptions.next>=0&&now<hostOptions.next&&hostOptions.next-now<1.f)return;hostOptions.next=now+0.5f;
 hostOptions.hunted=cvar_value("sc_hunted")!=0;hostOptions.scale=cvar_value("sc_scale")!=0;hostOptions.outline=cvar_value("sc_outline")!=0;hostOptions.timer=static_cast<int>(cvar_value("sc_timer"));
 hostOptions.norespawn=cvar_value("sc_norespawn")!=0;hostOptions.nosave=cvar_value("sv_coop_nosave")!=0;hostOptions.nohints=cvar_value("sv_disablehints")!=0;
}
// sv_disablehints (the game's own "no hints" setting) also turns off the co-op key prompt and objective pop-ups.
bool hints_off(){return hostOptions.nohints!=0;}
// Each player's colour and black-and-white choice (sc_mdl words 4 and 5; older games send neither).
struct PlayerPrefs {int colour=0;bool noir=false;};
std::map<int,PlayerPrefs> playerPrefs;
void player_prefs_report(int index,const std::string& colour,const std::string& noir){
 if(index<1||index>32)return;auto& p=playerPrefs[index];
 if(colour.size()==1&&colour[0]>='0'&&colour[0]<='8')p.colour=colour[0]-'0';
 if(noir.size()==1&&(noir[0]=='0'||noir[0]=='1'))p.noir=noir[0]=='1';
}
// 0 automatic: by player slot. 1 red, 2 orange, 3 yellow, 4 green, 5 cyan, 6 blue, 7 purple, 8 white.
std::array<unsigned char,3> player_colour(int index){
 static const unsigned char table[9][3]={{0,0,0},{230,40,40},{240,140,30},{235,220,50},{60,210,70},{40,210,220},{50,100,240},{170,70,230},{230,230,230}};
 static const int slot[4]={1,6,4,3};auto it=playerPrefs.find(index);int c=it==playerPrefs.end()?0:it->second.colour;if(c<1||c>8)c=slot[(index-1)&3];
 return {table[c][0],table[c][1],table[c][2]};
}

// --- #19 Hardcore options ---------------------------------------------------------------
// No saves: sv_coop_nosave (the game's own co-op setting) also refuses the co-op tape recorder save
// (checkpoint_save). No respawn while down: the native respawn waits (MultiplayerKilled 100E9290..:
// m_flNextDeadNotifyTime +0x3D0, m_flCoopRespawnDelayTime +0x3D4 (Space after 60 s),
// m_flDeadAutoReviveTime +0x1F2C (sv_auto_respawn_time)) are pushed ahead while a player is down, so only
// a revive brings him back; with everyone down the game's own game over restarts the map. A fall
// respawn (v0.6) keeps its 2 s. Morphine trip time is the game's sv_morphinetriptime.
// Review 2026-11-06: only while a teammate is alive to revive him. The game ends the map when the last
// living player dies, not when he leaves: a player left alone and down gets the game's own respawn back.
bool hardcore_nosave(){return cvar_value("sv_coop_nosave")!=0;}
constexpr std::size_t PlayerDeadNotify=0x3d0,PlayerRespawnDelay=0x3d4;
void norespawn_frame(Edict* e){
 if(!hostOptions.norespawn||!scoped()||!globals)return;auto v=vars(e);void* p=nullptr;std::size_t n=0;if(!v||!player_memory(e,p,n))return;
 if(load<int>(v,0x174)==0&&load<float>(v,0x164)>0)return;
 auto fb=fallBacks.find(e);if(fb!=fallBacks.end()&&fb->second.serial==serial(e))return;
 bool reviver=false;for(auto o:connected_players())if(o!=e&&view_alive(o))reviver=true;if(!reviver)return;
 const float now=load<float>(globals,0);
 for(std::size_t at:{PlayerDeadNotify,PlayerRespawnDelay})if(load<float>(p,at)<now+30.f)store(p,at,now+60.f);
 const float revive=load<float>(p,PlayerAutoRevive);if(revive!=0.f&&revive<now+300.f)store(p,PlayerAutoRevive,now+600.f);
}

// --- #9 Harder with more players -----------------------------------------------------------
// Monster health x(1 + 0.35 per extra player), each monster once, when first seen (map monsters and
// those made later). Bosses and the hunted Simon keep theirs (boss bars read the boss's own health).
// One player: x1.0, nothing changes.
std::unordered_map<Edict*,int> scaledMonsters;float scaleNext=0;int scaleLogged=-1;
int party_size(){int n=0;for(auto e:connected_players())if(live(e))++n;return n<1?1:n;}
float scale_factor(int players){return 1.f+0.35f*static_cast<float>((players<1?1:players>4?4:players)-1);}
// Review 2026-11-06: a map state or a save brings monsters back with the health they had (scaled once
// already); state_activate marks them so a revisit or a load does not scale them again.
void scale_restored(Edict* e){if(live(e))scaledMonsters[e]=serial(e);}
void scale_frame(){
 if(!hostOptions.scale||!scoped()||!globals||!engine[72])return;const float now=load<float>(globals,0);if(now<scaleNext&&scaleNext-now<2.f)return;scaleNext=now+0.5f;
 const int players=party_size();const float f=scale_factor(players);if(f<=1.f)return;int count=0;
 const int max=load<int>(globals,0x94);
 for(int i=load<int>(globals,0x90)+1;i<max;++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);auto v=vars(e);if(!v)continue;
  if(!(load<unsigned>(v,0x1a8)&0x20u)||load<float>(v,0x164)<=0||load<float>(v,0x170)==0.f||load<int>(v,0x174)!=0)continue;   // FL_MONSTER, alive, takes damage
  auto& seen=scaledMonsters[e];if(seen==serial(e))continue;seen=serial(e);
  const char* cls=classname(e);if(std::strncmp(cls,"monster_",8)||!std::strcmp(cls,"monster_booksimonsledgehammer")||std::any_of(std::begin(BossClasses),std::end(BossClasses),[&](const char* b){return !std::strcmp(cls,b);}))continue;
  store(v,0x164,load<float>(v,0x164)*f);if(load<float>(v,0x1b4)>0)store(v,0x1b4,load<float>(v,0x1b4)*f);++count;
 }
 if(count&&scaleLogged!=players){scaleLogged=players;char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SCALE monster health x%.2f for %d players (%d monster(s) so far)\n",f,players,count);log(line);}
}

// --- #21 Dodge on/off kept --------------------------------------------------------------
// "dodgetoggle" flips m_bDodgeDisabled (+0x1EE9, hl.dll 10019F94); every new map built the player again
// with dodge on. The choice is kept per player (Steam ID, else slot) and put back when he is ready.
constexpr std::size_t PlayerDodgeDisabled=0x1ee9;
std::map<std::string,unsigned char> dodgeChoice;std::unordered_map<Edict*,int> dodgeApplied;
std::string pref_key(Edict* e){std::string k=steam_identity(e);return k.empty()?"slot"+std::to_string(eng<int(__cdecl*)(Edict*)>(71)(e)):k;}
void dodge_noted(Edict* e){void* p=nullptr;std::size_t n=0;if(player_memory(e,p,n)){dodgeChoice[pref_key(e)]=load<unsigned char>(p,PlayerDodgeDisabled);dodgeApplied[e]=serial(e);}}
void dodge_frame(Edict* e){
 auto& done=dodgeApplied[e];if(done==serial(e)||!client_ready(e))return;done=serial(e);
 auto it=dodgeChoice.find(pref_key(e));void* p=nullptr;std::size_t n=0;if(it==dodgeChoice.end()||!player_memory(e,p,n)||load<unsigned char>(p,PlayerDodgeDisabled)==it->second)return;
 store<unsigned char>(p,PlayerDodgeDisabled,it->second);char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DODGE player=%d kept %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),it->second?"off":"on");log(line);
}

// --- #22 Late joiners catch up ------------------------------------------------------------
// A player new to this game (no carried record) who joins after the others have moved on (they have
// records) gets the weapons the host carries now (the game's own cof_giveitems, at most what fits) and
// is moved beside the host if far. Once per player per game.
std::set<std::string> caughtUp;
void catch_up_frame(Edict* e){
 if(!scoped()||!globals||!client_ready(e)||!view_alive(e)||checkpoint_host(e)||in_scene(e)||!carry_settled(e))return;
 const auto key=pref_key(e);if(caughtUp.count(key))return;caughtUp.insert(key);
 // Review 2026-11-06: only a player who got here more than 15 s after the first (the objective's own
 // "joined late"); the players who start a game together get the story's weapons from the story.
 {auto late=lateObjectives.find(e);if(late==lateObjectives.end()||late->second.serial!=serial(e)||late->second.due<0)return;}
 if(carry_record(e)&&!carry_record(e)->weapons.empty())return;   // he had his own (rejoin, map change)
 Edict* host=eng<Edict*(__cdecl*)(int)>(72)(1);if(!checkpoint_host(host)||!view_alive(host))return;
 SavedPlayer have,want;if(!capture_player(e,have,false)||!capture_player(host,want,false)||want.weapons.empty())return;
 int given=0;const int limit=post_crash_map()?2:5;
 for(const auto& w:want.weapons){if(given>=limit)break;if(w.name.rfind("weapon_",0)||w.name=="weapon_dualweapon")continue;
  if(std::any_of(have.weapons.begin(),have.weapons.end(),[&](const SavedWeapon& s){return s.name==w.name;}))continue;if(helper_use("cof_giveitems","message",w.name,e))++given;}
 auto a=vars(host),b=vars(e);float d=0;for(int k=0;k<3;++k){const float x=load<float>(a,8+k*4)-load<float>(b,8+k*4);d+=x*x;}
 const bool moved=d>512.f*512.f&&move_beside(e,host);
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CATCHUP player=%d joined late: %d of the host's weapon(s) given%s\n",eng<int(__cdecl*)(Edict*)>(71)(e),given,moved?", moved beside the host":"");log(line);
}

// --- #8 Ping ------------------------------------------------------------------------------
// "sc_ping" (G): a glow at the spot the player looks at (TE_GLOWSPRITE with the game's own
// sprites/glow01.spr, precached on story maps) for 3 s, and "<name>: here (12 m)" to everyone, in the
// pinging player's colour. Once per 2 s; not while down or in a scene. TE_GLOWSPRITE bytes (engine CL_TENT.C
// 2479): life in 0.1 s, scale in 0.1, brightness. Story maps only: the sprite is precached there.
int pingSprite=0;std::unordered_map<Edict*,float> pingUsed;
void ping_precache(){pingSprite=engine[0]?eng<int(__cdecl*)(const char*)>(0)("sprites/glow01.spr"):0;}
std::array<float,3> eye_forward(void* v){
 const float pitch=load<float>(v,0x74)*0.01745329f,yaw=load<float>(v,0x78)*0.01745329f;
 return {std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch)};
}
bool ping_command(Edict* e){
 auto v=vars(e);if(!scoped()||!globals||!v||!engine[31]||!engine[46])return true;
 const float now=load<float>(globals,0);auto& used=pingUsed[e];if(used>now)used=0;if(used&&now-used<2.f)return true;
 if(!view_alive(e)||in_scene(e))return true;used=now;
 const std::array<float,3> eye{load<float>(v,8)+load<float>(v,0x178),load<float>(v,12)+load<float>(v,0x17c),load<float>(v,16)+load<float>(v,0x180)};const auto f=eye_forward(v);
 const std::array<float,3> end{eye[0]+f[0]*4096.f,eye[1]+f[1]*4096.f,eye[2]+f[2]*4096.f};
 Trace t{};eng<void(__cdecl*)(const float*,const float*,int,Edict*,Trace*)>(31)(eye.data(),end.data(),0,e,&t);
 if(t.fraction>=1.f||t.allSolid)return true;
 const std::array<float,3> at{t.end[0]-f[0]*8.f,t.end[1]-f[1]*8.f,t.end[2]-f[2]*8.f};
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);const auto c=player_colour(index);
 if(pingSprite>0){eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(2,23,nullptr,nullptr);eng<void(__cdecl*)(int)>(48)(23);for(float x:at)eng<void(__cdecl*)(float)>(53)(x);eng<void(__cdecl*)(int)>(50)(pingSprite);for(int b:{30,30,220})eng<void(__cdecl*)(int)>(48)(b);eng<void(__cdecl*)()>(47)();}
 const std::string name=player_name(e);const unsigned char colour[4]={c[0],c[1],c[2],255};
 for(auto o:connected_players()){auto q=vars(o);if(!q||!client_ready(o)||in_scene(o))continue;float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-at[k];d+=x*x;}
  char text[96]{};std::snprintf(text,sizeof(text),"%s: here (%d m)",name.c_str(),static_cast<int>(std::sqrt(d)/40.f+0.5f));hud_text(o,3,-1.f,0.32f,colour,2.f,text,0.f);}
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] PING player=%d at %.0f %.0f %.0f\n",index,at[0],at[1],at[2]);log(line);
 return true;
}

// --- #18 Scare ----------------------------------------------------------------------------
// "sc_scare" (J): one of the game's laughs (ambience/laugh1-3.wav, precached in every game; the game's
// own "laugh" command is for its developers only, client.cpp 1322) plays at the nearest teammate.
// Once per 20 s; not while down, in a scene or during the load wait. Alone: nothing to scare.
std::unordered_map<Edict*,float> scareUsed;
bool scare_command(Edict* e){
 auto v=vars(e);if(!scoped()||!globals||!v||!engine[29]||!engine[80])return true;auto say=[&](const char* t){eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,t);};
 const float now=load<float>(globals,0);auto& used=scareUsed[e];if(used>now)used=0;if(used&&now-used<20.f){say("Not yet.");return true;}
 if(!view_alive(e)||in_scene(e)||loadWait.armed)return true;
 Edict* target=nullptr;float best=0;
 for(auto o:connected_players()){if(o==e||!view_alive(o)||in_scene(o)||!client_ready(o))continue;auto q=vars(o);float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-load<float>(v,8+k*4);d+=x*x;}if(!target||d<best){target=o;best=d;}}
 if(!target){say("Nobody to scare.");return true;}
 used=now;static const char* const laughs[]={"ambience/laugh1.wav","ambience/laugh2.wav","ambience/laugh3.wav"};
 const int pick=engine[90]?eng<std::int32_t(__cdecl*)(std::int32_t,std::int32_t)>(90)(0,2):0;
 eng<void(__cdecl*)(Edict*,int,const char*,float,float,int,int)>(29)(target,0,laughs[pick<0||pick>2?0:pick],1.f,0.8f,0,100);
 say(("You scared "+player_name(target)+".").c_str());
 char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SCARE player=%d scared player=%d\n",eng<int(__cdecl*)(Edict*)>(71)(e),eng<int(__cdecl*)(Edict*)>(71)(target));log(line);
 return true;
}

// --- #17 Run timer and chapter times -------------------------------------------------------
// Server time on story maps, from a new game (or a chapter start), kept in a save (slotN.cofrun beside
// it). sc_timer 1: each chapter title card (the game's Chapter message) gets "Chapter ...: run
// 0:42:13 (last chapter 0:12:05)" 7 s later, and the credits the total. 2: also the run time at the top
// right, once a second. The game's own survival timer counts down and is not used.
struct RunTimer {double seconds=0,chapterStart=0;float last=-1,next=0,splitDue=-1;std::string title,shown;float titleAt=-100;} runTimer;
std::string run_clock(double s){const long t=static_cast<long>(s);char out[24]{};std::snprintf(out,sizeof(out),"%ld:%02ld:%02ld",t/3600,(t/60)%60,t%60);return out;}
void run_timer_set(double seconds){runTimer=RunTimer{};runTimer.seconds=runTimer.chapterStart=seconds<0?0:seconds;}
std::filesystem::path run_timer_file(int slot){const auto dir=checkpoint_directory();return dir.empty()?std::filesystem::path():dir/(L"slot"+std::to_wstring(slot)+L".cofrun");}
void run_timer_save(int slot){const auto f=run_timer_file(slot);if(f.empty())return;std::ofstream out(f,std::ios::binary|std::ios::trunc);if(out)out<<static_cast<long long>(runTimer.seconds);}
double run_timer_saved(int slot){const auto f=run_timer_file(slot);long long s=0;if(!f.empty()){std::ifstream in(f,std::ios::binary);if(in)in>>s;}return s>0&&s<10LL*24*3600?static_cast<double>(s):0.0;}
void run_timer_show(const std::string& text,float y,float hold){static const unsigned char grey[4]={210,210,210,255};for(auto e:connected_players())if(client_ready(e)&&!in_scene(e))hud_text(e,2,-1.f,y,grey,hold,text.c_str(),0.5f);}
void run_timer_frame(){
 if(!globals)return;const float now=load<float>(globals,0);
 if(!campaign_map()||connected_players().empty()){runTimer.last=-1;return;}
 if(runTimer.last>=0&&now>runTimer.last&&now-runTimer.last<1.f)runTimer.seconds+=now-runTimer.last;runTimer.last=now;
 if(runTimer.splitDue>=0&&now>=runTimer.splitDue){runTimer.splitDue=-1;if(hostOptions.timer>=1)run_timer_show(runTimer.shown,0.82f,6.f);}
 if(hostOptions.timer>=2&&(now>=runTimer.next||runTimer.next-now>2.f)){runTimer.next=now+1.f;static const unsigned char grey[4]={200,200,200,200};const auto text=run_clock(runTimer.seconds);
  for(auto e:connected_players())if(client_ready(e)&&!in_scene(e))hud_text(e,2,0.88f,0.03f,grey,1.5f,text.c_str(),0.f);}
}
// From the message hooks: the first string of a Chapter message (cof_chapter, effects.cpp 9234) or a Credits message.
void run_timer_title(const char* title){
 if(!title||!*title||!std::strcmp(title,"Game over")||!globals||!campaign_map())return;const float now=load<float>(globals,0);
 if(runTimer.title==title)return;runTimer.title=title;runTimer.titleAt=now;   // the same card again (game over restart): no new split
 const double chapter=runTimer.seconds-runTimer.chapterStart;runTimer.chapterStart=runTimer.seconds;
 runTimer.shown=std::string(title)+": run "+run_clock(runTimer.seconds)+(chapter>=1.0?" (last chapter "+run_clock(chapter)+")":"");runTimer.splitDue=now+7.f;
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] TIMER %s at %s\n",title,run_clock(runTimer.seconds).c_str());log(line);
}
void run_timer_credits(){if(!globals||!scoped())return;const float now=load<float>(globals,0);if(runTimer.title=="(credits)"&&now-runTimer.titleAt<60.f&&now>=runTimer.titleAt)return;runTimer.title="(credits)";runTimer.titleAt=now;runTimer.shown="Run time "+run_clock(runTimer.seconds);runTimer.splitDue=now+2.f;log(("[STORYCOOP_RC4] TIMER credits at "+run_clock(runTimer.seconds)+"\n").c_str());}

// --- #35 Black and white (each player's own choice) -------------------------------------------
// The game's BlackWhite message (player.cpp 9392; client hud_msg.cpp 882) for that player only: sent
// when he is ready on each map, and kept on when a scene's own message would turn it off. The byte is the
// strength (client gl_postprocess.cpp 45: grayscale = byte / 255; VidInit clears it each map): 255 = full.
std::unordered_map<Edict*,int> noirSent;
bool noir_for(Edict* e){if(!live(e))return false;auto it=playerPrefs.find(eng<int(__cdecl*)(Edict*)>(71)(e));return it!=playerPrefs.end()&&it->second.noir;}
void noir_frame(Edict* e){
 if(blackWhiteMessage<=0||!(scoped()||opening())||!client_ready(e)||!noir_for(e)||!engine[46])return;auto& sent=noirSent[e];if(sent==serial(e))return;sent=serial(e);
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,blackWhiteMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(255);eng<void(__cdecl*)()>(47)();
}

// --- Message hooks (adapter.cpp message_begin / write_byte / write_string) ----------------
struct Qol2Wire {int kind=0;Edict* to=nullptr;int strings=0;} qol2Wire;   // 1 BlackWhite to a noir player, 2 Chapter, 3 Credits
void qol2_wire_begin(int dest,int type,Edict* e){
 qol2Wire={};if(blackWhiteMessage>0&&type==blackWhiteMessage&&dest==1&&noir_for(e))qol2Wire.kind=1;
 else if(chapterMessage>0&&type==chapterMessage&&(dest==1||dest==2))qol2Wire.kind=2;else if(creditsMessage>0&&type==creditsMessage)qol2Wire.kind=3;
 qol2Wire.to=e;if(qol2Wire.kind==3)run_timer_credits();
}
int qol2_wire_byte(int value){return qol2Wire.kind==1?255:value;}
void qol2_wire_string(const char* s){if(qol2Wire.kind==2&&qol2Wire.strings++==0)run_timer_title(s);}

// --- #10 / #30 Teammate outline in each player's colour ---------------------------------------
// sc_outline: in the packet each player gets (AddToFullPack, adapter.cpp full_pack), a teammate more
// than 384 units away is drawn with the game's glow shell (renderfx 19) in his colour. Never while the
// receiver is held by a scene, never on a down or hidden player. Nothing changes for the others.
bool outline_state(void* state,Edict* e,Edict* host){
 if(!hostOptions.outline||!state||e==host||!scoped()||!live(e)||!live(host)||std::strcmp(classname(e),"player")||std::strcmp(classname(host),"player"))return false;
 auto a=vars(e),b=vars(host);float d=0;for(int k=0;k<3;++k){const float x=load<float>(a,8+k*4)-load<float>(b,8+k*4);d+=x*x;}
 if(d<384.f*384.f||!view_alive(e)||in_scene(host))return false;
 const auto c=player_colour(eng<int(__cdecl*)(Edict*)>(71)(e));
 store<int>(state,0x54,19);store<int>(state,0x4c,6);for(int k=0;k<3;++k)store<unsigned char>(state,0x50+k,c[static_cast<std::size_t>(k)]);return true;
}

// --- #16 Hunted: Book Simon stalks the team -------------------------------------------------
// sc_hunted (experimental, off by default). Not the Simon-mode boss (monster_booksimon sets the global
// Simon mode, which changes the co-op doors, plays its own music to everyone and starts a boss bar:
// CoF_SickSimon.cpp Spawn); the game's other Book Simon, monster_booksimonsledgehammer
// (CoF_BookSimonSledgehammer.cpp), has none of that: a slow walker with a sledgehammer and a flare.
// He comes about 2.5 min into a story map (and 4 min after he is gone), at a spot the team walked
// through at least 45 s before, 900+ units from everyone and out of sight, with 60 health. He goes when
// any player is held by a scene, after 3 min without coming within 700 units, and with the map.
// Never on the opening maps, during the load wait, with a boss alive or while a boss game over runs.
struct Hunt {Edict* who=nullptr;int serial=-1;float bornAt=-1,lastNear=-1,next=-1,nextCrumb=0;std::vector<std::pair<float,std::array<float,3>>> crumbs;bool precached=false;int spawned=0;} hunt;
const char* const HuntSounds[]={"weapons/sledgehammer/sledgehammer_hitbody.wav","weapons/flare/flare_burn.wav","weapons/flare/flare_ignite.wav"};
void hunt_precache(){
 hunt.precached=false;if(!hostOptions.hunted&&cvar_value("sc_hunted")==0)return;const auto dir=game_dir_path();std::error_code error;
 if(!engine[0]||!engine[1]||dir.empty()||!std::filesystem::is_regular_file(dir/L"models"/L"booksimon.mdl",error)){log("[STORYCOOP_RC4] HUNTED off on this map: models/booksimon.mdl is missing\n");return;}
 eng<int(__cdecl*)(const char*)>(0)("models/booksimon.mdl");
 for(const char* s:HuntSounds){std::error_code e2;if(std::filesystem::is_regular_file(dir/L"sound"/std::filesystem::path(s),e2))eng<int(__cdecl*)(const char*)>(1)(s);}
 hunt.precached=true;
}
// Review 2026-11-06: the map state (state_capture) keeps every live runtime monster; the hunter is not
// one of the map's, so a level change or a tape save never keeps him.
bool hunt_entity(Edict* e){auto v=e?vars(e):nullptr;return v&&!std::strcmp(classname(e),"monster_booksimonsledgehammer")&&!std::strcmp(targetname(e),"sc10_hunter");}
bool hunt_alive(){auto v=hunt.who?vars(hunt.who):nullptr;return v&&serial(hunt.who)==hunt.serial&&!std::strcmp(classname(hunt.who),"monster_booksimonsledgehammer")&&load<float>(v,0x164)>0&&load<int>(v,0x174)==0;}
void hunt_remove(const char* why){
 Edict* host=globals&&engine[72]?eng<Edict*(__cdecl*)(int)>(72)(1):nullptr;
 if(hunt.who&&serial(hunt.who)==hunt.serial&&live(hunt.who)&&!std::strcmp(targetname(hunt.who),"sc10_hunter")&&live(host))helper_use_pairs("trigger_relay",{{"killtarget","sc10_hunter"},{"triggerstate","2"}},host);
 hunt.who=nullptr;hunt.serial=-1;if(globals)hunt.next=load<float>(globals,0)+240.f;
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] HUNTED Book Simon gone (%s)\n",why);log(line);
}
bool hunt_visible(const std::array<float,3>& at,const std::vector<Edict*>& players){
 for(auto e:players){auto v=vars(e);const std::array<float,3> eye{load<float>(v,8),load<float>(v,12),load<float>(v,16)+28.f};Trace t{};const std::array<float,3> head{at[0],at[1],at[2]+30.f};
  eng<void(__cdecl*)(const float*,const float*,int,Edict*,Trace*)>(31)(eye.data(),head.data(),1,e,&t);if(t.fraction>=1.f)return true;}
 return false;
}
bool hunt_spawn(const std::array<float,3>& at,const std::vector<Edict*>& players){
 if(!engine[23]||!engine[67]||!original[1]||!original[6])return false;
 Edict* m=eng<Edict*(__cdecl*)(int)>(23)(eng<int(__cdecl*)(const char*)>(67)("monster_booksimonsledgehammer"));if(!live(m))return false;
 auto v0=vars(players.front());const float dx=load<float>(v0,8)-at[0],dy=load<float>(v0,12)-at[1];
 char origin[96]{},angles[48]{};std::snprintf(origin,sizeof(origin),"%.1f %.1f %.1f",at[0],at[1],at[2]-35.f);std::snprintf(angles,sizeof(angles),"0 %.1f 0",std::atan2(dy,dx)*57.29578f);
 for(const auto& [key,value]:std::vector<std::pair<const char*,const char*>>{{"targetname","sc10_hunter"},{"origin",origin},{"angles",angles},{"health","60"}}){KeyValue data{const_cast<char*>("monster_booksimonsledgehammer"),const_cast<char*>(key),const_cast<char*>(value),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(m,&data);}
 if(native<int(__cdecl*)(Edict*)>(1)(m)!=0||!live(m)){if(live(m)){assist_unlink(m);eng<void(__cdecl*)(Edict*)>(22)(m);}return false;}
 hunt.who=m;hunt.serial=serial(m);return true;
}
void hunt_frame(){
 if(!globals)return;const float now=load<float>(globals,0);
 if(!hostOptions.hunted||!scoped()||!hunt.precached){if(hunt.who)hunt_remove("hunted mode off");return;}
 const auto all=connected_players();std::vector<Edict*> players;bool scene=false;for(auto e:all){if(in_scene(e))scene=true;if(client_ready(e)&&view_alive(e))players.push_back(e);}
 if(now>=hunt.nextCrumb&&!players.empty()){hunt.nextCrumb=now+5.f;for(auto e:players){auto v=vars(e);if(load<unsigned>(v,0x1a8)&0x200u)hunt.crumbs.push_back({now,{load<float>(v,8),load<float>(v,12),load<float>(v,16)}});}   // FL_ONGROUND
  if(hunt.crumbs.size()>240)hunt.crumbs.erase(hunt.crumbs.begin(),hunt.crumbs.begin()+static_cast<std::ptrdiff_t>(hunt.crumbs.size()-240));}
 if(hunt.who){
  if(!hunt_alive()){hunt.who=nullptr;hunt.serial=-1;hunt.next=now+240.f;log("[STORYCOOP_RC4] HUNTED Book Simon is down; he comes back later\n");return;}
  if(scene||loadWait.armed||bossOver){hunt_remove(scene?"a scene started":"the map is busy");return;}
  auto v=vars(hunt.who);for(auto e:players){auto q=vars(e);float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-load<float>(v,8+k*4);d+=x*x;}if(d<700.f*700.f)hunt.lastNear=now;}
  if(now-hunt.lastNear>180.f)hunt_remove("he never caught up");
  return;
 }
 if(hunt.next<0)hunt.next=now+150.f;
 if(now<hunt.next||players.empty()||scene||loadWait.armed||bossOver||boss_alive())return;
 hunt.next=now+30.f;   // try again soon if no spot fits
 for(auto it=hunt.crumbs.rbegin();it!=hunt.crumbs.rend();++it){
  if(now-it->first<45.f)continue;const auto& at=it->second;bool away=true;
  for(auto e:players){auto q=vars(e);float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-at[k];d+=x*x;}if(d<900.f*900.f)away=false;}
  if(!away||!landing_clear(at,at,nullptr)||hunt_visible(at,players))continue;
  if(!hunt_spawn(at,players))return;
  hunt.bornAt=hunt.lastNear=now;++hunt.spawned;
  if(proFontMessage>0)for(auto e:players){eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,proFontMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(1);eng<void(__cdecl*)(const char*)>(54)("Something is following you.");eng<void(__cdecl*)()>(47)();}
  char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] HUNTED Book Simon appears at %.0f %.0f %.0f (%d this map)\n",at[0],at[1],at[2],hunt.spawned);log(line);return;
 }
}

// --- Wiring ----------------------------------------------------------------------------
bool qol2_client_command(Edict* e,const char* command){
 if(!command)return false;
 if(!std::strcmp(command,"sc_ping"))return ping_command(e);
 if(!std::strcmp(command,"sc_scare"))return scare_command(e);
 return false;
}
void qol2_after_command(Edict* e,const char* command){if(command&&!std::strcmp(command,"dodgetoggle"))dodge_noted(e);}
void qol2_world_spawned(){if(!scoped())return;ping_precache();hunt_precache();}
void qol2_frame(){host_options_frame();scale_frame();run_timer_frame();hunt_frame();}
void qol2_player_frame(Edict* e){if(!live(e)||std::strcmp(classname(e),"player"))return;norespawn_frame(e);if(!scoped())return;dodge_frame(e);catch_up_frame(e);noir_frame(e);}
void qol2_map_changed(){pingSprite=0;scaledMonsters.clear();scaleNext=0;scaleLogged=-1;dodgeApplied.clear();noirSent.clear();pingUsed.clear();scareUsed.clear();hunt=Hunt{};hostOptions.next=-1;runTimer.last=-1;runTimer.splitDue=-1;runTimer.next=0;}
void qol2_disconnect(Edict* e){dodgeApplied.erase(e);noirSent.erase(e);pingUsed.erase(e);scareUsed.erase(e);if(live(e))playerPrefs.erase(eng<int(__cdecl*)(Edict*)>(71)(e));}
void qol2_new_game(){caughtUp.clear();}
