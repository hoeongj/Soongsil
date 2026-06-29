# 🗄️ File Processing (파일 처리)

저수준 파일 입출력 시스템 콜부터 **플래시 메모리(FTL)** 시뮬레이션, **가변길이 레코드 기반 파일 데이터베이스**까지, 파일을 다루는 여러 계층을 C로 구현한 과목입니다.

> **기술 스택:** C · POSIX 파일 I/O(`open`/`read`/`write`/`lseek`) · `fopen`/`fread`/`fwrite` 바이너리 I/O · 비트/바이트 단위 데이터 처리

---

## 📂 01. 저수준 파일 I/O (`01_LowLevel_File_IO/`)
파일 디스크립터 기반 시스템 콜(`open`/`read`/`write`/`lseek`)만으로 파일을 조작하는 유틸리티 모음입니다.

| 파일 | 기능 | 핵심 |
|------|------|------|
| `copy.c` | 파일 복사 | 버퍼 단위 `read`→`write` 루프 |
| `insert.c` | 지정 위치에 데이터 삽입 | `lseek`로 뒷부분 밀어내기 |
| `delete.c` | 지정 구간 바이트 삭제 | 뒤 데이터 당겨오기 + `ftruncate` |
| `overwrite.c` | 지정 위치 덮어쓰기 | `lseek` + `write` |
| `read.c` | 지정 범위 읽기 | `lseek`로 시작점 이동 후 범위 read |
| `merge.c` | 두 파일 병합 | 순차 append |
| `rprint.c` | 파일 내용 역순 출력 | 끝에서부터 `lseek(SEEK_END)` 역방향 |

- **핵심:** `lseek`로 파일 오프셋을 직접 제어하고, stdio 버퍼링 없이 **시스템 콜 수준에서 바이트를 조작**. 인자 검증·에러 처리 포함.

## 📂 02. FTL — Flash Translation Layer (`02_Flash_Translation_Layer/`)
플래시 메모리의 물리적 특성을 반영한 **FTL 기능**을 구현했습니다. (`ftl.c`)

- **과제 목표:** 플래시 에뮬레이터에 **페이지 단위 읽기/쓰기, 블록 단위 소거**를 수행하고, **in-place update**로 갱신을 구현한다.
- **구현 내용 (명령별):**
  - `c`(생성): 블록 수만큼 `fdd_erase`로 `0xFF` 초기화 → 플래시 파일 생성
  - `w`(쓰기): 페이지 버퍼를 `0xFF`로 초기화 후 **섹터 데이터 + 스페어(정수, binary)** 기록
  - `r`(읽기): 페이지를 읽어 섹터/스페어 데이터를 **분리** 출력
  - `e`(소거): 블록 단위 `fdd_erase`
  - **`u`(in-place update):** 플래시는 덮어쓰기 불가 → ① 대상 블록 유효 페이지를 **예약 블록으로 복사** → ② 원본 블록 소거 → ③ 예약 블록에서 복원 → ④ 빈 자리에 새 데이터 기록 → ⑤ 예약 블록 소거. **read/write/erase 횟수를 카운트**해 출력.
- **핵심:** *erase-before-write* 제약, 페이지/블록 단위 연산, 스페어 영역, 가비지 컬렉션과 유사한 블록 재배치(copy-back).

## 📂 03. 가변길이 레코드 파일 DB (`03_VariableLength_Records/`)
파일 하나를 데이터베이스처럼 다루어 **가변길이 학생 레코드**의 삽입·삭제·검색을 구현합니다. (`student.c`, `student.h`)

- **레코드 구조:** `길이 지시자(2B) + id#name#dept#addr#email#` (`#` 구분자 직렬화).
- **파일 헤더:** `레코드 수(4B) + 리스트 헤드(4B)` — **삭제된 레코드를 잇는 free list**의 시작점.
- **구현한 연산:**

  | 함수 | 기능 | 핵심 |
  |------|------|------|
  | `insert` | 레코드 추가 | **free list에 재사용 가능한 공간이 있으면 거기에, 없으면 파일 끝에 append** |
  | `delete` | 필드 조건으로 삭제 | 해당 레코드를 free list에 연결(공간 재사용 대비) |
  | `search` | 필드 조건으로 검색 | `parseCondition`으로 `field=value` 질의 파싱 후 매칭 |
  | `makeRecordData`/`parseRecord` | 직렬화/역직렬화 | 길이 지시자·`#` 구분자 처리, `copyField` |

- **핵심:** 고정길이가 아닌 **가변길이 레코드 직렬화**, 길이 지시자, **free list 기반 삭제 공간 재사용**, 필드 기반 질의 — 파일 기반 저장 엔진의 축소판.

---

## 핵심 기술
- POSIX 시스템 콜(`open`/`read`/`write`/`lseek`/`ftruncate`)과 바이너리 파일 I/O
- 플래시 메모리 특성(erase-before-write, 페이지/블록)과 **FTL in-place update(copy-back)**
- 가변길이 레코드 직렬화/역직렬화, 파일 헤더·**free list** 설계
- 바이트/오프셋 단위 정밀 데이터 조작

> 📝 FTL은 강의 제공 프레임워크(`flash.h`, `fdevdriver.c`)의 디바이스 드라이버 인터페이스(`fdd_read`/`fdd_write`/`fdd_erase`)를 사용하며, 직접 작성한 `ftl.c`가 포함되어 있습니다.

## 실행
```bash
gcc -o student 03_VariableLength_Records/student.c   # 가변길이 레코드 DB
gcc -o copy 01_LowLevel_File_IO/copy.c               # 저수준 I/O 유틸 (각 파일 개별 컴파일)
```
