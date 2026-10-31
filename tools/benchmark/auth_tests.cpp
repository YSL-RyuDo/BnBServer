#pragma comment(lib, "Ws2_32.lib")
#include "Server.h"
#include "PasswordHash.h"
#include <iostream>
#include <stdexcept>
#define REQUIRE(x) do { if (!(x)) throw std::runtime_error("Authentication assertion failed"); } while (0)
static void Load(UserManager& u) {
    u.LoadAccountUsers("UsersAccount.csv"); u.LoadUserProfiles("UserProfile.csv");
    u.LoadUserCharacters("UserCharacters.csv"); u.LoadUserEmotes("UserEmotes.csv");
    u.LoadUserWinLossStats("UserWinLossStats.csv"); u.LoadUserBallons("UserBalloon.csv"); u.LoadUserIcons("UserIcon.csv");
}
int main(int argc, char** argv) {
    try {
        Server server; UserManager users(server);
        if (argc > 1 && std::string(argv[1]) == "crash") {
            // Simulate termination after publishing a durable credential migration journal.
            const auto image = CsvStore::Read("UsersAccount.v2.csv");
            REQUIRE(CsvStore::Replace("users.redo", CsvStore::Encode({{"UsersAccount.v2.csv",image}})));
            REQUIRE(DeleteFileA("UsersAccount.v2.csv")); // Synthetic fixture only.
            ExitProcess(73);
        }
        if (argc > 1 && std::string(argv[1]) == "recover") {
            CsvStore::Recover(); Load(users);
            REQUIRE(users.CheckLogin("u0","synthetic-only") == "LOGIN_SUCCESS|u0,n0\n");
            REQUIRE(GetFileAttributesA("users.redo") == INVALID_FILE_ATTRIBUTES);
            std::cout << "PASS credential journal crash recovery\n"; return 0;
        }
        const auto legacy = CsvStore::Read("UsersAccount.csv");
        REQUIRE(CreateDirectoryA("users.redo", nullptr));
        bool rejected=false;
        try { users.LoadAccountUsers("UsersAccount.csv"); } catch (const std::runtime_error&) { rejected=true; }
        REQUIRE(rejected && GetFileAttributesA("UsersAccount.v2.csv") == INVALID_FILE_ATTRIBUTES);
        REQUIRE(CsvStore::Read("UsersAccount.csv") == legacy);
        REQUIRE(RemoveDirectoryA("users.redo"));
        std::cout << "PASS migration write failure preserves legacy and publishes no credentials\n";
        Load(users);
        const auto image=CsvStore::Read("UsersAccount.v2.csv");
        REQUIRE(CsvStore::Read("UsersAccount.csv") == legacy);
        REQUIRE(users.CheckLogin("u0","synthetic-only") == "LOGIN_SUCCESS|u0,n0\n");
        const auto encoded=PasswordHash::Create("synthetic-only");
        REQUIRE(PasswordHash::Verify("synthetic-only",encoded));
        REQUIRE(!PasswordHash::Verify("incorrect",encoded));
        REQUIRE(!PasswordHash::Verify(encoded,encoded));
        REQUIRE(!PasswordHash::IsValid("plaintext"));
        REQUIRE(!PasswordHash::IsValid(encoded+"$extra"));
        std::string future=encoded; future.replace(1,13,"unknown-scheme");
        REQUIRE(!PasswordHash::IsValid(future));
        std::cout << "PASS KDF verifies password, rejects wrong password, pass-the-hash and unknown formats\n";
        for (const auto& invalid : {std::string("id,password_hash,nickname\nu0,plaintext,n0\n"),
                std::string("unknown-header\n"),image + image.substr(image.find('\n')+1)}) {
            REQUIRE(CsvStore::Replace("UsersAccount.v2.csv",invalid));
            UserManager broken(server); rejected=false;
            try { broken.LoadAccountUsers("UsersAccount.csv"); } catch (const std::runtime_error&) { rejected=true; }
            REQUIRE(rejected && CsvStore::Read("UsersAccount.v2.csv") == invalid);
            REQUIRE(CsvStore::Read("UsersAccount.csv") == legacy);
        }
        REQUIRE(CsvStore::Replace("UsersAccount.v2.csv",image));
        std::cout << "PASS invalid hash, header and duplicate account fail closed without plaintext fallback\n";
        CsvStore::Images before;
        for (const auto& name : {"UsersAccount.v2.csv","UserProfile.csv","UserCharacters.csv","UserEmotes.csv",
                                "UserWinLossStats.csv","UserBalloon.csv","UserIcon.csv"}) before[name]=CsvStore::Read(name);
        REQUIRE(CreateDirectoryA("users.redo",nullptr));
        REQUIRE(users.RegisterUser("rollback","synthetic-new","rollbacknick") == "FILE_WRITE_ERROR|\n");
        REQUIRE(RemoveDirectoryA("users.redo"));
        REQUIRE(users.CheckLogin("rollback","synthetic-new") == "ID_NOT_FOUND|\n");
        for (const auto& entry:before) REQUIRE(CsvStore::Read(entry.first)==entry.second);
        REQUIRE(CsvStore::Read("UsersAccount.csv")==legacy);
        REQUIRE(users.RegisterUser("rollback","synthetic-new","rollbacknick") == "REGISTER_SUCCESS|\n");
        std::cout << "PASS failed registration rolls back all seven files and memory; retry succeeds\n";
        return 0;
    } catch (...) { std::cerr << "FAIL authentication test\n"; return 1; }
}
