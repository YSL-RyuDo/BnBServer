#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <string>
#include <stdexcept>
#pragma comment(lib, "bcrypt.lib")

// CNG performs all cryptography. v1 pins parameters; unknown formats fail closed.
namespace PasswordHash {
constexpr ULONG Iterations = 600000;
constexpr size_t SaltBytes = 16, HashBytes = 32, MaxFieldBytes = 4096;
inline const std::string& Prefix() {
    static const std::string value = "$pbkdf2-sha256$600000$";
    return value;
}
inline bool ValidField(const std::string& value, bool allowEmpty = false) {
    if ((!allowEmpty && value.empty()) || value.size() > MaxFieldBytes) return false;
    for (unsigned char c : value) if (c < 32 || c == 127 || c == ',' || c == '|') return false;
    return true;
}
inline void Clear(std::string& value) {
    if (!value.empty()) SecureZeroMemory(&value[0], value.size());
    value.clear();
}
inline int HexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
template<size_t N> bool Decode(const std::string& text, std::array<UCHAR,N>& bytes) {
    if (text.size() != N * 2) return false;
    for (size_t i=0; i<N; ++i) {
        const int a=HexDigit(text[2*i]), b=HexDigit(text[2*i+1]);
        if (a<0 || b<0) return false;
        bytes[i]=static_cast<UCHAR>((a<<4)|b);
    }
    return true;
}
template<size_t N> std::string Encode(const std::array<UCHAR,N>& bytes) {
    static const char digits[]="0123456789abcdef";
    std::string text; text.reserve(N*2);
    for (auto b:bytes) { text.push_back(digits[b>>4]); text.push_back(digits[b&15]); }
    return text;
}
inline bool Parse(const std::string& encoded, std::array<UCHAR,SaltBytes>& salt,
                  std::array<UCHAR,HashBytes>& hash) {
    const size_t offset=Prefix().size();
    return encoded.size()==offset+SaltBytes*2+1+HashBytes*2 &&
        encoded.compare(0,offset,Prefix())==0 && encoded[offset+SaltBytes*2]=='$' &&
        Decode(encoded.substr(offset,SaltBytes*2),salt) && Decode(encoded.substr(offset+SaltBytes*2+1),hash);
}
inline bool IsValid(const std::string& encoded) {
    std::array<UCHAR,SaltBytes> salt{}; std::array<UCHAR,HashBytes> hash{};
    return Parse(encoded,salt,hash);
}
inline void Derive(const std::string& password, std::array<UCHAR,SaltBytes>& salt,
                   std::array<UCHAR,HashBytes>& hash) {
    // Bound expensive KDF work without waiting while holding gameplay state locks.
    static std::atomic<unsigned> active{0};
    struct Slot {
        std::atomic<unsigned>& count;
        explicit Slot(std::atomic<unsigned>& c):count(c) {
            if (count.fetch_add(1)>=2) { --count; throw std::runtime_error("Authentication busy"); }
        }
        ~Slot() { --count; }
    } slot(active);
    if (password.size()>MaxFieldBytes) throw std::runtime_error("Invalid credential length");
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0)
        throw std::runtime_error("Password provider unavailable");
    const NTSTATUS status=BCryptDeriveKeyPBKDF2(algorithm,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),static_cast<ULONG>(password.size()),
        salt.data(),static_cast<ULONG>(salt.size()),Iterations,hash.data(),static_cast<ULONG>(hash.size()),0);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if (status<0) { SecureZeroMemory(hash.data(),hash.size()); throw std::runtime_error("Password derivation failed"); }
}
inline std::string Create(const std::string& password) {
    std::array<UCHAR,SaltBytes> salt{}; std::array<UCHAR,HashBytes> hash{};
    if (BCryptGenRandom(nullptr,salt.data(),static_cast<ULONG>(salt.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)
        throw std::runtime_error("Password randomness unavailable");
    Derive(password,salt,hash);
    auto result=Prefix()+Encode(salt)+"$"+Encode(hash);
    SecureZeroMemory(hash.data(),hash.size());
    return result;
}
inline bool Verify(const std::string& password, const std::string& encoded) {
    std::array<UCHAR,SaltBytes> salt{};
    std::array<UCHAR,HashBytes> expected{},actual{};
    if (!Parse(encoded,salt,expected)) return false;
    Derive(password,salt,actual);
    volatile unsigned difference=0;
    for (size_t i=0;i<HashBytes;++i) difference |= actual[i]^expected[i];
    SecureZeroMemory(actual.data(),actual.size()); SecureZeroMemory(expected.data(),expected.size());
    return difference==0;
}
}
