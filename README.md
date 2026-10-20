# BNB — Multiplayer Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

A Windows C++/Winsock server for the BNB team project, covering accounts, rooms, gameplay packets and the shop/reward loop.

## My contribution

- Worked on server packet handling and integration with the Unity game.
- Implemented/updated shop packets, profile data handling and item/icon parsing.
- Investigated reward accumulation and item pricing through the related CISC-W 2025 simulation study.

## Contribution evidence

- [Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)
- [Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)
- [Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

Commits are evidence of the stated changes, not a percentage-of-authorship score. Third-party assets and teammates' work remain separately credited.

## Open and inspect

Open [TeamOFYServer.sln](TeamOFYServer.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[BNB](https://github.com/YSL-RyuDo/BNB)

## Development and verification scope

The implementation uses connection threads and CSV-backed data. It is a learning/team-game server, not a benchmarked large-scale service. The research compares pricing policies; deployment of the experimental dynamic policy is not claimed. The paper and the research context are maintained in the BNB client repository.
