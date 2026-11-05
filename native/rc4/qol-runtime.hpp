// Included in adapter.cpp after client-menu.hpp (v0.9, the QoL pack: Katz's "Final picked list" and
// round 6 picks in docs/COOP_QOL_PROPOSAL.md). Every feature works for a host alone. Server parts run in
// the host's game; client parts run in each player's own game (the adapter is loaded there too).
// Message formats are from the decompile (cof-decompiled/src); offsets were read in the pinned DLLs.

// --- 1. storycoop_server.cfg (host settings file) ---------------------------------------
// The installer drops a commented default; the host's game runs it at the start of every story map.
bool serverCfgMissingLogged=false;
void server_cfg_exec(){
 if(!campaign_map()||!engine[39])return;const auto dir=game_dir_path();std::error_code error;
 if(dir.empty()||!std::filesystem::is_regular_file(dir/L"storycoop_server.cfg",error)){if(!serverCfgMissingLogged){serverCfgMissingLogged=true;log("[STORYCOOP_RC4] SERVER storycoop_server.cfg not found; the game's settings stay as they are\n");}return;}
 eng<void(__cdecl*)(const char*)>(39)("exec storycoop_server.cfg\n");
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SERVER storycoop_server.cfg run on %s\n",map_name());log(line);
}

// --- 9. Each player's own subtitle language ----------------------------------------------
// cof_subtitlelanguage is a server cvar (decomp game.cpp 71): CSubtitle::Use (CoF_SubtitleMain.cpp)
// reads the HOST's language file and sends every player that line as "ProFont", and UpdateClientData
// sends the host's hints file with "Objectives" (player.cpp 6719). A joiner's "subtitleset" changed the
// host's setting (client.cpp 1925). Now each game reports its own value with sc_mdl (playerLanguage,
// session-runtime.hpp); a line of the host's subtitles.txt sent to a player of another language becomes
// the same line of that language's file, and the objective is read from that player's hints file. The
// host's value is put back if the game's save code (player.cpp 5430) changes it while a map starts.
int hostLanguage=-1;float hostLanguageUntil=-1;
int language_norm(int l){return l>=2&&l<=7?l:1;}
const char* language_folder(int l){static const char* const folders[]={"dutch","french","german","norwegian","spanish","swedish"};return l>=2&&l<=7?folders[l-2]:nullptr;}
std::string language_file(int l,const char* name){const char* folder=language_folder(l);return folder?std::string("txtfiles/languages/")+folder+"/"+name:std::string("txtfiles/")+name;}
int server_language(){return engine[57]?language_norm(static_cast<int>(eng<float(__cdecl*)(const char*)>(57)("cof_subtitlelanguage"))):1;}
// -1: the host itself, or a game that did not say (an older adapter): the host's language, as before.
int language_of(Edict* e){if(!live(e)||checkpoint_host(e))return -1;auto it=playerLanguage.find(eng<int(__cdecl*)(Edict*)>(71)(e));return it==playerLanguage.end()?-1:language_norm(it->second);}
struct SubtitleTable {bool loaded=false;std::vector<std::string> lines;std::unordered_map<std::string,std::size_t> index;};
std::array<SubtitleTable,8> subtitleTables;
// The text CSubtitle::Use sends for a line: cut at the first '*' after the first character; longer than 254: none.
std::string subtitle_shown(std::string line){if(line.size()>254)return {};for(std::size_t i=1;i<line.size();++i)if(line[i]=='*'){line.resize(i);break;}return line;}
const SubtitleTable& subtitle_table(int l){
 auto& t=subtitleTables[static_cast<std::size_t>(language_norm(l))];if(t.loaded)return t;t.loaded=true;const auto dir=game_dir_path();if(dir.empty())return t;
 std::ifstream file(dir/std::filesystem::path(language_file(l,"subtitles.txt")));std::string line;
 while(std::getline(file,line)){t.lines.push_back(subtitle_shown(line));const auto& shown=t.lines.back();if(shown.size()>1)t.index.emplace(shown,t.lines.size()-1);}
 return t;
}
const char* subtitle_for(Edict* to,const char* s,std::string& keep){
 const int want=language_of(to);if(want<0||!s||std::strlen(s)<2)return s;const int have=server_language();if(want==have)return s;
 const auto& from=subtitle_table(have);const auto it=from.index.find(s);if(it==from.index.end())return s;
 const auto& into=subtitle_table(want);if(it->second>=into.lines.size()||into.lines[it->second].size()<2)return s;
 keep=into.lines[it->second];return keep.c_str();
}
bool standard_hints(const char* s){if(!s)return false;for(int l=1;l<=7;++l)if(language_file(l,"hints.txt")==s)return true;return false;}
// The message being written (message_begin / write_string in adapter.cpp): 1 ProFont, 2 Objectives.
struct LanguageWire {int kind=0;Edict* to=nullptr;int strings=0;std::string keep;} languageWire;
void language_wire_begin(int dest,int type,Edict* e){
 languageWire.kind=0;languageWire.strings=0;languageWire.to=nullptr;
 if(!campaign_map()||!live(e)||std::strcmp(classname(e),"player"))return;
 if(proFontMessage>0&&type==proFontMessage&&(dest==1||dest==8))languageWire.kind=1;else if(objectivesMessage>0&&type==objectivesMessage&&dest==1)languageWire.kind=2;
 languageWire.to=e;
}
const char* language_wire_text(const char* s){
 if(!languageWire.kind)return s;++languageWire.strings;
 if(languageWire.kind==1&&languageWire.strings==1)return subtitle_for(languageWire.to,s,languageWire.keep);
 if(languageWire.kind==2&&languageWire.strings==2&&standard_hints(s)){const int l=language_of(languageWire.to);if(l>=0){languageWire.keep=language_file(l,"hints.txt");return languageWire.keep.c_str();}}
 return s;
}
// A ProFont text copied to another player (mirror_flush in adapter.cpp).
const char* language_mirror_text(Edict* to,int type,const char* s,std::string& keep){return proFontMessage>0&&type==proFontMessage?subtitle_for(to,s,keep):s;}
// The first 15 s of each story map; outside it the host's current value is simply followed (his own
// choice in the game's options, the console or "subtitleset").
void language_activate(){if(!globals)return;const bool story=campaign_map();if(story&&hostLanguage<0)hostLanguage=server_language();hostLanguageUntil=story?load<float>(globals,0)+15.f:-1.f;}
void language_frame(){
 if(hostLanguage<0||!globals||!engine[59])return;const int now=server_language();if(now==hostLanguage)return;
 if(load<float>(globals,0)>hostLanguageUntil){hostLanguage=now;return;}
 eng<void(__cdecl*)(const char*,float)>(59)("cof_subtitlelanguage",static_cast<float>(hostLanguage));
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LANGUAGE the host's subtitle language set back to %d (changed while the map started)\n",hostLanguage);log(line);
}
// "subtitleset N": the host's own choice as always; a joiner's choice stays his (his own game's cvar).
bool language_command(Edict* e,const char* command){
 if(!command||std::strcmp(command,"subtitleset")||!live(e))return false;
 const char* a=eng<const char*(__cdecl*)(int)>(83)(1);const int n=a&&*a?std::atoi(a):-1;
 if(checkpoint_host(e)){if(n>=0&&n<=7)hostLanguage=language_norm(n);return false;}
 if(n<0||n>7)return true;const int index=eng<int(__cdecl*)(Edict*)>(71)(e);playerLanguage[index]=n;
 if(engine[41])eng<void(__cdecl*)(Edict*,const char*,...)>(41)(e,"cof_subtitlelanguage %d\n",n);
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LANGUAGE player=%d chose subtitles %d (the host's setting is not changed)\n",index,n);log(line);return true;
}

// --- Shared: scenes, names --------------------------------------------------------------
bool view_elsewhere(Edict* e){auto it=clientViews.find(e);if(it==clientViews.end())return false;auto v=camera_resolve(it->second);return v&&v!=e;}
// A scene camera (or any other view than his own, a ladder climb aside) holds this player.
bool in_scene(Edict* e){return skip_viewing(e)||lease_camera(e)!=nullptr||(view_elsewhere(e)&&!climbing(e));}
std::string player_name(Edict* e){
 std::string name;if(engine[106]&&engine[107]){auto info=eng<char*(__cdecl*)(Edict*)>(106)(e);const char* n=info?eng<char*(__cdecl*)(char*,char*)>(107)(info,const_cast<char*>("name")):nullptr;name=clean_name(n);}
 return name.empty()?std::string("Your partner"):name;
}

// --- 6. Objective reminder ---------------------------------------------------------------
// The game's "Objectives" message (player.cpp 6762: key, hints file, show byte) with show=1 puts the
// objective at the top of the screen for 5 s (client hud_msg.cpp 1236). Sent to a player who joins a
// map more than 15 s after it started, who respawned, or who used unstuck. B still shows it any time.
void objective_remind(Edict* e,const char* why){
 if(objectivesMessage<=0||objectiveText.empty()||!live(e)||!client_ready(e)||!engine[46]||!engine[47]||!engine[48]||!engine[54])return;
 const int l=language_of(e);const std::string file=!objectiveFile.empty()?objectiveFile:language_file(l<0?server_language():l,"hints.txt");
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,objectivesMessage,nullptr,e);
 eng<void(__cdecl*)(const char*)>(54)(objectiveText.c_str());eng<void(__cdecl*)(const char*)>(54)(file.c_str());eng<void(__cdecl*)(int)>(48)(1);eng<void(__cdecl*)()>(47)();
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] OBJECTIVE reminder to player=%d (%s): %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),why,objectiveText.c_str());log(line);
}
struct LateObjective {int serial=-1;float due=-1;bool done=false;};
std::unordered_map<Edict*,LateObjective> lateObjectives;float mapFirstReady=-1;
void objective_late_frame(Edict* e){
 if(!scoped()||!globals||!live(e)||!client_ready(e))return;const float now=load<float>(globals,0);
 if(mapFirstReady<0)mapFirstReady=now;
 auto& s=lateObjectives[e];if(s.serial!=serial(e))s={serial(e),now-mapFirstReady>15.f?now+3.f:-1.f,false};
 if(s.done||s.due<0||now<s.due||in_scene(e)||!view_alive(e))return;
 s.done=true;objective_remind(e,"joined late");
}

// --- 3. Unstuck (K) ----------------------------------------------------------------------
// "sc_unstuck", bound to K by storycoop_comfort.cfg: a player inside a wall or a prop goes to the nearest
// spot where he fits (v0.8 item 6's clear-spot test, rings out from where he stands); a player who is
// not inside anything goes beside the nearest partner (Katz: this replaces a regroup vote); alone, to a
// clear spot nearby. Once per 10 s, never while down, in a scene, on a ladder or during the load wait.
constexpr float UnstuckCooldown=10.f;
std::unordered_map<Edict*,std::pair<int,float>> unstuckUsed;
bool hull_stuck(Edict* e,const std::array<float,3>& at){
 auto v=vars(e);if(!engine[34]||!v)return false;const bool duck=(load<unsigned>(v,0x1a8)&0x4000u)!=0;   // FL_DUCKING: the small hull
 Trace t{};eng<void(__cdecl*)(const float*,const float*,int,int,Edict*,Trace*)>(34)(at.data(),at.data(),0,duck?3:1,e,&t);return t.allSolid||t.startSolid;
}
// Rings out from `at`. Inside a wall there is no straight line out, so only the spot itself is tested
// (fits, open air, a floor below); otherwise the walk from `at` must be clear too (landing_clear).
bool unstuck_spot(const std::array<float,3>& at,Edict* e,bool inside,std::array<float,3>& out){
 for(float radius:{16.f,32.f,48.f,64.f,96.f,128.f,160.f,192.f}){
  if(!inside&&radius<48.f)continue;const int steps=radius<=48.f?8:16;
  for(int k=0;k<steps;++k){const float angle=static_cast<float>(k)*6.2831853f/static_cast<float>(steps);
   for(float lift:{0.f,18.f,36.f}){const std::array<float,3> p{at[0]+radius*std::cos(angle),at[1]+radius*std::sin(angle),at[2]+lift};
    if(inside?landing_clear(p,p,e):landing_clear(at,p,e)){out=p;return true;}}}
 }
 return false;
}
bool unstuck_command(Edict* e,const char* command){
 if(!command||std::strcmp(command,"sc_unstuck"))return false;
 if(!(scoped()||opening())||!globals||!engine[80]||!live(e)||std::strcmp(classname(e),"player"))return true;
 auto say=[&](const char* text){eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);};
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);const float now=load<float>(globals,0);
 auto& used=unstuckUsed[e];if(used.first!=serial(e)||used.second>now)used={serial(e),-100.f};
 if(now-used.second<UnstuckCooldown){char text[64]{};std::snprintf(text,sizeof(text),"Unstuck again in %d s.",static_cast<int>(std::ceil(UnstuckCooldown-(now-used.second))));say(text);return true;}
 auto v=vars(e);const char* refused=!view_alive(e)?"Unstuck is off while you are down.":in_scene(e)?"Unstuck is off during a scene.":climbing(e)||load<int>(v,0x108)==5?"Unstuck is off on a ladder.":loadWait.armed?"Wait until everyone has loaded.":nullptr;
 if(refused){say(refused);char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] UNSTUCK player=%d refused: %s\n",index,refused);log(line);return true;}
 const std::array<float,3> at{load<float>(v,8),load<float>(v,12),load<float>(v,16)};const float yaw=load<float>(v,0x54);
 std::array<float,3> to{};const char* how=nullptr;const bool inside=hull_stuck(e,at);
 if(inside){if(unstuck_spot(at,e,true,to)&&teleport_to(e,to,yaw))how="out of the wall";}
 else{
  Edict* partner=nullptr;float best=0;
  for(auto o:connected_players()){if(o==e||!player_ready_for_move(o))continue;auto q=vars(o);float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-at[k];d+=x*x;}if(!partner||d<best){partner=o;best=d;}}
  if(partner){if(move_beside(e,partner))how="beside your partner";}
  else if(unstuck_spot(at,e,false,to)&&teleport_to(e,to,yaw))how="to a clear spot nearby";
 }
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] UNSTUCK player=%d at %.0f %.0f %.0f%s: %s\n",index,at[0],at[1],at[2],inside?" (inside something)":"",how?how:"no free spot found");log(line);
 if(!how){say("No free spot found.\nTry again from another place.");return true;}
 used.second=now;say((std::string("Moved ")+how+".").c_str());objective_remind(e,"used unstuck");return true;
}
// The game's own key prompt (ControlHint, player.cpp 14925; client HUDControl_guess.cpp 192): line1 of
// "storycoop_unstuck" in txtfiles/storycoop_hints.txt (installed), the '*' replaced by the key bound to
// sc_unstuck. Once per player per hosted game, 8 s after he is first ready on a story map, out of scenes.
std::set<std::string> unstuckHinted;std::unordered_map<Edict*,std::pair<int,float>> unstuckHintDue;
void unstuck_hint_frame(Edict* e){
 if(controlHintMessage<=0||!scoped()||!globals||!live(e)||!client_ready(e)||!view_alive(e)||in_scene(e)||!engine[46]||!engine[47]||!engine[54])return;
 std::string key=steam_identity(e);if(key.empty())key="slot"+std::to_string(eng<int(__cdecl*)(Edict*)>(71)(e));if(unstuckHinted.count(key))return;
 const float now=load<float>(globals,0);auto& due=unstuckHintDue[e];if(due.first!=serial(e)){due={serial(e),now+8.f};return;}if(now<due.second)return;
 unstuckHinted.insert(key);
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,controlHintMessage,nullptr,e);eng<void(__cdecl*)(const char*)>(54)("storycoop_unstuck");eng<void(__cdecl*)(const char*)>(54)("txtfiles/storycoop_hints.txt");eng<void(__cdecl*)()>(47)();
 char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] UNSTUCK key prompt shown to player=%d\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
}

// --- 4. "<player> is down" and 5. the revive groan ----------------------------------------
// Down: the white notice the game uses for "Dodge enabled" (UTIL_ProMessage colour 1, client.cpp 1383)
// to every other player, once per fall to the floor (not in a fall zone: that player respawns in 2 s).
// Getting up: RevivePlayer sets fuser3 3 for the get-up (player.cpp 2344); then the revived player's
// own low-health or out-of-breath voice plays at him. Police models: the police set, precached in every
// multiplayer game (client.cpp 2840); everyone else: Simon's set, precached here on story maps.
const char* const SimonGroans[]={"player/vocal/LowHealth/lhealth1.wav","player/vocal/LowHealth/lhealth2.wav","player/vocal/LowHealth/lhealth3.wav","player/vocal/LowHealth/lhealth4.wav","player/vocal/LowHealth/lhealth5.wav","player/player_out_of_breath.wav"};
const char* const PoliceGroans[]={"player/police/LowHealth/lhealth1.wav","player/police/LowHealth/lhealth2.wav","player/police/LowHealth/lhealth3.wav","player/police/LowHealth/lhealth4.wav","player/police/player_out_of_breath1.wav","player/police/player_out_of_breath2.wav"};
void groans_precache(){if(!engine[1])return;for(const char* s:SimonGroans)eng<int(__cdecl*)(const char*)>(1)(s);}
const char* groan_for(Edict* e){
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);auto it=modelStates.find(index);const bool police=it!=modelStates.end()&&it->second.want.rfind("police",0)==0;
 const auto& set=police?PoliceGroans:SimonGroans;const int n=static_cast<int>(std::size(set));
 const int pick=engine[90]?eng<std::int32_t(__cdecl*)(std::int32_t,std::int32_t)>(90)(0,n-1):0;return set[pick<0||pick>=n?0:pick];
}
void revive_groan(Edict* e){
 if(!engine[29])return;const char* sample=groan_for(e);eng<void(__cdecl*)(Edict*,int,const char*,float,float,int,int)>(29)(e,2,sample,1.f,0.8f,0,100);   // CHAN_VOICE, ATTN_NORM
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REVIVE player=%d gets up: %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),sample);log(line);
}
void down_notice(Edict* e){
 if(proFontMessage<=0||!engine[46]||!engine[47]||!engine[48]||!engine[54])return;const std::string text=player_name(e)+" is down";int told=0;
 for(auto o:connected_players()){if(o==e||!client_ready(o))continue;eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,proFontMessage,nullptr,o);eng<void(__cdecl*)(int)>(48)(1);eng<void(__cdecl*)(const char*)>(54)(text.c_str());eng<void(__cdecl*)()>(47)();++told;}
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DOWN player=%d is down; %d other player(s) told\n",eng<int(__cdecl*)(Edict*)>(71)(e),told);log(line);
}
struct DownState {int serial=-1;bool dead=false,getup=false;};
std::unordered_map<Edict*,DownState> downStates;
std::unordered_map<Edict*,int> readingDocs,bagsOpen;   // busy tags below; value: the player's serial
// Prethink, before fall_frame (which moves a body out of a fall zone).
void down_frame(Edict* e){
 auto v=vars(e);if(!scoped()||!globals||!v||std::strcmp(classname(e),"player"))return;
 const bool dead=load<int>(v,0x174)!=0||load<float>(v,0x164)<=0;auto& s=downStates[e];
 if(s.serial!=serial(e)){s={serial(e),dead,false};return;}
 if(dead&&!s.dead){s.dead=true;s.getup=false;readingDocs.erase(e);bagsOpen.erase(e);const float o[3]={load<float>(v,8),load<float>(v,12),load<float>(v,16)};if(client_ready(e)&&!in_fatal_zone(o))down_notice(e);return;}
 if(dead&&!s.getup&&load<float>(v,PlayerMode)==3.f){s.getup=true;revive_groan(e);return;}
 if(!dead&&s.dead){const bool revived=s.getup;s.dead=false;s.getup=false;if(!revived)objective_remind(e,"respawned");}
}

// --- 7. Native team HUD and the busy tag (server side) --------------------------------------
// The client draws the game's co-op list "Name - Health 85 - Ping 40" with the syringe icon on maps whose
// name holds "4" (client patch since coop-fixes-013; client-menu.hpp). Each player's game is told
// "sc_teamhud 0" while a scene holds him (the black box) and "sc_teamhud 1" otherwise, which shows it on
// every story map; "sc_busy <player> <tag>" names what a player is busy with: 1 phone (m_iUsingComputer
// OrPhone 2 or 3, +0x1EA0 in the pinned hl.dll), 4 puzzle (1: keypad or computer), 2 reading (a NoteDoc
// sent to him until "closedocument"), 3 bag (his own game reports its inventory panel, sc_bag).
constexpr std::size_t PlayerUsingComputerOrPhone=0x1ea0;
struct TeamView {int serial=-1,shown=-1;std::array<int,5> tags{-1,-1,-1,-1,-1};};
std::unordered_map<Edict*,TeamView> teamViews;float teamNext=0;
int busy_code(Edict* e){
 if(!live(e)||!view_alive(e))return 0;void* p=nullptr;std::size_t n=0;
 if(player_memory(e,p,n)){const int use=load<int>(p,PlayerUsingComputerOrPhone);if(use==2||use==3)return 1;if(use==1)return 4;}
 auto r=readingDocs.find(e);if(r!=readingDocs.end()&&r->second==serial(e))return 2;
 auto b=bagsOpen.find(e);if(b!=bagsOpen.end()&&b->second==serial(e))return 3;
 return 0;
}
void team_hud_frame(){
 if(!campaign_map()||!globals||!engine[41]||!engine[72])return;const float now=load<float>(globals,0);
 if(now<teamNext&&teamNext-now<1.f)return;teamNext=now+0.2f;
 std::array<int,5> codes{};for(int i=1;i<=4&&i<=load<int>(globals,0x90);++i){auto o=eng<Edict*(__cdecl*)(int)>(72)(i);codes[static_cast<std::size_t>(i)]=live(o)&&!std::strcmp(classname(o),"player")?busy_code(o):0;}
 for(auto r:connected_players()){
  if(!skip_ready(r))continue;auto& tv=teamViews[r];if(tv.serial!=serial(r)){tv=TeamView{};tv.serial=serial(r);}
  std::string text;const int want=in_scene(r)?0:1;
  if(want!=tv.shown){text+="sc_teamhud "+std::to_string(want)+"\n";tv.shown=want;}
  for(std::size_t i=1;i<=4;++i)if(codes[i]!=tv.tags[i]){text+="sc_busy "+std::to_string(i)+" "+std::to_string(codes[i])+"\n";tv.tags[i]=codes[i];}
  if(!text.empty())eng<void(__cdecl*)(Edict*,const char*,...)>(41)(r,"%s",text.c_str());
 }
}
void busy_note_opened(int dest,int type,Edict* e){if(noteDoc>0&&type==noteDoc&&dest==1&&scoped()&&live(e))readingDocs[e]=serial(e);}
// Client commands: sc_unstuck, subtitleset, sc_bag (true: handled here); closedocument is only noted.
bool qol_client_command(Edict* e,const char* command){
 if(!command)return false;
 if(!std::strcmp(command,"closedocument")){readingDocs.erase(e);return false;}
 if(!std::strcmp(command,"sc_bag")){const char* a=eng<const char*(__cdecl*)(int)>(83)(1);if(a&&*a=='1'&&scoped()&&live(e))bagsOpen[e]=serial(e);else bagsOpen.erase(e);return true;}
 // Older games answer these as unknown commands back to the server: nothing to do.
 if(!std::strcmp(command,"sc_teamhud")||!std::strcmp(command,"sc_busy")||!std::strcmp(command,"sc_hud"))return true;
 return unstuck_command(e,command)||language_command(e,command);
}
void qol_map_changed(){downStates.clear();readingDocs.clear();bagsOpen.clear();teamViews.clear();teamNext=0;lateObjectives.clear();mapFirstReady=-1;unstuckHintDue.clear();}
void qol_disconnect(Edict* e){downStates.erase(e);readingDocs.erase(e);bagsOpen.erase(e);teamViews.erase(e);lateObjectives.erase(e);unstuckUsed.erase(e);unstuckHintDue.erase(e);}

// --- Client side (each player's own game) ---------------------------------------------------
// Team HUD (client.dll 1003BF20, decomp HUDControl_guess.cpp 1974): the list is drawn while
// strstr(level name, *push operand at 1003C66D*) is not null. The operand points at "4" since
// coop-fixes-013; now also at "" (the byte after "4": always) or at the list's own format string
// (never: no map name has " - "). Default "4" until a v0.9 host says otherwise.
constexpr std::uint32_t TeamListPattern=0x1003C66D,TeamFour=0x1014231C,TeamAlways=0x1014231D,TeamNever=0x10146894;
int teamHudState=-1;bool hudHidden=false,teamHudServer=false;std::array<int,33> teamTags{};
bool client_dword_set(std::uint32_t va,std::initializer_list<std::uint32_t> allowed,std::uint32_t want){
 if(!clientImage||va<ClientPreferredBase||va-ClientPreferredBase+4>clientImageSize)return false;const std::uint32_t base=reinterpret_cast<std::uint32_t>(clientImage);
 unsigned char* at=clientImage+(va-ClientPreferredBase);const std::uint32_t now=load<std::uint32_t>(at,0),target=want-ClientPreferredBase+base;if(now==target)return true;
 if(std::none_of(allowed.begin(),allowed.end(),[&](std::uint32_t a){return now==a-ClientPreferredBase+base;}))return false;
 DWORD old=0;if(!VirtualProtect(at,4,PAGE_EXECUTE_READWRITE,&old))return false;store(at,0,target);VirtualProtect(at,4,old,&old);FlushInstructionCache(GetCurrentProcess(),at,4);return true;
}
void team_hud_pointer(){
 const std::uint32_t want=hudHidden||teamHudState==0?TeamNever:teamHudState==1?TeamAlways:TeamFour;
 if(!client_dword_set(TeamListPattern,{TeamFour,TeamAlways,TeamNever},want)&&clientImage)session_log("[STORYCOOP_RC4] TEAM HUD switch refused: unexpected bytes at 1003C66D\n");
}
void team_hud_reset(){teamHudState=-1;teamHudServer=false;teamTags={};team_hud_pointer();}
// The list's line: "push name; push health; push ping; sprintf(buf,"%s - Health %i - Ping %d",...)" at
// 1003C807 (28 bytes, ebx = &g_PlayerInfoList[row+1], stride 20) becomes sprintf(buf,"%s",line) with the
// line read from a table at the same stride in this DLL (ebx+distance): the adapter composes the game's
// own text plus the busy tag every 50 ms from the same client values (name and ping from
// g_PlayerInfoList, health from m_iCoopHealth 1018D66C). "%s" is the client's own string at 1013C71B.
struct TeamRow {const char* line;unsigned char unused[16];};
static_assert(sizeof(TeamRow)==20,"g_PlayerInfoList stride");
TeamRow teamRows[4]{};char teamLines[4][112]{};bool teamLinePatched=false;
constexpr std::uint32_t TeamLineVa=0x1003C807,TeamRowsVa=0x10556CC4,TeamHealthVa=0x1018D66C,TeamFormatVa=0x10146894,TeamPlainFormatVa=0x1013C71B;
std::vector<unsigned char> team_line_shipped(std::uint32_t base){
 std::vector<unsigned char> b{0x0f,0xbf,0x43,0x04,0x50,0x8b,0x85,0x98,0xfd,0xff,0xff,0xff,0x34,0x30,0x8d,0x85,0x1c,0xff,0xff,0xff,0xff,0x33,0x68};
 const auto f=client_dword(TeamFormatVa-ClientPreferredBase+base);b.insert(b.end(),f.begin(),f.end());b.push_back(0x50);return b;
}
std::vector<unsigned char> team_line_ours(std::uint32_t base,std::uint32_t rows){
 std::vector<unsigned char> b{0x8d,0x85,0x1c,0xff,0xff,0xff,0x50,0x50,0xff,0xb3};   // lea eax,[ebp-0E4h]; push eax; push eax; push [ebx+distance]
 const auto d=client_dword(rows-(TeamRowsVa-ClientPreferredBase+base));b.insert(b.end(),d.begin(),d.end());
 b.push_back(0x68);const auto f=client_dword(TeamPlainFormatVa-ClientPreferredBase+base);b.insert(b.end(),f.begin(),f.end());   // push "%s"
 b.push_back(0x50);b.insert(b.end(),8,0x90);return b;   // push eax (buf); nops up to the unchanged call
}
const char* const TeamTagText[]={""," (phone)"," (reading)"," (bag)"," (puzzle)"};
void team_rows_update(){
 if(!teamLinePatched||!clientImage)return;
 for(std::size_t i=0;i<4;++i){
  const unsigned char* entry=clientImage+(TeamRowsVa-ClientPreferredBase)+i*20;const char* name=load<const char*>(entry,0);const int ping=load<short>(entry,4);
  const int health=load<int>(clientImage,(TeamHealthVa-ClientPreferredBase)+i*4);const int tag=teamTags[i+1];
  char shown[32]{};if(name)for(std::size_t k=0;k<31&&name[k];++k)shown[k]=name[k];
  std::snprintf(teamLines[i],sizeof(teamLines[i]),"%s - Health %i - Ping %d%s",shown,health,ping,tag>0&&tag<=4?TeamTagText[tag]:"");
 }
}
void team_line_patch(){
 if(!clientImage||teamLinePatched)return;for(std::size_t i=0;i<4;++i){teamLines[i][0]=0;teamRows[i].line=teamLines[i];}
 const std::uint32_t base=reinterpret_cast<std::uint32_t>(clientImage);std::string why;
 const int r=patch_apply({{TeamLineVa,team_line_shipped(base),team_line_ours(base,reinterpret_cast<std::uint32_t>(&teamRows[0])),false}},clientImage,clientImageSize,base,why);
 teamLinePatched=r!=0;team_rows_update();
 log(r?"[STORYCOOP_RC4] TEAM HUD line shows busy tags (phone, reading, bag, puzzle)\n":("[STORYCOOP_RC4] TEAM HUD line left as the game's: "+why+"\n").c_str());
}
// Before this DLL goes away (DllMain): the client code must not read the table in it any more.
void team_line_unpatch(){
 HMODULE loaded=nullptr;if(!teamLinePatched||!clientImage||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(clientImage),&loaded)||reinterpret_cast<unsigned char*>(loaded)!=clientImage)return;teamLinePatched=false;const std::uint32_t base=reinterpret_cast<std::uint32_t>(clientImage);std::string why;
 patch_apply({{TeamLineVa,team_line_ours(base,reinterpret_cast<std::uint32_t>(&teamRows[0])),team_line_shipped(base),false}},clientImage,clientImageSize,base,why);
}
void __cdecl team_hud_command(){const char* a=engine[83]?eng<const char*(__cdecl*)(int)>(83)(1):nullptr;teamHudState=a&&*a=='1'?1:a&&*a=='0'?0:-1;teamHudServer=true;team_hud_pointer();}
void __cdecl team_busy_command(){
 if(!engine[83])return;const char* a=eng<const char*(__cdecl*)(int)>(83)(1);const char* b=eng<const char*(__cdecl*)(int)>(83)(2);
 const int i=a?std::atoi(a):0,c=b?std::atoi(b):0;if(i>=1&&i<=32&&c>=0&&c<=4)teamTags[static_cast<std::size_t>(i)]=c;
}
// 8. HUD off for screenshots ("sc_hud", H by default): hud_draw (decomp hud_redraw.cpp 540: every
// HUD element, the adapter's on-screen texts included; HUDControl_guess.cpp 1155: the bars) and the list.
void __cdecl hud_toggle_command(){
 hudHidden=!hudHidden;if(engine[59])eng<void(__cdecl*)(const char*,float)>(59)("hud_draw",hudHidden?0.f:1.f);team_hud_pointer();
 session_log(hudHidden?"[STORYCOOP_RC4] HUD hidden (HUD key)\n":"[STORYCOOP_RC4] HUD shown (HUD key)\n");
}
// The inventory (bag) panel is the client's own: IN_InventoryDown (client.dll 100790C0) reads
// gViewPort [101B2600] -> m_pRECBInventory +0x1404 -> isVisible (vtable +0x28). Reported to a v0.9 host.
bool bagReported=false;
bool panel_visible(void* panel){__try{auto table=*reinterpret_cast<void***>(panel);return reinterpret_cast<bool(__thiscall*)(void*)>(table[0x28/4])(panel);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
void bag_poll(){
 if(!clientImage||!teamHudServer||!engine[39])return;void* viewport=load<void*>(clientImage,0x1B2600);if(!viewport)return;void* panel=load<void*>(viewport,0x1404);if(!panel)return;
 const bool open=panel_visible(panel);if(open==bagReported)return;bagReported=open;eng<void(__cdecl*)(const char*)>(39)(open?"cmd sc_bag 1\n":"cmd sc_bag 0\n");
}
// 8. Comfort settings and co-op keys: storycoop_comfort.cfg (installed; "Settings" in the launcher writes
// it) runs once, 4 s after the game starts (after config.cfg). FOV is cl_fovmultiplier (decomp
// hud_update.cpp 45: the view is m_flFOV * cl_fovmultiplier; default_fov is turned off in CoF), so the
// v0.8 zoom reset to 90 keeps each player's own FOV. Head bob: cl_bob, cl_bobup, cl_bobcycle (view.cpp).
unsigned long long processStart=0;bool comfortDone=false;
void comfort_frame(){
 if(comfortDone||!engine[39])return;if(!processStart)processStart=GetTickCount64();if(GetTickCount64()-processStart<4000)return;comfortDone=true;
 // The HUD key's hud_draw 0 is saved in config.cfg if the game was closed with the HUD hidden.
 if(engine[57]&&engine[59]&&eng<float(__cdecl*)(const char*)>(57)("hud_draw")==0.f){eng<void(__cdecl*)(const char*,float)>(59)("hud_draw",1.f);session_log("[STORYCOOP_RC4] COMFORT hud_draw was 0 at start: HUD shown\n");}
 const auto dir=game_dir_path();std::error_code error;
 if(dir.empty()||!std::filesystem::is_regular_file(dir/L"storycoop_comfort.cfg",error)){session_log("[STORYCOOP_RC4] COMFORT storycoop_comfort.cfg not found; keys and view as they are\n");return;}
 eng<void(__cdecl*)(const char*)>(39)("exec storycoop_comfort.cfg\n");session_log("[STORYCOOP_RC4] COMFORT storycoop_comfort.cfg run (co-op keys, FOV, head bob)\n");
}
// 11. For the launcher's rejoin offer: storycoop_session.txt says whether this game is a joiner.
// "joining <ip:port>" (join box), "joined <ip:port>" (a host's server asked for sc_report while this
// game's own server is off), "local" (this game runs a map), "closed" (DllMain: a normal quit).
// A crash leaves "joining" or "joined". Plain Win32 writes: DllMain may run it.
std::wstring sessionFilePath;std::string lastJoinTarget;bool localServerActive=false;
void session_state(const char* state){
 if(sessionFilePath.empty()){const auto dir=game_dir_path();if(dir.empty())return;sessionFilePath=(dir/L"storycoop_session.txt").wstring();}
 char text[320]{};std::snprintf(text,sizeof(text),"%s %s\n",state,lastJoinTarget.empty()?"-":lastJoinTarget.c_str());
 HANDLE file=CreateFileW(sessionFilePath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return;
 DWORD written=0;WriteFile(file,text,static_cast<DWORD>(std::strlen(text)),&written,nullptr);CloseHandle(file);
}
// DllMain: only once the path is known (the engine may be gone by then).
void session_closed(){if(!sessionFilePath.empty())session_state("closed");}
void __cdecl qol_register_commands(){
 if(!engine[140])return;auto add=eng<void(__cdecl*)(const char*,void(__cdecl*)())>(140);
 add("sc_teamhud",&team_hud_command);add("sc_busy",&team_busy_command);add("sc_hud",&hud_toggle_command);
}
void spray_client_install();
void qol_timer(){team_rows_update();bag_poll();comfort_frame();spray_client_install();}
void session_join_started(const std::string& target){lastJoinTarget=target;session_state("joining");}
// A game started with "+connect <ip:port>" (the launcher's rejoin, join-coop.ps1) joined that address.
std::string command_line_target(const char* line){
 const char* at=line?std::strstr(line,"+connect "):nullptr;if(!at)return {};at+=9;while(*at==' '||*at=='"')++at;
 std::string target;while(*at&&*at!=' '&&*at!='"'&&target.size()<260)target+=*at++;
 if(target.find(':')==std::string::npos)target+=":27015";
 const auto colon=target.rfind(':');return join_address(target.substr(0,colon))&&join_port(target.substr(colon+1))?target:std::string();
}
void session_joined_report(){if(localServerActive)return;if(lastJoinTarget.empty())lastJoinTarget=command_line_target(GetCommandLineA());session_state("joined");}
