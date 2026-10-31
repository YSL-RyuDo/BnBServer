# 인증 변경 후 원격 CI 시간 초과 수정

## 최초 실패

[GitHub Actions 실행 41219682491](https://github.com/YSL-RyuDo/BnBServer/actions/runs/41219682491),
커밋 `5ccc108375b9cae61b10397a00f770c228e926a6`의 `windows-sockets` 작업이 실패했다.
서버 빌드, 프레이밍, 송신 큐, 연결 수명, 역압력, 피격, CSV 복구, 방 상태 검사는 통과했다.
실패는 `purchase_regression.py`의 구매 요청 이전 로그인 준비 단계에서 발생한
`asyncio.exceptions.TimeoutError`이다. 공격 테이블 검사는 앞 단계 실패로 건너뛰었다.
실행 환경은 Windows Server 2022, Python 3.10.11, psutil 7.1.3이었다.

최초 보고서는 로컬 결과만 확인하고 푸시 후 원격 CI까지 확인하지 못했다.
메일의 작업 실패 표시는 개별 검사 전부가 실패했다는 의미는 아니다.

## 원인 및 재현

서버는 `Initialize`에서 `listen()`을 호출하고, `Run`에서 계정 CSV를 읽어 해시 전환을
마친 뒤 `AcceptClients`를 시작한다. 기존 테스트 `run.start`는 TCP 연결 성공만으로
초기화 완료라고 판단했다. 따라서 32개 합성 계정의 초기 해싱 시간이 첫 로그인 응답의
5초 제한에 포함됐다. 로컬 장비에서는 통과했지만 원격 CI에서는 해당 제한을 초과했다.

동일 서버의 격리 빌드에서 `listen()` 이후 `server.Run()` 이전에만 `Sleep(6000)`을
삽입하여 초기화 지연을 재현했다. 수정 전 구매 테스트는 exit 1과 `TimeoutError`로 실패했고,
수정 후 같은 바이너리·32개 합성 계정·구매 테스트는 32/32 통과했다.
프로덕션 서버 소스에는 지연을 추가하지 않았다.

로컬 재검증: 초기화 지연 구매 32/32, 일반 구매 32/32, 프레이밍 8/8,
연결 수명 5/5, 역압력 300/300 전달(누락/중복 0), 피격 28/28,
방 상태 5/5 조건, 공격 테이블 값/비정상 로드/동시 읽기 검사가 모두 통과했다.

## 수정

- `tools/benchmark/run.py::start`: 첫 연결에서 기존 `GET_ROOM_LIST|` 요청을 보내고
  `ROOM_LIST|` 응답까지 기다린 다음 테스트 준비 완료로 처리한다.
  초기화 대기는 최대 60초이며, 준비 완료 후 일반 요청의 5초 제한은 유지한다.
  준비 실패 시 자식 서버와 이미 열린 소켓을 정리한다.
- `tools/benchmark/test_startup.py`: 초기화 6초 지연을 넣은 별도 서버 빌드를 만들고
  실제 32계정 구매 회귀 테스트를 실행한다.
- `.github/workflows/socket-regression.yml`: 기존 검사를 유지하면서 인증 검사 18개,
  동시 인증/프레이밍 6개, 인증 저장 실패/복구 검사, 초기화 지연 검사를 추가한다.
  인증 원문 로그와 계정 CSV를 artifact에 포함하지 않는다.

암호화 강도, 인증 성공 조건, 구매 검증 조건, 게임 코드, 원본 계정 데이터는 변경하지 않았다.

## 재현 명령

```powershell
python tools/benchmark/test_startup.py --out ../validation/startup-regression
python tools/benchmark/regression.py --build ../validation/final --out ../validation/ci-fix-network
python tools/benchmark/lifecycle.py --build ../validation/final --out ../validation/ci-fix-lifecycle
python tools/benchmark/backpressure.py --candidate ../validation/final --out ../validation/ci-fix-pressure --repeats 1 --pressure-only --check
python tools/benchmark/hit_regression.py --build ../validation/final --out ../validation/ci-fix-hit
python tools/benchmark/room_state.py --build ../validation/final --out ../validation/ci-fix-room
python tools/benchmark/purchase_regression.py --build ../validation/final --out ../validation/ci-fix-purchase
python tools/benchmark/build.py --out ../validation/ci-fix-lookup-build --port 39116
python tools/benchmark/stats_lookup.py --candidate ../validation/ci-fix-lookup-build --out ../validation/ci-fix-lookup --check-only
```

`final`은 앞선 인증 변경의 격리 서버 빌드이다. 공격 테이블 검사는 빌드의 실행 파일을
검사용 main으로 교체하므로 별도 빌드 디렉터리를 사용한다.

수정된 테스트는 [feature 브랜치 Actions](https://github.com/YSL-RyuDo/BnBServer/actions/workflows/socket-regression.yml?query=branch%3Afeature%2Fauth-hardening)에서 다시 실행한다.
