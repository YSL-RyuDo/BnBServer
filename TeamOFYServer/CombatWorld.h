#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Deterministic planar simulation. No sockets, files, wall-clock reads or client hit claims.
// Dimensions/speeds come from the shipped Unity prefabs; see COLLISION_AUTHORITY.ko.md.
class CombatWorld {
public:
    struct Vec {
        double x=0,z=0;
        Vec()=default; Vec(double a,double b):x(a),z(b){}
        Vec operator+(Vec b)const{return {x+b.x,z+b.z};}
        Vec operator-(Vec b)const{return {x-b.x,z-b.z};}
        Vec operator*(double n)const{return {x*n,z*n};}
        double dot(Vec b)const{return x*b.x+z*b.z;}
        double length()const{return std::sqrt(dot(*this));}
    };
    struct Player {
        std::string nickname;
        Vec position,input;
        double heading=0,inputUntil=0,attackAt=0,balloonAt=0,waterAt=0;
        uint64_t moveSeq=0,attackSeq=0,balloonSeq=0;
        int weapon=0,balloonType=0;
        bool alive=true,ready=false;
    };
    struct Attack {
        uint64_t id=0;
        std::string owner;
        int weapon=0;
        Vec origin,position,direction;
        double yaw=0,pitch=0,born=0,expires=0,speed=0,laserLength=0;
        std::set<std::string> touched;
    };
    struct Balloon {
        uint64_t id=0; std::string owner; int x=0,z=0,type=0;
        double expires=0; bool ownerInside=true;
    };
    struct Water {
        uint64_t id=0; std::string owner; std::vector<Vec> cells;
        double expires=0; std::set<std::string> inside;
    };
    using Damage=std::function<void(const std::string&,const std::string&,bool)>;
    std::map<std::string,Player> players;
    std::vector<std::vector<int>> map;
    std::vector<Attack> attacks;
    std::vector<Balloon> balloons;
    std::vector<Water> waters;
    std::vector<std::string> events;
    std::string mapName;
    uint64_t matchId=0;
    double now=0;
    bool started=false;
    int width=0,height=0;

    bool Initialize(const std::string& name,const std::vector<std::vector<int>>& grid,uint64_t match) {
        if ((name!="Map1" && name!="Map2") || grid.size()!=26 || grid[0].size()!=15 || !match) return false;
        for(const auto& row:grid) {
            if(row.size()!=15)return false;
            for(int value:row)if(value<0 || value>(name=="Map1"?5:7))return false;
        }
        mapName=name;map=grid;width=15;height=13;matchId=match;
        now=nextSnapshot_=0;started=false;nextId_=0;
        players.clear();attacks.clear();balloons.clear();waters.clear();events.clear();return true;
    }
    bool AddPlayer(const std::string& id,const std::string& nick,int weapon,int balloon,Vec spawn) {
        if(players.size()>=4 || weapon<0 || weapon>6 || balloon<0 || balloon>9 || !Walkable(spawn,id))return false;
        Player p;p.nickname=nick;p.weapon=weapon;p.balloonType=balloon;p.position=spawn;
        return players.emplace(id,p).second;
    }
    bool Ready(const std::string& id) {
        auto it=players.find(id);if(it==players.end())return false;
        it->second.ready=true;
        if(!started && !players.empty() && std::all_of(players.begin(),players.end(),[](const std::pair<const std::string,Player>& p){return p.second.ready;})) {
            started=true;events.push_back("COMBAT_BEGIN|"+std::to_string(matchId)+"\n");
        }
        return true;
    }
    bool Input(const std::string& id,uint64_t sequence,Vec direction) {
        auto it=players.find(id);
        if(!started || it==players.end() || !it->second.alive || !sequence || sequence<=it->second.moveSeq ||
            !std::isfinite(direction.x) || !std::isfinite(direction.z) || direction.length()>1.00001)return false;
        it->second.moveSeq=sequence;it->second.input=direction;it->second.inputUntil=now+.25;
        if(direction.length()>.01)it->second.heading=Yaw(direction);
        return true;
    }
    bool Fire(const std::string& id,uint64_t sequence,double yaw) {
        auto p=players.find(id);
        if(!started || p==players.end() || !p->second.alive || !sequence || sequence<=p->second.attackSeq ||
            !std::isfinite(yaw) || yaw<0 || yaw>=360 || now+1e-8<p->second.attackAt || attacks.size()>=32)return false;
        p->second.attackSeq=sequence;p->second.attackAt=now+(p->second.weapon==6?2:1.5);
        Attack a;a.id=++nextId_;a.owner=id;a.weapon=p->second.weapon;a.yaw=yaw;a.direction=Forward(yaw);
        a.origin=p->second.position;a.position=a.origin+a.direction*(a.weapon==5?1.3:.8);a.born=now;
        const double life[]={.3,5,1.5,.5,1.5,.3,.3};a.expires=now+life[a.weapon];
        a.speed=a.weapon==1?10:(a.weapon==2?5:(a.weapon==4?3:0));
        // A projectile cannot be spawned through a nearby obstacle, even with a forward offset.
        if(a.weapon!=2) {
            Vec normal;double radius=a.weapon==1?.15117:(a.weapon==4?.35:0);
            double fraction=Trace(a.origin,a.position,radius,normal);
            if(fraction<1)a.position=a.origin+(a.position-a.origin)*(std::max)(0.0,fraction-.001);
        }
        if(a.weapon==6) {Vec normal;a.laserLength=15*Trace(a.position,a.position+a.direction*15,0,normal);}
        Pose(a);attacks.push_back(a);
        events.push_back("WEAPON_SPAWN|"+std::to_string(matchId)+"|"+std::to_string(a.id)+"|"+p->second.nickname+"|"+
            std::to_string(a.weapon)+"|"+XYZ(a.position,a.weapon==5?.4:.5)+"|"+Number(a.yaw)+","+Number(a.pitch)+"|"+Number(a.laserLength)+"\n");
        return true;
    }
    bool Place(const std::string& id,uint64_t sequence) {
        auto it=players.find(id);
        if(!started || it==players.end() || !it->second.alive || !sequence || sequence<=it->second.balloonSeq ||
            now+1e-8<it->second.balloonAt || balloons.size()>=16)return false;
        int x=static_cast<int>(std::nearbyint(it->second.position.x)),z=static_cast<int>(std::nearbyint(it->second.position.z));
        if(Solid(x,z))return false;
        for(const auto& b:balloons)if(b.x==x && b.z==z)return false;
        for(const auto& other:players)if(other.first!=id && other.second.alive && (other.second.position-Vec{double(x),double(z)}).length()<.8)return false;
        it->second.balloonSeq=sequence;it->second.balloonAt=now+3;
        Balloon b;b.id=++nextId_;b.owner=id;b.x=x;b.z=z;b.type=it->second.balloonType;b.expires=now+3;
        balloons.push_back(b);
        events.push_back("PLACE_BALLOON_RESULT|"+it->second.nickname+"|"+XY({double(x),double(z)})+"|"+std::to_string(b.type)+"\n");
        return true;
    }
    void RemovePlayer(const std::string& id) {
        auto p=players.find(id);if(p!=players.end()){p->second.alive=false;p->second.input={};p->second.ready=true;Ready(id);}
        // Keep ownership records until the match ends: already placed balloons still expire.
    }
    void Advance(double seconds,const Damage& damage) {
        if(!started || !std::isfinite(seconds) || seconds<=0)return;
        seconds=(std::min)(seconds,.1); // Bounded catch-up; never teleport after a stalled worker.
        while(seconds>1e-9) {double dt=(std::min)(seconds,.01);seconds-=dt;now+=dt;Step(dt,damage);}
        if(now+1e-8>=nextSnapshot_) {
            do { nextSnapshot_+=.05; } while(nextSnapshot_<=now+1e-8);
            for(const auto& p:players)if(p.second.alive)
                events.push_back("PLAYER_STATE|"+std::to_string(matchId)+"|"+p.second.nickname+"|"+XY(p.second.position)+"|"+std::to_string(p.second.moveSeq)+"\n");
            for(const auto& a:attacks)events.push_back("WEAPON_STATE|"+std::to_string(matchId)+"|"+std::to_string(a.id)+"|"+XYZ(a.position,a.weapon==5?.4:.5)+"|"+Number(a.yaw)+","+Number(a.pitch)+"\n");
        }
    }
    int Tile(int x,int z)const {return x<0 || z<0 || x>=width || z>=height ? -2 : map[z+height][x];}
    bool Solid(int x,int z)const {return Tile(x,z)!=5;}
    bool Block(int x,int z)const {int t=Tile(x,z);return t==3 || (mapName=="Map2" && t==4);}
    bool Wall(int x,int z)const {int t=Tile(x,z);return mapName=="Map1"?t==4:(t==6 || t==7);}
    static std::string Number(double n){std::ostringstream s;s.precision(8);s<<n;return s.str();}
    static std::string XY(Vec p){return Number(p.x)+","+Number(p.z);}
    static std::string XYZ(Vec p,double y=.5){return Number(p.x)+","+Number(y)+","+Number(p.z);}
private:
    uint64_t nextId_=0;double nextSnapshot_=0;
    static double Clamp(double v,double a,double b){return (std::max)(a,(std::min)(b,v));}
    static Vec Forward(double degrees){double r=degrees*3.14159265358979323846/180;return {std::sin(r),std::cos(r)};}
    static double Yaw(Vec direction){double y=std::atan2(direction.x,direction.z)*180/3.14159265358979323846;return y<0?y+360:y;}
    Vec HalfTile(int tile)const {
        if(mapName=="Map1")return tile==3?Vec{.37559,.37852}:Vec{.39149,.35155};
        if(tile==3)return {.47962,.48823};if(tile==4)return {.48349,.44750};return {.44901,.44604};
    }
    static bool CircleBox(Vec p,Vec c,Vec half,double radius) {
        Vec q{Clamp(p.x,c.x-half.x,c.x+half.x),Clamp(p.z,c.z-half.z,c.z+half.z)};
        return (p-q).dot(p-q)<=radius*radius+1e-12;
    }
    bool Walkable(Vec position,const std::string& id)const {
        if(!std::isfinite(position.x) || !std::isfinite(position.z) || position.x<-.2 || position.z<-.2 || position.x>width-.8 || position.z>height-.8)return false;
        for(int z=(std::max)(0,int(std::floor(position.z-1)));z<=(std::min)(height-1,int(std::ceil(position.z+1)));++z)
            for(int x=(std::max)(0,int(std::floor(position.x-1)));x<=(std::min)(width-1,int(std::ceil(position.x+1)));++x)
                if(Solid(x,z) && CircleBox(position,{double(x),double(z)},HalfTile(Tile(x,z)),.3))return false;
        for(const auto& b:balloons)if(!(b.owner==id && b.ownerInside) && (position-Vec{double(b.x),double(b.z)}).length()<.8)return false;
        for(const auto& p:players)if(p.first!=id && p.second.alive && (position-p.second.position).length()<.6-1e-8)return false;
        return true;
    }
    static double RayBox(Vec from,Vec to,Vec center,Vec half,Vec& normal) {
        Vec delta=to-from;double lo=0,hi=1;normal={};
        for(int axis=0;axis<2;++axis) {
            double v=axis?from.z:from.x,d=axis?delta.z:delta.x,c=axis?center.z:center.x,h=axis?half.z:half.x;
            if(std::abs(d)<1e-12){if(v<c-h || v>c+h)return 1;continue;}
            double entryTime=(c-h-v)/d,exitTime=(c+h-v)/d;double sign=-1;
            if(entryTime>exitTime){std::swap(entryTime,exitTime);sign=1;}
            if(entryTime>=lo){lo=entryTime;normal=axis?Vec{0,sign}:Vec{sign,0};}
            hi=(std::min)(hi,exitTime);if(lo>hi || hi<0 || lo>1)return 1;
        }
        return Clamp(lo,0,1);
    }
    double Trace(Vec from,Vec to,double radius,Vec& normal)const {
        double best=1;
        // Include the immutable outside border; no client-supplied object names are used.
        for(int z=-1;z<=height;++z)for(int x=-1;x<=width;++x)if(Solid(x,z)) {
            Vec n,half=(x<0 || z<0 || x==width || z==height)?Vec{.5,.5}:HalfTile(Tile(x,z));
            double t=RayBox(from,to,{double(x),double(z)},half+Vec{radius,radius},n);
            if(t<best){best=t;normal=n;}
        }
        return best;
    }
    static double RayCircle(Vec from,Vec to,Vec center,double radius) {
        Vec d=to-from,m=from-center;double a=d.dot(d),c=m.dot(m)-radius*radius;
        if(c<=0)return 0;if(a<1e-12)return 2;
        double b=m.dot(d),disc=b*b-a*c;if(disc<0)return 2;
        double t=(-b-std::sqrt(disc))/a;return t>=0 && t<=1?t:2;
    }
    static bool InBox(Vec point,Vec position,double yaw,double center,double halfX,double halfZ,double radius) {
        Vec f=Forward(yaw),right{f.z,-f.x},d=point-position;
        return CircleBox({d.dot(right),d.dot(f)},{0,center},{halfX,halfZ},radius);
    }
    static bool BoxesOverlap(Vec tile,Vec tileHalf,Vec position,double yaw,double center,double halfX,double halfZ) {
        Vec forward=Forward(yaw),right{forward.z,-forward.x},delta=tile-(position+forward*center);
        for(Vec axis:{Vec{1,0},Vec{0,1},forward,right}) {
            double tileRadius=tileHalf.x*std::abs(axis.x)+tileHalf.z*std::abs(axis.z);
            double weaponRadius=halfX*std::abs(axis.dot(right))+halfZ*std::abs(axis.dot(forward));
            if(std::abs(delta.dot(axis))>tileRadius+weaponRadius)return false;
        }
        return true;
    }
    void Pose(Attack& a) {
        double age=now-a.born;
        if(a.weapon==0) {a.position=a.origin+a.direction*.8;a.yaw=Yaw(a.direction)-60+120*Clamp(age/.3,0,1);}
        if(a.weapon==3) {
            auto p=players.find(a.owner);if(p!=players.end())a.origin=p->second.position;
            double angle=-720*age*3.14159265358979323846/180;
            a.position=a.origin+Vec{std::cos(angle),std::sin(angle)}*1.5;a.yaw=Yaw(a.origin-a.position);
        }
        if(a.weapon==5) {a.position=a.origin+a.direction*1.3;a.pitch=-90+90*Clamp(age/.3,0,1);}
    }
    void RemoveTile(int x,int z) {
        if(Tile(x,z)<0 || Tile(x,z)==5)return;
        map[z+height][x]=5;events.push_back("TILE_REMOVE|"+std::to_string(matchId)+"|"+std::to_string(x)+","+std::to_string(z)+"\n");
    }
    void Step(double dt,const Damage& damage) {
        for(auto& item:players) {
            auto& p=item.second;if(!p.alive)continue;
            Vec move=now<=p.inputUntil?p.input*(3*dt):Vec{};
            Vec candidate=p.position+Vec{move.x,0};if(Walkable(candidate,item.first))p.position=candidate;
            candidate=p.position+Vec{0,move.z};if(Walkable(candidate,item.first))p.position=candidate;
        }
        for(auto& b:balloons) {
            auto owner=players.find(b.owner);
            if(owner==players.end() || (owner->second.position-Vec{double(b.x),double(b.z)}).length()>=.8)b.ownerInside=false;
        }
        for(auto it=balloons.begin();it!=balloons.end();) {
            if(now+1e-8<it->expires){++it;continue;}
            Balloon b=*it;it=balloons.erase(it);Water w;w.id=b.id;w.owner=b.owner;w.expires=now+2;
            std::string nick=players.at(b.owner).nickname;
            events.push_back("REMOVE_BALLOON|"+nick+"|"+XY({double(b.x),double(b.z)})+"\n");
            w.cells.push_back({double(b.x),double(b.z)});
            for(Vec d:{Vec{1,0},Vec{-1,0},Vec{0,1},Vec{0,-1}})for(int n=1;n<=2;++n) {
                int x=b.x+int(d.x)*n,z=b.z+int(d.z)*n;
                if(Solid(x,z)){if(Block(x,z))RemoveTile(x,z);break;}
                w.cells.push_back({double(x),double(z)});
            }
            std::string msg="WATER_SPREAD|"+nick+"|"+XY({double(b.x),double(b.z)})+"|"+std::to_string(b.type)+"|";
            for(auto cell:w.cells)msg+=XY(cell)+";";msg.pop_back();events.push_back(msg+"\n");waters.push_back(w);
        }
        for(auto it=waters.begin();it!=waters.end();) {
            if(now+1e-8>=it->expires){it=waters.erase(it);continue;}
            for(auto& item:players) {
                auto& p=item.second;bool inside=p.alive && std::any_of(it->cells.begin(),it->cells.end(),[&](Vec c){return CircleBox(p.position,c,{.45,.45},.3);});
                if(inside && it->inside.insert(item.first).second && now+1e-8>=p.waterAt){p.waterAt=now+2;damage(item.first,item.first,true);}
                if(!inside)it->inside.erase(item.first);
            }
            ++it;
        }
        for(auto it=attacks.begin();it!=attacks.end();) {
            Attack& a=*it;bool remove=now>a.expires+1e-8 || now-a.born>10 || !players.at(a.owner).alive;
            if(!remove && (a.weapon==1 || a.weapon==2 || a.weapon==4)) {
                Vec end=a.position+a.direction*(a.speed*dt),normal;double wall=a.weapon==2?1:Trace(a.position,end,a.weapon==1?.15117:.35,normal);
                double first=2;std::string victim;
                for(const auto& p:players)if(p.first!=a.owner && p.second.alive) {
                    double t=RayCircle(a.position,end,p.second.position,.3+(a.weapon==1?.15117:(a.weapon==2?.27827:.35)));
                    if(t<first){first=t;victim=p.first;}
                }
                if(!victim.empty() && first<=wall){a.position=a.position+(end-a.position)*first;damage(a.owner,victim,false);remove=true;}
                else if(wall<1) {
                    a.position=a.position+(end-a.position)*wall+normal*.001;
                    if(a.weapon==4 && normal.length()>.5){a.direction=a.direction-normal*(2*a.direction.dot(normal));a.yaw=Yaw(a.direction);a.speed=2.4;a.expires=now+1.5;}
                    else remove=true;
                } else a.position=end;
                if(a.position.x<-.5 || a.position.x>width-.5 || a.position.z<-.5 || a.position.z>height-.5)remove=true;
            } else if(!remove) {
                Pose(a);
                double center=0,halfX=0,halfZ=0;
                if(a.weapon==0){center=2.1166012;halfX=1.4272887;halfZ=4.326109;}
                if(a.weapon==3){center=2.2715468;halfX=.37200525;halfZ=2.2715468;}
                if(a.weapon==5){double r=-a.pitch*3.14159265358979323846/180;center=.8892782*std::cos(r)+.30631492*std::sin(r);halfX=.5;halfZ=1.26621185*std::cos(r)+.40620965*std::sin(r);}
                if(a.weapon==6){center=a.laserLength/2;halfX=.1;halfZ=a.laserLength/2;}
                if(a.weapon==5)for(int z=0;z<height;++z)for(int x=0;x<width;++x)
                    if(Wall(x,z) && BoxesOverlap({double(x),double(z)},HalfTile(Tile(x,z)),a.position,a.yaw,center,halfX,halfZ))RemoveTile(x,z);
                for(const auto& p:players)if(p.first!=a.owner && p.second.alive && !a.touched.count(p.first) &&
                    InBox(p.second.position,a.position,a.yaw,center,halfX,halfZ,.3)) {
                    Vec normal;bool clear=a.weapon==6 || Trace(a.origin,p.second.position,0,normal)>=1;
                    if(clear){a.touched.insert(p.first);damage(a.owner,p.first,false);}
                }
            }
            if(remove){events.push_back("WEAPON_REMOVE|"+std::to_string(matchId)+"|"+std::to_string(a.id)+"\n");it=attacks.erase(it);}
            else ++it;
        }
    }
};
