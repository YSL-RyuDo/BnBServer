# Slap Squad, 残る作業

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

2026-10-21, [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

実サーバーに32アカウントで接続し、キャラクター、水風船、エモート、アイコンを同時購入しました。各要求を2回送り、保存後の残高と所持情報が正しかった数は、この再現では修正前0/32、修正後32/32でした。別途96件のプロフィールと戦績保存、16件の同時購入試行、登録時の7ファイル保存、確定前の失敗時の復元、2ファイル中1ファイルを置換した直後の強制終了からの復旧を検査しました。別ルームの移動と12試合の終了を重ね、報酬とGAME_ENDが試合ごとに1回であることを確認し、30個の一時ルームを生成、削除しました。

アカウントの書き込みは1つのロックで直列化し、各トランザクションでユーザーベクトルをコピーして対象CSVを書き直します。既存形式と整合性を優先した設計で、高い書き込み処理能力を示す測定ではありません。ディスク破損や再起動後の試合復元は検証範囲外です。命中報告の独立したサーバー検証、プロセス全体の終了処理、Unityの割り当て計測は残っています。

[コードと測定データ](STATE_SAFETY.ja.md), [IOCP](IOCP.ja.md), [Initial source inventory before the state-safety change](audit-source-inventory.json)

GameSystem.csとNetworkConnector.csにはリストのコピーとオブジェクト、コンポーネントの反復検索があります。WeaponSystem.csとBalloonSystem.csのエフェクト生成、破棄、シーン検索も計測対象です。Unity Profilerで割り当て量とフレーム時間を測ってから、キャッシュやプーリングの採否を判断します。
