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
