# Slap Squad / Server

<!-- portfolio-languages -->
[![English](https://ysl-ryudo.github.io/assets/diagrams/language-en.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko.svg)](README.ko.md) [![日本語（選択中）](https://ysl-ryudo.github.io/assets/diagrams/language-ja-current.svg)](README.ja.md)
<!-- /portfolio-languages -->

水風船の戦闘、試合報酬、アイテムショップがつながるチーム制作のマルチプレイヤーゲームです。

| プロジェクト情報 | 内容 |
|---|---|
| 開発状況 | チーム開発 |
| 使用技術 | C++, Winsock, C#, Unity |
| 期間とチーム構成 | 2025.03 ~ 2026.02, 8名：プログラマー2名、グラフィック6名 |
| 担当 | ゲームプレイ、C++サーバーとクライアント連携 |

[開発事例を見る](https://ysl-ryudo.github.io/ja/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/ja/)

[![Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## ゲーム内の実装

[![2つのクライアントが同じルームでキャラクターを選び、ホストの開始要求で試合に進みます。](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)

2つのクライアントが同じルームでキャラクターを選び、ホストの開始要求で試合に進みます。

[![ローカルサーバーでの2クライアントプレイ。キャラクター状態、水風船、障害物、ダメージ更新が連動します。](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)

ローカルサーバーでの2クライアントプレイ。キャラクター状態、水風船、障害物、ダメージ更新が連動します。

## ゲーム全体で担当した実装

C++サーバーとUnityの7種の武器を実装し、移動、水風船とマップの相互作用、試合結果、通信連携を担当しました。ショップUIでは返却されたアイテムデータに従って所有状態を更新するよう変更しました。

[![担当実装の流れ](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-ja.svg)](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-ja.svg)

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

## 貢献の根拠

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

外部素材・チームメンバーの貢献は区別します。

## 実装と設計

ルーム、ゲーム内操作、報酬、購入をクライアント・サーバー間で連携させる必要がありました。その実装から、獲得報酬に対してアイテム価格をどう設定するかという疑問が生まれました。

パケット・ゲーム連携、ルームリーダー・水風船更新、ショップ・プロフィール処理を担当しました。別のCISC-W研究では仮想プレイヤーの行動を用い、固定価格と需要連動政策を比較しました。

## コードの動作と実装構造の理由

### 購入クリックだけでは所有状態を変えない理由

**実装の流れ**

私のShopUI変更では、BuyItemが購入要求を送りボタンを無効にする一方、直ちにselectedOwnedをtrueにする処理を削除しました。水風船、エモート、アイコンのデータ到着時には、対応するタブが選択中の場合に表示を更新するよう変更しました。

**この構造が必要な理由**

クライアントが知っているのはクリックの事実であり、サーバーの購入承認ではありません。返された商品データに合わせれば、推測した所有状態を確定扱いする問題を減らせます。現在のタブも確認することで、別カテゴリの遅延応答が表示中の一覧を置き換えるのを避けます。

**代案と現在の実装の折合い**

クリック直後の所有表示は速い一方、失敗時に正確な巻戻しが必要です。サーバーの商品データ待ちは遅延が見える代わりに所有を早まって確定しません。この変更は後者です。処理中表示なら取引完了を装わず入力への反応を示せます。

**利点・代償と検証範囲**

返却データを待つことで未承認購入の確定を避けますが、処理中・失敗・タイムアウトの状態が必要です。後のVAISでは取引層まで再要求処理を拡張しました。

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### ゲーム同期と価格研究で異なる判断の対象

**実装の流れ**

私のマルチプレイヤー変更は武器操作、水風船更新、ルームリーダー情報をクライアントのネットワークメッセージにつなぎます。C++サーバーは接続ごとのスレッドとCSVでルーム、アカウント、ショップを扱います。アイテム価格の検討は別途CNNによる消費傾向の研究につながり、I2P-CON発表後に補完してCISC-Wポスターで発表しました。

**この構造が必要な理由**

一部のワールド判断をルームリーダーから送る方式はサーバー側のシミュレーション範囲を減らしますが、そのクライアントへの信頼と切断対応が条件になります。価格は別の問題で、購入機能の動作だけでは適切さは分かりません。シミュレーションでは報酬や消費の仮定を明示し、同じ条件で政策を比較できます。

**代案と現在の実装の折合い**

全ワールドのサーバー計算は制御を広げる代わりに多くの規則をサーバーにも実装します。ルームリーダー入力は実装範囲を減らしクライアントへの信頼を受け入れます。固定価格は説明しやすく、需要連動価格は仮定と安定性の検討が増えます。研究は比較であり動的価格の普遍的優位を示すものではありません。

**利点・代償と検証範囲**

ルームリーダーの判定にはクライアントを信頼する境界が生じ、リーダー退出への対応も必要です。価格研究は仮想消費で固定価格と需要連動政策を比較し、CNN指標は消費パターンの分類精度です。

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

## 実行・コードの確認

Windows C++ワークロードを備えたVisual Studioで[TeamOFYServer.sln](TeamOFYServer.sln)を開いてください。構成・データパス・依存関係・接続設定を確認し、対応するクライアントを起動します。既存アカウントデータは維持してください。

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

CNN価格政策研究を「データで設計するゲーム経済」として発表。その後、研究を補完してCISC-W 2025のポスター発表につなげました。

[CISC-W 2025](https://ysl-ryudo.github.io/ja/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## 関連リポジトリ

[BNB](https://github.com/YSL-RyuDo/BNB), 公開リポジトリ

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), 公開リポジトリ

**CISC-W 2025: 筆頭著者、ポスター発表**

[2025年 韓国情報保護学会 冬季学術大会](https://kiisc.or.kr/payment/pay/292)

2025.11.27 ~ 28, 韓国・京畿道広州、昆池岩リゾート
