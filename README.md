# Slap Squad / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

A team multiplayer game connecting water-balloon combat, match rewards and an item shop.

| Project | Details |
|---|---|
| Status | Team project |
| Stack | C++, Winsock, C#, Unity |
| Period and team | Mar 2025 ~ Feb 2026, 8 members: 2 programmers, 6 graphics |
| My role | Server, network integration & shop/profile systems |

[Explore the case](https://ysl-ryudo.github.io/en/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/en/)

[![Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## My contribution

Worked on server packet handling and integration with the Unity game.

Implemented/updated shop packets, profile data handling and item/icon parsing.

Investigated reward accumulation and item pricing through the related CISC-W 2025 simulation study.

## Contribution evidence

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

Commits are evidence of the stated changes, not a percentage-of-authorship score. Third-party assets and teammates' work remain separately credited.

## Implementation and design

Rooms, in-game actions, rewards and purchases need a connected client/server flow. Building that loop raised a second question: how should item prices relate to the rewards players earn?

I implemented packet and game integration, room-leader/water-balloon updates, and shop/profile flows. The separate CISC-W study compares fixed and demand-adjusted pricing using simulated player behavior.

| Area | Implementation |
|---|---|
| Client | Water-balloon updates, purchase UI and result/reward flow |
| Transport | C++ Winsock; per-connection threads |
| State & storage | Accounts, rooms, shop and profiles backed by CSV data |
| Authority boundary | Some world updates originate from the room leader |

## Open and inspect

Open [TeamOFYServer.sln](TeamOFYServer.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

## Development and verification scope

The implementation uses connection threads and CSV-backed data. It is a learning/team-game server, not a benchmarked large-scale service. The research compares pricing policies; deployment of the experimental dynamic policy is not claimed. The paper and the research context are maintained in the Slap Squad client repository.

Presented the CNN pricing-policy research in “Designing Game Economies with Data.” Subsequently refined the research for the CISC-W 2025 poster presentation.

[CISC-W 2025](https://ysl-ryudo.github.io/en/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## Related repositories

[BNB](https://github.com/YSL-RyuDo/BNB), Public repository

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), Public repository

**CISC-W 2025: First author, poster presenter**

[2025 Winter Conference of the Korea Institute of Information Security and Cryptology](https://kiisc.or.kr/payment/pay/292)

27 ~ 28 Nov 2025, Konjiam Resort, Gwangju, Korea
