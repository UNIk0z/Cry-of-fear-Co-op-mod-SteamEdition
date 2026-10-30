// Included in adapter.cpp after session-runtime.hpp (v0.1).
// --- Co-op main menu in co-op launches only ------------------------------------------
// User request (v0.1): a normal launch of the game must show the game's own
// main menu; only the co-op launchers ("Host a game", "Join a game", both
// with -dll cl_dlls/storycoop_rc4.dll) show the co-op one. coop-fixes-013
// wrote its client.dll menu patch to the file on disk, so every launch got it.
// The file now stays the game's own; this adapter, which only a co-op launch
// loads, makes the same reviewed changes (scripts/client-menu-patch.mjs, same
// addresses and bytes) in the loaded client.dll before the menu is built
// (supplied 2026-10-15 host log: the adapter boots before VidInit loads the
// menu fonts and CGameMenu 1002C390 runs). New in v0.1: the CO-OP title is
// read from gfx/vgui/sc_logo.tga instead of replacing menu_logo.tga
// (CGameMenu's string at 1014389C, pushed only at 1002C3CF).
// Every site is checked before anything is written: each one must hold either
// the game's own bytes (changed now) or the co-op bytes already (an older
// install that patched the file); any other byte -> nothing is changed.
// client.dll is DYNAMIC_BASE: absolute addresses follow the loaded base.
struct ClientPatch {std::uint32_t va=0;std::vector<unsigned char> from,to;bool absolute=false;};
constexpr std::uint32_t ClientPreferredBase=0x10000000;
std::vector<unsigned char> client_dword(std::uint32_t v){return {static_cast<unsigned char>(v),static_cast<unsigned char>(v>>8),static_cast<unsigned char>(v>>16),static_cast<unsigned char>(v>>24)};}
std::vector<unsigned char> client_text(const char* s,std::size_t length){std::vector<unsigned char> b(length,0);for(std::size_t i=0;i<length&&s[i];++i)b[i]=static_cast<unsigned char>(s[i]);return b;}
std::vector<ClientPatch> client_patches(){
 std::vector<ClientPatch> p;
 // Main menu click ids 1..10 (jump table 1002DF68): Simon's Story list, Join
 // Server, Options, Quit; ids 5..10 return at once.
 const std::uint32_t was[10]={0x1002D97C,0x1002D9D2,0x1002DB2E,0x1002DCAD,0x1002DCC2,0x1002DCD7,0x1002DD19,0x1002DD7C,0x1002DDC1,0x1002DDD6};
 // v0.5.1: id 1 (Host Simon's Story) runs the game's own Host Server handler
 // 1002DDC1 ("optionshack 1" -> GameUI's Create Server window, GameUI 10031388).
 // v0.7 (Player Model, Katz 2026-10-27): Host, Join, Player Model, Options, Quit.
 // Id 3 runs 1002DCD7 (id 6's handler, unused since v0.1), rewritten below to
 // send "sc_modelbox" the way id 2 sends "sc_joinbox".
 const std::uint32_t now[10]={0x1002DDC1,0x1002DCAD,0x1002DCD7,0x1002DCC2,0x1002DDD6,0x1002DF61,0x1002DF61,0x1002DF61,0x1002DF61,0x1002DF61};
 for(std::uint32_t i=0;i<10;++i)p.push_back({0x1002DF68+i*4,client_dword(was[i]),client_dword(now[i]),true});
 struct Image {std::uint32_t va;const char* was;const char* is;std::size_t length;};
 for(const auto& m:{Image{0x10143648,"gfx/vgui/menu/newgame.tga","gfx/vgui/menu/sc_host.tga",26},Image{0x10143900,"gfx/vgui/menu/newgame_mo.tga","gfx/vgui/menu/sc_host_mo.tga",29},
   Image{0x10143664,"gfx/vgui/menu/loadgame.tga","gfx/vgui/menu/sc_join.tga",27},Image{0x10143920,"gfx/vgui/menu/loadgame_mo.tga","gfx/vgui/menu/sc_join_mo.tga",30},
   Image{0x10143680,"gfx/vgui/menu/customc.tga","gfx/vgui/menu/sc_mdl.tga",26},Image{0x10143940,"gfx/vgui/menu/customc_mo.tga","gfx/vgui/menu/sc_mdl_mo.tga",29},
   Image{0x1014369C,"gfx/vgui/menu/jserver.tga","gfx/vgui/menu/options.tga",26},Image{0x10143960,"gfx/vgui/menu/jserver_mo.tga","gfx/vgui/menu/options_mo.tga",29},
   Image{0x101436B8,"gfx/vgui/menu/options.tga","gfx/vgui/menu/quit.tga",26},Image{0x10143980,"gfx/vgui/menu/options_mo.tga","gfx/vgui/menu/quit_mo.tga",29},
   Image{0x101437B0,"PARSING CUSTOM CAMPAIGN: %s","sc_modelbox",27},   // its print of the campaign list now says sc_modelbox
   Image{0x1014389C,"gfx/vgui/menu_logo.tga","gfx/vgui/sc_logo.tga",23}})
  p.push_back({m.va,client_text(m.was,m.length),client_text(m.is,m.length),false});
 // Slots 6..10 (v0.7: slot 5 is Quit now): every setVisible(1) of their buttons and images -> setVisible(0).
 for(const std::uint32_t va:{0x1002DE46u,0x1002DE53u,0x1002DE60u,0x1002DE6Du,0x1002DE7Au,0x1002ED74u,0x1002ED81u,0x1002ED8Eu,0x1002ED9Bu,0x1002EDA8u,
   0x1002CB57u,0x1002CBBDu,0x1002CC23u,0x1002CC8Bu,0x1002CCEDu,0x1002DED5u,0x1002DEE2u,0x1002DEEFu,0x1002DEFCu,0x1002DF0Bu,0x1002EE03u,0x1002EE10u,0x1002EE1Du,0x1002EE2Au,0x1002EE37u})
  p.push_back({va,{0x6a,0x01},{0x6a,0x00},false});
 // The Simon's Story page lists maps/storyc*m (the co-op entries only).
 p.push_back({0x10143888,client_text("*.custom",8),client_text("storyc*m",8),false});
 // HUD 1003BF20: the ping/health/name list on every co-op map ("4" at 1014231C).
 p.push_back({0x1003C66D,client_dword(0x1014688C),client_dword(0x1014231C),true});
 // v0.5 (D9, Simon's hands for every player): in multiplayer the studio
 // renderer paints the first-person "fingers" and "sleeve" textures with the
 // police ones (decomp gl_studiomodel.cpp; LoadHandsTexture 100648D0 loads
 // police/police_hands.tga, LoadPoliceSleeveTexture 10064A40 police_sleeve.tga).
 // Neither loads now, so the view model keeps its own (Simon's) textures; a
 // custom finger or sleeve texture from the clothes unlocks still loads.
 // Hands: CreateTexture(police hands) -> xor eax,eax (no texture).
 p.push_back({0x10064942,{0xe8,0xe9,0x4c,0x00,0x00},{0x33,0xc0,0x90,0x90,0x90},false});
 // Sleeve: "push edx; push 0; cmp ecx,2; jbe police" -> "cmp ecx,2; jbe ret; push edx; push 0".
 p.push_back({0x10064A80,{0x52,0x6a,0x00,0x83,0xf9,0x02,0x76,0x07},{0x83,0xf9,0x02,0x76,0x2f,0x52,0x6a,0x00},false});
 // v0.5.1 (Join, Katz 2026-10-23: "a box with 2 fields ... cancel or join"): the
 // Join button's "optionshack 0" is handled inside the engine (hw.dll 01D21AF5)
 // and never reaches the server; "sc_joinbox" does, and the menu map's own
 // server (this adapter) answers it with the join box below.
 p.push_back({0x10143778,client_text("optionshack 0",14),client_text("sc_joinbox",14),false});
 // v0.7: id 6's handler 1002DCD7 becomes "push 101437B0 (sc_modelbox); call
 // [101B2654]; add esp,4; pop edi; pop ebx; pop esi; pop ebp; ret 4": a short
 // jump to its own push/call (1002DCF3, both absolute operands relocated as
 // before), then the epilogue in place of "add esp,8; mov ecx,[eax+1478h]".
 p.push_back({0x1002DCD7,{0x8b,0x06},{0xeb,0x1a},false});
 p.push_back({0x1002DCF4,client_dword(0x1014078C),client_dword(0x101437B0),true});
 p.push_back({0x1002DCFA,client_dword(0x101B2660),client_dword(0x101B2654),true});
 p.push_back({0x1002DD05,{0x08},{0x04},false});
 p.push_back({0x1002DD06,{0x8b,0x88,0x78,0x14,0x00,0x00,0x85},{0x5f,0x5b,0x5e,0x5d,0xc2,0x04,0x00},false});
 // v0.7: the single-player save window (SaveLoad_guess.cpp) reads and writes its slot
 // labels as SAVE/coopinfo<slot>.cof in co-op launches, never the single-player
 // SAVE/saveinfo<slot>.cof (ReadSlot 10144xxx "%ssaveinfo%i.cof", COM_LoadFile and
 // WriteSaveInfo names for slots 1-5).
 for(std::uint32_t va:{0x101442B4u,0x10144339u,0x10144378u,0x1014438Du,0x101443BCu,0x101443D1u,0x10144400u,0x10144415u,0x10144444u,0x10144459u,0x10144488u})p.push_back({va,client_text("saveinfo",8),client_text("coopinfo",8),false});
 return p;
}
// While the join box is open on the main menu, the game's computer login panel
// (decomp Computer_guess.cpp: two text fields, Log on and Cancel, opened by the
// "Computer" message, answering "computercheck <field 1> <field 2>") shows our
// own picture and texts. Put back as soon as it closes, so the computer puzzle
// in the story is untouched. Same length or shorter, padded with zeros; the two
// prompts keep their 19 characters (setText length) and the errors their 29.
std::vector<ClientPatch> join_box_patches(){
 return {{0x101473CC,client_text("gfx/vgui/puzzle/computer/computer_logon.tga",44),client_text("gfx/vgui/sc_joinbox.tga",44),false},
  {0x101474E0,client_text("Enter a username...",20),client_text("Type the host IP...",20),false},
  {0x101474F4,client_text("Enter a password...",20),client_text("Port (empty: 27015)",20),false},
  {0x10147414,client_text("Invalid username or password!",30),client_text("That is not an IP address.   ",30),false},
  {0x10147434,client_text("Invalid password or password!",30),client_text("Port must be 1 to 65535.     ",30),false}};
}
// GameUI.dll (the engine's own windows): the Create Server window lists
// maps/*-*bsp (only the story entries Install adds) and reads its options from
// sc_setup.scr instead of settings.scr. Only in co-op launches; files untouched.
std::vector<ClientPatch> gameui_patches(){
 return {{0x100E2E80,client_text("maps/*.bsp",12),client_text("maps/*-*bsp",12),false},
  {0x100E2728,client_text("settings.scr",13),client_text("sc_setup.scr",13),false},
  {0x100E27B0,client_text("settings.scr",13),client_text("sc_setup.scr",13),false}};
}
std::vector<unsigned char> client_value(const std::vector<unsigned char>& bytes,bool absolute,std::uint32_t base){
 if(!absolute||bytes.size()!=4)return bytes;
 const std::uint32_t v=static_cast<std::uint32_t>(bytes[0])|static_cast<std::uint32_t>(bytes[1])<<8|static_cast<std::uint32_t>(bytes[2])<<16|static_cast<std::uint32_t>(bytes[3])<<24;
 return client_dword(v-ClientPreferredBase+base);
}
// 1: applied now, 2: the image already had all of them, 0: refused (nothing changed).
int patch_apply(const std::vector<ClientPatch>& patches,unsigned char* image,std::size_t imageSize,std::uint32_t base,std::string& why);
int client_menu_apply(unsigned char* image,std::size_t imageSize,std::uint32_t base,std::string& why){return patch_apply(client_patches(),image,imageSize,base,why);}
int patch_apply(const std::vector<ClientPatch>& patches,unsigned char* image,std::size_t imageSize,std::uint32_t base,std::string& why){
 int asShipped=0,changed=0;
 for(const auto& p:patches){
  const std::size_t at=p.va-ClientPreferredBase;if(p.va<ClientPreferredBase||at+p.from.size()>imageSize){why="address outside the image";return 0;}
  const auto from=client_value(p.from,p.absolute,base),to=client_value(p.to,p.absolute,base);
  if(std::equal(from.begin(),from.end(),image+at))++asShipped;else if(std::equal(to.begin(),to.end(),image+at))++changed;else {char text[80]{};std::snprintf(text,sizeof(text),"unexpected bytes at %08X",p.va);why=text;return 0;}
 }
 if(!asShipped)return 2;
 for(const auto& p:patches){
  unsigned char* at=image+(p.va-ClientPreferredBase);const auto from=client_value(p.from,p.absolute,base),to=client_value(p.to,p.absolute,base);DWORD old=0;
  if(!std::equal(from.begin(),from.end(),at))continue;
  if(!VirtualProtect(at,to.size(),PAGE_EXECUTE_READWRITE,&old)){why="memory protection";return 0;}
  std::memcpy(at,to.data(),to.size());VirtualProtect(at,to.size(),old,&old);
 }
 FlushInstructionCache(GetCurrentProcess(),image,imageSize);return 1;
}
bool clientMenuTried=false;unsigned char* clientImage=nullptr;std::size_t clientImageSize=0;
// on: the join box texts and picture; off: the game's own again. Each site must
// hold one of the two values; anything else changes nothing.
bool join_box_text(bool on){
 if(!clientImage)return false;std::vector<ClientPatch> list=join_box_patches();
 if(!on)for(auto& p:list)std::swap(p.from,p.to);
 std::string why;const int result=patch_apply(list,clientImage,clientImageSize,reinterpret_cast<std::uint32_t>(clientImage),why);
 if(!result){const std::string line="[STORYCOOP_RC4] JOIN box texts not changed: "+why+"\n";log(line.c_str());}
 return result!=0;
}
// v0.5.2 (Katz 2026-10-23: clicking in the Create Server window "opens a
// settings menu for no reason"). The menu's buttons are client VGUI panels
// under the GameUI window and still take its clicks: in a dev run a click on
// the window's map list at the Join button's height logged "JOIN box open",
// and one at the Options button's height opened the game's Options window.
// While GameUI is active (its window and its own menu are up), the four
// button ids return at once (1002DF61, as ids 5..10 do); they work again as
// soon as GameUI closes. Checked every 50 ms by a timer of the game's own
// main thread (its message loop runs the timer): the menu map's server
// frames stop while GameUI is up, so a server frame never sees it. Inside the
// timer only the adapter log file is written, never the engine console.
std::vector<ClientPatch> menu_hold_patches(){
 std::vector<ClientPatch> p;const std::uint32_t now[5]={0x1002DDC1,0x1002DCAD,0x1002DCD7,0x1002DCC2,0x1002DDD6};
 for(std::uint32_t i=0;i<5;++i)p.push_back({0x1002DF68+i*4,client_dword(now[i]),client_dword(0x1002DF61),true});
 return p;
}
bool menuHeld=false;UINT_PTR menuTimer=0;
// user32 is already loaded by the game; the adapter links only kernel32/bcrypt.
FARPROC user32_proc(const char* name){HMODULE u=GetModuleHandleW(L"user32.dll");return u?GetProcAddress(u,name):nullptr;}
void menu_hold_stop(){auto kill=reinterpret_cast<BOOL(WINAPI*)(HWND,UINT_PTR)>(user32_proc("KillTimer"));if(menuTimer&&kill)kill(nullptr,menuTimer);menuTimer=0;}
void menu_hold(bool on){
 if(!clientImage||on==menuHeld)return;auto list=menu_hold_patches();if(!on)for(auto& p:list)std::swap(p.from,p.to);
 std::string why;if(!patch_apply(list,clientImage,clientImageSize,reinterpret_cast<std::uint32_t>(clientImage),why)){const std::string line="[STORYCOOP_RC4] MENU buttons hold off: "+why+"\n";session_log(line.c_str());menu_hold_stop();return;}
 menuHeld=on;session_log(on?"[STORYCOOP_RC4] MENU buttons paused while a game window is open\n":"[STORYCOOP_RC4] MENU buttons active again\n");
}
// v0.5.5: R in this game's window sends "sc_skip" to the server this game
// plays on (the host's own server or a friend's); see skip_client_command.
// Only while this game's window has the focus and its console is closed
// (GameUI not active), so typing in the console or the chat box never votes.
bool skipKeyDown=false;
void skip_key_poll(bool gameuiActive){
 auto keyState=reinterpret_cast<SHORT(WINAPI*)(int)>(user32_proc("GetAsyncKeyState"));
 auto foreground=reinterpret_cast<HWND(WINAPI*)()>(user32_proc("GetForegroundWindow"));
 auto owner=reinterpret_cast<DWORD(WINAPI*)(HWND,LPDWORD)>(user32_proc("GetWindowThreadProcessId"));
 if(!keyState||!foreground||!owner||!engine[39])return;
 DWORD pid=0;const HWND w=foreground();if(w)owner(w,&pid);
 const bool down=pid==GetCurrentProcessId()&&!gameuiActive&&(keyState('R')&0x8000)!=0;
 if(down&&!skipKeyDown)eng<void(__cdecl*)(const char*)>(39)("cmd sc_skip\n");
 skipKeyDown=down;
}
void CALLBACK menu_hold_timer(HWND,UINT,UINT_PTR,DWORD){
 pause_probe();if(!pauseProbe.ready)return;
 skip_key_poll(reinterpret_cast<bool(__thiscall*)(void*)>((*reinterpret_cast<void***>(pauseProbe.ui))[11])(pauseProbe.ui));
 menu_hold(reinterpret_cast<bool(__thiscall*)(void*)>((*reinterpret_cast<void***>(pauseProbe.ui))[11])(pauseProbe.ui));
}
void menu_hold_start(){auto set=reinterpret_cast<UINT_PTR(WINAPI*)(HWND,UINT_PTR,UINT,TIMERPROC)>(user32_proc("SetTimer"));if(clientImage&&!menuTimer&&set)menuTimer=set(nullptr,0,50,&menu_hold_timer);}
bool gameuiTried=false;
void gameui_patch(){
 if(gameuiTried)return;HMODULE ui=GetModuleHandleW(L"GameUI.dll");if(!ui)return;gameuiTried=true;
 wchar_t path[MAX_PATH]{};std::error_code error;const bool named=GetModuleFileNameW(ui,path,MAX_PATH)!=0;const auto bytes=named?std::filesystem::file_size(path,error):0;
 auto base=reinterpret_cast<unsigned char*>(ui);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
 if(error||bytes!=1056863){log("[STORYCOOP_RC4] HOST Create Server window not changed: GameUI.dll is not the expected game version\n");return;}
 std::string why;const int result=patch_apply(gameui_patches(),base,nt->OptionalHeader.SizeOfImage,reinterpret_cast<std::uint32_t>(base),why);
 const std::string line=result?"[STORYCOOP_RC4] HOST Create Server window lists the story chapters and reads sc_setup.scr\n":"[STORYCOOP_RC4] HOST Create Server window not changed: "+why+"\n";log(line.c_str());
}
// --- The join box (server side, on the main menu map) ------------------------------
struct JoinBox {Edict* who=nullptr;int serial=-1;};
JoinBox joinBox;
void join_box_show(Edict* e,bool wrong){
 if(computerMessage<=0||!live(e))return;
 eng<void(__cdecl*)(int,int,const float*,Edict*)>(46)(1,computerMessage,nullptr,e);
 for(int v:{0,wrong?1:0,wrong?1:0,1})eng<void(__cdecl*)(int)>(48)(v);   // logged on, wrong user, wrong password, silent
 eng<void(__cdecl*)(const char*)>(54)("");eng<void(__cdecl*)()>(47)();
}
void join_box_close(const char* line){if(!joinBox.who)return;join_box_text(false);joinBox={};if(line)log(line);}
bool join_address(const std::string& s){return !s.empty()&&s.size()<=253&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-';});}
int join_port(const std::string& s){if(s.empty()||s.rfind("Port",0)==0||s.rfind("(",0)==0)return 27015;if(s.size()>5||!std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}))return 0;const int p=std::atoi(s.c_str());return p>=1&&p<=65535?p:0;}
struct PendingJoin {std::string target;unsigned long long due=0;} pendingJoin;
bool join_box_command(Edict* e,const char* command){
 if(!command||!globals||std::strcmp(map_name(),"c_game_menu1"))return false;
 auto arg=[](int i){const char* a=eng<const char*(__cdecl*)(int)>(83)(i);return std::string(a?a:"");};
 if(!std::strcmp(command,"sc_joinbox")){
  if(computerMessage<=0||!join_box_text(true)){log("[STORYCOOP_RC4] JOIN box unavailable; use the console: connect <IP>:27015\n");if(engine[80]&&live(e))eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,"Open the console (~) and type: connect <host IP>:27015");return true;}
  joinBox={e,serial(e)};join_box_show(e,false);log("[STORYCOOP_RC4] JOIN box open\n");return true;
 }
 if(!joinBox.who||e!=joinBox.who||serial(e)!=joinBox.serial)return false;
 if(!std::strcmp(command,"stopcomputer")){join_box_close("[STORYCOOP_RC4] JOIN box closed (Cancel)\n");return true;}
 if(std::strcmp(command,"computercheck"))return false;
 // v0.6.1 (playtest: the join box "did not parse the console command correctly"):
 // the panel sends "computercheck <IP field> <port field>" (decomp
 // Computer_guess.cpp), and the engine's tokenizer cuts ':' '(' ')' into tokens
 // of their own, so "1.2.3.4:27015" or the untouched placeholder "Port (empty:
 // 27015)" became several arguments and the port was read from the wrong one.
 // The raw text after the command (pfnCmd_Args, slot 82) is parsed instead:
 // first word = IP (with an optional :port), second word = port, placeholders
 // and an empty port mean 27015.
 std::string raw;if(engine[82]){const char* a=eng<const char*(__cdecl*)()>(82)();raw=a?a:"";}else raw=arg(1)+" "+arg(2);
 auto word=[&](std::size_t& at){while(at<raw.size()&&(raw[at]==' '||raw[at]=='\t'||raw[at]=='"'))++at;const std::size_t from=at;while(at<raw.size()&&raw[at]!=' '&&raw[at]!='\t'&&raw[at]!='"')++at;return raw.substr(from,at-from);};
 std::size_t at=0;std::string ip=word(at),port=word(at);const auto colon=ip.find(':');if(colon!=std::string::npos){port=ip.substr(colon+1);ip=ip.substr(0,colon);}
 const int p=join_port(port);
 if(ip=="Type"||!join_address(ip)||!p){join_box_show(e,true);char bad[200]{};std::snprintf(bad,sizeof(bad),"[STORYCOOP_RC4] JOIN refused: not an address and port (box sent \"%.120s\")\n",raw.c_str());log(bad);return true;}
 const std::string target=ip+":"+std::to_string(p);char line[300]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] JOIN connecting to %s\n",target.c_str());
 join_box_close(line);
 // v0.6.2 (tester log: Server tried to send invalid command:"connect 11.11.11.11:2701"):
 // the game refuses a "connect" sent by a server to its client. The join box runs
 // on the player's own menu server, in the same game, so the command goes into
 // this game's own console (ServerCommand) instead.
 // v0.7 (Playtest 3, partner crash-20261028-204940.dmp: hw.dll+0xC9260 in
 // S_StartDynamicSound <- S_FindName <- EV_PlaySound, read 0xE78, right after
 // "JOIN connecting" and the menu server's deactivate): the connect ran in the
 // next frame, while the closing box still had a sound to play from the menu
 // server that the connect was tearing down. The connect now waits 0.6 s.
 pendingJoin={target,GetTickCount64()+600};
 return true;
}
void join_connect_frame(){
 if(pendingJoin.target.empty()||GetTickCount64()<pendingJoin.due)return;const std::string target=pendingJoin.target;pendingJoin={};
 if(engine[39])eng<void(__cdecl*)(const char*)>(39)(("connect "+target+"\n").c_str());
}
// --- The player model box (v0.7, docs/V07_TODO.md 3) ----------------------------------------
// Katz 2026-10-27: a "Player Model" button on the co-op main menu, beside Host
// and Join, with a box like the Join box. The button (client_patches: slot 3,
// "sc_modelbox") opens the game's computer login panel (as the Join box) with
// its own texts; the models this PC has are listed above it (centre print):
// "simon" and every other folder models/player/<name>/<name>.mdl except the
// game's police1-4 (Run Co-op Mod.cmd copies the mod's playermodels folder
// there). The name typed into the first field is kept in
// cryoffear/storycoop_model.txt (the launcher passes it as "setinfo sc_model"
// at every start) and set at once with setinfo in this game's own console.
// The host's server shows each player with the model his sc_model names when
// the host has that model too (session-runtime.hpp model_frame), else as Simon.
std::vector<ClientPatch> model_box_patches(){
 return {{0x101474E0,client_text("Enter a username...",20),client_text("Type a model name..",20),false},
  {0x101474F4,client_text("Enter a password...",20),client_text("(leave this empty) ",20),false},
  {0x10147414,client_text("Invalid username or password!",30),client_text("That model is not on this PC.",30),false},
  {0x10147434,client_text("Invalid password or password!",30),client_text("That model is not on this PC.",30),false}};
}
bool model_box_text(bool on){
 if(!clientImage)return false;std::vector<ClientPatch> list=model_box_patches();
 if(!on)for(auto& p:list)std::swap(p.from,p.to);
 std::string why;const int result=patch_apply(list,clientImage,clientImageSize,reinterpret_cast<std::uint32_t>(clientImage),why);
 if(!result){const std::string line="[STORYCOOP_RC4] MODEL box texts not changed: "+why+"\n";log(line.c_str());}
 return result!=0;
}
bool model_name_ok(const std::string& s){return !s.empty()&&s.size()<=32&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-';});}
std::filesystem::path game_dir_path(){char dir[MAX_PATH]{};if(!engine[99])return {};eng<void(__cdecl*)(char*)>(99)(dir);if(!dir[0]||!std::memchr(dir,0,MAX_PATH))return {};return std::filesystem::path(dir);}
std::vector<std::string> local_player_models(){
 std::vector<std::string> out{"simon"};const auto dir=game_dir_path();if(dir.empty())return out;std::error_code error;
 for(const auto& d:std::filesystem::directory_iterator(dir/L"models"/L"player",error)){
  std::string n=d.path().filename().string();for(auto& c:n)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if(!model_name_ok(n)||n=="simon"||(n.size()==7&&n.rfind("police",0)==0))continue;std::error_code e2;
  if(std::filesystem::is_regular_file(d.path()/(d.path().filename().wstring()+L".mdl"),e2))out.push_back(n);
  if(out.size()>=40)break;}
 return out;
}
struct ModelBox {Edict* who=nullptr;int serial=-1;} modelBox;
void model_box_close(const char* line){if(!modelBox.who)return;model_box_text(false);modelBox={};if(line)log(line);}
void model_box_list(Edict* e,const char* head){
 if(!engine[80]||!live(e))return;std::string text=std::string(head)+"Player models on this PC:\n";int n=0;
 for(const auto& m:local_player_models()){text+=m;text+=++n%6?"   ":"\n";}
 eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,text.c_str());
}
bool model_box_command(Edict* e,const char* command){
 if(!command||!globals||std::strcmp(map_name(),"c_game_menu1"))return false;
 if(!std::strcmp(command,"sc_modelbox")){
  if(joinBox.who)join_box_close(nullptr);
  if(computerMessage<=0||!model_box_text(true)){log("[STORYCOOP_RC4] MODEL box unavailable\n");return true;}
  modelBox={e,serial(e)};join_box_show(e,false);model_box_list(e,"");log("[STORYCOOP_RC4] MODEL box open\n");return true;
 }
 if(!modelBox.who||e!=modelBox.who||serial(e)!=modelBox.serial)return false;
 if(!std::strcmp(command,"stopcomputer")){if(engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,"");model_box_close("[STORYCOOP_RC4] MODEL box closed (Cancel)\n");return true;}
 if(std::strcmp(command,"computercheck"))return false;
 std::string raw;if(engine[82]){const char* a=eng<const char*(__cdecl*)()>(82)();raw=a?a:"";}
 std::size_t at=0;while(at<raw.size()&&(raw[at]==' '||raw[at]=='"'))++at;std::string name;while(at<raw.size()&&raw[at]!=' '&&raw[at]!='"')name+=static_cast<char>(std::tolower(static_cast<unsigned char>(raw[at++])));
 const auto models=local_player_models();
 if(name=="type"||std::find(models.begin(),models.end(),name)==models.end()){join_box_show(e,true);model_box_list(e,"That model is not on this PC.\n");char bad[160]{};std::snprintf(bad,sizeof(bad),"[STORYCOOP_RC4] MODEL refused \"%.60s\"\n",name.c_str());log(bad);return true;}
 const auto dir=game_dir_path();bool saved=false;
 if(!dir.empty()){std::ofstream f(dir/L"storycoop_model.txt",std::ios::binary|std::ios::trunc);if(f){f<<name;saved=static_cast<bool>(f);}}
 if(engine[39])eng<void(__cdecl*)(const char*)>(39)(("setinfo sc_model \""+name+"\"\n").c_str());
 if(engine[80])eng<void(__cdecl*)(Edict*,int,const char*)>(80)(e,1,("Player model: "+name).c_str());
 char line[160]{};std::snprintf(line,sizeof(line),"[STORYCOOP_RC4] MODEL chosen: %s%s\n",name.c_str(),saved?"":" (could not be saved for the next start)");model_box_close(line);
 return true;
}
// --- Up to ten syringes in co-op (v0.6.1) ---------------------------------------------
// Katz 2026-10-25: "players should be able to hold more than one morphine/syringe
// ... in singleplayer simon can hold max 10". CSyringe::GetItemInfo gives 10
// (decomp CoF_Syringe.cpp), but CBasePlayerItem::iMaxAmmo1 (pinned hl.dll
// 101202F0, decomp weapons.cpp) returns 1 for weapon_syringe whenever the game
// rules are multiplayer: "mov eax,1" at 10120359. Its immediate byte becomes 10
// (one byte of the verified backend, the same checked patch the client and
// GameUI changes use; nothing else in that function changes).
bool syringeLimitTried=false;
void syringe_limit_patch(){
 if(syringeLimitTried||!backend||!verified)return;syringeLimitTried=true;
 auto base=reinterpret_cast<unsigned char*>(backend);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
 std::string why;const int r=patch_apply({{0x10120359,{0xB8,0x01,0x00,0x00,0x00},{0xB8,0x0A,0x00,0x00,0x00},false}},base,nt->OptionalHeader.SizeOfImage,reinterpret_cast<std::uint32_t>(backend),why);
 log(r==1?"[STORYCOOP_RC4] SYRINGE co-op limit raised to 10 (as in single player)\n":r==2?"[STORYCOOP_RC4] SYRINGE limit already 10\n":("[STORYCOOP_RC4] SYRINGE limit not changed: "+why+"\n").c_str());
}
void client_menu_patch(){
 if(clientMenuTried)return;clientMenuTried=true;
 HMODULE own=nullptr;wchar_t path[MAX_PATH]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&client_menu_patch),&own)||!GetModuleFileNameW(own,path,MAX_PATH))return;
 std::wstring file(path);const auto slash=file.find_last_of(L"\\/");if(slash==std::wstring::npos)return;file=file.substr(0,slash+1)+L"client.dll";
 // The engine loads this same cl_dlls\client.dll; loading it here first only
 // makes sure the menu is changed before the engine builds it.
 HMODULE client=GetModuleHandleW(file.c_str());if(!client)client=LoadLibraryExW(file.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!client){log("[STORYCOOP_RC4] MENU co-op menu off: cl_dlls/client.dll could not be opened\n");return;}
 auto base=reinterpret_cast<unsigned char*>(client);auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
 std::error_code error;const auto bytes=std::filesystem::file_size(file,error);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||nt->Signature!=IMAGE_NT_SIGNATURE||error||bytes!=1754112){log("[STORYCOOP_RC4] MENU co-op menu off: client.dll is not the expected game version\n");return;}
 std::string why;const int result=client_menu_apply(base,nt->OptionalHeader.SizeOfImage,reinterpret_cast<std::uint32_t>(base),why);
 if(result){clientImage=base;clientImageSize=nt->OptionalHeader.SizeOfImage;}
 if(result==1)log("[STORYCOOP_RC4] MENU co-op main menu set up in memory (the client.dll file is not changed)\n");
 else if(result==2)log("[STORYCOOP_RC4] MENU co-op main menu already in place\n");
 else {const std::string line="[STORYCOOP_RC4] MENU co-op menu off: "+why+"; the game's own menu stays\n";log(line.c_str());}
}
