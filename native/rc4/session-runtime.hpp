// Included in adapter.cpp after checkpoint-runtime.hpp (coop-fixes-005).
// User requests 2026-10-12: only the phone and the lantern give light in
// co-op, cutscenes show no light, players look like Simon, the pause holds
// while either player has the ESC menu open, and the real main menu hosts the
// story co-op. Public engine/game interfaces and pinned data reads only.

// --- Lights -------------------------------------------------------------------
// CoF keeps every player's light flags in pev->frags (+0x168): UpdateClientData
// (100EDB6E) copies FlashFlags there whenever FlashFlags changes and broadcasts
// ScoreInfo, and the multiplayer "flashlight" command (1001C363) toggles frags
// between 0 and 32 (the police flashlight, bit 0x20). Client 10065260 draws a
// lit spot for EVERY player whose frags has bit 1, 8 or 0x20, aimed from that
// player's eyes, and unlike the spotlight passes it is not suppressed by a
// camera view. The user's "flashlight in cutscenes" with spot=0 in the log was
// that police flashlight. In co-op: the "flashlight" command is refused, the
// Maglite (+0x1EA8 / bit 8) and the Glock light (bit 4) were switched off until
// coop-fixes-012 (now allowed, user request), and a stray frags bit 0x20 is cleared by forcing UpdateClientData to rewrite
// frags from FlashFlags (+0x5D4 = -1). Phone (bits 1|2) and lantern untouched.
std::unordered_map<Edict*,std::pair<int,unsigned>> lightNotified,lightSeen;
int lightLines=0;bool openingLightLogged=false;
void light_notify(Edict* e,unsigned what,const char* text,const char* line){
 auto& n=lightNotified[e];if(n.first!=serial(e))n={serial(e),0u};if(n.second&what)return;n.second|=what;
 if(engine[80]&&live(e))eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);log(line);
}
// coop-fixes-011: the user saw these centre texts cut off; two short lines now.
void light_reset(){lightNotified.clear();lightSeen.clear();lightLines=0;openingLightLogged=false;}
bool light_command(Edict* e,const char* command){
 if(!scoped()||!command||std::strcmp(command,"flashlight"))return false;
 light_notify(e,4u,"No F flashlight in co-op.\nUse your in-game lights.\n","[STORYCOOP_RC4] LIGHT police flashlight refused (F key); other lights allowed\n");return true;
}
void light_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;if(!scoped()||!player_memory(e,p,n))return;auto v=vars(e);if(!v)return;
 // coop-fixes-012 (user request): the Maglite and the Glock light work again;
 // only the multiplayer police flashlight (the F key's "flashlight" command and
 // its frags bit 0x20) stays off.
 const float frags=load<float>(v,0x168);
 // FlashFlags itself never carries 0x20 natively; clear it too so the forced
 // rewrite happens once, never every frame.
 if(std::isfinite(frags)&&frags>=0&&frags<4096&&(static_cast<int>(frags)&0x20)){store(p,0x5d0,load<unsigned>(p,0x5d0)&~0x20u);store(p,0x5d4,-1);}
 // Evidence for the P2 phone report: each player's flags and server view.
 const unsigned flags=load<unsigned>(p,0x5d0)&0x3fu;auto& seen=lightSeen[e];const bool first=seen.first!=serial(e);
 if(first||seen.second!=flags){seen={serial(e),flags};if((!first||flags)&&lightLines<200){++lightLines;char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LIGHT player=%d flags=%x phone=%d view=%d\n",eng<int(__cdecl*)(Edict*)>(71)(e),flags,load<int>(p,0x1e98),load<int>(p,0x2038));log(line);}}
}

// --- Simon player models --------------------------------------------------------
// Player models come from each client's userinfo "model" (both configs say
// police1). models/player.mdl (Simon) has the same 32 bones and 85 sequences
// as the police models, so the installer copies it to
// models/player/simon/simon.mdl and the server sets "model" to simon through
// the public SetClientKeyValue. Only when that file exists and only in co-op.
// coop-fixes-007: the game's own ClientUserInfoChanged (100C79E0) forces every
// player's "model" back to the world's per-player model (world +0xE0..+0xEC,
// string_t, set by cof_customiseplayer; empty = police1..police4). Setting
// "simon" every frame therefore looped: each change made the engine send a
// full player-info update to everyone once a second, and those piled up in a
// loading client's reliable buffer until "reliable overflow" kicked P2 when
// joining s4_start (both users' 006 logs). The world fields now name simon
// (only empty ones: a map's own models win), so the game itself keeps it;
// each player's userinfo is set once per map, never fought over.
// coop-fixes-008: the 007 test (both PCs still police, no MODEL player line)
// showed the one-shot check could pass silently. Each player is now checked
// once a second: "simon" already -> logged once; a map's own model -> left
// alone (logged once); otherwise the empty world field is refilled and the
// userinfo set, at most 3 times per player per map so it can never flood.
// coop-fixes-009: the 008 test still showed police officers on the passive
// opening (cof_campaign_story4, s4_nightmare: the "camera level"), which
// scoped() leaves out. Neither step needs the player's private memory (the
// world object and the public userinfo only), so Simon applies there too.
bool model_map(){return scoped()||opening();}
// coop-fixes-011: the 010 test still showed police on s4_nightmare. Its
// worldspawn names "player1".."player4" = police1..police4 (the game's own
// co-op default, the same as an empty field), and those were kept as "the
// map's own model". The police defaults are now replaced like empty fields;
// any other model a map names still wins.
bool default_player_model(int named){
 if(!named)return true;const char* m=string_at(named);
 return m&&std::strlen(m)==7&&_strnicmp(m,"police",6)==0&&m[6]>='1'&&m[6]<='4';
}
struct ModelState{float next=0;int attempts=0;unsigned logged=0;};
bool simonModel=false;int simonString=0;std::map<int,ModelState> modelStates;
constexpr std::uint32_t WorldPointerRva=0x223190;   // hl.dll 10223190: the CWorld (10120EFC)
constexpr std::size_t WorldPlayerModels=0xe0;
void* world_memory(){
 if(!backend||!engine[64]||!engine[72])return nullptr;auto world=eng<Edict*(__cdecl*)(int)>(72)(0);if(!world)return nullptr;
 void* memory=eng<void*(__cdecl*)(Edict*)>(64)(world);
 void* pinned=*reinterpret_cast<void* const*>(reinterpret_cast<const unsigned char*>(backend)+WorldPointerRva);
 return memory&&memory==pinned?memory:nullptr;
}
int world_models_simon(){
 if(!simonModel||!model_map()||!globals||load<int>(globals,0x90)<2||!engine[67])return 0;void* w=world_memory();if(!w)return -1;
 if(!simonString)simonString=eng<int(__cdecl*)(const char*)>(67)("simon");int set=0;
 for(std::size_t i=0;i<4;++i)if(default_player_model(load<int>(w,WorldPlayerModels+i*4))){store<int>(w,WorldPlayerModels+i*4,simonString);++set;}
 return set;
}
void model_activate(){
 modelStates.clear();simonModel=false;simonString=0;char dir[MAX_PATH]{};if(!engine[99])return;eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return;
 std::error_code error;simonModel=std::filesystem::is_regular_file(std::filesystem::path(dir)/L"models"/L"player"/L"simon"/L"simon.mdl",error);
 const int set=world_models_simon();
 if(set<0)log("[STORYCOOP_RC4] MODEL world object not recognized; players keep the map's models\n");
 else if(set>0){char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MODEL world player models -> simon (%d of 4)\n",set);log(line);}
}
void model_line(ModelState& s,unsigned what,const char* format,int index,const char* text){
 if(s.logged&what)return;s.logged|=what;char line[160]{};std::snprintf(line,sizeof(line),format,index,text);log(line);
}
void model_frame(Edict* e){
 if(!simonModel||!simonString||!model_map()||!globals||!live(e)||!engine[106]||!engine[107]||!engine[109]||std::strcmp(classname(e),"player"))return;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);if(index<1||index>4)return;
 auto& s=modelStates[index];const float now=load<float>(globals,0);if(now<s.next)return;s.next=now+1.f;
 void* w=world_memory();if(!w){model_line(s,1u,"[STORYCOOP_RC4] MODEL player=%d world object not recognized%s\n",index,"");return;}
 const std::size_t field=WorldPlayerModels+static_cast<std::size_t>(index-1)*4;int named=load<int>(w,field);
 if(named&&named!=simonString&&!default_player_model(named)){model_line(s,2u,"[STORYCOOP_RC4] MODEL player=%d keeps the map's model %s\n",index,string_at(named));return;}
 if(named!=simonString)store<int>(w,field,simonString);
 auto buffer=eng<char*(__cdecl*)(Edict*)>(106)(e);if(!buffer)return;
 const char* value=eng<char*(__cdecl*)(char*,char*)>(107)(buffer,const_cast<char*>("model"));
 if(value&&std::strcmp(value,"simon")==0){model_line(s,4u,"[STORYCOOP_RC4] MODEL player=%d is simon%s\n",index,named==simonString?"":" (world field refilled)");return;}
 // InfoKeyValue returns the engine's shared buffer: keep a copy for the log.
 const std::string previous=value&&*value?std::string(value).substr(0,32):"-";
 if(s.attempts>=3){model_line(s,8u,"[STORYCOOP_RC4] MODEL player=%d still %s after 3 tries; left alone this map\n",index,previous.c_str());return;}
 ++s.attempts;eng<void(__cdecl*)(int,char*,char*,char*)>(109)(index,buffer,const_cast<char*>("model"),const_cast<char*>("simon"));
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MODEL player=%d now simon (was %s, try %d)\n",index,previous.c_str(),s.attempts);log(line);
}

// --- Pause while ANY player has the ESC menu open --------------------------------
// GameUI (CGameUI::ActivateGameUI/HideGameUI, GameUI.dll 10031463/10031509)
// sends "setpause"/"unpause" for every client; the engine (hw.dll 01D859C1/
// 01D85A25) applies whichever arrives last, so P2 closing the menu resumed the
// game while P1 was still in it. The host's own menu state is the public
// IGameUI::IsGameUIActive (GameUI007, slot 11) and the server's pause is the
// engine's sv.paused (pinned hw.dll data 025203A4). A pause/unpause right after
// the host's menu opened/closed is the host's; any other one is the partner's.
// While either menu is open and the game runs, the host re-issues "setpause".
// A partner who opens the menu while the game is already paused is invisible
// to the server (documented limitation).
// quietUntil: no re-pause right after a map starts (loading screens activate
// GameUI too); the host's own setpause gets 250 ms before the guard helps.
struct PauseState {bool othersOpen=false,lastPaused=false,lastHost=false,selfPending=false;unsigned long long hostOpenAt=0,hostCloseAt=0,selfAt=0,nextRepause=0,quietUntil=0;};
PauseState pauseState;
// Returns true when "setpause" must be issued now.
bool pause_step(PauseState& s,bool paused,bool host,unsigned long long now){
 if(host&&!s.lastHost)s.hostOpenAt=now;if(!host&&s.lastHost)s.hostCloseAt=now;s.lastHost=host;
 if(paused&&!s.lastPaused){if(s.selfPending&&now-s.selfAt<3000)s.selfPending=false;else if(!(host&&now-s.hostOpenAt<1500))s.othersOpen=true;}
 if(!paused&&s.lastPaused){if(!(!host&&now-s.hostCloseAt<1500))s.othersOpen=false;}
 s.lastPaused=paused;
 if(s.selfPending&&now-s.selfAt>=3000)s.selfPending=false;
 if(!paused&&((host&&now-s.hostOpenAt>=250)||s.othersOpen)&&now>=s.nextRepause&&now>=s.quietUntil){s.nextRepause=now+1000;s.selfPending=true;s.selfAt=now;return true;}
 return false;
}
struct PauseProbe {int tried=0;bool ready=false;const volatile int* paused=nullptr;void* ui=nullptr;unsigned long long lastPoll=0;};
PauseProbe pauseProbe;
bool module_pinned(HMODULE module,const char* pin){wchar_t path[MAX_PATH]{};return module&&GetModuleFileNameW(module,path,MAX_PATH)&&sha_file(path)==pin;}
void pause_probe(){
 if(pauseProbe.ready||pauseProbe.tried>=600)return;++pauseProbe.tried;
 HMODULE hw=GetModuleHandleW(L"hw.dll"),ui=GetModuleHandleW(L"GameUI.dll");if(!hw||!ui)return;
 pauseProbe.tried=600;
 if(!module_pinned(hw,"9dd34e536c4bb7cda3bc1bc4f0f5f6163687566a16f35c0ea0be59f93da62875")||!module_pinned(ui,"8255926f84eaa08e9096f2f78dea7f2827c3733e7af8f4408efa0b423acca838")){log("[STORYCOOP_RC4] PAUSE guard off: engine or GameUI build differs\n");return;}
 auto create=reinterpret_cast<void*(__cdecl*)(const char*,int*)>(GetProcAddress(ui,"CreateInterface"));void* gameui=create?create("GameUI007",nullptr):nullptr;
 if(!gameui){log("[STORYCOOP_RC4] PAUSE guard off: GameUI007 unavailable\n");return;}
 pauseProbe.paused=reinterpret_cast<const volatile int*>(reinterpret_cast<const unsigned char*>(hw)+(0x025203A4-0x01D00000));pauseProbe.ui=gameui;pauseProbe.ready=true;
 log("[STORYCOOP_RC4] PAUSE guard ready (host menu via GameUI007, server pause via pinned engine)\n");
}
bool campaign_map(){const char* name=map_name();return verified&&name&&(std::strncmp(name,"s4_",3)==0||std::strncmp(name,"s2_",3)==0||std::strcmp(name,"cof_campaign_story4")==0);}
// v0.5 (every solo playtest log): alone in the stuck lobby, testers typed
// "changelevel c_intro / c_start / c_nightmare", the single-player maps. Those
// have no cof_coop_spawn1, so the host lands on info_player_start (decomp
// player.cpp EntSelectSpawnPoint), on c_start inside the "block" wall: a black
// box, and with the eye in solid the renderer draws no world (gl_renderer.cpp
// DrawBump). In a co-op game (2+ slots) such a map is swapped at once for its
// co-op version: c_X -> s4_X, else s4_X_e0; c_intro -> the co-op opening.
std::string coop_version_of(const char* map){
 if(!map||std::strncmp(map,"c_",2)||!engine[110])return {};
 auto valid=[](const std::string& m){return eng<int(__cdecl*)(char*)>(110)(const_cast<char*>(m.c_str()))!=0;};
 if(!std::strcmp(map,"c_intro"))return valid("cof_campaign_story4")?"cof_campaign_story4":"";
 for(const std::string m:{std::string("s4_")+(map+2),std::string("s4_")+(map+2)+"_e0"})if(m.size()<32&&valid(m))return m;
 return {};
}
void coop_version_redirect(){
 if(!verified||!globals||load<int>(globals,0x90)<2||!engine[39])return;
 const auto target=coop_version_of(map_name());if(target.empty())return;
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MAP %s is the single-player map; co-op continues on %s\n",map_name(),target.c_str());log(line);
 eng<void(__cdecl*)(const char*)>(39)(("changelevel "+target+"\n").c_str());
}
// coop-fixes-009 (user request): "*paused*" on every screen while the game is
// paused, so the player outside the menu knows why everything froze. The
// server still sends client messages while paused (only physics stops), and
// this poll also runs from SetupVisibility; the centre text is re-sent every
// second so it stays up for the whole pause and is cleared on resume.
struct PauseNotice {bool shown=false;unsigned long long at=0;};
PauseNotice pauseNotice;
// Returns the text to send now (nullptr: nothing).
const char* pause_notice_step(PauseNotice& n,bool paused,unsigned long long now){
 if(paused){if(n.shown&&now-n.at<1000)return nullptr;n.shown=true;n.at=now;return "*paused*";}
 if(!n.shown)return nullptr;n.shown=false;return "";
}
void pause_notice(bool paused,unsigned long long now){
 const char* text=pause_notice_step(pauseNotice,paused,now);if(!text||!engine[80]||!engine[72])return;
 for(int i=1;i<=load<int>(globals,0x90);++i){Edict* c=eng<Edict*(__cdecl*)(int)>(72)(i);if(live(c)&&!std::strcmp(classname(c),"player"))eng<void(__cdecl*)(Edict*,int,const char*)>(80)(c,1,text);}
 if(*text)return;log("[STORYCOOP_RC4] PAUSE resumed; *paused* cleared\n");
}
void pause_reset(){pauseState={};pauseNotice={};}
void pause_activate(){pauseState={};pauseNotice={};pauseState.quietUntil=GetTickCount64()+5000;}
void pause_poll(){
 if(!globals||load<int>(globals,0x90)<2||!campaign_map())return;
 const unsigned long long now=GetTickCount64();if(now-pauseProbe.lastPoll<5)return;pauseProbe.lastPoll=now;
 pause_probe();if(!pauseProbe.ready)return;
 auto active=reinterpret_cast<bool(__thiscall*)(void*)>((*reinterpret_cast<void***>(pauseProbe.ui))[11]);
 const bool host=active(pauseProbe.ui),paused=*pauseProbe.paused!=0,others=pauseState.othersOpen;
 if(paused&&!pauseNotice.shown)log("[STORYCOOP_RC4] PAUSE *paused* shown to every player\n");
 pause_notice(paused,now);
 const bool repause=pause_step(pauseState,paused,host,now);
 if(others!=pauseState.othersOpen){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] PAUSE partner menu %s\n",pauseState.othersOpen?"open":"closed");log(line);}
 if(!repause)return;
 if(!engine[39]||!engine[57]||eng<float(__cdecl*)(const char*)>(57)("pausable")<=0)return;
 eng<void(__cdecl*)(const char*)>(39)("setpause\n");
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] PAUSE kept: %s still in the menu\n",host?"host":"partner");log(line);
}

// --- Main menu: Custom Campaign entries "Simon's Story" ----------------------------
// Client 1002E340 lists maps/*.custom in the main menu's Custom Campaign page;
// a choice opens the native difficulty page, which sends "campaign <firstmap>"
// and "skillset N" (server 10019C91/10019DA9: skill, nightmare, white fade) and
// five seconds later CLIENT_COMMAND("map <firstmap>") (100DFC10). For the story
// co-op entries that final command is replaced by a fresh two-player listen
// server: a new campaign, or a saved slot.
// coop-fixes-012 (user request): "New Game" and "Load Slot 1..3" (the host
// picks the slot). The main menu's own buttons are images and actions inside
// client.dll (Host/Join Server call GameUI through the engine's "optionshack",
// hw.dll 01D21AF5) and stay as they are. The old "Continue" (newest slot) id
// still works for an older entry file.
int menuMode=0;
// 1 new game, 2 newest slot (older entry), 11..13 slot 1..3.
int menu_mode(const char* id){
 if(!id)return 0;if(!std::strcmp(id,"sc4coop_new"))return 1;if(!std::strcmp(id,"sc4coop_continue"))return 2;
 return !std::strncmp(id,"sc4coop_slot",12)&&id[12]>='1'&&id[12]<='3'&&!id[13]?10+(id[12]-'0'):0;
}
bool menu_command(const char* command){
 if(!command||std::strcmp(command,"campaign"))return false;menuMode=menu_mode(eng<const char*(__cdecl*)(int)>(83)(1));
 if(menuMode==1)log("[STORYCOOP_RC4] MENU host new co-op game selected; difficulty next\n");
 else if(menuMode){char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MENU load co-op %s selected; difficulty next\n",menuMode==2?"newest save":menuMode==11?"slot 1":menuMode==12?"slot 2":"slot 3");log(line);}
 return false;
}
bool menu_slot_usable(const std::filesystem::path& dir,int slot,Checkpoint& value,bool& backup,std::filesystem::file_time_type& time){
 if(dir.empty()||!read_checkpoint(dir,slot,value,backup))return false;
 std::error_code error;auto file=dir/(L"slot"+std::to_wstring(slot)+L".cofsave");if(backup)file+=L".bak";time=std::filesystem::last_write_time(file,error);if(error)return false;
 std::error_code mapError;return std::filesystem::is_regular_file(std::filesystem::path(dir).parent_path()/L"maps"/(std::wstring(value.map.begin(),value.map.end())+L".bsp"),mapError);
}
std::string host_start_command(const SessionOptions& o,int difficulty,const std::string& map){
 std::string s="disconnect;maxplayers "+std::to_string(o.maxPlayers<2?2:o.maxPlayers>4?4:o.maxPlayers)+";mp_footsteps 1;sv_maxupdaterate 100;sv_lan 0;";   // v0.6.2: joins over the internet/VPN ("LAN servers are restricted to local clients (class C)")
 if(difficulty>=1&&difficulty<=4)s+="difficulty "+std::to_string(difficulty)+";";return s+"map "+map+"\n";
}
std::string menu_host_command(int mode,std::string& notice){
 // v0.5: the server takes the playthrough's player count (setup, or the save's)
 // and a load its own difficulty (D1), never the one the menu page just chose.
 constexpr const char* back="disconnect;maxplayers 1;deathmatch 0;map c_game_menu1\n";
 if(mode==1){reset();checkpointWorldStarted=false;checkpointPrepared=false;sessionOptions=SessionOptions{};sessionOptions.have=true;return host_start_command(sessionOptions,0,"cof_campaign_story4");}
 const auto dir=checkpoint_directory();int best=0;std::filesystem::file_time_type newest{};Checkpoint chosen;bool chosenBackup=false;
 for(int slot=1;slot<=3;++slot){if(mode>10&&slot!=mode-10)continue;Checkpoint value;bool backup=false;std::filesystem::file_time_type time{};if(!menu_slot_usable(dir,slot,value,backup,time))continue;
  if(!best||time>newest){best=slot;newest=time;chosen=std::move(value);chosenBackup=backup;}}
 if(!best){notice=mode>10?"Save slot "+std::to_string(mode-10)+" is empty.\n":std::string("No co-op save was found. Use a tape recorder in a co-op game first.\n");return back;}
 reset();checkpointWorldStarted=false;const std::string map=chosen.map;const int difficulty=chosen.difficulty;if(chosen.options.have)sessionOptions=chosen.options;
 loadedCheckpoint=std::move(chosen);checkpointSlot=best;checkpointPrepared=true;checkpointPreparedBackup=chosenBackup;
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP] PREPARE accepted from the main menu: slot%d map=%s%s difficulty=%s\n",best,map.c_str(),chosenBackup?" (backup)":"",difficulty?std::to_string(difficulty).c_str():"menu choice (older save)");log(line);
 return host_start_command(sessionOptions,difficulty,map);
}
// --- Host Simon's Story in the game's Create Server window (v0.5.1, Katz 2026-10-23) ---
// "Host Simon's Story" opens the game's own Create Server window (client-menu.hpp).
// Its map list shows maps/*-*bsp only (GameUI patch): the files Install makes
// for the story (a copy of the game's small c_difficulty_settings.bsp each),
// one per chapter start and one per save slot. The window's options come from
// cryoffear/sc_setup.scr (players 2-4, Player collision, Difficulty). When the
// server starts on such an entry the adapter restarts it on the real map with
// those options, exactly as the old menu entries did.
// --- The lost bag: three slots from Chapter 6 on (v0.5.3) ---------------------------
// Katz 2026-10-23: "in the chapter 6 simon loses his bag and so all players must
// have the newly reduced inventory with 3 slots in chapter 6 and the next
// chapters". The game does it once, in c_bridge: cof_clearitems "bagremove"
// (spawnflags 5: clear the activator's items, then m_bThreeSlots, decomp
// effects.cpp CCofClearItems::Use), for the activator only, and a co-op map
// change builds every player again with six slots. Now:
//  - bagremove also clears every other player (bag_lookup, the next frame);
//  - on every map after the crash (c_bridge and all it leads to, found from the
//    maps' level changes; none is reachable from c_start without c_bridge) each
//    player gets three slots as soon as their HUD is ready, through the game's
//    own path: a cof_clearitems with spawnflags 4 only (no clearing) sets
//    m_bThreeSlots (player+0x1E9C, save table 101D9D98) and sends slots 1-3.
const char* const PostCrashMaps[]={"c_bridge","c_forest1","c_forest2","c_forest3","c_forest4","c_forest5","c_forest6","c_forest7","c_forestday",
 "c_village1","c_village2","c_village3","c_asylum1","c_asylum1dark","c_asylum1day","c_asylum2","c_asylum2dark","c_asylumb","c_attic","c_lake","c_fsewer1",
 "c_ending1","c_ending1b","c_ending1c","c_ending2","c_ending2b","c_ending2c","c_ending3","c_ending3b","c_ending3c","c_ending4","c_ending4b","c_ending5","c_ending5b","c_ending5c"};
constexpr std::size_t ThreeSlotsOffset=0x1e9c;
// s4_attic_e0 / s2_forest3_e1 -> c_attic / c_forest3; c_* names as they are.
bool post_crash_map(){
 std::string m=globals?map_name():"";if(m.rfind("s4_",0)==0||m.rfind("s2_",0)==0)m="c_"+m.substr(3);
 const auto e=m.rfind("_e");if(e!=std::string::npos&&e+2<m.size()&&std::all_of(m.begin()+e+2,m.end(),[](char c){return c>='0'&&c<='9';}))m.resize(e);
 return std::any_of(std::begin(PostCrashMaps),std::end(PostCrashMaps),[&](const char* p){return m==p;});
}
void three_slots_apply(Edict* e){
 void* p=nullptr;std::size_t n=0;if(!scoped()||!post_crash_map()||!client_ready(e)||!player_memory(e,p,n)||load<unsigned char>(p,ThreeSlotsOffset))return;
 const bool ok=helper_use_pairs("cof_clearitems",{{"spawnflags","4"}},e)&&load<unsigned char>(p,ThreeSlotsOffset);
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BAG player=%d %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),ok?"has three slots (bag lost in the train crash)":"three slots FAILED");log(line);
}
// bagremove is fired by the map's own managers (FireTargets, not DispatchUse);
// the lookup that finds it marks the map.
// v0.5.7 (playtest 2: "after losing the bag the player inventory must be wiped,
// right now it retains all the inventory"; log: "BAG bagremove: every player
// lost the bag (0 player(s))" right after s4_bridge_e1 started): bagremove fires
// as the map starts, before any HUD is ready, and the inventory carried from the
// last map was given back afterwards. Now every player on that map, the
// activator too, is cleared once their HUD is ready AND their carried inventory
// has been applied (carry_settled), with the game's own cof_clearitems
// (spawnflags 5: clear, then three slots). A player who joins later on that
// map is cleared the same way.
bool carry_settled(Edict* e);   // coop-runtime.hpp
void bag_wiped(Edict* e);       // coop-runtime.hpp
struct BagLoss {bool active=false;std::string map;std::set<std::pair<Edict*,int>> wiped;};
BagLoss bagLoss;
void bag_lookup(Edict* start,const char* key,const char* value,Edict* found){
 if(start||!key||!value||std::strcmp(key,"targetname")||!live(found)||!scoped()||std::strcmp(classname(found),"cof_clearitems"))return;
 auto v=vars(found);const unsigned flags=v?load<unsigned>(v,0x1a4):0;if((flags&5u)!=5u||(flags&2u))return;   // clear + three slots, activator only
 if(!bagLoss.active||bagLoss.map!=map_name()){bagLoss={true,map_name(),{}};char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BAG %s fired: every player loses the bag on this map\n",targetname(found));log(line);}
}
void three_slots_frame(){
 if(!globals||!verified)return;
 if(bagLoss.active&&bagLoss.map!=map_name())bagLoss={};
 if(bagLoss.active)for(auto e:connected_players()){
  if(!client_ready(e)||!carry_settled(e)||!bagLoss.wiped.insert({e,serial(e)}).second)continue;
  const bool ok=helper_use_pairs("cof_clearitems",{{"spawnflags","5"}},e);bag_wiped(e);
  char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BAG player=%d %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),ok?"inventory cleared (bag lost)":"clear FAILED");log(line);}
 for(auto e:connected_players())three_slots_apply(e);
}

// v0.5.2 (Katz 2026-10-23: "chapter 03 starts at the boss"; each chapter must
// start where its title card shows, with the weapons found by then). The card
// is a cof_chapter in the map (checked in the game's own c_* maps):
//   1 c_start, 2 c_basement, 6 c_bridge: fired by a trigger at the arrival
//     spot, where the map's info_player_start already puts the player;
//   3 c_sewer3: after Mace (button -> scene -> cof_teleport sew3alt_tele to
//     sew3alt_dest by the exit to c_city, card "chapterheh");
//   4 c_park3: back from the roofs (park3roofed enables trigger tro_inte ->
//     Sophie scene -> isteleport2_des, card "chapterlehe");
//   5 c_subway2st1: arriving from c_subwaysick2 (trigger *40 at that landmark
//     -> multigrejer: teleport 2teleportion_ld, card "chapterx");
//   7 c_attic: after the Doctor dies (multidscene ... multistamp: teleport
//     deteleport1_des, card "chapterss").
// at: the player origin (the destination +37, as cof_teleport lands); kill:
// removed like a trigger_relay killtarget; fire: Used with the player as
// activator, in order, before the move (4 needs tro_inte enabled first).
// weapons: the story weapons found before the card (map pickups and
// cof_giveitems in story order), at most five cards so a key still fits in
// the six slots. 6 starts after the train crash takes everything
// (c_bridge cof_clearitems "bagremove", fired by its own trigger); from then
// on there are three slots (v0.5.3), so 7 gives two (the Doctor drops a gun).
struct ChapterStart {const char* label;const char* map;bool move;float at[3],yaw;std::vector<const char*> kill,fire,weapons;};
const ChapterStart ChapterStarts[]={
 {"Prologue","cof_campaign_story4",false,{},0,{},{},{}},
 {"Chapter 1 - Lost in a city","s4_start",false,{},0,{},{},{}},
 {"Chapter 2 - Who is that doctor?","s4_basement_e0",false,{},0,{},{},{"weapon_mobile","weapon_switchblade","weapon_glock"}},
 {"Chapter 3 - The city is not safe","s4_sewer3_e1",true,{-3342,163,-115},0,{},{"door_rotating","killdoor_rotating","chapterheh"},{"weapon_mobile","weapon_switchblade","weapon_glock","weapon_shotgun"}},
 {"Chapter 4 - Drowned in sorrow","s4_park3_e2",true,{1814,-271,708},54,{},{"park3roofed"},{"weapon_mobile","weapon_switchblade","weapon_glock","weapon_shotgun","weapon_rifle"}},
 {"Chapter 5 - Leaving this for good","s4_subway2st1_e0",true,{-461,1135,-1093},270,{},{},{"weapon_mobile","weapon_switchblade","weapon_shotgun","weapon_rifle","weapon_m16"}},
 {"Chapter 6 - It's not over yet","s4_bridge_e1",false,{},0,{},{},{}},
 {"Chapter 7 - Only safe at home","s4_attic_e0",true,{-262,784,325},55,{"doctorboss"},{"killsuiciders","killtoncedeath","bossmusicend","door2changee","lockthatdoor","whatweapon","chapterss","objectiveee"},{"weapon_flashlight","weapon_shotgun"}}};
// The chapter the host picked (-1: none, or a save), applied once on its map.
struct ChapterRun {int pending=-1,active=-1;bool fired=false;std::map<Edict*,int> done;};
ChapterRun chapterRun;
void chapter_world_started(){
 chapterRun.active=chapterRun.pending>=0&&globals&&!std::strcmp(map_name(),ChapterStarts[chapterRun.pending].map)?chapterRun.pending:-1;
 chapterRun.pending=-1;chapterRun.fired=false;chapterRun.done.clear();
}
// UTIL_Remove only marks FL_KILLME (pev->flags 0x1A8); the engine frees it after the frame.
bool chapter_removed(const char* name){for(Edict* t=find_next(nullptr,"targetname",name);t;t=find_next(t,"targetname",name)){auto v=vars(t);if(!v||!(load<unsigned>(v,0x1a8)&0x40000000u))return false;}return true;}
void chapter_fire(const ChapterStart& c,Edict* activator){
 for(const char* name:c.kill){const bool found=find_next(nullptr,"targetname",name)!=nullptr;helper_use_pairs("trigger_relay",{{"killtarget",name},{"triggerstate","2"}},activator);
  char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CHAPTER removed %s (%s)\n",name,!found?"not in the map":chapter_removed(name)?"removed":"STILL THERE");log(line);}
 for(const char* name:c.fire){int used=0;for(Edict* t=find_next(nullptr,"targetname",name);t&&used<16;t=find_next(t,"targetname",name),++used)native<void(__cdecl*)(Edict*,Edict*)>(3)(t,activator);
  char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CHAPTER fired %s (%d)\n",name,used);log(line);}
}
// Each player once, when their HUD is ready: weapons, then the start spot.
void chapter_frame(){
 if(chapterRun.active<0||!globals)return;const ChapterStart& c=ChapterStarts[chapterRun.active];
 for(Edict* e:connected_players()){
  auto it=chapterRun.done.find(e);if(it!=chapterRun.done.end()&&it->second==serial(e))continue;
  if(!client_ready(e)||!view_alive(e))continue;chapterRun.done[e]=serial(e);three_slots_apply(e);
  SavedPlayer now;capture_player(e,now,false);int given=0;std::string missing;
  for(const char* w:c.weapons){if(std::any_of(now.weapons.begin(),now.weapons.end(),[&](const SavedWeapon& s){return s.name==w;}))continue;if(helper_use("cof_giveitems","message",w,e))++given;else missing+=std::string(" ")+w;}
  if(!chapterRun.fired){chapterRun.fired=true;chapter_fire(c,e);}
  bool moved=!c.move;std::array<float,3> at{};
  if(c.move&&find_landing({c.at[0],c.at[1],c.at[2]},e,at))moved=teleport_to(e,at,c.yaw);
  char line[300]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CHAPTER %s player=%d weapons given=%d%s%s start=%s\n",c.label,eng<int(__cdecl*)(Edict*)>(71)(e),given,missing.empty()?"":" refused:",missing.c_str(),!c.move?"map spawn":moved?"moved to the title card spot":"FAILED to move");log(line);
 }
}
struct StoryEntry {const char* file;int chapter,slot;};
constexpr StoryEntry StoryEntries[]={{"Chapter-0-Prologue",0,0},{"Chapter-1-Lost-in-a-City",1,0},{"Chapter-2-Who-is-that-Doctor",2,0},{"Chapter-3-The-City-is-not-Safe",3,0},
 {"Chapter-4-Drowned-in-Sorrow",4,0},{"Chapter-5-Leaving-this-for-Good",5,0},{"Chapter-6-Its-not-Over-Yet",6,0},{"Chapter-7-Only-Safe-at-Home",7,0},
 {"Load-Save-Slot-1",-1,1},{"Load-Save-Slot-2",-1,2},{"Load-Save-Slot-3",-1,3}};
struct cvar_t {char* name;char* string;int flags;float value;cvar_t* next;};   // engine cvar_s
cvar_t collisionCvar{const_cast<char*>("sc_collision"),const_cast<char*>("1"),0,1.f,nullptr};
const StoryEntry* story_entry(const char* map){if(!map)return nullptr;for(const auto& s:StoryEntries)if(!_stricmp(map,s.file))return &s;return nullptr;}
std::string story_entry_command(const StoryEntry& s,std::string& notice){
 if(s.slot)return menu_host_command(10+s.slot,notice);   // saved players, collision and difficulty win
 reset();checkpointWorldStarted=false;checkpointPrepared=false;sessionOptions=SessionOptions{};sessionOptions.have=true;
 const int slots=globals?load<int>(globals,0x90):2;sessionOptions.maxPlayers=slots<2?2:slots>4?4:slots;
 sessionOptions.collision=engine[57]&&eng<float(__cdecl*)(const char*)>(57)("sc_collision")==0.f?0:1;
 chapterRun.pending=s.chapter;
 return host_start_command(sessionOptions,difficulty_now(),ChapterStarts[s.chapter].map);
}
void story_entry_redirect(){
 const auto s=verified&&globals?story_entry(map_name()):nullptr;if(!s||!engine[39])return;
 std::string notice;std::string command;try{command=story_entry_command(*s,notice);}catch(...){notice="The co-op save could not be read.";command="disconnect;maxplayers 1;deathmatch 0;map c_game_menu1\n";}
 char line[300]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] HOST %s: players=%d collision=%s difficulty=%d%s%s\n",s->file,sessionOptions.maxPlayers,sessionOptions.collision?"on":"off",difficulty_now(),notice.empty()?"":" refused: ",notice.c_str());log(line);
 eng<void(__cdecl*)(const char*)>(39)(command.c_str());
}
void __cdecl stuff_command(Edict* e,const char* format,...){
 // Same 1024-byte limit as the engine's own ClientCommand buffer.
 char text[1024]{};va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format?format:"",args);va_end(args);
 std::string trimmed(text);while(!trimmed.empty()&&(trimmed.back()=='\n'||trimmed.back()==' '))trimmed.pop_back();
 if(menuMode&&trimmed.rfind("map ",0)==0&&menu_mode(trimmed.c_str()+4)==menuMode){
  const int mode=menuMode;menuMode=0;std::string notice,command;
  // Never let a file-system error escape into the engine callback.
  try{command=menu_host_command(mode,notice);}catch(...){notice="The co-op save could not be read.\n";command="disconnect;maxplayers 1;deathmatch 0;map c_game_menu1\n";}
  if(!notice.empty()){log("[STORYCOOP_RC4] MENU load refused: no valid co-op checkpoint in that slot\n");if(engine[80]&&live(e))eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,notice.c_str());}
  else log(mode==1?"[STORYCOOP_RC4] MENU starting a two-player co-op server: cof_campaign_story4\n":"[STORYCOOP_RC4] MENU starting a two-player co-op server from a save\n");
  if(engine[39])eng<void(__cdecl*)(const char*)>(39)(command.c_str());return;
 }
 eng<void(__cdecl*)(Edict*,const char*,...)>(41)(e,"%s",text);
}

// --- Main menu credit (coop-fixes-009, user request) ------------------------------
// The 3D main menu is the map c_game_menu1, served by this adapter. A
// game_text-style message (TE_TEXTMESSAGE, the engine's own HUD text that
// maps use) puts the credit in the bottom-left corner on channel 4 (map texts
// default to channel 1) and is re-sent before its 250 s hold runs out.
// coop-fixes-013 (user): the credit names Anas Sammak (koolkat), with a thanks
// line below it, and it was gone after coming back to the train menu from a
// game. A message sent before that client finished loading is dropped or
// wiped by the HUD reset. It is now sent once the client's own commands arrive
// (CmdStart) and then every 5 s without a fade-in, so it is always there.
// v0.1 (user): the build is named v0.1 (v0.2 since 2026-10-16), and Special Thanks lists Mesilll,
// Michal Eminov and caixa in that order (it misspelled "Michael" before). v0.6 (Katz 2026-10-25): joah added.
constexpr const char* MenuCredit="Co-op mod v0.6.2 developed by Anas Sammak (koolkat)\nSpecial thanks: Mesilll, Michal Eminov, caixa, joah";
struct MenuCreditState{int serial=-1;float next=0,active=-1;bool logged=false,shown=false;};
MenuCreditState menuCredit;
void menu_credit_reset(){const bool logged=menuCredit.logged;menuCredit={};menuCredit.logged=logged;}
// From CmdStart: the menu client is running.
void menu_credit_active(Edict* e){
 if(!verified||!globals||menuCredit.active>=0||std::strcmp(map_name(),"c_game_menu1")||!engine[71]||eng<int(__cdecl*)(Edict*)>(71)(e)!=1)return;
 menuCredit.active=load<float>(globals,0);
}
int fixed16(float value,float scale,bool sign){const float v=value*scale;return static_cast<int>(sign?std::clamp(v,-32768.f,32767.f):std::clamp(v,0.f,65535.f));}
void hud_text(Edict* e,int channel,float x,float y,const unsigned char (&color)[4],float hold,const char* text,float fadeIn=1.f){
 auto byte=eng<void(__cdecl*)(int)>(48),shortValue=eng<void(__cdecl*)(int)>(50);
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,23,nullptr,e);   // MSG_ONE, SVC_TEMPENTITY
 byte(29);byte(channel&0xff);shortValue(fixed16(x,8192.f,true));shortValue(fixed16(y,8192.f,true));byte(0);
 for(int k=0;k<2;++k)for(const unsigned char c:color)byte(c);
 shortValue(fixed16(fadeIn,256.f,false));shortValue(fixed16(1.f,256.f,false));shortValue(fixed16(hold,256.f,false));
 eng<void(__cdecl*)(const char*)>(54)(text);eng<void(__cdecl*)()>(47)();
}
void menu_credit_frame(){
 if(!verified||!globals||std::strcmp(map_name(),"c_game_menu1")||!engine[46]||!engine[47]||!engine[48]||!engine[50]||!engine[54]||!engine[72])return;
 Edict* e=eng<Edict*(__cdecl*)(int)>(72)(1);if(!live(e)||std::strcmp(classname(e),"player"))return;
 const float now=load<float>(globals,0);
 if(menuCredit.serial!=serial(e)){const float active=menuCredit.active;menu_credit_reset();menuCredit.active=active;menuCredit.serial=serial(e);}
 if(menuCredit.active<0||now<menuCredit.active+0.5f||now<menuCredit.next)return;menuCredit.next=now+5.f;
 static constexpr unsigned char grey[4]={200,200,200,180};
 hud_text(e,4,0.02f,0.91f,grey,30.f,MenuCredit,menuCredit.shown?0.f:1.f);menuCredit.shown=true;
 if(!menuCredit.logged){menuCredit.logged=true;log("[STORYCOOP_RC4] MENU credit shown bottom-left\n");}
}

// --- Per-session adapter log -------------------------------------------------------
// Launching from Steam with launch options keeps no qconsole archive; every
// adapter line also goes to cryoffear/storycoop_logs/rc4-<time>.log (last 20).
std::ofstream sessionLog;bool sessionLogTried=false;
void session_log(const char* s){
 if(!sessionLogTried)try{sessionLogTried=true;char dir[MAX_PATH]{};if(engine[99]){eng<void(__cdecl*)(char*)>(99)(dir);}
  if(dir[0]&&std::memchr(dir,0,MAX_PATH)){std::error_code error;const auto folder=std::filesystem::path(dir)/L"storycoop_logs";std::filesystem::create_directories(folder,error);
   SYSTEMTIME t{};GetLocalTime(&t);wchar_t name[64]{};std::swprintf(name,64,L"rc4-%04u%02u%02u-%02u%02u%02u.log",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
   std::vector<std::filesystem::path> old;for(const auto& item:std::filesystem::directory_iterator(folder,error)){const auto n=item.path().filename().wstring();if(n.rfind(L"rc4-",0)==0&&item.path().extension()==L".log")old.push_back(item.path());}
   std::sort(old.begin(),old.end());while(old.size()>=20){std::filesystem::remove(old.front(),error);old.erase(old.begin());}
   sessionLog.open(folder/name,std::ios::app);}}catch(...){sessionLog=std::ofstream();}
 if(sessionLog){sessionLog<<s;if(!std::strchr(s,'\n'))sessionLog<<'\n';sessionLog.flush();}
}

// --- Second co-op game crash (v0.5.2) ----------------------------------------------
// Katz 2026-10-23 12:14: Chapter 1, back to the menu, Chapter 7 -> CRASH at
// hl.dll+0x290C0, inside CCoopLevelChangeManager::GivePlayerVariables (10029090),
// called from PlayerSpawn (100C93E0, return 100C93E5 on the logged stack).
// CWorld::Precache (decomp world.cpp; hl.dll 10121138) deletes the manager on a
// single-player map (the menu) and keeps the pointer g_pLevelChange (10222E10);
// the next co-op map sees it non-null, never makes a new one, and reads freed
// memory. After a single-player world spawn the pointer is cleared, as the
// delete left it; the next co-op world makes a fresh manager.
constexpr std::uint32_t LevelChangeRva=0x222E10;
void level_change_release(){
 if(!verified||!backend||!globals||load<float>(globals,0x14)!=0.f)return;   // gpGlobals->deathmatch: multiplayer keeps it
 void** manager=reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(backend)+LevelChangeRva);
 if(*manager){*manager=nullptr;log("[STORYCOOP_RC4] LEVELCHANGE released the deleted co-op level-change manager (single-player map)\n");}
}

// --- Crash report (v0.3) ------------------------------------------------------------
// Tester crashes on a revisited map (s4_start2 -> s4_start_e0) left no error
// text and no dump the tester could send. The adapter now keeps the last eight
// steps it handed to the game (phase, entity index, classname) and, on an
// unhandled exception, writes one line to this log naming the exception code,
// the faulting module+offset, the map, those steps and the code addresses found
// on the stack, then a small minidump beside the log (dbghelp, MiniDumpNormal).
// The previous filter (the game's own crash handler) still runs afterwards.
// Read only: nothing in the game is changed.
struct Crumb {const char* phase=nullptr;int entity=-1;char type[32]{};};
Crumb crumbs[8]{};unsigned crumbAt=0;
void crumb(const char* phase,Edict* e=nullptr){
 Crumb& c=crumbs[crumbAt++&7u];c.phase=phase;c.entity=e&&engine[71]?eng<int(__cdecl*)(Edict*)>(71)(e):-1;
 const char* type=e?classname(e):"";std::snprintf(c.type,sizeof(c.type),"%s",type?type:"");
}
bool crash_module(std::uintptr_t address,char* name,std::size_t n,std::uintptr_t& offset){
 HMODULE m=nullptr;if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(address),&m)||!m)return false;
 char path[MAX_PATH]{};if(!GetModuleFileNameA(m,path,MAX_PATH))return false;const char* base=std::strrchr(path,'\\');
 std::snprintf(name,n,"%s",base?base+1:path);offset=address-reinterpret_cast<std::uintptr_t>(m);return true;
}
void crash_line(const EXCEPTION_POINTERS* x,char* out,std::size_t n){
 const auto r=x?x->ExceptionRecord:nullptr;const auto address=r?reinterpret_cast<std::uintptr_t>(r->ExceptionAddress):0;
 char where[64]="?";std::uintptr_t offset=0;char module[48]{};if(crash_module(address,module,sizeof(module),offset))std::snprintf(where,sizeof(where),"%s+0x%X",module,static_cast<unsigned>(offset));
 int at=std::snprintf(out,n,"[STORYCOOP_RC4] CRASH code=%08X at %s",r?static_cast<unsigned>(r->ExceptionCode):0u,where);
 if(r&&r->ExceptionCode==EXCEPTION_ACCESS_VIOLATION&&r->NumberParameters>=2&&at>0&&static_cast<std::size_t>(at)<n)at+=std::snprintf(out+at,n-at," (%s 0x%08X)",r->ExceptionInformation[0]==1?"write":r->ExceptionInformation[0]==8?"execute":"read",static_cast<unsigned>(r->ExceptionInformation[1]));
 if(at>0&&static_cast<std::size_t>(at)<n)at+=std::snprintf(out+at,n-at," map=%.32s last=[",globals?map_name():"");
 for(unsigned k=0;k<8&&at>0&&static_cast<std::size_t>(at)<n;++k){const Crumb& c=crumbs[(crumbAt+k)&7u];if(!c.phase)continue;at+=std::snprintf(out+at,n-at,"%s %d %s; ",c.phase,c.entity,c.type);}
 if(at>0&&static_cast<std::size_t>(at)<n)at+=std::snprintf(out+at,n-at,"] stack=[");
 // Code addresses on the stack (frame pointers are not reliable in hl.dll).
 if(x&&x->ContextRecord){
  const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());std::uintptr_t sp=x->ContextRecord->Esp;const std::uintptr_t top=reinterpret_cast<std::uintptr_t>(tib->StackBase);
  int found=0;for(;sp+4<=top&&sp<x->ContextRecord->Esp+4096&&found<12&&at>0&&static_cast<std::size_t>(at)<n;sp+=4){
   const auto value=*reinterpret_cast<const std::uintptr_t*>(sp);char m[48]{};std::uintptr_t o=0;
   if(value<0x10000||!crash_module(value,m,sizeof(m),o)||!_strnicmp(m,"ntdll",5)||!_strnicmp(m,"kernel",6)||!_strnicmp(m,"ucrtbase",8))continue;
   at+=std::snprintf(out+at,n-at,"%s%s+0x%X",found?" ":"",m,static_cast<unsigned>(o));++found;
  }
 }
 if(at>0&&static_cast<std::size_t>(at)<n)std::snprintf(out+at,n-at,"]\n");
}
LPTOP_LEVEL_EXCEPTION_FILTER crashPrevious=nullptr;bool crashWritten=false;
void crash_dump(EXCEPTION_POINTERS* x){
 char dir[MAX_PATH]{};if(!engine[99])return;eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return;
 SYSTEMTIME t{};GetLocalTime(&t);char path[MAX_PATH+64]{};std::snprintf(path,sizeof(path),"%s\\storycoop_logs\\crash-%04u%02u%02u-%02u%02u%02u.dmp",dir,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
 HMODULE help=LoadLibraryW(L"dbghelp.dll");auto write=help?reinterpret_cast<decltype(&MiniDumpWriteDump)>(GetProcAddress(help,"MiniDumpWriteDump")):nullptr;if(!write)return;
 HANDLE file=CreateFileA(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return;
 MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(),x,FALSE};const bool ok=write(GetCurrentProcess(),GetCurrentProcessId(),file,MiniDumpNormal,x?&info:nullptr,nullptr,nullptr)!=FALSE;CloseHandle(file);
 char line[MAX_PATH+96]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CRASH minidump %s: %s\n",ok?"written":"failed",path);session_log(line);
}
LONG WINAPI crash_filter(EXCEPTION_POINTERS* x){
 if(!crashWritten){crashWritten=true;char line[1400]{};crash_line(x,line,sizeof(line));session_log(line);crash_dump(x);if(engine[81])eng<void(__cdecl*)(const char*)>(81)(line);}
 return crashPrevious?crashPrevious(x):EXCEPTION_CONTINUE_SEARCH;
}
// v0.4: in the real v0.3 crash (Katz 2026-10-17, reproduced under a debugger
// 2026-10-18) Steam's crashhandler.dll acted on the first-chance exception and
// started steamerrorreporter.exe, which left the game hung; the unhandled
// filter above never ran, so no CRASH line or dump existed. A vectored handler
// (first in the chain, re-added at every map start) writes the same report at
// the first chance, for faults the game never raises in normal play (a whole
// debugger-watched load of s4_start_e0 had none), and lets the search go on.
bool crash_fatal_code(DWORD code){return code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_ILLEGAL_INSTRUCTION||code==EXCEPTION_PRIV_INSTRUCTION||code==EXCEPTION_INT_DIVIDE_BY_ZERO||code==EXCEPTION_ARRAY_BOUNDS_EXCEEDED;}
LONG WINAPI crash_vectored(EXCEPTION_POINTERS* x){
 if(!crashWritten&&x&&x->ExceptionRecord&&crash_fatal_code(x->ExceptionRecord->ExceptionCode)){crashWritten=true;char line[1400]{};crash_line(x,line,sizeof(line));const char* note="[STORYCOOP_RC4] CRASH reported at the first chance (before Steam's crash handler)\n";session_log(line);session_log(note);crash_dump(x);if(engine[81]){eng<void(__cdecl*)(const char*)>(81)(line);eng<void(__cdecl*)(const char*)>(81)(note);}}
 return EXCEPTION_CONTINUE_SEARCH;
}
PVOID crashVectored=nullptr;
// Re-installed at every map start: a filter set later by the game would replace it.
void crash_install(){
 const auto previous=SetUnhandledExceptionFilter(&crash_filter);if(previous!=&crash_filter)crashPrevious=previous;
 if(crashVectored)RemoveVectoredExceptionHandler(crashVectored);crashVectored=AddVectoredExceptionHandler(1,&crash_vectored);
}
// v0.4: the engine unloads this DLL when the game quits; both handlers point
// into it, so they leave first (DllMain, adapter.cpp). A vectored handler left
// behind made every later exception jump into unloaded code (seen under a
// debugger on quit, 2026-10-18); the unhandled filter had the same exposure.
void crash_uninstall(){
 if(crashVectored){RemoveVectoredExceptionHandler(crashVectored);crashVectored=nullptr;}
 const auto current=SetUnhandledExceptionFilter(crashPrevious);if(current!=&crash_filter)SetUnhandledExceptionFilter(current);
}
