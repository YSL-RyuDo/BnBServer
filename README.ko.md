# 슬랩스쿼드, Slap Squad / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

물풍선 전투와 경기 보상, 아이템 상점이 이어지는 팀 멀티플레이 게임입니다.

| 프로젝트 정보 | 내용 |
|---|---|
| 개발 상태 | 팀 프로젝트 |
| 사용 기술 | C++, Winsock, C#, Unity |
| 기간과 팀 구성 | 2025.03 ~ 2026.02, 8명, 프로그래머 2명, 그래픽 6명 |
| 내 역할 | 서버, 네트워크 연동, 상점 및 프로필 구현 |

[개발 사례 보기](https://ysl-ryudo.github.io/ko/work/bnb/), [Portfolio](https://ysl-ryudo.github.io/ko/)

[![슬랩스쿼드, Slap Squad](https://i.ytimg.com/vi/1LMGsY_qY_8/hqdefault.jpg)](https://www.youtube.com/watch?v=1LMGsY_qY_8)

[Slap Squad, OFY Studio](https://www.youtube.com/watch?v=1LMGsY_qY_8), [Early development demo](https://www.youtube.com/watch?v=MI5NtNU9eP8)

## 내 기여

서버 패킷 처리와 Unity 게임 연동을 작업했습니다.

상점 패킷, 프로필 데이터 처리와 아이템, 아이콘 파싱을 구현, 수정했습니다.

관련 CISC-W 2025 시뮬레이션 연구에서 보상 누적과 아이템 가격 문제를 분석했습니다.

## 기여 근거

[Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)

[Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)

[Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

커밋은 설명한 변경의 근거이며 기여율 점수가 아닙니다. 외부 에셋과 팀원 작업의 크레딧은 구분합니다.

## 구현과 설계

방, 게임 내 행동, 보상과 구매를 클라이언트, 서버 흐름으로 연결해야 했습니다. 이를 구현하면서 플레이어가 얻는 보상에 비해 아이템 가격을 어떻게 정해야 할지 고민하게 되었습니다.

패킷, 게임 연동, 룸 리더, 물풍선 갱신, 상점, 프로필 흐름을 작업했습니다. 별도의 CISC-W 연구에서는 가상 플레이어의 행동을 이용해 고정 가격과 수요 기반 조정 정책을 비교했습니다.

| 영역 | 구현 |
|---|---|
| 클라이언트 | 물풍선 갱신, 구매 UI, 결과, 보상 흐름 |
| 통신 | C++ Winsock, 연결별 스레드 |
| 상태, 저장 | CSV 데이터 기반 계정, 방, 상점, 프로필 |
| 판정 경계 | 일부 월드 갱신은 룸 리더에서 전달 |

## 코드의 동작과 구현 방식을 택한 이유

확인한 구현이 어떤 제약을 다루며 어떤 장단점이 생기는지 설명합니다. 설계 효과에 대한 설명은 코드 분석이며, 비교한 모든 대안을 당시 직접 구현했다는 뜻은 아닙니다. 기여 링크는 공동 프로젝트 안의 담당 변경을 가리키고 소스 링크는 기존 접근 권한을 유지합니다.

### 구매 클릭만으로 보유 상태를 바꾸지 않는 이유

**구현 흐름**

제가 수정한 ShopUI의 BuyItem은 선택한 상품의 구매 요청을 보내고 버튼을 비활성화하지만, 클릭 직후 selectedOwned를 true로 바꾸는 코드는 제거했습니다. 물풍선, 감정 표현, 아이콘 데이터가 도착하면 해당 탭이 현재 선택되어 있을 때 표시를 갱신하도록 수정했습니다.

**이 구조가 필요한 이유**

클라이언트가 아는 것은 사용자가 클릭했다는 사실이지 서버가 구매를 승인했다는 사실은 아닙니다. 반환된 상품 데이터에 맞춰 표시하면 추정한 소유 상태를 확정 상태처럼 보여주는 문제를 줄일 수 있습니다. 현재 탭도 확인해야 늦게 도착한 다른 분류의 응답이 보고 있는 목록을 바꾸지 않습니다.

**대안과 현재 구현의 절충**

클릭 직후 소유 표시를 바꾸면 반응은 빠르지만 실패했을 때 정확히 되돌려야 합니다. 서버 상품 데이터를 기다리면 응답 지연이 보이는 대신 소유를 먼저 확정하지 않습니다. 이 변경은 후자에 해당합니다. 처리 중 표시를 추가하면 거래가 끝난 것처럼 보이지 않으면서도 입력을 받았다는 반응을 줄 수 있습니다.

**장단점과 검증 범위**

공동 클라이언트 코드 안의 이 변경이 제 기여이며 ShopUI 전체의 단독 작성이라는 뜻은 아닙니다. 소유 표시를 기다리는 만큼 실패와 시간 초과 처리도 필요하지만, 이 패치만으로 재시도 처리가 완성되었다고 할 수는 없습니다. 이후 바이스의 구매 구조에서는 거래 계층에서도 재요청을 다룹니다.

[BNB 57cedd2e](https://github.com/YSL-RyuDo/BNB/commit/57cedd2e5c3b0dbbb7ea51b336602cc339630cea)

### 게임 동기화와 가격 연구에서 서로 다른 판단 대상

**구현 흐름**

제 멀티플레이 변경은 무기 행동, 물풍선 갱신, 룸 리더 정보를 클라이언트 네트워크 메시지와 연결합니다. C++ 서버는 연결별 스레드와 CSV 저장으로 방 및 계정, 상점 데이터를 처리합니다. 이 게임의 아이템 가격을 정하는 고민은 별도로 CNN 기반 소비 패턴 연구로 이어졌고, I2P-CON 발표 이후 보완하여 CISC-W 포스터로 발표했습니다.

**이 구조가 필요한 이유**

일부 월드 결정을 룸 리더가 보내는 방식은 서버가 직접 소유할 월드 시뮬레이션 범위를 줄이지만, 그 클라이언트에 대한 신뢰와 이탈 처리가 설계 조건이 됩니다. 가격 문제는 또 다릅니다. 구매 기능이 동작한다고 가격이 적절한지는 알 수 없습니다. 시뮬레이션에서는 보상과 소비에 관한 가정을 명시하고 같은 조건에서 정책을 비교할 수 있습니다.

**대안과 현재 구현의 절충**

전체 월드를 서버에서 계산하면 통제 범위가 넓어지지만 더 많은 게임 규칙을 서버에도 구현해야 합니다. 룸 리더 입력은 서버 구현 범위를 줄이는 대신 클라이언트 신뢰를 받아들입니다. 가격에서는 고정 정책이 설명하기 쉽고, 수요 연동 정책은 가정과 안정성 검토가 더 필요합니다. 연구는 정책 비교이며 동적 가격이 언제나 우월하다는 주장은 아닙니다.

**장단점과 검증 범위**

게임 전체를 서버 권위 시뮬레이션으로 설명하지 않습니다. 논문의 가상 실험 결과는 실제 이용자 행동 측정과 다르며 CNN 가격 모델을 BnBServer에 운영 배포했다는 근거도 아닙니다. 게임 구현과 연구 실험은 각각의 근거로 구분합니다.

[BNB 95b28f22](https://github.com/YSL-RyuDo/BNB/commit/95b28f22b7e88ed2c801e6d47a61ca573bcbc8a5), [BnBServer/ClientHandler.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/ClientHandler.cpp), [BnBServer/StoreData.cpp](https://github.com/YSL-RyuDo/BnBServer/blob/5f5f067f7cdc6ef71ab11a4d869ad2e1b6fdc8a2/TeamOFYServer/StoreData.cpp)

## 실행, 코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [TeamOFYServer.sln](TeamOFYServer.sln)을 여세요. 구성, 데이터 경로, 의존성, 접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[Slap Squad](https://github.com/YSL-RyuDo/BNB)

## 개발, 검증 범위

연결별 스레드와 CSV 기반 데이터를 사용하는 학습, 팀 게임 서버입니다. 대규모 서비스 성능을 검증했다는 주장은 하지 않습니다. 연구는 가격 정책 비교이며 실험한 동적 정책의 실제 배포를 주장하지 않습니다. 논문과 연구 배경은 Slap Squad 클라이언트 저장소에서 관리합니다.

CNN 가격정책 연구를 「데이터로 설계하는 게임 경제」로 직접 발표했습니다. 교내 발표 이후 연구를 보완해 CISC-W 2025 학회 포스터로 발표했습니다.

[CISC-W 2025](https://ysl-ryudo.github.io/ko/research/cisc/), [I2P-CON](https://www.youtube.com/watch?v=oq_e0TAwvJg)

## 관련 저장소

[BNB](https://github.com/YSL-RyuDo/BNB), 공개 저장소

[BnBServer](https://github.com/YSL-RyuDo/BnBServer), 공개 저장소

**CISC-W 2025: 제1저자, 포스터 발표**

[2025 한국정보보호학회 동계학술대회](https://kiisc.or.kr/payment/pay/292)

2025.11.27 ~ 28, 경기도 광주 곤지암 리조트
