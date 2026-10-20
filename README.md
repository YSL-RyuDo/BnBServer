# Slap Squad / Server

<!-- portfolio-languages -->
[![English (current)](https://ysl-ryudo.github.io/assets/diagrams/language-en-current.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja.svg)](README.ja.md)
<!-- /portfolio-languages -->

A team multiplayer game connecting water-balloon combat, match rewards and an item shop.

| Project | Details |
|---|---|
| Status | Team project |
| Stack | C++, Winsock, C#, Unity |
| Period and team | Mar 2025 ~ Feb 2026, 8 members: 2 programmers, 6 graphics |
| My role | Gameplay, C++ server and client integration |

[Explore the case](https://ysl-ryudo.github.io/en/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/en/)

[![Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## Inside the game

[![Two clients join the same room and select characters before the host starts the match.](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)

Two clients join the same room and select characters before the host starts the match.

[![Local two-client play, with character state, water balloons, obstacles and damage updates.](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)

Local two-client play, with character state, water balloons, obstacles and damage updates.

## What I built across the game

I built the C++ server and seven Unity weapon behaviors, and worked on movement, balloon and map interactions, match results and network integration. In the shared shop UI, I changed ownership updates to follow the returned item data.

[![My implementation flow](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-en.svg)](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-en.svg)

### Seven weapon behaviors

Implemented Arrow, Laser, Mace, Melody, Pitchfork, Spell and Sword behaviors. WeaponSystem connects equipped weapon selection, spawning, cooldown and sound. This is gameplay implementation beyond the server's packet handling.

[WeaponSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/WeaponSystem.cs), [My change c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Arrow.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Arrow.cs), [My change c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Sword.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Sword.cs), [My change c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### Player control and attack timing

Worked on movement synchronization and switching between weapon attacks and balloon placement. Input is sampled in Update and Rigidbody movement runs in FixedUpdate. Attack requests carry the selected weapon and attack transform; laser requests also include a distance calculated against map obstacles.

[LocalPlayerController.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/LocalPlayerController.cs), [My change c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### Balloon and destructible-map interaction

Worked on balloon placement, cooldowns, overlapping placement checks and water propagation, together with map and block updates. The server relays world updates under the room-leader model.

[BalloonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/BalloonSystem.cs), [My change 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [MapSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/MapSystem.cs), [My change 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5)

### Room setup and scene transitions

Connected character choice, team setup and host-controlled starts to gameplay. NetworkConnector preserves character selection across transitions and handles match-related scene routing and cached state.

[RoomSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Room/RoomSystem.cs), [My change be42bb9](https://github.com/YSL-RyuDo/BNB/commit/be42bb99ced515ad702e9a15e9d95bd5e029839c), [NetworkConnector.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Network/NetworkConnector.cs), [My change c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### Match results, rewards and expression

Worked on player spawning, damage/death, winner and reward processing, and the ready-to-return flow in GameSystem. Implemented the emote request/display path and its cooldown. Result data is part of the play loop, not just a disconnected result screen.

[GameSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/GameSystem.cs), [My change eb6a766](https://github.com/YSL-RyuDo/BNB/commit/eb6a7660b4f690e0d46a7a3e75c50c744d40f745), [EmoticonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/EmoticonSystem.cs), [My change 576f512](https://github.com/YSL-RyuDo/BNB/commit/576f51272b078731e71e882afce04ce8ae164ac3)

### Account, economy and purchase feedback

Implemented server account, room, shop and profile processing with CSV persistence. In the shared ShopUI I removed premature ownership confirmation and refreshed the relevant item tab from returned data. The item-pricing question also led to the separate CNN simulation study.

[Server.cpp (main)](https://github.com/YSL-RyuDo/BnBServer/blob/d2a360c07276ec057a92d1361e8ff84de743520c/TeamOFYServer/Server.cpp), [My change e41da66](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7), [ShopUI.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Shop/ShopUI.cs), [My change 57cedd2](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

## Contribution evidence

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

## Implementation and design

Rooms, in-game actions, rewards and purchases need a connected client/server flow. Building that loop raised a second question: how should item prices relate to the rewards players earn?

I implemented packet and game integration, room-leader/water-balloon updates, and shop/profile flows. The separate CISC-W study compares fixed and demand-adjusted pricing using simulated player behavior.

## How the code works and why it is structured this way

### The purchase click is not proof of ownership

**Implementation**

In my ShopUI change, BuyItem still sends the selected purchase request and disables the button, but no longer immediately sets selectedOwned to true. When balloon, emote or icon data arrives, the corresponding setter refreshes that tab only if it is currently selected.

**Design rationale**

The client knows that the player clicked, but that alone does not establish that the server accepted the purchase. Waiting for returned item data avoids showing speculative ownership as confirmed. Checking the current tab also prevents a delayed response for one category from replacing another category's visible contents.

**Alternatives and the current tradeoff**

Optimistic ownership feedback is immediate, but needs a reliable rollback if the purchase fails. Waiting for server item data adds response delay while avoiding a premature ownership claim. This patch follows the latter approach. A pending indicator can preserve responsiveness without pretending that the transaction has finished.

**Tradeoffs and verification**

Waiting for returned item data avoids confirming an unaccepted purchase, but requires pending, failure and timeout states. VAIS later extends retry handling into the transaction layer.

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### Gameplay integration and the pricing question

**Implementation**

My multiplayer changes connect weapon actions, water-balloon updates and room-leader information to client network messages. The C++ server handles rooms and account/shop data with connection threads and CSV storage. Separately, deciding item prices for this game motivated my CNN-based consumption-pattern research, followed by I2P-CON presentation and a revised CISC-W poster.

**Design rationale**

Room-leader-driven world decisions reduce the amount of world simulation the server must own, but make that client's role part of the trust and disconnect model. Pricing is a different question: implementing a purchase does not tell us whether a price is appropriate. A simulation makes assumptions about rewards and consumption explicit so policies can be compared under stated conditions.

**Alternatives and the current tradeoff**

Full server simulation gives the server more control but requires server-side implementations of more world rules. Room-leader input keeps that server scope smaller while accepting client trust. For prices, a fixed policy is easier to explain; a demand-adjusted policy needs assumptions and stability checks. The study compares policies rather than proving that dynamic prices are always preferable.

**Tradeoffs and verification**

Room-leader decisions introduce a client-trust boundary and require handling leader departure. The pricing study compares simulated consumption under fixed and demand-based policies; its CNN metric is consumption-pattern classification accuracy.

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

## Open and inspect

Open [TeamOFYServer.sln](TeamOFYServer.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

Presented the CNN pricing-policy research in “Designing Game Economies with Data.” Subsequently refined the research for the CISC-W 2025 poster presentation.

[CISC-W 2025](https://ysl-ryudo.github.io/en/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## Related repositories

[BNB](https://github.com/YSL-RyuDo/BNB), Public repository

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), Public repository

**CISC-W 2025: First author, poster presenter**

[2025 Winter Conference of the Korea Institute of Information Security and Cryptology](https://kiisc.or.kr/payment/pay/292)

27 ~ 28 Nov 2025, Konjiam Resort, Gwangju, Korea
