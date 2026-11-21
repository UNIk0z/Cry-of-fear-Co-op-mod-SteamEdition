// Owned mock engine/native callbacks. This is NOT a Cry of Fear runtime test.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
namespace rc4 {struct Edict{std::array<unsigned char,0x400> data{};};}
using namespace rc4;
namespace {
std::array<Edict,6> edicts{};
std::array<unsigned char,0xa0> g{};
std::unordered_map<Edict*,std::vector<unsigned char>> memory;
int nativeCommands=0,nativePuts=0,nativePrethinks=0,nativeActivate=0,nativeDeactivate=0,nativeSpawns=0,nativeThinks=0;
std::string command;
std::vector<int> prefix;
std::vector<std::string> payload;
int messages=0;
struct Wire{int type;Edict* recipient;std::vector<int> prefix;std::vector<std::string> strings;};
int wireType=0;Edict* wireRecipient=nullptr;std::vector<Wire> wires;
std::vector<std::string> logs;
int sounds=0;Edict* soundSource=nullptr;std::string soundSample;
void sound(Edict* e,int channel,const char* sample,float volume,float attenuation,int flags,int pitch){++sounds;soundSource=e;soundSample=sample;assert(channel==4&&volume==1&&attenuation==0.8f&&flags==0&&pitch==100);}
const char* strings(int n){switch(n){case 1:return "player";case 2:return "cof_computer";case 3:return "worldspawn";case 4:return "item_padlock";case 5:return "multi_manager";case 6:return "kunitei2";case 7:return "func_tracktrain";case 8:return "skittag";case 10:return "cof_campaign_story4";case 11:return "s4_start2";case 12:return "s4_start_e0";case 13:return "s2_apartment1_e7";case 14:return "s4_nightmare";}return "";}
int index_of(Edict* e){return static_cast<int>(e-edicts.data());}
Edict* by_index(int i){return i>=0&&i<6?&edicts[i]:nullptr;}
void* vars_of(Edict* e){return e->data.data()+0x80;}
void* priv(Edict* e){auto found=memory.find(e);return found==memory.end()?nullptr:found->second.data();}
void* alloc(Edict* e,int size){memory[e]=std::vector<unsigned char>(size);return memory[e].data();}
void free_private(Edict* e){memory.erase(e);}
const char* argv(int i){return i==0?command.c_str():"";}
void print(const char* text){logs.emplace_back(text);}
void begin_message(int,int type,const float*,Edict* e){wireType=type;wireRecipient=e;prefix.clear();payload.clear();}
void end_message(){++messages;wires.push_back({wireType,wireRecipient,prefix,payload});}
void byte(int n){prefix.push_back(n);}
void short_(int n){prefix.push_back(n);}
void str(const char* n){payload.emplace_back(n);}
int user_message(const char* name,int){return std::strcmp(name,"NoteDoc")==0?8:std::strcmp(name,"TextMsg")==0?9:7;}
Edict* find_(Edict*,const char*,const char*){return &edicts[1];}
int rng(int,int){return 7;}
void native_put(Edict* e){++nativePuts;auto p=allocate(e,PlayerBytes);std::array<int,4> digits=index_of(e)==1?std::array<int,4>{1,2,3,4}:std::array<int,4>{9,8,7,6};std::memcpy(static_cast<unsigned char*>(p)+0x235c,digits.data(),16);}
void native_pre(Edict*){++nativePrethinks;}
void native_activate(Edict*,int,int){++nativeActivate;}
void native_deactivate(){++nativeDeactivate;}
int native_spawn(Edict*){++nativeSpawns;return -1;}
void native_think(Edict*){++nativeThinks;}
int invisible(Edict*,unsigned char*){return 0;}
int native_pack(void*,int,Edict* e,Edict*,int,int,unsigned char* set){return eng<int(__cdecl*)(Edict*,unsigned char*)>(126)(e,set);}
void native_visibility(Edict*,Edict*,unsigned char**,unsigned char**){}
void emit(Edict* e,const Note& n,NoteMetadata meta,int slot=1){message_begin(1,7,nullptr,e);write_byte(slot);write_byte(meta.codeFlag);write_short(meta.codeValue);write_byte(meta.codeType);for(auto f:NoteFields)write_string(reinterpret_cast<const char*>(n.data()+f.offset));message_end();}
Note make_note(const char* name){Note n{};std::strcpy(reinterpret_cast<char*>(n.data()+0x40),name);std::strcpy(reinterpret_cast<char*>(n.data()),"page_orig");std::strcpy(reinterpret_cast<char*>(n.data()+0xb0),"Original title");std::strcpy(reinterpret_cast<char*>(n.data()+0xf0),"Original label");return n;}
void native_command(Edict* e){++nativeCommands;if(command=="closedocument"){auto note=make_note("note_orig_1");std::memcpy(static_cast<unsigned char*>(priv(e))+JournalOffset,note.data(),note.size());emit(e,note,{});}else if(command=="computercheck"){auto note=make_note("nerds");std::memcpy(static_cast<unsigned char*>(priv(e))+JournalOffset+0x120,note.data(),note.size());emit(e,note,{1,1234,1});}}
void empty(){}
void set_map(int n){store(g.data(),0x0c,n);}
void tick(Edict* e){auto time=load<float>(g.data(),0);store(g.data(),0,time+2.0f);auto it=allocations.find(e);if(it!=allocations.end())it->second.commands=true;prethink(e);}
}
int main(){
 globals=g.data();verified=true;store(g.data(),0x90,2);store(g.data(),0x94,6);set_map(11);
 for(int i=0;i<6;++i){store(&edicts[i],0,0);store(&edicts[i],4,i+1);store(vars_of(&edicts[i]),0,i==3?2:1);}
 engine[14]=reinterpret_cast<void*>(&find_);engine[46]=reinterpret_cast<void*>(&begin_message);engine[47]=reinterpret_cast<void*>(&end_message);
 engine[29]=reinterpret_cast<void*>(&sound);
 engine[126]=reinterpret_cast<void*>(&invisible);
 engine[48]=reinterpret_cast<void*>(&byte);engine[50]=reinterpret_cast<void*>(&short_);engine[54]=reinterpret_cast<void*>(&str);
 engine[63]=reinterpret_cast<void*>(&alloc);engine[64]=reinterpret_cast<void*>(&priv);engine[65]=reinterpret_cast<void*>(&free_private);
 engine[66]=reinterpret_cast<void*>(&strings);engine[68]=reinterpret_cast<void*>(&vars_of);engine[71]=reinterpret_cast<void*>(&index_of);engine[72]=reinterpret_cast<void*>(&by_index);
 engine[75]=reinterpret_cast<void*>(&user_message);engine[81]=reinterpret_cast<void*>(&print);engine[83]=reinterpret_cast<void*>(&argv);engine[90]=reinterpret_cast<void*>(&rng);
 configure_puzzle({&scoped,&player_memory,&serial,&classname,&resolve,&solved});
 Table table{};for(auto& p:table)p=reinterpret_cast<void*>(&empty);
 table[1]=reinterpret_cast<void*>(&native_spawn);table[2]=reinterpret_cast<void*>(&native_think);table[18]=reinterpret_cast<void*>(&native_put);table[19]=reinterpret_cast<void*>(&native_command);table[21]=reinterpret_cast<void*>(&native_activate);table[22]=reinterpret_cast<void*>(&native_deactivate);table[23]=reinterpret_cast<void*>(&native_pre);
 table[39]=reinterpret_cast<void*>(&native_pack);
 table[37]=reinterpret_cast<void*>(&native_visibility);
 const auto untouched=table;wrap(table);for(std::size_t i=0;i<50;++i)if(i!=0&&i!=1&&i!=2&&i!=3&&i!=4&&i!=6&&i!=9&&i!=16&&i!=18&&i!=19&&i!=21&&i!=22&&i!=23&&i!=24&&i!=25&&i!=37&&i!=39&&i!=43)assert(table[i]==untouched[i]);
 assert(table[3]!=untouched[3]&&table[4]!=untouched[4]); // v0.3 crash report: Use/Touch leave a breadcrumb, then forward
 assert(table[6]!=untouched[6]); // v0.3 KeyValue: records lockedby keys for the ITEMUSE evidence line
 assert(table[9]!=untouched[9]); // v0.10.8 SetAbsBox: a monster's abs box covers what it plays (coop-runtime.hpp monster_abs_box)
 assert(table[24]!=untouched[24]); // PostThink: ladder reservation refresh after each player's think
 assert(register_message("AddNote",-1)==7);put(&edicts[1]);put(&edicts[2]);assert(nativePuts==2&&puzzle_has_solution());
 auto solution=puzzle_solution();assert((solution==std::array<int,4>{1,2,3,4}));assert(!std::memcmp(static_cast<unsigned char*>(priv(&edicts[2]))+0x235c,solution.data(),16));
 store(vars_of(&edicts[3]),0,4);store(priv(&edicts[2]),0x2350,reinterpret_cast<std::uint32_t>(&edicts[3]));store(priv(&edicts[2]),0x2354,4);
 assert(puzzle_before_client_command(&edicts[2],"padlockrotate")==PuzzleDecision::ForwardOwned);
 emit_sound(&edicts[3],4,"items/padl_turn.wav",1,0.8f,0,100);assert(sounds==1&&soundSource==&edicts[2]&&soundSample=="items/padl_turn.wav");
 emit_sound(&edicts[3],4,"items/padl_unlock.wav",1,0.8f,0,100);assert(sounds==2&&soundSource==&edicts[2]);
 puzzle_after_client_command();emit_sound(&edicts[3],4,"items/padl_turn.wav",1,0.8f,0,100);assert(sounds==3&&soundSource==&edicts[3]);
 store(priv(&edicts[2]),0x2350,0u);store(vars_of(&edicts[3]),0,2);puzzle_map_changed();
 command="closedocument";client_command(&edicts[2]);assert(nativeCommands==1&&journal.size()==1);
 tick(&edicts[1]);assert(nativePrethinks==1&&prefix==std::vector<int>({1,0,0,0})&&payload.size()==11&&payload[0]=="note_orig_1");
 command="computercheck";client_command(&edicts[1]);assert(nativeCommands==2&&journal.size()==2&&journal.metadata(1).codeValue==1234);
 tick(&edicts[2]);tick(&edicts[2]);assert(prefix==std::vector<int>({2,1,1234,1}));
 const auto acquired=journal.size();deactivate();assert(nativeDeactivate==1&&allocations.empty()&&journal.size()==acquired);
 memory.clear();set_map(12);activate(edicts.data(),6,2);put(&edicts[1]);put(&edicts[2]);assert(nativeActivate==1&&journal.size()==2&&puzzle_solution()==solution);
 tick(&edicts[1]);tick(&edicts[1]);assert(prefix==std::vector<int>({2,1,1234,1}));
 // A different later code type/value must not be rendered as the cafe code.
 auto another=make_note("nerds");emit(&edicts[2],another,{1,6789,3});assert(journal.size()==3&&journal.metadata(2).codeType==3);
 // Wrong allocation or stale edict serial never receives journal/code writes.
 auto guard=allocate(&edicts[4],32);std::memset(guard,0xa5,32);restore(&edicts[4]);for(int i=0;i<32;++i)assert(static_cast<unsigned char*>(guard)[i]==0xa5);
 store(&edicts[1],4,999);void* p=nullptr;std::size_t n=0;assert(!player_memory(&edicts[1],p,n));
 reset();assert(journal.size()==0&&!puzzle_has_solution());
 // Four recipients; real native long-ID archival buffers, reverse collector
 // order, unchanged original reader packets except their truncated filename.
 allocations.clear();memory.clear();store(g.data(),0x90,4);
 for(int i=1;i<=4;++i){store(vars_of(&edicts[i]),0,1);put(&edicts[i]);}
 assert(register_message("NoteDoc",-1)==8&&register_message("TextMsg",-1)==9);
 Note cafe{};std::memcpy(cafe.data()+0x40,"marriageinapril.txt",16);
 std::strcpy(reinterpret_cast<char*>(cafe.data()),"pass7");std::strcpy(reinterpret_cast<char*>(cafe.data()+0xb0),"marriageinapril");
 std::strcpy(reinterpret_cast<char*>(cafe.data()+0xf0),"gfx/vgui/notes/hiddennote1.tga");
 std::memcpy(static_cast<unsigned char*>(priv(&edicts[2]))+JournalOffset,cafe.data(),cafe.size());emit(&edicts[2],cafe,{});
 for(int i=1;i<=4;++i)tick(&edicts[i]);assert(journal.size()==1);
 for(int i=1;i<=4;++i){
  assert(!std::memcmp(static_cast<unsigned char*>(priv(&edicts[i]))+JournalOffset,cafe.data(),cafe.size()));
  assert(std::count_if(wires.begin(),wires.end(),[i](const Wire& w){return w.type==7&&w.recipient==&edicts[i]&&!w.strings.empty()&&w.strings[0]=="marriageinapril.txt";})==1);
  assert(std::count_if(wires.begin(),wires.end(),[i](const Wire& w){return w.type==9&&w.recipient==&edicts[i]&&w.prefix==std::vector<int>{1};})==1);
  for(int read=0;read<3;++read){message_begin(1,8,nullptr,&edicts[i]);for(int k=4;k<8;++k)write_string(reinterpret_cast<const char*>(cafe.data()+NoteFields[k].offset));for(int k=0;k<4;++k)write_string(reinterpret_cast<const char*>(cafe.data()+NoteFields[k].offset));write_string("");message_end();assert(payload[4]=="marriageinapril.txt"&&payload[0]=="pass7");}
 }
 const auto noteSent=messages;for(int i=1;i<=4;++i)tick(&edicts[i]);assert(messages==noteSent);
 // A native HUD resync must be followed by the canonical full-path replay.
 emit(&edicts[1],cafe,{});tick(&edicts[1]);assert(wires.back().strings[0]=="marriageinapril.txt");
 Note username{};std::memcpy(username.data()+0x40,"jillusername.txt",16);std::strcpy(reinterpret_cast<char*>(username.data()),"user2");std::strcpy(reinterpret_cast<char*>(username.data()+0xb0),"jillusername");std::strcpy(reinterpret_cast<char*>(username.data()+0xf0),"gfx/vgui/notes/hiddennote2.tga");
 std::memcpy(static_cast<unsigned char*>(priv(&edicts[1]))+JournalOffset+0x120,username.data(),username.size());emit(&edicts[1],username,{},2);
 for(int i=1;i<=4;++i){tick(&edicts[i]);tick(&edicts[i]);if(i!=1)assert(wires.back().type==9&&wires.back().strings[0]=="Archived: jillusername\n");}
 assert(journal.size()==2);const auto afterBoth=messages;for(int i=1;i<=4;++i)tick(&edicts[i]);assert(messages==afterBoth);
 // A malformed/conflicting bank stays intact and produces one bounded warning.
 auto conflict=cafe;conflict[0]='X';std::memcpy(static_cast<unsigned char*>(priv(&edicts[4]))+JournalOffset,conflict.data(),conflict.size());
 const auto bank=memory.at(&edicts[4]);const auto warn=logs.size();tick(&edicts[4]);tick(&edicts[4]);assert(memory.at(&edicts[4])==bank&&logs.size()==warn+1);
 reset();allocations.clear();memory.clear();textMessage=0;noteDoc=0;store(g.data(),0x90,2);
 // A server PreThink before sign-on must not send UI packets. A native HUD
 // replay with zero code metadata must be corrected AFTER active commands,
 // without duplicating unchanged ordinary notes or touching weapon fields.
 put(&edicts[1]);auto code=make_note("nerds");assert(journal.grant(code,{1,9799,1})==0);
 const auto unready=wires.size();prethink(&edicts[1]);assert(wires.size()==unready);
 allocations.at(&edicts[1]).commands=true;tick(&edicts[1]);assert(prefix==std::vector<int>({1,1,9799,1}));
 const auto once=wires.size();tick(&edicts[1]);assert(wires.size()==once);
 emit(&edicts[1],code,{},1);prethink(&edicts[1]);assert(prefix==std::vector<int>({1,1,9799,1}));
 tick(&edicts[1]);assert(prefix==std::vector<int>({1,1,9799,1}));
 const auto repaired=wires.size();tick(&edicts[1]);assert(wires.size()==repaired);
 reset();allocations.clear();memory.clear();
 // Opening isolation: both maps forward callbacks and arbitrary commands,
 // return native Spawn results and never track/write player memory or replay
 // AddNote. A pre-existing host bank remains intact outside the fresh lobby.
 memory.clear();allocations.clear();assert(journal.grant(make_note("prior_note"))==0);
 for(const int map:{10,14}){
  set_map(map);assert(!scoped());const auto puts=nativePuts,commands=nativeCommands,frames=nativePrethinks,spawns=nativeSpawns,thinks=nativeThinks,sent=messages;
  put(&edicts[1]);put(&edicts[2]);assert(nativePuts==puts+2&&allocations.empty());
  const auto before=memory;tick(&edicts[1]);tick(&edicts[2]);
  assert(memory==before&&nativePrethinks==frames+2&&messages==sent&&!puzzle_has_solution());
  assert(spawn(&edicts[3])==-1&&nativeSpawns==spawns+1);think(&edicts[3]);assert(nativeThinks==thinks+1);
  command="closedocument";client_command(&edicts[2]);assert(nativeCommands==commands+1&&journal.size()==1&&messages==sent+1);
  assert(journal.at(0)==make_note("prior_note"));
  assert(find(nullptr,"classname","player")==&edicts[1]);assert(random_long(1,8)==7);
 }
 set_map(10);store(vars_of(&edicts[3]),0,5);store(vars_of(&edicts[3]),0x108,4);store(vars_of(&edicts[3]),0x1d0,6);
 const auto logCount=logs.size();think(&edicts[3]);think(&edicts[3]);assert(logs.size()==logCount+1&&logs.back().find("NATIVE_THINK kunitei2")!=std::string::npos);
 store(vars_of(&edicts[3]),0,7);store(vars_of(&edicts[3]),0x1d0,8);spawn(&edicts[3]);assert(logs.back().find("VEHICLE_STATE phase=AFTER_SPAWN")!=std::string::npos);
 const auto vehicleMemory=edicts[3].data;const auto packetLogs=logs.size();
 assert(full_pack(nullptr,3,&edicts[3],&edicts[2],0,0,nullptr)==0);assert(full_pack(nullptr,3,&edicts[3],&edicts[2],0,0,nullptr)==0);
 assert(logs.size()==packetLogs+1&&edicts[3].data==vehicleMemory&&logs.back().find("packed=0")!=std::string::npos);
 // v0.1: a camera view no longer forces the parked title train past the PVS.
 assert(full_pack(nullptr,3,&edicts[3],&edicts[2],0,0,nullptr)==0&&edicts[3].data==vehicleMemory);
 assert(full_pack(nullptr,4,&edicts[4],&edicts[2],0,0,nullptr)==0);
 set_map(14);assert(full_pack(nullptr,3,&edicts[3],&edicts[2],0,0,nullptr)==0);set_map(10);
 unsigned char* pvs=nullptr;unsigned char* pas=nullptr;const auto viewLogs=logs.size();visibility(&edicts[3],&edicts[2],&pvs,&pas);visibility(&edicts[3],&edicts[2],&pvs,&pas);assert(logs.size()==viewLogs+1&&pvs==nullptr&&pas==nullptr&&edicts[3].data==vehicleMemory);
 set_map(10);store(vars_of(&edicts[0]),0,3);assert(spawn(&edicts[0])==-1&&journal.size()==0);
 set_map(12);assert(scoped());put(&edicts[1]);put(&edicts[2]);assert(puzzle_has_solution()&&allocations.size()==2);
 assert(!std::memcmp(static_cast<unsigned char*>(priv(&edicts[2]))+0x235c,puzzle_solution().data(),16));
 std::puts("PASS owned adapter fixture: native callback forwarding, P1/P2 initialization, shared journal, captured code-note packets, map lifecycle, distinct code types, allocation and serial guards, passive opening callbacks/memory/messages and wake reactivation");
}
