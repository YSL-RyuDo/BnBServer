# 슬랩스쿼드, Slap Squad / Server

<!-- portfolio-languages -->
[![English](https://ysl-ryudo.github.io/assets/diagrams/language-en.svg)](README.md) [![한국어 (현재 언어)](https://ysl-ryudo.github.io/assets/diagrams/language-ko-current.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja.svg)](README.ja.md)
<!-- /portfolio-languages -->

물풍선 전투와 경기 보상, 아이템 상점이 이어지는 팀 멀티플레이 게임입니다.

| 프로젝트 정보 | 내용 |
|---|---|
| 개발 상태 | 팀 프로젝트 |
| 사용 기술 | C++, Winsock, C#, Unity |
| 기간과 팀 구성 | 2025.03 ~ 2026.02, 8명, 프로그래머 2명, 그래픽 6명 |
| 내 역할 | 게임플레이, C++ 서버와 클라이언트 연동 |

[개발 사례 보기](https://ysl-ryudo.github.io/ko/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/ko/)

[![슬랩스쿼드, Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## 게임에서 보는 구현

[![두 클라이언트가 같은 방에서 캐릭터를 선택하고 방장의 시작 요청으로 경기에 진입합니다.](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-room.webp)

두 클라이언트가 같은 방에서 캐릭터를 선택하고 방장의 시작 요청으로 경기에 진입합니다.

[![로컬 서버의 두 클라이언트 플레이. 캐릭터 상태와 물풍선, 장애물, 피해량 갱신이 연결된 화면입니다.](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)](https://ysl-ryudo.github.io/assets/gameplay/slapsquad-combat.webp)

로컬 서버의 두 클라이언트 플레이. 캐릭터 상태와 물풍선, 장애물, 피해량 갱신이 연결된 화면입니다.

## 게임 전체에서 맡은 구현

C++ 서버와 Unity 무기 7종을 구현하고 이동, 물풍선과 맵 상호작용, 경기 결과와 네트워크 연동을 작업했습니다. 상점 UI에서는 반환된 아이템 데이터에 따라 소유 상태를 갱신하도록 수정했습니다.

[![담당 구현 흐름](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-ko.svg)](https://ysl-ryudo.github.io/assets/diagrams/bnb-flow-ko.svg)

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

## 기여 근거

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

## 구현과 설계

방, 게임 내 행동, 보상과 구매를 클라이언트, 서버 흐름으로 연결해야 했습니다. 이를 구현하면서 플레이어가 얻는 보상에 비해 아이템 가격을 어떻게 정해야 할지 고민하게 되었습니다.

패킷, 게임 연동, 룸 리더, 물풍선 갱신, 상점, 프로필 흐름을 작업했습니다. 별도의 CISC-W 연구에서는 가상 플레이어의 행동을 이용해 고정 가격과 수요 기반 조정 정책을 비교했습니다.

## 코드의 동작과 구현 방식을 택한 이유

### 구매 클릭만으로 보유 상태를 바꾸지 않는 이유

**구현 흐름**

제가 수정한 ShopUI의 BuyItem은 선택한 상품의 구매 요청을 보내고 버튼을 비활성화하지만, 클릭 직후 selectedOwned를 true로 바꾸는 코드는 제거했습니다. 물풍선, 감정 표현, 아이콘 데이터가 도착하면 해당 탭이 현재 선택되어 있을 때 표시를 갱신하도록 수정했습니다.

**이 구조가 필요한 이유**

클라이언트가 아는 것은 사용자가 클릭했다는 사실이지 서버가 구매를 승인했다는 사실은 아닙니다. 반환된 상품 데이터에 맞춰 표시하면 추정한 소유 상태를 확정 상태처럼 보여주는 문제를 줄일 수 있습니다. 현재 탭도 확인해야 늦게 도착한 다른 분류의 응답이 보고 있는 목록을 바꾸지 않습니다.

**대안과 현재 구현의 절충**

클릭 직후 소유 표시를 바꾸면 반응은 빠르지만 실패했을 때 정확히 되돌려야 합니다. 서버 상품 데이터를 기다리면 응답 지연이 보이는 대신 소유를 먼저 확정하지 않습니다. 이 변경은 후자에 해당합니다. 처리 중 표시를 추가하면 거래가 끝난 것처럼 보이지 않으면서도 입력을 받았다는 반응을 줄 수 있습니다.

**장단점과 검증 범위**

반환 데이터를 기다리면 승인되지 않은 구매를 확정하지 않지만 처리 중 표시, 실패와 시간 초과 처리가 필요합니다. 이후 바이스에서는 거래 계층까지 재요청 처리를 확장했습니다.

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### 게임 동기화와 가격 연구에서 서로 다른 판단 대상

**구현 흐름**

제 멀티플레이 변경은 무기 행동, 물풍선 갱신, 룸 리더 정보를 클라이언트 네트워크 메시지와 연결합니다. C++ 서버는 연결별 스레드와 CSV 저장으로 방 및 계정, 상점 데이터를 처리합니다. 이 게임의 아이템 가격을 정하는 고민은 별도로 CNN 기반 소비 패턴 연구로 이어졌고, I2P-CON 발표 이후 보완하여 CISC-W 포스터로 발표했습니다.

**이 구조가 필요한 이유**

일부 월드 결정을 룸 리더가 보내는 방식은 서버가 직접 소유할 월드 시뮬레이션 범위를 줄이지만, 그 클라이언트에 대한 신뢰와 이탈 처리가 설계 조건이 됩니다. 가격 문제는 또 다릅니다. 구매 기능이 동작한다고 가격이 적절한지는 알 수 없습니다. 시뮬레이션에서는 보상과 소비에 관한 가정을 명시하고 같은 조건에서 정책을 비교할 수 있습니다.

**대안과 현재 구현의 절충**

전체 월드를 서버에서 계산하면 통제 범위가 넓어지지만 더 많은 게임 규칙을 서버에도 구현해야 합니다. 룸 리더 입력은 서버 구현 범위를 줄이는 대신 클라이언트 신뢰를 받아들입니다. 가격에서는 고정 정책이 설명하기 쉽고, 수요 연동 정책은 가정과 안정성 검토가 더 필요합니다. 연구는 정책 비교이며 동적 가격이 언제나 우월하다는 주장은 아닙니다.

**장단점과 검증 범위**

룸 리더 판정은 클라이언트를 신뢰하는 경계를 만들며 방장 이탈 처리도 필요합니다. 가격 연구에서는 가상 소비를 바탕으로 고정 가격과 수요 기반 정책을 비교했고, CNN 지표는 소비 패턴 분류 정확도입니다.

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

## 실행, 코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [TeamOFYServer.sln](TeamOFYServer.sln)을 여세요. 구성, 데이터 경로, 의존성, 접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

CNN 가격정책 연구를 「데이터로 설계하는 게임 경제」로 직접 발표했습니다. 교내 발표 이후 연구를 보완해 CISC-W 2025 학회 포스터로 발표했습니다.

[CISC-W 2025](https://ysl-ryudo.github.io/ko/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## 관련 저장소

[BNB](https://github.com/YSL-RyuDo/BNB), 공개 저장소

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), 공개 저장소

**CISC-W 2025: 제1저자, 포스터 발표**

[2025 한국정보보호학회 동계학술대회](https://kiisc.or.kr/payment/pay/292)

2025.11.27 ~ 28, 경기도 광주 곤지암 리조트
