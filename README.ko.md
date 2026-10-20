# BNB — Multiplayer Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

BNB 팀 프로젝트의 Windows C++/Winsock 서버로, 계정·방·게임플레이 패킷과 상점·보상 흐름을 다룹니다.

## 내 기여

- 서버 패킷 처리와 Unity 게임 연동을 작업했습니다.
- 상점 패킷, 프로필 데이터 처리와 아이템·아이콘 파싱을 구현·수정했습니다.
- 관련 CISC-W 2025 시뮬레이션 연구에서 보상 누적과 아이템 가격 문제를 분석했습니다.

## 기여 근거

- [Shop packets / 상점 패킷 / ショップパケット](https://github.com/YSL-RyuDo/BnBServer/commit/e41da66566252a105da68d98087198e632c0e9b7)
- [Profile / 프로필 / プロフィール](https://github.com/YSL-RyuDo/BnBServer/commit/37d6db7917815de40823af132aac8c31ed302a87)
- [Icon parsing / 아이콘 파싱 / アイコン解析](https://github.com/YSL-RyuDo/BnBServer/commit/c834f97f91bd8aaf62fe6bc2c7a2c29ea610ed9e)

커밋은 설명한 변경의 근거이며 기여율 점수가 아닙니다. 외부 에셋과 팀원 작업의 크레딧은 구분합니다.

## 실행·코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [TeamOFYServer.sln](TeamOFYServer.sln)을 여세요. 구성·데이터 경로·의존성·접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[BNB](https://github.com/YSL-RyuDo/BNB)

## 개발·검증 범위

연결별 스레드와 CSV 기반 데이터를 사용하는 학습·팀 게임 서버입니다. 대규모 서비스 성능을 검증했다는 주장은 하지 않습니다. 연구는 가격 정책 비교이며 실험한 동적 정책의 실제 배포를 주장하지 않습니다. 논문과 연구 배경은 BNB 클라이언트 저장소에서 관리합니다.
