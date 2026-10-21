# Slap Squad, CSV transactions and room lifetime

[English](STATE_SAFETY.md), [한국어](STATE_SAFETY.ko.md), [日本語](STATE_SAFETY.ja.md)

2026-10-21, [8a6245d](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017) → [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

A 32-client purchase test exposed a persistence failure: all four item categories wrote through the same temp.csv. Different requests could replace or remove one another’s temporary file. The follow-up change keeps CSV storage but commits a purchase’s balance and ownership together.

## Storage commit

A user transaction holds the user-state lock from validation through mutation and commit. Writes build after-images in memory. A flushed redo journal is atomically published before replacing individual CSV files; startup replays an unfinished journal before accepting clients. A failed precommit write rolls memory back. Failure after the journal commit point stops the process with the journal retained for recovery. A directory lease rejects a second server process using the same data directory.

## Room ownership

Room lookups now return immutable copies made under the room lock. Team changes and death registration mutate state only inside RoomManager. Death registration claims the result once per match, while the ready barrier clears match state before releasing its lock. A failed reward commit releases the claim for retry, checking the match generation. Broadcasts retain their recipients before releasing the room and client locks; queue insertion occurs after both locks are released.

## Design choices

A unique temporary filename alone would leave lost updates and a balance/ownership split. A shared_ptr to a mutable Room alone would prevent deletion but not data races. The selected transaction boundary covers one logical account operation; room snapshots cover one consistent read. MOVE does not acquire the user persistence lock.

## Verification

The real server purchase test sends two requests per account, with 32 accounts split across character, balloon, emote and icon purchases. Correct persisted balance and ownership: 0/32 before, 32/32 after, in this reproduction. The unit harness also checks 96 concurrent profile/stat commits, 16 simultaneous purchase attempts, seven-file registration, precommit failure rollback and recovery after process termination between two replacements. The room test completes 12 matches while another room moves, checks one reward and one GAME_END per match, and churns 30 temporary rooms.

## Limits

The user-state mutex serializes account writes, and each transaction snapshots the user vectors and rewrites affected CSVs. This favors correctness and compatibility over high write throughput. These tests do not establish database-scale capacity, recovery from disk corruption, or match restoration after a server restart. Client-reported hits still need independent server validation. Full process shutdown and Unity allocation profiling remain separate work.

![Storage commit](diagrams/shop-sequence-en.svg)

## Latency retest

The same MOVE relay harness, Windows host, Release x64, 4 clients per room and 20 messages/s/client are used for both versions. Each load has five runs per version, with alternating order, 2 s warm-up, 10 s measurement and 2 s drain. Latency includes sender and observer event-loop scheduling. CPU 100% means one logical core. This is a waiting-room relay test without Unity rendering or battle simulation.

| Connections | p95 ms, before / after | CPU %, before / after | Peak threads, before / after | Peak RSS MiB, before / after |
| --- | --- | --- | --- | --- |
| 8 | 11.64 / 13.14 | 1.56 / 2.03 | 13 / 13 | 11.45 / 11.49 |
| 32 | 14.07 / 15.18 | 3.43 / 3.75 | 37 / 37 | 12.83 / 12.94 |
| 64 | 17.55 / 18.03 | 9.22 / 7.65 | 69 / 69 | 14.50 / 14.67 |

Median of five runs per version. Lower p95 and CPU are better. Each dot is one run.

The median p95 increased at all three loads. At 64 connections, median CPU fell from 9.22% to 7.65%; at 8 and 32 it increased. Thread counts stayed at 13, 37 and 69. All 624,000 expected deliveries arrived with no cross-room delivery or duplication. This change fixes state correctness; it does not establish a general latency improvement.

The earlier p95 increase came from three runs per condition and does not identify a cause. A separate 32-client diagnostic changed only server TCP_NODELAY; it did not demonstrate an improvement. Starting the first overlapped send on the receive thread reduced one worker handoff but increased CPU in the pilot, so it was discarded. The final implementation retains IOCP batching. The retest below reports every run and the median rather than attributing all variation to the server.

[30 runs and build manifests](measurements/2026-10-21-state), [source diff](https://github.com/YSL-RyuDo/BnBServer/compare/8a6245d1a58fdbed77fc62b48eb6984a3af7f017...3e3dcef826fa09c4349038ca0b81ccc97f2c63a6), [Windows CI](https://github.com/YSL-RyuDo/BnBServer/actions/workflows/socket-regression.yml)

## Reproduce

Run from the BnBServer repository with MSVC v143, Python and psutil. Build directories and data are isolated synthetic fixtures. Do not point these tests at production data.

```powershell
python tools/benchmark/build.py --out ../before-state --port 39094 --ref 8a6245d1a58fdbed77fc62b48eb6984a3af7f017
python tools/benchmark/build.py --out ../after-state --port 39098 --ref 3e3dcef826fa09c4349038ca0b81ccc97f2c63a6
python tools/benchmark/compare.py --baseline ../before-state --candidate ../after-state --out ../state-scale --clients 8 32 64 --repeats 5
python tools/benchmark/purchase_regression.py --build ../before-state --out ../state-purchase-before --observe
python tools/benchmark/purchase_regression.py --build ../after-state --out ../state-purchase
python tools/benchmark/room_state.py --build ../after-state --out ../state-rooms
python tools/benchmark/test_state.py --out ../state-unit
```
