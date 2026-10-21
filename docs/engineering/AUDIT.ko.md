# Slap Squad, 남은 작업

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

2026-10-21, [3e3dcef](https://github.com/YSL-RyuDo/BnBServer/tree/3e3dcef826fa09c4349038ca0b81ccc97f2c63a6)

실제 서버에 32개 계정으로 접속해 캐릭터, 물풍선, 감정 표현, 아이콘 구매를 동시에 요청하고 계정마다 같은 요청을 두 번 보냈습니다. 저장된 잔액과 보유 목록이 모두 맞은 계정은 이 재현에서 수정 전 0/32, 수정 후 32/32였습니다. 별도로 프로필과 전적 저장 96건, 동시 구매 시도 16건, 가입 시 파일 7개 저장, 기록 확정 전 실패 시 복원, 파일 두 개 중 하나를 교체한 직후 종료한 상태의 복구를 검사했습니다. 방 검사는 다른 방의 이동과 경기 종료 12회를 겹치고, 경기마다 보상과 GAME_END가 한 번인지 확인한 뒤 임시 방 30개를 생성하고 삭제합니다.

계정 쓰기는 하나의 사용자 잠금으로 직렬화하며, 트랜잭션마다 사용자 벡터를 복사하고 변경한 CSV를 다시 씁니다. 기존 형식과 저장 일관성을 유지하는 선택이며 높은 쓰기 처리량을 입증한 결과는 없습니다. 디스크 손상 복구와 재시작 후 진행 중 경기 복원도 검증 범위에 포함되지 않습니다. 클라이언트가 보고하는 명중의 서버 검증, 서버 전체 종료, Unity 할당량 측정은 남아 있습니다.

[코드와 측정 원자료](STATE_SAFETY.ko.md), [IOCP](IOCP.ko.md), [Initial source inventory before the state-safety change](audit-source-inventory.json)

GameSystem.cs와 NetworkConnector.cs의 리스트 복사와 오브젝트, 컴포넌트 반복 검색, WeaponSystem.cs와 BalloonSystem.cs의 효과 생성과 제거, 장면 검색은 프로파일링 대상으로 남아 있습니다. Unity Profiler로 할당량과 프레임 시간을 측정한 뒤 참조 캐시나 풀링 적용 여부를 정합니다.
