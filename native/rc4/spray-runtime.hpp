// Included after qol-runtime.hpp. The SDK draw callback renders engine-transferred player decals
// after CoF's world pass, which suppresses the engine's normal world/decal drawing.
constexpr std::uint32_t SprayHwBase=0x01D00000, SprayPoolVa=0x023ABD90, SprayPlayersVa=0x02F8B770, SprayDrawSlotVa=0x02FB9C38;
constexpr char SprayHwSha[]="9dd34e536c4bb7cda3bc1bc4f0f5f6163687566a16f35c0ea0be59f93da62875";
struct SprayDecal {void* next;void* surface;float x,y,scale;short texture,flags,entity;};
static_assert(sizeof(SprayDecal)==28,"engine decal ABI");
struct SprayVertex {float x,y,z,u,v;};
struct SprayPolygon {int texture=0,entity=0;std::vector<SprayVertex> vertices;};
using SprayDraw=void(__cdecl*)();
unsigned char* sprayHw=nullptr;SprayDraw sprayOriginal=nullptr;bool sprayRejected=false,sprayHwVerified=false,spraySoundReady=false;
int sprayWireByte=-1,sprayWirePlayer=0;std::map<int,float> spraySoundTimes;
bool spray_readable(const void* p,std::size_t n){
 if(!p||!n)return false;const auto begin=reinterpret_cast<std::uintptr_t>(p);if(begin+n<begin)return false;
 for(auto at=begin;at<begin+n;){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
  const auto end=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize;if(end<=at)return false;at=end;}
 return true;
}
void* spray_hw(std::uint32_t va){return sprayHw+(va-SprayHwBase);}
const void* spray_texture(int player){
 if(!sprayHw||player<0||player>=32)return nullptr;
 auto cust=load<const void*>(spray_hw(SprayPlayersVa),static_cast<std::size_t>(player)*0x250+0x234);
 for(int i=0;i<8&&spray_readable(cust,0xa4);++i,cust=load<const void*>(cust,0xa0)){
  if(!load<int>(cust,0)||load<int>(cust,0x44)!=3||!(load<unsigned char>(cust,0x50)&4)||!load<int>(cust,0x8c))continue;
  const auto wad=load<const void*>(cust,0x98);if(!spray_readable(wad,0x30)||load<int>(wad,8)!=1)continue;
  const auto cache=load<const void*>(wad,4);if(!spray_readable(cache,0x44))continue;const auto tex=load<const void*>(cache,0x40);
  if(spray_readable(tex,28)&&load<unsigned>(tex,16)>0&&load<unsigned>(tex,16)<=256&&load<unsigned>(tex,20)>0&&load<unsigned>(tex,20)<=256&&load<int>(tex,24)>0)return tex;
 }
 return nullptr;
}
std::vector<SprayVertex> spray_clip(std::vector<SprayVertex> poly){
 for(int edge=0;edge<4&&!poly.empty();++edge){std::vector<SprayVertex> out;const bool vertical=edge<2;const float bound=(edge&1)?1.f:0.f;
  const auto coord=[&](const SprayVertex& a){return vertical?a.u:a.v;};const auto inside=[&](const SprayVertex& a){return (edge&1)?coord(a)<=bound:coord(a)>=bound;};
  auto prev=poly.back();bool was=inside(prev);for(const auto& cur:poly){const bool is=inside(cur);if(was!=is){const float t=(bound-coord(prev))/(coord(cur)-coord(prev));out.push_back({prev.x+(cur.x-prev.x)*t,prev.y+(cur.y-prev.y)*t,prev.z+(cur.z-prev.z)*t,prev.u+(cur.u-prev.u)*t,prev.v+(cur.v-prev.v)*t});}if(is)out.push_back(cur);prev=cur;was=is;}
  poly=std::move(out);if(poly.size()>64)return {};
 }
 return poly;
}
bool spray_polygon(const SprayDecal& d,SprayPolygon& out){
 out={};if(!(d.flags&4)||d.texture>=0||d.texture < -32||d.entity<0||!std::isfinite(d.scale)||d.scale<=0||!spray_readable(d.surface,48))return false;
 const auto texture=spray_texture(-d.texture-1);if(!texture)return false;
 const auto info=load<const void*>(d.surface,44),poly=load<const void*>(d.surface,36);if(!spray_readable(info,44)||!spray_readable(poly,16))return false;
 const int count=load<int>(poly,8);if(count<3||count>48||!spray_readable(poly,16+static_cast<std::size_t>(count)*28))return false;
 const auto wall=load<const void*>(info,36);if(!spray_readable(wall,24))return false;
 const float sx=static_cast<float>(load<unsigned>(wall,16))*d.scale/static_cast<float>(load<unsigned>(texture,16));
 const float sy=static_cast<float>(load<unsigned>(wall,20))*d.scale/static_cast<float>(load<unsigned>(texture,20));
 if(!std::isfinite(sx)||!std::isfinite(sy)||sx<=0||sy<=0||!std::isfinite(d.x)||!std::isfinite(d.y))return false;
 for(int i=0;i<count;++i){const auto at=16+static_cast<std::size_t>(i)*28;SprayVertex v{load<float>(poly,at),load<float>(poly,at+4),load<float>(poly,at+8),(load<float>(poly,at+12)-d.x)*sx,(load<float>(poly,at+16)-d.y)*sy};
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.u)||!std::isfinite(v.v))return false;if(d.flags&8)v.u=1.f-v.u;if(d.flags&16)v.v=1.f-v.v;out.vertices.push_back(v);}
 out.vertices=spray_clip(std::move(out.vertices));out.texture=load<int>(texture,24);out.entity=d.entity;return out.vertices.size()>=3;
}
struct SprayGL {
 void(APIENTRY*push)(unsigned)=nullptr;void(APIENTRY*pop)()=nullptr;void(APIENTRY*enable)(unsigned)=nullptr;void(APIENTRY*disable)(unsigned)=nullptr;
 void(APIENTRY*blend)(unsigned,unsigned)=nullptr;void(APIENTRY*depthMask)(unsigned char)=nullptr;void(APIENTRY*depthFunc)(unsigned)=nullptr;
 void(APIENTRY*alpha)(unsigned,float)=nullptr;void(APIENTRY*offset)(float,float)=nullptr;void(APIENTRY*color)(float,float,float,float)=nullptr;
 void(APIENTRY*bind)(unsigned,unsigned)=nullptr;void(APIENTRY*begin)(unsigned)=nullptr;void(APIENTRY*end)()=nullptr;
 void(APIENTRY*uv)(float,float)=nullptr;void(APIENTRY*vertex)(float,float,float)=nullptr;void(APIENTRY*active)(unsigned)=nullptr;
 void(APIENTRY*matrixMode)(unsigned)=nullptr;void(APIENTRY*pushMatrix)()=nullptr;void(APIENTRY*popMatrix)()=nullptr;void(APIENTRY*translate)(float,float,float)=nullptr;void(APIENTRY*rotate)(float,float,float,float)=nullptr;
 bool ready=false;
} sprayGL;
bool spray_gl_init(){
 if(sprayGL.ready)return true;const auto gl=GetModuleHandleW(L"opengl32.dll");if(!gl)return false;
 #define SPRAY_GL(member,name) sprayGL.member=reinterpret_cast<decltype(sprayGL.member)>(GetProcAddress(gl,name));if(!sprayGL.member)return false
 SPRAY_GL(push,"glPushAttrib");SPRAY_GL(pop,"glPopAttrib");SPRAY_GL(enable,"glEnable");SPRAY_GL(disable,"glDisable");SPRAY_GL(blend,"glBlendFunc");
 SPRAY_GL(depthMask,"glDepthMask");SPRAY_GL(depthFunc,"glDepthFunc");SPRAY_GL(alpha,"glAlphaFunc");SPRAY_GL(offset,"glPolygonOffset");SPRAY_GL(color,"glColor4f");
 SPRAY_GL(bind,"glBindTexture");SPRAY_GL(begin,"glBegin");SPRAY_GL(end,"glEnd");SPRAY_GL(uv,"glTexCoord2f");SPRAY_GL(vertex,"glVertex3f");
 SPRAY_GL(matrixMode,"glMatrixMode");SPRAY_GL(pushMatrix,"glPushMatrix");SPRAY_GL(popMatrix,"glPopMatrix");SPRAY_GL(translate,"glTranslatef");SPRAY_GL(rotate,"glRotatef");
 const auto extension=reinterpret_cast<PROC(WINAPI*)(LPCSTR)>(GetProcAddress(gl,"wglGetProcAddress"));if(!extension)return false;
 sprayGL.active=reinterpret_cast<decltype(sprayGL.active)>(extension("glActiveTextureARB"));if(reinterpret_cast<std::uintptr_t>(sprayGL.active)<=3||reinterpret_cast<std::uintptr_t>(sprayGL.active)==~std::uintptr_t(0))return false;
 #undef SPRAY_GL
 sprayGL.ready=true;return true;
}
// Keep the newest native spray burst per player; one burst may cover several BSP faces.
std::array<SprayDecal,4096> spraySeen{};
std::array<bool,4096> sprayRetry{};
std::array<std::vector<SprayPolygon>,32> sprayLatest;
std::string sprayLevel;float sprayClock=-1.f;
bool spray_same(const SprayDecal& a,const SprayDecal& b){
 return a.surface==b.surface&&a.texture==b.texture&&a.entity==b.entity&&((a.flags^b.flags)&28)==0&&std::memcmp(&a.x,&b.x,3*sizeof(float))==0;
}
void spray_cache_reset(){spraySeen={};sprayRetry={};for(auto& latest:sprayLatest)latest.clear();}
void spray_refresh(){
 const auto level=load<const char*(__cdecl*)()>(clientImage,0x1b2608+76*4);
 const auto clock=load<float(__cdecl*)()>(clientImage,0x1b2608+55*4);
 const char* name=level?level():nullptr;const float now=clock?clock():-1.f;
 // Network corrections can rewind cl.time; a fresh server starts at 1 s (sv_main.c).
 if((name&&sprayLevel!=name)||(now>=0.f&&now<2.f&&sprayClock>now+1.f))spray_cache_reset();
 if(name)sprayLevel=name;sprayClock=now;
 std::array<std::vector<SprayPolygon>,32> changed;bool any=false;
 const auto pool=static_cast<const SprayDecal*>(spray_hw(SprayPoolVa));
 for(std::size_t i=0;i<spraySeen.size();++i){const auto& d=pool[i];any=any||d.surface!=nullptr;
  if(!sprayRetry[i]&&spray_same(d,spraySeen[i]))continue;spraySeen[i]=d;sprayRetry[i]=false;
  if(!d.surface||!(d.flags&4)||d.texture>=0||d.texture < -32)continue;
  SprayPolygon polygon;if(!spray_polygon(d,polygon)){sprayRetry[i]=true;continue;}
  changed[static_cast<std::size_t>(-d.texture-1)].push_back(std::move(polygon));
 }
 if(!any){spray_cache_reset();return;}
 for(std::size_t player=0;player<sprayLatest.size();++player){
  if(!changed[player].empty())sprayLatest[player]=std::move(changed[player]);
  if(sprayLatest[player].empty())continue;const auto texture=spray_texture(static_cast<int>(player));
  if(!texture){sprayLatest[player].clear();continue;}
  for(auto& polygon:sprayLatest[player])polygon.texture=load<int>(texture,24);
 }
}
void spray_render(){
 if(!sprayHw||!clientImage||clientImageSize<0x556b4c)return;spray_refresh();
 if(load<int>(clientImage,0x556b48)||!spray_gl_init())return;
 const auto clientCvar=load<float(__cdecl*)(const char*)>(clientImage,0x1b2608+15*4);if(!clientCvar||clientCvar("gl_renderer")==0.f||clientCvar("gl_customdecals")==0.f||clientCvar("r_decals")==0.f)return;
 const auto byIndex=load<void*(__cdecl*)(int)>(clientImage,0x1b2608+54*4);if(!byIndex||!byIndex(0))return;
 bool begun=false;
 for(const auto& player:sprayLatest)for(const auto& p:player){
  const auto entity=p.entity?byIndex(p.entity):nullptr;if(p.entity&&!spray_readable(entity,0xb60))continue;
  if(!begun){begun=true;sprayGL.push(0x000fffff);for(unsigned unit=1;unit<4;++unit){sprayGL.active(0x84c0+unit);sprayGL.disable(0x0de1);}sprayGL.active(0x84c0);
   sprayGL.enable(0x0de1);sprayGL.disable(0x0b50);sprayGL.disable(0x0b44);sprayGL.enable(0x0be2);sprayGL.blend(0x0302,0x0303);sprayGL.enable(0x0bc0);sprayGL.alpha(0x0204,0.f);
   sprayGL.enable(0x0b71);sprayGL.depthFunc(0x0203);sprayGL.depthMask(0);sprayGL.enable(0x8037);sprayGL.offset(-1.f,-1.f);sprayGL.color(1,1,1,1);sprayGL.matrixMode(0x1700);}
  sprayGL.pushMatrix();if(entity){sprayGL.translate(load<float>(entity,0xb48),load<float>(entity,0xb4c),load<float>(entity,0xb50));sprayGL.rotate(load<float>(entity,0xb58),0,0,1);sprayGL.rotate(-load<float>(entity,0xb54),0,1,0);sprayGL.rotate(load<float>(entity,0xb5c),1,0,0);}
  sprayGL.bind(0x0de1,static_cast<unsigned>(p.texture));sprayGL.begin(0x0006);for(const auto& v:p.vertices){sprayGL.uv(v.u,v.v);sprayGL.vertex(v.x,v.y,v.z);}sprayGL.end();sprayGL.popMatrix();
 }
 if(begun)sprayGL.pop();
}
void __cdecl spray_draw_callback(){if(sprayOriginal)sprayOriginal();spray_render();}
bool spray_attach(SprayDraw& slot,SprayDraw expected){if(slot!=expected)return false;sprayOriginal=slot;slot=spray_draw_callback;return true;}
void spray_restore(SprayDraw& slot){if(slot==spray_draw_callback)slot=sprayOriginal;}
void spray_client_install(){
 if(sprayRejected)return;if(sprayHw){auto& slot=*static_cast<SprayDraw*>(spray_hw(SprayDrawSlotVa));if(slot==spray_draw_callback)return;sprayHw=nullptr;sprayOriginal=nullptr;}
 const auto module=GetModuleHandleW(L"hw.dll"),client=GetModuleHandleW(L"client.dll");if(!module||!client||!clientImage)return;
 const auto expected=reinterpret_cast<SprayDraw>(GetProcAddress(client,"HUD_DrawNormalTriangles"));if(!expected)return;
 wchar_t file[MAX_PATH]{};if(!sprayHwVerified&&(!GetModuleFileNameW(module,file,MAX_PATH)||sha_file(file)!=SprayHwSha)){sprayRejected=true;log("[STORYCOOP_RC4] SPRAY renderer refused: unsupported hw.dll\n");return;}
 sprayHwVerified=true;sprayHw=reinterpret_cast<unsigned char*>(module);auto& slot=*static_cast<SprayDraw*>(spray_hw(SprayDrawSlotVa));if(slot!=expected){sprayHw=nullptr;return;}
 if(!spray_attach(slot,expected)){sprayHw=nullptr;return;}log("[STORYCOOP_RC4] SPRAY SDK draw callback installed; received player logos rendered after CoF world\n");
}
void spray_client_uninstall(){
 if(sprayHw){HMODULE module=nullptr;if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(sprayHw),&module)&&reinterpret_cast<unsigned char*>(module)==sprayHw){auto& slot=*static_cast<SprayDraw*>(spray_hw(SprayDrawSlotVa));spray_restore(slot);}}
 sprayHw=nullptr;sprayOriginal=nullptr;spray_cache_reset();sprayLevel.clear();sprayClock=-1.f;
}
void spray_precache(){
 spraySoundReady=false;spraySoundTimes.clear();std::error_code error;const auto dir=game_dir_path();
 if(!engine[1]||dir.empty()||!std::filesystem::is_regular_file(dir/L"sound"/L"storycoop"/L"spray.wav",error)){log("[STORYCOOP_RC4] SPRAY sound missing; install the complete spray fix package\n");return;}
 spraySoundReady=eng<int(__cdecl*)(const char*)>(1)("storycoop/spray.wav")>0;
}
void spray_wire_begin(int type){sprayWireByte=type==23?0:-1;sprayWirePlayer=0;}
void spray_wire_byte(int value){if(sprayWireByte<0)return;if(sprayWireByte==0&&value!=112){sprayWireByte=-1;return;}if(sprayWireByte==1)sprayWirePlayer=value;++sprayWireByte;}
void spray_wire_end(){
 const int player=sprayWirePlayer;const bool sent=sprayWireByte==3;sprayWireByte=-1;sprayWirePlayer=0;
 if(!sent||!spraySoundReady||!scoped()||player<1||player>load<int>(globals,0x90)||!engine[29]||!engine[72])return;
 const float now=load<float>(globals,0);const auto last=spraySoundTimes.find(player);if(last!=spraySoundTimes.end()&&now>=last->second&&now-last->second<0.2f)return;
 auto e=eng<Edict*(__cdecl*)(int)>(72)(player);if(!live(e))return;spraySoundTimes[player]=now;
 eng<void(__cdecl*)(Edict*,int,const char*,float,float,int,int)>(29)(e,4,"storycoop/spray.wav",1.f,0.8f,0,100);
}
