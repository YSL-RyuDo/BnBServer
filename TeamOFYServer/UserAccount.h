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
    int char1 = 0;
    int char2 = 0;
    int char3 = 0;
    int char4 = 0;
    int char5 = 0;
    int char6 = 0;

    UserCharacters() = default;
};

class UserCharacterEmotes
{
public:
    std::string id = "";
    int emo0 = 1;
    int emo1 = 1;
    int emo2 = 1;
    int emo3 = 1;
    int emo4 = 0;
    int emo5 = 0;
    int emo6 = 0;
    int emo7 = 0;
    int emo8 = 0;
    int emo9 = 0;
    int emo10 = 0;
    int emo11 = 0;
    int emo12 = 0;
    int emo13 = 0;
    int emo14 = 0;
    int emo15 = 0;
    int emo16 = 0;
    int emo17 = 0;
    int emo18 = 0;
    int emo19 = 0;
    int emo20 = 0;
    int emo21 = 0;
    int emo22 = 0;
    int emo23 = 0;
    int emo24 = 0;
    int emo25 = 0;
    int emo26 = 0;
    int emo27 = 0;
    int emo28 = 0;
    int emo29 = 0;
    int emo30 = 0;
    int emo31 = 0;
    int emo32 = 0;
    int emo33 = 0;
    int emo34 = 0;
    int emo35 = 0;

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

class UserBallons
{
public:
    string id = "";
    int balloon0 = 1;
    int balloon1 = 0;
    int balloon2 = 0;
    int balloon3 = 0;
    int balloon4 = 0;
    int balloon5 = 0;
    int balloon6 = 0;
    int balloon7 = 0;
    int balloon8 = 0;
    int balloon9 = 0;

    UserBallons() = default;
};

class UserIcons
{
public:
    string id = "";
    int icon0 = 1;
    int icon1 = 0;
    int icon2 = 0;
    int icon3 = 0;
    int icon4 = 0;
    int icon5 = 0;
    int icon6 = 0;
    int icon7 = 0;
    int icon8 = 0;
    int icon9 = 0;

    UserIcons() = default;
};