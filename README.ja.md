# Slap Squad / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

水風船の戦闘、試合報酬、アイテムショップがつながるチーム制作のマルチプレイヤーゲームです。

| プロジェクト情報 | 内容 |
|---|---|
| 開発状況 | チーム開発 |
| 使用技術 | C++, Winsock, C#, Unity |
| 期間とチーム構成 | 2025.03 ~ 2026.02, 8名：プログラマー2名、グラフィック6名 |
| 担当 | サーバー・ネットワーク連携・ショップとプロフィール実装 |

[開発事例を見る](https://ysl-ryudo.github.io/ja/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/ja/)

[![Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## 担当した内容

サーバーパケット処理とUnityゲーム連携を担当しました。

ショップパケット、プロフィール情報処理、アイテム・アイコン解析を実装・修正しました。

関連するCISC-W 2025のシミュレーション研究で、報酬の蓄積とアイテム価格を分析しました。

## 貢献の根拠

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

コミットは記載した変更の根拠であり、貢献率の点数ではありません。外部素材・チームメンバーの貢献は区別します。

## 実装と設計

ルーム、ゲーム内操作、報酬、購入をクライアント・サーバー間で連携させる必要がありました。その実装から、獲得報酬に対してアイテム価格をどう設定するかという疑問が生まれました。

パケット・ゲーム連携、ルームリーダー・水風船更新、ショップ・プロフィール処理を担当しました。別のCISC-W研究では仮想プレイヤーの行動を用い、固定価格と需要連動政策を比較しました。

| 領域 | 実装 |
|---|---|
| クライアント | 水風船の更新・購入UI・結果と報酬の処理 |
| 通信 | C++ Winsock・接続ごとのスレッド |
| 状態・保存 | CSVデータによるアカウント・ルーム・ショップ・プロフィール |
| 判定の境界 | 一部のワールド更新はルームリーダーから送信 |

## コードの動作と実装構造の理由

確認した実装が扱う制約と利点・代償を説明します。設計効果の説明はコード分析であり、比較する全代案を当時試作したという意味ではありません。担当リンクは共同制作内の変更を示し、ソースの閲覧権限は維持します。

### 購入クリックだけでは所有状態を変えない理由

**実装の流れ**

私のShopUI変更では、BuyItemが購入要求を送りボタンを無効にする一方、直ちにselectedOwnedをtrueにする処理を削除しました。水風船、エモート、アイコンのデータ到着時には、対応するタブが選択中の場合に表示を更新するよう変更しました。

**この構造が必要な理由**

クライアントが知っているのはクリックの事実であり、サーバーの購入承認ではありません。返された商品データに合わせれば、推測した所有状態を確定扱いする問題を減らせます。現在のタブも確認することで、別カテゴリの遅延応答が表示中の一覧を置き換えるのを避けます。

**代案と現在の実装の折合い**

クリック直後の所有表示は速い一方、失敗時に正確な巻戻しが必要です。サーバーの商品データ待ちは遅延が見える代わりに所有を早まって確定しません。この変更は後者です。処理中表示なら取引完了を装わず入力への反応を示せます。

**利点・代償と検証範囲**

これは共同クライアント内の担当変更であり、ShopUI全体の単独実装を意味しません。所有表示を待つ分、失敗とタイムアウト対応も重要ですが、この差分だけで再試行処理の完成は示せません。後のVAISでは取引層でも再要求を扱います。

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### ゲーム同期と価格研究で異なる判断の対象

**実装の流れ**

私のマルチプレイヤー変更は武器操作、水風船更新、ルームリーダー情報をクライアントのネットワークメッセージにつなぎます。C++サーバーは接続ごとのスレッドとCSVでルーム、アカウント、ショップを扱います。アイテム価格の検討は別途CNNによる消費傾向の研究につながり、I2P-CON発表後に補完してCISC-Wポスターで発表しました。

**この構造が必要な理由**

一部のワールド判断をルームリーダーから送る方式はサーバー側のシミュレーション範囲を減らしますが、そのクライアントへの信頼と切断対応が条件になります。価格は別の問題で、購入機能の動作だけでは適切さは分かりません。シミュレーションでは報酬や消費の仮定を明示し、同じ条件で政策を比較できます。

**代案と現在の実装の折合い**

全ワールドのサーバー計算は制御を広げる代わりに多くの規則をサーバーにも実装します。ルームリーダー入力は実装範囲を減らしクライアントへの信頼を受け入れます。固定価格は説明しやすく、需要連動価格は仮定と安定性の検討が増えます。研究は比較であり動的価格の普遍的優位を示すものではありません。

**利点・代償と検証範囲**

ゲーム全体をサーバー権威型シミュレーションとは説明しません。論文の仮想実験は実利用者の測定とは異なり、CNN価格モデルをBnBServerで運用した証拠でもありません。実装と研究はそれぞれの根拠で区別します。

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

## 実行・コードの確認

Windows C++ワークロードを備えたVisual Studioで[TeamOFYServer.sln](TeamOFYServer.sln)を開いてください。構成・データパス・依存関係・接続設定を確認し、対応するクライアントを起動します。既存アカウントデータは維持してください。

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

## 開発・検証範囲

接続ごとのスレッドとCSVベースのデータを使う学習・チームゲーム用サーバーです。大規模サービス性能の検証は主張しません。研究は価格政策の比較であり、実験した動的政策の本番導入は主張しません。論文と背景説明はSlap Squadクライアント側にまとめています。

CNN価格政策研究を「データで設計するゲーム経済」として発表。その後、研究を補完してCISC-W 2025のポスター発表につなげました。

[CISC-W 2025](https://ysl-ryudo.github.io/ja/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## 関連リポジトリ

[BNB](https://github.com/YSL-RyuDo/BNB), 公開リポジトリ

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), 公開リポジトリ

**CISC-W 2025: 筆頭著者、ポスター発表**

[2025年 韓国情報保護学会 冬季学術大会](https://kiisc.or.kr/payment/pay/292)

2025.11.27 ~ 28, 韓国・京畿道広州、昆池岩リゾート
