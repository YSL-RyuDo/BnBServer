# Slap Squad, Remaining work

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

2026-10-21, [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

The real server purchase test sends two requests per account, with 32 accounts split across character, balloon, emote and icon purchases. Correct persisted balance and ownership: 0/32 before, 32/32 after, in this reproduction. The unit harness also checks 96 concurrent profile/stat commits, 16 simultaneous purchase attempts, seven-file registration, precommit failure rollback and recovery after process termination between two replacements. The room test completes 12 matches while another room moves, checks one reward and one GAME_END per match, and churns 30 temporary rooms.

The user-state mutex serializes account writes, and each transaction snapshots the user vectors and rewrites affected CSVs. This favors correctness and compatibility over high write throughput. These tests do not establish database-scale capacity, recovery from disk corruption, or match restoration after a server restart. Client-reported hits still need independent server validation. Full process shutdown and Unity allocation profiling remain separate work.

[Code and test data](STATE_SAFETY.md), [IOCP](IOCP.md), [Initial source inventory before the state-safety change](audit-source-inventory.json)

GameSystem.cs and NetworkConnector.cs copy lists and repeat object/component lookups. WeaponSystem.cs and BalloonSystem.cs instantiate and destroy effects and search scenes. The next step is to measure allocations and frame time with Unity Profiler before selecting caches or pooling.
