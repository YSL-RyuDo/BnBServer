#include "SocketOutput.h"
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace {
std::mutex registryMutex, statsFileMutex;
std::unordered_map<SOCKET, std::weak_ptr<SocketOutput>> registry;
}

// The completion owns the connection and immutable bytes until the kernel is done.
struct OutputOperation : OVERLAPPED {
    std::shared_ptr<SocketOutput> owner;
    std::string bytes;
    size_t offset = 0, frames = 0;
    explicit OutputOperation(std::shared_ptr<SocketOutput> p)
        : OVERLAPPED{}, owner(std::move(p)) {}
};

class OutputDispatcher {
public:
    static OutputDispatcher& Instance() { static OutputDispatcher instance; return instance; }
    bool Attach(SOCKET socket) {
        return CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), port_, 0, 0) == port_;
    }
    bool Post(OutputOperation* operation, ULONG_PTR key) {
        return PostQueuedCompletionStatus(port_, 0, key, operation) != FALSE;
    }
    bool Start(OutputOperation* operation) {
        ++operations_;
        if (Post(operation, 1)) return true;
        --operations_; return false;
    }
    void Release(OutputOperation* operation) { delete operation; --operations_; }
private:
    HANDLE port_;
    std::thread worker_;
    std::atomic<size_t> operations_{0};
    OutputDispatcher() : port_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1)) {
        if (!port_) throw std::runtime_error("output completion port creation failed");
        worker_ = std::thread([this] {
            while (true) {
                DWORD bytes = 0; ULONG_PTR key = 0; OVERLAPPED* overlapped = nullptr;
                BOOL success = GetQueuedCompletionStatus(port_, &bytes, &key, &overlapped, INFINITE);
                if (!overlapped) { if (key == 2) break; else continue; }
                auto operation = static_cast<OutputOperation*>(overlapped);
                if (key == 1) operation->owner->Begin(operation);
                else if (key == 3) operation->owner->Submit(operation);
                else operation->owner->Complete(operation, bytes, success != FALSE);
            }
        });
    }
    ~OutputDispatcher() {
        std::vector<std::shared_ptr<SocketOutput>> live;
        { std::lock_guard<std::mutex> lock(registryMutex);
          for (const auto& entry : registry) if (auto p = entry.second.lock()) live.push_back(p); }
        for (const auto& p : live) p->Stop();
        while (operations_) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        PostQueuedCompletionStatus(port_, 0, 2, nullptr);
        worker_.join(); CloseHandle(port_);
    }
};

SocketOutput::SocketOutput(SOCKET socket, SendFunction sender)
    : socket_(socket), sender_(std::move(sender)) {}
std::shared_ptr<SocketOutput> SocketOutput::Create(SOCKET socket, SendFunction sender) {
    auto output = std::shared_ptr<SocketOutput>(new SocketOutput(socket, std::move(sender)));
    auto& dispatcher = OutputDispatcher::Instance();
    if (!output->sender_ && !dispatcher.Attach(socket)) output->Stop(2);
    { std::lock_guard<std::mutex> lock(registryMutex); registry[socket] = output; }
    return output;
}
void SocketOutput::Stop(int reason) {
    int unset = 0; stopReason_.compare_exchange_strong(unset, reason);
    if (!stopping_.exchange(true) && socket_ != INVALID_SOCKET) {
        shutdown(socket_, SD_BOTH);
        CancelIoEx(reinterpret_cast<HANDLE>(socket_), nullptr);
    }
    idle_.notify_all();
}
SocketOutput::~SocketOutput() {
    Stop();
    { std::lock_guard<std::mutex> lock(registryMutex);
      auto it = registry.find(socket_);
      if (it != registry.end() && it->second.expired()) registry.erase(it); }
    if (socket_ != INVALID_SOCKET) closesocket(socket_);
    char* path = nullptr; size_t pathSize = 0;
    _dupenv_s(&path, &pathSize, "BNB_TRANSPORT_STATS");
    std::unique_ptr<char, decltype(&std::free)> freePath(path, &std::free);
    if (path) {
        const auto s = Snapshot(); std::lock_guard<std::mutex> lock(statsFileMutex);
        std::ofstream f(path, std::ios::app);
        f << "{\"accepted_frames\":" << s.acceptedFrames << ",\"completed_frames\":" << s.completedFrames
          << ",\"send_calls\":" << s.sendCalls << ",\"batches\":" << s.batches
          << ",\"sent_bytes\":" << s.sentBytes << ",\"peak_pending_bytes\":" << s.peakBytes
          << ",\"peak_pending_frames\":" << s.peakFrames << ",\"stop_reason\":" << s.stopReason << "}\n";
    }
}
int SocketOutput::Enqueue(const char* bytes, int length) {
    if (length < 0 || (!bytes && length)) { WSASetLastError(WSAEINVAL); return SOCKET_ERROR; }
    if (!length) return 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) { WSASetLastError(WSAESHUTDOWN); return SOCKET_ERROR; }
        if (static_cast<size_t>(length) <= MaxPendingBytes - pendingBytes_ && pendingFrames_ < MaxPendingFrames) {
            queue_.emplace_back(bytes, static_cast<size_t>(length));
            pendingBytes_ += length; ++pendingFrames_; ++stats_.acceptedFrames;
            stats_.peakBytes = (std::max)(stats_.peakBytes, pendingBytes_);
            stats_.peakFrames = (std::max)(stats_.peakFrames, pendingFrames_);
            if (!active_) {
                active_ = true;
                auto operation = new OutputOperation(shared_from_this());
                if (!OutputDispatcher::Instance().Start(operation)) {
                    delete operation; active_ = false; Stop(2); return SOCKET_ERROR;
                }
            }
            return length;
        }
    }
    Stop(3); WSASetLastError(WSAENOBUFS); return SOCKET_ERROR;
}
void SocketOutput::Begin(OutputOperation* operation) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stopping_ && !queue_.empty()) {
            do {
                operation->bytes += queue_.front(); queue_.pop_front(); ++operation->frames;
            } while (!queue_.empty() && operation->bytes.size() + queue_.front().size() <= BatchBytes);
            ++stats_.batches;
        }
    }
    if (operation->bytes.empty() || stopping_) Complete(operation, 0, false);
    else Submit(operation);
}
void SocketOutput::Submit(OutputOperation* operation) {
    if (stopping_) { Complete(operation, 0, false); return; }
    { std::lock_guard<std::mutex> lock(mutex_); ++stats_.sendCalls; }
    if (sender_) {
        // Test seam only: same completion/accounting path, controlled short writes.
        const int n = sender_(socket_, operation->bytes.data() + operation->offset,
            static_cast<int>(operation->bytes.size() - operation->offset), 0);
        Complete(operation, n > 0 ? static_cast<DWORD>(n) : 0, n > 0);
        return;
    }
    *static_cast<OVERLAPPED*>(operation) = OVERLAPPED{};
    WSABUF buffer{static_cast<ULONG>(operation->bytes.size() - operation->offset),
        &operation->bytes[operation->offset]};
    const int status = WSASend(socket_, &buffer, 1, nullptr, 0, operation, nullptr);
    if (status == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        Complete(operation, 0, false);
    // Immediate success also queues a completion; do not complete twice.
}
void SocketOutput::Complete(OutputOperation* operation, DWORD bytes, bool success) {
    bool retry = false, next = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (success && bytes && bytes <= operation->bytes.size() - operation->offset) {
            operation->offset += bytes; stats_.sentBytes += bytes;
            retry = !stopping_ && operation->offset < operation->bytes.size();
        } else if (!stopping_) Stop(2);
        if (!retry) {
            pendingBytes_ -= operation->bytes.size(); pendingFrames_ -= operation->frames;
            if (operation->offset == operation->bytes.size() && operation->frames) stats_.completedFrames += operation->frames;
            if (stopping_) { queue_.clear(); pendingBytes_ = 0; pendingFrames_ = 0; }
            next = !stopping_ && !queue_.empty();
            if (!next) active_ = false;
        }
    }
    if (retry || next) {
        if (next) { operation->bytes.clear(); operation->offset = 0; operation->frames = 0; }
        // One batch per turn prevents one busy stream monopolizing the worker.
        if (!OutputDispatcher::Instance().Post(operation, retry ? 3 : 1)) {
            Stop(2); Complete(operation, 0, false);
        }
    } else {
        idle_.notify_all(); OutputDispatcher::Instance().Release(operation);
    }
}
bool SocketOutput::WaitIdle(unsigned milliseconds) {
    std::unique_lock<std::mutex> lock(mutex_);
    return idle_.wait_for(lock, std::chrono::milliseconds(milliseconds), [&] { return !active_; });
}
SocketOutput::Stats SocketOutput::Snapshot() {
    std::lock_guard<std::mutex> lock(mutex_); auto result = stats_; result.stopReason = stopReason_.load(); return result;
}
int QueueSend(SOCKET socket, const char* bytes, int length, int flags) {
    if (flags) { WSASetLastError(WSAEOPNOTSUPP); return SOCKET_ERROR; }
    std::shared_ptr<SocketOutput> output;
    { std::lock_guard<std::mutex> lock(registryMutex);
      auto it = registry.find(socket); if (it != registry.end()) output = it->second.lock(); }
    if (!output) { WSASetLastError(WSAENOTSOCK); return SOCKET_ERROR; }
    return output->Enqueue(bytes, length);
}
