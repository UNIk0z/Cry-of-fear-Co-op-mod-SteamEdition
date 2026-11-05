#define _CRT_SECURE_NO_WARNINGS
#include "adapter.cpp"
#include <cassert>
#include <limits>
namespace rc4 {struct Edict {unsigned char data[0x400]{};};}
using namespace rc4;
namespace {
std::vector<unsigned char> hw(0x12e8000),client(0x600000),entity(0xb60),cust(0xa4),wad(0x30),cache(0x44),texture(28),surface(48),info(44),wall(24),poly(16+4*28);
int polygons=0,nativeCalls=0,attribs=0,matrices=0,voices=0,rotations=0;unsigned bound=0;float tx=0;bool decalsOn=true,benchmarkMode=false;std::vector<SprayVertex> drawn;std::vector<unsigned> textures;Edict playerEdict;unsigned char globalData[0xa0]{};
std::string currentLevel="s4_fixture";float currentClock=100.f;int fixtureMap=0;int unexpectedFadeWrites=0,unexpectedShakeCalls=0;
const char* level_(){return currentLevel.c_str();}float clock_(){return currentClock;}
void next_map(){currentLevel="s4_fixture"+std::to_string(++fixtureMap);}
const char* wrong_fade_slot(){++unexpectedFadeWrites;return "s4_wrong_fade_slot";}
float wrong_shake_slot(){++unexpectedShakeCalls;return currentClock;}
void native_draw(){++nativeCalls;}
float cvar_(const char*){return decalsOn?1.f:0.f;}
void* entity_(int n){return n==0||n==2?entity.data():nullptr;}
void APIENTRY push_(unsigned){++attribs;}void APIENTRY pop_(){--attribs;}
void APIENTRY noop_(unsigned){}void APIENTRY pair_(unsigned,unsigned){}void APIENTRY depth_(unsigned char){}void APIENTRY alpha_(unsigned,float){}void APIENTRY offset_(float,float){}void APIENTRY color_(float,float,float,float){}
void APIENTRY bind_(unsigned,unsigned id){bound=id;}
void APIENTRY begin_(unsigned mode){assert(mode==6);++polygons;if(!benchmarkMode)textures.push_back(bound);}
void APIENTRY end_(){}void APIENTRY uv_(float u,float v){assert(u>=-0.001f&&u<=1.001f&&v>=-0.001f&&v<=1.001f);}
void APIENTRY vertex_(float x,float y,float z){if(!benchmarkMode)drawn.push_back({x,y,z,0,0});}
void APIENTRY matrix_push(){++matrices;}void APIENTRY matrix_pop(){--matrices;}void APIENTRY translate_(float x,float,float){tx=x;}void APIENTRY rotate_(float angle,float,float,float){if(angle==90.f)++rotations;}
void print_(const char*){}int index_(Edict*){return 1;}Edict* edict_(int i){return i==1?&playerEdict:nullptr;}
const char* string_(int n){return n==1?"s4_start":"player";}
void* vars_(Edict* e){return e->data+0x80;}
void sound_(Edict* e,int channel,const char* s,float,float,int,int){assert(e==&playerEdict&&channel==4&&!std::strcmp(s,"storycoop/spray.wav"));++voices;}
void wire_end(){}void wire_begin(int,int,const float*,Edict*){}void wire_byte(int){}void wire_short(int){}
void setup(){
 sprayHw=hw.data();clientImage=client.data();clientImageSize=client.size();sprayOriginal=&native_draw;
 store(clientImage,0x1b2608+15*4,&cvar_);// Independent API order from the engine initializer, not from the renderer's offsets.
 store(clientImage,0x1b2608+53*4,&entity_);store(clientImage,0x1b2608+54*4,&clock_);store(clientImage,0x1b2608+74*4,&level_);
 store(clientImage,0x1b2608+55*4,&wrong_shake_slot);store(clientImage,0x1b2608+76*4,&wrong_fade_slot);
 store(spray_hw(SprayPlayersVa),0x234,cust.data());store(cust.data(),0,1);store(cust.data(),0x44,3);store<unsigned char>(cust.data(),0x50,4);store(cust.data(),0x8c,1);store(cust.data(),0x98,wad.data());
 store(wad.data(),4,cache.data());store(wad.data(),8,1);store(cache.data(),0x40,texture.data());store<unsigned>(texture.data(),16,64);store<unsigned>(texture.data(),20,64);store(texture.data(),24,77);
 store(surface.data(),36,poly.data());store(surface.data(),44,info.data());store(info.data(),36,wall.data());store<unsigned>(wall.data(),16,64);store<unsigned>(wall.data(),20,64);store(poly.data(),8,4);
 const float xyz[4][5]={{0,0,0,0,0},{64,0,0,1,0},{64,64,0,1,1},{0,64,0,0,1}};
 for(int i=0;i<4;++i)for(int j=0;j<5;++j)store(poly.data(),16+static_cast<std::size_t>(i)*28+static_cast<std::size_t>(j)*4,xyz[i][j]);
 auto& d=*static_cast<SprayDecal*>(spray_hw(SprayPoolVa));d={};d.surface=surface.data();d.scale=1;d.texture=-1;d.flags=4;
 sprayGL.push=push_;sprayGL.pop=pop_;sprayGL.enable=noop_;sprayGL.disable=noop_;sprayGL.blend=pair_;sprayGL.depthMask=depth_;sprayGL.depthFunc=noop_;sprayGL.alpha=alpha_;sprayGL.offset=offset_;sprayGL.color=color_;sprayGL.bind=bind_;sprayGL.begin=begin_;sprayGL.end=end_;sprayGL.uv=uv_;sprayGL.vertex=vertex_;sprayGL.active=noop_;sprayGL.matrixMode=noop_;sprayGL.pushMatrix=matrix_push;sprayGL.popMatrix=matrix_pop;sprayGL.translate=translate_;sprayGL.rotate=rotate_;sprayGL.ready=true;
 globals=globalData;verified=true;spraySoundReady=true;store(globals,0x0c,1);store(globals,0x90,2);store(globals,0x94,32);store(globals,0,10.f);store(playerEdict.data,4,1);engine[66]=reinterpret_cast<void*>(&string_);engine[68]=reinterpret_cast<void*>(&vars_);engine[71]=reinterpret_cast<void*>(&index_);engine[72]=reinterpret_cast<void*>(&edict_);engine[29]=reinterpret_cast<void*>(&sound_);engine[81]=reinterpret_cast<void*>(&print_);engine[46]=reinterpret_cast<void*>(&wire_begin);engine[47]=reinterpret_cast<void*>(&wire_end);engine[48]=reinterpret_cast<void*>(&wire_byte);engine[50]=reinterpret_cast<void*>(&wire_short);
}
}
void retention_checks(){
 auto pool=static_cast<SprayDecal*>(spray_hw(SprayPoolVa));const auto clearPool=[&](){for(int i=0;i<4096;++i)pool[i]={};};clearPool();spray_cache_reset();next_map();currentClock=100.f;
 const float xyz[4][5]={{0,0,0,0,0},{64,0,0,1,0},{64,64,0,1,1},{0,64,0,0,1}};
 store(poly.data(),8,4);for(int i=0;i<4;++i)for(int j=0;j<5;++j)store(poly.data(),16+static_cast<std::size_t>(i)*28+static_cast<std::size_t>(j)*4,xyz[i][j]);
 SprayDecal base{};base.surface=surface.data();base.scale=1;base.texture=-1;base.flags=4;
 auto cust2=cust,wad2=wad,cache2=cache,texture2=texture,surface2=surface;
 store(cust2.data(),0x98,wad2.data());store(wad2.data(),4,cache2.data());store(cache2.data(),0x40,texture2.data());store(texture2.data(),24,88);
 store(spray_hw(SprayPlayersVa),0x250+0x234,cust2.data());
 const auto frame=[&](int expected){const int before=polygons;drawn.clear();textures.clear();spray_draw_callback();assert(polygons-before==expected&&attribs==0&&matrices==0);};
 pool[0]=base;frame(1);pool[1]=base;pool[1].texture=-2;frame(2);
 pool[2]=base;pool[2].x=0.25f;frame(2);assert(drawn[0].x>=15.999f&&textures[0]==77&&textures[1]==88);
 // Merely relinking an older surface decal must not make it the newest spray again.
 pool[0].next=&pool[1];frame(2);assert(drawn[0].x>=15.999f);
 // Both faces of one accepted burst stay together, and the partner's logo stays.
 pool[3]=base;pool[3].x=0.5f;pool[4]=pool[3];pool[4].surface=surface2.data();pool[4].entity=2;frame(3);
 assert(sprayLatest[0].size()==2&&sprayLatest[1].size()==1);
 // Reuse an earlier pool slot: array order is not spray age.
 pool[0]=base;pool[0].x=0.75f;frame(2);assert(sprayLatest[0].size()==1&&drawn[0].x>=47.999f);
 for(int i=0;i<20;++i)frame(2);
 // A new upload may finish later; a departed player's earlier logo must not return.
 store<void*>(spray_hw(SprayPlayersVa),0x250+0x234,nullptr);frame(1);pool[5]=base;pool[5].texture=-2;pool[5].x=0.4f;frame(1);
 store(spray_hw(SprayPlayersVa),0x250+0x234,cust2.data());frame(2);assert(sprayLatest[1].size()==1);
 currentClock-=0.0075f;frame(2);currentClock=50.f;frame(2);currentClock=100.f;frame(2);
 clearPool();frame(0);assert(sprayLatest[0].empty()&&sprayLatest[1].empty());
 // A map change can reuse addresses and identical decal fields with different geometry.
 pool[0]=base;frame(1);const float firstX=drawn[0].x;
 for(int i=0;i<4;++i){const auto at=16+static_cast<std::size_t>(i)*28;store(poly.data(),at,load<float>(poly.data(),at)+128.f);}next_map();frame(1);assert(std::fabs(drawn[0].x-firstX-128.f)<0.001f);
 for(int i=0;i<4;++i){const auto at=16+static_cast<std::size_t>(i)*28;store(poly.data(),at,load<float>(poly.data(),at)+64.f);}currentClock=1.f;frame(1);assert(std::fabs(drawn[0].x-firstX-192.f)<0.001f);
 decalsOn=false;pool[10]=base;pool[10].x=0.2f;frame(0);pool[11]=base;pool[11].x=0.6f;frame(0);decalsOn=true;frame(1);assert(sprayLatest[0].size()==1);
 spray_client_uninstall();for(const auto& latest:sprayLatest)assert(latest.empty());
 std::puts("PASS retention: owner-only replacement, partner preserved, multi-face burst, slot reuse, relinking, deferred upload, departure, empty pool, map/time reset, hidden decals and unload");
}
int main(int argc,char** args){
 setup();auto& d=*static_cast<SprayDecal*>(spray_hw(SprayPoolVa));
 if(argc>1&&!std::strcmp(args[1],"--abi")){
  auto pool=static_cast<SprayDecal*>(spray_hw(SprayPoolVa));pool[0]={};spray_draw_callback();
  if(unexpectedFadeWrites||unexpectedShakeCalls){std::printf("FAIL real API layout: fade setter calls=%d shake calls=%d\n",unexpectedFadeWrites,unexpectedShakeCalls);return 1;}
  if(sprayLevel!=currentLevel||sprayClock!=currentClock){std::puts("FAIL real API getter values");return 1;}
  std::puts("PASS real API layout: correct getters, no screen-fade setter or shake calls");return 0;
 }

 if(argc>1&&!std::strcmp(args[1],"--benchmark")){
  benchmarkMode=true;auto pool=static_cast<SprayDecal*>(spray_hw(SprayPoolVa));const auto base=d;pool[0]={};
  for(int player=0;player<4;++player)store(spray_hw(SprayPlayersVa),static_cast<std::size_t>(player)*0x250+0x234,cust.data());
  spray_draw_callback();
  for(int i=0;i<1000;++i){pool[i]=base;pool[i].texture=static_cast<short>(-1-i%4);pool[i].x=static_cast<float>(i%16)*0.01f;spray_draw_callback();}
  for(int i=0;i<5;++i)spray_draw_callback();
  const int frames=200;polygons=0;const auto start=std::chrono::steady_clock::now();for(int i=0;i<frames;++i)spray_draw_callback();
  const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/frames;
  std::printf("BENCH sprays=1000 players=4 frames=%d polygons_per_frame=%d cpu_us_per_frame=%.3f\n",frames,polygons/frames,us);return 0;
 }
 if(argc>1&&!std::strcmp(args[1],"--retention")){
  retention_checks();return 0;
 }
 if(argc>1){native_draw();if(!polygons){std::puts("FAIL original SDK callback: transferred spray exists, zero polygons drawn");return 1;}}
 spray_draw_callback();assert(nativeCalls==1&&polygons==1&&bound==77&&drawn.size()==4&&attribs==0&&matrices==0);
 // Wall, floor, ceiling: the projection works in arbitrary surface planes.
 for(int axis=0;axis<3;++axis){for(int i=0;i<4;++i){const float a=(i==1||i==2)?64.f:0.f,b=i>=2?64.f:0.f;store(poly.data(),16+static_cast<std::size_t>(i)*28+static_cast<std::size_t>(axis)*4,12.f);store(poly.data(),16+static_cast<std::size_t>(i)*28+static_cast<std::size_t>((axis+1)%3)*4,a);store(poly.data(),16+static_cast<std::size_t>(i)*28+static_cast<std::size_t>((axis+2)%3)*4,b);}const int before=polygons;next_map();spray_draw_callback();assert(polygons==before+1);for(std::size_t j=drawn.size()-4;j<drawn.size();++j){const auto& v=drawn[j];assert(std::fabs((axis==0?v.x:axis==1?v.y:v.z)-12.f)<0.001f);}}
 d.entity=2;store(entity.data(),0xb48,100.f);store(entity.data(),0xb58,90.f);spray_draw_callback();assert(tx==100&&matrices==0&&rotations==1);d.entity=0;
 SprayPolygon p;d.x=0.5f;d.flags|=24;assert(spray_polygon(d,p)&&p.vertices.size()>=3);d.x=0;d.flags=4;
 const int before=polygons;d.texture=-2;next_map();spray_draw_callback();assert(polygons==before);d.texture=-1;
 d.scale=std::numeric_limits<float>::quiet_NaN();next_map();spray_draw_callback();assert(polygons==before);d.scale=1;store(poly.data(),8,1000000);next_map();spray_draw_callback();assert(polygons==before);store(poly.data(),8,4);
 decalsOn=false;spray_draw_callback();assert(polygons==before);decalsOn=true;
 // A second player's downloaded texture has its own cache and is drawn by this same local client.
 auto cust2=cust,wad2=wad,cache2=cache,texture2=texture;
 store(cust2.data(),0x98,wad2.data());store(wad2.data(),4,cache2.data());store(cache2.data(),0x40,texture2.data());store(texture2.data(),24,88);
 store(spray_hw(SprayPlayersVa),0x250+0x234,cust2.data());
 auto& partner=static_cast<SprayDecal*>(spray_hw(SprayPoolVa))[1];partner=d;partner.texture=-2;
 const int twoBefore=polygons;spray_draw_callback();assert(polygons==twoBefore+2&&textures[textures.size()-2]==77&&textures.back()==88);partner={};
 // Head-shield passes must forward the game's callback without drawing into its preview.
 store(clientImage,0x556b48,1);const int guarded=polygons;spray_draw_callback();assert(polygons==guarded);store(clientImage,0x556b48,0);
 auto slot=&native_draw;assert(spray_attach(slot,&native_draw)&&slot==spray_draw_callback);spray_restore(slot);assert(slot==native_draw);
 slot=nullptr;assert(!spray_attach(slot,&native_draw));spray_restore(slot);assert(!slot);
 // Native TE_PLAYERDECAL wire path emits a shared sound only after a successful spray.
 message_begin(0,23,nullptr,nullptr);write_byte(112);write_byte(1);write_short(0);write_byte(0);message_end();assert(voices==1);
 message_begin(0,23,nullptr,nullptr);write_byte(112);write_byte(1);write_short(0);write_byte(0);message_end();assert(voices==1);
 store(globals,0,11.f);message_begin(0,23,nullptr,nullptr);write_byte(109);message_end();assert(voices==1);
 message_begin(0,23,nullptr,nullptr);write_byte(112);write_byte(1);write_short(0);write_byte(0);message_end();assert(voices==2);
 retention_checks();
 assert(unexpectedFadeWrites==0&&unexpectedShakeCalls==0);
 std::puts("PASS spray: SDK render callback, received player texture, walls/floors/ceilings, brush transform, clipping, invalid input, GL state balance, native broadcast sound and deduplication. No game launched.");
}
