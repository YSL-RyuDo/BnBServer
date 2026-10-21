# Slap Squad / C++ Server

[![English](https://ysl-ryudo.github.io/assets/diagrams/language-en-current.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja.svg)](README.ja.md)

Seven weapon behaviors, multiplayer rooms and a C++ game server.

March 2025 to February 2026, 2 programmers and 6 graphics team members

I implemented the C++ server and Unity weapon behaviors, and connected movement, balloon interactions, match results and purchase feedback.

[Portfolio](https://ysl-ryudo.github.io/en/work/bnb/), [Full engineering notes](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.md), [Unity](https://github.com/YSL-RyuDo/BNB), [C++ server](https://github.com/YSL-RyuDo/BnBServer)

[![Slap Squad, four-player gameplay](https://ysl-ryudo.github.io/assets/evidence/slap-four-balloons.webp)](https://ysl-ryudo.github.io/en/work/bnb/#gameplay)

## Reducing output-thread cost without losing slow-reader isolation

At 64 connections, the median observed peak thread count fell from 132 to 69, and server CPU from 12.03% to 7.34% of one logical core. Median peak RSS fell from 16.03 to 14.49 MiB. The p95 median increased from 14.81 to 15.35 ms. This is a reduction in resource cost, not a claim that every response became faster.

| Connections | Threads, before → after | CPU %, before → after | p95 ms, before → after | Peak RSS MiB, before → after |
| --- | --- | --- | --- | --- |
| 8 | 20 → 13 | 1.87 → 1.72 | 12.41 → 15.31 | 11.65 → 11.45 |
| 32 | 68 → 37 | 11.09 → 4.06 | 13.53 → 14.16 | 13.60 → 12.83 |
| 64 | 132 → 69 | 12.03 → 7.34 | 14.81 → 15.35 | 16.03 → 14.49 |

Same Windows machine, Release x64, TCP loopback, 4 clients per room, MOVE at 20 Hz per client. Each variant has three runs per connection count, with 2 s warmup, 10 s measurement and 2 s drain. Variant order alternates. Values below are medians of per-run values; p95 is the median of run p95s. CPU 100% means one logical core. These populated waiting-room tests exclude Unity rendering, WAN traffic and full battle simulation.

GetAttackByIndex reopened and parsed CharStat.csv for every HIT. The server now validates and loads the seven-entry attack table before accepting connections, then reads an immutable in-memory map. Unknown indices still return zero. Invalid startup data fails loading; changes to the file require a restart. This is a deliberate startup-snapshot policy, not hot reload or a database migration.

Actual UserManager code, 100,000 lookups, five alternating runs per variant, matching checksums. Warm filesystem cache; initial loading is excluded. Median elapsed time: 9044.14 ms → 0.2805 ms. This isolates file-open/parsing work versus a tiny hot map. It must not be presented as an equivalent speedup in the game or server throughput.

[Implementation and reproduction](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/IOCP.md), [Raw measurements](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21-iocp), [Code audit and remaining work](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/AUDIT.md)


## Earlier fix: isolating a stalled reader

Earlier stage, one blocking writer per connection. The current implementation is the IOCP version above. These original measurements are retained as history.

With a stopped reader in another room: 0/300 responses before, 300/300 after in each of three runs. Bounded queues, serialized partial writes and tested connection cleanup.

| Condition / metric | Before, room-scoped blocking send | After, bounded output |
| --- | --- | --- |
| Stopped reader, healthy-room deliveries per run | 0 / 300 | 300 / 300 |
| Stopped reader, median run p95 | Undefined, no responses | 13.82 ms |
| Quiet, healthy-room deliveries per run | 300 / 300 | 300 / 300 |
| Quiet, median run p95 | 14.00 ms | 13.62 ms |
| Observed peak threads, 8 connections | 12 | 20 |

[Output queue design and reproduction](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/OUTPUT_QUEUE.md), [12-run raw data and build provenance](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21-output-queue), [Git diff](https://github.com/YSL-RyuDo/BnBServer/compare/737a92c397adb4320cf439d99d148362ce1ca090...9e9110727ff5fa8c4cd016f9a55efe4aa93ecc4e)


## Engineering evidence

October 2026 follow-up: room delivery and TCP framing fixes, IOCP output, startup attack-table cache, UML, reproducible measurements and gameplay clips.

The original MOVE handler called BroadcastMessageExcept, which sent to every connected client except the sender. With 64 clients in 16 four-player rooms, each move could reach 63 clients although only three were intended recipients. This was a correctness bug as well as unnecessary work.

At 64 connections, median p95 changed from 4,381.75 ms to 19.91 ms. Received application payload changed from 27.30 MiB to 1.30 MiB per run, approximately 95.2% less. This is explained by restricting each update from up to 63 recipients to three, not by a new network stack.

Latency is measured from immediately before the sender writes a request until another client receives the complete response line. It includes the load generator, TCP and OS scheduling. These are populated waiting rooms exercising the actual MOVE handler, not full game simulations or an Internet concurrency limit.

| Clients / rooms | Original p95 | Patched p95 | Original CPU | Patched CPU | Patched missing / cross-room |
| --- | --- | --- | --- | --- | --- |
| 4 / 1 | 12.22 ms | 13.42 ms | 1.09% | 1.09% | 0 / 0 |
| 16 / 4 | 12.22 ms | 11.78 ms | 7.82% | 2.81% | 0 / 0 |
| 32 / 8 | 28.77 ms | 15.90 ms | 42.95% | 5.77% | 0 / 0 |
| 64 / 16 | 4,381.75 ms | 19.91 ms | 99.10% | 17.94% | 0 / 0 |

Median of three per-run values. CPU: 100% equals one logical core. Missing and cross-room counts are totals across all three patched runs.

[Raw measurements and environment](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21), [Source change](https://github.com/YSL-RyuDo/BnBServer/commit/737a92c397adb4320cf439d99d148362ce1ca090), [Reproduce the experiment](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.md#reproduction)

![UML](https://ysl-ryudo.github.io/assets/slap-squad/server-classes-en.svg)

## What I built across the game

I built the C++ server and seven Unity weapon behaviors, and worked on movement, balloon and map interactions, match results and network integration. In the shared shop UI, I changed ownership updates to follow the returned item data.


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



[CSV persistence and confirmed purchase feedback](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.md), [Gameplay recordings and screenshots](https://ysl-ryudo.github.io/en/work/bnb/#more-demos)
