# 코드 점검과 남은 작업

[English](AUDIT.md), [한국어](AUDIT.ko.md), [日本語](AUDIT.ja.md)

네이티브 소스와 헤더 26개, Unity C# 51개, 총 15,313줄을 목록화했습니다. 패턴 검색 후 메시지 처리, 저장, 방 객체 수명, 프레임별 동작을 직접 확인했습니다. 정적 점검 결과이며 모든 경로를 실행해 프로파일링했거나 안전성을 증명했다는 뜻은 아닙니다.

| 상태 | 위치 | 확인한 내용 / 다음 작업 |
| --- | --- | --- |
| 수정, 측정 완료 | SocketOutput.cpp | 연결별 송신 스레드를 공용 IOCP 워커로 교체하고 같은 부하로 비교했습니다. |
| 수정, 측정 완료 | UserManager.cpp | HIT마다 CSV를 읽던 경로를 시작 시 공격력 캐시로 바꿨습니다. 조회 함수 측정은 별도로 표시했습니다. |
| 수정, 검사 완료 | NetworkConnector.cs, LineMessageBuffer.cs | 미완성 메시지 처리와 UTF-8 분할 문제를 수정했습니다. 독립 C# 검사는 통과했고 Unity 전체 빌드는 아직 검증하지 않았습니다. |
| 우선 수정 대상 | UserManager.cpp | 여러 저장 함수가 temp.csv를 공유하며 재화와 소유 목록은 따로 저장합니다. 쓰기 직렬화와 원자적 저장을 먼저 설계해야 합니다. |
| 우선 수정 대상 | RoomManager.cpp | HandleReadyToExit에 clientsMutex → roomsMutex 순서가 남아 방송 경로와 반대입니다. GetRoomByUserId는 잠금 해제 후 raw pointer를 반환합니다. 동시 퇴장과 방 삭제 재현 검사가 먼저 필요합니다. |
| 우선 수정 대상 | ClientHandler.h | 닉네임과 ID 조회 함수가 clientsMap을 일관된 잠금 경계 없이 읽습니다. 방 객체 수명과 함께 호출 경로를 정리해야 합니다. |
| 프로파일링 대상 | GameSystem.cs, NetworkConnector.cs | 프레임별 리스트 복사와 반복적인 오브젝트, 컴포넌트 검색이 있습니다. 참조 캐시와 상태 변경 시 갱신을 검토하되 Unity Profiler로 할당량부터 측정해야 합니다. |
| 프로파일링 대상 | WeaponSystem.cs, BalloonSystem.cs | 효과의 Instantiate/Destroy와 장면 전체 검색이 있습니다. 할당과 수명을 측정한 뒤 풀링이나 위치 인덱스 적용 여부를 결정해야 합니다. |

[Source inventory](audit-source-inventory.json)

[Server code](https://github.com/YSL-RyuDo/BnBServer/tree/8a6245d1a58fdbed77fc62b48eb6984a3af7f017/TeamOFYServer), [Unity code](https://github.com/YSL-RyuDo/BNB/tree/9b024d899e8aae17844ae6faacaaab4267c12509/Assets/Script), [Measured changes](IOCP.ko.md)
