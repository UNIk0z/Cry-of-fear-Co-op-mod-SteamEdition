#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "checkpoint.hpp"
#include <cstring>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <set>
#include <map>
#include <stdexcept>
namespace rc4 {
namespace {
constexpr std::size_t Limit=65536;
struct Writer {
 std::vector<unsigned char> b;
 template<class T>void put(T v){const auto p=reinterpret_cast<const unsigned char*>(&v);b.insert(b.end(),p,p+sizeof(v));}
 void raw(const void* p,std::size_t n){auto a=static_cast<const unsigned char*>(p);b.insert(b.end(),a,a+n);}
 void str(const std::string& s){if(s.size()>255)throw std::runtime_error("string");put<std::uint16_t>(static_cast<std::uint16_t>(s.size()));raw(s.data(),s.size());}
};
struct Reader {
 const std::vector<unsigned char>& b;std::size_t at=0;
 void raw(void* p,std::size_t n){if(n>b.size()-at)throw std::runtime_error("bounds");std::memcpy(p,b.data()+at,n);at+=n;}
 template<class T>T get(){T v{};raw(&v,sizeof(v));return v;}
 std::string str(){auto n=get<std::uint16_t>();if(n>255||n>b.size()-at)throw std::runtime_error("string");std::string s(reinterpret_cast<const char*>(b.data()+at),n);at+=n;if(s.find('\0')!=s.npos)throw std::runtime_error("nul");return s;}
};
std::uint32_t crc(const unsigned char* p,std::size_t n){std::uint32_t c=0xffffffff;for(std::size_t i=0;i<n;++i){c^=p[i];for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<int>(c&1)));}return ~c;}
bool text(const std::string& s,std::size_t max){return s.size()<=max&&std::all_of(s.begin(),s.end(),[](char c){return c>=32&&c<=126;});}
bool resource(const std::string& s){return !s.empty()&&s.size()<128&&s.find("..") ==s.npos&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='/'||c=='.';});}
// Inventory cards keep the map's spelling: c_apartment4's tape is
// "inventoryitems/Videotape.txt" (the only mixed-case card in the shipped maps).
bool card(const std::string& s){return !s.empty()&&s.size()<128&&s.find("..")==s.npos&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='/'||c=='.';});}
bool weapon(const std::string& s){return resource(s)&&s.rfind("weapon_",0)==0&&s.find('/')==s.npos;}
bool coordinate(float n){return std::isfinite(n)&&n>=-32768.f&&n<=32768.f;}
bool entity_type(const std::string& s){return !s.empty()&&s.size()<64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';});}
bool valid(const Checkpoint& v){
 const bool legacy=legacy_checkpoint_map(v.map);
 if(!checkpoint_map(v.map)||!checkpoint_identity(v.owner)||(legacy?(v.recorder!=419&&v.recorder!=421):(v.recorder<1||v.recorder>8191))||v.players.empty()||v.players.size()>4||v.notes.size()>16||v.notes.size()!=v.metadata.size()||!validate_globals(v.globals))return false;
 if(v.removed.size()>WorldRemovedLimit||v.unlocked.size()>WorldUnlockedLimit)return false;
 std::set<int> seen;for(const auto& [index,type]:v.removed)if(index<1||index>8191||!entity_type(type)||!seen.insert(index).second)return false;
 seen.clear();for(int index:v.unlocked)if(index<1||index>8191||!seen.insert(index).second)return false;
 for(const auto& p:v.players){
  if(!legacy&&!p.hasPose)return false;
  if(p.hasPose&&(!std::all_of(p.origin.begin(),p.origin.end(),coordinate)||!std::all_of(p.angles.begin(),p.angles.end(),coordinate)))return false;
 }
 if(v.haveKeypad&&(v.keypadCode<0||v.keypadCode>9999||v.keypadType<0||v.keypadType>16))return false;
 if(v.endingFlags<0||v.endingFlags>7)return false;
 if(v.music.size()>60||(v.musicLoop!=0&&v.musicLoop!=1)||!std::all_of(v.music.begin(),v.music.end(),[](char c){return std::isalnum(static_cast<unsigned char>(c))||c=='_'||c=='-'||c=='.'||c=='/';}))return false;
 for(const auto& p:v.players)if(p.healthCap<0||p.healthCap>2||p.phoneMessage<0||p.phoneMessage>255||p.phoneDisabled<0||p.phoneDisabled>255)return false;
 for(const auto& p:v.players)for(const auto& q:p.quick)if(q.slot<0||q.slot>6||(q.slot==0)!=q.weapon.empty()||(!q.weapon.empty()&&!weapon(q.weapon)))return false;
 if(v.haveDigits&&!std::all_of(v.digits.begin(),v.digits.end(),[](int n){return n>=1&&n<=9;}))return false;
 if(v.haveCredentials&&!std::all_of(v.credentials.begin(),v.credentials.end(),[](int n){return n>=1&&n<=8;}))return false;
 std::set<std::string> ids;bool owner=false;
 for(const auto& p:v.players){
  if(!checkpoint_identity(p.identity)||!ids.insert(p.identity).second||!std::isfinite(p.health)||p.health<=0||p.health>1000||!std::isfinite(p.stamina)||p.stamina<0||p.stamina>1000||p.weapons.size()>32||p.tapes<0||p.tapes>10000||p.battery<0||p.battery>100)return false;
  owner=owner||p.identity==v.owner;std::set<std::string> names;
  for(const auto& w:p.weapons)if(!weapon(w.name)||!names.insert(w.name).second||w.clip<-1||w.clip>10000||w.ammoIndex<-1||w.ammoIndex>=32)return false;
  if(!p.equipped.empty()&&!names.count(p.equipped))return false;
  for(auto n:p.ammo)if(n<0||n>100000)return false;
  for(auto n:p.items)if(n<0||n>100000)return false;
  for(const auto& n:p.inventory)if(!n.empty()&&!card(n))return false;
 }
 Journal checked;for(std::size_t i=0;i<v.notes.size();++i){const auto m=v.metadata[i];if(!Journal::valid(v.notes[i])||m.codeFlag<0||m.codeFlag>255||m.codeValue<-32768||m.codeValue>32767||m.codeType<0||m.codeType>255||checked.grant(v.notes[i],m))return false;}if(checked.size()!=v.notes.size())return false;
 return owner;
}
std::filesystem::path slot_path(const std::filesystem::path& d,int slot){return d/(L"slot"+std::to_wstring(slot)+L".cofsave");}
bool ordinary(const std::filesystem::path& p,bool directory){auto a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_REPARSE_POINT)&&((a&FILE_ATTRIBUTE_DIRECTORY)!=0)==directory;}
bool file_read(const std::filesystem::path& p,Checkpoint& v){if(!ordinary(p,false))return false;std::ifstream f(p,std::ios::binary|std::ios::ate);auto n=f.tellg();if(n<56||n>static_cast<std::streamoff>(Limit))return false;std::vector<unsigned char> b(static_cast<std::size_t>(n));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),n);return f.good()&&decode_checkpoint(b,v);}
}
bool legacy_checkpoint_map(const std::string& n){return n=="s4_start"||n=="s4_start_e0"||n=="s4_start_e2";}
// Any served RC2 (s2_) or RC4 (s4_) campaign alias except the passive
// nightmare; the opening map never reaches here (it is not scoped).
bool checkpoint_map(const std::string& n){
 if(legacy_checkpoint_map(n))return true;
 if(n.size()<4||n.size()>31||(n.rfind("s2_",0)!=0&&n.rfind("s4_",0)!=0)||n=="s4_nightmare")return false;
 return std::all_of(n.begin(),n.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';});
}
bool checkpoint_identity(const std::string& n){return !n.empty()&&n.size()<=63&&std::all_of(n.begin(),n.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c==':'||c=='-';});}
// Only the pinned native CGlobalState's primitive stream is accepted. No entity
// pointer, function, whole SP world, or player object is serialized/restored.
bool validate_globals(const GlobalSnapshot& g){
 if(g.bytes.empty()||g.bytes.size()>32768||g.tokens.size()!=1024)return false;
 const std::set<std::string> permitted{"GLOBAL","GENT","cGLOBAL","cGENT","m_listCount","name","levelName","state"};
 for(const auto& s:g.tokens)if(!s.empty()&&!permitted.count(s))return false;
 std::size_t pos=0;int expected=-1,records=0,left=0;bool started=false,record=false;std::set<std::string> keys,recordNames;
 while(pos<g.bytes.size()){
  if(g.bytes.size()-pos<4)return false;std::uint16_t size=0,token=0;std::memcpy(&size,g.bytes.data()+pos,2);std::memcpy(&token,g.bytes.data()+pos+2,2);pos+=4;
  if(token>=g.tokens.size()||size>g.bytes.size()-pos)return false;const auto& name=g.tokens[token];
  const bool section=name=="GLOBAL"||name=="GENT"||name=="cGLOBAL"||name=="cGENT";
  if(section){if(left!=0||size!=4)return false;int count=0;std::memcpy(&count,g.bytes.data()+pos,4);const bool isRecord=name=="GENT"||name=="cGENT";if((!started&&isRecord)||(started&&!isRecord)||count<0||count>(isRecord?3:1))return false;started=true;record=isRecord;left=count;keys.clear();}
  else {if(!started||left<=0||!keys.insert(name).second||(!record&&name!="m_listCount")||(record&&name!="name"&&name!="levelName"&&name!="state"))return false;--left;}
  if(name=="GLOBAL"||name=="GENT"||name=="cGLOBAL"||name=="cGENT"||name=="m_listCount"||name=="state"){
   if(size!=4)return false;int n=0;std::memcpy(&n,g.bytes.data()+pos,4);
   if(name=="m_listCount"){if(expected!=-1||n<0||n>256)return false;expected=n;}
   else if(name=="state"){if(n<0||n>2)return false;}
   else if(n<0||n>3)return false;
  }else if(name=="name"||name=="levelName"){
   if(size!=(name=="name"?64:32)||!std::memchr(g.bytes.data()+pos,0,size))return false;
   if(!text(reinterpret_cast<const char*>(g.bytes.data()+pos),size-1))return false;if(name=="name"){if(!recordNames.insert(reinterpret_cast<const char*>(g.bytes.data()+pos)).second)return false;++records;}
  }else return false;
  pos+=size;
 }
 // FIELD_INTEGER zero values can be omitted by the original writer.
 return started&&left==0&&records==(expected==-1?0:expected);
}
int checkpoint_global(const GlobalSnapshot& g,const std::string& name){
 if(!validate_globals(g))return -1;std::size_t pos=0;bool match=false;int result=-1;
 while(pos<g.bytes.size()){std::uint16_t n=0,t=0;std::memcpy(&n,g.bytes.data()+pos,2);std::memcpy(&t,g.bytes.data()+pos+2,2);pos+=4;const auto& key=g.tokens[t];if(key=="GENT"||key=="cGENT")match=false;
  if(key=="name"){match=name==reinterpret_cast<const char*>(g.bytes.data()+pos);if(match)result=0;}else if(key=="state"&&match)std::memcpy(&result,g.bytes.data()+pos,4);pos+=n;
 }return result;
}
bool checkpoint_globals_equal(const GlobalSnapshot& a,const GlobalSnapshot& b){
 if(!validate_globals(a)||!validate_globals(b))return false;
 auto records=[](const GlobalSnapshot& g){std::map<std::string,std::pair<std::string,int>> result;std::size_t pos=0;std::string name,level;int state=0;
  auto finish=[&](){if(!name.empty())result[name]={level,state};name.clear();level.clear();state=0;};
  while(pos<g.bytes.size()){std::uint16_t n=0,t=0;std::memcpy(&n,g.bytes.data()+pos,2);std::memcpy(&t,g.bytes.data()+pos+2,2);pos+=4;const auto& key=g.tokens[t];
   if(key=="GENT"||key=="cGENT")finish();else if(key=="name")name=reinterpret_cast<const char*>(g.bytes.data()+pos);else if(key=="levelName")level=reinterpret_cast<const char*>(g.bytes.data()+pos);else if(key=="state")std::memcpy(&state,g.bytes.data()+pos,4);pos+=n;
  }finish();return result;
 };return records(a)==records(b);
}
std::vector<unsigned char> encode_checkpoint(const Checkpoint& v){
 if(!valid(v))return {};Writer w;w.raw("COFCKP01",8);w.put<std::uint32_t>(8);w.put<std::uint32_t>(0);w.put<std::uint32_t>(0);char map[32]{};std::memcpy(map,v.map.data(),v.map.size());w.raw(map,32);w.put<std::int32_t>(v.recorder);
 w.str("cof-original-story-rc4");w.str(v.owner);w.put<std::uint32_t>(static_cast<std::uint32_t>(v.notes.size()));
 for(std::size_t i=0;i<v.notes.size();++i){w.raw(v.notes[i].data(),v.notes[i].size());w.put(v.metadata[i].codeFlag);w.put(v.metadata[i].codeValue);w.put(v.metadata[i].codeType);}
 w.put<int>(v.haveDigits?1:0);for(int n:v.digits)w.put(n);w.put<int>(v.haveCredentials?1:0);for(int n:v.credentials)w.put(n);
 w.put<std::uint32_t>(static_cast<std::uint32_t>(v.globals.bytes.size()));w.raw(v.globals.bytes.data(),v.globals.bytes.size());for(const auto& s:v.globals.tokens)w.str(s);
 w.put<std::uint32_t>(static_cast<std::uint32_t>(v.players.size()));for(const auto& p:v.players){w.str(p.identity);w.str(p.equipped);w.put(p.health);w.put(p.stamina);for(int n:p.ammo)w.put(n);for(int n:p.items)w.put(n);for(const auto& s:p.inventory)w.str(s);w.put(p.tapes);w.put(p.battery);w.put<int>(p.usedSyringe?1:0);w.put<std::uint32_t>(static_cast<std::uint32_t>(p.weapons.size()));for(const auto& a:p.weapons){w.str(a.name);w.put(a.clip);w.put(a.ammoIndex);}}
 // Schema 2 appendix: poses, then the world-state lists.
 for(const auto& p:v.players){w.put<int>(p.hasPose?1:0);if(p.hasPose){for(float n:p.origin)w.put(n);for(float n:p.angles)w.put(n);}}
 w.put<std::uint32_t>(static_cast<std::uint32_t>(v.removed.size()));for(const auto& [index,type]:v.removed){w.put<int>(index);w.str(type);}
 w.put<std::uint32_t>(static_cast<std::uint32_t>(v.unlocked.size()));for(int index:v.unlocked)w.put<int>(index);
 // Schema 3 appendix: the shared keypad code.
 w.put<int>(v.haveKeypad?1:0);w.put<int>(v.keypadCode);w.put<int>(v.keypadType);
 // Schema 4 appendix: per-player story wounds/night vision, then the endings.
 for(const auto& p:v.players){w.put<int>(p.healthCap);w.put<int>(p.nightVision?1:0);}
 w.put<int>(v.endingFlags);
 // Schema 5 appendix: per-player quick-select keys.
 for(const auto& p:v.players)for(const auto& q:p.quick){w.put<int>(q.slot);w.str(q.weapon);}
 // Schema 6 appendix: per-player phone screen and phone-disabled state.
 for(const auto& p:v.players){w.put<int>(p.phoneMessage);w.put<int>(p.phoneDisabled);}
 // Schema 7 appendix: the music track and its loop flag.
 w.str(v.music);w.put<int>(v.musicLoop);
 // Schema 8 appendix: difficulty, the host's setup, per-player dual wield.
 w.put<int>(v.difficulty);w.put<int>(v.options.have?1:0);w.put<int>(v.options.maxPlayers);w.put<int>(v.options.collision);w.put<int>(v.options.friendlyFire);
 for(const auto& p:v.players){w.str(p.dualA);w.str(p.dualB);}w.put<int>(v.doctorWeapon);w.str(v.objective);w.str(v.objectiveFile);
 if(w.b.size()>Limit)return {};auto count=static_cast<std::uint32_t>(w.b.size()),sum=crc(w.b.data()+20,w.b.size()-20);std::memcpy(w.b.data()+12,&count,4);std::memcpy(w.b.data()+16,&sum,4);return w.b;
}
bool decode_checkpoint(const std::vector<unsigned char>& b,Checkpoint& out){
 try{if(b.size()<56||b.size()>Limit||std::memcmp(b.data(),"COFCKP01",8))return false;Reader r{b,8};const auto version=r.get<std::uint32_t>();if(version<1||version>8||r.get<std::uint32_t>()!=b.size()||r.get<std::uint32_t>()!=crc(b.data()+20,b.size()-20))return false;
  Checkpoint v;char map[32]{};r.raw(map,32);if(!std::memchr(map,0,32))return false;v.map=map;v.recorder=r.get<int>();if(r.str()!="cof-original-story-rc4")return false;v.owner=r.str();auto n=r.get<std::uint32_t>();if(n>16)return false;
  for(std::uint32_t i=0;i<n;++i){Note note{};r.raw(note.data(),note.size());v.notes.push_back(note);v.metadata.push_back({r.get<int>(),r.get<int>(),r.get<int>()});}
  auto flag=r.get<int>();if(flag!=0&&flag!=1)return false;v.haveDigits=flag!=0;for(auto& k:v.digits)k=r.get<int>();flag=r.get<int>();if(flag!=0&&flag!=1)return false;v.haveCredentials=flag!=0;for(auto& k:v.credentials)k=r.get<int>();
  n=r.get<std::uint32_t>();if(n>32768)return false;v.globals.bytes.resize(n);r.raw(v.globals.bytes.data(),n);for(int i=0;i<1024;++i)v.globals.tokens.push_back(r.str());
  n=r.get<std::uint32_t>();if(n>4)return false;for(std::uint32_t i=0;i<n;++i){SavedPlayer p;p.identity=r.str();p.equipped=r.str();p.health=r.get<float>();p.stamina=r.get<float>();for(auto& k:p.ammo)k=r.get<int>();for(auto& k:p.items)k=r.get<int>();for(auto& s:p.inventory)s=r.str();p.tapes=r.get<int>();p.battery=r.get<int>();flag=r.get<int>();if(flag!=0&&flag!=1)return false;p.usedSyringe=flag!=0;auto m=r.get<std::uint32_t>();if(m>32)return false;for(std::uint32_t j=0;j<m;++j)p.weapons.push_back({r.str(),r.get<int>(),r.get<int>()});v.players.push_back(p);}
  if(version>=2){
   for(auto& p:v.players){flag=r.get<int>();if(flag!=0&&flag!=1)return false;p.hasPose=flag!=0;if(p.hasPose){for(auto& k:p.origin)k=r.get<float>();for(auto& k:p.angles)k=r.get<float>();}}
   n=r.get<std::uint32_t>();if(n>WorldRemovedLimit)return false;for(std::uint32_t i=0;i<n;++i){const int index=r.get<int>();v.removed.push_back({index,r.str()});}
   n=r.get<std::uint32_t>();if(n>WorldUnlockedLimit)return false;for(std::uint32_t i=0;i<n;++i)v.unlocked.push_back(r.get<int>());
  }
  if(version>=3){flag=r.get<int>();if(flag!=0&&flag!=1)return false;v.haveKeypad=flag!=0;v.keypadCode=r.get<int>();v.keypadType=r.get<int>();}
  if(version>=4){for(auto& p:v.players){p.healthCap=r.get<int>();flag=r.get<int>();if(flag!=0&&flag!=1)return false;p.nightVision=flag!=0;}v.endingFlags=r.get<int>();}
  if(version>=5)for(auto& p:v.players)for(auto& q:p.quick){q.slot=r.get<int>();q.weapon=r.str();}
  if(version>=6)for(auto& p:v.players){p.phoneMessage=r.get<int>();p.phoneDisabled=r.get<int>();}
  if(version>=7){v.music=r.str();v.musicLoop=r.get<int>();}
  if(version>=8){v.difficulty=r.get<int>();flag=r.get<int>();if(flag!=0&&flag!=1)return false;v.options.have=flag!=0;v.options.maxPlayers=r.get<int>();v.options.collision=r.get<int>();v.options.friendlyFire=r.get<int>();
   if(v.difficulty<0||v.difficulty>4||v.options.maxPlayers<1||v.options.maxPlayers>4||(v.options.collision!=0&&v.options.collision!=1)||(v.options.friendlyFire!=0&&v.options.friendlyFire!=1))return false;
   for(auto& p:v.players){p.dualA=r.str();p.dualB=r.str();}v.doctorWeapon=r.get<int>();if(v.doctorWeapon<0||v.doctorWeapon>2)return false;v.objective=r.str();v.objectiveFile=r.str();}
  if(r.at!=b.size()||!valid(v))return false;out=std::move(v);return true;
 }catch(const std::exception&){return false;}
}
bool read_checkpoint(const std::filesystem::path& d,int slot,Checkpoint& v,bool& backup){backup=false;if(slot<1||slot>5||!ordinary(d,true))return false;auto p=slot_path(d,slot);if(file_read(p,v))return true;backup=true;return file_read(p.wstring()+L".bak",v);}
bool commit_checkpoint(const std::filesystem::path& d,int slot,const Checkpoint& v){
 if(slot<1||slot>5)return false;auto b=encode_checkpoint(v);if(b.empty())return false;
 if(!CreateDirectoryW(d.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return false;if(!ordinary(d,true))return false;
 const auto p=slot_path(d,slot),backup=std::filesystem::path(p.wstring()+L".bak"),tmp=std::filesystem::path(p.wstring()+L".tmp-"+std::to_wstring(GetCurrentProcessId()));
 HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);if(h==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool okay=WriteFile(h,b.data(),static_cast<DWORD>(b.size()),&written,nullptr)&&written==b.size()&&FlushFileBuffers(h);CloseHandle(h);
 if(okay){Checkpoint old;const auto attr=GetFileAttributesW(p.c_str());if(attr==INVALID_FILE_ATTRIBUTES)okay=MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
  else if(ordinary(p,false)&&file_read(p,old)&&(GetFileAttributesW(backup.c_str())==INVALID_FILE_ATTRIBUTES||ordinary(backup,false)))okay=ReplaceFileW(p.c_str(),tmp.c_str(),backup.c_str(),0,nullptr,nullptr)!=0;
  else okay=false; // A damaged primary is recovered on read, never made the backup.
 }DeleteFileW(tmp.c_str());return okay;
}
}
