#pragma once
#include <iostream>
#include "Utils.h"
class UserAccount
{
public:
    string id;
    string password;
    string nickname;
};


class UserProfile
{
public:
    string id = "";
    int level = 1;
    float exp = 0.0f;
    int icon = 0;
    int money0 = 0;
    int money1 = 0;
    int emo0 = 0;
    int emo1 = 1;
    int emo2 = 2;
    int emo3 = 3;
    int balloon = 0;
    UserProfile() = default;
};

class UserCharacters
{
public:
    string id = "";
    int char0 = 1;
    int char1 = 1;
    int char2 = 1;
    int char3 = 1;
    int char4 = 1;
    int char5 = 1;
    int char6 = 1;

    UserCharacters() = default;
};

class UserCharacterEmotes
{
public:
    string id = "";
    int emo0 = 1;
    int emo1 = 1;
    int emo2 = 1;
    int emo3 = 1;

    UserCharacterEmotes() = default;
};

class UserWinLossStats
{
public:
    string id = "";
    int winCount = 0;
    int loseCount = 0;
    int char0_win = 0;
    int char0_lose = 0;
    int char1_win = 0;
    int char1_lose = 0;
    int char2_win = 0;
    int char2_lose = 0;
    int char3_win = 0;
    int char3_lose = 0;
    int char4_win = 0;
    int char4_lose = 0;
    int char5_win = 0;
    int char5_lose = 0;
    int char6_win = 0;
    int char6_lose = 0;

    UserWinLossStats() = default;
};

class UserBallon
{
public:
    string id = "";
    int balloon0 = 1;
    int balloon1 = 1;
    int balloon2 = 1;
    int balloon3 = 1;
};