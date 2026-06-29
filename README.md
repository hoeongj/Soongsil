# 🎓 숭실대학교 컴퓨터학부 전공 과제 포트폴리오

> **홍성주 (Hong Seong-ju)** · 학번 20221528 · Soongsil University, School of Computer Science & Engineering

학부 전공 수업에서 직접 수행한 과제와 프로젝트를 **과목별로 정리한 포트폴리오**입니다.
시스템 프로그래밍(C), 알고리즘·자료구조(C++/Java), 딥러닝(PyTorch), 네트워크 프로그래밍(Python),
데이터 분석, 프로그래밍 언어론(인터프리터 구현), 컴퓨터 구조(RISC-V 시뮬레이터) 등
컴퓨터공학 전반을 아우르는 결과물을 담고 있습니다.

각 과목 폴더의 `README.md`에 **과제 목표 → 구현 내용 → 핵심 기술 → 결과**가 상세히 정리되어 있습니다.

---

## 📌 한눈에 보기

| 과목 | 핵심 주제 | 언어 / 기술 | 바로가기 |
|------|-----------|-------------|----------|
| **Linux System Programming** | 자동 채점 시스템, MD5 기반 중복파일 탐색·정리 도구(`fdupes` 류, ~2,300줄) | C, POSIX, pthread, OpenSSL, BFS | [📂](./Linux_System_Programming) |
| **Programming Languages** | 직접 만든 미니 언어의 **렉서 + 재귀하향 파서 + AST 트리워킹 인터프리터** | Python, C++ | [📂](./Programming_Languages) |
| **Computer Architecture** | **RISC-V 기계어 디스어셈블러 + 명령어 시뮬레이터** | C, RISC-V ISA | [📂](./Computer_Architecture) |
| **Artificial Intelligence** | CNN·**Transformer(from scratch)**·**ViT** 구현 및 학습 | PyTorch | [📂](./Artificial_Intelligence) |
| **Algorithm** | **이동 의미론(move semantics)** 기반 정렬 4종 (복사 비용 최소화) | C++ (STL, std::move) | [📂](./Algorithm) |
| **Network Programming** | 소켓·**TLS**·**asyncio**·**ZeroMQ** 분산 처리·HTTP/Flask·웹 스크래핑 | Python | [📂](./Network_Programming) |
| **File Processing** | 저수준 파일 I/O, **FTL(Flash Translation Layer)**, 가변길이 레코드 DB | C, POSIX syscalls | [📂](./File_Processing) |
| **Data Structures** | 스택 기반 수식 계산기, 정렬 4종, 동적 배열 클래스 | Java | [📂](./Data_Structures) |
| **Object-Oriented Programming** | 캡슐화·상속·다형성·추상클래스·제네릭 컬렉션 | Java | [📂](./Object_Oriented_Programming) |
| **Data Analysis** | 표 연산 → 시뮬레이션·확률 → 가설검정·A/B 테스트 → 회귀 (Berkeley *Data 8*) | Python, NumPy, `datascience` | [📂](./Data_Analysis) |
| **Computer Networks** | TCP / UDP 소켓 통신 프로그램 | Java (Socket, DatagramSocket) | [📂](./Computer_Networks) |
| **p5.js** | 크리에이티브 코딩 / 인터랙티브 그래픽스 | JavaScript, p5.js | [📂](./p5.js) |

---

## ⭐ 대표 프로젝트

### 1. MD5 기반 중복 파일 탐색·정리 도구 — `ssu_find-md5` (Linux System Programming)
- 디렉토리 트리를 **BFS**로 순회하며 후보 파일을 수집하고, **OpenSSL EVP** 인터페이스로 파일을 청크 단위로 읽어 **MD5 해시**를 계산해 동일 내용 파일을 하나의 "중복 세트"로 묶는 `fdupes` 스타일 CLI 도구.
- 확장자/크기(B·KB·MB·GB)/접근·수정·변경 시각(`YYYY:MM:DD:HH`, 윤년 처리 포함)/권한(8진수)/하드링크/제외 디렉토리 등 **풍부한 필터 옵션**과, 보존 규칙(newest·oldest·최단·최장 경로)에 따른 중복 삭제 기능 구현.
- 배열 없이 **연결 리스트만으로 삽입 정렬**, 메모리 누수 없는 동적 자료구조 관리. 단일 소스 **약 2,300줄**.
- 👉 [자세히 보기](./Linux_System_Programming)

### 2. 미니 프로그래밍 언어 인터프리터 (Programming Languages)
- 변수/상수 선언, `repeat-until` 반복문, `select-else` 조건문, 산술·비교 연산을 지원하는 미니 언어를 위해 **렉서(정규식 토크나이저) → 재귀하향 파서 → AST → 트리워킹 인터프리터** 파이프라인을 전부 직접 구현.
- 연산자 우선순위, 단항 부정, 괄호, 상수 재할당 금지 등 **문법·의미 규칙**을 파서·평가기 수준에서 처리. **Python · C++ 두 버전**으로 구현.
- 👉 [자세히 보기](./Programming_Languages)

### 3. RISC-V 디스어셈블러 + 명령어 시뮬레이터 (Computer Architecture)
- 32비트 RISC-V 기계어(이진 문자열)를 읽어 **R/I/Load/Store/Branch 타입**을 디코딩하고, 어셈블리(`.s`)로 역어셈블(분기 대상에는 라벨 자동 생성).
- 이어서 32개 레지스터 + PC를 갖는 CPU 모델 위에서 **fetch–decode–execute 루프**로 실제 실행하여 최종 레지스터 값을 출력. 즉치값 부호 확장, B-type 즉치값 재조합 등 ISA 디테일 구현.
- 👉 [자세히 보기](./Computer_Architecture)

### 4. 딥러닝: CNN · Transformer(from scratch) · ViT (Artificial Intelligence)
- **CNN** 이미지 분류기를 PyTorch로 학습 → **테스트 정확도 82%** (10,000장).
- **Transformer를 밑바닥부터 구현** (Positional Encoding, Multi-Head Attention 등) — 100 epoch 학습으로 loss 약 3.33까지 수렴.
- **Vision Transformer(ViT)** 구현 (Patch Embedding 등) → **테스트 정확도 85.06%**.
- 👉 [자세히 보기](./Artificial_Intelligence)

### 5. 이동 의미론 기반 정렬 알고리즘 (Algorithm)
- 삽입·선택·병합·퀵 정렬을 C++로 구현하되, 생성자/복사/이동/소멸 횟수를 세는 **계측형 타입(`MyInteger`)** 위에서 동작시켜 **`std::move`로 불필요한 복사를 제거**했음을 정량적으로 검증.
- 👉 [자세히 보기](./Algorithm)

### 6. 고급 네트워크 프로그래밍 (Network Programming)
- 원시 TCP/HTTPS 소켓, **TLS 에코 서버**, `pickle`+`struct` 직렬화, **asyncio 비동기 채팅 릴레이**, **ZeroMQ + memcached 분산 작업 처리**, HTTP 사전 서버, **Flask REST API + BeautifulSoup 웹 스크래핑**까지.
- 👉 [자세히 보기](./Network_Programming)

---

## 🧰 기술 스택

- **Languages:** C, C++, Java, Python, JavaScript
- **Systems / Low-level:** POSIX 시스템 콜, 파일 I/O, pthread, 시그널, OpenSSL(MD5), 플래시 메모리(FTL), RISC-V
- **AI / Data:** PyTorch, NumPy, Matplotlib, `datascience`(Berkeley Data 8)
- **Networking:** TCP/UDP 소켓, TLS/SSL, asyncio, ZeroMQ, memcached, Flask, HTTP
- **CS Fundamentals:** 자료구조, 알고리즘, 객체지향 설계, 컴파일러/인터프리터, 운영체제·컴퓨터구조 개념

---

## 🗂️ 저장소 구조

```
Soongsil/
├── Algorithm/                     # C++ 정렬 (이동 의미론)
├── Artificial_Intelligence/       # PyTorch (CNN, Transformer, ViT)
├── Computer_Architecture/         # RISC-V 디스어셈블러 + 시뮬레이터
├── Computer_Networks/             # Java TCP/UDP 소켓
├── Data_Analysis/                 # Berkeley Data 8 랩 (22개)
├── Data_Structures/               # 수식 계산기, 정렬, 동적 배열
├── File_Processing/               # 저수준 I/O, FTL, 가변길이 레코드
├── Linux_System_Programming/      # 채점 시스템, 중복파일 정리 도구
├── Network_Programming/           # 소켓·TLS·async·ZeroMQ·Flask
├── Object_Oriented_Programming/   # Java OOP
├── Programming_Languages/         # 미니 언어 인터프리터
└── p5.js/                         # 크리에이티브 코딩
```

각 폴더의 `README.md`에서 과제별 상세 설명을 확인하실 수 있습니다.

---

## 🔧 빌드 & 실행

과제별 빌드/실행 방법은 각 과목 README에 기재되어 있습니다. 대표 예시는 다음과 같습니다.

```bash
# C (Linux System Programming, File Processing)
make                      # 제공된 Makefile 사용
gcc -o out src.c          # 단일 파일 직접 컴파일

# C++ (Algorithm)
g++ -std=c++17 -O2 testmain.cpp -o sort_test

# Java (Data Structures, OOP, Computer Networks)
javac myCalculator.java && java myCalculator

# Python (Programming Languages, Network Programming)
python interpreter.py

# Jupyter (Artificial Intelligence, Data Analysis)
jupyter notebook   # 또는 Google Colab 업로드
```

> 📝 소스코드 내 한글 주석은 모두 **UTF-8**로 정리되어 GitHub에서 정상적으로 표시됩니다.
> 보고서(PDF/DOCX)와 실행 화면 캡처도 함께 포함하여 과제 수행 내용을 검증할 수 있도록 했습니다.
