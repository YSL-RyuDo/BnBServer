# Slap Squad, 同時購入の保存とルームの寿命を修正

[English](STATE_SAFETY.md), [한국어](STATE_SAFETY.ko.md), [日本語](STATE_SAFETY.ja.md)

2026-10-21, [8a6245d](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017) → [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

32クライアントの同時購入で保存競合を再現しました。4種類のアイテム保存が同じtemp.csvを使い、別の要求の一時ファイルを上書き、削除していました。CSV形式を維持し、残高と所持情報を1回の購入単位で保存するように変更しました。

## 保存の確定と復旧

ユーザー状態のロック内で検証、メモリ更新、コミットを行います。CSVの更新後イメージをメモリにまとめ、復旧ジャーナルをフラッシュして確定してから各CSVを置換します。途中で終了した場合は、起動時にジャーナルを再適用してから接続を受け付けます。確定前の書き込み失敗ではメモリを戻します。確定後の置換失敗では記録を保持してプロセスを停止し、次回起動時に復旧します。同じデータディレクトリを別のサーバープロセスが開くこともファイルロックで防ぎます。

## ルームの参照と試合終了

ルームの参照はロック内で作成する読み取り専用コピーに変更しました。チーム変更と死亡処理はRoomManagerのロック内で行い、勝敗の確定を試合ごとに1回に制限します。復帰準備が揃った時点で、同じロック内で状態を整理します。報酬の保存失敗時には試合番号を確認して確定を取り消し、再試行を可能にします。配信先の所有権を保持してルームと接続者のロックを解放した後、送信キューに追加します。

## 設計の理由

一時ファイル名の変更だけでは更新の消失や残高と所持情報の不一致が残ります。Roomをshared_ptrに変えるだけでも同時変更は保護できません。購入は1つのトランザクションにまとめ、ルームは一貫したコピーを読み取ります。MOVE処理はユーザー保存のロックを取得しません。

## 再現と検証

実サーバーに32アカウントで接続し、キャラクター、水風船、エモート、アイコンを同時購入しました。各要求を2回送り、保存後の残高と所持情報が正しかった数は、この再現では修正前0/32、修正後32/32でした。別途96件のプロフィールと戦績保存、16件の同時購入試行、登録時の7ファイル保存、確定前の失敗時の復元、2ファイル中1ファイルを置換した直後の強制終了からの復旧を検査しました。別ルームの移動と12試合の終了を重ね、報酬とGAME_ENDが試合ごとに1回であることを確認し、30個の一時ルームを生成、削除しました。

## 制約

アカウントの書き込みは1つのロックで直列化し、各トランザクションでユーザーベクトルをコピーして対象CSVを書き直します。既存形式と整合性を優先した設計で、高い書き込み処理能力を示す測定ではありません。ディスク破損や再起動後の試合復元は検証範囲外です。命中報告の独立したサーバー検証、プロセス全体の終了処理、Unityの割り当て計測は残っています。

![保存の確定と復旧](diagrams/shop-sequence-ja.svg)

## 同じ条件での遅延再測定

同じWindows環境、MOVE配信ハーネス、Release x64で、1ルーム4人、各クライアント毎秒20要求を送ります。接続数ごとに各版5回、準備2秒、測定10秒、受信待ち2秒で版の実行順を交互にします。遅延は送信直前から観測側の行受信までで、測定クライアントのイベントループ待機も含みます。CPU 100%は論理コア1個です。Unity描画と戦闘シミュレーションを含まない待機ルームの配信測定です。

| 接続数 | p95 ms、変更前 / 後 | CPU %、変更前 / 後 | 最大スレッド、変更前 / 後 | 最大RSS MiB、変更前 / 後 |
| --- | --- | --- | --- | --- |
| 8 | 11.64 / 13.14 | 1.56 / 2.03 | 13 / 13 | 11.45 / 11.49 |
| 32 | 14.07 / 15.18 | 3.43 / 3.75 | 37 / 37 | 12.83 / 12.94 |
| 64 | 17.55 / 18.03 | 9.22 / 7.65 | 69 / 69 | 14.50 / 14.67 |

各版5回の中央値です。p95とCPUは低いほど良く、各点は1回の測定です。

p95の中央値は3条件とも増加しました。64接続ではCPUの中央値が9.22%から7.65%に低下しましたが、8接続と32接続では増加しました。スレッド数は13、37、69のままです。期待した624,000件をすべて受信し、別ルームへの配信と重複はありませんでした。状態の整合性を修正した結果であり、全般的な遅延改善を示すものではありません。

前回のp95増加は条件ごとに3回測定した結果であり、原因を特定するものではありません。32接続でサーバー側TCP_NODELAYだけを変更した別実験では改善を示せませんでした。最初の非同期送信を受信スレッドから直接開始する案も、CPUコストが増えたため採用していません。最終版はIOCPバッチ送信を維持します。以下では各回と中央値を示し、変動をサーバー内部処理時間と断定しません。

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
