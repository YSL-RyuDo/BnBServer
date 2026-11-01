#pragma comment(lib, "ws2_32.lib")
#include "CombatWorld.h"
#include "MapManager.h"
#include <iostream>
#include <stdexcept>

int passed=0;
void Check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);++passed;std::cout<<"PASS "<<name<<std::endl;}
CombatWorld World(int weapon=1,CombatWorld::Vec target={8,2},const std::string& name="Map1") {
    CombatWorld w;std::vector<std::vector<int>> grid(26,std::vector<int>(15,5));
    if(!w.Initialize(name,grid,1) || !w.AddPlayer("a","alpha",weapon,0,{2,2}) || !w.AddPlayer("b","beta",0,0,target))throw std::runtime_error("fixture");
    w.Ready("a");w.Ready("b");w.events.clear();return w;
}
void Advance(CombatWorld& w,double time,const CombatWorld::Damage& hit=[](const std::string&,const std::string&,bool){}) {
    for(int i=0;i<int(std::round(time*100));++i)w.Advance(.01,hit);
}
int main() {
    try {
        {auto w=World();w.started=false;Check(!w.Fire("a",1,90) && !w.Place("a",1) && !w.Input("a",1,{1,0}),"readiness barrier");}
        {auto w=World();Check(!w.Input("a",1,{1,1}) && !w.Input("a",1,{NAN,0}),"invalid direction rejected");
         for(int i=1;i<=1000;++i)w.Input("a",i,{1,0});Advance(w,.2);
         Check(std::abs(w.players.at("a").position.x-2.6)<.001,"input flood cannot accelerate time");
         Check(!w.Input("a",999,{-1,0}),"old movement sequence rejected");
         Advance(w,1);Check(w.players.at("a").position.x<2.76,"silent input expires after 250ms");}
        {auto w=World();w.map[15][4]=4;
         for(int i=1;i<=20;++i){w.Input("a",i,{1,0});Advance(w,.1);}
         Check(w.players.at("a").position.x<3.32,"movement cannot cross wall");}
        {auto w=World();w.Input("a",1,{1,0});w.Advance(10000,[](const std::string&,const std::string&,bool){});
         Check(w.players.at("a").position.x<=2.301,"bounded catch-up after stall");}
        {auto w=World();int hits=0;w.Fire("a",1,90);
         Check(!w.Fire("a",2,90) && !w.Fire("a",1,90),"attack cooldown and replay");
         Advance(w,.7,[&](const std::string& a,const std::string& b,bool water){if(a=="a"&&b=="b"&&!water)++hits;});
         Check(hits==1 && w.attacks.empty(),"server projectile hits exactly once without HIT report");}
        {auto w=World();w.map[15][5]=4;int hits=0;w.Fire("a",1,90);
         Advance(w,1,[&](const std::string&,const std::string&,bool){++hits;});
         Check(hits==0 && w.attacks.empty(),"arrow blocked by wall");}
        {auto w=World(2,{6,2});w.map[15][4]=4;int hits=0;w.Fire("a",1,90);
         Advance(w,1,[&](const std::string&,const std::string&,bool){++hits;});
         Check(hits==1,"spell preserves authored wall-passing behavior");}
        {auto w=World(6);w.map[15][5]=4;int hits=0;w.Fire("a",1,90);
         Check(w.attacks[0].laserLength<3,"laser range computed from server map");
         Advance(w,.4,[&](const std::string&,const std::string&,bool){++hits;});Check(!hits && w.attacks.empty(),"laser wall occlusion and 300ms lifetime");}
        for(int weapon=0;weapon<7;++weapon) {
            auto w=World(weapon,{3.5,2});int hits=0;w.Fire("a",1,90);
            Advance(w,1,[&](const std::string&,const std::string&,bool){++hits;});
            std::string name="authored weapon "+std::to_string(weapon)+" collision";Check(hits==1,name.c_str());
        }
        {auto w=World(4);w.map[15][4]=4;w.Fire("a",1,90);Advance(w,.4);
         Check(!w.attacks.empty() && w.attacks[0].direction.x<0 && w.attacks[0].speed==2.4,"melody server reflection and speed");
         Advance(w,11);Check(w.attacks.empty(),"reflected projectile lifetime bounded");}
        {auto w=World(4);w.map[15][3]=4;w.Fire("a",1,90);Advance(w,.1);
         Check(!w.attacks.empty() && w.attacks[0].direction.x<0 && w.attacks[0].position.x<2.26,"melody near-wall spawn reflects outside obstacle");}
        {auto w=World(4);w.map[15][3]=4;w.players.at("a").position.x=2.3;w.Fire("a",1,90);Advance(w,.1);
         Check(w.attacks.empty(),"overlapping projectile spawn cannot stick or cross wall");}
        {auto w=World(5);w.map[15][4]=4;w.map[23][12]=4;w.Fire("a",1,90);Advance(w,.3);
         Check(w.Tile(4,2)==5 && w.Tile(12,10)==4,"pitchfork destroys only contacted wall");}
        {auto w=World();w.map[15][3]=4;w.map[16][2]=3;int hits=0;
         Check(w.Place("a",1) && !w.Place("a",2),"balloon origin derived and cooldown enforced");
         Advance(w,2.99,[&](const std::string&,const std::string&,bool){++hits;});Check(w.balloons.size()==1 && w.waters.empty(),"balloon fuse not client-controlled");
         Advance(w,.02,[&](const std::string&,const std::string&,bool water){if(water)++hits;});
         Check(w.balloons.empty() && w.waters.size()==1 && hits==1,"server explosion damages silent stationary player");
         Check(w.Tile(2,3)==5 && w.Tile(3,2)==4,"water destroys block and stops at wall");
         bool beyond=false;for(auto c:w.waters[0].cells)if((c.x==4&&c.z==2)||(c.x==2&&c.z==4))beyond=true;
         Check(!beyond,"water cannot propagate through obstacle");
         Advance(w,2.1,[&](const std::string&,const std::string&,bool){++hits;});Check(w.waters.empty() && hits==1,"water expires without duplicate stay damage");}
        {auto w=World(1,{8,2},"Map2");w.map[16][2]=4;w.map[15][3]=6;w.Place("a",1);Advance(w,3.01);
         Check(w.Tile(2,3)==5 && w.Tile(3,2)==6,"Map2 block and wall types");}
        {auto w=World();w.Place("a",1);for(int i=1;i<=3;++i){w.Input("a",i,{0,1});Advance(w,.1);}
         for(int i=4;i<=10;++i){w.Input("a",i,{0,-1});Advance(w,.1);}
         Check(w.players.at("a").position.z>2.64,"owner may leave balloon but cannot walk back through it");}
        {MapManager loader;for(const char* name:{"Map1","Map2"}) {
            auto data=loader.LoadMapByNameWithSpawns(name);CombatWorld w;bool ok=w.Initialize(name,data.first,1)&&data.second.size()==4;
            for(size_t i=0;ok&&i<data.second.size();++i)ok=w.AddPlayer(std::to_string(i),"test",0,0,{double(data.second[i].first),double(data.second[i].second-13)});
            Check(ok,name);
         }Check(loader.LoadMapByNameWithSpawns("../Map1").first.empty(),"map path traversal rejected");}
        std::cout<<passed<<" checks passed\n";
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}
}
