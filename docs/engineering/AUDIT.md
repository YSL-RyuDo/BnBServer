# Code audit and remaining work

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

The inventory covers 26 native source/header files and 51 Unity C# files, 15,313 lines. Pattern searches guided manual inspection of message handling, storage, room lifetime and per-frame work. This is static triage, not a claim of full runtime profiling or proof that every path is safe.

| Status | Location | Finding / next action |
| --- | --- | --- |
| Fixed and measured | SocketOutput.cpp | Per-connection writers → one IOCP output worker; same-workload measurements above. |
| Fixed and measured | UserManager.cpp | Per-hit CSV parsing → immutable startup attack table; isolated lookup benchmark. |
| Fixed and tested | NetworkConnector.cs, LineMessageBuffer.cs | Incomplete-frame dispatch and split UTF-8 decoding; standalone C# tests, no full Unity build yet. |
| High priority, unresolved | UserManager.cpp | Save functions reuse temp.csv; wallet and ownership are written separately. Serialize writes and design atomic persistence before performance tuning. |
| High priority, unresolved | RoomManager.cpp | HandleReadyToExit still has a clientsMutex → roomsMutex path, opposite the broadcast lock order. GetRoomByUserId returns a raw pointer after unlocking. Add concurrent exit/delete tests before changing ownership. |
| High priority, unresolved | ClientHandler.h | Nickname/id helpers access clientsMap without a consistent locking boundary. Audit call sites together with room lifetime. |
| Profile next | GameSystem.cs, NetworkConnector.cs | Per-frame list copies and repeated scene/component searches. Cache stable player references and measure allocations with Unity Profiler before claiming an improvement. |
| Profile next | WeaponSystem.cs, BalloonSystem.cs | Instantiate/Destroy and scene searches during effects. Measure allocation/lifetime before deciding on pools or spatial indexing. |

[Source inventory](audit-source-inventory.json)

[Server code](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017/TeamOFYServer), [Unity code](https://github.com/YSL-RyuDo/BNB/tree/9b024d899e8aae17844ae6faacaaab4267c12509/Assets/Script), [Measured changes](IOCP.md)
