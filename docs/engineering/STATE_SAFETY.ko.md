# Slap Squad, 동시 구매 저장과 방 객체 수명 수정

[English](STATE_SAFETY.md), [한국어](STATE_SAFETY.ko.md), [日本語](STATE_SAFETY.ja.md)

2026-10-21, [8a6245d](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017) → [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

32개 클라이언트의 동시 구매에서 저장 충돌을 재현했습니다. 네 종류의 아이템 저장 함수가 모두 temp.csv를 사용해 다른 요청의 임시 파일을 덮어쓰거나 삭제했습니다. 기존 CSV 형식을 유지하면서 재화 차감과 보유 목록을 한 번의 구매 단위로 저장하도록 고쳤습니다.

## 저장 확정과 복구

사용자 상태 잠금을 잡은 상태에서 구매 조건 확인, 메모리 변경, 저장을 처리합니다. 변경할 CSV 내용을 메모리에 모으고, 복구 기록을 디스크에 기록한 뒤 각 CSV를 교체합니다. 중간에 종료되면 다음 시작 때 복구 기록을 재적용하고 나서 접속을 받습니다. 복구 기록을 확정하기 전의 쓰기 실패는 메모리를 되돌립니다. 확정 뒤 파일 교체에 실패하면 기록을 남기고 서버를 종료해 다음 시작 때 복구합니다. 같은 데이터 폴더를 두 서버가 동시에 열 수 없도록 파일 잠금도 둡니다.

## 방 조회와 경기 종료

방 조회는 잠금 안에서 만든 읽기 전용 복사본을 반환합니다. 팀 변경과 사망 처리는 RoomManager 안에서 잠금을 잡고 수행하며, 경기별 승패 확정은 한 번만 허용합니다. 복귀 준비가 끝나면 같은 잠금 안에서 경기 상태를 정리합니다. 보상 저장에 실패하면 경기 번호를 확인한 뒤 재시도할 수 있도록 승패 확정을 되돌립니다. 방송은 수신자의 소유권을 확보한 뒤 방과 접속자 잠금을 풀고 송신 큐에 넣습니다.

## 선택한 이유

임시 파일명만 바꾸면 동시 변경 유실과 재화, 보유 목록의 불일치가 남습니다. 방을 shared_ptr로만 바꾸어도 삭제 이후 접근은 막을 수 있지만 동시 변경은 보호하지 못합니다. 구매는 하나의 트랜잭션으로 묶고, 방 조회는 일관된 복사본을 읽는 방식으로 정리했습니다. MOVE 처리는 사용자 저장 잠금을 거치지 않습니다.

## 재현과 검증

실제 서버에 32개 계정으로 접속해 캐릭터, 물풍선, 감정 표현, 아이콘 구매를 동시에 요청하고 계정마다 같은 요청을 두 번 보냈습니다. 저장된 잔액과 보유 목록이 모두 맞은 계정은 이 재현에서 수정 전 0/32, 수정 후 32/32였습니다. 별도로 프로필과 전적 저장 96건, 동시 구매 시도 16건, 가입 시 파일 7개 저장, 기록 확정 전 실패 시 복원, 파일 두 개 중 하나를 교체한 직후 종료한 상태의 복구를 검사했습니다. 방 검사는 다른 방의 이동과 경기 종료 12회를 겹치고, 경기마다 보상과 GAME_END가 한 번인지 확인한 뒤 임시 방 30개를 생성하고 삭제합니다.

## 제약

계정 쓰기는 하나의 사용자 잠금으로 직렬화하며, 트랜잭션마다 사용자 벡터를 복사하고 변경한 CSV를 다시 씁니다. 기존 형식과 저장 일관성을 유지하는 선택이며 높은 쓰기 처리량을 입증한 결과는 없습니다. 디스크 손상 복구와 재시작 후 진행 중 경기 복원도 검증 범위에 포함되지 않습니다. 클라이언트가 보고하는 명중의 서버 검증, 서버 전체 종료, Unity 할당량 측정은 남아 있습니다.

![저장 확정과 복구](diagrams/shop-sequence-ko.svg)

## 같은 조건에서 지연 재측정

같은 Windows 장비와 MOVE 전달 하네스, Release x64에서 방당 4명, 클라이언트당 초당 20회를 보냈습니다. 각 연결 수에서 버전별 5회, 준비 2초, 측정 10초, 수신 대기 2초를 적용하고 버전 순서를 번갈아 실행했습니다. 지연은 송신 직전부터 관찰자가 완성된 줄을 받을 때까지이며 측정 클라이언트의 이벤트 루프 대기도 포함합니다. CPU 100%는 논리 코어 하나입니다. Unity 렌더링과 전투 시뮬레이션을 제외한 대기실 전달 경로입니다.

| 연결 수 | p95 ms, 이전 / 이후 | CPU %, 이전 / 이후 | 최대 스레드, 이전 / 이후 | 최대 RSS MiB, 이전 / 이후 |
| --- | --- | --- | --- | --- |
| 8 | 11.64 / 13.14 | 1.56 / 2.03 | 13 / 13 | 11.45 / 11.49 |
| 32 | 14.07 / 15.18 | 3.43 / 3.75 | 37 / 37 | 12.83 / 12.94 |
| 64 | 17.55 / 18.03 | 9.22 / 7.65 | 69 / 69 | 14.50 / 14.67 |

버전별 5회 중앙값입니다. p95와 CPU는 낮을수록 좋으며 점 하나가 한 회차입니다.

p95 중앙값은 세 조건 모두 늘었습니다. 64개 연결에서는 CPU 중앙값이 9.22%에서 7.65%로 줄었고, 8개와 32개에서는 늘었습니다. 스레드는 13, 37, 69개로 같았습니다. 기대한 전달 624,000건은 모두 도착했고 다른 방 전달과 중복은 없었습니다. 저장과 방 상태의 오류를 고친 결과이며, 전반적인 지연 개선을 입증한 결과는 아닙니다.

앞선 p95 증가는 조건별 3회 측정에서 관찰된 값으로, 그것만으로 원인을 특정할 수는 없습니다. 32개 연결에서 서버 TCP_NODELAY만 바꾼 별도 실험은 개선을 입증하지 못했습니다. 첫 비동기 송신을 수신 스레드에서 바로 시작하는 시도는 워커 전달 한 단계를 줄였지만 CPU 비용이 증가해 제외했습니다. 최종 버전은 IOCP 배치 송신을 유지합니다. 아래에는 모든 회차와 중앙값을 함께 제시하며, 측정 변동을 서버 내부 처리 시간으로 단정하지 않습니다.

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
