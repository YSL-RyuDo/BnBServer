# 인증된 연결과 게임 명령의 접근 경계

후속 리뷰에서 지적한 `HIT` 사칭을 실제 서버/합성 계정으로 재현한 뒤 수정했다.
Full IOCP 전환이나 게임 콘텐츠 변경 없이 `ClientHandler::ProcessMessages`에 적용했다.

## 변경

- 매 요청에서 현재 TCP 연결이 로그인 세션 맵의 동일한 연결인지 확인한다.
  단순히 패킷 닉네임이나 비어 있지 않은 ID를 신뢰하지 않는다.
- 인증 전에는 `LOGIN`, `REGISTER`, `QUIT`, 준비 상태 확인용 `GET_ROOM_LIST`만 허용한다.
  인증 전 방 목록 응답은 항상 빈 `ROOM_LIST|`이며 실제 방/플레이어 정보를 제공하지 않는다.
  다른 요청에는 고정된 `AUTH_REQUIRED|` 응답을 보내고 실행하지 않는다.
- `HIT` 공격자 닉네임을 인증된 세션 닉네임과 대조하고, 공격자 ID는 현재 세션에서 가져온다.
  대상이 존재하고 공격자와 같은 방의 구성원인지도 검사한다.
- 정상 인증된 플레이어의 기존 `HIT|weaponIndex|attackerNick|targetNick` 형식과
  기존 피해량/팀 판정은 유지한다.

## 동일 테스트 변경 전후

`tools/benchmark/access_regression.py`는 실제 TCP 연결 9개로 두 방과 비로그인 연결을 만든다.
요청과 방 목록 응답의 FIFO 순서를 이용하여 관찰 대상 연결의 송신 큐까지 비운 뒤 검사한다.
임의 sleep 이후 결과를 추측하지 않는다.

| 경우 | 변경 전 | 변경 후 |
| --- | --- | --- |
| 비로그인 연결의 HIT | 피해 패킷 4개, 실패 | 0개, 통과 |
| 같은 방 다른 플레이어 사칭 | 4개, 실패 | 0개, 통과 |
| 다른 방 플레이어의 공격자 사칭 | 4개, 실패 | 0개, 통과 |
| 다른 방 대상 공격 | 4개, 실패 | 0개, 통과 |
| 존재하지 않는 대상 공격 | 4개, 실패 | 0개, 통과 |
| 정상 공격 | 4개, 통과 | 4개, 통과 |
| 비로그인 프로필 요청 | 명시적 인증 거부 없음, 실패 | AUTH_REQUIRED, 통과 |
| 비로그인 구매 요청 | 명시적 인증 거부 없음, 실패 | AUTH_REQUIRED, 통과 |
| 로그아웃 후 HIT 사칭 | 3개, 실패 | 0개, 통과 |

```powershell
python tools/benchmark/access_regression.py --build ../validation/final --out ../validation/access-before
python tools/benchmark/build.py --out ../validation/access-build --port 39117
python tools/benchmark/access_regression.py --build ../validation/access-build --out ../validation/access-final
```

기준 바이너리 `final`은 접근 제어 수정 전 인증 해시 버전이다. 변경 전 1/9, 변경 후 9/9 통과.
동일 변경 후 인증 18/18, 동시 인증 6/6, 프레이밍 8/8, 연결 수명 5/5,
구매 32/32, 기존 피격 전달 28/28, 방 상태 5/5도 통과했다.
접근 경계 검사는 서버 GitHub Actions에 등록했다.

## 남은 범위

이 수정은 모든 게임 명령의 서버 권한 검증 완료를 뜻하지 않는다. 인증된 공격자의
거리/위치/충돌/쿨다운/무기 소유권과 다른 명령의 소유권 검증은 후속 작업이다.
IP·계정별 시도 제한과 인증 대기 정책도 아직 구현하지 않았다.
TLS, 기존 평문 파일·Git 이력 처리, 실제 Unity Editor 검증은 기존 보고서의 미해결 항목으로 남는다.
원본 계정 데이터는 변경하지 않았고 main에 병합하지 않았다.
