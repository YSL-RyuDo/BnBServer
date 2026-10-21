#include "Server.h"
#include "UserManager.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <stdexcept>
#pragma comment(lib, "Ws2_32.lib")
void require(bool ok) { if (!ok) throw std::runtime_error("attack table check failed"); }
int main(int argc, char**) {
    Server server; UserManager users(server);
#ifdef CACHED_ATTACK_STATS
    require(users.LoadAttackStats("CharStat.csv"));
#endif
    for(int i=0;i<7;++i) require(users.GetAttackByIndex(i)==10+i);
    require(users.GetAttackByIndex(99)==0);
    if(argc>1) {
#ifdef CACHED_ATTACK_STATS
        std::ofstream("broken.csv") << "id,hp,attack\n0,100,invalid\n";
        require(!users.LoadAttackStats("broken.csv"));
        require(users.GetAttackByIndex(0)==10);
        require(!users.LoadAttackStats("missing.csv"));
        std::atomic<int> total{0};std::vector<std::thread> threads;
        for(int t=0;t<4;++t)threads.emplace_back([&]{int sum=0;for(int i=0;i<7000;++i)sum+=users.GetAttackByIndex(i%7);total+=sum;});
        for(auto& thread:threads)thread.join();require(total==364000);
        std::ofstream("CharStat.csv") << "CharIndex,Health,Attack\n0,100,999\n";
        require(users.GetAttackByIndex(0)==10);
        std::cout << "PASS seven values, unknown index, malformed/missing load, unchanged prior snapshot, concurrent reads, immutable startup snapshot\n";
#endif
        return 0;
    }
    const int calls=100000;volatile long long sum=0;
    auto start=std::chrono::steady_clock::now();
    for(int i=0;i<calls;++i)sum+=users.GetAttackByIndex(i%7);
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout << "{\"calls\":"<<calls<<",\"checksum\":"<<sum<<",\"elapsed_ms\":"<<ms<<"}\n";
}
