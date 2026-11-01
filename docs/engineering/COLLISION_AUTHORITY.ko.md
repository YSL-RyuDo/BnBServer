# 서버 권한 이동·공간 충돌 판정

2026-11-01. `feature/server-authoritative-combat`의 HP/사망/보상 처리에 이어,
`feature/server-authoritative-collision`에서 공격과 물풍선의 실체 및 충돌을 서버로 옮겼다.
Hybrid Winsock 수신 + IOCP 송신 구조를 유지했다. main 병합, 실제 계정 파일 변경은 하지 않았다.
실제 Unity 플레이 검증을 마친 배포 버전이라는 뜻은 아니다.

## 재현한 문제와 수정

기준 서버 `e8df8e4`는 인증된 클라이언트가 유효한 경기 번호로 HIT를 보내면 실제 공격 없이도
HP를 줄일 수 있었다. 물 피해 보고를 생략하거나 원격 좌표에 무기/풍선을 만들고,
존재하지 않는 풍선을 폭발시키거나 임의 오브젝트를 삭제하는 요청도 전달했다.
`collision_regression.py`의 동일한 11개 시나리오를 기준 서버와 수정 서버에 적용했다.
기준 서버는 보호 검사 11/11 실패(위조 결과 전달), 수정 서버는 11/11 통과했다.
검증은 합성 계정과 격리된 CSV에서만 수행했다.

클라이언트는 이제 입력 방향과 공격 의도만 전송한다. 서버가 경기별 위치, 공격 ID, 무기 종류,
투사체 이동·반사·만료, 풍선 위치·종류·폭발 시각, 물 타일, 장애물 및 HP를 결정한다.
피해자 클라이언트가 아무 보고도 보내지 않아도 서버 충돌로 피해를 받는다.

## 변경 파일과 함수

| 파일 | 주요 함수와 역할 |
| --- | --- |
| `TeamOFYServer/CombatWorld.h` | `Initialize/AddPlayer/Ready`, `Input/Fire/Place`, `Advance/Step`: 맵과 경기 상태, 입력 순서·쿨다운, 이동 및 충돌 계산 |
| `TeamOFYServer/Room.h` | 경기별 `world` 보관 |
| `TeamOFYServer/RoomManager.h/.cpp` | `InitializeCombat`: 맵/스폰/장비 확정. `WorldCommand`: 세션·경기 검사. `MapResponse`: 요청자에게 동일 경기 맵 전송. `TickWorld/ApplyDamageLocked`: 시뮬레이션 결과를 기존 HP/보상 처리에 연결. `BroadcastToMatch`: 경기 번호로 결과 전송 범위 확인 |
| `TeamOFYServer/ClientHandler.h/.cpp` | `ProcessMessages`: 입력 전용 명령의 엄격한 형식 검사, 이전 결과 주장 명령 차단. `CombatMap/CombatBalloon/PublishDeath`: 기존 저장소·보상 처리 연결 |
| `TeamOFYServer/Server.h/.cpp` | `Run`, 소멸자: 약 20ms 간격의 소유된 시뮬레이션 스레드 시작·종료 |
| `TeamOFYServer/MapManager.cpp` | `LoadMapByNameWithSpawns`: Map1/Map2 허용 목록, 숫자/크기 검사, 경로 입력 차단 |
| `TeamOFYServer/TeamOFYServer.vcxproj` | 새 헤더 등록 |
| `tools/benchmark/collision_*`, `test_collision.py`, `world_client.py`, `world_socket.py` | 변경 전후 공격 재현, 결정적 시계 충돌 검사, 실제 소켓 검증 |
| 기존 combat/access/hit/room 테스트와 `run.py` | 실제 서버 공격 의도와 준비 완료 흐름 사용, 합성 맵 fixture 추가 |
| `.github/workflows/socket-regression.yml` | 신규 검사 등록, 기존 검사 유지 |

Unity 변경은 대응 저장소의 `docs/COLLISION_AUTHORITY.ko.md`에 기록했다.

## 데이터와 프로토콜

계정 CSV 형식은 이번 단계에서 변경하지 않았다. 기존 인증 v2 해시 저장 구조를 그대로 사용한다.
경기 상태는 메모리에만 있다. 재시작 후 진행 중 경기를 복원하는 기능은 없다.

모든 패킷은 기존 UTF-8/LF 프레이밍을 사용한다. `match`는 서버 경기 번호, `seq`는 연결된
플레이어의 명령 종류별 증가 번호다. 클라이언트 시각·피해량·공격자·대상·생성 좌표를 신뢰하지 않는다.

| 이전 클라이언트 요청 | 새 요청/동작 |
| --- | --- |
| `MOVE|nickname|x,z` | 경기 중 무시. `MOVE_INPUT|match|seq|dx,dz`로 단위 방향 전송 |
| `WEAPON_ATTACK|nickname|weapon|position|yaw...` 및 `HIT|...` | `ATTACK|match|seq|yaw`. 서버가 선택된 무기·생성 위치·피해자 계산 |
| `PLACE_BALLOON|nickname|position|type` | `BALLOON|match|seq`. 서버 위치의 인접 격자와 장착 풍선 사용 |
| `WATER_HIT`, `REMOVE_BALLOON`, `MELODY_*`, `DESTROY_SPELL`, `DESTROY_BLOCK`, `HITWALL` | 클라이언트 요청은 상태를 바꾸지 않음 |
| 방장만 GET_MAP | 각 클라이언트가 자기 방/맵을 요청. 스폰은 경기 시작 시 한 번 확정 |
| 장면 로딩 순서에 따른 즉시 전투 | 맵/캐릭터 생성 후 `MATCH_READY|match`, 전원 준비 시 `COMBAT_BEGIN|match` |

서버 이벤트:

```text
PLAYER_STATE|match|nickname|x,z|lastMoveSeq
WEAPON_SPAWN|match|id|nickname|weapon|x,y,z|yaw,pitch|laserLength
WEAPON_STATE|match|id|x,y,z|yaw,pitch
WEAPON_REMOVE|match|id
TILE_REMOVE|match|x,z
COMBAT_BEGIN|match
```

기존 `PLACE_BALLOON_RESULT`, `REMOVE_BALLOON`, `WATER_SPREAD`, `DAMAGE`, `PLAYER_DEAD`,
승패/재경기 이벤트는 표시용 응답으로 유지한다. 풍선/물 이벤트는 기존 형식이며 경기 번호가 없다.
서버가 현재 방 수신자에게만 보내지만 재접속·지연 이벤트의 전면적인 세대 구분은 후속 과제다.
구 클라이언트와의 경기 프로토콜 호환은 제공하지 않으므로 서버·Unity를 함께 업데이트해야 한다.
로비의 기존 MOVE는 네트워크 전달 회귀 검사와 호환을 위해 남겼으며 경기 상태에는 반영하지 않는다.

## 시뮬레이션과 게임 규칙

- 고정된 두 맵의 15×13 지형, 15×13 상단 오브젝트를 사용한다. Map2의 3/4는 Block,
  6/7은 Wall이다. 바깥 경계는 파괴할 수 없다.
- 이동 속도 3, 플레이어 평면 반경 0.3. 벽·풍선·다른 플레이어를 통과할 수 없다.
  입력은 250ms 후 만료한다. 전송 횟수가 많아도 시뮬레이션 시간이 빨라지지 않는다.
- 서버 스텝은 최대 10ms로 나누며 한 번의 지연 복구를 100ms로 제한한다. 위치/무기 스냅샷은
  목표 20Hz다. 긴 정지 후 순간이동하지 않는 대신 과부하 시 경기 시간이 느려질 수 있다.
- 7종 무기의 기존 prefab 수치와 동작을 바탕으로 XZ 평면 충돌을 구현했다. Sword는 0.3초/120도,
  Arrow는 속도 10, Spell은 속도 5/1.5초 및 기존 벽 통과, Mace는 반경 1.5/720도/0.5초,
  Melody는 속도 3/반사 후 2.4/반사 시 1.5초 재설정, Pitchfork는 0.3초 회전,
  Laser는 최대 길이 15/0.3초다. 피해량과 팀/피해 무적 시간은 기존 서버 HP 규칙을 사용한다.
- 무기 쿨다운은 1.5초, Laser는 2초. 공격당 대상 중복 피해를 막는다. 방당 공격 32개,
  풍선 16개로 제한한다. Arrow는 최대 5초, Melody는 반사 횟수와 무관하게 최대 10초로 제한했다.
- 투사체는 프레임 사이 선분과 피해자 원을 검사한다. 벽 가까이 생성된 투사체가 벽 너머에서
  시작하지 않게 한다. 이미 겹친 반사 투사체는 제거하여 무한 끼임을 막는다.
- 근접 무기는 회전한 상자와 플레이어 원, 시야를 검사한다. Pitchfork의 벽 파괴는 실제
  겹친 상자에만 적용한다. 이 평면 근사는 Unity PhysX 전체를 재현하지 않는다.
- 풍선은 3초 후 서버가 폭발시키며 십자 2칸, 물은 2초 유지한다. Block을 만나면 해당 블록만
  파괴하고 전파를 멈추며 Wall은 막는다. 설치자는 풍선에서 나올 수 있지만 다시 통과하지 못한다.
  다른 플레이어와 겹치는 설치는 거부한다. 물 진입 피해의 2초 제한을 서버에서 적용한다.

의도적인 수정: 기존 Laser 스크립트는 collider 크기와 transform 배율에 길이를 중복 적용해
보이는 광선보다 긴 판정을 만들 수 있었다. 새 판정은 표시 범위 최대 15에 맞췄다.
Melody의 10초 전체 수명 제한과 입력 만료/충돌 보정도 이전과 달라지는 부분이다.
기존 리소스·UI·맵·개인전/협동전 흐름은 유지했지만 모든 조작감이 동일하다고 단정하지 않는다.

## 재현 명령과 결과

MSVC v143 Release x64/C++14, Python, .NET 9 사용. `<out>`은 실제 계정이 없는 별도 출력 디렉터리다.
빌드 도구는 서버 복사본의 바인딩을 loopback과 테스트 포트로만 바꾼다.

```powershell
python tools/benchmark/build.py --ref e8df8e4 --out <before-build> --port 39130
python tools/benchmark/build.py --out <after-build> --port 39133
python tools/benchmark/collision_regression.py --build <before-build> --out <before>
python tools/benchmark/collision_regression.py --build <after-build> --out <after>
python tools/benchmark/test_collision.py --out <unit>
python tools/benchmark/world_socket.py --build <after-build> --out <world>
```

나머지 소켓 검사는 `python tools/benchmark/<name>.py --build <after-build> --out <out>`으로 실행했다.
대상은 combat_regression, combat_completion, access_regression, hit_regression, room_state,
regression, lifecycle, auth_regression, auth_concurrency, purchase_regression이다.
각 검사 결과와 최종 빌드 소스 해시는 `measurements/2026-11-01-collision/`에 보관한다.
결과의 `source_commit`은 미커밋 빌드 시 기준 커밋이므로 실제 검증 대상은 `source_hashes`로 식별한다.

| 실제 로컬 검사 | 결과 |
| --- | --- |
| 서버 Release x64 빌드 | 성공, 최종 소스의 CRLF 정규화 해시 대조 일치 |
| 동일 위조 요청 11종 | 변경 전 11 실패, 변경 후 11 통과 |
| 결정적 시계/실제 두 맵 충돌 검사 | 36/36 통과 |
| 실제 소켓 이동·준비·자동 공격/폭발 | 11/11 통과 |
| 전투 요청, 협동전/보상 저장 실패 복구, 접근 제어 | 각각 19/19, 9/9, 9/9 통과 |
| 7종 무기, 7개 방 | 피해 이벤트 11건 × 방 인원 4 = 44건 전달, 누락/중복 없음 |
| 12회 연속 경기/방 변경 | 5개 불변식 통과 |
| 프레이밍/방 전달, 연결 수명 | 각각 8/8, 5/5 통과 |
| 인증, 인증 동시성, 동시 구매 | 각각 18/18, 6/6, 32/32 통과 |
| 느린 수신자 격리 | 정상 방 전달 300/300, 누락/중복 0 |
| 인증/CSV 충돌 복구, 6초 지연 초기화 구매 | 통과. 의도한 강제 종료 코드 73 후 복구 성공 |
| 송신 큐, 공격력 캐시 | 7/7 및 병렬 읽기/스냅샷 검사 통과 |
| Unity 순수 C# | world 9개 그룹, 전투 11개 그룹, 인증 9개 그룹, 프레이밍 4개 그룹 통과 |
| Unity Editor PlayMode/전체 빌드 | 미실행: 2022.3.40f1 미설치 |

추가 검사 명령:

```powershell
python tools/benchmark/test_auth.py --out <auth-unit>
python tools/benchmark/test_startup.py --out <startup>
python tools/benchmark/test_output.py --out <output>
python tools/benchmark/test_state.py --out <state>
python tools/benchmark/backpressure.py --candidate <after-build> --out <pressure> --repeats 1 --pressure-only --check
python tools/benchmark/stats_lookup.py --candidate <separate-build-copy> --out <lookup> --check-only
```

lookup 검사는 main.cpp를 교체하므로 별도 빌드 복사본에서 실행한다. 기존 framing/slow-reader
검사는 로비 전달 경로를 측정한다. 이를 다수의 동시 경기 시뮬레이션 성능 검증으로 해석하면 안 된다.

## 남은 검증과 후속 과제

- Unity 2022.3.40f1 Editor가 없어 PlayMode/전체 빌드는 실행하지 못했다. 설치된 6000.3.1f1로
  원본 프로젝트를 업그레이드하지 않았다. 순수 C# 검사와 구문 검사는 Unity 컴파일/플레이를 대신하지 않는다.
- 동일 Unity 버전에서 두 맵, 7종 무기, 풍선 10종, 개인전/협동전, 연속 재경기, 결과 화면,
  애니메이션 시점, 3D collider와 평면 판정의 차이, 높은 RTT에서 이동 보정을 확인해야 한다.
- 현재 클라이언트 예측은 단순 위치 보정이다. 입력 재실행/보간 시각 동기화/지연 보상은 없다.
- 준비 완료를 보내지 않는 연결에 대한 경기 시작 제한 시간, 연결 세대 관리, 경기 중 이탈 승패,
  서버 재시작 복원, 요청 빈도 제한, 많은 방에서의 CPU/락/송신량 측정은 후속 과제다.
- TCP는 여전히 TLS가 아니다. 외부 네트워크 자격 증명 보호는 완성되지 않았다.
  이전 평문 계정 파일/Git 이력 및 비밀번호 교체 정책도 기존 과제로 남아 있다.
- main 병합은 수행하지 않았다. 실제 플레이 검증 전 최종 호환성 통과로 취급하지 않는다.
