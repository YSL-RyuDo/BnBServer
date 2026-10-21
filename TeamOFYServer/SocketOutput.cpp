#include "SocketOutput.h"
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace {
std::mutex registryMutex, statsFileMutex;
std::unordered_map<SOCKET, std::weak_ptr<SocketOutput>> registry;
}

SocketOutput::SocketOutput(SOCKET socket, SendFunction sender)
    : socket_(socket), sender_(sender ? std::move(sender) : SendFunction(::send)) {}

std::shared_ptr<SocketOutput> SocketOutput::Create(SOCKET socket, SendFunction sender) {
    auto output = std::shared_ptr<SocketOutput>(new SocketOutput(socket, std::move(sender)));
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        registry[socket] = output;
    }
    output->writer_ = std::thread([raw = output.get()] { raw->Run(); });
    return output;
}

void SocketOutput::Stop(int reason) {
    int unset = 0;
    stopReason_.compare_exchange_strong(unset, reason);
    if (!stopping_.exchange(true) && socket_ != INVALID_SOCKET)
        shutdown(socket_, SD_BOTH); // Unblock recv and a writer stalled in send.
    ready_.notify_all();
    idle_.notify_all();
}

SocketOutput::~SocketOutput() {
    Stop();
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        auto it = registry.find(socket_);
        if (it != registry.end() && it->second.expired()) registry.erase(it);
    }
    if (writer_.joinable()) writer_.join();
    if (socket_ != INVALID_SOCKET) closesocket(socket_);
    // Opt-in local experiment output, without account names or packet contents.
    char* path = nullptr;
    size_t pathSize = 0;
    _dupenv_s(&path, &pathSize, "BNB_TRANSPORT_STATS");
    std::unique_ptr<char, decltype(&std::free)> freePath(path, &std::free);
    if (path) {
        const auto s = Snapshot();
        std::lock_guard<std::mutex> lock(statsFileMutex);
        std::ofstream f(path, std::ios::app);
        f << "{\"accepted_frames\":" << s.acceptedFrames
          << ",\"completed_frames\":" << s.completedFrames
          << ",\"send_calls\":" << s.sendCalls << ",\"batches\":" << s.batches
          << ",\"sent_bytes\":" << s.sentBytes << ",\"peak_pending_bytes\":" << s.peakBytes
          << ",\"peak_pending_frames\":" << s.peakFrames
          << ",\"stop_reason\":" << s.stopReason << "}\n";
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
            ready_.notify_one();
            return length;
        }
    }
    // Do not wait for this peer while a caller may hold a room or client lock.
    Stop(3);
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
}

void SocketOutput::Run() {
    while (true) {
        std::string batch;
        size_t frames = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
            if (stopping_) break;
            do {
                batch += queue_.front(); queue_.pop_front(); ++frames;
            } while (!queue_.empty() && batch.size() + queue_.front().size() <= BatchBytes);
            ++stats_.batches;
        }
        size_t offset = 0;
        while (offset < batch.size() && !stopping_) {
            const int sent = sender_(socket_, batch.data() + offset, static_cast<int>(batch.size() - offset), 0);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++stats_.sendCalls;
                if (sent > 0) stats_.sentBytes += sent;
            }
            if (sent <= 0) { Stop(2); break; }
            offset += sent;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingBytes_ -= batch.size(); pendingFrames_ -= frames;
            if (offset == batch.size()) stats_.completedFrames += frames;
        }
        idle_.notify_all();
    }
}

bool SocketOutput::WaitIdle(unsigned milliseconds) {
    std::unique_lock<std::mutex> lock(mutex_);
    return idle_.wait_for(lock, std::chrono::milliseconds(milliseconds), [&] { return !pendingFrames_ || stopping_; }) && !pendingFrames_;
}

SocketOutput::Stats SocketOutput::Snapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = stats_; result.stopReason = stopReason_.load(); return result;
}

int QueueSend(SOCKET socket, const char* bytes, int length, int flags) {
    if (flags) { WSASetLastError(WSAEOPNOTSUPP); return SOCKET_ERROR; }
    std::shared_ptr<SocketOutput> output;
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        auto it = registry.find(socket);
        if (it != registry.end()) output = it->second.lock();
    }
    if (!output) { WSASetLastError(WSAENOTSOCK); return SOCKET_ERROR; }
    return output->Enqueue(bytes, length);
}
