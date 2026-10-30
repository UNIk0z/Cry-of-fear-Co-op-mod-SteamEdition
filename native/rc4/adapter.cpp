#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <dbghelp.h>
#include "abi.hpp"
#include "journal.hpp"
#include "puzzle.hpp"
#include "checkpoint.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <set>
#include <map>
#include <cmath>
#include <chrono>
#include <cstdarg>
#include <cwchar>
#include <cctype>

static_assert(sizeof(void*)==4,"CoF requires an x86 adapter");
extern "C" {
 void* rc4_exports[846]{};
 __declspec(noreturn) void __cdecl rc4_refused_export(){RaiseFailFastException(nullptr,nullptr,0);}
}
namespace {struct InitializeExports{InitializeExports(){for(auto& entry:rc4_exports)entry=reinterpret_cast<void*>(&rc4_refused_export);}} initializeExports;}
namespace rc4 {
namespace {
Engine engine{},forwardEngine{};Table original{};void* globals=nullptr;
HMODULE backend=nullptr;bool verified=false;int clothesMessage=0,addNote=0,noteDoc=0,textMessage=0,resetHud=0,initHud=0,proFontMessage=0,healthCapMessage=0,cofLogoMessage=0,qslotUpdateMessage=0,computerMessage=0,creditsMessage=0,unlocksMessage=0,concussMessage=0;
Journal journal;
void checkpoint_reset();
void keypad_reset();
void story_reset();
void rejoin_reset();
void carry_reset();void give_reset();void give_owed_reset();void give_lookup(Edict*,const char*,const char*,Edict*);void regroup_lookup(Edict*,const char*,const char*,Edict*);void give_flush();bool hide_player_from(Edict*,Edict*);void door_music(const char*,int,float);
bool logo_begin(int,int,Edict*);bool logo_write(int);bool logo_end();bool score_begin(int,int,Edict*);bool score_write(int,int);bool score_end();void score_reset();void score_sync();extern int scoreInfoMessage;
void fanout_lookup(Edict*,const char*,const char*,Edict*);void door_lock_lookup(Edict*,const char*,const char*,Edict*);bool ladder_find_override(Edict*,const char*,const char*,Edict*&);void strip_clear_arm(Edict*);void strip_clear_sent(int,Edict*);void deploy_guard(Edict*);void collision_hold(Edict*);bool collision_touch_blocked(Edict*,Edict*);void door_hide_frame(Edict*);void strip_clear_frame(Edict*);void bag_lookup(Edict*,const char*,const char*,Edict*);void evidence_sound(Edict*,const char*);void evidence_lookup(Edict*,const char*,const char*);
void checkpoint_world_ready();
bool recorder_begin(int,int,const float*,Edict*);
bool recorder_write(int,int,float,const char*);
bool recorder_end();
extern int saveLoadMessage,screenFadeMessage;
struct NotePacket{bool active=false,valid=true;std::size_t bytes=0,shorts=0,strings=0;int slot=0;Edict* recipient=nullptr;NoteMetadata meta{};Note note{};} packet;
struct Allocation{void* memory;std::size_t bytes;int serial;std::uint64_t sent=0;std::size_t next=0;float due=0;std::uint32_t rejection=0;std::size_t notified=0;bool commands=false;float hudDue=0;float born=0;unsigned stamp=0;std::array<std::string,NoteLimit> delivered{};};
// Readiness comes from the pinned CmdStart callback, not server PreThink.
// Native HUD initialization sends sixteen AddNote records with ZERO code
// metadata (100F71D5); client 1006F190 mutates live journal widgets on receipt.
std::string note_signature(const Note& note,NoteMetadata meta,bool full){
 std::string out=std::to_string(meta.codeFlag)+":"+std::to_string(meta.codeValue)+":"+std::to_string(meta.codeType)+":";
 for(std::size_t i=0;i<NoteFields.size();++i){out+=i==0&&full?Journal::resource(note):reinterpret_cast<const char*>(note.data()+NoteFields[i].offset);out.push_back(1);}return out;
}
bool client_ready(Edict* e);
struct ReaderPacket{bool active=false;std::size_t strings=0;std::string page;} reader;
std::unordered_map<Edict*,Allocation> allocations;
std::vector<std::pair<std::uint32_t,std::string>> exportedNames;
std::unordered_map<Edict*,int> openingThinks;
std::set<std::array<int,5>> vehiclePackets;
std::set<std::array<int,3>> openingViews;
float openingTimeline=-1;
std::set<int> netLogged;float lastFrame=-1,frameWindow=-1,maxGap=0;int frameGaps=0;
template<class F>F eng(std::size_t i){return reinterpret_cast<F>(engine[i]);}
template<class F>F native(std::size_t i){return reinterpret_cast<F>(original[i]);}
const char* string_at(int n){return eng<const char*(__cdecl*)(int)>(66)(n);}
const char* map_name(){return globals?string_at(load<int>(globals,0x0c)):"";}
void session_log(const char* s);
void pause_poll();
void log(const char* s){if(engine[81])eng<void(__cdecl*)(const char*)>(81)(s);session_log(s);}
bool scoped(){
 const char* name=map_name();if(!verified||!name)return false;
 // Subsequent RC2 campaign maps reuse the same native player journal layout.
 // RC3 standalone launches retain the original DLL and never load this adapter.
 // The verified opening has no journal/puzzle persistence to restore. Keep
 // player memory and cinematic callbacks native until Simon's wake-up map.
 if(std::strcmp(name,"cof_campaign_story4")==0||std::strcmp(name,"s4_nightmare")==0)return false;
 return std::strncmp(name,"s4_",3)==0||std::strncmp(name,"s2_",3)==0;
}
bool live(Edict* e){
 if(!e||!globals||!engine[71]||!engine[72])return false;
 const int index=eng<int(__cdecl*)(Edict*)>(71)(e),max=load<int>(globals,0x94);
 return index>=0&&index<max&&eng<Edict*(__cdecl*)(int)>(72)(index)==e&&load<int>(e,0)==0;
}
int serial(Edict* e){return live(e)?load<int>(e,4):-1;}
void* vars(Edict* e){return live(e)?eng<void*(__cdecl*)(Edict*)>(68)(e):nullptr;}
const char* classname(Edict* e){auto p=vars(e);return p?string_at(load<int>(p,0)):"";}
// v0.4 (v0.3 revisit and load crash: call 0x70 from ApplyDesiredSettings hl
// 100C3E7D under CheckDesiredList 100C4120 from PlayerPostThink). CoF (Spirit
// movewith.cpp) chains entities with pending assist/desired work from g_pWorld
// (the world's private data) through CBaseEntity::m_pAssistLink (+0x3C, read at
// 100C4164/100C4170). An entity this adapter frees must leave that chain first
// (assist_unlink; found in parallel by the agitated-murdock session, fe8ad2e).
constexpr std::size_t AssistLink=0x3c;
void assist_unlink(Edict* e){
 if(!live(e)||!engine[64]||!engine[72])return;
 void* victim=eng<void*(__cdecl*)(Edict*)>(64)(e);Edict* world=eng<Edict*(__cdecl*)(int)>(72)(0);void* node=world?eng<void*(__cdecl*)(Edict*)>(64)(world):nullptr;
 if(!victim||!node||node==victim)return;
 for(int n=0;n<8192&&node;++n){void* next=load<void*>(node,AssistLink);if(next==victim){store(node,AssistLink,load<void*>(victim,AssistLink));store<void*>(victim,AssistLink,nullptr);return;}node=next;}
}
bool player_memory(Edict* e,void*& memory,std::size_t& bytes){
 auto it=allocations.find(e);if(it==allocations.end()||serial(e)!=it->second.serial||std::strcmp(classname(e),"player"))return false;
 auto& a=it->second;if(a.bytes!=PlayerBytes||eng<void*(__cdecl*)(Edict*)>(64)(e)!=a.memory)return false;
 memory=a.memory;bytes=a.bytes;return true;
}
Edict* resolve(std::uint32_t bits,int expected){auto e=reinterpret_cast<Edict*>(bits);return serial(e)==expected?e:nullptr;}
// The engine's FindEntityByString returns the WORLD edict (index 0), not null,
// when nothing more matches (user log: one map reported 192 ladders for 3).
// Every adapter search loop must stop there or it wraps around forever.
Edict* find_next(Edict* start,const char* key,const char* value){
 auto e=eng<Edict*(__cdecl*)(Edict*,const char*,const char*)>(14)(start,key,value);
 return e&&eng<int(__cdecl*)(Edict*)>(71)(e)>0?e:nullptr;
}
bool solved(Edict* e){auto p=vars(e);return p&&load<int>(p,0x114)!=0;}
#include "camera-runtime.hpp"
bool opening(){const char* name=map_name();return verified&&name&&(std::strcmp(name,"cof_campaign_story4")==0||std::strcmp(name,"s4_nightmare")==0);}
// Pinned backend entvars metadata identifies targetname at1D0;108 is movetype.
const char* targetname(Edict* e){auto p=vars(e);return p?string_at(load<int>(p,0x1d0)):"";}
// skittag/skittag22: title train (coop-fixes-003 holds it until multiintro1).
// trainunder: the bridge train of the multikattej shots (func_train).
bool title_vehicle(Edict* e){const auto name=targetname(e);if(!name)return false;const char* type=classname(e);return (std::strcmp(type,"func_tracktrain")==0&&(std::strcmp(name,"skittag")==0||std::strcmp(name,"skittag22")==0))||(std::strcmp(type,"func_train")==0&&std::strcmp(name,"trainunder")==0);}
void vehicle_evidence(Edict* e,const char* phase,int host,int packed){
 const auto p=vars(e);if(!p)return;char s[400]{};
 // These offsets were read from this exact backend's entvars save metadata.
 std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] VEHICLE_STATE phase=%s entity=%d host=%d packed=%d name=%s model=%s modelindex=%d origin=%.1f,%.1f,%.1f speed=%.1f effects=%x rendermode=%d renderamt=%.1f flags=%x group=%x time=%.2f\n",phase,eng<int(__cdecl*)(Edict*)>(71)(e),host,packed,targetname(e),string_at(load<int>(p,0xb8)),load<int>(p,0xb4),load<float>(p,8),load<float>(p,12),load<float>(p,16),load<float>(p,0x1fc),load<unsigned>(p,0x118),load<int>(p,0x14c),load<float>(p,0x150),load<unsigned>(p,0x1a8),load<unsigned>(p,0x244),load<float>(globals,0));log(s);
}
bool collision_off();   // coop-runtime.hpp (v0.5 host setup)
bool player_force_visible(Edict* e,Edict* host);void player_forced_log(Edict* e,Edict* host,bool shown);   // coop-runtime.hpp (v0.6)
int __cdecl full_pack(void* state,int number,Edict* e,Edict* host,int flags,int player,unsigned char* set){
 // coop-fixes-014: no player model inside this client's scene camera, but the
 // player is still sent. 013 returned 0 here, so a client joining into a running
 // camera never got its own player entity and client.dll read a NULL model
 // (P2 crash 2026-10-15, client.dll+5f5c8). Native AddToFullPack copies
 // entvars effects (edict+198) to entity_state+3C; EF_NODRAW=0x80 is added there.
 // v0.2: staged actors are hidden by the maps themselves (coop-runtime.hpp).
 const bool hide=player&&hide_player_from(e,host);
 // v0.1 (user: "the train appears at the bottom left"): the title train waits
 // at the station until multiintro1 starts it (coop-fixes-003 timing, as in
 // the single-player map). Earlier builds forced these brushes past the PVS
 // check (and retried with flags/groupinfo cleared) in every camera view, so
 // the title camera inkamera2222 (-1125,-2968,1355, looking north) was sent
 // the parked train although the station is outside its PVS (c_intro vis
 // data: 0 of the station leaves visible). The client then drew it alone in
 // the lower left of the title shot. Every vehicle packet is native again.
 int result=native<int(__cdecl*)(void*,int,Edict*,Edict*,int,int,unsigned char*)>(39)(state,number,e,host,flags,player,set);
 // v0.6: a nearby partner the PVS test refused is sent anyway (pSet NULL passes the test).
 if(!result&&player&&!hide&&player_force_visible(e,host)){result=native<int(__cdecl*)(void*,int,Edict*,Edict*,int,int,unsigned char*)>(39)(state,number,e,host,flags,player,nullptr);player_forced_log(e,host,result!=0);}
 if(opening()&&title_vehicle(e)&&live(host)){
  const float elapsed=openingTimeline<0?-1:load<float>(globals,0)-openingTimeline;
  const int phase=elapsed<0?0:elapsed<16?1:elapsed<44?2:elapsed<100?3:4;
  // At most one accepted and one rejected native packet per entity/client/
  // phase. Native state/PVS/return value are forwarded without modification.
  const auto key=std::array<int,5>{eng<int(__cdecl*)(Edict*)>(71)(e),serial(e),eng<int(__cdecl*)(Edict*)>(71)(host),phase,result?1:0};
  if(vehiclePackets.insert(key).second)vehicle_evidence(e,phase==0?"LOBBY":phase==1?"READY":phase==2?"TITLE_WINDOW":phase==3?"LATER":"RUN",key[2],result);
 }
 if(result&&hide&&state)store(state,0x3c,load<unsigned>(state,0x3c)|0x80u);
 if(result&&player&&state&&collision_off())store<short>(state,0x3a,0);   // v0.5 E2: collision off
 return result;
}
void __cdecl visibility(Edict* view,Edict* client,unsigned char** pvs,unsigned char** pas){
 native<void(__cdecl*)(Edict*,Edict*,unsigned char**,unsigned char**)>(37)(view,client,pvs,pas);
 view_seen(view,client);pause_poll();
 if(!opening()||!live(client)||openingViews.size()>=128)return;
 const float elapsed=openingTimeline<0?-1:load<float>(globals,0)-openingTimeline;
 const int phase=elapsed<0?0:elapsed<16?1:elapsed<44?2:3;
 const auto key=std::array<int,3>{eng<int(__cdecl*)(Edict*)>(71)(client),live(view)?eng<int(__cdecl*)(Edict*)>(71)(view):0,phase};
 if(openingViews.insert(key).second){const auto p=vars(live(view)?view:client);char s[220]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] VIEW_PVS host=%d phase=%d view=%d class=%s origin=%.1f,%.1f,%.1f pvs=%s time=%.2f\n",key[0],phase,key[1],classname(live(view)?view:client),load<float>(p,8),load<float>(p,12),load<float>(p,16),pvs&&*pvs?"native-set":"null",load<float>(globals,0));log(s);}
}
void opening_think_evidence(Edict* e){
 if(!opening()||std::strcmp(classname(e),"multi_manager"))return;
 const char* name=targetname(e);if(!name)return;
 if(std::strcmp(name,"sc3_timeline_multiintroduct")&&std::strcmp(name,"multikattej")&&std::strcmp(name,"multiintro1")&&std::strcmp(name,"multiintro2")&&std::strcmp(name,"kunitei2"))return;
 if(openingTimeline<0&&std::strcmp(name,"sc3_timeline_multiintroduct")==0)openingTimeline=load<float>(globals,0);
 const int id=serial(e);auto found=openingThinks.find(e);if(found!=openingThinks.end()&&found->second==id)return;
 openingThinks[e]=id;char s[200]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4_HOTFIX] NATIVE_THINK %s edict=%d time=%.2f\n",name,eng<int(__cdecl*)(Edict*)>(71)(e),load<float>(globals,0));log(s);
}
void state_reset();void objective_session_reset();
void reset(){objective_session_reset();state_reset();carry_reset();give_reset();give_owed_reset();rejoin_reset();journal.reset();puzzle_reset();checkpoint_reset();keypad_reset();story_reset();for(auto& item:allocations){item.second.sent=0;item.second.next=0;item.second.notified=0;item.second.rejection=0;}}
std::string sha_file(const std::wstring& path){
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE h=nullptr;DWORD size=0,got=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
 if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)<0){BCryptCloseAlgorithmProvider(alg,0);return {};}
 std::vector<unsigned char> object(size);unsigned char digest[32]{};
 bool okay=BCryptCreateHash(alg,&h,object.data(),size,nullptr,0,0)>=0;
 std::ifstream file(path,std::ios::binary);std::array<char,65536> block{};
 if(!file)okay=false;
 while(okay&&file){file.read(block.data(),block.size());auto n=file.gcount();if(n>0&&BCryptHashData(h,reinterpret_cast<PUCHAR>(block.data()),static_cast<ULONG>(n),0)<0)okay=false;}
 if(file.bad())okay=false;
 if(okay)okay=BCryptFinishHash(h,digest,sizeof(digest),0)>=0;
 if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(alg,0);
 if(!okay)return {};
 char out[65]{};for(int i=0;i<32;++i)std::snprintf(out+i*2,3,"%02x",digest[i]);return out;
}
bool load_backend(){
 if(backend)return verified;
 HMODULE own=nullptr;wchar_t path[MAX_PATH]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&load_backend),&own)||!GetModuleFileNameW(own,path,MAX_PATH))return false;
 std::wstring file(path);auto slash=file.find_last_of(L"\\/");if(slash==std::wstring::npos)return false;
 file=file.substr(0,slash+1)+L"hl.dll";
 if(sha_file(file)!="0036b91c01e92ed205513f52563053a55a66a32d257efb5f476b8f1f3dde0e63"){
  log("[STORYCOOP_RC4] REFUSED backend hash; no state hooks installed\n");return false;
 }
 backend=LoadLibraryExW(file.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!backend||!GetProcAddress(backend,"GetEntityAPI")||!GetProcAddress(backend,"GetEntityAPI2")||!GetProcAddress(backend,"GiveFnptrsToDll"))return false;
 // The native callback names belong to the backend image. A transparent SDK
 // proxy must preserve engine function/name lookup as well as entity exports.
 const auto base=reinterpret_cast<unsigned char*>(backend);
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)return false;
 auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
 auto ex=reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base+dir.VirtualAddress);
 auto names=reinterpret_cast<const DWORD*>(base+ex->AddressOfNames);
 auto ordinals=reinterpret_cast<const WORD*>(base+ex->AddressOfNameOrdinals);
 if(ex->Base!=1||ex->NumberOfFunctions!=846||ex->NumberOfNames!=846)return false;
 for(DWORD i=0;i<ex->NumberOfNames;++i){const char* name=reinterpret_cast<const char*>(base+names[i]);auto f=GetProcAddress(backend,name);if(!f||ordinals[i]>=846)return false;rc4_exports[ordinals[i]]=reinterpret_cast<void*>(f);exportedNames.emplace_back(reinterpret_cast<std::uint32_t>(f),name);}
 verified=true;log("[STORYCOOP_RC4] BOOT backend SHA256 verified; API140 forwarding probe ready\n");return true;
}
void* __cdecl allocate(Edict* e,std::int32_t bytes){
 auto p=eng<void*(__cdecl*)(Edict*,std::int32_t)>(63)(e,bytes);
 // born/stamp (coop-fixes-007): when this player entity was created, so a
 // rejoin is told apart from the same edict before it.
 static unsigned stamps=0;
 if(scoped()&&p&&bytes>0){auto& a=allocations[e];a={p,static_cast<std::size_t>(bytes),serial(e)};a.born=globals?load<float>(globals,0):0;a.stamp=++stamps;}return p;
}
void __cdecl release(Edict* e){camera_forget(e);allocations.erase(e);eng<void(__cdecl*)(Edict*)>(65)(e);}
int __cdecl register_message(const char* name,int bytes){int id=eng<int(__cdecl*)(const char*,int)>(75)(name,bytes);if(name&&std::strcmp(name,"Clothes")==0)clothesMessage=id;if(name&&std::strcmp(name,"ViewEntity")==0)cameraViewMessage=id;if(name&&std::strcmp(name,"ModelAttach")==0)cameraAttachMessage=id;if(name&&std::strcmp(name,"ResetHUD")==0)resetHud=id;if(name&&std::strcmp(name,"ScoreInfo")==0)scoreInfoMessage=id;if(name&&std::strcmp(name,"InitHUD")==0)initHud=id;if(name&&std::strcmp(name,"AddNote")==0&&bytes==-1)addNote=id;if(name&&std::strcmp(name,"NoteDoc")==0&&bytes==-1)noteDoc=id;if(name&&std::strcmp(name,"TextMsg")==0&&bytes==-1)textMessage=id;if(name&&std::strcmp(name,"SaveLoad")==0&&bytes==-1)saveLoadMessage=id;if(name&&std::strcmp(name,"ScreenFade")==0)screenFadeMessage=id;if(name&&std::strcmp(name,"ProFont")==0)proFontMessage=id;if(name&&std::strcmp(name,"HealthCap")==0&&bytes==1)healthCapMessage=id;if(name&&std::strcmp(name,"CoFLogo")==0&&bytes==1)cofLogoMessage=id;if(name&&std::strcmp(name,"Computer")==0)computerMessage=id;if(name&&std::strcmp(name,"QSlotUpdate")==0&&bytes==2)qslotUpdateMessage=id;if(name&&std::strcmp(name,"Credits")==0&&bytes==-1)creditsMessage=id;if(name&&std::strcmp(name,"Unlocks")==0)unlocksMessage=id;if(name&&std::strcmp(name,"Concuss")==0)concussMessage=id;return id;}
#include "note-layout.hpp"
// --- Story text for every player (coop-fixes-004) ---------------------------
// Subtitles (subtitle_main think 1005FC20, driven by subtitle_linechange
// 10077240) and similar story texts look up the FIRST player
// (FindEntityByString(NULL,"classname","player")) and send the "ProFont"
// message (10119810) to that player only. A ProFont message sent right after
// such a lookup inside the same native think is copied to every other
// player; texts addressed to the acting player (locked doors, pickups) are not.
bool firstPlayerLookup=false;
Edict* actingPlayer=nullptr;   // v0.3: set only during that player's inventoryuse command
// v0.3 (s4_asylumb boiler bars): CPuzzleBar*::BarCheck (decomp CoF_PuzzleBars.cpp)
// releases the FIRST player's view and puzzle state when the bars are solved,
// while the bars were turned by whoever sent "boilercheck" (client.cpp). Inside
// a bar's think, "the player" is the last player who turned a bar.
Edict* barPlayer=nullptr;int barPlayerSerial=-1;
bool acting_override(Edict* start,const char* key,const char* value,Edict*& out){
 if(start||!actingPlayer||!key||!value||std::strcmp(key,"classname")||std::strcmp(value,"player")||!scoped()||!live(actingPlayer))return false;
 out=actingPlayer;return true;
}int mirroredTexts=0;
struct MirrorOp {int kind=0,value=0;float number=0;std::string text;};
struct Mirror {bool active=false;int dest=0,type=0;Edict* to=nullptr;std::vector<MirrorOp> ops;};
Mirror mirror;
void mirror_op(int kind,int value,float number,const char* text){if(mirror.active)mirror.ops.push_back({kind,value,number,text?text:""});}
void mirror_flush(){
 if(!mirror.active)return;const auto m=mirror;mirror={};int copies=0;
 for(int i=1;i<=load<int>(globals,0x90);++i){auto other=eng<Edict*(__cdecl*)(int)>(72)(i);void* p=nullptr;std::size_t n=0;if(other==m.to||!player_memory(other,p,n))continue;
  eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(m.dest,m.type,nullptr,other);
  for(const auto& op:m.ops){if(op.kind==52||op.kind==53)eng<void(__cdecl*)(float)>(op.kind)(op.number);else if(op.kind==54)eng<void(__cdecl*)(const char*)>(54)(op.text.c_str());else eng<void(__cdecl*)(int)>(op.kind)(op.value);}
  eng<void(__cdecl*)()>(47)();++copies;}
 if(copies&&mirroredTexts++==0){char line[120]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] STORYTEXT subtitle/story text copied to %d other player(s)\n",copies);log(line);}
}
// v0.5 (playtest: "stats show nothing"). cof_credits (decomp effects.cpp) sends each
// player "Credits" with their own stats; its 10th byte is IsMultiplayer(), which
// makes client.dll (Credits_guess.cpp 460/810) show the CO-OP stats panel instead,
// filled only by "CoopStats" from the native co-op campaign's cof_coop_stats,
// which the story maps never fire: the panel stayed empty. On story maps that
// byte reads 0, so each player gets the single-player stats screen with their stats.
int creditsBytes=-1;bool creditsLogged=false;
// v0.5 (D8): nightmare is always on offer in a co-op launch. The menu's difficulty
// page shows and accepts "skillset 4" only with unlock bit 0x20 (client
// Difficulty_guess.cpp; HUD m_iUnlocks from the "Unlocks" message, a long). This
// adapter is loaded by co-op launches only; no unlock file is written.
bool unlocksActive=false;
int credits_byte(int value){
 if(creditsBytes<0)return value;if(++creditsBytes!=10||value==0)return value;
 if(!creditsLogged){creditsLogged=true;log("[STORYCOOP_RC4] CREDITS stats screen: single-player layout with each player's own stats\n");}
 return 0;
}
void __cdecl message_begin(int dest,int type,const float* origin,Edict* e){
 creditsBytes=scoped()&&creditsMessage>0&&type==creditsMessage?0:-1;
 unlocksActive=verified&&unlocksMessage>0&&type==unlocksMessage;
 if(recorder_begin(dest,type,origin,e)){packet={};return;}
 if(logo_begin(dest,type,e)){packet={};reader={};mirror={};return;}
 if(score_begin(dest,type,e)){packet={};reader={};mirror={};return;}
 mirror={};if(scoped()&&firstPlayerLookup&&proFontMessage>0&&type==proFontMessage&&(dest==1||dest==8)&&live(e)&&std::strcmp(classname(e),"player")==0){mirror.active=true;mirror.dest=dest;mirror.type=type;mirror.to=e;}
 if(scoped()&&proFontMessage>0&&type==proFontMessage)strip_clear_sent(dest,e);
 if(scoped()&&(type==resetHud||type==initHud)&&type>0&&dest==1){strip_clear_arm(e);auto it=allocations.find(e);if(it!=allocations.end()){it->second.delivered={};it->second.hudDue=load<float>(globals,0)+1.0f;it->second.sent=0;}void* p=nullptr;std::size_t n=0;if(player_memory(e,p,n))view_entity_resync(p);}
 packet={};packet.active=scoped()&&type==addNote&&addNote>0&&dest==1&&live(e)&&std::strcmp(classname(e),"player")==0;
 packet.recipient=e;
 reader={};reader.active=scoped()&&noteDoc>0&&type==noteDoc&&dest==1&&live(e)&&std::strcmp(classname(e),"player")==0;
 if(packet.active){noteWire={};noteWire.active=true;noteWire.dest=dest;noteWire.type=type;noteWire.recipient=e;noteWire.hasOrigin=origin!=nullptr;if(origin)std::copy(origin,origin+3,noteWire.origin.begin());return;}
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(dest,type,origin,e);
}
void __cdecl write_byte(int value){value=credits_byte(value);if(logo_write(value)||score_write(48,value))return;mirror_op(48,value,0,nullptr);if(recorder_write(48,value,0,nullptr))return;if(packet.active){switch(packet.bytes++){case 0:packet.slot=value;break;case 1:packet.meta.codeFlag=value;break;case 2:packet.meta.codeType=value;break;default:packet.valid=false;}}
 if(!note_buffer(48,value))eng<void(__cdecl*)(int)>(48)(value);
}
void __cdecl write_short(int value){if(score_write(50,value))return;mirror_op(50,value,0,nullptr);if(recorder_write(50,value,0,nullptr))return;if(packet.active){if(packet.shorts++)packet.valid=false;packet.meta.codeValue=value;}
 if(!note_buffer(50,value))eng<void(__cdecl*)(int)>(50)(value);
}
void __cdecl write_string(const char* s){mirror_op(54,0,0,s);if(recorder_write(54,0,0,s))return;if(packet.active){if(packet.strings>=NoteFields.size()||!s)packet.valid=false;else{auto f=NoteFields[packet.strings];std::size_t n=0;while(n<f.capacity&&s[n])++n;if(n==f.capacity)packet.valid=false;else std::memcpy(packet.note.data()+f.offset,s,n+1);}++packet.strings;}
 std::string corrected;
 if(reader.active&&s){
  if(reader.strings==0){std::size_t length=0;while(length<32&&s[length])++length;if(length<32)reader.page.assign(s,length);else reader.active=false;}
  if(reader.strings==4)for(std::size_t i=0;i<journal.size();++i){const auto& note=journal.at(i);if(reader.page==reinterpret_cast<const char*>(note.data())&&std::strcmp(s,reinterpret_cast<const char*>(note.data()+0x40))==0){corrected=Journal::resource(note);break;}}
  ++reader.strings;
 }
 if(!corrected.empty()&&corrected!=s){char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] JOURNAL recall resource=%s page=%s\n",corrected.c_str(),reader.page.c_str());log(line);s=corrected.c_str();}
 if(!note_buffer(54,0,0,s))eng<void(__cdecl*)(const char*)>(54)(s);
}
void __cdecl message_end(){if(logo_end()||score_end())return;if(recorder_end()){packet={};reader={};return;}if(flush_note())eng<void(__cdecl*)()>(47)();
 if(packet.active){auto allocation=allocations.find(packet.recipient);if(allocation!=allocations.end()){
   auto& a=allocation->second;a.sent=0;a.hudDue=load<float>(globals,0)+0.5f;
   if(packet.valid&&packet.slot>=1&&packet.slot<=16&&packet.bytes==3&&packet.shorts==1&&packet.strings==11&&Journal::valid(packet.note)&&client_ready(packet.recipient))
    a.delivered[packet.slot-1]=note_signature(packet.note,packet.meta,false);
   else if(packet.slot>=1&&packet.slot<=16)a.delivered[packet.slot-1].clear();
  }
  if(packet.valid&&packet.slot>=1&&packet.slot<=16&&packet.bytes==3&&packet.shorts==1&&packet.strings==11){auto before=journal.size();auto rejected=journal.grant(packet.note,packet.meta);if(rejected&&packet.note[0x40])log("[STORYCOOP_RC4] JOURNAL packet rejected: invalid/conflicting/full; native packet forwarded\n");if(journal.size()!=before){char s[180]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] JOURNAL acquired=%u player=%d slot=%d id=%s page=%s codeType=%d\n",static_cast<unsigned>(journal.size()),eng<int(__cdecl*)(Edict*)>(71)(packet.recipient),packet.slot,Journal::resource(packet.note).c_str(),reinterpret_cast<const char*>(packet.note.data()),packet.meta.codeType);log(s);}}
  else log("[STORYCOOP_RC4] JOURNAL malformed packet; native packet forwarded, no grant\n");
 }packet={};reader={};
 mirror_flush();
}
void __cdecl write_char(int n){mirror_op(49,n,0,nullptr);if(!recorder_write(49,n,0,nullptr)&&!note_buffer(49,n))eng<void(__cdecl*)(int)>(49)(n);}
void __cdecl write_long(int n){if(unlocksActive)n|=0x20;mirror_op(51,n,0,nullptr);if(!recorder_write(51,n,0,nullptr)&&!note_buffer(51,n))eng<void(__cdecl*)(int)>(51)(n);}
void __cdecl write_angle(float n){mirror_op(52,0,n,nullptr);if(!recorder_write(52,0,n,nullptr)&&!note_buffer(52,0,n))eng<void(__cdecl*)(float)>(52)(n);}
void __cdecl write_coord(float n){mirror_op(53,0,n,nullptr);if(!recorder_write(53,0,n,nullptr)&&!note_buffer(53,0,n))eng<void(__cdecl*)(float)>(53)(n);}
void __cdecl write_entity(int n){mirror_op(55,n,0,nullptr);if(!recorder_write(55,n,0,nullptr)&&!note_buffer(55,n))eng<void(__cdecl*)(int)>(55)(n);}
std::int32_t __cdecl random_long(std::int32_t low,std::int32_t high){auto generated=eng<std::int32_t(__cdecl*)(std::int32_t,std::int32_t)>(90)(low,high),result=generated;puzzle_random_long_override(low,high,generated,result);return result;}
Edict* __cdecl find(Edict* start,const char* key,const char* value){Edict* out=nullptr;if(!start&&key&&value&&std::strcmp(key,"classname")==0&&std::strcmp(value,"player")==0)firstPlayerLookup=true;if(puzzle_find_override(start,key,value,out))return out;if(acting_override(start,key,value,out))return out;if(ladder_find_override(start,key,value,out))return out;out=eng<Edict*(__cdecl*)(Edict*,const char*,const char*)>(14)(start,key,value);fanout_lookup(start,key,value,out);bag_lookup(start,key,value,out);give_lookup(start,key,value,out);regroup_lookup(start,key,value,out);evidence_lookup(start,key,value);door_lock_lookup(start,key,value,out);return out;}
void __cdecl emit_sound(Edict* e,int channel,const char* sample,float volume,float attenuation,int flags,int pitch){
 if(sample&&(std::strcmp(sample,"items/padl_turn.wav")==0||std::strcmp(sample,"items/padl_unlock.wav")==0)){
  if(auto actor=puzzle_sound_actor(e))e=actor;
 }
 evidence_sound(e,sample);
 // Forward the original event once: retain asset/channel/volume/timing. No
 // client-command-generated second sound or synthetic success response.
 eng<void(__cdecl*)(Edict*,int,const char*,float,float,int,int)>(29)(e,channel,sample,volume,attenuation,flags,pitch);
}
std::uint32_t __cdecl from_name(const char* name){auto p=name?GetProcAddress(backend,name):nullptr;return p?reinterpret_cast<std::uint32_t>(p):eng<std::uint32_t(__cdecl*)(const char*)>(78)(name);}
const char* __cdecl name_for(std::uint32_t f){for(const auto& item:exportedNames)if(item.first==f)return item.second.c_str();return eng<const char*(__cdecl*)(std::uint32_t)>(79)(f);}
void collect(Edict* e){void* p=nullptr;std::size_t n=0;if(!scoped()||!player_memory(e,p,n))return;
 const auto rejected=journal.collect(p,n);auto& a=allocations.at(e);
 if(rejected){std::uint32_t hash=2166136261u;auto bytes=static_cast<const unsigned char*>(p)+JournalOffset;for(std::size_t i=0;i<0x120*NoteLimit;++i)hash=(hash^bytes[i])*16777619u;
  if(!hash)hash=1;
  if(hash!=a.rejection){a.rejection=hash;char s[160]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] JOURNAL bank rejected player=%d count=%u; native bank PRESERVED; repeated identical warning suppressed\n",eng<int(__cdecl*)(Edict*)>(71)(e),static_cast<unsigned>(rejected));log(s);}
 }else a.rejection=0;
}
bool client_ready(Edict* e){auto it=allocations.find(e);return it!=allocations.end()&&it->second.commands&&load<float>(globals,0)>=it->second.hudDue;}
void restore(Edict* e,bool initial=false){
 void* p=nullptr;std::size_t n=0;if(!scoped()||!player_memory(e,p,n))return;
 puzzle_on_frame(e,p,n);collect(e);auto& a=allocations.at(e);if(a.rejection)return;const bool changed=journal.restore(p,n);
 if(initial||changed||a.sent!=journal.generation()){a.sent=journal.generation();a.next=0;a.due=0;}
 if(!client_ready(e))return;
 while(a.next<journal.size()&&a.delivered[a.next]==note_signature(journal.at(a.next),journal.metadata(a.next),true))++a.next;
 if(!addNote||a.next>=journal.size())return;
 const float now=load<float>(globals,0);if(now<a.due)return;
 const auto& note=journal.at(a.next);
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,addNote,nullptr,e);
 const auto meta=layout_metadata(e,static_cast<int>(a.next+1),journal.metadata(a.next),true);
 if(meta.codeType>0)code_trace("replay",e,static_cast<int>(a.next+1),meta,true);
 eng<void(__cdecl*)(int)>(48)(static_cast<int>(a.next+1));eng<void(__cdecl*)(int)>(48)(meta.codeFlag);
 eng<void(__cdecl*)(int)>(50)(meta.codeValue);eng<void(__cdecl*)(int)>(48)(meta.codeType);
 const auto resource=Journal::resource(note);
 for(std::size_t i=0;i<NoteFields.size();++i)eng<void(__cdecl*)(const char*)>(54)(i==0?resource.c_str():reinterpret_cast<const char*>(note.data()+NoteFields[i].offset));
 eng<void(__cdecl*)()>(47)();a.delivered[a.next]=note_signature(note,journal.metadata(a.next),true);++a.next;a.due=now+0.1f;
 if(a.notified<a.next){
  char line[180]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] JOURNAL delivered player=%d slot=%u id=%s page=%s\n",eng<int(__cdecl*)(Edict*)>(71)(e),static_cast<unsigned>(a.next),resource.c_str(),reinterpret_cast<const char*>(note.data()));log(line);
  if(textMessage){const auto title=reinterpret_cast<const char*>(note.data()+0xb0);std::string notice=std::string("Archived: ")+(title[0]?title:resource.c_str())+"\n";eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,textMessage,nullptr,e);eng<void(__cdecl*)(int)>(48)(1);eng<void(__cdecl*)(const char*)>(54)(notice.c_str());eng<void(__cdecl*)()>(47)();}
  a.notified=a.next;
 }
}
void everyone(bool capture){if(!globals)return;for(int i=1;i<=load<int>(globals,0x90);++i){auto e=eng<Edict*(__cdecl*)(int)>(72)(i);if(capture)collect(e);else restore(e);}}
#include "world-runtime.hpp"
#include "checkpoint-runtime.hpp"
#include "keydoor-runtime.hpp"
#include "ladder-runtime.hpp"
#include "pp-runtime.hpp"
#include "session-runtime.hpp"
#include "story-runtime.hpp"
#include "coop-runtime.hpp"
#include "state-runtime.hpp"
#include "client-menu.hpp"
void frame_evidence();
void __cdecl start_frame(){crumb("start_frame");++netFrame;collision_restore();gameui_patch();firstPlayerLookup=false;pause_poll();menu_credit_frame();give_flush();three_slots_frame();chapter_frame();join_wait_frame();regroup_frame();checkpoint_start_frame();keypad_frame();story_frame();logo_frame();score_sync();keydoor_frame();ladder_frame();pp_frame();half_frame();skip_frame();party_counters_frame();prologue_skip_frame();boss_frame();train_escape_frame();floor_walls_frame();stayout_fades_frame();wardrobe_frame();join_connect_frame();load_wait_frame();door_lock_frame();item_copies_frame();frame_evidence();}
void __cdecl game_init(){crash_install();reset();native<void(__cdecl*)()>(0)();if(engine[140])eng<void(__cdecl*)(const char*,void(__cdecl*)())>(140)("sc_checkpoint_load",&checkpoint_load_command);if(engine[56])eng<void(__cdecl*)(cvar_t*)>(56)(&collisionCvar);if(engine[56])eng<void(__cdecl*)(cvar_t*)>(56)(&extrasCvar);if(engine[140])eng<void(__cdecl*)(const char*,void(__cdecl*)())>(140)("sc_checkpoint_prepare",&checkpoint_prepare_command);if(engine[140])eng<void(__cdecl*)(const char*,void(__cdecl*)())>(140)("sc_door_music",&door_music_command);log("[STORYCOOP_RC4] GameInit forwarded; checkpoint command registered\n");}
// v0.3 audit: the main menu map (c_game_menu1) ends every game. Only the mod's
// New Game/Load entries and cof_campaign_story4 reset the session, so a game
// started another way in the same process (the tester went c_game_menu1 ->
// cof_campaign_01_p2 -> s4_apartment1_e0) got the previous game's kept maps,
// carried inventories and a sticky failed-restore flag. A Continue already
// prepared for the next map is kept.
void menu_reset(){
 auto value=std::move(loadedCheckpoint);const bool prepared=checkpointPrepared,backup=checkpointPreparedBackup;const int slot=checkpointSlot;
 reset();extrasUnlocked=false;loadedCheckpoint=std::move(value);checkpointPrepared=prepared;checkpointPreparedBackup=backup;checkpointSlot=slot;
}
int __cdecl spawn(Edict* e){
 const bool world=std::strcmp(classname(e),"worldspawn")==0;crumb("spawn",e);
 if(world){camera_reset();netLogged.clear();openingThinks.clear();vehiclePackets.clear();openingViews.clear();openingTimeline=-1;checkpointMapTime=load<float>(globals,0);if(verified&&std::strcmp(map_name(),"cof_campaign_story4")==0)reset();else if(verified&&std::strcmp(map_name(),"c_game_menu1")==0)menu_reset();}
 const bool computer=scoped()&&std::strcmp(classname(e),"cof_computer")==0&&(std::strcmp(map_name(),"s4_start2")==0||std::strcmp(map_name(),"s2_start2")==0)&&puzzle_before_computer_spawn(e);
 story_door_locked_sound(e);
 // v0.3 map state: the entity's BSP identity, read before its own Spawn runs.
 std::string stateKey;bool dm=false;if(scoped()){if(world)state_map_begin();stateKey=state_identity(e);auto p=vars(e);dm=p&&(load<unsigned>(p,0x1a4)&2048u);}
 int result=native<int(__cdecl*)(Edict*)>(1)(e);if(world){checkpoint_world_ready();level_change_release();chapter_world_started();}
 if(world&&scoped())deathmatch_open();   // v0.5 RC-2048 (world-runtime.hpp)
 if(scoped())world_spawned(e,world,result,stateKey,dm);
 // User request: shared notes must glow red like every other pickup. Native
 // CPasswordNote::Spawn (100835A6) sets renderfx 100 (+0x160); the shared-note
 // visual props (env_model "cof_noteN_visual") never had it.
 // coop-fixes-004 (user: the glow makes notes stand out; apply it to every
 // note): any note-model prop without its own render effect gets the same glow.
 if(scoped()&&result==0&&(std::strcmp(classname(e),"env_model")==0||std::strcmp(classname(e),"cof_hoodie")==0))if(auto p=vars(e)){
  const char* name=targetname(e);const std::size_t length=name?std::strlen(name):0;
  const bool shared=length>15&&std::strncmp(name,"cof_note",8)==0&&std::strcmp(name+length-7,"_visual")==0;
  const char* model=string_at(load<int>(p,0xb8));const bool note=model&&_stricmp(model,"models/items/generic_note.mdl")==0;
  if(shared||(note&&load<int>(p,0x160)==0))store(p,0x160,100);
 }
 if(opening()&&std::strcmp(classname(e),"func_tracktrain")==0){char s[200]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4_HOTFIX] VEHICLE_SPAWN %s nativeResult=%d edict=%d\n",targetname(e),result,eng<int(__cdecl*)(Edict*)>(71)(e));log(s);}
 if(opening()&&title_vehicle(e))vehicle_evidence(e,"AFTER_SPAWN",0,-1);
 if(computer){
  auto p=vars(e);if(result==0&&p){auto u=load<std::int32_t>(p,0x24c),w=load<std::int32_t>(p,0x250);puzzle_after_computer_spawn(e,u,w);}else puzzle_cancel_computer_spawn();
 }
 return result;
}
void __cdecl think(Edict* e){crumb("think",e);firstPlayerLookup=false;if(!scoped()){opening_think_evidence(e);native<void(__cdecl*)(Edict*)>(2)(e);return;}auto decision=puzzle_before_think(e);if(decision==PuzzleDecision::Block)return;drowned_begin(e);Edict* acting=barPlayer&&serial(barPlayer)==barPlayerSerial&&!std::strncmp(classname(e),"cof_puzzlebar",13)?barPlayer:roofboss_enemy(e);if(!acting)acting=strangle_activator(e);const bool bar=acting!=nullptr;if(bar)actingPlayer=acting;native<void(__cdecl*)(Edict*)>(2)(e);if(bar)actingPlayer=nullptr;drowned_end();firstPlayerLookup=false;if(decision==PuzzleDecision::ForwardOwned)puzzle_after_think();give_flush();}
void __cdecl put(Edict* e){crumb("putinserver",e);native<void(__cdecl*)(Edict*)>(18)(e);void* p=nullptr;std::size_t n=0;if(scoped()&&player_memory(e,p,n))puzzle_on_player_initialized(e,p,n);}
void __cdecl client_command(Edict* e){
 crumb("command",e);
 const char* early=eng<const char*(__cdecl*)(int)>(83)(0);menu_command(early);stuck_command(e,early);if(model_box_command(e,early)||join_box_command(e,early)||skip_client_command(e,early))return;
 // Every story co-op map, the passive opening included: no police flashlight.
 if(campaign_map()&&early&&!std::strcmp(early,"flashlight")){if(!light_command(e,early)&&!openingLightLogged){openingLightLogged=true;log("[STORYCOOP_RC4] LIGHT police flashlight refused (F key); other lights allowed\n");}return;}
 if(!scoped()){native<void(__cdecl*)(Edict*)>(19)(e);return;}
 const char* command=eng<const char*(__cdecl*)(int)>(83)(0);
 if(save_window_command(e,command))return;if(save_choice_command(e,command))return;
 if(checkpoint_command(e,command))return;
 if(dual_command(e,command))return;
 auto decision=puzzle_before_client_command(e,command);if(decision==PuzzleDecision::Block){log("[STORYCOOP_RC4] blocked unsafe/concurrent padlock input; Cancel remains available\n");return;}
 // v0.3: evidence for inventory item use (cards, native sounds, locks in reach).
 const bool itemCommand=item_command(command);std::vector<std::string> cardsBefore;std::string itemSlot;const std::string itemName=command?command:"";
 if(itemCommand){cardsBefore=item_cards(e);const char* a=eng<const char*(__cdecl*)(int)>(83)(1);itemSlot=a?std::string(a).substr(0,8):"";evidence_item_begin(command);}
 // v0.3: inside this player's own inventory use, "the player" is them. The
 // native func_button CBaseButton::UnlockDoor (buttons.cpp, decomp) looks up
 // the FIRST player for the activator, "You have placed the rope." and the
 // unlock sound, while InventoryUse spends the card of whoever used it.
 if(itemCommand&&!std::strcmp(command,"inventoryuse"))actingPlayer=e;
 if(command&&!std::strcmp(command,"boilercheck")&&barPlayer!=e){barPlayer=e;barPlayerSerial=serial(e);char line[100]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] PUZZLEBAR player=%d turns the bars\n",eng<int(__cdecl*)(Edict*)>(71)(e));log(line);}
 const std::string quickArg=itemName=="quicksel"?[]{const char* a=eng<const char*(__cdecl*)(int)>(83)(1);return std::string(a?a:"").substr(0,4);}():std::string();
 native<void(__cdecl*)(Edict*)>(19)(e);actingPlayer=nullptr;
 quick_dual_command(e,itemName.c_str(),quickArg);
 if(itemCommand)evidence_item_end(e,itemName.c_str(),itemSlot.c_str(),cardsBefore);
 if(decision==PuzzleDecision::ForwardOwned)puzzle_after_client_command();
 auto before=journal.size();collect(e);if(journal.size()!=before){char s[100]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] JOURNAL acquired=%u shared after native command\n",static_cast<unsigned>(journal.size()));log(s);everyone(false);}
}
void __cdecl activate(Edict* list,int count,int clients){
 // v0.3: the map's saved state goes back before ServerActivate, as in a single-player level load.
 deathmatch_close();crash_install();crumb("activate mapstate");puzzle_map_changed();native<void(__cdecl*)(Edict*,int,int)>(21)(list,state_activate_edicts(count),clients);crumb("activate adapter");syringe_limit_patch();
 char s[200]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] MAP %s journal=%u puzzle=%u scope=%u\n",map_name(),static_cast<unsigned>(journal.size()),puzzle_has_solution()?1u:0u,scoped()?1u:0u);log(s);
 if(opening())log("[STORYCOOP_RC4_HOTFIX] OPENING_PASSIVE native callbacks; journal/puzzle player writes disabled\n");
 keydoor_activate();ladder_activate();pp_activate();model_activate();pause_activate();story_door_report();if(scoped())checkpoint_world_activate();
 coop_version_redirect();story_entry_redirect();
}
void __cdecl deactivate(){deathmatch_close();checkpoint_game_over_check();state_deactivate();join_box_close(nullptr);model_box_close(nullptr);crumb("deactivate");everyone(true);for(int i=1;i<=load<int>(globals,0x90);++i){checkpoint_capture_departing(eng<Edict*(__cdecl*)(int)>(72)(i));carry_departing(eng<Edict*(__cdecl*)(int)>(72)(i));}carry_map_changed();coop_map_changed();story_adopt();story_map_changed();rejoin_map_changed();logo_reset();score_reset();netTuned.clear();pose_reset();loadingCheckpoint=false;restoredPlayers.clear();native<void(__cdecl*)()>(22)();camera_reset();keydoor_reset();ladder_map_reset();pp_reset();boat_reset();keypad_map_changed();objective_reset();light_reset();pause_reset();menu_credit_reset();saveChoice={};mirror={};spawnRecords.clear();allocations.clear();puzzle_map_changed();}
// Movement/jitter evidence: each client's own network settings once per map,
// and server frame gaps. Observational only; no rate or prediction changes.
void client_net_evidence(Edict* e){
 if(!engine[106]||!engine[107]||!engine[57]||!live(e)||!netLogged.insert(eng<int(__cdecl*)(Edict*)>(71)(e)).second)return;
 auto buffer=eng<char*(__cdecl*)(Edict*)>(106)(e);if(!buffer)return;
 auto key=[&](const char* k){const char* v=eng<char*(__cdecl*)(char*,char*)>(107)(buffer,const_cast<char*>(k));return std::string(v&&*v?v:"-");};
 const auto rate=key("rate"),update=key("cl_updaterate"),lw=key("cl_lw"),lc=key("cl_lc");auto cvar=[](const char* n){return eng<float(__cdecl*)(const char*)>(57)(n);};
 char s[260]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] NET player=%d host=%d rate=%s cl_updaterate=%s cl_lw=%s cl_lc=%s sv_maxupdaterate=%.0f sv_maxrate=%.0f fps_max=%.0f map=%s\n",eng<int(__cdecl*)(Edict*)>(71)(e),eng<int(__cdecl*)(Edict*)>(71)(e)==1?1:0,rate.c_str(),update.c_str(),lw.c_str(),lc.c_str(),cvar("sv_maxupdaterate"),cvar("sv_maxrate"),cvar("fps_max"),map_name());log(s);
}
void frame_evidence(){
 if(!globals)return;const float now=load<float>(globals,0);
 if(lastFrame<0||now<lastFrame){lastFrame=now;frameWindow=now;frameGaps=0;maxGap=0;return;}
 const float gap=now-lastFrame;lastFrame=now;if(gap>0.1f)++frameGaps;if(gap>maxGap)maxGap=gap;
 if(now-frameWindow>=60.f){if(frameGaps){char s[160]{};std::snprintf(s,sizeof(s),"[STORYCOOP_RC4] SERVER_FRAME last60s gaps>100ms=%d max=%.0fms map=%s\n",frameGaps,maxGap*1000.f,map_name());log(s);}frameWindow=now;frameGaps=0;maxGap=0;}
}
void __cdecl command_start(const Edict* actor,const void* command,unsigned int seed){
 native<void(__cdecl*)(const Edict*,const void*,unsigned int)>(43)(actor,command,seed);
 auto e=const_cast<Edict*>(actor);if(verified){client_net_evidence(e);net_tune(e);menu_credit_active(e);prologue_skip_command(e,command);}if(!scoped())return;idle_command(e,command);skip_command(e,command);auto it=allocations.find(e);
 if(it!=allocations.end()&&!it->second.commands){it->second.commands=true;it->second.hudDue=load<float>(globals,0)+1.0f;it->second.delivered={};it->second.sent=0;log("[STORYCOOP_RC4] CLIENT active commands; waiting for native HUD to settle\n");}
}
// Ladder reservations are refreshed after each player's think as well as at
// StartFrame, so a second player's Use later in the same frame sees the lock.
void __cdecl prethink(Edict* e){crumb("prethink",e);objective_share();deploy_guard(e);native<void(__cdecl*)(Edict*)>(23)(e);collision_hold(e);strip_clear_frame(e);idle_stage(e,1);fall_frame(e);scene_teleport_frame(e);model_frame(e);name_frame(e);light_frame(e);camera_frame(e);camera_join(e);checkpoint_player_frame(e);restore(e);quick_notice_frame(e);carry_frame(e);quest_notice_frame(e);arrival_frame(e);equip_frame(e);nightmare_frame(e);morphine_frame(e);limp_frame(e);give_owed_frame(e);rejoin_frame(e);ladder_frame();boat_frame(e);idle_stage(e,2);collision_pre(e);}
// v0.1: the inventory carry reads every player when a level change is asked
// for (see carry_level_change), then the engine's own ChangeLevel runs.
// v0.3: the Drowned's victim lookup (see drowned_index in coop-runtime.hpp).
Edict* __cdecl entity_of_index(int index){return drowned_index(index,eng<Edict*(__cdecl*)(int)>(72)(index));}
void __cdecl change_level(char* map,char* landmark){carry_level_change(map);state_level_change(map);eng<void(__cdecl*)(char*,char*)>(6)(map,landmark);}
// v0.3: trigger_command exits run "changelevel <map>" as a server command.
void __cdecl server_command_hook(char* text){
 if(text&&scoped()){const char* s=text;while(*s==' '||*s=='\t')++s;if(!_strnicmp(s,"changelevel",11)&&(s[11]==' '||s[11]=='\t')){s+=11;while(*s==' '||*s=='\t')++s;char map[40]{};int n=0;while(*s&&*s!=' '&&*s!=';'&&*s!='\n'&&*s!='\r'&&n<39)map[n++]=*s++;state_level_change(map);}}
 eng<void(__cdecl*)(char*)>(39)(text);
}
void __cdecl key_value(Edict* e,KeyValue* kv){if(extras_keyvalue_dropped(kv)){kv->handled=1;return;}native<void(__cdecl*)(Edict*,KeyValue*)>(6)(e,kv);evidence_keyvalue(e,kv);ladder_keyvalue(e,kv);door_global_keyvalue(e,kv);state_keyvalue(e,kv);}
void __cdecl postthink(Edict* e){crumb("postthink",e);collision_restore();idle_stage(e,-1);deploy_guard(e);native<void(__cdecl*)(Edict*)>(24)(e);door_hide_frame(e);idle_stage(e,3);light_frame(e);pose_frame(e);ladder_frame();pp_frame();idle_stage(e,4);}
void __cdecl disconnect(Edict* e){pauseState.othersOpen=false;carry_departing(e);stuckUsed.erase(e);codeLayouts.erase(note_connection(e));camera_forget(e);boatDrivers.erase(e);checkpoint_disconnect(e);}
// v0.3 crash report: Use/Touch only leave a breadcrumb, then the game's own.
void __cdecl dispatch_use(Edict* e,Edict* other){crumb("use",e);native<void(__cdecl*)(Edict*,Edict*)>(3)(e,other);}
void __cdecl dispatch_touch(Edict* e,Edict* other){crumb("touch",e);if(scoped()){pp_touch(e,other);train_escape_touch(e,other);if(collision_touch_blocked(e,other))return;}native<void(__cdecl*)(Edict*,Edict*)>(4)(e,other);}
void wrap(Table& t){
 original=t;
 t[0]=reinterpret_cast<void*>(&game_init);t[1]=reinterpret_cast<void*>(&spawn);t[2]=reinterpret_cast<void*>(&think);
 if(t[6])t[6]=reinterpret_cast<void*>(&key_value);t[18]=reinterpret_cast<void*>(&put);t[19]=reinterpret_cast<void*>(&client_command);
 t[21]=reinterpret_cast<void*>(&activate);t[22]=reinterpret_cast<void*>(&deactivate);t[23]=reinterpret_cast<void*>(&prethink);
 if(t[24])t[24]=reinterpret_cast<void*>(&postthink);
 t[16]=reinterpret_cast<void*>(&disconnect);t[25]=reinterpret_cast<void*>(&start_frame);
 t[39]=reinterpret_cast<void*>(&full_pack);if(t[3])t[3]=reinterpret_cast<void*>(&dispatch_use);if(t[4])t[4]=reinterpret_cast<void*>(&dispatch_touch);
 t[37]=reinterpret_cast<void*>(&visibility);t[43]=reinterpret_cast<void*>(&command_start);
}
}
}

extern "C" void __stdcall GiveFnptrsToDll(void* table,void* globals){
 using namespace rc4;if(!table||!globals)return;std::memcpy(engine.data(),table,sizeof(engine));rc4::globals=globals;
 if(!load_backend())return;
 client_menu_patch();menu_hold_start();
 forwardEngine=engine;
 forwardEngine[14]=reinterpret_cast<void*>(&find);forwardEngine[63]=reinterpret_cast<void*>(&allocate);
 forwardEngine[65]=reinterpret_cast<void*>(&release);forwardEngine[75]=reinterpret_cast<void*>(&register_message);
 forwardEngine[78]=reinterpret_cast<void*>(&from_name);forwardEngine[79]=reinterpret_cast<void*>(&name_for);
 forwardEngine[46]=reinterpret_cast<void*>(&message_begin);forwardEngine[47]=reinterpret_cast<void*>(&message_end);
 forwardEngine[48]=reinterpret_cast<void*>(&write_byte);forwardEngine[50]=reinterpret_cast<void*>(&write_short);forwardEngine[54]=reinterpret_cast<void*>(&write_string);
 forwardEngine[90]=reinterpret_cast<void*>(&random_long);
 forwardEngine[6]=reinterpret_cast<void*>(&change_level);
 forwardEngine[72]=reinterpret_cast<void*>(&entity_of_index);
 forwardEngine[39]=reinterpret_cast<void*>(&server_command_hook);
 forwardEngine[29]=reinterpret_cast<void*>(&emit_sound);
 forwardEngine[92]=reinterpret_cast<void*>(&set_view);forwardEngine[95]=reinterpret_cast<void*>(&load_file_hook);
 forwardEngine[41]=reinterpret_cast<void*>(&stuff_command);
 forwardEngine[49]=reinterpret_cast<void*>(&write_char);forwardEngine[51]=reinterpret_cast<void*>(&write_long);forwardEngine[52]=reinterpret_cast<void*>(&write_angle);forwardEngine[53]=reinterpret_cast<void*>(&write_coord);forwardEngine[55]=reinterpret_cast<void*>(&write_entity);
 configure_puzzle({&scoped,&player_memory,&serial,&classname,&resolve,&solved});
 reinterpret_cast<void(__stdcall*)(void*,void*)>(GetProcAddress(backend,"GiveFnptrsToDll"))(forwardEngine.data(),globals);
}
extern "C" int __cdecl GetEntityAPI(void* table,int version){
 using namespace rc4;if(!table||version!=140||!verified)return 0;
 Table t{};int result=reinterpret_cast<int(__cdecl*)(void*,int)>(GetProcAddress(backend,"GetEntityAPI"))(t.data(),version);
 if(!result)return 0;for(auto i:{0,1,2,18,19,21,22,23})if(!t[i])return 0;
 wrap(t);std::memcpy(table,t.data(),sizeof(t));return result;
}
extern "C" int __cdecl GetEntityAPI2(void* table,int* version){
 if(!version)return 0;if(*version!=140){*version=140;return 0;}return GetEntityAPI(table,*version);
}
// v0.4: leave the exception handlers before the engine unloads this DLL (session-runtime.hpp crash_uninstall).
extern "C" BOOL WINAPI DllMain(HINSTANCE,DWORD reason,LPVOID){if(reason==DLL_PROCESS_DETACH){rc4::menu_hold_stop();rc4::crash_uninstall();}return TRUE;}
