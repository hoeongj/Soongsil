# 🖥️ Computer Architecture (컴퓨터 구조)

32비트 **RISC-V 기계어를 디스어셈블**하고, 디코딩한 명령어를 가상 CPU 위에서 **직접 실행(시뮬레이션)** 하는 프로그램을 C로 구현한 과목입니다. 명령어 인코딩, 비트 필드 추출, 즉치값 부호 확장, fetch-decode-execute 사이클 등 프로세서 동작 원리를 코드로 구현했습니다.

> **기술 스택:** C · RISC-V ISA(RV32I) · 비트 연산(시프트·마스크) · 파일 I/O

소스: [`src/main.c`](./src/main.c) · 보고서: `report.pdf`

---

## 과제 목표
입력 파일의 **32비트 이진 문자열(기계어)** 들을 읽어 ① RISC-V 어셈블리로 **역어셈블**하여 `.s` 파일로 출력하고, ② 가상 CPU에서 **실행**하여 최종 레지스터(PC, x1~x5) 값을 출력한다. 형식 오류는 `Instruction Format Error!`로 처리한다.

## 구현 내용

### 1. 입력 검증 & 비트 필드 추출
- 각 줄이 정확히 **32자의 `0`/`1`** 인지 검사. `parseBinary(str, start, len)`로 임의 비트 구간을 정수로 변환하여 opcode/funct3/funct7/rd/rs1/rs2/imm 추출.

### 2. 명령어 디코딩 & 디스어셈블 — 5개 타입 / 20+ 명령어

| 타입 | opcode | 지원 명령어 | 핵심 처리 |
|------|--------|-------------|-----------|
| **R-type** | 51 | ADD, SUB, SLL, XOR, SRL, SRA, OR, AND | funct3 + funct7 조합으로 구분 (예: funct3=0 & funct7=32 → SUB) |
| **I-type** | 19 | ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI | 12비트 즉치값 **부호 확장**, shift는 하위 5비트만 사용 |
| **Load** | 3 | LW | `offset(rs1)` 주소 계산 형식 |
| **Store** | 35 | SW | 분산된 즉치값 비트(imm[11:5], imm[4:0]) **재조합** |
| **Branch** | 99 | BEQ, BNE, BLT, BGE | **B-type 즉치값**(imm[12\|10:5\|4:1\|11]) 비트 재조합 + 부호 확장 |

- **유효성 검증:** R/I 타입의 funct7 조합이 정의되지 않은 값이면 미지원 명령으로 판정(`is_unknown`).
- **라벨 자동 생성:** 분기 대상 인덱스를 1차 스캔에서 계산해 `label1:`, `label2:` … 를 부여 → 어셈블리 가독성 확보(2-pass 방식).

### 3. CPU 시뮬레이션 (fetch-decode-execute)
- `CPU` 구조체(32개 정수 레지스터 + PC). PC 시작값 1000, x1~x5를 1~5로 초기화.
- `pc_idx = (pc - 1000) / 4`로 명령어 인출 → 디코딩 → 실행. R/I는 산술·논리·시프트 수행, Branch는 조건 성립 시 `pc += imm`.
- **ISA 규약 반영:** `x0`(zero) 쓰기 무시, SRL/SRA의 논리/산술 시프트 구분(`unsigned` 캐스팅), Store가 메모리에 쓰는 값은 명세에 따라 무시.

### 핵심 기술
- RISC-V 명령어 인코딩(R/I/S/B 타입)과 **비트 필드 추출·재조합**
- 2의 보수 **부호 확장**(immediate sign-extension), 논리 vs 산술 시프트
- 2-pass 디스어셈블러(분기 대상 라벨링)
- **fetch-decode-execute** 명령어 실행 사이클 시뮬레이션

## 실행
```bash
gcc -o riscv_sim src/main.c
./riscv_sim
# >> Enter Input File Name: <기계어파일>   →  같은 이름의 .s 생성 + 최종 레지스터(PC,x1~x5) 출력
# 'terminate' 입력 시 종료
```
