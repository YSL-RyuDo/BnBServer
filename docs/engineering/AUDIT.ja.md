# コード点検と残る作業

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

ネイティブのソースとヘッダー26ファイル、Unity C# 51ファイル、計15,313行を一覧化しました。パターン検索後にメッセージ処理、保存、ルーム寿命、フレーム処理を確認しました。静的な点検であり、全経路の実行時プロファイリングや安全性の証明ではありません。

| 状態 | 場所 | 確認内容 / 次の作業 |
| --- | --- | --- |
| 修正・測定済み | SocketOutput.cpp | 接続別送信スレッドを共通IOCPワーカーに変更し、同じ負荷で比較しました。 |
| 修正・測定済み | UserManager.cpp | HITごとのCSV解析を起動時キャッシュに変更。参照関数の測定は別に表示しました。 |
| 修正・検査済み | NetworkConnector.cs, LineMessageBuffer.cs | 未完成フレームとUTF-8分割を修正。独立C#検査は通過、Unity全体ビルドは未検証です。 |
| 優先対応、未解決 | UserManager.cpp | 複数の保存関数がtemp.csvを共有し、通貨と所有権は別々に保存されます。書き込みの直列化と原子的保存を先に設計する必要があります。 |
| 優先対応、未解決 | RoomManager.cpp | HandleReadyToExitに配信経路と逆のclientsMutex → roomsMutex順序が残ります。GetRoomByUserIdはロック解除後のraw pointerを返します。同時退出と削除の再現検査が必要です。 |
| 優先対応、未解決 | ClientHandler.h | 名前とIDの検索が一貫したロック境界なしでclientsMapを参照します。ルーム寿命と一緒に呼び出し側を確認する必要があります。 |
| 次に計測 | GameSystem.cs, NetworkConnector.cs | 毎フレームのリストコピーと反復するオブジェクト検索。参照キャッシュを検討し、Unity Profilerで割り当て量を測定します。 |
| 次に計測 | WeaponSystem.cs, BalloonSystem.cs | 効果生成・破棄とシーン検索。割り当てと寿命を計測してからプールや位置索引の採否を決めます。 |

[Source inventory](audit-source-inventory.json)

[Server code](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017/TeamOFYServer), [Unity code](https://github.com/YSL-RyuDo/BNB/tree/9b024d899e8aae17844ae6faacaaab4267c12509/Assets/Script), [Measured changes](IOCP.ja.md)
