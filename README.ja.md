# Slap Squad / C++ Server

[![English](https://ysl-ryudo.github.io/assets/diagrams/language-en.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja-current.svg)](README.ja.md)

7種類の武器、マルチプレイルーム、C++ゲームサーバーの実装。

2025年3月から2026年2月、プログラマー2名とグラフィック担当6名

C++サーバーとUnityの武器動作を実装し、移動、水風船の相互作用、試合結果、購入応答を連携させました。

[Portfolio](https://ysl-ryudo.github.io/ja/work/bnb/), [技術ドキュメント全文](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ja.md), [Unity](https://github.com/YSL-RyuDo/BNB), [C++ server](https://github.com/YSL-RyuDo/BnBServer)

[![Slap Squad, four-player gameplay](https://ysl-ryudo.github.io/assets/evidence/slap-four-balloons.webp)](https://ysl-ryudo.github.io/ja/work/bnb/#gameplay)

## 送信キューの設計と再現方法

別室の1人が読み取りを停止した条件で、応答は変更前0/300件、変更後300/300件を3回とも確認しました。上限付きキュー、部分送信の直列化、接続の後処理を検証しました。

| 条件 / 指標 | 変更前、ルーム別blocking send | 変更後、上限付き送信キュー |
| --- | --- | --- |
| 読み取り停止、各回の別室への配信 | 0 / 300 | 300 / 300 |
| 読み取り停止、各回p95の中央値 | 定義不可、応答なし | 13.82 ms |
| 通常条件、各回の別室への配信 | 300 / 300 | 300 / 300 |
| 通常条件、各回p95の中央値 | 14.00 ms | 13.62 ms |
| 観測した最大スレッド数、8接続 | 12 | 20 |

[送信キューの設計と再現方法](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/OUTPUT_QUEUE.ja.md), [12回の生データとビルド根拠](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21-output-queue), [Git diff](https://github.com/YSL-RyuDo/BnBServer/compare/737a92c397adb4320cf439d99d148362ce1ca090...9e9110727ff5fa8c4cd016f9a55efe4aa93ecc4e)


## 実装と検証

2026年10月の追加作業: ルーム別配信、TCP受信境界、上限付き送信キュー、接続の後処理、UMLと36回の測定。

従来のMOVE処理はBroadcastMessageExceptを呼び、送信者以外の全接続へ通知していました。4人ずつ16ルームに接続する64人条件では、本来3人に届くべき更新が最大63人に届きます。余分な負荷だけでなく、ルーム境界を破る動作でした。

64接続でp95の中央値は4,381.75 msから19.91 msに変わりました。各回に受信したアプリケーションデータの中央値は27.30 MiBから1.30 MiBへ約95.2%減少しました。新しいネットワーク方式ではなく、更新1件の受信者を最大63人から3人に限定した結果です。

遅延は送信側の書き込み直前から別クライアントが応答の1行を受信するまでです。負荷生成器、TCP、OSのスケジューリングを含みます。参加者のいる待機ルームで実際のMOVE処理を測定しており、試合全体のシミュレーションやインターネット環境の同時接続上限ではありません。

| 接続 / ルーム | 従来 p95 | 修正 p95 | 従来 CPU | 修正 CPU | 修正後 未着 / 他ルーム |
| --- | --- | --- | --- | --- | --- |
| 4 / 1 | 12.22 ms | 13.42 ms | 1.09% | 1.09% | 0 / 0 |
| 16 / 4 | 12.22 ms | 11.78 ms | 7.82% | 2.81% | 0 / 0 |
| 32 / 8 | 28.77 ms | 15.90 ms | 42.95% | 5.77% | 0 / 0 |
| 64 / 16 | 4,381.75 ms | 19.91 ms | 99.10% | 17.94% | 0 / 0 |

各回で算出した値の3回の中央値。CPUの100%は論理コア1つです。未着と他ルームへの配信は修正版3回の合計です。

[生データと測定環境](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21), [ソース変更](https://github.com/YSL-RyuDo/BnBServer/commit/737a92c397adb4320cf439d99d148362ce1ca090), [実験の再現](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ja.md#reproduction)

![UML](https://ysl-ryudo.github.io/assets/slap-squad/server-classes-ja.svg)

## ゲーム全体で担当した実装

C++サーバーとUnityの7種の武器を実装し、移動、水風船とマップの相互作用、試合結果、通信連携を担当しました。ショップUIでは返却されたアイテムデータに従って所有状態を更新するよう変更しました。


### 7種の武器動作

Arrow、Laser、Mace、Melody、Pitchfork、Spell、Swordの動作を実装しました。WeaponSystemで装備選択、生成、クールダウン、音を接続しました。通信処理に加えて、実際の戦闘行動を作るクライアント実装です。

[WeaponSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/WeaponSystem.cs), [担当変更 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Arrow.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Arrow.cs), [担当変更 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Sword.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Sword.cs), [担当変更 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 操作と攻撃タイミング

移動同期と武器攻撃・水風船設置の入力切替を担当しました。Updateで入力を読み、FixedUpdateでRigidbodyを移動します。攻撃要求に武器・位置・向きを渡し、レーザーにはマップ障害物から求めた距離も含めます。

[LocalPlayerController.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/LocalPlayerController.cs), [担当変更 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 水風船と破壊可能なマップ

水風船設置、クールダウン、重複設置確認、水の伝播、マップとブロック更新を担当しました。サーバーはルームリーダーモデルでワールド更新を中継します。

[BalloonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/BalloonSystem.cs), [担当変更 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [MapSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/MapSystem.cs), [担当変更 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5)

### ルーム準備とシーン遷移

キャラクター選択、チーム編成、ホストの開始要求を試合へ接続しました。NetworkConnectorで遷移前後の選択、試合シーン移動、キャッシュ状態を扱いました。

[RoomSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Room/RoomSystem.cs), [担当変更 be42bb9](https://github.com/YSL-RyuDo/BNB/commit/be42bb99ced515ad702e9a15e9d95bd5e029839c), [NetworkConnector.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Network/NetworkConnector.cs), [担当変更 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 試合結果、報酬とエモート

GameSystemのプレイヤー生成・被弾・死亡・勝者・報酬、復帰準備を担当しました。エモートの要求・表示・クールダウンも実装しました。結果データを単独画面で終わらせず、プレイ終了と次の流れにつなぎました。

[GameSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/GameSystem.cs), [担当変更 eb6a766](https://github.com/YSL-RyuDo/BNB/commit/eb6a7660b4f690e0d46a7a3e75c50c744d40f745), [EmoticonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/EmoticonSystem.cs), [担当変更 576f512](https://github.com/YSL-RyuDo/BNB/commit/576f51272b078731e71e882afce04ce8ae164ac3)

### アカウント、経済と購入結果

CSV保存を使うサーバーのアカウント、ルーム、ショップ、プロフィール処理を実装しました。共同ShopUIでは購入直後の所有確定を除き、返却データで該当タブを更新しました。価格設計の問いは別のCNNシミュレーション研究へ発展しました。

[Server.cpp (main)](https://github.com/YSL-RyuDo/BnBServer/blob/d2a360c07276ec057a92d1361e8ff84de743520c/TeamOFYServer/Server.cpp), [担当変更 e41da66](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7), [ShopUI.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Shop/ShopUI.cs), [担当変更 57cedd2](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)



[CSV保存と購入結果の表示](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ja.md), [ゲームプレイ動画とスクリーンショット](https://ysl-ryudo.github.io/ja/work/bnb/#more-demos)
