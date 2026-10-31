# 인증 보안 고도화 — 2026-11-01

대상: `YSL-RyuDo/BnBServer`, `YSL-RyuDo/BNB`의 `feature/auth-hardening`.
서버 기준 커밋: `7394d2e424f2cb7b7cfad94ba50ddd2c61261f93`.
클라이언트 기준 커밋: `d98ea1da298f3c31713407db6875702ed8383a96`.

Hybrid Winsock 수신 + IOCP 송신 큐를 유지했다. 게임 콘텐츠, 패킷 프레이밍,
이동/전투/방/상점 규칙을 바꾸지 않았다. 이 작업에서 저장소의 실제 계정 CSV로
서버를 실행하거나 계정을 생성하지 않았다. 모든 실행 데이터는 별도 디렉터리의 합성 계정이다.
기존 계정 및 게임 CSV는 수정·삭제하지 않았다.

## 발견한 흐름과 문제

1. `Server::Run`은 작업 디렉터리 잠금과 CSV journal 복구 후 사용자 테이블 7개를 읽는다.
2. `ClientHandler::HandleClient`가 줄 단위 TCP 프레임을 조립하고 `ProcessMessages`에 전달한다.
3. 종전 `UserManager::CheckLogin`은 메모리의 평문 비밀번호와 입력을 직접 비교했다.
   `LOGIN_SUCCESS`는 ID, 비밀번호, 닉네임을 반환했고 서버의 일반 수신/송신 로그가 이를 출력했다.
   회원가입 파싱 로그와 계정 CSV 파싱 실패 로그에도 자격 증명이 노출됐다.
4. `RegisterUser`는 계정, 프로필, 캐릭터, 이모트, 전적, 풍선, 아이콘을 기존 트랜잭션으로 저장한다.
   계정 파일에 평문 비밀번호를 추가했다.
5. Unity는 `AuthSender`/`LoginUI`/`RegisterUI` → `AuthReceiver` 경로와
   `LoginSystem`/`RegisterSystem` 경로를 모두 가진다. 두 로그인 파서가 세 필드를 요구했다.
   오류 응답은 서버가 `WRONG_PASSWORD|`처럼 보내는데 일부 UI는 구분자 없는 문자열과 비교했다.
6. Unity의 방 입장 UI 두 곳에서도 방 비밀번호를 로그로 출력했다.

## 구현과 변경 함수

| 파일 | 함수/변경 |
| --- | --- |
| `TeamOFYServer/PasswordHash.h` | `Create`, `Verify`, `Derive`, `Parse`, `IsValid`, `ValidField`: CNG 암호화, 엄격한 형식/입력 검증, 동시 계산 제한 |
| `TeamOFYServer/UserAccount.h` | `password` 대신 해시 전용 `passwordHash` 필드 |
| `TeamOFYServer/UserManager.cpp` | `LoadAccountUsers`: 원본 보존 별도 v2 저장소 전환; `CheckLogin`: 해시 검증 및 비밀번호 없는 응답; `RegisterUser`: 저장 전 해싱; `RegisterUserAccount`, `SaveUserAccount`: 해시만 v2에 저장 |
| `TeamOFYServer/CsvStore.h` | `Allowed`: v2 파일을 기존 원자적 저장/복구 대상에 추가 |
| `TeamOFYServer/Server.cpp` | `Run`: 계정 저장소 초기화 실패 시 접속 처리 중단, 고정 오류 문구 |
| `TeamOFYServer/ClientHandler.cpp` | `ProcessMessages`: 새 응답 파싱, 동일 연결 재로그인 차단, 원문/회원가입 입력 로그 제거; `HandleClient`: 예외 원문 로그 제거; `SendToClient`: 응답 본문 로그 제거 |
| 프로젝트 파일 및 `.gitignore` | 새 헤더 포함, 런타임 v2 계정 파일 제외 |
| `tools/benchmark/auth_regression.py` | 동일한 변경 전후 소켓 검사 18개와 독립 PBKDF2 검증 |
| `tools/benchmark/auth_concurrency.py` | 동시 로그인, 연결 정리, 분할/병합 인증 프레임 검사 6개 |
| `tools/benchmark/auth_tests.cpp`, `test_auth.py` | 실제 CNG/UserManager/CsvStore의 오류·복구 검사 |

Unity의 전체 변경 목록은 동반 저장소의 `docs/AUTH_SECURITY.ko.md`에 기록했다.

## 암호화와 저장 구조

암호화 구현은 Windows CNG의 `BCryptDeriveKeyPBKDF2`와 `BCryptGenRandom`을 사용한다.
PBKDF2-HMAC-SHA256, 600,000회, 계정별 16바이트 무작위 salt, 32바이트 결과이다.
직접 구현한 해시나 PRNG는 없다. 결과 비교는 고정 길이를 모두 순회한다.
형식은 현재 지원하는 알고리즘/반복 수로 엄격히 제한하므로 임의 반복 수에 의한 계산 폭증이나
알 수 없는 형식의 평문 대체 검증을 허용하지 않는다. 일부 임시 버퍼를 지우지만
프로세스 전체 메모리에서 자격 증명이 완전히 제거된다고 보장하지는 않는다.

근거: [Microsoft PBKDF2 API](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptderivekeypbkdf2),
[Microsoft RNG API](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom),
[OWASP Password Storage](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html).
OWASP의 PBKDF2-HMAC-SHA256 작업 계수를 적용했고 기존 Windows 빌드에 포함되는 OS API를 선택했다.
이는 FIPS 인증을 받았다는 주장이 아니다.

이전 파일(보존):

```text
UsersAccount.csv
id,password,nickname
<id>,<plaintext>,<nickname>
```

새 운영 파일:

```text
UsersAccount.v2.csv
id,password_hash,nickname
<id>,$pbkdf2-sha256$600000$<salt:32 hex>$<derived key:64 hex>,<nickname>
```

프로필/소유 아이템/전적 테이블과 계정 ID, 닉네임은 유지한다.
신규 회원가입은 v2 계정 파일과 나머지 6개 테이블을 기존 journal 트랜잭션으로 함께 저장한다.
해시 계산 중에는 사용자 상태 및 연결 맵 잠금을 잡지 않는다. 동시에 최대 2개 KDF만 수행하며
포화 또는 암호화 API 오류는 `AUTH_ERROR|`로 실패한다. 로그인 세션 등록은 검증 후 연결 맵 잠금
안에서 다시 확인하므로 두 연결이 동시에 같은 계정을 인증해도 한 연결만 등록된다.

## 전환 및 복구 절차

- 서버 시작 시 v2가 없으면 기존 CSV를 읽고 전체 행/중복/필드를 검증한 뒤 모든 비밀번호를
  해싱하여 v2 파일만 생성한다. 원본 CSV를 변경하거나 평문 백업 사본을 추가하지 않는다.
  계정 수에 비례해 첫 시작이 느려질 수 있다. 대규모 운영 데이터의 시간은 측정하지 않았다.
- v2가 존재하면 v2만 읽는다. 손상된 헤더, 해시, 중복 계정, 읽기 실패 시 접속 처리를 시작하지
  않는다. 이때 평문 원본으로 되돌아가지 않는다. v2를 수동 삭제하면 최초 전환으로 간주되므로
  운영 중 v2 삭제/누락을 복구 방식으로 사용해서는 안 된다.
- 프로세스 단위 `DirectoryLease`를 유지한다. `users.redo`가 내구성 있는 commit point이며
  저장 중 중단되면 다음 시작 시 기존 복구 로직이 v2 기록을 재적용한다.
- 배포는 서버 정지/기존 journal 상태 확인 → 접근 제한된 일관된 전체 데이터 백업 →
  서버와 Unity 동시 배포 → 기존 계정 로그인·프로필·아이템 확인 순서로 수행한다.
  이 문서는 절차를 제시하며 실제 운영 배포/백업/마이그레이션은 수행하지 않았다.
- 신규 쓰기 이전에는 보존한 원본과 이전 바이너리로 되돌릴 수 있다. 신규 계정/게임 상태가
  추가된 뒤에는 이전 원본만 복원하면 안 된다. v2와 최신 6개 테이블, journal을 보존하고
  v2를 읽는 서버로 복구해야 한다. 이전 서버는 새 해시 계정을 읽지 못하며 해시를 평문으로
  변환하는 도구는 제공하지 않는다.
- 기존 `UsersAccount.csv`와 Git 이력에는 평문이 그대로 남는다. 원본 보존 요구에 따라 삭제하지
  않았다. 해당 데이터·백업·journal·v2 파일에 운영 OS 접근 권한을 제한해야 한다.
  이전 공개 노출은 해시 전환만으로 취소되지 않으므로 비밀번호 변경 및 저장소 이력 처리는
  별도 승인/작업으로 남긴다.

## 프로토콜 및 호환성

요청 형식은 기존과 같다: `LOGIN|id,password\n`, `REGISTER|id,password,nickname\n`.
기존 UI의 비밀번호 Trim 동작도 유지한다. 필드 안의 CSV/프로토콜 구분자 및 제어 문자 입력은 거부한다.

```text
이전: LOGIN_SUCCESS|id,password,nickname\n
이후: LOGIN_SUCCESS|id,nickname\n
오류: AUTH_ERROR|\n  (검증 입력 오류, 암호화 실패/포화)
```

기존 실패 코드와 중복 로그인 동작은 유지한다. 구버전 Unity와 새 서버는 로그인 성공 형식이
호환되지 않는다. 새 Unity도 비밀번호가 포함된 구형 성공 응답을 의도적으로 거부한다.
두 저장소의 변경을 함께 배포해야 한다. 새 계정 파일은 구버전 서버와 호환되지 않는다.

## 실행 결과와 재현 명령

환경: Windows, MSVC v143, Release x64/C++14, Python 3.14, psutil 7.2.2,
.NET SDK 9.0.318. 테스트 실행은 모두 IPv4 loopback과 합성 데이터에 한정했다.
실행 환경 psutil은 저장소 requirements의 7.1.3과 버전 차이가 있다.
출력 디렉터리는 저장소 밖의 `validation`을 사용했다. 아래 `python`은 테스트용 venv의 인터프리터이다.

```powershell
python tools/benchmark/build.py --out ../validation/before --port 39101 --ref 7394d2e424f2cb7b7cfad94ba50ddd2c61261f93
python tools/benchmark/auth_regression.py --build ../validation/before --out ../validation/auth-before
python tools/benchmark/regression.py --build ../validation/before --out ../validation/network-before
python tools/benchmark/build.py --out ../validation/final --port 39103
python tools/benchmark/auth_regression.py --build ../validation/final --out ../validation/auth-final
python tools/benchmark/auth_concurrency.py --build ../validation/final --out ../validation/auth-concurrency-final
python tools/benchmark/test_auth.py --out ../validation/auth-unit-final
python tools/benchmark/test_state.py --out ../validation/state-unit-final
python tools/benchmark/test_output.py --out ../validation/output-unit
python tools/benchmark/regression.py --build ../validation/final --out ../validation/network-final
python tools/benchmark/lifecycle.py --build ../validation/final --out ../validation/lifecycle-final
python tools/benchmark/purchase_regression.py --build ../validation/final --out ../validation/purchase-final
python tools/benchmark/hit_regression.py --build ../validation/final --out ../validation/hit-final
python tools/benchmark/room_state.py --build ../validation/final --out ../validation/rooms-final
python tools/benchmark/backpressure.py --candidate ../validation/final --out ../validation/pressure-final --repeats 1 --check
MSBuild.exe TeamOFYServer.sln /p:Configuration=Release /p:Platform=x64 /p:OutDir=<validation>/solution/bin/ /p:IntDir=<validation>/solution/obj/ /v:minimal /nologo
```

최초 baseline 빌드는 구현 전 동일 HEAD의 작업 트리에서 `--ref` 없이 실행했다.
위 `--ref` 명령은 수정 후에도 같은 기준을 재현하는 방법이다. 일부 테스트는 기존 출력 디렉터리
재사용을 거부하므로 재실행 시 새 이름을 사용한다.

| 검사 | 실제 결과 |
| --- | --- |
| 변경 전 서버 빌드 | 성공, exit 0 |
| 변경 전 인증 18개 | 13 통과 / 5 실패, exit 1: 성공 응답 비밀번호, 원본 변경, 해시 저장, salt, 로그 검사 실패 |
| 변경 전 네트워크 | 8/8 통과 |
| 최종 서버 솔루션/격리 빌드 | 성공, exit 0 |
| 최종 동일 인증 검사 | 18/18 통과, 독립 Python PBKDF2 대조 포함 |
| 인증 동시성/프레이밍 | 6/6 통과 |
| 인증 저장/복구 C++ | auth exit 0, 의도된 crash exit 73, recover exit 0, 모두 기대 결과 |
| 기존 상태/트랜잭션 C++ | state exit 0, 의도된 crash exit 73, recover exit 0 |
| 기존 송신 큐 C++ | 7/7 통과 |
| 최종 네트워크 프레이밍/격리 | 8/8 통과 |
| 연결 수명 | 5/5 통과 |
| 동시 구매 | 32/32 계정 잔액/소유권 일치 |
| 피격 회귀 | 28/28 전달, 다른 방 전달 0 |
| 방 상태 | 5/5 조건 통과 |
| 역압력 | 정상/압력 두 조건 모두 300/300 전달, 중복·누락 0, 큐 한도와 연결 종료 검사 통과 |
| Unity 인증 C# 및 기존 프레이밍 | 9개 PASS 그룹, 실제 인증 스크립트와 테스트 UI 대체 객체 사용 |
| Unity Editor/PlayMode/전체 플레이어 빌드 | 미실행: 프로젝트 2022.3.40f1 미설치, 설치된 Editor는 6000.3.1f1. 자동 업그레이드하지 않음 |

원문 인증 로그/CSV를 포함하지 않는 결과는 [측정 결과](measurements/2026-11-01-auth/results.json)에 보관한다.
처음 솔루션 빌드에 발생한 사용하지 않는 예외 변수 경고는 제거했고 최종 빌드는 성공했다.

## 미해결 및 다음 단계

- **TCP에 TLS가 없다.** 요청 비밀번호가 네트워크에서는 여전히 평문이다. 외부 네트워크에서
  자격 증명 보호가 완성됐다고 주장하지 않는다. 서버 인증서 검증을 포함한 전송 계층 보호가 후속 과제다.
- KDF 동시성 제한은 IP/계정별 로그인 시도 제한이 아니다. 지속적 인증 부하, 분산 공격,
  연결/스레드 수 제한 및 암호화 부하 성능 검증은 후속 과제다.
- 기존 UX 때문에 ID 없음/비밀번호 오류/중복 로그인을 구분한다. 계정 존재 여부 추측 가능성은 남는다.
- 원본 평문, 이전 Git 이력, 기존 로그/백업은 그대로다. 자동 삭제·계정 재설정·이력 재작성은 수행하지 않았다.
- 실제 Unity 2022.3.40f1의 장면 전환·렌더링·플레이 검증은 필요하다. C# 대체 객체 테스트를 실제 Unity 실행으로 간주하지 않는다.
- 이번 결과는 인증과 기존 회귀 검사 결과이며 전체 게임의 보안/성능을 입증하지 않는다.
  다음 순서는 서버 권한 전투 판정 → 클라이언트 요청 검증 → 연결 수명 관리 → 네트워크 성능 검증이다.
