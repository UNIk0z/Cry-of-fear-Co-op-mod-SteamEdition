// ("patch" stays out of this file and exe name: Windows treats such an exe as an installer and refuses to run it unelevated.)
// The patch lists against the real game files, read as bytes and never loaded.
// v0.7.2: one wrong address in the 5-slot save patches turned the whole co-op
// menu off in the game ("MENU co-op menu off: unexpected bytes at 101442B4") in
// v0.6.3 to v0.7.1, which the fake images of the other suites cannot show.
#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
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
 auto ui=mapped(game/"cl_dlls"/"GameUI.dll");apply("gameui_patches",gameui_patches(),ui);
 std::puts("PASS patch lists: every patch fits the real client.dll and GameUI.dll");
 return 0;
}
