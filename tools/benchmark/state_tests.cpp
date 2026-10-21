#pragma comment(lib, "Ws2_32.lib")
#include "Server.h"
#include "CsvStore.h"
#include <thread>
#include <atomic>
#include <cassert>
#include <iostream>
#include <stdexcept>
#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while(0)
static void Load(UserManager& u) {
 u.LoadAccountUsers("UsersAccount.csv");u.LoadUserProfiles("UserProfile.csv");
 u.LoadUserCharacters("UserCharacters.csv");u.LoadUserEmotes("UserEmotes.csv");
 u.LoadUserWinLossStats("UserWinLossStats.csv");u.LoadUserBallons("UserBalloon.csv");u.LoadUserIcons("UserIcon.csv");
}
int main(int argc, char** argv) {
 try {
  if(argc>1 && std::string(argv[1])=="crash") {
   CsvStore::Images images{{"UserProfile.csv","new-profile\n"},{"UserCharacters.csv","new-characters\n"}};
   REQUIRE(CsvStore::Replace("users.redo",CsvStore::Encode(images)));
   REQUIRE(CsvStore::Replace("UserProfile.csv",images.at("UserProfile.csv")));
   ExitProcess(73); // Simulated process death after first of two durable replacements.
  }
  if(argc>1 && std::string(argv[1])=="recover") {
   CsvStore::Recover(); REQUIRE(CsvStore::Read("UserProfile.csv")=="new-profile\n");
   REQUIRE(CsvStore::Read("UserCharacters.csv")=="new-characters\n");
   REQUIRE(GetFileAttributesA("users.redo")==INVALID_FILE_ATTRIBUTES);
   std::cout<<"PASS crash replay completes both files\n";return 0;
  }
  Server server; UserManager users(server); Load(users);
  std::atomic<int> committed{0}; std::vector<std::thread> threads;
  for(int t=0;t<8;++t) threads.emplace_back([&,t] {
    for(int i=0;i<12;++i) {
      UserManager::Transaction tx(users);
      auto& profile=users.GetUserProfileById("u"+std::to_string(t%4));
      profile.money0+=1; users.SaveUserProfile(profile.id);
      users.UpdateWinLoss(profile.id,true,0); users.SaveUserWinLossStats(profile.id);
      REQUIRE(tx.Commit()); ++committed;
    }
  });
  for(auto& t:threads)t.join(); REQUIRE(committed==96);
  UserManager disk(server);Load(disk);
  for(int i=0;i<4;++i) {
    auto guard=disk.LockState();const auto id="u"+std::to_string(i);
    REQUIRE(disk.GetUserProfileById(id).money0==100024);
    REQUIRE(disk.GetUserWinLossStatsById(id).winCount==24);
  }
  std::cout<<"PASS 96 concurrent profile/stats commits, no lost updates\n";
  const auto before=CsvStore::Read("UserProfile.csv");
  REQUIRE(CreateDirectoryA("users.redo",nullptr)); // Force failure before durable commit point.
  { UserManager::Transaction tx(users); users.GetUserProfileById("u0").money0=0;
    users.SaveUserProfile("u0"); REQUIRE(!tx.Commit()); }
  REQUIRE(RemoveDirectoryA("users.redo"));
  {auto guard=users.LockState();REQUIRE(users.GetUserProfileById("u0").money0==100024);}
  REQUIRE(CsvStore::Read("UserProfile.csv")==before);
  std::cout<<"PASS precommit I/O failure rolls memory back and retains original CSV\n";
  std::atomic<int> purchases{0};threads.clear();
  for(int i=0;i<16;++i)threads.emplace_back([&] {
    UserManager::Transaction tx(users);
    auto& profile=users.GetUserProfileById("u0");auto& items=users.GetUserCharactersById("u0");
    if(items.char1)return;
    profile.money0-=5000;items.char1=1;users.SaveUserProfile("u0");users.SaveUserCharacters("u0");
    REQUIRE(tx.Commit());++purchases;
  });
  for(auto& t:threads)t.join();REQUIRE(purchases==1);
  {auto guard=users.LockState();REQUIRE(users.GetUserProfileById("u0").money0==95024);}
  Load(disk);{auto guard=disk.LockState();REQUIRE(disk.GetUserCharactersById("u0").char1==1);REQUIRE(disk.GetUserProfileById("u0").money0==95024);}
  std::cout<<"PASS 16 simultaneous purchase attempts charge and grant once\n";
  REQUIRE(users.RegisterUser("new","synthetic","newnick")=="REGISTER_SUCCESS|\n");
  Load(disk);REQUIRE(disk.CheckLogin("new","synthetic").find("LOGIN_SUCCESS|")==0);
  for(const auto& name:{"UserProfile.csv","UserCharacters.csv","UserEmotes.csv","UserWinLossStats.csv","UserBalloon.csv","UserIcon.csv"}) REQUIRE(CsvStore::Read(name).find("new,")!=std::string::npos);
  std::cout<<"PASS registration commits all seven files\n";
  REQUIRE(CsvStore::Replace("users.redo","BNBCSV1\n../escape.csv\n1\nx"));
  bool rejected=false;try {CsvStore::Recover();}catch(...){rejected=true;}REQUIRE(rejected);REQUIRE(DeleteFileA("users.redo"));
  std::cout<<"PASS malformed journal rejected\n";
  return 0;
 } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
