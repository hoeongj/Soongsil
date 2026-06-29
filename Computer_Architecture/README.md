# 🖥️ Computer Architecture (컴퓨터 구조)

32비트 **RISC-V 기계어를 디스어셈블**하고, 디코딩한 명령어를 가상 CPU 위에서 **직접 실행(시뮬레이션)** 하는 프로그램을 C로 구현한 과목입니다. 명령어 인코딩, 즉치값 처리, fetch-decode-execute 사이클 등 프로세서 동작 원리를 코드로 구현했습니다.

> **기술 스택:** C · RISC-V ISA(RV32I) · 비트 연산 · 파일 I/O

소스: [`src/main.c`](./src/main.c) · 보고서: `report.pdf`

---

## 과제 목표
입력 파일에 담긴 **32비트 이진 문자열(기계어)** 들을 읽어,
1. 각 명령어를 RISC-V 어셈블리로 **역어셈블(disassemble)** 하여 `.s` 파일로 출력하고,
2. 가상 CPU에서 **실행**하여 최종 레지스터(PC, x1~x5) 값을 출력한다.
잘못된 형식의 입력은 `Instruction Format Error!`로 처리한다.

## 구현 내용

### 1. 입력 검증 & 파싱
- 각 줄이 정확히 32자의 `0`/`1`로만 구성되었는지 검사하고, `parseBinary()`로 비트 필드(opcode, funct3, funct7, rd, rs1, rs2, imm)를 추출.

### 2. 명령어 디코딩 & 디스어셈블 (5개 타입)
- **R-type**(opcode 51): `ADD/SUB/SLL/XOR/SRL/SRA/OR/AND` — funct3·funct7 조합으로 구분
- **I-type**(opcode 19): `ADDI/XORI/ORI/ANDI/SLLI/SRLI/SRAI` — 12비트 즉치값 **부호 확장**
- **Load**(opcode 3): `LW`
- **Store**(opcode 35): `SW` — 분산된 즉치값 비트 재조합
- **Branch**(opcode 99): `BEQ/BNE/BLT/BGE` — **B-type 즉치값**(imm[12|10:5|4:1|11])을 비트 단위로 재조합하고 부호 확장
- 분기 명령의 **대상 주소에 `label`을 자동 생성**하여 어셈블리 가독성을 확보.

### 3. CPU 시뮬레이션 (fetch-decode-execute)
- 32개 정수 레지스터 + PC(시작값 1000)를 갖는 `CPU` 구조체를 정의하고, 초기 레지스터(x1~x5)를 1~5로 설정.
- PC를 기준으로 명령어를 인출 → 디코딩 → 실행하는 루프를 돌며, R/I 연산은 실제 산술·논리·시프트 연산을 수행하고, Branch는 조건 성립 시 PC를 분기 오프셋만큼 이동.
- `x0`(zero 레지스터)에는 쓰기를 무시하고, Store로 메모리에 쓰는 값은 과제 명세에 따라 무시하는 등 **ISA 규약**을 반영.

### 핵심 기술
- RISC-V 명령어 인코딩(R/I/S/B 타입)과 **비트 필드 추출·재조합**
- 2의 보수 **부호 확장**(immediate sign-extension)
- 분기 대상 라벨링을 통한 디스어셈블러 구현
- **fetch-decode-execute** 명령어 실행 사이클 시뮬레이션

## 실행
```bash
gcc -o riscv_sim src/main.c
./riscv_sim
# >> Enter Input File Name: <기계어파일>   (입력 시 같은 이름의 .s 파일 생성 + 최종 레지스터 출력)
# 'terminate' 입력 시 종료
```
