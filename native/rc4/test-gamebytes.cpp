// ("patch" stays out of this file and exe name: Windows treats such an exe as an installer and refuses to run it unelevated.)
// The patch lists against the real game files, read as bytes and never loaded.
// v0.7.2: one wrong address in the 5-slot save patches turned the whole co-op
// menu off in the game ("MENU co-op menu off: unexpected bytes at 101442B4") in
// v0.6.3 to v0.7.1, which the fake images of the other suites cannot show.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
using namespace rc4;
namespace {
std::vector<unsigned char> mapped(const std::filesystem::path& path){
 std::ifstream in(path,std::ios::binary);std::vector<unsigned char> file((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
 std::vector<unsigned char> image(nt->OptionalHeader.SizeOfImage);std::memcpy(image.data(),file.data(),nt->OptionalHeader.SizeOfHeaders);
 auto section=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i,++section)if(section->SizeOfRawData)std::memcpy(image.data()+section->VirtualAddress,file.data()+section->PointerToRawData,section->SizeOfRawData);
 return image;
}
void apply(const char* name,const std::vector<ClientPatch>& list,std::vector<unsigned char>& image,bool undo=false){
 auto use=list;if(undo)for(auto& p:use)std::swap(p.from,p.to);
 std::string why;const int result=patch_apply(use,image.data(),image.size(),ClientPreferredBase,why);
 if(result!=1){std::printf("FAIL %s%s: result %d %s\n",name,undo?" (undo)":"",result,why.c_str());std::exit(1);}
}
}
int main(int argc,char** argv){
 if(argc<2||!std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"cl_dlls"/"client.dll")){std::puts("SKIP patch lists: no game folder given");return 0;}
 const std::filesystem::path game=argv[1];
 auto client=mapped(game/"cl_dlls"/"client.dll");
 // In the order the game runs them: the co-op menu first, then each box and the hold on top of it.
 apply("client_patches",client_patches(),client);
 for(const auto& [name,list]:std::vector<std::pair<const char*,std::vector<ClientPatch>>>{{"join_box_patches",join_box_patches()},{"model_box_patches",model_box_patches()},{"menu_hold_patches",menu_hold_patches()}}){apply(name,list,client);apply(name,list,client,true);}
 // The picture name the model box changes per model must be there too.
 apply("model box picture",{{ModelBoxPicture,client_text(ModelBoxPictureShipped,ModelBoxPictureBytes),client_text("gfx/vgui/sc_mb0.tga",ModelBoxPictureBytes),false}},client);
 // v0.9 team HUD (qol-runtime.hpp): the list line, the list switch and every address the adapter reads.
 apply("team list line",{{TeamLineVa,team_line_shipped(ClientPreferredBase),team_line_ours(ClientPreferredBase,0x20000000),false}},client);
 for(const std::uint32_t to:{TeamAlways,TeamNever}){apply("team list switch",{{TeamListPattern,client_dword(TeamFour),client_dword(to),false}},client);apply("team list switch",{{TeamListPattern,client_dword(TeamFour),client_dword(to),false}},client,true);}
 auto bytes_at=[&](std::uint32_t va,const std::vector<unsigned char>& want){return va>=ClientPreferredBase&&va-ClientPreferredBase+want.size()<=client.size()&&std::equal(want.begin(),want.end(),client.begin()+(va-ClientPreferredBase));};
 const std::vector<std::pair<std::uint32_t,std::vector<unsigned char>>> reads{
  {TeamFour,{'4',0}},{TeamNever,client_text("%s - Health %i - Ping %d",25)},{TeamPlainFormatVa,{'%','s',0}},
  {0x1003C9DF,{0x83,0xc3,0x14}},                                                                                      // g_PlayerInfoList rows: add ebx,14h
  {0x1003C80C,{0x8b,0x85,0x98,0xfd,0xff,0xff,0xff,0x34,0x30}},                                                          // (before the line patch: health from the HUD's own array)
  {0x10079156,{0xa1,0x00,0x26,0x1b,0x10,0x85,0xc0,0x74,0x63,0x8b,0x88,0x04,0x14,0x00,0x00,0x85,0xc9,0x74,0x59,0x8b,0x01,0x8b,0x40,0x28}}};   // gViewPort->m_pRECBInventory->isVisible
 auto shipped=mapped(game/"cl_dlls"/"client.dll");
 for(const auto& [va,want]:reads){const bool ok=va>=0x1003C807&&va<0x1003C823?std::equal(want.begin(),want.end(),shipped.begin()+(va-ClientPreferredBase)):bytes_at(va,want);if(!ok){std::printf("FAIL team HUD read at %08X\n",va);return 1;}}
 auto ui=mapped(game/"cl_dlls"/"GameUI.dll");apply("gameui_patches",gameui_patches(),ui);
 auto server=mapped(game/"cl_dlls"/"hl.dll");apply("ammo_limit_patches",ammo_limit_patches(),server);
 // v0.10 (qol2-runtime.hpp): the player fields the adapter reads and writes, at the code that uses them.
 for(const auto& [va,want]:std::vector<std::pair<std::uint32_t,std::vector<unsigned char>>>{
   {0x10019F94,{0x8a,0x8a,0xe9,0x1e,0x00,0x00}},                         // dodgetoggle: m_bDodgeDisabled +0x1EE9
   {0x100E9290,{0xf3,0x0f,0x11,0x87,0xd4,0x03,0x00,0x00}},               // MultiplayerKilled: m_flCoopRespawnDelayTime +0x3D4
   {0x100E92A9,{0xf3,0x0f,0x11,0x87,0xd0,0x03,0x00,0x00}},               // m_flNextDeadNotifyTime +0x3D0
   {0x100E92CB,{0xd9,0x9f,0x2c,0x1f,0x00,0x00}},                         // m_flDeadAutoReviveTime +0x1F2C
   {0x100195CA,{0xc7,0x80,0xa0,0x1e,0x00,0x00,0x02,0x00,0x00,0x00}}})    // freeze 768: m_iUsingComputerOrPhone +0x1EA0 = 2 (v0.9)
  if(va-ClientPreferredBase+want.size()>server.size()||!std::equal(want.begin(),want.end(),server.begin()+(va-ClientPreferredBase))){std::printf("FAIL hl.dll field use at %08X\n",va);return 1;}
 // The spray pass reads engine data, wraps an SDK callback, and never calls a private game function.
 assert(sha_file((game.parent_path()/L"hw.dll").wstring())==SprayHwSha);
 auto hardware=mapped(game.parent_path()/"hw.dll");
 const std::vector<unsigned char> poolRef{0x68,0x00,0xc0,0x01,0x00,0x6a,0x00,0x68,0x90,0xbd,0x3a,0x02};
 assert(std::equal(poolRef.begin(),poolRef.end(),hardware.begin()+(0x01D6F7F2-SprayHwBase)));
 const std::vector<unsigned char> sdkRef{0xff,0x15,0x38,0x9c,0xfb,0x02};
 assert(std::equal(sdkRef.begin(),sdkRef.end(),hardware.begin()+(0x01D22D6C-SprayHwBase)));
 const std::vector<unsigned char> passGuard{0x83,0x3d,0x48,0x6b,0x55,0x10,0};
 assert(std::equal(passGuard.begin(),passGuard.end(),shipped.begin()+0x93cf0));
 // Independent retail API initializer: nested callback parameters are not table fields.
 for(const auto& getter:std::vector<std::pair<std::size_t,std::uint32_t>>{{SprayCvarSlot,0x01D21A5E},{SprayEntitySlot,0x01D23831},{SprayClockSlot,0x01D23868},{SprayLevelSlot,0x01D23BFB}})
  assert(load<std::uint32_t>(hardware.data(),0x01EA9A90-SprayHwBase+getter.first*4)==getter.second);
 assert(load<std::uint32_t>(hardware.data(),0x01EA9A90-SprayHwBase+76*4)==0x01D23C40); // recorded crash: SetScreenFade + 0x1f
 std::puts("PASS real client API getters: entity 53, time 54, level 74; fade setter 76 is never used as a getter");
 std::puts("PASS spray engine pool, public SDK callback and render-pass guard pins");
 std::puts("PASS patch lists: every patch fits the real client.dll, GameUI.dll and hl.dll");
 return 0;
}
