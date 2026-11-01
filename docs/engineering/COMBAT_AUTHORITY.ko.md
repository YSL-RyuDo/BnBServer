# 서버 권한 전투 상태 1차

> 이 문서는 이전 HP/사망 처리 단계의 기록이다. 후속 공간 충돌 구현과 현재 제한은 [COLLISION_AUTHORITY.ko.md](COLLISION_AUTHORITY.ko.md)를 참조한다.

인증 고도화 `3b6731c`에서 `feature/server-authoritative-combat`를 분기했다.
Hybrid Winsock 수신 + IOCP 송신 구조, 기존 캐릭터 능력치, 개인전/협동전 보상은 유지한다.
실제 계정 CSV와 main은 변경하지 않는다. 이 변경은 체력·사망·보상의 권한 이전이며,
공간 충돌까지 서버가 검증하는 전투 시스템의 완성은 아니다.

## 재현과 변경

기존 서버에는 게임 시작 전 HIT, 타인의 DEAD, 다른 방을 향한 WATER_HIT, 음수 피해량,
선택하지 않은 무기 사용, 클라이언트 피격 간격 우회가 있었다.
최종 `combat_regression.py`를 동일하게 실행한 결과 기존 소스는 19건 중 1건 통과/18건 실패,
수정 후는 19건 통과했다. 실패에는 신규 절대 체력 프로토콜 검증도 포함되므로
18개의 독립적인 취약점이라는 의미는 아니다.

| 파일·함수 | 변경 |
| --- | --- |
| `Room.h` | 경기별 캐릭터, 체력, 공격력, 다음 피격 가능 시각, 경기 번호, 상태 순번, 보상 완료 여부 |
| `RoomManager::InitializeCombat`, `TryStartGame`, `TryStartCoopGame` | 시작 시 서버 능력치를 복사하고 경기 상태를 초기화 |
| `RoomManager::ApplyDamage`, `RecordDeathLocked` | 같은 방/경기, 생존, 무기 일치, 팀, 1초 피격 간격을 한 room mutex 안에서 검사하고 체력·사망을 원자적으로 변경 |
| `RoomManager::HandleCharacterChoice` | 선택 주체, 인덱스 범위 및 경기 중 선택 변경 차단 |
| `RoomManager::HandleReadyToExit`, `MarkRewardsCommitted`, `RetryDeath` | 보상 저장 전 종료 금지, 이미 저장된 보상 재시도 금지 |
| `ClientHandler::ProcessMessages` | 엄격한 HIT/WATER_HIT 형식, 경기 번호, 세션 소유권 검사. 경기 시작은 해당 방장만 가능 |
| `ClientHandler::FinishDeath` | 서버 사망 결과로 기존 개인전/팀전 보상 계산 실행. 계정 트랜잭션 잠금을 해제한 뒤 방 잠금/송신 처리 |
| `UserManager::LoadAttackStats`, `GetHealthByIndex` | 시작 시 양수 체력/공격력과 인덱스를 검증하고 캐시 |
| Unity `CombatState`, `NetworkConnector`, `GameSystem`, 7종 무기, `LocalPlayerController` | 경기 번호 전송, 서버 절대 체력 적용, 중복·역순 상태 거부, 서버 사망 표시 |

LOBBY_MESSAGE, TEAMCHANGE, EXIT_ROOM, 무기/멜로디 표시, 풍선 설치/제거,
EMO_CLICK의 송신자 이름도 로그인 세션과 대조한다. GET_MAP은 현재 경기의 방/맵으로 제한한다.
이 검사만으로 오브젝트 소유권·거리·속도까지 검증된 것은 아니다.

## 프로토콜과 상태

모든 패킷은 기존처럼 LF로 구분한다. 계정 저장 형식은 인증 고도화의 v2 그대로이며 변경하지 않는다.
전투 상태는 메모리에만 존재한다.

| 용도 | 이전 | 변경 후 |
| --- | --- | --- |
| 경기 시작 통지 | `GAME_START|room` | `GAME_START|room|matchId` |
| 무기 충돌 신고 | `HIT|weapon|attacker|target` | `HIT|weapon|attacker|target|matchId` |
| 물 충돌 신고 | `WATER_HIT|nickname|damage` | `WATER_HIT|nickname|matchId` (본인만, 피해량 10은 서버 결정) |
| 체력 통지 | `DAMAGE|nickname|damage` 또는 `PLAYER_HIT|nickname|damage` | `DAMAGE|nickname|damage|remainingHP|matchId|revision` |
| 사망 통지 | `PLAYER_DEAD|nickname` | `PLAYER_DEAD|nickname|matchId` |
| 사망 신고 | `DEAD|nickname`이 사망 생성 | 본인 서버 체력이 이미 0일 때만 저장 실패 재시도용 수신 확인으로 사용 |

Unity는 더 이상 슬라이더의 체력에서 피해량을 빼거나 그 결과로 사망을 결정하지 않는다.
상태 순번은 플레이어별로 비교하므로 서로 다른 수신 스레드의 통지가 역순으로 도착해도
이전 체력으로 되돌아가지 않는다. UI 생성 전 피해도 캐시한다.
서버가 알려준 본인 체력이 0이면 2초 간격의 DEAD 확인으로 실패한 보상 저장을 재시도할 수 있다.
저장 실패 시 경기 진행/복귀를 막고, 실제 파일 저장 성공 후 한 번만 보상한다.
저널 커밋 이후 복구 불가능한 파일 오류는 기존 CsvStore 정책대로 프로세스를 종료한다.

서버·클라이언트를 함께 업데이트해야 한다. 구버전 HIT와 피해량 기반 PLAYER_HIT는 호환하지 않는다.
구버전 WATER_HIT의 마지막 숫자가 현재 경기 번호와 우연히 같아도 본인에게 고정 피해 10만 적용한다.
게임 시작 장면 전환은 GAME_START에서 한 번 실행하고 START_GAME_SUCCESS는 승인 응답으로만 취급한다.

## 실행 검증

Python 3.14, MSVC v143 Release x64, .NET 9 환경에서 실행했다.
서버는 별도 출력 디렉터리에서 loopback 주소와 테스트 포트로만 빌드/실행한다.
`build.py --ref 3b6731c`로 이전 서버를 만들고 동일한 최종 재현 테스트를 적용했다.
다음 명령의 `<out>`은 실제 계정이 없는 새로운 테스트 출력 디렉터리다.

```powershell
python tools/benchmark/build.py --ref 3b6731c --out <before-build> --port 39120
python tools/benchmark/build.py --out <after-build> --port 39121
python tools/benchmark/combat_regression.py --build <before-build> --out <before-results>
python tools/benchmark/combat_regression.py --build <after-build> --out <after-results>
python tools/benchmark/combat_completion.py --build <after-build> --out <completion-results>
python tools/benchmark/access_regression.py --build <after-build> --out <access-results>
python tools/benchmark/hit_regression.py --build <after-build> --out <hit-results>
python tools/benchmark/room_state.py --build <after-build> --out <room-results>
```

로컬 실제 결과: 전투 요청 19/19, 협동전·동시 치명타·보상 저장 실패 복구 9/9,
접근 제어 9/9, 7종 무기 28/28 전달, 12회 연속 경기 및 방 변경 5/5 검사 통과.
기존 framing/방 전달, 연결 수명, 인증 18건, 인증 동시성 6건, 구매 32건도 통과했다.
7종 무기 검사는 각 무기를 실제로 선택한 뒤 7개 방에서 시작한다.
방 상태 검사는 합성 캐릭터 체력을 10으로 설정해 실제 HIT로 사망시키며,
기존처럼 DEAD만 보내서 승패를 만드는 동작을 기대하지 않는다.
느린 수신자 격리/송신 큐 검사, 공격력 캐시 병렬 읽기 검사, 인증·CSV 트랜잭션·송신 큐 단위 검사와
6초 지연 초기화 구매 검사도 통과했다. 새 두 전투 검사는 GitHub Actions에 등록했다.
실제 결과 JSON과 빌드 소스 해시는 `measurements/2026-11-01-combat/`에 보관했다.

## 남은 범위와 병합 조건

- 실제 Unity 2022.3.40f1 Editor PlayMode/전체 빌드는 미실행이다. 이 PC에는 해당 버전이 없고
  설치된 6000.3.1f1로 프로젝트를 업그레이드하지 않았다. .NET 상태 처리 검사는 플레이 검증을 대신하지 않는다.
- HIT의 실제 거리·시야·투사체 궤적, 이동 속도/위치, 풍선 실체·폭발 시각·물 타일은 아직 서버가 검증하지 않는다.
  따라서 인증된 사용자는 유효한 경기·무기로 거짓 HIT를 보내거나 WATER_HIT를 생략할 수 있다.
  다음 단계는 서버의 공격/투사체/풍선 등록과 공간 충돌 판정이다.
- DESTROY_BLOCK 등 오브젝트 변경 명령의 개별 권한·수명 검증, 캐릭터 구매 소유권 전면 검증,
  경기 중 이탈·서버 재시작 시 승패/보상 복구는 후속 과제다. 현재 경기는 메모리 상태다.
- 방장 이외의 GET_MAP 재요청과 동일 경기 맵 재전송 정책은 추가 검토가 필요하다.
- TCP는 TLS가 아니다. 전송 자격 증명 보호, 이전 평문 CSV/Git 이력 처리, 인증 시도 제한은 기존 후속 과제다.
- main 병합 전 동일 버전 Unity로 개인전/협동전, 7종 무기, 물 피해, 2회 이상 재경기, 결과 화면을 확인해야 한다.
