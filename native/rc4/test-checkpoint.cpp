#include "checkpoint.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <fstream>
using namespace rc4;
template<class T>void field(GlobalSnapshot& g,int token,T v){const std::uint16_t size=sizeof(v),t=static_cast<std::uint16_t>(token);auto append=[&](const void* p,std::size_t n){auto a=static_cast<const unsigned char*>(p);g.bytes.insert(g.bytes.end(),a,a+n);};append(&size,2);append(&t,2);append(&v,sizeof(v));}
void named_field(GlobalSnapshot& g,int token,const char* name,std::size_t count){const std::uint16_t size=static_cast<std::uint16_t>(count),t=static_cast<std::uint16_t>(token);unsigned char bytes[64]{};std::memcpy(bytes,name,std::strlen(name));auto add=[&](const void* p,std::size_t n){auto a=static_cast<const unsigned char*>(p);g.bytes.insert(g.bytes.end(),a,a+n);};add(&size,2);add(&t,2);add(bytes,count);}
GlobalSnapshot global_fixture(){GlobalSnapshot g;g.tokens.resize(1024);g.tokens[1]="GLOBAL";g.tokens[2]="m_listCount";g.tokens[3]="GENT";g.tokens[4]="name";g.tokens[5]="levelName";g.tokens[6]="state";field(g,1,1);field(g,2,1);field(g,3,3);named_field(g,4,"sc4.event.c_start.wake.done",64);named_field(g,5,"s4_start",32);field(g,6,1);return g;}
int main(int argc,char** argv){
 assert(argc==2);Checkpoint v;v.map="s4_start_e0";v.owner="profile:host";v.recorder=419;v.globals=global_fixture();v.haveDigits=true;v.digits={1,2,3,4};v.haveCredentials=true;v.credentials={3,5};
 SavedPlayer p;p.identity=v.owner;p.health=83;p.stamina=75;p.ammo[1]=28;p.weapons={{"weapon_glock",7,1},{"weapon_syringe",2,-1}};p.equipped="weapon_glock";p.inventory[0]="weapon_glock";p.inventory[1]="weapon_syringe";v.players.push_back(p);p.identity="profile:guest";p.health=41;p.ammo[1]=9;p.weapons[0].clip=3;v.players.push_back(p);
 auto bytes=encode_checkpoint(v);assert(!bytes.empty());Checkpoint loaded;assert(decode_checkpoint(bytes,loaded));assert(loaded.players[1].identity=="profile:guest"&&loaded.players[1].health==41&&loaded.players[1].weapons[0].clip==3&&loaded.digits==v.digits);assert(checkpoint_global(loaded.globals,"sc4.event.c_start.wake.done")==1);
 assert(checkpoint_globals_equal(v.globals,loaded.globals));auto changedGlobal=loaded.globals;changedGlobal.bytes[changedGlobal.bytes.size()-4]=2;assert(validate_globals(changedGlobal)&&!checkpoint_globals_equal(v.globals,changedGlobal));
 for(std::size_t i=0;i<bytes.size();i+=101){auto bad=bytes;bad[i]^=1;assert(!decode_checkpoint(bad,loaded));}auto truncated=bytes;truncated.pop_back();assert(!decode_checkpoint(truncated,loaded));
 auto invalid=v;invalid.players[1].identity=v.owner;assert(encode_checkpoint(invalid).empty());invalid=v;invalid.players[0].weapons[0].ammoIndex=32;assert(encode_checkpoint(invalid).empty());invalid=v;invalid.globals.tokens[4]="pointer";assert(encode_checkpoint(invalid).empty());invalid=v;invalid.players[0].inventory[0]="../arbitrary";assert(encode_checkpoint(invalid).empty());invalid=v;invalid.owner="slot1";assert(encode_checkpoint(invalid).empty());
 // v0.3: the basement tape card keeps its capital V (save failed while carried in v0.2).
 {auto t=v;t.players[1].inventory[2]="inventoryitems/Videotape.txt";auto bt=encode_checkpoint(t);Checkpoint back;assert(!bt.empty()&&decode_checkpoint(bt,back)&&back.players[1].inventory[2]=="inventoryitems/Videotape.txt");
  t.players[1].inventory[2]="inventoryitems/../Videotape.txt";assert(encode_checkpoint(t).empty());t.players[1].inventory[2]="inventoryitems/Video tape.txt";assert(encode_checkpoint(t).empty());t.players[0].weapons[0].name="weapon_Glock";assert(encode_checkpoint(t).empty());}
 // Schema 5: quick-select keys round trip; bad keys are refused; a schema 4
 // file (no quick keys) still loads with empty keys.
 {auto q=v;q.players[0].quick[0]={2,"weapon_syringe"};q.players[0].quick[2]={1,"weapon_glock"};auto b5=encode_checkpoint(q);Checkpoint back;
  assert(!b5.empty()&&b5[8]==8&&decode_checkpoint(b5,back)&&back.players[0].quick[0].slot==2&&back.players[0].quick[0].weapon=="weapon_syringe"&&back.players[0].quick[2].weapon=="weapon_glock"&&back.players[0].quick[1].slot==0&&back.players[1].quick[0].weapon.empty());
  for(auto bad:{QuickSlot{7,"weapon_glock"},QuickSlot{1,""},QuickSlot{0,"weapon_glock"},QuickSlot{1,"item_key"},QuickSlot{1,"weapon_../x"}}){auto w=q;w.players[1].quick[1]=bad;assert(encode_checkpoint(w).empty());}
  // Re-seal an older schema: drop the newer appendices, fix length and CRC.
  auto older=[&](std::vector<unsigned char> b,std::size_t tail,unsigned char version){b.resize(b.size()-tail);b[8]=version;const auto n=static_cast<std::uint32_t>(b.size());std::memcpy(b.data()+12,&n,4);
   std::uint32_t c=0xffffffff;for(std::size_t i=20;i<b.size();++i){c^=b[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<int>(c&1)));}c=~c;std::memcpy(b.data()+16,&c,4);return b;};
  const std::size_t v8Tail=28+v.players.size()*4,musicTail=v8Tail+2+4,phoneTail=v.players.size()*8,quickTail=v.players.size()*3*(4+2);
  auto b4=older(encode_checkpoint(v),musicTail+phoneTail+quickTail,4);
  assert(decode_checkpoint(b4,back)&&back.players.size()==v.players.size()&&back.players[0].quick[0].slot==0&&back.players[0].health==83);
  // Schema 6 (coop-fixes-013): phone screen and phone-disabled per player; a
  // schema 5 file loads with the first SMS; out-of-range values are refused.
  auto ph=q;ph.players[0].phoneMessage=3;ph.players[1].phoneMessage=1;ph.players[1].phoneDisabled=1;auto b6=encode_checkpoint(ph);
  assert(!b6.empty()&&b6[8]==8&&decode_checkpoint(b6,back)&&back.players[0].phoneMessage==3&&back.players[0].phoneDisabled==0&&back.players[1].phoneMessage==1&&back.players[1].phoneDisabled==1&&back.players[0].quick[0].weapon=="weapon_syringe");
  auto old5=older(b6,musicTail+phoneTail,5);assert(decode_checkpoint(old5,back)&&back.players[0].phoneMessage==0&&back.players[1].phoneDisabled==0&&back.players[0].quick[2].weapon=="weapon_glock");
  for(int bad:{-1,256}){auto w=ph;w.players[0].phoneMessage=bad;assert(encode_checkpoint(w).empty());}
  // Schema 7 (v0.2): the music the host heard round-trips; a schema 6 file has none; unsafe names are refused.
  auto mu=ph;mu.music="aphall2.mp3";mu.musicLoop=1;auto b7=encode_checkpoint(mu);
  assert(!b7.empty()&&b7[8]==8&&decode_checkpoint(b7,back)&&back.music=="aphall2.mp3"&&back.musicLoop==1&&back.players[0].phoneMessage==3);
  auto old6=older(b6,musicTail,6);assert(decode_checkpoint(old6,back)&&back.music.empty()&&back.musicLoop==0&&back.players[1].phoneDisabled==1);
  for(const char* bad:{"a b.mp3","x\\y.mp3"}){auto w=mu;w.music=bad;assert(encode_checkpoint(w).empty());}
  {auto w=mu;w.musicLoop=2;assert(encode_checkpoint(w).empty());}
  // Schema 8 (v0.5): difficulty, the host's setup and dual wield round-trip; a schema 7 file has none; bad values are refused.
  auto d8=mu;d8.difficulty=4;d8.options={true,3,0,1};d8.players[0].dualA="weapon_switchblade";d8.players[0].dualB="weapon_mobile";d8.doctorWeapon=2;d8.objective="OBJ_FIND_KEY";auto b8=encode_checkpoint(d8);
  assert(!b8.empty()&&b8[8]==8&&decode_checkpoint(b8,back)&&back.difficulty==4&&back.options.have&&back.options.maxPlayers==3&&back.options.collision==0&&back.options.friendlyFire==1&&back.players[0].dualA=="weapon_switchblade"&&back.players[0].dualB=="weapon_mobile"&&back.music=="aphall2.mp3"&&back.doctorWeapon==2&&back.objective=="OBJ_FIND_KEY"&&back.objectiveFile.empty());
  auto old7=older(b7,v8Tail,7);assert(decode_checkpoint(old7,back)&&back.difficulty==0&&!back.options.have&&back.players[0].dualA.empty()&&back.music=="aphall2.mp3");
  for(int bad:{5,-1}){auto w=d8;w.difficulty=bad;auto e=encode_checkpoint(w);assert(e.empty()||!decode_checkpoint(e,back));}}
 // Four stable records may include absent participants; join order is irrelevant.
 p.identity="profile:third";v.players.push_back(p);p.identity="profile:fourth";v.players.push_back(p);assert(decode_checkpoint(encode_checkpoint(v),loaded)&&loaded.players.size()==4);
 const std::filesystem::path dir(argv[1]);assert(commit_checkpoint(dir,1,v));bool backup=false;assert(read_checkpoint(dir,1,loaded,backup)&&!backup);v.players[0].health=64;assert(commit_checkpoint(dir,1,v));assert(read_checkpoint(dir,1,loaded,backup)&&loaded.players[0].health==64&&!backup);
 {std::ofstream f(dir/"slot1.cofsave",std::ios::binary|std::ios::trunc);f<<"damaged";}
 assert(read_checkpoint(dir,1,loaded,backup)&&backup&&loaded.players[0].health==83);assert(!commit_checkpoint(dir,1,v));assert(!commit_checkpoint(dir,6,v)&&!commit_checkpoint(dir,0,v));   // v0.7: slots 1-5
 std::puts("PASS checkpoint format and actual Windows atomic files: distinct/four retained identities, inventory/ammo/code round trip, corruption/truncation/path/identity rejection, backup recovery and corrupt-primary commit refusal");
}
