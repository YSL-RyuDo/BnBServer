# 슬랩스쿼드, Slap Squad / C++ Server

[![English](https://ysl-ryudo.github.io/assets/diagrams/language-en.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko-current.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja.svg)](README.ja.md)

무기 7종부터 멀티플레이 방과 C++ 서버까지 구현한 게임.

2025년 3월부터 2026년 2월, 프로그래머 2명과 그래픽 팀원 6명

C++ 서버와 Unity 무기 동작을 구현하고 이동, 물풍선 상호작용, 경기 결과와 구매 응답을 연결했습니다.

[Portfolio](https://ysl-ryudo.github.io/ko/work/bnb/), [전체 기술 문서](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ko.md), [Unity](https://github.com/YSL-RyuDo/BNB), [C++ server](https://github.com/YSL-RyuDo/BnBServer)

[![Slap Squad, four-player gameplay](https://ysl-ryudo.github.io/assets/evidence/slap-four-balloons.webp)](https://ysl-ryudo.github.io/ko/work/bnb/#gameplay)

## 느린 수신자 격리를 유지하면서 송신 스레드 비용 줄이기

64개 연결에서 관찰한 최대 스레드 수의 중앙값은 132개에서 69개, 서버 CPU는 논리 코어 하나 기준 12.03%에서 7.34%로 줄었습니다. 최대 RSS의 중앙값은 16.03 MiB에서 14.49 MiB로 줄었습니다. 반면 p95 중앙값은 14.81 ms에서 15.35 ms로 늘었습니다. 자원 비용은 줄었지만 모든 응답이 빨라졌다는 결과는 아닙니다.

| 연결 수 | 스레드, 이전 → 이후 | CPU %, 이전 → 이후 | p95 ms, 이전 → 이후 | 최대 RSS MiB, 이전 → 이후 |
| --- | --- | --- | --- | --- |
| 8 | 20 → 13 | 1.87 → 1.72 | 12.41 → 15.31 | 11.65 → 11.45 |
| 32 | 68 → 37 | 11.09 → 4.06 | 13.53 → 14.16 | 13.60 → 12.83 |
| 64 | 132 → 69 | 12.03 → 7.34 | 14.81 → 15.35 | 16.03 → 14.49 |

같은 Windows 장비, Release x64, TCP 루프백에서 방마다 4명, 클라이언트마다 초당 MOVE 20회를 보냈습니다. 연결 수와 버전별로 3회 반복하고 준비 2초, 측정 10초, 수신 대기 2초를 적용했습니다. 버전 실행 순서는 번갈아 배치했습니다. 아래 값은 회차별 값의 중앙값이며 p95도 회차별 p95의 중앙값입니다. CPU 100%는 논리 코어 하나입니다. 참가자가 있는 대기실의 전달 경로를 측정했으며 Unity 렌더링, 인터넷 통신, 전체 전투 시뮬레이션은 포함하지 않습니다.

GetAttackByIndex는 HIT마다 CharStat.csv를 열고 파싱했습니다. 서버가 연결을 받기 전에 공격력 7개를 검증해 읽고, 이후에는 변경하지 않는 메모리 맵을 조회하도록 바꿨습니다. 없는 인덱스는 기존처럼 0을 반환합니다. 시작 데이터가 잘못되면 로딩에 실패하며, 파일 변경은 서버를 다시 시작해야 반영됩니다. 시작 시점의 설정을 유지하는 정책이며 핫 리로드나 DB 전환은 아닙니다.

실제 UserManager 코드에서 조회 10만 회를 버전별 5회씩 번갈아 실행했고 결과 체크섬은 모두 같았습니다. 파일 캐시가 준비된 조건이며 초기 로딩은 제외했습니다. 소요 시간 중앙값은 9044.14 ms → 0.2805 ms였습니다. 반복적인 파일 열기와 파싱을 작은 메모리 맵 조회와 비교한 결과입니다. 게임 전체나 서버 처리량이 같은 비율로 빨라졌다고 해석할 수 없습니다.

[구현과 재현 방법](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/IOCP.ko.md), [측정 원자료](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21-iocp), [코드 점검과 남은 작업](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/AUDIT.ko.md)


## 앞선 수정: 느린 수신자의 영향 분리

이전 단계의 기록입니다. 당시에는 연결마다 blocking 송신 스레드를 두었습니다. 현재 구현은 위의 IOCP 방식이며 이전 측정값은 변경 이력으로 보존합니다.

다른 방의 수신자 한 명이 읽기를 멈춘 조건에서 응답은 수정 전 0/300건, 수정 후 300/300건으로 3회 모두 확인했습니다. 크기를 제한한 큐, 부분 송신의 순차 처리, 연결 정리 검사를 추가했습니다.

| 조건 / 지표 | 이전, 방별 blocking send | 이후, 제한된 송신 큐 |
| --- | --- | --- |
| 읽기 중단, 회차별 정상 B방 내부 응답 | 0 / 300 | 300 / 300 |
| 읽기 중단, 회차별 p95의 중앙값 | 계산 불가, 수신 응답 없음 | 13.82 ms |
| 정상 조건, 회차별 정상 B방 내부 응답 | 300 / 300 | 300 / 300 |
| 정상 조건, 회차별 p95의 중앙값 | 14.00 ms | 13.62 ms |
| 관찰한 최대 스레드, 연결 8개 | 12 | 20 |

[송신 큐 설계와 재현 방법](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/OUTPUT_QUEUE.ko.md), [12회 원자료와 빌드 근거](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21-output-queue), [Git diff](https://github.com/YSL-RyuDo/BnBServer/compare/737a92c397adb4320cf439d99d148362ce1ca090...9e9110727ff5fa8c4cd016f9a55efe4aa93ecc4e)


## 구현과 검증

2026년 10월 후속 작업: 방별 전달과 TCP 수신 경계 수정, IOCP 송신, 시작 공격력 캐시, UML과 재현 가능한 측정, 플레이 영상.

기존 MOVE 처리는 BroadcastMessageExcept를 호출해 송신자를 제외한 모든 연결에 전달했습니다. 4명씩 16개 방에 접속한 64명 조건에서는 실제 수신 대상 3명 대신 최대 63명에게 이동이 전달됩니다. 불필요한 부하이면서 방의 경계를 어기는 동작이었습니다.

64개 연결에서 p95의 중앙값은 4,381.75 ms에서 19.91 ms로 바뀌었습니다. 매회 수신한 애플리케이션 데이터의 중앙값은 27.30 MiB에서 1.30 MiB로 약 95.2% 줄었습니다. 네트워크 방식을 교체한 효과가 아니라 이동 하나의 수신 대상을 최대 63명에서 3명으로 제한한 결과입니다.

지연 시간은 송신 클라이언트가 쓰기 직전부터 다른 클라이언트가 응답 한 줄을 모두 받을 때까지입니다. 부하 생성기, TCP, 운영체제 스케줄링을 포함합니다. 참가자가 들어 있는 대기실에서 실제 MOVE 처리를 측정했으며, 전체 경기 시뮬레이션이나 인터넷 환경의 동접 한계를 측정한 것은 아닙니다.

| 연결 / 방 | 기존 p95 | 수정 p95 | 기존 CPU | 수정 CPU | 수정 후 누락 / 다른 방 |
| --- | --- | --- | --- | --- | --- |
| 4 / 1 | 12.22 ms | 13.42 ms | 1.09% | 1.09% | 0 / 0 |
| 16 / 4 | 12.22 ms | 11.78 ms | 7.82% | 2.81% | 0 / 0 |
| 32 / 8 | 28.77 ms | 15.90 ms | 42.95% | 5.77% | 0 / 0 |
| 64 / 16 | 4,381.75 ms | 19.91 ms | 99.10% | 17.94% | 0 / 0 |

각 실행에서 계산한 값의 3회 중앙값입니다. CPU 100%는 논리 코어 하나입니다. 누락과 다른 방 전달은 수정 버전 3회 합계입니다.

[원자료와 측정 환경](https://github.com/YSL-RyuDo/BnBServer/tree/main/docs/engineering/measurements/2026-10-21), [소스 변경](https://github.com/YSL-RyuDo/BnBServer/commit/737a92c397adb4320cf439d99d148362ce1ca090), [실험 재현](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ko.md#reproduction)

![UML](https://ysl-ryudo.github.io/assets/slap-squad/server-classes-ko.svg)

## 게임 전체에서 맡은 구현

C++ 서버와 Unity 무기 7종을 구현하고 이동, 물풍선과 맵 상호작용, 경기 결과와 네트워크 연동을 작업했습니다. 상점 UI에서는 반환된 아이템 데이터에 따라 소유 상태를 갱신하도록 수정했습니다.


### 무기 7종의 동작

Arrow, Laser, Mace, Melody, Pitchfork, Spell, Sword의 동작을 구현했습니다. WeaponSystem에서 장착 무기 선택과 생성, 쿨다운과 소리를 연결했습니다. 서버의 패킷 처리 외에 실제 전투 행동을 만드는 클라이언트 작업입니다.

[WeaponSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/WeaponSystem.cs), [내 변경 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Arrow.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Arrow.cs), [내 변경 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf), [Sword.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/Weapon/Sword.cs), [내 변경 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 플레이어 조작과 공격 시점

이동 동기화와 무기 공격, 물풍선 배치 사이의 입력 전환을 작업했습니다. Update에서 입력을 읽고 FixedUpdate에서 Rigidbody를 이동합니다. 공격 요청에는 무기와 위치 및 방향을 넣고, 레이저는 맵 장애물을 검사한 거리도 함께 보냅니다.

[LocalPlayerController.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/LocalPlayerController.cs), [내 변경 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 물풍선과 파괴 가능한 맵

물풍선 배치와 쿨다운, 겹치는 배치 검사와 물줄기 전파, 맵과 블록 갱신을 작업했습니다. 서버는 룸 리더 모델에 맞춰 월드 갱신을 전달합니다.

[BalloonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/BalloonSystem.cs), [내 변경 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [MapSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/MapSystem.cs), [내 변경 95b28f2](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5)

### 방 준비와 씬 전환

캐릭터 선택과 팀 구성, 방장의 시작 요청을 게임 진입에 연결했습니다. NetworkConnector에서 전환 전후의 캐릭터 선택과 경기 관련 씬 이동, 캐시 상태를 다뤘습니다.

[RoomSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Room/RoomSystem.cs), [내 변경 be42bb9](https://github.com/YSL-RyuDo/BNB/commit/be42bb99ced515ad702e9a15e9d95bd5e029839c), [NetworkConnector.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Network/NetworkConnector.cs), [내 변경 c79844d](https://github.com/YSL-RyuDo/BNB/commit/c79844d425e5c81e1c9549885e29deeeb66515cf)

### 경기 결과, 보상과 감정 표현

GameSystem의 플레이어 생성과 피격, 사망, 승자와 보상 처리, 복귀 준비 흐름을 작업했습니다. 감정 표현의 요청과 표시, 쿨다운도 구현했습니다. 결과 데이터를 별도 화면에만 두지 않고 플레이 종료와 다음 흐름에 연결했습니다.

[GameSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/GameSystem.cs), [내 변경 eb6a766](https://github.com/YSL-RyuDo/BNB/commit/eb6a7660b4f690e0d46a7a3e75c50c744d40f745), [EmoticonSystem.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Game/EmoticonSystem.cs), [내 변경 576f512](https://github.com/YSL-RyuDo/BNB/commit/576f51272b078731e71e882afce04ce8ae164ac3)

### 계정, 경제와 구매 결과 표시

CSV 저장을 사용하는 서버의 계정, 방, 상점과 프로필 처리를 구현했습니다. 공동 ShopUI에서는 구매 직후 소유를 확정하던 처리를 제거하고 반환 데이터에 따라 해당 탭을 갱신하도록 수정했습니다. 아이템 가격에 대한 고민은 별도의 CNN 시뮬레이션 연구로 이어졌습니다.

[Server.cpp (main)](https://github.com/YSL-RyuDo/BnBServer/blob/d2a360c07276ec057a92d1361e8ff84de743520c/TeamOFYServer/Server.cpp), [내 변경 e41da66](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7), [ShopUI.cs (main)](https://github.com/YSL-RyuDo/BNB/blob/8a3defa6001cf741eceb322f11d7c8c6f7dd9bd7/Assets/Script/Shop/ShopUI.cs), [내 변경 57cedd2](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)



[CSV 저장과 구매 결과 표시](https://github.com/YSL-RyuDo/BnBServer/blob/main/docs/engineering/README.ko.md), [게임플레이 영상과 스크린샷](https://ysl-ryudo.github.io/ko/work/bnb/#more-demos)
