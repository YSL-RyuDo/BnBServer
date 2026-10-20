# BNB — Multiplayer Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

BNBチームプロジェクト向けWindows C++/Winsockサーバーです。アカウント、ルーム、ゲームパケット、ショップ・報酬フローを扱います。

## 担当した内容

- サーバーパケット処理とUnityゲーム連携を担当しました。
- ショップパケット、プロフィール情報処理、アイテム・アイコン解析を実装・修正しました。
- 関連するCISC-W 2025のシミュレーション研究で、報酬の蓄積とアイテム価格を分析しました。

## 貢献の根拠

- [Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)
- [Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)
- [Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

コミットは記載した変更の根拠であり、貢献率の点数ではありません。外部素材・チームメンバーの貢献は区別します。

## 実行・コードの確認

Windows C++ワークロードを備えたVisual Studioで[TeamOFYServer.sln](TeamOFYServer.sln)を開いてください。構成・データパス・依存関係・接続設定を確認し、対応するクライアントを起動します。既存アカウントデータは維持してください。

[BNB](https://github.com/YSL-RyuDo/BNB)

## 開発・検証範囲

接続ごとのスレッドとCSVベースのデータを使う学習・チームゲーム用サーバーです。大規模サービス性能の検証は主張しません。研究は価格政策の比較であり、実験した動的政策の本番導入は主張しません。論文と背景説明はBNBクライアント側にまとめています。
