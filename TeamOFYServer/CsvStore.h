#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <atomic>
#include <cstdint>

// One server process owns the data directory. Call under the UserManager state lock.
class CsvStore {
public:
    class DirectoryLease {
        HANDLE handle_;
    public:
        DirectoryLease() : handle_(CreateFileA("users.lock", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)) {
            if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("User data directory is already owned or inaccessible");
        }
        ~DirectoryLease() { CloseHandle(handle_); }
        DirectoryLease(const DirectoryLease&) = delete;
        DirectoryLease& operator=(const DirectoryLease&) = delete;
    };
    using Images = std::map<std::string, std::string>;
    static bool Allowed(const std::string& name) {
        static const Images names{{"UsersAccount.csv", ""}, {"UserProfile.csv", ""},
            {"UserCharacters.csv", ""}, {"UserEmotes.csv", ""}, {"UserWinLossStats.csv", ""},
            {"UserBalloon.csv", ""}, {"UserIcon.csv", ""}};
        return names.count(name) != 0;
    }
    static std::string Read(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("CSV read failed: " + path);
        std::ostringstream s; s << f.rdbuf();
        if (f.bad()) throw std::runtime_error("CSV read failed: " + path);
        return s.str();
    }
    static bool Replace(const std::string& path, const std::string& bytes) {
        static std::atomic<unsigned long> sequence{0};
        const auto temporary = path + ".stage." + std::to_string(GetCurrentProcessId()) + "." + std::to_string(++sequence);
        HANDLE h = CreateFileA(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        size_t offset = 0; bool ok = true;
        while (offset < bytes.size()) {
            DWORD written = 0;
            const DWORD count = static_cast<DWORD>((std::min)(bytes.size() - offset, size_t(1024 * 1024)));
            if (!WriteFile(h, bytes.data() + offset, count, &written, nullptr) || !written) { ok = false; break; }
            offset += written;
        }
        if (ok) ok = FlushFileBuffers(h) != FALSE;
        if (!CloseHandle(h)) ok = false;
        if (ok) ok = MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        if (!ok) DeleteFileA(temporary.c_str());
        return ok;
    }
    static std::string Encode(const Images& images) {
        std::ostringstream out(std::ios::binary);
        out << "BNBCSV1\n";
        for (const auto& entry : images) {
            if (!Allowed(entry.first)) throw std::runtime_error("Invalid CSV target");
            out << entry.first << '\n' << entry.second.size() << '\n';
            out.write(entry.second.data(), entry.second.size());
        }
        return out.str();
    }
    static Images Decode(const std::string& bytes) {
        std::istringstream in(bytes, std::ios::binary); std::string line; Images result;
        if (!std::getline(in, line) || line != "BNBCSV1") throw std::runtime_error("Invalid CSV journal");
        while (std::getline(in, line)) {
            const auto name = line;
            if (!Allowed(name) || result.count(name) || !std::getline(in, line)) throw std::runtime_error("Invalid CSV journal target");
            size_t used = 0; const auto length = std::stoull(line, &used);
            if (used != line.size() || length > 64 * 1024 * 1024) throw std::runtime_error("Invalid CSV journal length");
            std::string contents(static_cast<size_t>(length), '\0');
            if (!contents.empty()) in.read(&contents[0], contents.size());
            if (static_cast<size_t>(in.gcount()) != contents.size()) throw std::runtime_error("Incomplete CSV journal");
            result.emplace(name, std::move(contents));
        }
        if (result.empty()) throw std::runtime_error("Empty CSV journal");
        return result;
    }
    static void Recover() {
        if (GetFileAttributesA("users.redo") == INVALID_FILE_ATTRIBUTES) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) return;
            throw std::runtime_error("Cannot inspect CSV journal");
        }
        const auto images = Decode(Read("users.redo"));
        for (const auto& entry : images)
            if (!Replace(entry.first, entry.second)) throw std::runtime_error("CSV recovery failed: " + entry.first);
        if (!DeleteFileA("users.redo")) throw std::runtime_error("Cannot retire CSV journal");
    }
    static bool Commit(const Images& images) {
        if (images.empty()) return true;
        // The durable journal is the commit point. After it exists, recovery must finish.
        if (!Replace("users.redo", Encode(images))) return false;
        try { Recover(); }
        catch (...) {
            // Do not acknowledge or serve stale memory after a partially applied commit.
            // Leave users.redo for startup recovery and let the process supervisor restart.
            std::terminate();
        }
        return true;
    }
};

