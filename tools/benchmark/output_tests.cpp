#include "SocketOutput.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#pragma comment(lib, "Ws2_32.lib")
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
    try {
        { // Force every send to accept at most 7 bytes, regardless of socket buffering.
            std::string actual, expected;
            auto q = SocketOutput::Create(INVALID_SOCKET, [&](SOCKET, const char* p, int n, int) {
                int count = (std::min)(7, n); actual.append(p, count); return count;
            });
            for (int i = 0; i < 100; ++i) { auto s = "frame:" + std::to_string(i) + "\n"; expected += s; check(q->Enqueue(s.data(), (int)s.size()) > 0, "enqueue"); }
            check(q->WaitIdle(2000), "partial-write timeout");
            check(actual == expected, "partial writes lost or reordered bytes");
            check(q->Snapshot().completedFrames == 100, "completion accounting");
            std::cout << "PASS partial_writes, frames=100, calls=" << q->Snapshot().sendCalls << "\n";
        }
        { // Four producers share one stream, with deliberately short writes.
            std::string stream;
            auto q = SocketOutput::Create(INVALID_SOCKET, [&](SOCKET, const char* p, int n, int) { int k = (std::min)(3,n); stream.append(p,k); return k; });
            std::vector<std::thread> producers;
            for (int t=0;t<4;++t) producers.emplace_back([&,t] { for(int i=0;i<100;++i) { auto s=std::to_string(t)+":"+std::to_string(i)+"\n"; q->Enqueue(s.data(),(int)s.size()); } });
            for(auto& t:producers)t.join();
            check(q->WaitIdle(2000), "producer timeout");
            std::istringstream input(stream);std::string line;int last[4]={-1,-1,-1,-1};int count=0;
            while(std::getline(input,line)) { const auto at=line.find(':');check(at!=std::string::npos,"torn frame");int t=std::stoi(line.substr(0,at)),i=std::stoi(line.substr(at+1));check(t>=0&&t<4&&i==last[t]+1,"producer order");last[t]=i;++count; }
            check(count==400,"missing concurrent frames");std::cout<<"PASS concurrent_producers, frames=400\n";
        }
        for (bool bytes : {true,false}) {
            std::mutex m;std::condition_variable cv;bool entered=false,release=false;
            auto q=SocketOutput::Create(INVALID_SOCKET,[&](SOCKET,const char*,int n,int) {std::unique_lock<std::mutex> l(m);entered=true;cv.notify_all();cv.wait(l,[&]{return release;});return n;});
            std::string first(bytes?16384:1,'x');q->Enqueue(first.data(),(int)first.size());
            {std::unique_lock<std::mutex> l(m);check(cv.wait_for(l,std::chrono::seconds(2),[&]{return entered;}),"writer not entered");}
            if(bytes) {std::string rest(SocketOutput::MaxPendingBytes-first.size(),'x');check(q->Enqueue(rest.data(),(int)rest.size())>0,"byte capacity");}
            else for(size_t i=1;i<SocketOutput::MaxPendingFrames;++i)check(q->Enqueue("x",1)>0,"frame capacity");
            check(q->Enqueue("x",1)==SOCKET_ERROR,"limit not enforced");
            const auto stats=q->Snapshot();check(stats.stopReason==3,"overflow policy");
            check(stats.peakBytes<=SocketOutput::MaxPendingBytes && stats.peakFrames<=SocketOutput::MaxPendingFrames,"unbounded backlog");
            {std::lock_guard<std::mutex> l(m);release=true;}cv.notify_all();
            std::cout<<"PASS "<<(bytes?"byte_limit":"frame_limit")<<", peak_bytes="<<stats.peakBytes<<", peak_frames="<<stats.peakFrames<<"\n";
        }
        { // Fill the queue behind a blocked first write, then observe batch call count.
            std::mutex m;std::condition_variable cv;bool entered=false,release=false;
            auto q=SocketOutput::Create(INVALID_SOCKET,[&](SOCKET,const char*,int n,int) {std::unique_lock<std::mutex> l(m);entered=true;cv.notify_all();cv.wait(l,[&]{return release;});return n;});
            q->Enqueue("x\n",2);
            {std::unique_lock<std::mutex> l(m);cv.wait(l,[&]{return entered;});}
            for(int i=0;i<999;++i)check(q->Enqueue("x\n",2)==2,"burst enqueue");
            {std::lock_guard<std::mutex> l(m);release=true;}cv.notify_all();
            check(q->WaitIdle(2000),"batch timeout");auto s=q->Snapshot();
            check(s.completedFrames==1000 && s.sentBytes==2000 && s.sendCalls==2,"batch coalescing");
            std::cout<<"PASS burst_batching, frames=1000, bytes=2000, calls="<<s.sendCalls<<" (injected sender, not a network benchmark)\n";
        }
        {
            auto q=SocketOutput::Create(INVALID_SOCKET,[](SOCKET,const char*,int,int){return SOCKET_ERROR;});q->Enqueue("x",1);
            q->WaitIdle(2000);check(q->IsStopping()&&q->Snapshot().stopReason==2,"send failure");check(q->Enqueue("x",1)==SOCKET_ERROR,"closed acceptance");
            std::cout<<"PASS send_failure\n";
        }
        std::cout<<"6/6 output tests passed\n";return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
