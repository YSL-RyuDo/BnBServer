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

## How the code works and why it is structured this way

These notes connect the inspected implementation to the constraints it addresses and its tradeoffs. Explanations of design consequences are code analysis, not a claim that every alternative was prototyped. Contribution links identify my changes within team projects; source links retain their original access permissions.

### The purchase click is not proof of ownership

**Implementation**

In my ShopUI change, BuyItem still sends the selected purchase request and disables the button, but no longer immediately sets selectedOwned to true. When balloon, emote or icon data arrives, the corresponding setter refreshes that tab only if it is currently selected.

**Design rationale**

The client knows that the player clicked, but that alone does not establish that the server accepted the purchase. Waiting for returned item data avoids showing speculative ownership as confirmed. Checking the current tab also prevents a delayed response for one category from replacing another category's visible contents.

**Alternatives and the current tradeoff**

Optimistic ownership feedback is immediate, but needs a reliable rollback if the purchase fails. Waiting for server item data adds response delay while avoiding a premature ownership claim. This patch follows the latter approach. A pending indicator can preserve responsiveness without pretending that the transaction has finished.

**Tradeoffs and verification**

This is a specific contribution inside shared client code, not sole authorship of ShopUI. Delaying ownership feedback makes failure and timeout handling important; the cited patch alone does not establish complete retry handling. The later VAIS purchase contract addresses retries at the transaction layer as well.

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### Gameplay integration and the pricing question

**Implementation**

My multiplayer changes connect weapon actions, water-balloon updates and room-leader information to client network messages. The C++ server handles rooms and account/shop data with connection threads and CSV storage. Separately, deciding item prices for this game motivated my CNN-based consumption-pattern research, followed by I2P-CON presentation and a revised CISC-W poster.

**Design rationale**

Room-leader-driven world decisions reduce the amount of world simulation the server must own, but make that client's role part of the trust and disconnect model. Pricing is a different question: implementing a purchase does not tell us whether a price is appropriate. A simulation makes assumptions about rewards and consumption explicit so policies can be compared under stated conditions.

**Alternatives and the current tradeoff**

Full server simulation gives the server more control but requires server-side implementations of more world rules. Room-leader input keeps that server scope smaller while accepting client trust. For prices, a fixed policy is easier to explain; a demand-adjusted policy needs assumptions and stability checks. The study compares policies rather than proving that dynamic prices are always preferable.

**Tradeoffs and verification**

The game is not described as a fully server-authoritative simulation. The paper's simulated result is not measured live-player behavior, and the research does not establish that a CNN pricing model was deployed in BnBServer. The implementation and experiment have separate evidence.

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

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
