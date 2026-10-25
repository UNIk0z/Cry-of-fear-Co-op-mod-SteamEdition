// Included in adapter.cpp after story-runtime.hpp (coop-fixes-013).
// User test 2026-10-13 (friend's PC hosting, s4_apartment* and s4_nightmare):
// after every map change each player was back to the phone and the knife
// (morphine, handgun and tapes gone); the phone screen went back to the first
// SMS; the story handgun reached only the player who started the scene; a
// player model stood inside scene cameras; the first tape recorder only showed
// its hint; one photo in the nightmare camera rooms shut the partner out.
// Public engine/game interfaces only, plus the pinned player fields the
// checkpoint code already reads.

// --- Inventory carried across every map change -----------------------------------
// The co-op level change rebuilds every player, and on the RC4 aliases the
// native carry did not survive (user screenshots: bag = knife + phone after
// each change). Each player's full record (capture_player: weapons, clips,
// reserve ammo, items, counters, tapes, health, stamina, quick keys, phone
// screen) is refreshed once a second while it can be captured and again when
// the map ends. On the next story map, once that player's HUD is ready, the
// record is applied without a teleport: missing weapons and items are given,
// values are set; weapons the new map already gave are kept. A game over
// without a save of the map goes back to what everyone had on arrival (the
// game's own restart rule). A pending save load or a rejoin owns the player.
std::unordered_map<std::string,SavedPlayer> carryBank,carryEntry;
std::unordered_map<Edict*,int> carryDone;
std::unordered_map<Edict*,float> carryNext;
int carryLines=0;
void carry_log(const char* line){if(carryLines<200){++carryLines;log(line);}}
void carry_reset(){carryBank.clear();carryEntry.clear();carryDone.clear();carryNext.clear();carryLines=0;}
// A loaded save's records: replaced by each player's own state once restored.
void carry_seed(const std::vector<SavedPlayer>& saved){for(const auto& p:saved)if(!p.identity.empty())carryBank[p.identity]=p;}
// True when the player's record was read now (false: the last one is kept).
bool carry_departing(Edict* e){
 if(!scoped()||checkpointRestoreFailed||!live(e)||std::strcmp(classname(e),"player"))return false;
 if(loadingCheckpoint){auto r=restoredPlayers.find(e);if(r==restoredPlayers.end()||r->second!=serial(e))return false;}
 const auto identity=player_identity(e);SavedPlayer p;if(identity.empty()||!client_ready(e)||!capture_player(e,p,false))return false;
 carryBank[identity]=std::move(p);return true;
}
// v0.1: every map change of the story goes through the engine's ChangeLevel
// (trigger_coop_changelevel, inter_door_coop_changelevel, cof_changelevel,
// the game-over restart). It is wrapped so each player's inventory is read at
// the moment the change is requested, before the native level-change manager
// or the new map can rebuild anyone. A player who is dead keeps their last
// good record (the once-a-second refresh), never an empty one.
void load_wait_level_change(int players);void arrival_level_change(const char* map);   // below
void carry_level_change(const char* map){
 if(!scoped()||!globals)return;int banked=0,kept=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!live(e)||std::strcmp(classname(e),"player"))continue;if(carry_departing(e))++banked;else ++kept;}
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CARRY level change to %.40s: %d player record(s) read now, %d kept from before\n",map&&*map?map:"?",banked,kept);carry_log(line);load_wait_level_change(banked+kept);arrival_level_change(map);
}
// ServerDeactivate, after every player was banked.
void carry_map_changed(){
 if(!gameOverMap.empty()&&!carryEntry.empty()){for(const auto& [id,entry]:carryEntry)carryBank[id]=entry;carry_log("[STORYCOOP_RC4] CARRY game over: every player goes back to what they had on arrival\n");}
 carryEntry.clear();carryDone.clear();carryNext.clear();
}
void carry_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;
 if(!scoped()||!globals||checkpointRestoreFailed||!player_memory(e,p,n))return;
 // The first recorder Use only shows CTapeRecorder's hint while player+0x21D8
 // is 0 (1007C987); in co-op every recorder offers the save at once.
 if(!load<unsigned char>(p,0x21d8))store<unsigned char>(p,0x21d8,1);
 if(loadingCheckpoint){auto r=restoredPlayers.find(e);if(r==restoredPlayers.end()||r->second!=serial(e))return;}
 const auto identity=player_identity(e);if(identity.empty()||!client_ready(e))return;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 auto done=carryDone.find(e);
 if(done==carryDone.end()||done->second!=serial(e)){
  SavedPlayer now;if(!capture_player(e,now))return;   // dead, frozen in a scene...: wait
  carryDone[e]=serial(e);
  auto it=carryBank.find(identity);
  if(!loadingCheckpoint&&!departedPlayers.count(identity)&&it!=carryBank.end()){
   SavedPlayer saved=it->second;char line[220]{};
   // v0.6 (playtest 2 clip 24, log 2026-10-24: both players arrived on
   // s4_subway2st1 with "fuse.txt" left over from Chapter 4 and placed them in
   // its sockets at once, before the subway's own fuses): the subway chapter
   // starts without Chapter 4 fuses, as in single player.
   if(std::string(map_name()).rfind("s4_subway1st1",0)==0){int stripped=0;for(auto& item:saved.inventory)if(_strnicmp(item.c_str(),"fuse",4)==0||item.find("/fuse")!=std::string::npos){item.clear();++stripped;}
    if(stripped){char note[120]{};std::snprintf(note,sizeof(note),"[STORYCOOP_RC4] CARRY player=%d: %d Chapter 4 fuse(s) left behind\n",index,stripped);carry_log(note);}}
   if(restore_player(e,saved,false,false,false))std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CARRY player=%d kept inventory=%u weapons=%u tapes=%d phone=%d health=%.0f\n",index,static_cast<unsigned>(std::count_if(saved.inventory.begin(),saved.inventory.end(),[](const std::string& s){return !s.empty();})),static_cast<unsigned>(saved.weapons.size()),saved.tapes,saved.phoneMessage,saved.health);
   else std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] CARRY player=%d incomplete: %.150s\n",index,checkpointFailure.c_str());
   carry_log(line);
   SavedPlayer after;if(capture_player(e,after))now=std::move(after);
  }
  carryEntry[identity]=now;carryBank[identity]=std::move(now);return;
 }
 const float t=load<float>(globals,0);auto& next=carryNext[e];if(t<next)return;next=t+1.f;
 SavedPlayer latest;if(capture_player(e,latest,false))carryBank[identity]=std::move(latest);
}
// v0.5.7: the carried inventory (or a loaded save) has been applied to this player on this map.
bool carry_settled(Edict* e){auto d=carryDone.find(e);return d!=carryDone.end()&&d->second==serial(e);}
void equip_cancel(Edict* e);
// After the bag loss clear: no weapon is asked back, and the record is read again now.
void bag_wiped(Edict* e){equip_cancel(e);const auto identity=player_identity(e);SavedPlayer p;if(!identity.empty()&&capture_player(e,p,false)){carryEntry[identity]=p;carryBank[identity]=std::move(p);}}
// The last good record of a player who cannot be captured right now (the
// partner of the one saving is on a ladder, in a scene, reviving...).
const SavedPlayer* carry_record(Edict* e){const auto identity=player_identity(e);auto it=carryBank.find(identity);return identity.empty()||it==carryBank.end()?nullptr:&it->second;}

// --- The held weapon and dual wield come back (v0.5) --------------------------------
// Playtest: "players revert to the knife" after a level change or a load (logs:
// "RESTORE soft differences ... equipped" on every arrival). The restore's one
// cof_weaponswitch (CBasePlayer::SelectItem) is refused while the fresh player's
// deploy still holds m_flNextAttack and similar timers, so nothing switched. The
// wanted weapon is asked for again every half second (up to 6 s) until it is
// held; a saved dual wield is rebuilt the way the inventory does it
// (CBasePlayer::DualWield on the two weapons' inventory slots, dual_command).
// v0.5.7 (playtest 2 clip: "most of the time when change to next map he does
// that tweaking, doesn't matter with what item"; both host logs: "EQUIP ...
// could not take ... back" after nearly every map change): the switch every
// half second for 6 s re-deployed the weapon up to twelve times, and the phone
// (a toggle: each switch raises or lowers it) flapped. Now at most two asks,
// 1.5 s apart, and the phone is never asked for (it stays in the inventory).
constexpr int EquipTries=2;
struct EquipWant {int serial=-1;std::string equipped,dualA,dualB;float next=0,until=0;bool logged=false;int tries=0;};
std::unordered_map<Edict*,EquipWant> equipWants;
void equip_cancel(Edict* e){equipWants.erase(e);}
void equip_want(Edict* e,const SavedPlayer& saved){
 const bool dual=!saved.dualA.empty()&&!saved.dualB.empty();
 if(!globals||!live(e)||(saved.equipped.empty()&&!dual)||(!dual&&saved.equipped=="weapon_mobile"))return;const float now=load<float>(globals,0);
 equipWants[e]={serial(e),saved.equipped,saved.dualA,saved.dualB,now+1.f,now+6.f,false,0};
}
int weapon_card_slot(void* p,const std::string& weapon){
 for(int i=0;i<6;++i){const char* s=string_at(load<int>(p,0x2200+i*4));if(!s)continue;std::string c=s;const auto slash=c.find_last_of('/');if(slash!=std::string::npos)c=c.substr(slash+1);if(c.size()>4)c.resize(c.size()-4);if(_stricmp(c.c_str(),weapon.c_str())==0)return i+1;}
 return 0;
}
bool dual_rebuild(Edict* e,void* p,const std::string& a,const std::string& b);   // story-runtime.hpp
void equip_frame(Edict* e){
 auto it=equipWants.find(e);if(it==equipWants.end()||!globals)return;auto& w=it->second;
 if(w.serial!=serial(e)){equipWants.erase(it);return;}
 const float now=load<float>(globals,0);if(now<w.next)return;w.next=now+1.5f;
 void* p=nullptr;std::size_t n=0;SavedPlayer current;if(!player_memory(e,p,n)||!capture_player(e,current,false)){if(now>w.until)equipWants.erase(it);return;}
 const bool wantDual=!w.dualA.empty()&&!w.dualB.empty();
 Edict* held=nullptr;void* heldMemory=nullptr;const auto bits=load<std::uint32_t>(p,0x252c);const bool dualNow=bits&&item_object(bits,held,heldMemory)&&!std::strcmp(classname(held),"weapon_dualweapon");
 const bool done=wantDual?dualNow:current.equipped==w.equipped;
 char line[200]{};const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 if(done){std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] EQUIP player=%d holds %s again\n",index,wantDual?(w.dualA+" + "+w.dualB).c_str():w.equipped.c_str());carry_log(line);equipWants.erase(it);return;}
 if(now>w.until||w.tries>=EquipTries){std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] EQUIP player=%d could not take %s back (holds %s)\n",index,wantDual?"the dual wield":w.equipped.c_str(),current.equipped.empty()?"-":current.equipped.c_str());carry_log(line);equipWants.erase(it);return;}
 ++w.tries;if(wantDual)dual_rebuild(e,p,w.dualA,w.dualB);else helper_use("cof_weaponswitch","message",w.equipped,e);
}

// --- Nightmare in co-op (v0.5, D8) --------------------------------------------------
// The playthrough's difficulty is the "difficulty" cvar (world-runtime.hpp). On
// nightmare every player's own m_bNightmare (+0x1EE0) is set, so the native
// stamina rules (decomp player.cpp: slower recharge, longer out-of-breath) apply
// to everyone, as in single player. Other difficulties leave it alone.
constexpr std::size_t PlayerNightmare=0x1ee0;
bool nightmareLogged=false;
void nightmare_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;if(!scoped()||difficulty_now()!=4||!player_memory(e,p,n)||load<unsigned char>(p,PlayerNightmare))return;
 store<unsigned char>(p,PlayerNightmare,1);
 if(!nightmareLogged){nightmareLogged=true;log("[STORYCOOP_RC4] NIGHTMARE stamina rules on for every player\n");}
}

// --- Morphine trip and limping, as in single player (v0.5, D6/D7) ---------------------
// CSyringe::Inject (decomp CoF_Syringe.cpp) starts CBasePlayer::MorphineTrip
// only when !IsMultiplayer(): a second shot while the last one's window
// (m_fMorphineTripTime, +0x2274, = time + sv_morphinetriptime) is still open.
// It still sets the window in co-op, so a shot that renews an open window is
// seen here and the trip starts for THAT player (the one healed, own or
// partner's syringe) with MorphineTrip's own writes (decomp player.cpp): tripping
// +0x2278, trip end +0x227C (30 s), vomit time +0x2270, dream sound +0x440, blur
// +0x1EBC, a white ScreenFade (0.25 s, alpha 64) and Concuss 10. The game's own
// think then plays it out and StopMorphineTrip ends it. Offsets: the pinned
// hl.dll save table. m_bHasVomited has no save entry: a third of single-player
// trips skip the vomit, here by a vomit time past the trip's end.
// Limping: PostThink sets the physics key "low" below 25 health only when
// !IsMultiplayer(); pm_shared.c reads it on both sides (slower, uneven steps).
struct MorphineWatch {int serial=-1;float window=0;};
std::unordered_map<Edict*,MorphineWatch> morphineWatch;
void morphine_frame(Edict* e){
 void* p=nullptr;std::size_t n=0;auto v=vars(e);if(!scoped()||!globals||!v||!player_memory(e,p,n))return;
 auto& w=morphineWatch[e];const float window=load<float>(p,0x2274),now=load<float>(globals,0);
 if(w.serial!=serial(e)){w={serial(e),window};return;}
 const float before=w.window;w.window=window;
 if(!(window>before+0.5f)||before<=now||load<unsigned char>(p,0x2278)||load<int>(p,0x1ebc)||load<int>(v,0x174)!=0)return;
 store<unsigned char>(p,0x2278,1);store(p,0x227c,now+30.f);store(p,0x440,now);store(p,0x1ebc,1);
 const int roll=eng<std::int32_t(__cdecl*)(std::int32_t,std::int32_t)>(90)(0,2);store(p,0x2270,roll>0?now+static_cast<float>(eng<std::int32_t(__cdecl*)(std::int32_t,std::int32_t)>(90)(5,20)):now+100000.f);
 if(screenFadeMessage>0){eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,screenFadeMessage,nullptr,e);eng<void(__cdecl*)(int)>(50)(1024);eng<void(__cdecl*)(int)>(50)(0);eng<void(__cdecl*)(int)>(50)(0);for(int c:{255,255,255,64})eng<void(__cdecl*)(int)>(48)(c);eng<void(__cdecl*)()>(47)();}
 if(concussMessage>0){eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,concussMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(10);eng<void(__cdecl*)(int)>(48)(1);eng<void(__cdecl*)()>(47)();}
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MORPHINE player=%d took syringes too fast: trip started\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);
}
// v0.5.3 fix (Katz 2026-10-23 17:43 and 17:49 logs: Host_Error "EV_Precache: 'low' Precache can
// only be done in spawn functions", back to the menu): the engine table has
// pfnGetPhysicsKeyValue at 119 and pfnSetPhysicsKeyValue at 120 (hl.dll calls
// g_engfuncs 10222F48 + 120*4 with "low"); 121 is GetPhysicsInfoString and 122
// PrecacheEvent, which this used below 25 health since v0.5.
void limp_frame(Edict* e){
 auto v=vars(e);if(!scoped()||!v||!engine[119]||!engine[120]||std::strcmp(classname(e),"player"))return;
 const bool low=load<int>(v,0x174)==0&&load<float>(v,0x164)>0&&load<float>(v,0x164)<25.f;
 const char* now=eng<const char*(__cdecl*)(const Edict*,const char*)>(119)(e,"low");const bool is=now&&now[0]=='1';
 if(low!=is)eng<void(__cdecl*)(const Edict*,const char*,const char*)>(120)(e,"low",low?"1":"0");
}

// --- Story weapons for every player ------------------------------------------------
// A cof_giveitems fired by a scene gives its weapon to the scene's activator
// only (user: "the handgun after the cutscene is given to only the player that
// started the cutscene"; s4_apartment1 giveitem1 weapon_glock, s4_sewer2
// shotgun, s4_asylumb flashlight). The trigger's lookup of the entity is seen
// here; after the firing entity's think (or the next frame) the same entity is
// Used once for each other living player who does not own that weapon yet.
// Item cards (keys: spawnflag 1) and removals stay with the activator; the
// wake-up's sc3_native_* entities already reach everyone.
struct GiveFanout {Edict* entity=nullptr;int serial=-1;std::string weapon;};
std::vector<GiveFanout> givePending;int giveLines=0;
// v0.1: a player who is dead (waiting for a revive) when a scene gives its
// weapon is owed it, by identity: it is given once they are alive and their
// HUD is ready, on this map or a later one, unless they own it by then.
std::unordered_map<std::string,std::set<std::string>> giveOwed;
void give_reset(){givePending.clear();giveLines=0;}
void give_owed_reset(){giveOwed.clear();}
void give_line(const char* format,const char* weapon,int player){if(giveLines<60){++giveLines;char line[160]{};std::snprintf(line,sizeof(line),format,weapon,player);log(line);}}
void give_lookup(Edict* start,const char* key,const char* value,Edict* found){
 if(start||!scoped()||!key||!value||std::strcmp(key,"targetname")||!std::strncmp(value,"sc3_native_",11)||!live(found)||std::strcmp(classname(found),"cof_giveitems"))return;
 auto v=vars(found);if(!v||(load<int>(v,0x1a4)&1))return;
 const char* item=string_at(load<int>(v,0x1d8));if(!item||std::strncmp(item,"weapon_",7)||std::strlen(item)>48)return;
 for(const auto& g:givePending)if(g.entity==found&&g.serial==serial(found))return;
 if(givePending.size()<16)givePending.push_back({found,serial(found),item});
}
void give_flush(){
 if(givePending.empty()||!globals||!original[3])return;const auto pending=std::move(givePending);givePending.clear();
 for(const auto& g:pending){
  if(serial(g.entity)!=g.serial)continue;
  for(int i=1;i<=load<int>(globals,0x90);++i){
   auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;auto v=vars(e);
   if(!player_memory(e,p,n)||!v)continue;
   if(load<int>(v,0x174)!=0||load<float>(v,0x164)<=0){const auto identity=player_identity(e);if(!identity.empty()&&giveOwed[identity].insert(g.weapon).second)give_line("[STORYCOOP_RC4] STORYITEM %s owed to player=%d (dead now; given when revived)\n",g.weapon.c_str(),i);continue;}
   if(owns_weapon(e,g.weapon))continue;
   native<void(__cdecl*)(Edict*,Edict*)>(3)(g.entity,e);
   give_line("[STORYCOOP_RC4] STORYITEM %s also given to player=%d\n",g.weapon.c_str(),i);
  }
 }
}
void give_owed_frame(Edict* e){
 if(giveOwed.empty()||!scoped()||!live(e)||std::strcmp(classname(e),"player"))return;auto v=vars(e);
 if(!v||load<int>(v,0x174)!=0||load<float>(v,0x164)<=0||!client_ready(e))return;
 if(loadingCheckpoint){auto r=restoredPlayers.find(e);if(r==restoredPlayers.end()||r->second!=serial(e))return;}
 const auto identity=player_identity(e);auto it=giveOwed.find(identity);if(identity.empty()||it==giveOwed.end())return;
 const auto owed=std::move(it->second);giveOwed.erase(it);const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 for(const auto& weapon:owed){
  if(owns_weapon(e,weapon))continue;
  if(helper_use("cof_giveitems","message",weapon,e))give_line("[STORYCOOP_RC4] STORYITEM %s given to player=%d after the revive\n",weapon.c_str(),index);
  else give_line("[STORYCOOP_RC4] STORYITEM %s could not be given to player=%d after the revive\n",weapon.c_str(),index);
 }
}

// --- No player models inside scene cameras --------------------------------------------
// User: "simon model can be seen standing still while entering to get the
// handgun". While a client's view is a running trigger_camera (seen in
// SetupVisibility) no player entity is sent to that client; everyone else
// keeps seeing players normally.
bool camera_view(Edict* host){auto it=clientViews.find(host);if(it==clientViews.end())return false;auto view=camera_resolve(it->second);return view&&view!=host&&scene_camera(view);}
bool hide_player_from(Edict* e,Edict* host){return (scoped()||opening())&&live(e)&&live(host)&&std::strcmp(classname(e),"player")==0&&camera_view(host);}
// --- A nearby partner is always sent (v0.6) -------------------------------------------
// Playtest 2 clips 22 ("in this whole area everyone is invisible") and 28 (past
// the doctor's door). The game's AddToFullPack (decomp client.cpp) drops an
// entity that is EF_NODRAW, has no model, or fails ENGINE_CHECK_VISIBILITY
// against the viewer's PVS; nothing in the logs or the map data showed which.
// A living player within 2048 units of a living viewer who is not in a scene
// camera is now asked again with no PVS set (the documented "always passes"
// case), so a stale or wrong visibility result cannot hide the partner. The
// first time per pair and map the log says whether that made them visible.
std::set<std::pair<int,int>> forcedVisible;
bool player_force_visible(Edict* e,Edict* host){
 if(!(scoped()||opening())||e==host||!live(e)||!live(host)||std::strcmp(classname(e),"player")||std::strcmp(classname(host),"player")||!view_alive(e)||camera_view(host))return false;
 auto a=vars(e),b=vars(host);float d=0;for(int k=0;k<3;++k){const float x=load<float>(a,8+k*4)-load<float>(b,8+k*4);d+=x*x;}return d<2048.f*2048.f;
}
void player_forced_log(Edict* e,Edict* host,bool shown){
 const std::pair<int,int> key{eng<int(__cdecl*)(Edict*)>(71)(e),eng<int(__cdecl*)(Edict*)>(71)(host)};if(forcedVisible.size()>=64||!forcedVisible.insert(key).second)return;
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] VISIBLE player=%d to player=%d: PVS test refused, %s (effects %x)\n",key.first,key.second,shown?"sent anyway":"still not sent",load<unsigned>(vars(e),0x118));log(line);
}

// --- Staged cutscene actors (v0.2) ---------------------------------------------------
// v0.1 drew a fixed list of Simon models only inside scene views. The v0.1
// playtest still showed staged actors standing idle before their scenes, and
// the handgun scene's actor (c_apartment1 env_model scene1_1, tauruss.mdl) was
// never on that list. The cause is in the maps: single player hides every
// staged actor when the level starts, from the trigger the arriving player
// stands in (c_apartment1 trigger_once 343 -> multispawnhe, c_asylum1 2 ->
// multistartas, c_start 120, ...: env_customize m_iVisible 2 at 0 s). Co-op
// players arrive on cof_coop_spawn points (or the chain is gated to the first
// visit), so those hides never ran. The v0.2 maps (scripts/build-v02-maps.mjs)
// apply the same hides at load; no model is hidden by the adapter any more.

// --- Door music (v0.2) -------------------------------------------------------------
// User (v0.1 playtest): doors that change the music in single player stayed
// silent in co-op. CInterDoor (decomp CoF_InterDoor.cpp) stops the user's
// track at Use for a "silentroom" door (PlayerStopMP3(musicfadeout)) and in
// EndSequence calls PlayerSetMP3(music, spawnflags&2, 0). The converted co-op
// doors only set the sv_coop_music cvar, which the game reads in PlayerSpawn
// (multiplay_gamerules.cpp) and nowhere else, so nobody heard the change.
// Converted doors now run "sc_door_music <track|-> <loop> <fade>". As with the
// game's own multiplayer trigger_mp3 (triggers.cpp CTriggerMP3::Use), the
// music is shared: every player gets it and sv_coop_music is set for later
// spawns. A player whose current track is already that one is left alone, so
// the second player taking the same door does not restart it.
// PlayerSetMP3/PlayerStopMP3 (pinned 100EA5C0/100EA600) only write
// player+0x2310 (current track string), +0x2314 (send update), +0x2318 (stop
// fade), +0x231C (loop byte) and +0x2320 (fade-in); UpdateClientData sends it.
int doorMusicLines=0;
bool music_track(const char* s){
 if(!s)return false;const std::size_t n=std::strlen(s);if(n<5||n>60||_stricmp(s+n-4,".mp3"))return false;
 for(const char* c=s;*c;++c)if(!std::isalnum(static_cast<unsigned char>(*c))&&*c!='_'&&*c!='-'&&*c!='.'&&*c!='/')return false;
 return true;
}
void door_music(const char* track,int loop,float fade){
 if(!globals||!engine[60]||!engine[67])return;
 const bool stop=track&&!std::strcmp(track,"-");
 if(!scoped()||(!stop&&!music_track(track))){log("[STORYCOOP_RC4] MUSIC door request refused (not a story map or not an mp3 name)\n");return;}
 if(!std::isfinite(fade)||fade<0||fade>10)fade=stop?1.f:0.f;   // CInterDoor: PlayerStopMP3(1) / PlayerSetMP3(..., 0)
 const int name=stop?0:eng<int(__cdecl*)(const char*)>(67)(track);int changed=0,already=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto e=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))continue;
  const int current=load<int>(p,0x2310);const char* now=current?string_at(current):nullptr;
  if(stop){if(!current){++already;continue;}store(p,0x2310,0);store<unsigned char>(p,0x231c,0);store(p,0x2318,fade);store(p,0x2314,1);++changed;continue;}
  if(now&&!_stricmp(now,track)){++already;continue;}
  store(p,0x2310,name);store<unsigned char>(p,0x231c,loop?1:0);store(p,0x2320,fade);store(p,0x2314,1);++changed;
 }
 eng<void(__cdecl*)(const char*,const char*)>(60)("sv_coop_music",stop?"":track);
 if(doorMusicLines<200){++doorMusicLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MUSIC door %s%s: changed for %d player(s), %d already on it\n",stop?"silence":track,loop&&!stop?" (loop)":"",changed,already);log(line);}
}
void __cdecl door_music_command(){
 if(!engine[83])return;auto arg=[](int i){const char* s=eng<const char*(__cdecl*)(int)>(83)(i);return s?s:"";};
 const char* fade=arg(3);door_music(arg(1),std::atoi(arg(2)),*fade?static_cast<float>(std::atof(fade)):-1.f);   // no fade: the game's defaults
}

// --- Scene teleports reach every player (v0.2) ---------------------------------------
// User (v0.1 playtest): "the elevator cutscene (first floor to fourth floor)
// did not teleport the players, and players became invisible". c_apartment1
// multi_hiss_1: trigger_once *126 starts the cameras hisskam1/hisskam2 for
// every client (native trigger_camera Use loops over all players), while
// trigger_cofteleport *127 (decomp triggers.cpp CTriggerCofTeleport::Touch)
// moves only a player whose hull touches it to hiss1_land1. Single player has
// one player; in co-op a partner outside the car stayed on the first floor
// (out of sight of the other, who was upstairs) and two players inside the car
// were put on the same point, inside each other. The same holds for every
// scene-driven teleport that moves one activator (cof_teleport fired by a
// scene, trigger_cofteleport). While a player watches a scene camera (a
// camera lease from SetView) and their origin jumps more than 256 units in
// one frame, every other living player watching the same camera is moved next
// to them (find_landing + teleport_to, as "stuck"), and a player left inside
// the jumper's hull is moved aside. Door relocations and map arrivals happen
// without a scene camera and are not touched; the camera itself is untouched.
struct LastPlace {int serial=-1;float x=0,y=0,z=0,ignoreUntil=0;bool known=false;};
std::unordered_map<Edict*,LastPlace> lastPlaces;int sceneMoveLines=0;
// v0.4 (Katz, v0.3 playtest: P2 in a dark box after "the door to the right
// after the padlock door"; host log rc4-20261017-230225, s4_start_e0
// multiscene3): at 0.5 s cof_teleport mainteleport parks the ACTIVATOR in a
// closed staging room (teleportdes) while ckamera1 plays a Simon actor; at
// 15.4 s 2teleport brings the activator out to teleportdes2, in the same frame
// as the last camera ends (CCofTeleport::Use moves only pActivator, decomp
// effects.cpp). The partner was moved into the room with the activator, but
// the exit jump came when no scene camera was live any more, so the partner
// stayed in the dark box. The campaign audit (docs/V04_AUDIT_HANDOFF.md) has
// the same shape in 50+ scene chains: a teleport AT the camera end (attic,
// sewer2, city2, ending5, ...) or a return seconds later (subway2st3 multitscene2
// returns 16 s after its cameras). Two rules now cover the whole lifecycle:
//  - "watching": a player still sees the scene camera, or saw it within
//    SceneExitWindow seconds (the exit teleport of the same frame or just after);
//  - "follower": a partner this code moved for a scene keeps following that
//    activator's jumps while they still stand where they were put, until
//    SceneFollowGrace seconds after the activator's scene camera ended. If the
//    activator leaves the game first, the follower goes back to where they stood
//    before the scene (an exit fired for a gone activator never moves anyone).
// The 5 s follow grace and the return-home rule come from the agitated-murdock
// session's parallel fix (fe8ad2e); the window, the radius and the longer grace
// cover the chains the audit found.
constexpr float SceneExitWindow=1.5f,SceneFollowGrace=30.f,SceneFollowRadius=96.f;
struct SceneSeen {CameraRef camera{};float at=-1;};
std::unordered_map<Edict*,SceneSeen> sceneSeen;
struct SceneFollower {Edict* leader=nullptr;int leaderSerial=-1,serial=-1;float until=0,homeYaw=0;std::array<float,3> home{},placed{};};
std::unordered_map<Edict*,SceneFollower> sceneFollowers;
Edict* lease_camera(Edict* e){auto it=cameraLeases.find(e);if(it==cameraLeases.end()||it->second.playerSerial!=serial(e)||it->second.ended)return nullptr;auto c=camera_resolve(it->second.owner);return c&&camera_active(c)?c:nullptr;}
// The scene camera this player watches now, or the one that ended for them a moment ago.
Edict* scene_watched(Edict* e,float now){
 if(auto c=lease_camera(e))return c;auto it=sceneSeen.find(e);if(it==sceneSeen.end()||now-it->second.at>SceneExitWindow)return nullptr;
 auto c=camera_resolve(it->second.camera);return c&&!std::strcmp(classname(c),"trigger_camera")?c:nullptr;
}
float place_distance(Edict* o,const std::array<float,3>& at){auto q=vars(o);if(!q)return 1e9f;float d=0;for(int k=0;k<3;++k){const float c=load<float>(q,8+k*4)-at[k];d+=c*c;}return std::sqrt(d);}
void scene_teleport_frame(Edict* e){
 auto v=vars(e);if(!scoped()||!globals||!live(e)||!v||std::strcmp(classname(e),"player"))return;
 auto& last=lastPlaces[e];const float now=load<float>(globals,0),x=load<float>(v,8),y=load<float>(v,12),z=load<float>(v,16);
 const bool fresh=!last.known||last.serial!=serial(e);const float dx=x-last.x,dy=y-last.y,dz=z-last.z;
 const bool jumped=!fresh&&dx*dx+dy*dy+dz*dz>256.f*256.f&&now>=last.ignoreUntil;
 last.serial=serial(e);last.x=x;last.y=y;last.z=z;last.known=true;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);
 // This player as a follower: free again once they walked off, the grace ran
 // out, or back home when their activator left the game.
 if(auto f=sceneFollowers.find(e);f!=sceneFollowers.end()){
  const auto& s=f->second;
  if(s.serial!=serial(e)||now>s.until||place_distance(e,s.placed)>SceneFollowRadius)sceneFollowers.erase(f);
  else if(!live(s.leader)||serial(s.leader)!=s.leaderSerial||std::strcmp(classname(s.leader),"player")){
   const auto home=s.home;const float yaw=s.homeYaw;sceneFollowers.erase(f);std::array<float,3> at{};
   const bool back=view_alive(e)&&find_landing(home,e,at)&&teleport_to(e,at,yaw);if(back){last.x=at[0];last.y=at[1];last.z=at[2];last.ignoreUntil=now+1.f;}
   if(sceneMoveLines<100){++sceneMoveLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SCENE_TELEPORT player=%d: the player whose scene moved them left; %s\n",index,back?"back where they stood before the scene":"no landing at the old spot (say stuck)");log(line);}
   return;
  }
 }
 Edict* live_camera=lease_camera(e);
 if(live_camera){sceneSeen[e]={camera_ref(live_camera),now};for(auto& [o,f]:sceneFollowers)if(f.leader==e&&f.leaderSerial==serial(e))f.until=now+SceneFollowGrace;}
 if(!jumped||!view_alive(e))return;
 Edict* camera=scene_watched(e,now);
 auto following=[&](Edict* o){auto f=sceneFollowers.find(o);return f!=sceneFollowers.end()&&f->second.leader==e&&f->second.leaderSerial==serial(e)&&f->second.serial==serial(o)&&now<=f->second.until&&place_distance(o,f->second.placed)<=SceneFollowRadius;};
 int moved=0,aside=0,failed=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){
  auto o=eng<Edict*(__cdecl*)(int)>(72)(i);auto q=vars(o);if(o==e||!q||!live(o)||std::strcmp(classname(o),"player")||!view_alive(o))continue;
  const bool follower=following(o);if(!follower&&(!camera||scene_watched(o,now)!=camera))continue;
  const float ox=load<float>(q,8)-x,oy=load<float>(q,12)-y,oz=load<float>(q,16)-z;
  const bool inside=ox*ox+oy*oy<32.f*32.f&&std::fabs(oz)<72.f,distant=ox*ox+oy*oy+oz*oz>200.f*200.f;if(!inside&&!distant)continue;
  const std::array<float,3> before{load<float>(q,8),load<float>(q,12),load<float>(q,16)};const float beforeYaw=load<float>(q,0x54);
  std::array<float,3> at{};
  if(find_landing({x,y,z},o,at)&&teleport_to(o,at,load<float>(v,0x54))){
   auto& p=lastPlaces[o];p.serial=serial(o);p.x=at[0];p.y=at[1];p.z=at[2];p.known=true;p.ignoreUntil=now+1.f;inside?++aside:++moved;
   auto& f=sceneFollowers[o];if(!follower){f.home=before;f.homeYaw=beforeYaw;}f.leader=e;f.leaderSerial=serial(e);f.serial=serial(o);f.placed=at;f.until=now+SceneFollowGrace;
  }else ++failed;
 }
 if((moved||aside||failed)&&sceneMoveLines<100){++sceneMoveLines;char line[220]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SCENE_TELEPORT player=%d moved %s camera %d; partners moved=%d aside=%d failed=%d\n",index,live_camera?"during scene":camera?"as its scene ended,":"after its scene,",camera?eng<int(__cdecl*)(Edict*)>(71)(camera):-1,moved,aside,failed);log(line);}
}

// --- P2 idle camera diagnostics (v0.2, read only) -------------------------------------
// User: "P2's camera slightly jitters while standing still" (P1 is fine). The
// adapter writes no movement or view field of a player every frame, and the
// native PreThink/PostThink (decomp player.cpp) has no idle-time write to
// view_ofs or punchangle, so the writer is not known yet. For a player who is
// not the listen-server host (a remote client predicts its own movement) and
// stands still (no movement keys, speed < 1), the fields the client predicts
// or reads for its view are compared between the stages of each player
// command: CmdStart -> native PreThink -> adapter prethink -> [pmove] ->
// native PostThink -> adapter postthink -> (rest of the frame) -> next CmdStart.
// Every 10 s one line per changed field names the stage, the count and the
// largest change. Changes inside pmove are expected and not counted. Nothing
// is written to the game.
struct IdleField {const char* name;int offset;int floats;};
constexpr IdleField IdleFields[]={{"origin",0x08,3},{"velocity",0x20,3},{"basevelocity",0x2c,3},{"punchangle",0x68,3},{"view_ofs",0x178,3},{"fixangle",0xa0,-1},{"movetype",0x108,-1},{"friction",0x120,1},{"gravity",0x11c,1},{"flags",0x1a8,-1},{"groundentity",0x1a0,-1},{"waterlevel",0x1c4,-1},{"maxspeed",0x214,1},{"fov",0x218,1},{"weaponanim",0x21c,-1},{"bInDuck",0x224,-1},{"flDuckTime",0x230,1},{"iuser1",0x248,-1},{"iuser2",0x24c,-1},{"iuser3",0x250,-1},{"iuser4",0x254,-1},{"fuser1",0x258,1},{"fuser2",0x25c,1},{"fuser3",0x260,1},{"fuser4",0x264,1},{"vuser1",0x268,3},{"vuser2",0x274,3},{"vuser3",0x280,3},{"vuser4",0x28c,3}};
constexpr int IdleFieldCount=static_cast<int>(sizeof(IdleFields)/sizeof(IdleFields[0]));
constexpr const char* IdleStages[]={"between commands","native PreThink","adapter prethink","native PostThink","adapter postthink"};
// v0.5.6 NET_CADENCE (tester: "the connecting player has a small constant
// jittering, like shivering hands"): a remote client sends its commands in
// packets, cl_cmdrate (default 30, CL_MAIN.C) a second, and the server moves
// that player only when a packet arrives. The host draws the partner from a
// snapshot every host frame (fps_max 72 in the logs), so at 30 packets the
// partner moves in one of every 2-3 frames and holds still in the others.
// Per 10 s window: how many of the host's frames (netFrame, counted in
// start_frame) brought this player's commands, and the most in one frame.
int netFrame=0;
struct IdleProbe {int serial=-1;bool idle=false,have=false;std::array<std::array<std::uint32_t,3>,IdleFieldCount> last{};std::array<std::array<int,5>,IdleFieldCount> count{};std::array<std::array<float,5>,IdleFieldCount> most{};float windowStart=-1;int commands=0,idleCommands=0,msecMin=255,msecMax=0,frame=-1,frames=0,inFrame=0,mostInFrame=0;};
std::unordered_map<Edict*,IdleProbe> idleProbes;int idleLines=0;
void idle_reset(){idleProbes.clear();}
// stage 0 = CmdStart (compares with the end of the last command), 1..4 as IdleStages; -1 = after pmove (snapshot only).
void idle_stage(Edict* e,int stage){
 if(!scoped()||!globals||!live(e)||checkpoint_host(e)||idleLines>=400)return;auto v=vars(e);if(!v||std::strcmp(classname(e),"player"))return;
 auto& s=idleProbes[e];if(s.serial!=serial(e)){s={};s.serial=serial(e);}
 if(!s.idle){s.have=false;return;}
 for(int f=0;f<IdleFieldCount;++f){
  const auto& d=IdleFields[f];const int words=d.floats<0?1:d.floats;std::array<std::uint32_t,3> now{};
  for(int k=0;k<words;++k)now[k]=load<std::uint32_t>(v,d.offset+k*4);
  if(s.have&&stage>=0&&now!=s.last[f]){
   float delta=0;if(d.floats>0)for(int k=0;k<words;++k){float a=0,b=0;std::memcpy(&a,&now[k],4);std::memcpy(&b,&s.last[f][k],4);delta=(std::max)(delta,std::fabs(a-b));}else delta=1;
   ++s.count[f][stage];s.most[f][stage]=(std::max)(s.most[f][stage],delta);
  }
  s.last[f]=now;
 }
 s.have=true;
}
void idle_command(Edict* e,const void* command){
 if(!scoped()||!globals||!live(e)||checkpoint_host(e)||!command||idleLines>=400)return;auto v=vars(e);if(!v||std::strcmp(classname(e),"player"))return;
 auto& s=idleProbes[e];if(s.serial!=serial(e)){s={};s.serial=serial(e);}
 const auto* c=static_cast<const unsigned char*>(command);float move[3]{};std::memcpy(move,c+0x10,12);unsigned short buttons=0;std::memcpy(&buttons,c+0x1e,2);
 const float vx=load<float>(v,0x20),vy=load<float>(v,0x24),vz=load<float>(v,0x28);
 const bool idle=!(buttons&(1u|2u|4u|8u|16u|0x200u|0x400u|0x800u|0x1000u))&&move[0]==0&&move[1]==0&&move[2]==0&&vx*vx+vy*vy+vz*vz<1.f;
 const float now=load<float>(globals,0);if(s.windowStart<0)s.windowStart=now;
 ++s.commands;if(idle)++s.idleCommands;const int msec=c[2];s.msecMin=(std::min)(s.msecMin,msec);s.msecMax=(std::max)(s.msecMax,msec);
 if(s.frame!=netFrame){s.frame=netFrame;++s.frames;s.inFrame=0;}s.mostInFrame=(std::max)(s.mostInFrame,++s.inFrame);
 if(idle&&!s.idle)s.have=false;s.idle=idle;idle_stage(e,0);
 if(now-s.windowStart<10.f)return;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);int printed=0;
 if(idleLines<400){++idleLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] NET_CADENCE player=%d %d commands arrived in %d host frames (up to %d at once) in %.0f s\n",index,s.commands,s.frames,s.mostInFrame,now-s.windowStart);log(line);}
 for(int f=0;f<IdleFieldCount;++f)for(int k=0;k<5;++k){if(!s.count[f][k]||idleLines>=400)continue;++printed;++idleLines;char line[220]{};
  std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] IDLE_DIAG player=%d field=%s changed %d time(s) at %s, largest %.4f (10 s, %d of %d commands idle)\n",index,IdleFields[f].name,s.count[f][k],IdleStages[k],s.most[f][k],s.idleCommands,s.commands);log(line);}
 if(!printed&&s.idleCommands>0&&idleLines<400){++idleLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] IDLE_DIAG player=%d no change outside pmove (10 s, %d of %d commands idle, msec %d..%d)\n",index,s.idleCommands,s.commands,s.msecMin,s.msecMax);log(line);}
 const auto serialKeep=s.serial;const bool idleKeep=s.idle;const int frameKeep=s.frame;s={};s.serial=serialKeep;s.idle=idleKeep;s.frame=frameKeep;s.windowStart=now;
}

// --- "stuck": back to the partner ------------------------------------------------------
// A player who is shut out (a story wall, a door that closed behind the other
// player, a fall) types "stuck" in chat (say or say_team) and is moved to a
// clear spot beside the other living player. Not during a scene camera or
// while dead; once per 15 s.
std::unordered_map<Edict*,float> stuckUsed;
bool player_ready_for_move(Edict* e){auto v=vars(e);return v&&live(e)&&!std::strcmp(classname(e),"player")&&load<int>(v,0x174)==0&&load<float>(v,0x164)>0&&!(load<unsigned>(v,0x1a8)&0x100000u)&&!camera_view(e);}
bool move_beside(Edict* who,Edict* anchor){
 auto a=vars(anchor),w=vars(who);if(!a||!w)return false;std::array<float,3> at{};
 if(!find_landing({load<float>(a,8),load<float>(a,12),load<float>(a,16)},who,at))return false;
 return teleport_to(who,at,load<float>(a,0x54));
}
bool stuck_text(const char* s){
 if(!s)return false;while(*s==' '||*s=='"'||*s=='/'||*s=='!')++s;
 std::string t(s);while(!t.empty()&&(t.back()==' '||t.back()=='"'||t.back()=='\n'))t.pop_back();
 for(auto& c:t)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return t=="stuck"||t=="unstuck";
}
void stuck_command(Edict* e,const char* command){
 if(!command||(std::strcmp(command,"say")&&std::strcmp(command,"say_team"))||!(scoped()||opening())||!globals||!engine[80])return;
 if(!stuck_text(eng<const char*(__cdecl*)(int)>(83)(1)))return;
 auto print=[&](const char* text){eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);};
 const float now=load<float>(globals,0);auto& used=stuckUsed[e];if(used>now)used=0;
 if(used&&now-used<15.f){print("Wait a moment before\nusing stuck again.\n");return;}
 if(!player_ready_for_move(e)){print("Can't move you right now.\n");return;}
 Edict* partner=nullptr;
 for(int i=1;i<=load<int>(globals,0x90)&&!partner;++i){auto o=eng<Edict*(__cdecl*)(int)>(72)(i);if(o!=e&&player_ready_for_move(o))partner=o;}
 if(!partner){print("No partner to move to.\n");return;}
 used=now;const bool moved=move_beside(e,partner);print(moved?"Moved next to your partner.\n":"No free spot next to\nyour partner. Try again.\n");
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] STUCK player=%d %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),moved?"moved beside the partner":"no landing found");log(line);
}

// --- A fall kills only the one who falls (v0.5) ----------------------------------------
// Playtest 2026-10-21 (s4_subwaysick1): "when he fell to his death in the death
// trigger i also died". The v0.5 maps turn the cof_killplayer fall zones into
// trigger_hurt 1000 (build-v05-maps.mjs fallZones), which kills only the faller;
// the game keeps the others alive and the dead player revivable. The body would
// lie in the pit out of reach, so a player who dies inside a deadly trigger_hurt
// (dmg >= 100: these and the maps' own fall pits) is moved, dead, beside the
// nearest living player, once per death. Nobody alive: nothing moves (game over).
// v0.6 (playtest 2 clips 13 and 23: "if you fall then it black screens and
// bugs", "our favorite black box"; logs: FALL player=2 died in a fall zone on
// s4_subwaysick1/2): a 1000-damage fall gibs the player, the game then marks
// him m_bCantBeRevived (decomp combat.cpp GibMonster), and the native co-op
// respawn waits 60 s for SPACE (m_flCoopRespawnDelayTime) or 384 s
// (sv_auto_respawn_time), then strips every item (decomp player.cpp PreThink).
// Now the faller is respawned 2 s after the fall through the game's own
// auto-respawn (m_flDeadAutoReviveTime +0x1F2C set to now+2; MultiplayerKilled
// 100E92CB writes it, PreThink 100EF629 reads it), then gets back the inventory
// he had before the fall (his last carried record) and is moved beside the
// partner. Nobody alive: nothing happens (game over as before).
constexpr std::size_t PlayerAutoRevive=0x1f2c;
bool bossOver=false;bool boss_alive();   // boss section below
struct FallBack {int serial=-1;SavedPlayer record;bool have=false;Edict* partner=nullptr;int partnerSerial=-1;float due=-1;};
std::unordered_map<Edict*,FallBack> fallBacks;
void fall_back_frame(Edict* e){
 auto it=fallBacks.find(e);if(it==fallBacks.end()||!globals)return;auto& b=it->second;if(b.serial!=serial(e)){fallBacks.erase(it);return;}
 if(!view_alive(e))return;const float now=load<float>(globals,0);if(b.due<0){b.due=now+0.5f;return;}if(now<b.due)return;
 const bool restored=b.have&&restore_player(e,b.record,false,false,false);if(b.have)equip_want(e,b.record);
 Edict* partner=live(b.partner)&&serial(b.partner)==b.partnerSerial&&player_ready_for_move(b.partner)?b.partner:nullptr;
 for(auto o:connected_players())if(!partner&&o!=e&&player_ready_for_move(o))partner=o;
 const bool moved=partner&&move_beside(e,partner);
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] FALL player=%d back: inventory %s, %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),restored?"restored":b.have?"restore FAILED":"no record",moved?"beside the partner":"at the map's spawn");log(line);
 fallBacks.erase(it);
}
struct FallWatch {int serial=-1;bool dead=false;};
std::unordered_map<Edict*,FallWatch> fallWatch;
bool in_fatal_zone(const float* o){
 Edict* h=nullptr;
 for(int n=0;n<256;++n){h=find_next(h,"classname","trigger_hurt");if(!h)break;auto q=vars(h);if(!q||load<float>(q,0x1e4)<100.f)continue;
  bool inside=true;for(int k=0;k<3;++k){const float lo=load<float>(q,0xc4+k*4)-48.f,hi=load<float>(q,0xd0+k*4)+48.f;if(o[k]<lo||o[k]>hi)inside=false;}
  if(inside)return true;}
 return false;
}
void fall_frame(Edict* e){
 auto v=vars(e);if(!scoped()||!globals||!v||std::strcmp(classname(e),"player"))return;
 auto& w=fallWatch[e];if(w.serial!=serial(e))w={serial(e),false};
 const bool dead=load<int>(v,0x174)!=0||load<float>(v,0x164)<=0;if(!dead){w.dead=false;fall_back_frame(e);return;}if(w.dead)return;w.dead=true;
 const float o[3]={load<float>(v,8),load<float>(v,12),load<float>(v,16)};if(!in_fatal_zone(o))return;
 Edict* partner=nullptr;float best=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto p=eng<Edict*(__cdecl*)(int)>(72)(i);if(p==e||!player_ready_for_move(p))continue;auto q=vars(p);
  float d=0;for(int k=0;k<3;++k){const float x=load<float>(q,8+k*4)-o[k];d+=x*x;}if(!partner||d<best){partner=p;best=d;}}
 if(!partner)return;auto a=vars(partner);std::array<float,3> at{};
 const bool moved=find_landing({load<float>(a,8),load<float>(a,12),load<float>(a,16)},e,at);
 if(moved){for(int k=0;k<3;++k)store(v,0x20+k*4,0.f);eng<void(__cdecl*)(Edict*,const float*)>(28)(e,at.data());}
 // v0.6: back in 2 s with what he had (see FallBack).
 void* p=nullptr;std::size_t n=0;bool respawn=false;
 if(!bossOver&&!boss_alive()&&player_memory(e,p,n)&&n>PlayerAutoRevive+4){
  FallBack b;b.serial=serial(e);b.partner=partner;b.partnerSerial=serial(partner);if(auto r=carry_record(e)){b.record=*r;b.have=true;}
  fallBacks[e]=std::move(b);store(p,PlayerAutoRevive,load<float>(globals,0)+2.f);respawn=true;}
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] FALL player=%d died in a fall zone; body %s player=%d%s\n",eng<int(__cdecl*)(Edict*)>(71)(e),moved?"moved beside":"could not reach",eng<int(__cdecl*)(Edict*)>(71)(partner),respawn?"; respawns there in 2 s":"");log(line);
}

// --- No wall after a floor breaks (v0.5.7) ----------------------------------------------
// Playtest 2 clips: "theres a wall on that line that appears when the floor
// breaks" and "wall is still there" (Katz: "remove that invisible wall so that
// the player would be able to go down"). In single player a func_wall_toggle
// that starts off is switched on together with the breaking floor, so the one
// player cannot walk back; in co-op it shuts the partner out above the hole.
// Entity lumps: s4_down multicome -> breakfloor + spawnm2s; s4_fsewer1
// multistairs -> breakstairs + blockstairs. Those walls are removed when the
// map starts (entity_retire: no name, never solid), so the partner can drop
// down too.
struct FloorWall {const char* map;const char* wall;};
const FloorWall FloorWalls[]={{"s4_down","spawnm2s"},{"s4_fsewer1","blockstairs"}};
bool floorWallsDone=false;
std::vector<Edict*> skip_named(const std::string& name);   // below
void floor_walls_frame(){
 if(floorWallsDone||!scoped()||!globals)return;floorWallsDone=true;
 const std::string m=map_name();
 for(const auto& w:FloorWalls){if(m.rfind(w.map,0)!=0)continue;int n=0;
  for(auto e:skip_named(w.wall))if(!std::strcmp(classname(e),"func_wall_toggle")){entity_retire(e);++n;}
  char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] FLOORWALL %s: %d wall(s) %s removed (partners can follow down)\n",m.c_str(),n,w.wall);log(line);}
}
// --- Pickups for every player (v0.6.1) ------------------------------------------------
// Katz 2026-10-25: item spawns follow the number of players the game is hosted
// for, laid out side by side so they neither clip nor fall off; weapons too,
// except the scene weapon (the Glock). On the first visit of a map in this
// timeline (state_first_visit: a revisit or a loaded save keeps what was taken),
// every map pickup that is still in place and fires no target gets one copy per
// extra player slot (maxplayers - 1): ammo_*, weapon_syringe and the weapons
// lying in the map. Never copied: keys and story items (item_*), the phone, the
// knife and the Glock. Each copy goes on a 24-unit grid around the original
// (then 48), only where the spot is empty, has floor within 40 units below and
// a clear straight line from the original (no copy behind a wall or past a
// ledge); a spot that fails is skipped. Copies are runtime entities (engine
// CreateNamedEntity, the DLL's own KeyValue and Spawn), so the map files stay
// the same for any player count.
bool state_first_visit();   // state-runtime.hpp
bool copyable_pickup(const std::string& t){
 if(t.rfind("ammo_",0)==0)return true;
 if(t.rfind("weapon_",0)!=0)return false;
 return t!="weapon_glock"&&t!="weapon_mobile"&&t!="weapon_switchblade"&&t!="weapon_dualweapon";
}
Trace point_trace(const std::array<float,3>& a,const std::array<float,3>& b,Edict* skip){Trace t{};eng<void(__cdecl*)(const float*,const float*,int,int,Edict*,Trace*)>(34)(a.data(),b.data(),1,0,skip,&t);return t;}
bool copy_spot_clear(const std::array<float,3>& from,const std::array<float,3>& at,Edict* skip){
 const std::array<float,3> up{at[0],at[1],at[2]+8.f},src{from[0],from[1],from[2]+8.f};
 if(eng<int(__cdecl*)(const float*)>(45)(up.data())!=-1)return false;
 const std::array<float,3> past{src[0]+(up[0]-src[0])*1.5f,src[1]+(up[1]-src[1])*1.5f,up[2]};
 const auto line=point_trace(src,past,skip);if(line.allSolid||line.startSolid||line.fraction<1.f)return false;
 const auto floor=point_trace(up,{at[0],at[1],at[2]-40.f},skip);return !floor.allSolid&&floor.fraction<1.f;
}
bool itemCopiesDone=false;
void item_copies_frame(){
 if(itemCopiesDone||!scoped()||!globals||!engine[23]||!engine[67]||!engine[34]||!engine[45]||!original[1]||!original[6])return;itemCopiesDone=true;
 const int slots=(std::min)(4,load<int>(globals,0x90)),copies=slots-1;if(copies<1)return;
 if(loadingCheckpoint||!state_first_visit()){log("[STORYCOOP_RC4] ITEMS no copies: this map was visited before or a save is loading\n");return;}
 static const float ring[][2]={{24,0},{-24,0},{0,24},{0,-24},{24,24},{-24,24},{24,-24},{-24,-24},{48,0},{-48,0},{0,48},{0,-48},{48,24},{-48,24},{48,-24},{-48,-24}};
 int made=0,originals=0,skipped=0;
 for(std::size_t i=1;i<spawnRecords.size()&&made<240;++i){
  const SpawnRecord r=spawnRecords[i];if(!r.e||!r.pickup||r.dm||world_taken(r)||!copyable_pickup(r.type))continue;auto v=vars(r.e);if(!v)continue;
  ++originals;const std::array<float,3> o{load<float>(v,8),load<float>(v,12),load<float>(v,16)};int placed=0;
  for(const auto& d:ring){if(placed>=copies)break;const std::array<float,3> at{o[0]+d[0],o[1]+d[1],o[2]};if(!copy_spot_clear(o,at,r.e))continue;
   auto c=eng<Edict*(__cdecl*)(int)>(23)(eng<int(__cdecl*)(const char*)>(67)(r.type.c_str()));if(!live(c))break;
   char origin[96]{},angles[64]{},flags[24]{};std::snprintf(origin,sizeof(origin),"%.1f %.1f %.1f",at[0],at[1],at[2]);std::snprintf(angles,sizeof(angles),"%.1f %.1f %.1f",load<float>(v,0x50),load<float>(v,0x54),load<float>(v,0x58));std::snprintf(flags,sizeof(flags),"%d",load<int>(v,0x1a4)&~2048);
   for(const auto& [k,value]:std::initializer_list<std::pair<const char*,const char*>>{{"origin",origin},{"angles",angles},{"spawnflags",flags}}){KeyValue data{const_cast<char*>(r.type.c_str()),const_cast<char*>(k),const_cast<char*>(value),0};native<void(__cdecl*)(Edict*,KeyValue*)>(6)(c,&data);}
   if(native<int(__cdecl*)(Edict*)>(1)(c)!=0){if(live(c)){assist_unlink(c);eng<void(__cdecl*)(Edict*)>(22)(c);}break;}
   ++placed;++made;}
  if(placed<copies)++skipped;
 }
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] ITEMS %d copies of %d pickups for %d player slots (%d pickup(s) without room for every copy)\n",made,originals,slots,skipped);log(line);
}
// --- Everyone waits for everyone after a level change (v0.6) ------------------------------
// Playtest note: "you could make the host wait for all the other players to finish
// loading until the map starts". The players who were in the game when the level
// changed (carry_level_change counts them) are expected on the new map. Until all
// of them have a ready HUD (or 60 s pass), the ones
// already in are held: frozen, no damage, not targeted (FL_FROZEN 1<<12,
// FL_GODMODE 1<<6, FL_NOTARGET 1<<7; only the bits the adapter set are cleared,
// and FL_FROZEN stays while a scene camera holds the player), with "Waiting for
// players k/N" on screen.
constexpr unsigned FlGodmode=0x40u,FlNotarget=0x80u,FlFrozen=0x1000u;
struct LoadWait {bool armed=false;int expected=0;unsigned long long since=0;std::map<std::pair<Edict*,int>,unsigned> held;int shown=-1;};
LoadWait loadWait;
void load_wait_level_change(int players){loadWait={};if(players>1){loadWait.armed=true;loadWait.expected=players;}}
void load_wait_release(const char* why){
 for(const auto& [k,bits]:loadWait.held){auto e=k.first;auto v=live(e)&&serial(e)==k.second?vars(e):nullptr;if(!v)continue;unsigned clear=bits;if(camera_view(e))clear&=~FlFrozen;store(v,0x1a8,load<unsigned>(v,0x1a8)&~clear);if(engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,"");}
 char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LOADWAIT released (%s), %u player(s) were held\n",why,static_cast<unsigned>(loadWait.held.size()));log(line);loadWait={};
}
void load_wait_frame(){
 if(!loadWait.armed||!scoped()||!globals)return;const unsigned long long now=GetTickCount64();if(!loadWait.since)loadWait.since=now;
 const auto players=connected_players();std::vector<Edict*> ready;for(auto e:players)if(client_ready(e))ready.push_back(e);
 if(static_cast<int>(ready.size())>=loadWait.expected){load_wait_release("everyone is in");return;}
 // A loading player has no player entity yet, so someone who left during the change is only given up after 60 s.
 if(now-loadWait.since>60000){load_wait_release("60 s passed");return;}
 for(auto e:ready){auto v=vars(e);if(!v||!view_alive(e))continue;auto& bits=loadWait.held[{e,serial(e)}];const unsigned f=load<unsigned>(v,0x1a8);
  for(unsigned b:{FlGodmode,FlNotarget,FlFrozen})if(!(f&b)){bits|=b;store(v,0x1a8,load<unsigned>(v,0x1a8)|b);}}
 const int sec=static_cast<int>((now-loadWait.since)/1000);if(sec==loadWait.shown||!engine[80])return;loadWait.shown=sec;
 char text[80]{};std::snprintf(text,sizeof(text),"Waiting for players  %d/%d",static_cast<int>(ready.size()),loadWait.expected);
 for(auto e:ready)eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text);
 if(sec==0){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] LOADWAIT %s: waiting for %d player(s)\n",map_name(),loadWait.expected);log(line);}
}
// --- Arrival spots of level changes without co-op spawns (v0.6) ---------------------------
// Playtest 2 clips 16 ("when you exit out of the emergency exit this is where you
// spawn") and 20 ("you instantly spawn where you have to place the ladder"). A
// co-op alias without cof_coop_spawn points puts arriving players on the map's
// info_player_start. For the nine level changes below the original map's
// trigger_changelevel (same brush model) has a landmark, so single player arrives
// beside that landmark instead (scratchpad arrivals.mjs over the installed maps:
// e.g. s4_college1dark *47 -> s4_hamn3_e0, original landmark 2hcl2 at
// -1351 1705 -632 while info_player_start is at 1714 -714 14; s4_down *19 ->
// s4_apartment3sick_e0, landmark down_li while the start is at the ladder hole).
// Each arriving player is moved to a clear spot beside the landmark once their
// HUD is ready and their inventory has been applied.
struct Arrival {const char* from;const char* to;float at[3];};
const Arrival Arrivals[]={
 {"apartment3sick","s4_down",{315,-402,380}},{"apartmentsick","s4_apartment3_e4",{-1201,2448,68}},{"apartmentsick","s4_basement_e0",{-3245,2496,502}},
 {"basement","s4_apartmentsick_e1",{-3245,2496,502}},{"buscity","s4_city_e1",{-507,-1184,-54}},{"college1dark","s4_hamn3_e0",{-1351,1705,-632}},
 {"down","s4_apartment3sick_e0",{315,-402,380}},{"park3","s4_roofs2_e1",{2043,357,745}},{"roofs2","s4_roofs_e0",{2044,-1435,122}}};
struct PendingArrival {const Arrival* a=nullptr;std::set<std::pair<Edict*,int>> placed;float until=0;bool armed=false;};
PendingArrival pendingArrival;
std::string map_base(const std::string& m){std::string b=m.rfind("s4_",0)==0?m.substr(3):m;const auto e=b.rfind("_e");if(e!=std::string::npos&&e+2<b.size()&&std::all_of(b.begin()+e+2,b.end(),[](char c){return c>='0'&&c<='9';}))b.resize(e);return b;}
void arrival_level_change(const char* map){
 pendingArrival={};if(!map||!globals)return;const std::string from=map_base(map_name());
 for(const auto& a:Arrivals)if(from==a.from&&!_stricmp(map,a.to)){pendingArrival.a=&a;return;}
}
void arrival_frame(Edict* e){
 auto& p=pendingArrival;if(!p.a||!globals||!scoped())return;
 if(_stricmp(map_name(),p.a->to)){p={};return;}
 const float now=load<float>(globals,0);if(!p.armed){p.armed=true;p.until=now+60.f;}if(now>p.until){p={};return;}
 if(!view_alive(e)||!client_ready(e)||!carry_settled(e)||p.placed.count({e,serial(e)}))return;p.placed.insert({e,serial(e)});
 std::array<float,3> at{};const bool moved=find_landing({p.a->at[0],p.a->at[1],p.a->at[2]},e,at)&&teleport_to(e,at,load<float>(vars(e),0x54));
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] ARRIVAL player=%d from %s: %s\n",eng<int(__cdecl*)(Edict*)>(71)(e),p.a->from,moved?"moved beside the original landmark":"no clear spot there, stays at the map start");log(line);
}
// --- A door that locks behind the first player (v0.6) --------------------------------------
// Playtest 2 clip 14 ("door just gets locked as soon as the other player goes
// in", s4_hamn_e1 fudoor1). The converted door's gate is a multisource on the
// global sc4.door.<map>.<N>.enabled; a story env_global switches it off behind
// the player who went through (s4_hamn_e1: trigger *34 -> multisawyou ->
// env_global lofu1, triggermode 0, then the chase). Every such env_global with
// triggermode 0 is recorded at spawn; when one is fired, the players still on
// the near side are moved beside the player who reached the door's
// destination (sc4_cd_<N>_dest), so nobody is shut out.
struct DoorGlobal {int serial=-1;std::string global;int mode=-1;};
std::unordered_map<Edict*,DoorGlobal> doorGlobals;
// The player who went through: the one whose trigger touch fired the lock (pp_touch).
struct DoorLock {int door=0;float due=0;Edict* who=nullptr;int whoSerial=-1;};
std::vector<DoorLock> doorLocks;
void door_global_keyvalue(Edict* e,const KeyValue* kv){
 if(!kv||!kv->classname||!kv->key||!kv->value||std::strcmp(kv->classname,"env_global"))return;
 auto& d=doorGlobals[e];if(d.serial!=serial(e))d={serial(e),{},-1};
 if(!std::strcmp(kv->key,"globalstate"))d.global=kv->value;else if(!std::strcmp(kv->key,"triggermode"))d.mode=std::atoi(kv->value);
}
void door_lock_lookup(Edict* start,const char* key,const char*,Edict* found){
 if(start||!scoped()||!globals||!key||std::strcmp(key,"targetname")||!live(found))return;
 auto it=doorGlobals.find(found);if(it==doorGlobals.end()||it->second.serial!=serial(found)||it->second.mode!=0)return;
 const auto& g=it->second.global;if(g.rfind("sc4.door.",0)||g.size()<17||g.compare(g.size()-8,8,".enabled"))return;
 const auto dot=g.rfind('.',g.size()-9);if(dot==std::string::npos)return;const int door=std::atoi(g.c_str()+dot+1);if(door<=0)return;
 for(const auto& l:doorLocks)if(l.door==door)return;
 const float now=load<float>(globals,0);const bool touched=live(lastToucher)&&serial(lastToucher)==lastToucherSerial&&now>=lastTouchTime&&now-lastTouchTime<=1.f;
 doorLocks.push_back({door,now+0.5f,touched?lastToucher:nullptr,touched?lastToucherSerial:-1});
}
void door_lock_frame(){
 if(doorLocks.empty()||!globals)return;const float now=load<float>(globals,0);
 for(auto it=doorLocks.begin();it!=doorLocks.end();){
  if(now<it->due){++it;continue;}const DoorLock l=*it;const int door=l.door;it=doorLocks.erase(it);
  Edict* dest=nullptr;{const std::string n="sc4_cd_"+std::to_string(door)+"_dest";dest=find_next(nullptr,"targetname",n.c_str());}
  auto dv=live(dest)?vars(dest):nullptr;if(!dv)continue;
  auto dist=[](Edict* p,void* to){auto v=vars(p);float d=0;for(int k=0;k<3;++k){const float x=load<float>(v,8+k*4)-load<float>(to,8+k*4);d+=x*x;}return d;};
  // Only a lock fired by a player's own touch beyond the door counts (map-start locks move nobody).
  Edict* through=live(l.who)&&serial(l.who)==l.whoSerial&&view_alive(l.who)&&dist(l.who,dv)<900.f*900.f?l.who:nullptr;
  if(!through)continue;int moved=0,failed=0;
  for(auto p:connected_players())if(p!=through&&view_alive(p)&&dist(p,vars(through))>=300.f*300.f)move_beside(p,through)?++moved:++failed;
  if(moved||failed){char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DOORLOCK door %d locked behind player=%d: %d partner(s) moved through, %d without a free spot\n",door,eng<int(__cdecl*)(Edict*)>(71)(through),moved,failed);log(line);}
 }
}
// --- Out of the hanging train together (v0.5.7) ----------------------------------------
// Katz 2026-10-25: "once one of the players exit the train before it falls down
// all of the other players must be teleported to the safety where the escaped
// player is". s4_bridge_e1 (entity lump): reaching safety is trigger_once *109
// (target "destroyall", delay 0.5); destroyall starts multikuken2, which drops
// the train (func_train togetfalla) with whoever is still inside. The player
// who touches that trigger is remembered, and on the next frame every other
// living player is moved beside them (move_beside, a clear spot), before the
// half-second delay ends and the train falls.
struct TrainEscape {Edict* who=nullptr;int serial=-1;bool done=false;};
TrainEscape trainEscape;
void train_escape_touch(Edict* trigger,Edict* other){
 if(trainEscape.done||!globals||!live(trigger)||!live(other)||std::strcmp(classname(other),"player")||std::strcmp(classname(trigger),"trigger_once")||std::strncmp(map_name(),"s4_bridge",9))return;
 auto v=vars(trigger);const char* t=v?string_at(load<int>(v,0x1cc)):nullptr;if(!t||std::strcmp(t,"destroyall")||!view_alive(other))return;
 trainEscape={other,serial(other),true};
}
void train_escape_frame(){
 if(!trainEscape.who)return;const auto t=trainEscape;trainEscape.who=nullptr;if(!live(t.who)||serial(t.who)!=t.serial)return;
 int moved=0,failed=0;
 for(auto o:connected_players()){if(o==t.who||!view_alive(o))continue;auto a=vars(t.who),b=vars(o);float d=0;for(int k=0;k<3;++k){const float x=load<float>(a,8+k*4)-load<float>(b,8+k*4);d+=x*x;}if(d<160.f*160.f)continue;move_beside(o,t.who)?++moved:++failed;}
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] TRAIN player=%d reached safety: %d partner(s) moved beside them, %d without a free spot\n",eng<int(__cdecl*)(Edict*)>(71)(t.who),moved,failed);log(line);
}
// --- A death in a boss fight is a game over (v0.5.7) -----------------------------------
// Katz 2026-10-25: "if the sawer first boss kills the player, the player can no
// longer be revived and it crashes the game, so ... if 1 of the players dies in
// the boss fights ... say gameover and restart from checkpoint or a save".
// Evidence: host log 2026-10-23 23:19, CRASH hl.dll+0xC7E8D (read 0) right
// after the think of a third_person_interaction on s4_apartmentsick_e0. That
// is CHalfLifeMultiplay::DeathNotice +0x9D (decomp progress.csv): the Sawer's
// grab (CoF_ChainsawBoss.cpp: the boss removes itself and spawns a
// third_person_interaction) kills the player with Killed(pev) and no
// TakeDamage, so DeathNotice reads the NULL inflictor's classname; single
// player has no DeathNotice. Now, as soon as a grab starts, or a player dies
// while a boss is alive, every player gets a notice and the map restarts
// through the game-over path (checkpoint_game_over_check): this session's save
// of the map is loaded if there is one, otherwise everyone gets back what
// they had on arrival. The grab never reaches its kill frame.
const char* const BossClasses[]={"monster_bosschainsaw","monster_sewerboss","monster_roofboss","monster_doctorboss","monster_booksimon"};
void __cdecl change_level(char* map,char* landmark);   // adapter.cpp
bool boss_alive(){
 for(const char* cls:BossClasses){Edict* b=nullptr;for(int n=0;n<64;++n){b=find_next(b,"classname",cls);if(!b)break;auto v=vars(b);
  if(live(b)&&v&&load<int>(v,0x174)==0&&load<float>(v,0x164)>0&&!(load<unsigned>(v,0x118)&0x80u))return true;}}
 return false;
}
void boss_frame(){
 if(bossOver||!scoped()||!globals||!engine[6])return;
 Edict* grab=find_next(nullptr,"classname","third_person_interaction");const bool grabbing=grab&&live(grab);
 if(!grabbing){
  bool down=false;for(auto e:connected_players()){auto v=vars(e);if(v&&client_ready(e)&&(load<int>(v,0x174)!=0||load<float>(v,0x164)<=0))down=true;}
  if(!down||!boss_alive())return;
 }
 bossOver=true;forcedGameOver=true;
 static const unsigned char red[4]={220,40,40,255};
 for(auto e:connected_players())hud_text(e,4,-1.f,0.4f,red,4.f,"A player died in the boss fight.\nGame over: restarting.",0.1f);
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] BOSS %s on %s: game over for everyone, the map restarts\n",grabbing?"grab started":"player died",map_name());log(line);
 std::string map=map_name();change_level(map.data(),nullptr);
}
// --- Solo prologue (v0.5, Katz: "in prologue after readying up the player stays
// inside a black box") ----------------------------------------------------------------
// cof_campaign_story4 (RC3/RC4 build) waits for exactly two players twice: the
// ready-up (cof_lobby_start -> game_zone_player sc_begin_story counts the players
// into game_counter sc3_ready_count, limit 2, which starts the intro) and the end
// (game_zone_player sc4_final_zone -> sc4_final_count, limit 2, -> s4_nightmare).
// Alone, both counts stop at 1. The limit (game_counter LimitValue = pev->health)
// follows the players in the game, 1 to 4, every frame of that map.
int partyLimitLogged=-1;
void party_counters_frame(){
 if(!verified||!globals||std::strcmp(map_name(),"cof_campaign_story4"))return;
 const int n=(std::max)(1,(std::min)(4,static_cast<int>(connected_players().size())));
 for(const char* name:{"sc3_ready_count","sc4_final_count"}){
  Edict* c=nullptr;for(int k=0;k<4;++k){c=find_next(c,"targetname",name);if(!c)break;auto v=vars(c);if(v&&!std::strcmp(classname(c),"game_counter")&&load<float>(v,0x164)!=static_cast<float>(n))store(v,0x164,static_cast<float>(n));}
 }
 if(partyLimitLogged!=n){partyLimitLogged=n;char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] PROLOGUE ready and final counts wait for %d player(s)\n",n);log(line);}
}

// --- Waiting for players on s4_start and s4_nightmare (v0.5.3) ----------------------------
// Katz 2026-10-23: Chapter 1 alone stays in the black box and the scene never
// starts. Both maps start their opening scene from game_counter
// "game_playerjoin" (limit 2, RC3 map build), which the game Uses once per
// player when that player's HUD starts (decomp player.cpp UpdateClientData).
// Alone the count stops at 1. Once the first player is in, every player sees
// "Waiting for players 5..1"; then the scene starts with whoever is there (the
// counter's targets are Used with the first player as activator and its limit
// is raised, so a later join never starts it twice). Both players in: the
// counter fires on its own as before.
struct JoinWait {float since=-1;int shown=-1;bool done=false;};
JoinWait joinWait;
void join_wait_frame(){
 if(!verified||!globals||joinWait.done||(std::strcmp(map_name(),"s4_start")&&std::strcmp(map_name(),"s4_nightmare")))return;
 Edict* c=find_next(nullptr,"targetname","game_playerjoin");auto v=c?vars(c):nullptr;
 if(!v||std::strcmp(classname(c),"game_counter")||(load<unsigned>(v,0x1a8)&0x40000000u)||load<float>(v,0x168)>=load<float>(v,0x164)){joinWait.done=true;return;}   // fired (FL_KILLME) or full
 // The count itself says a player is in (the game counts a player when their HUD
 // starts); s4_nightmare is a passive map where the adapter tracks no clients.
 std::vector<Edict*> ready=connected_players();if(ready.empty()||load<float>(v,0x168)<1.f)return;
 const float now=load<float>(globals,0);if(joinWait.since<0)joinWait.since=now;
 const int left=5-static_cast<int>(now-joinWait.since);
 static const unsigned char white[4]={230,230,230,255};
 if(left>0){if(left!=joinWait.shown){joinWait.shown=left;char text[64]{};std::snprintf(text,sizeof(text),"Waiting for players  %d",left);for(auto e:ready)hud_text(e,3,-1.f,0.6f,white,1.5f,text,0.f);}return;}
 joinWait.done=true;store(v,0x164,99.f);
 for(auto e:ready)hud_text(e,3,-1.f,0.6f,white,1.5f,"",0.f);
 const char* target=string_at(load<int>(v,0x1cc));int used=0;
 if(target&&*target)for(Edict* t=find_next(nullptr,"targetname",target);t&&used<8;t=find_next(t,"targetname",target),++used)native<void(__cdecl*)(Edict*,Edict*)>(3)(t,ready.front());
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] JOINWAIT %s: scene started with %d player(s) after 5 s (%d target(s))\n",map_name(),static_cast<int>(ready.size()),used);log(line);
}

// --- Nightmare camera rooms (s4_nightmare) ----------------------------------------------
// Each right photo fires multispotN: doorN opens and the room walls change.
// The player in another room was left behind a closed way (user report). After
// a right photo every other player more than 256 units from the photographer
// (the player nearest the photographed spot) is moved beside them.
struct Regroup {bool pending=false;float origin[3]{};std::string spot;};
Regroup nightmareRegroup;
void regroup_lookup(Edict* start,const char* key,const char* value,Edict* found){
 if(start||!key||!value||std::strcmp(key,"targetname")||std::strncmp(value,"multispot",9)||!live(found)||!globals||std::strcmp(map_name(),"s4_nightmare"))return;
 auto v=vars(found);if(!v||std::strcmp(classname(found),"multi_manager"))return;
 nightmareRegroup.pending=true;for(int k=0;k<3;++k)nightmareRegroup.origin[k]=load<float>(v,8+k*4);nightmareRegroup.spot=value;
}
void regroup_frame(){
 if(!nightmareRegroup.pending||!globals)return;const auto r=nightmareRegroup;nightmareRegroup={};
 Edict* anchor=nullptr;float best=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(!player_ready_for_move(e))continue;auto v=vars(e);
  float d=0;for(int k=0;k<3;++k){const float x=load<float>(v,8+k*4)-r.origin[k];d+=x*x;}if(!anchor||d<best){anchor=e;best=d;}}
 if(!anchor)return;auto a=vars(anchor);
 for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(e==anchor||!player_ready_for_move(e))continue;auto v=vars(e);
  float d=0;for(int k=0;k<3;++k){const float x=load<float>(v,8+k*4)-load<float>(a,8+k*4);d+=x*x;}if(d<=256.f*256.f)continue;
  const bool moved=move_beside(e,anchor);if(moved&&engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,"Moved to your partner:\nthe next room opened.\n");
  char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] REGROUP %s: player=%d %s player=%d\n",r.spot.c_str(),i,moved?"moved beside":"could not reach",eng<int(__cdecl*)(Edict*)>(71)(anchor));log(line);}
}
// --- Steam profile names (v0.1) ------------------------------------------------------
// User test 2026-10-15: every name was "Unknown" (config.cfg's name) although
// both launchers passed "+name <Steam name>". The listen-server host reads the
// names from Steam itself, through the game's own steam_api.dll (the same
// SteamFriends() hl.dll calls; interface SteamFriends013, module SHA256
// pinned): GetPersonaName (slot 0) for the host, GetFriendPersonaName (slot 7)
// for everyone else, by the Steam ID the engine authenticated. A name Steam
// does not know yet ("" or "[unknown]") is asked again every 2 s; meanwhile
// the "sc_name" the launcher put in that player's userinfo is used; with
// neither, the player keeps the name they joined with. Only the shown name
// (userinfo "name", what the HUD list and the scoreboard read) changes:
// saves and the inventory carry stay keyed by the Steam ID.
constexpr const char* SteamApiPin="b55054a9d9287c704b8e0ad3acfef1ea5c3fa6982b20b7e36466dc76a6ad8925";
struct SteamApi {int tries=0;bool ready=false;void* (__cdecl* friends)()=nullptr;};
SteamApi steamApi;
void* steam_friends(){
 if(!steamApi.ready&&steamApi.tries<3){
  ++steamApi.tries;HMODULE api=GetModuleHandleW(L"steam_api.dll");
  if(api&&module_pinned(api,SteamApiPin)){steamApi.friends=reinterpret_cast<void*(__cdecl*)()>(GetProcAddress(api,"SteamFriends"));steamApi.ready=steamApi.friends!=nullptr;}
  if(steamApi.tries==3&&!steamApi.ready)log("[STORYCOOP_RC4] NAME Steam names off: steam_api.dll missing or a different build\n");
 }
 return steamApi.ready?steamApi.friends():nullptr;
}
// "STEAM_0:Y:Z" -> the 64-bit ID of that individual public account.
bool steam_id64(const std::string& auth,unsigned long long& out){
 if(auth.size()<11||auth.compare(0,6,"STEAM_")||auth[6]<'0'||auth[6]>'9'||auth[7]!=':'||(auth[8]!='0'&&auth[8]!='1')||auth[9]!=':')return false;
 unsigned long long z=0;for(std::size_t i=10;i<auth.size();++i){if(auth[i]<'0'||auth[i]>'9'||z>0x7fffffffull)return false;z=z*10+static_cast<unsigned>(auth[i]-'0');}
 if(z>0x7fffffffull)return false;out=0x0110000100000000ull|(z*2+static_cast<unsigned>(auth[8]-'0'));return true;
}
// Printable, without the userinfo/console separators; at most 31 bytes.
std::string clean_name(const char* s){
 std::string out;if(!s)return out;
 for(;*s&&out.size()<31;++s){const unsigned char c=static_cast<unsigned char>(*s);if(c<32||c==127||c=='\\'||c=='"'||c==';'||c=='%')continue;out.push_back(static_cast<char>(c));}
 while(!out.empty()&&out.back()==' ')out.pop_back();while(!out.empty()&&out.front()==' ')out.erase(out.begin());
 return out=="[unknown]"||out=="Unknown"?std::string():out;
}
std::string steam_name(Edict* e){
 void* f=steam_friends();if(!f)return {};
 auto table=*reinterpret_cast<void***>(f);if(!table)return {};
 if(checkpoint_host(e))return clean_name(reinterpret_cast<const char*(__thiscall*)(void*)>(table[0])(f));
 unsigned long long id=0;if(!steam_id64(steam_identity(e),id))return {};
 return clean_name(reinterpret_cast<const char*(__thiscall*)(void*,unsigned long long)>(table[7])(f,id));
}
struct NameState {float next=0;std::string shown;int serial=-1;};
std::map<int,NameState> nameStates;
void name_frame(Edict* e){
 if(!globals||!(scoped()||opening())||!live(e)||std::strcmp(classname(e),"player")||!engine[106]||!engine[107]||!engine[109])return;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e);auto& s=nameStates[index];const float now=load<float>(globals,0);
 if(s.serial!=serial(e)){s={};s.serial=serial(e);}
 if(now<s.next)return;s.next=now+2.f;
 auto buffer=eng<char*(__cdecl*)(Edict*)>(106)(e);if(!buffer)return;
 std::string wanted=steam_name(e);const char* source="Steam";
 if(wanted.empty()){wanted=clean_name(eng<char*(__cdecl*)(char*,char*)>(107)(buffer,const_cast<char*>("sc_name")));source="the launcher";}
 if(wanted.empty())return;
 const char* current=eng<char*(__cdecl*)(char*,char*)>(107)(buffer,const_cast<char*>("name"));
 if(current&&wanted==current)return;
 eng<void(__cdecl*)(int,char*,char*,char*)>(109)(index,buffer,const_cast<char*>("name"),const_cast<char*>(wanted.c_str()));
 if(s.shown!=wanted){s.shown=wanted;char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] NAME player=%d shown as %s (from %s)\n",index,wanted.c_str(),source);log(line);}
}

// --- Locked sound on story-locked co-op doors (v0.1) ----------------------------------
// User: every locked door must sound locked, scripted locks included. A
// converted co-op door is a func_button whose target is its presentation
// manager cof_story_present<N>; a story lock is that button's master (a
// multisource on an env_global latch). CoF's CBaseButton::ButtonActivate
// (decomp buttons.cpp) then returns without a sound: only "locked_sound" (an
// index into ButtonSound) fills m_ls.sLockedSound for PlayLockSounds. Before
// the native Spawn (its Precache reads the index) each such button gets
// locked_sound 12 = buttons/latchlocked1.wav, the game's latch sound. An
// unlocked or ungated button never plays it. Key doors keep the native
// inter_door lockedsnd through the key-door bridge.
int lockedSoundButtons=0;
void story_door_locked_sound(Edict* e){
 if(!scoped()||!original[6]||std::strcmp(classname(e),"func_button"))return;auto p=vars(e);if(!p)return;
 const char* target=string_at(load<int>(p,0x1cc));if(!target||std::strncmp(target,"cof_story_present",17))return;
 static char type[]="func_button",key[]="locked_sound",value[]="12";KeyValue data{type,key,value,0};
 native<void(__cdecl*)(Edict*,KeyValue*)>(6)(e,&data);if(data.handled)++lockedSoundButtons;
}
void story_door_report(){
 if(!scoped())return;char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DOORS locked sound ready on %d co-op door button(s)\n",lockedSoundButtons);log(line);lockedSoundButtons=0;
}

// --- Drowned victim (v0.3) ----------------------------------------------------------
// CCrazyBitch::MonsterThink (hl 10055860, cof_monstercrazybitch.cpp) checks the
// player held in m_hEnemy (gun able to take the attack), then a second later
// starts the attack on CBaseEntity::Instance(INDEXENT(1)): always player 1 in
// co-op. While a monster_crazybitch runs its own think, INDEXENT(1) answers
// with that monster's enemy when the enemy is a live player; every other call
// keeps the engine answer. The victim's own gun then counts the victim's own
// button presses (CGlock ItemPostFrame), so the partner keeps control, and the
// native Killed/StopSuicide cleanup uses the same pSuicidePlayer.
// CBaseMonster m_hEnemy is the EHANDLE at private +0x184 (edict), +0x188 (serial).
Edict* drownedThinker=nullptr;
std::map<Edict*,int> drownedVictims;int drownedLines=0;
Edict* drowned_enemy(Edict* monster){
 void* p=eng<void*(__cdecl*)(Edict*)>(64)(monster);if(!p)return nullptr;
 auto enemy=load<Edict*>(p,0x184);if(!enemy||!live(enemy)||serial(enemy)!=load<int>(p,0x188)||std::strcmp(classname(enemy),"player"))return nullptr;
 auto v=vars(enemy);if(!v||load<int>(v,0x174)!=0||load<float>(v,0x164)<=0)return nullptr;return enemy;
}
void drowned_begin(Edict* e){drownedThinker=live(e)&&!std::strcmp(classname(e),"monster_crazybitch")?e:nullptr;}
void drowned_end(){drownedThinker=nullptr;}
Edict* drowned_index(int index,Edict* engineAnswer){
 if(index!=1||!drownedThinker)return engineAnswer;
 auto enemy=drowned_enemy(drownedThinker);auto victim=enemy?enemy:engineAnswer;
 const int slot=victim?eng<int(__cdecl*)(Edict*)>(71)(victim):0;
 auto& last=drownedVictims[drownedThinker];
 if(last!=slot&&drownedLines<60){++drownedLines;char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DROWNED monster %d attack victim player=%d (%s)\n",eng<int(__cdecl*)(Edict*)>(71)(drownedThinker),slot,enemy?"its enemy":"no live player enemy; engine player 1");log(line);}
 last=slot;return victim;
}
// v0.3 (s4_roofs2): CRoofBoss::MonsterThink (decomp CoF_RoofBoss.cpp) turns to,
// attacks only while it sees, and hands its head the FIRST player. During its
// own think "the player" is its live player enemy (the same m_hEnemy), so it
// fights whichever player it is after; without one, the engine answer.
Edict* roofboss_enemy(Edict* e){return live(e)&&!std::strcmp(classname(e),"monster_roofboss")?drowned_enemy(e):nullptr;}
void drowned_reset(){drownedThinker=nullptr;drownedVictims.clear();drownedLines=0;}
// v0.5 (C2, the chapter 8 valves): CCoFStrangle::Use (decomp CoF_Strangle.cpp)
// strangles the FIRST player, while the scene is started by whoever turned the
// last valve. A multi_manager carries that activator (CMultiManager
// m_hActivator, EHANDLE +0x184/+0x188 in the pinned hl.dll); during the think of
// a manager that fires a cof_strangle, "the player" is that activator when it is
// a live player.
Edict* strangle_activator(Edict* e){
 if(!live(e)||std::strcmp(classname(e),"multi_manager"))return nullptr;void* p=engine[64]?eng<void*(__cdecl*)(Edict*)>(64)(e):nullptr;if(!p||load<void*>(p,4)!=vars(e))return nullptr;
 const int n=load<int>(p,0xe0);if(n<=0||n>16)return nullptr;bool strangle=false;
 for(int i=0;i<n&&!strangle;++i){const char* t=string_at(load<int>(p,0xec+i*4));if(!t||!*t)continue;Edict* x=nullptr;for(int k=0;k<8;++k){x=find_next(x,"targetname",t);if(!x)break;if(!std::strcmp(classname(x),"cof_strangle")){strangle=true;break;}}}
 if(!strangle)return nullptr;auto a=load<Edict*>(p,0x184);
 return a&&live(a)&&serial(a)==load<int>(p,0x188)&&!std::strcmp(classname(a),"player")&&view_alive(a)?a:nullptr;
}
// --- Two-half items (v0.3, wiki puzzle list) -----------------------------------------
// The asylum number plate (leftplate*/rightplate*) and the forest scissors
// (scissors1/scissors2/motelscissors) are single pickups that must be combined
// in ONE inventory (CBasePlayer::InventoryCombine, client command
// "inventorycombine"; inventoryitems/*.txt "combined_with"). In co-op each
// player can pick up one half and nobody can combine. When one player holds a
// half and another holds its partner, the half moves to the partner's holder
// through the game's own cof_giveitems (spawnflag 1, by card path) and
// cof_clearitems (by card path); both are told. Pairs come from the game's
// files, not a list here; weapon combinations (taclight, phone battery) never
// move because each player has their own weapon.
struct HalfCard {std::string path,partner;};
std::vector<HalfCard> halfCards;bool halfCardsRead=false;float halfNext=0;int halfLines=0;
std::string half_value(const std::string& text,const char* key){
 const auto k=text.find(std::string("\"")+key+"\"");if(k==std::string::npos)return {};const auto d=text.find("data",k);if(d==std::string::npos)return {};
 const auto a=text.find('"',d);const auto b=a==std::string::npos?a:text.find('"',a+1);return b==std::string::npos?std::string():text.substr(a+1,b-a-1);
}
void half_read(){
 halfCardsRead=true;halfCards.clear();char dir[MAX_PATH]{};if(!engine[99])return;eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return;
 std::error_code error;std::map<std::string,std::string> byName;std::vector<std::pair<std::string,std::string>> wants;
 for(const auto& item:std::filesystem::directory_iterator(std::filesystem::path(dir)/L"inventoryitems",error)){
  if(item.path().extension()!=L".txt")continue;std::ifstream f(item.path());std::string text((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());if(text.size()>65536)continue;
  const std::string path="inventoryitems/"+item.path().filename().string(),name=half_value(text,"map_name"),with=half_value(text,"combined_with");
  if(!name.empty())byName[name]=path;if(!with.empty())wants.push_back({path,with});
 }
 for(const auto& [path,with]:wants){auto it=byName.find(with);if(it!=byName.end())halfCards.push_back({path,it->second});}
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] COMBINE %u two-half item(s) known from inventoryitems\n",static_cast<unsigned>(halfCards.size()));log(line);
}
// A full inventory makes cof_giveitems drop an item_key on the floor instead: never hand over then.
bool free_slot(Edict* e){void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))return false;for(std::size_t i=0;i<6;++i){const char* s=string_at(load<int>(p,0x2200+i*4));if(!s||!*s)return true;}return false;}
bool holds_card(Edict* e,const std::string& path){void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))return false;for(std::size_t i=0;i<6;++i){const char* s=string_at(load<int>(p,0x2200+i*4));if(s&&!_stricmp(s,path.c_str()))return true;}return false;}
void half_frame(){
 if(!scoped()||!globals)return;const float now=load<float>(globals,0);if(now<halfNext&&now>=halfNext-1.f)return;halfNext=now+0.5f;
 if(!halfCardsRead)half_read();if(halfCards.empty())return;
 const auto players=connected_players();
 for(const auto& h:halfCards)for(auto from:players){
  if(!holds_card(from,h.path)||holds_card(from,h.partner))continue;
  for(auto to:players){
   if(to==from||!holds_card(to,h.partner)||holds_card(to,h.path)||!client_ready(to)||!client_ready(from)||!free_slot(to))continue;
   const bool given=helper_use_pairs("cof_giveitems",{{"message",h.path},{"spawnflags","1"}},to)&&holds_card(to,h.path);
   const bool cleared=given&&helper_use("cof_clearitems","message",h.path,from)&&!holds_card(from,h.path);
   if(given&&engine[80]){eng<void(__cdecl*)(Edict*,int,const char*)>(80)(to,1,"Your partner's half is now in your inventory.\nCombine the two halves.\n");eng<void(__cdecl*)(Edict*,int,const char*)>(80)(from,1,"Your partner has the other half.\nYour half went to them.\n");}
   if(halfLines<40){++halfLines;char line[220]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] COMBINE %s moved from player=%d to player=%d (holds %s): given=%d cleared=%d\n",h.path.c_str(),eng<int(__cdecl*)(Edict*)>(71)(from),eng<int(__cdecl*)(Edict*)>(71)(to),h.partner.c_str(),given?1:0,cleared?1:0);log(line);}
   break;
  }
 }
}
// --- Item-use and story-door evidence (v0.3, read-only) ------------------------------
// User (v0.2 playtest, s4_apartment5 balcony): the rope did not attach and E on
// the climb spot played a locked sound. The traced chain (c_apartment5 /
// s4_apartment5_e1): item_key ropeheh (inventoryitems/rope.txt, map_name
// "rope") -> CBasePlayer::InventoryUse (decomp player.cpp, binary 100E4410,
// matched): a trace of 80 units from the eyes, then every entity whose bbox
// centre is within 64 of the player's origin; a func_button with m_isLocked and
// m_iszLockedBy == "rope" is ropebutton #268 (*68, SF_BUTTON_NOT_SOLID 128,
// so only that 64-unit sphere finds it) -> CBaseButton::UnlockDoor (consumes
// the rope, "You have placed the rope.", spawnflag 2048 ButtonUse) ->
// multiunlockedrope: rope_vis, killropebutton, ropelock. ropelock toggles the
// climb door: SP cof_interdooronoff -> inter_door ropelocked #142; co-op
// trigger_relay -> sc4_cd_142_router -> multisource sc4_cd_142_gate, the master
// of the converted climb button *45 (fade, cof_teleport to ropexit2). The bath
// trigger_once *70 fires ropelock first (locks the climb until the rope).
// A static comparison found every step identical to single player, so these
// lines record what actually happens: each inventory use with the cards, the
// sounds the native code played and the lock entities in reach; each gate
// toggle; each locked press on a converted door. Nothing is changed.
struct LockRecord {int serial=-1;std::string type,key;};
std::unordered_map<Edict*,LockRecord> lockRecords;
struct ItemUseCapture {bool active=false;std::vector<std::string> sounds;} itemUse;
int itemUseLines=0,gateLines=0,lockedLines=0;
std::unordered_map<std::string,int> gateToggles;
void evidence_reset(){lockRecords.clear();itemUse={};itemUseLines=gateLines=lockedLines=0;gateToggles.clear();}
// From the KeyValue wrapper: entities a carried item can unlock.
void evidence_keyvalue(Edict* e,const KeyValue* kv){
 if(!kv||!kv->key||!kv->value||!kv->classname||std::strcmp(kv->key,"lockedby")||!*kv->value)return;
 lockRecords[e]={serial(e),kv->classname,kv->value};
}
void evidence_sound(Edict* e,const char* sample){
 if(!sample)return;
 if(itemUse.active&&itemUse.sounds.size()<8)itemUse.sounds.emplace_back(sample);
 if(scoped()&&!std::strcmp(sample,"buttons/latchlocked1.wav")&&live(e)&&!std::strcmp(classname(e),"func_button")&&lockedLines<40){
  ++lockedLines;char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DOOR locked press on %.60s (story lock not open yet)\n",targetname(e));log(line);
 }
}
void evidence_lookup(Edict* start,const char* key,const char* value){
 if(start||!scoped()||!key||!value||std::strcmp(key,"targetname")||std::strncmp(value,"sc4_cd_",7))return;
 const std::size_t n=std::strlen(value);if(n<8||std::strcmp(value+n-7,"_router"))return;
 const int count=++gateToggles[value];if(gateLines>=60)return;++gateLines;
 char line[200]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] DOOR gate %.60s toggled (%d time(s) this map; odd = open)\n",value,count);log(line);
}
bool item_command(const char* c){return c&&(!std::strcmp(c,"inventoryuse")||!std::strcmp(c,"inventoryequip"));}
std::vector<std::string> item_cards(Edict* e){
 std::vector<std::string> out;void* p=nullptr;std::size_t n=0;if(!player_memory(e,p,n))return out;
 for(std::size_t i=0;i<6;++i){const char* s=string_at(load<int>(p,0x2200+i*4));std::string t=s?s:"";out.push_back(t.substr(t.find_last_of('/')==std::string::npos?0:t.find_last_of('/')+1));}
 return out;
}
void evidence_item_begin(const char* command){if(scoped()&&item_command(command)){itemUse={};itemUse.active=true;}}
void evidence_item_end(Edict* e,const char* command,const char* slot,const std::vector<std::string>& before){
 if(!itemUse.active)return;const auto sounds=itemUse.sounds;itemUse={};
 auto v=vars(e);if(!v||itemUseLines>=80)return;++itemUseLines;
 auto join=[](const std::vector<std::string>& list){std::string s;for(const auto& c:list)if(!c.empty())s+=(s.empty()?"":",")+c;return s;};
 // entvars absmin +0xC4, absmax +0xD0 (the engine's sphere search uses their centre).
 std::string reach;const float o[3]={load<float>(v,8),load<float>(v,12),load<float>(v,16)};
 for(const auto& [ent,r]:lockRecords){
  if(reach.size()>320)break;auto q=vars(ent);const bool gone=!q||serial(ent)!=r.serial;float d=0;
  if(!gone)for(int k=0;k<3;++k){const float c=(load<float>(q,0xc4+k*4)+load<float>(q,0xd0+k*4))*0.5f-o[k];d+=c*c;}
  if(!gone&&d>200.f*200.f)continue;
  char item[200]{};std::snprintf(item,sizeof(item),"%s%.40s(%s lockedby=%.24s %s)",reach.empty()?"":" ",gone?"-":targetname(ent),r.type.c_str(),r.key.c_str(),gone?"removed":(std::to_string(static_cast<int>(std::sqrt(d)))+"u from centre").c_str());reach+=item;
 }
 char line[1000]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] ITEMUSE player=%d %s %s cards=[%s] after=[%s] sounds=[%s] locks-in-reach=[%s] origin=%.0f,%.0f,%.0f\n",eng<int(__cdecl*)(Edict*)>(71)(e),command,slot?slot:"-",join(before).c_str(),join(item_cards(e)).c_str(),join(sounds).c_str(),reach.c_str(),o[0],o[1],o[2]);log(line);
}
// --- "Only host can save the game" (v0.2) ---------------------------------------------
// User (v0.1 playtest): P2 saw the restriction just by looking at a recorder.
// Only a partner's actual recorder Use shows it, to that partner alone
// (recorder_end in checkpoint-runtime.hpp); standing near or looking at a
// recorder shows nothing. The v0.1 proximity notice is gone.

// --- Vote to skip a cutscene (v0.4, Katz 2026-10-18) ---------------------------------
// While a scene camera holds the players, SPACE (the jump button in the usercmd,
// IN_JUMP 2) is a skip vote; "SPACE to skip k/N" shows bottom-left to everyone
// for a few seconds when the scene starts and after each vote. When every
// player in the game (connected, HUD ready) has voted, the scene is
// FAST-FORWARDED, never cut: each running multi_manager of the scene gets the
// think it would have had at the end (m_startTime moved back, then the game's
// own ManagerThink fires every remaining target in order with its own
// activator: the exit teleport, grants, music, fades, SMS), the managers and
// delayed relays that this starts are fast-forwarded the same way, and the
// scene cameras still on are toggled off once (their own end path gives every
// player view and controls back; one Use, never per player). The exit teleport
// then moves partners through scene_teleport_frame as in a watched scene.
// A scene is offered only when everything it still fires is a state change
// that can run at once (allowlist); trains, scripted sequences, model
// cutscenes, strangles, credits, boss bars, zone checks and monster makers
// depend on time, so those scenes are not skippable (logged once).
// CMultiManager layout (hl.dll: GetState 1010B5A0 reads +0xDC, ManagerUse
// 1010E190 reads m_sMaster +0x174; class size 0x190): m_iState +0xDC,
// m_cTargets +0xE0, m_index +0xE4, m_startTime +0xE8, m_iTargetName[16] +0xEC,
// m_flTargetDelay[16] +0x12C, m_iMode +0x178. STATE_TURN_ON 1, STATE_ON 2.
constexpr int MmState=0xdc,MmCount=0xe0,MmIndex=0xe4,MmStart=0xe8,MmNames=0xec,MmDelays=0x12c,MmMode=0x178;
struct SkipManager {Edict* e=nullptr;int serial=-1;};
struct SkipScene {bool active=false,allowed=false;float nextShow=0;std::vector<SkipManager> managers;std::set<std::string> names;std::set<std::pair<Edict*,int>> votes;std::string label,why;};
SkipScene skipScene;int skipLines=0;struct SkipAfter {std::set<std::string> names,ended;float until=0;} skipAfter;std::unordered_map<Edict*,std::pair<int,bool>> skipJump;
bool skip_class(const std::string& c){
 static const std::set<std::string> ok{"trigger_camera","env_fade","ambient_generic","env_model","env_customize","ambient_fmodstream","cof_teleport","trigger_relay","multi_manager","subtitle_linechange","env_dynlight","func_wall_toggle","cof_blackandwhite","cof_camerazoom","env_blood","cof_giveitems","cof_blur","env_global","func_door_rotating","func_breakable","env_particle","cof_chapter","trigger_hurt","cof_killmp3","cof_objective","func_door","trigger_cofmobile","cof_addcodenote","subtitle_main","subtitle_multiple","cof_keypad4","env_spark","cof_triggeron","env_sprite","trigger_changevalue","env_shake","cof_interdooronoff","light","game_counter_set","game_counter"};
 return ok.count(c)!=0;
}
void* manager_memory(Edict* e){
 if(!live(e)||std::strcmp(classname(e),"multi_manager")||!engine[64])return nullptr;void* p=eng<void*(__cdecl*)(Edict*)>(64)(e);
 if(!p||load<void*>(p,4)!=vars(e))return nullptr;const int n=load<int>(p,MmCount);return n>=0&&n<=16?p:nullptr;
}
bool manager_running(Edict* e){void* p=manager_memory(e);if(!p)return false;const int s=load<int>(p,MmState);auto v=vars(e);return (s==1||s==2)&&load<int>(p,MmMode)==0&&!(load<unsigned>(v,0x1a4)&4u)&&load<float>(v,0x104)>0;}
std::vector<std::string> manager_targets(Edict* e,int from){
 std::vector<std::string> out;void* p=manager_memory(e);if(!p)return out;const int n=load<int>(p,MmCount);
 for(int i=from>0?from:0;i<n;++i){const char* s=string_at(load<int>(p,MmNames+i*4));if(s&&*s)out.emplace_back(s);}return out;
}
std::vector<Edict*> skip_named(const std::string& name){std::vector<Edict*> out;Edict* e=nullptr;for(int n=0;n<256;++n){e=find_next(e,"targetname",name.c_str());if(!e)break;if(live(e))out.push_back(e);}return out;}
// Everything a not yet fired target would do must be a state change (nested managers checked too).
bool skip_check(const std::string& name,int depth,std::set<std::string>& seen,std::string& why){
 if(!seen.insert(name).second)return true;
 for(auto e:skip_named(name)){const std::string c=classname(e);
  if(!skip_class(c)){why=name+" ("+c+")";return false;}
  // v0.5: a relay or counter only passes the fire on; what it fires is checked too.
  if(c=="trigger_relay"||c=="game_counter"){auto v=vars(e);const char* t=v?string_at(load<int>(v,0x1cc)):nullptr;if(t&&*t&&depth<=4&&!skip_check(t,depth+1,seen,why))return false;}
  if(c=="multi_manager"){if(depth>4){why=name+" (nested too deep)";return false;}for(const auto& t:manager_targets(e,0))if(!skip_check(t,depth+1,seen,why))return false;}
 }
 return true;
}
// Every name the scene can still fire (managers, relays and counters followed), whatever its class.
void skip_collect(const std::string& name,int depth,std::set<std::string>& seen){
 if(depth>6||!seen.insert(name).second)return;
 for(auto e:skip_named(name)){const std::string c=classname(e);
  if(c=="trigger_relay"||c=="game_counter"){auto v=vars(e);const char* t=v?string_at(load<int>(v,0x1cc)):nullptr;if(t&&*t)skip_collect(t,depth+1,seen);}
  if(c=="multi_manager")for(const auto& t:manager_targets(e,0))skip_collect(t,depth+1,seen);}
}
// v0.5.7: true when the name (relays, counters and managers followed) reaches an entity of this class.
bool skip_reaches(const std::string& name,const char* cls,int depth,std::set<std::string>& seen){
 if(depth>6||!seen.insert(name).second)return false;
 for(auto e:skip_named(name)){const std::string c=classname(e);if(c==cls)return true;
  if(c=="trigger_relay"||c=="game_counter"){auto v=vars(e);const char* t=v?string_at(load<int>(v,0x1cc)):nullptr;if(t&&*t&&skip_reaches(t,cls,depth+1,seen))return true;}
  if(c=="multi_manager")for(const auto& t:manager_targets(e,0))if(skip_reaches(t,cls,depth+1,seen))return true;}
 return false;
}
std::vector<SkipManager> scene_managers(Edict* camera){
 std::vector<Edict*> running;Edict* m=nullptr;for(int n=0;n<1024;++n){m=find_next(m,"classname","multi_manager");if(!m)break;if(manager_running(m))running.push_back(m);}
 const std::string cam=targetname(camera);std::set<std::string> names;std::vector<SkipManager> out;std::set<Edict*> in;
 auto add=[&](Edict* e){if(in.insert(e).second){out.push_back({e,serial(e)});names.insert(targetname(e));}};
 for(auto e:running){const auto t=manager_targets(e,0);if(std::find(t.begin(),t.end(),cam)!=t.end())add(e);}
 for(bool grew=true;grew&&!out.empty();){grew=false;
  for(auto e:running){if(in.count(e))continue;const std::string own=targetname(e);const auto t=manager_targets(e,0);
   const bool parent=std::any_of(t.begin(),t.end(),[&](const std::string& s){return names.count(s)!=0;});
   if(names.count(own)||parent){add(e);grew=true;}}}
 return out;
}
// v0.5.5: s4_nightmare (the opening's passive map: no camera leases, no
// client tracking) has cutscenes too. There every connected player counts and
// the scene camera is found by scanning for an active trigger_camera.
bool skip_map(){return scoped()||(verified&&globals&&!std::strcmp(map_name(),"s4_nightmare"));}
bool skip_ready(Edict* e){return scoped()?client_ready(e):live(e);}
// Off the tracked maps the camera's own state (pev->iuser1 at 0x248, which CTriggerCamera sets while on) is read directly.
bool skip_camera_on(Edict* c){if(scoped())return camera_active(c);auto v=vars(c);return v&&!std::strcmp(classname(c),"trigger_camera")&&load<int>(v,0x248)!=0;}
// v0.5.7 (playtest 2: after a skip the camera stayed on for its whole time,
// "SKIP ended dockam1 again" then "offered for dockam1" over and over).
// CTriggerCamera::Use only toggles m_state (decomp triggers.cpp): a camera the
// fast-forward had already switched off (m_state 0, pev->iuser1 still 1 until
// its next FollowTarget think) was switched ON again by the adapter's own Use,
// for its full wait. A camera is now ended the way it ends by itself: its
// m_flReturnTime is moved into the past and the native FollowTarget runs the
// normal end path on its next think (every player's view and controls back,
// its message and targets fired). Never toggled. Save table (pinned hl.dll
// 1021A410..): m_flWait 0x104, m_flReturnTime 0x108, m_state 0x124.
constexpr std::size_t CameraWait=0x104,CameraReturnTime=0x108,CameraState=0x124;
// 1 ended, -1 held until its own Use(666) (left alone), 0 camera memory not
// known (the opening's passive maps): the old single toggle is used there.
int camera_end(Edict* c,Edict* activator){
 void* p=nullptr;
 if(!globals||!camera_memory(c,p)){native<void(__cdecl*)(Edict*,Edict*)>(3)(c,activator);return 0;}
 if(load<float>(p,CameraWait)==666.f)return -1;
 store(p,CameraReturnTime,load<float>(globals,0)-1.f);return 1;
}
// Switched off already, its end path runs on the next think.
bool camera_ending(Edict* c){void* p=nullptr;return camera_memory(c,p)&&load<int>(p,CameraState)==0;}
// v0.5.7 (tester screenshot: "Press R to skip 0/2" drawn over the health and
// stamina bars): the prompt sits higher, above the bars.
constexpr float SkipTextY=0.72f;
bool skipPromptShown=false;
void skip_show(const char* text,float hold=3.f){
 static const unsigned char white[4]={230,230,230,255};skipPromptShown=*text!=0;
 for(auto e:connected_players())if(skip_ready(e))hud_text(e,3,0.02f,SkipTextY,white,hold,text,0.1f);
}
// v0.6 (Katz 2026-10-25: "the skip text still stayed after skipping the cutscene"):
// the prompt had a 3 s hold and was re-sent every 2.5 s, so it outlived the scene.
// As soon as no scene camera holds anyone the prompt channel is cleared.
void skip_prompt_clear(){if(skipPromptShown)skip_show("",0.1f);}
int skip_needed(){int n=0;for(auto e:connected_players())if(skip_ready(e)&&view_alive(e))++n;return n;}
// Held by a scene camera: a native lease, or the running camera a late joiner was given (camera_join).
bool skip_viewing(Edict* e){
 if(!scoped())return skipScene.active;if(lease_camera(e))return true;auto it=clientViews.find(e);auto v=it==clientViews.end()?nullptr:camera_resolve(it->second);
 return v&&v!=e&&!std::strcmp(classname(v),"trigger_camera")&&skipScene.names.count(targetname(v))!=0;
}
void skip_count_show(){char text[64]{};std::snprintf(text,sizeof(text),"Press R to skip  %d/%d",static_cast<int>(skipScene.votes.size()),skip_needed());skip_show(text);}
void skip_log(const char* line){if(skipLines<200){++skipLines;log(line);}}
// The fast-forward: managers first (in the order found), then what they started, until nothing new runs.
void skip_now(Edict* activatorFallback){
 const float now=load<float>(globals,0);std::set<std::pair<Edict*,int>> done,relays;int fired=0,cameras=0,rounds=0;
 {Edict* d=nullptr;for(int n=0;n<512;++n){d=find_next(d,"classname","DelayedUse");if(!d)break;relays.insert({d,serial(d)});}}   // not ours
 for(;rounds<6;++rounds){
  std::vector<Edict*> todo;
  for(const auto& m:skipScene.managers)if(serial(m.e)==m.serial&&manager_running(m.e)&&!done.count({m.e,m.serial}))todo.push_back(m.e);
  {Edict* x=nullptr;for(int n=0;n<1024;++n){x=find_next(x,"classname","multi_manager");if(!x)break;if(manager_running(x)&&skipScene.names.count(targetname(x))&&!done.count({x,serial(x)}))todo.push_back(x);}}
  std::vector<Edict*> delayed;{Edict* d=nullptr;for(int n=0;n<512;++n){d=find_next(d,"classname","DelayedUse");if(!d)break;if(!relays.count({d,serial(d)}))delayed.push_back(d);}}
  if(todo.empty()&&delayed.empty())break;
  for(auto e:todo){void* p=manager_memory(e);if(!p)continue;done.insert({e,serial(e)});fired+=load<int>(p,MmCount)-load<int>(p,MmIndex);
   for(const auto& t:manager_targets(e,0))skipScene.names.insert(t);
   store(p,MmStart,now-100000.f);store(vars(e),0x104,now);crumb("skip manager",e);native<void(__cdecl*)(Edict*)>(2)(e);}
  for(auto d:delayed){relays.insert({d,serial(d)});if(live(d)){crumb("skip delayed",d);native<void(__cdecl*)(Edict*)>(2)(d);}}
 }
 // Scene cameras still on: ended through their own end path (camera_end), never toggled.
 Edict* c=nullptr;for(int n=0;n<512;++n){c=find_next(c,"classname","trigger_camera");if(!c)break;if(skip_camera_on(c)&&skipScene.names.count(targetname(c))&&camera_end(c,activatorFallback)>=0)++cameras;}
 char line[240]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP %s fast-forwarded: %d remaining target(s) fired in order, %d camera(s) ended, %d round(s)\n",skipScene.label.c_str(),fired,cameras,rounds);skip_log(line);
 skip_show("Cutscene skipped",1.5f);skipPromptShown=false;skipAfter={skipScene.names,{},load<float>(globals,0)+2.f};skipScene={};
}
void skip_frame(){
 if(!skip_map()||!globals||!engine[64])return;
 Edict* camera=nullptr;Edict* watcher=nullptr;for(auto e:connected_players())if(auto c=lease_camera(e)){camera=c;watcher=e;break;}
 if(!camera&&!scoped())for(auto e:connected_players()){void* pm=engine[64]?eng<void*(__cdecl*)(Edict*)>(64)(e):nullptr;Edict* c=pm?camera_handle(pm,0x2348):nullptr;if(c&&c!=e&&!std::strcmp(classname(c),"trigger_camera")&&skip_camera_on(c)){camera=c;watcher=e;break;}}
 if(!camera){skipScene={};skip_prompt_clear();return;}
 // v0.5.7: a camera already switched off is ending (its next think gives the views back): nothing to offer.
 if(camera_ending(camera))return;
 // v0.5.5 (dev runs: after a fast-forward one camera of the same scene came on
 // again for a moment, "SKIP offered for kam6 (0 manager(s))"): for 2 s a
 // camera of the skipped scene is ended at once instead of being offered again.
 if(load<float>(globals,0)<skipAfter.until&&skipAfter.names.count(targetname(camera))){if(!skipAfter.ended.insert(targetname(camera)).second)return;
  camera_end(camera,watcher);char line[140]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP ended %s again (part of the skipped scene)\n",targetname(camera));skip_log(line);return;}
 const bool same=skipScene.active&&skipScene.names.count(targetname(camera));
 if(!same){
  skipScene={};skipScene.active=true;skipScene.managers=scene_managers(camera);
  skipScene.names.insert(targetname(camera));
  // The whole scene: its managers and everything they fire (nested managers too), so the next camera of the same scene keeps the votes.
  {std::set<std::string> all;for(const auto& m:skipScene.managers){skipScene.names.insert(targetname(m.e));for(const auto& t:manager_targets(m.e,0))skip_collect(t,0,all);}skipScene.names.insert(all.begin(),all.end());}
  skipScene.label=skipScene.managers.empty()?std::string(targetname(camera)):std::string(targetname(skipScene.managers.front().e));
  // v0.5: a camera no running manager owns (logs: clothescamera, conkam) is
  // skipped by ending it once; its own end path gives every player back control.
  // The end-of-game stats camera (statskamera*, held for minutes) is never offered.
  bool ok=!(skipScene.managers.empty()&&!std::strncmp(targetname(camera),"statskamera",11));std::set<std::string> seen;
  if(!ok)skipScene.why="the stats camera";
  // v0.5.5 (Katz 2026-10-23: "FIX THE SKIP BUTTON AND TEXT ON ALL OF THE CUTSCENES";
  // log: "SKIP not offered for multidadoc: docweaponsec (cof_doctorweapontrigger)"):
  // every scene is offered. What is not a plain state change (trains, scripted
  // sequences, boss bars...) still fires in order and then runs on by itself;
  // the first such entity is named in the log.
  std::string runsOn;for(const auto& m:skipScene.managers){void* p=manager_memory(m.e);if(!p)continue;for(const auto& t:manager_targets(m.e,load<int>(p,MmIndex)))if(runsOn.empty()&&!skip_check(t,0,seen,runsOn))break;}
  // v0.5.7 (Katz: the doctor scene must not be skippable, or skip only up to
  // the choice; log: the skip of multits3 fired the cof_yesno at once and
  // "DOCTOR weapon=1" followed): a scene that still has a yes/no choice to
  // show is not offered, so the choice always comes up. The scene before it
  // (multits1) and the one after it (multits4) stay skippable.
  if(ok){std::set<std::string> asked;for(const auto& m:skipScene.managers){void* p=manager_memory(m.e);if(!p)continue;
   for(const auto& t:manager_targets(m.e,load<int>(p,MmIndex)))if(skip_reaches(t,"cof_yesno",0,asked)){ok=false;skipScene.why="its yes/no choice ("+t+") comes up first";break;}
   if(!ok)break;}}
  skipScene.allowed=ok;
  char line[300]{};if(ok)std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP offered for %s (%u manager(s))%s%.120s\n",skipScene.label.c_str(),static_cast<unsigned>(skipScene.managers.size()),runsOn.empty()?"":"; runs on after a skip: ",runsOn.c_str());
  else std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP not offered for %s: %.150s\n",skipScene.label.c_str(),skipScene.why.c_str());
  skip_log(line);if(ok){skip_count_show();skipScene.nextShow=load<float>(globals,0)+2.5f;}
  return;
 }
 // v0.5.3 (Katz: the Chapter 2 scene "didnt really show an option"): the offer
 // was sent once, before the arriving player's HUD was ready (log: SKIP offered,
 // then CHAPTER ... ready). The prompt now stays up for the whole skippable scene.
 if(skipScene.allowed&&load<float>(globals,0)>=skipScene.nextShow){skipScene.nextShow=load<float>(globals,0)+2.5f;skip_count_show();}
 if(!skipScene.allowed||skipScene.votes.empty())return;
 // Only players still in the game count; the scene is skipped when all of them voted.
 const auto players=connected_players();for(auto it=skipScene.votes.begin();it!=skipScene.votes.end();)it=std::any_of(players.begin(),players.end(),[&](Edict* e){return e==it->first&&serial(e)==it->second&&skip_ready(e)&&view_alive(e);})?std::next(it):skipScene.votes.erase(it);
 const int need=skip_needed();if(need>0&&static_cast<int>(skipScene.votes.size())>=need)skip_now(watcher);
}
// v0.5.1 (Katz's v0.5 test, 2026-10-23 log: "SKIP offered for the prologue",
// never a vote): in a cutscene the server sets pev->fuser3 1..4, the client reads
// it as vuser4.x (decomp entity.cpp iPlayerMode) and then IN_JumpDown, IN_UseDown,
// IN_AttackDown and IN_DuckDown drop the key (input.cpp). IN_ReloadDown has no
// such check, so R (IN_RELOAD 0x2000) reaches the server in every scene and is
// the skip key. SPACE (2) and E (32) still count where the client sends them.
// v0.5.7 (log: "SKIP vote (usercmd)" from both players while the doctor's
// yes/no choice was on screen, then the scene skipped): E and SPACE are used in
// scenes (choices, keypads), so only R counts.
bool skip_pressed(Edict* e,const void* command){
 unsigned short buttons=0;std::memcpy(&buttons,static_cast<const unsigned char*>(command)+0x1e,2);
 auto& held=skipJump[e];if(held.first!=serial(e))held={serial(e),false};const bool down=(buttons&0x2000u)!=0,pressed=down&&!held.second;held.second=down;return pressed;
}
// --- Skipping the prologue (v0.5) -------------------------------------------------------
// cof_campaign_story4 is the passive native opening (no camera leases, no player
// memory). Once its intro timeline runs (sc3_timeline_multiintroduct, the same
// mark the opening evidence uses), every player in the game can vote; at N/N the
// map's own end manager kunitei2 runs early: it gathers the party at the final
// spot and the final count changes level to s4_nightmare, as at the intro's end.
struct PrologueSkip {bool offered=false,done=false;std::set<std::pair<Edict*,int>> votes;};
PrologueSkip prologueSkip;
std::vector<Edict*> prologue_voters(){std::vector<Edict*> out;for(auto e:connected_players())if(view_alive(e))out.push_back(e);return out;}
void prologue_show(const char* text){static const unsigned char white[4]={230,230,230,255};for(auto e:connected_players())hud_text(e,3,0.02f,SkipTextY,white,3.f,text,0.1f);}
void prologue_count_show(){char text[64]{};std::snprintf(text,sizeof(text),"Press R to skip  %d/%d",static_cast<int>(prologueSkip.votes.size()),static_cast<int>(prologue_voters().size()));prologue_show(text);}
void prologue_skip_frame(){
 if(!verified||!globals||prologueSkip.done||openingTimeline<0||std::strcmp(map_name(),"cof_campaign_story4"))return;
 if(!prologueSkip.offered){prologueSkip.offered=true;skip_log("[STORYCOOP_RC4] SKIP offered for the prologue\n");prologue_count_show();return;}
 const auto voters=prologue_voters();if(voters.empty())return;
 for(auto it=prologueSkip.votes.begin();it!=prologueSkip.votes.end();)it=std::any_of(voters.begin(),voters.end(),[&](Edict* v){return v==it->first&&serial(v)==it->second;})?std::next(it):prologueSkip.votes.erase(it);
 if(static_cast<int>(prologueSkip.votes.size())<static_cast<int>(voters.size()))return;
 Edict* end=find_next(nullptr,"targetname","kunitei2");
 if(!end||std::strcmp(classname(end),"multi_manager")||!original[3]){skip_log("[STORYCOOP_RC4] SKIP prologue: its end manager kunitei2 is missing; watching on\n");prologueSkip.done=true;return;}
 prologueSkip.done=true;crumb("skip prologue",end);native<void(__cdecl*)(Edict*,Edict*)>(3)(end,voters.front());
 skip_log("[STORYCOOP_RC4] SKIP prologue: kunitei2 started (party to the final spot, then s4_nightmare)\n");prologue_show("Cutscene skipped");
}
void prologue_skip_command(Edict* e,const void* command){
 if(!command||!live(e)||!prologueSkip.offered||prologueSkip.done||std::strcmp(map_name(),"cof_campaign_story4"))return;
 if(!skip_pressed(e,command)||!view_alive(e)||!prologueSkip.votes.insert({e,serial(e)}).second)return;
 char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP vote player=%d on the prologue: %d/%d\n",eng<int(__cdecl*)(Edict*)>(71)(e),static_cast<int>(prologueSkip.votes.size()),static_cast<int>(prologue_voters().size()));skip_log(line);
 prologue_count_show();
}
// Collision off (E2): while one player moves, the others are not solid
// (SV_RunCmd builds the move's physents between PlayerPreThink and
// PlayerPostThink), and their packets say SOLID_NOT so the clients predict the
// same. pev->solid is entvars +0x10C; entity_state_t solid is the short at +0x3A.
std::vector<std::pair<Edict*,int>> collisionOff;
bool collision_off(){return sessionOptions.have&&!sessionOptions.collision;}
void collision_restore(){for(const auto& [p,s]:collisionOff)if(live(p))store(vars(p),0x10c,s);collisionOff.clear();}
void collision_pre(Edict* e){
 collision_restore();if(!collision_off()||!globals)return;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto p=eng<Edict*(__cdecl*)(int)>(72)(i);auto v=vars(p);if(p==e||!live(p)||!v||std::strcmp(classname(p),"player"))continue;
  const int solid=load<int>(v,0x10c);if(solid==0)continue;collisionOff.push_back({p,solid});store(v,0x10c,0);}
}
// v0.5.5 (Katz 2026-10-23: "FIX THE SKIP BUTTON"): in a dev run R, SPACE and
// E during the Chapter 2 scene never reached CmdStart (a per-command log stayed
// empty while the player watched the camera), so no vote was ever counted. Each
// player's own game now sends "sc_skip" when R is pressed (menu_hold_timer in
// client-menu.hpp, the game's main thread), which arrives as a client command
// in every state. The usercmd path stays for clients that do send R.
void skip_vote(Edict* e,const char* how){
 if(!skip_map()||!live(e))return;
 if(!skipScene.active||!skipScene.allowed||!skip_viewing(e)||!skip_ready(e)||!view_alive(e)){
  if(skipScene.active){char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP key (%s) player=%d not counted: allowed=%d viewing=%d ready=%d alive=%d\n",how,eng<int(__cdecl*)(Edict*)>(71)(e),skipScene.allowed?1:0,skip_viewing(e)?1:0,skip_ready(e)?1:0,view_alive(e)?1:0);skip_log(line);}
  return;}
 if(!skipScene.votes.insert({e,serial(e)}).second)return;
 char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP vote (%s) player=%d on %s: %d/%d\n",how,eng<int(__cdecl*)(Edict*)>(71)(e),skipScene.label.c_str(),static_cast<int>(skipScene.votes.size()),skip_needed());skip_log(line);
 skip_count_show();
}
void skip_command(Edict* e,const void* command){
 if(!scoped()||!command||!live(e))return;
 if(skip_pressed(e,command))skip_vote(e,"usercmd");
}
// "sc_skip" (R in each player's own game): the prologue vote or the scene vote.
bool skip_client_command(Edict* e,const char* command){
 if(!command||std::strcmp(command,"sc_skip")||!live(e))return false;
 if(prologueSkip.offered&&!prologueSkip.done&&globals&&!std::strcmp(map_name(),"cof_campaign_story4")){
  if(view_alive(e)&&prologueSkip.votes.insert({e,serial(e)}).second){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] SKIP vote (R) player=%d on the prologue: %d/%d\n",eng<int(__cdecl*)(Edict*)>(71)(e),static_cast<int>(prologueSkip.votes.size()),static_cast<int>(prologue_voters().size()));skip_log(line);prologue_count_show();}
  return true;}
 skip_vote(e,"R");return true;
}
void coop_map_changed(){collision_restore();itemCopiesDone=false;forcedVisible.clear();bossOver=false;fallBacks.clear();doorGlobals.clear();doorLocks.clear();trainEscape={};floorWallsDone=false;quickNotices.clear();fallWatch.clear();equipWants.clear();nightmareLogged=false;morphineWatch.clear();prologueSkip={};partyLimitLogged=-1;joinWait={};skipScene={};skipAfter={};skipJump.clear();skipLines=0;evidence_reset();drowned_reset();give_reset();stuckUsed.clear();nightmareRegroup={};nameStates.clear();lastPlaces.clear();sceneSeen.clear();sceneFollowers.clear();idleProbes.clear();doorMusicLines=0;sceneMoveLines=0;}
