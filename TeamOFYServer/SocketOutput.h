#pragma once
#include <winsock2.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// One writer owns each TCP byte stream. QueueSend reports acceptance, not delivery.
class SocketOutput {
public:
    using SendFunction = std::function<int(SOCKET, const char*, int, int)>;
    static constexpr size_t MaxPendingBytes = 256 * 1024;
    static constexpr size_t MaxPendingFrames = 4096;
    static constexpr size_t BatchBytes = 16 * 1024;
    struct Stats {
        size_t peakBytes = 0, peakFrames = 0;
        size_t acceptedFrames = 0, completedFrames = 0;
        size_t sendCalls = 0, batches = 0, sentBytes = 0;
        int stopReason = 0; // 1: owner/peer, 2: send failure, 3: queue limit
    };
    static std::shared_ptr<SocketOutput> Create(SOCKET socket, SendFunction sender = {});
    ~SocketOutput();
    int Enqueue(const char* bytes, int length);
    void Stop(int reason = 1);
    bool IsStopping() const { return stopping_.load(); }
    bool WaitIdle(unsigned milliseconds);
    Stats Snapshot();
private:
    SocketOutput(SOCKET socket, SendFunction sender);
    void Run();
    SOCKET socket_;
    SendFunction sender_;
    std::thread writer_;
    std::atomic<bool> stopping_{false};
    std::atomic<int> stopReason_{0};
    std::mutex mutex_;
    std::condition_variable ready_, idle_;
    std::deque<std::string> queue_;
    size_t pendingBytes_ = 0, pendingFrames_ = 0;
    Stats stats_;
};

int QueueSend(SOCKET socket, const char* bytes, int length, int flags = 0);
