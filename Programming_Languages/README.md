# 🧩 Programming Languages (프로그래밍 언어론)

직접 설계한 **미니 명령형 언어**를 해석·실행하는 인터프리터를 **렉서 → 파서 → AST → 평가기** 전 과정으로 구현한 과목입니다. 컴파일러/인터프리터의 프런트엔드 구조를 처음부터 끝까지 직접 작성했습니다.

> **기술 스택:** Python · C++ (동일 사양을 두 언어로 구현) · 정규식 토크나이저 · 재귀하향 파서(Recursive Descent) · AST 트리워킹 인터프리터

소스: [`interpreter.py`](./interpreter.py), [`interpreter.cpp`](./interpreter.cpp) · 보고서: `report.pdf`

---

## 과제 목표
한 줄로 주어지는 미니 언어 프로그램을 파싱·실행하여 `print` 결과를 출력하고, 문법 오류 시 `Syntax Error!`를 출력하는 인터프리터를 구현한다. 지원해야 하는 언어 요소:
- **선언:** `variable`(변수), `constant`(상수, 재할당 금지)
- **문장:** 대입, `print`, `repeat ... until(조건)` 반복문, `select(조건) ... else ...` 조건문
- **식:** 산술식(`+ - *`, 단항 부정, 괄호, 연산자 우선순위), 비교식(`== != < > <= >=`)
- **토큰 규칙:** 식별자/숫자 최대 길이 제한, 예약어 구분 등

## 구현 내용

### 1. 렉서 (Tokenizer)
- 정규식(`==|!=|<=|>=|[;()+\-*=<>]`)으로 연산자·구분자 주위에 공백을 삽입한 뒤 분리하는 방식으로 토큰열을 생성.
- `is_number` / `is_var_name` 로 숫자·식별자 유효성(길이·문자 범위·예약어 충돌)을 검사.

### 2. 재귀하향 파서 (Recursive Descent Parser)
- `parse_program → parse_declaration → parse_statement → parse_block → parse_aexpr → parse_term` 의 문법 규칙별 함수로 구성.
- **연산자 우선순위**를 문법 계층(`aexpr`는 `+ - *`, `term`은 단항 부정·괄호·원자)으로 자연스럽게 표현.
- 상수 재할당, 미선언 변수 사용, 중복 선언 등 **의미 오류를 파싱 단계에서 검출**하여 예외 처리.
- 파싱 결과를 **AST 노드 객체**(`NumberExpr`, `VarExpr`, `NegExpr`, `BinExpr`, `BoolExpr`, `AssignStmt`, `PrintStmt`, `RepeatStmt`, `SelectStmt`)로 구성.

### 3. 트리워킹 인터프리터 (Evaluator)
- 각 AST 노드가 `eval(env)`(식) / `execute(env, output)`(문)을 구현하는 **인터프리터 패턴**으로, AST를 재귀적으로 순회하며 환경(`env`, 변수 테이블)을 갱신하고 출력을 누적.
- `repeat-until`은 do-while 의미(최소 1회 실행), `select-else`는 조건 분기로 구현.

### 핵심 기술
- 컴파일러 프런트엔드(어휘 분석 → 구문 분석 → AST → 의미 평가)의 전체 흐름 이해 및 구현
- 재귀하향 파싱과 문법 계층을 통한 **연산자 우선순위** 처리
- AST 설계와 인터프리터 패턴(다형적 `eval`/`execute`)
- 견고한 오류 처리(모든 문법/의미 오류를 `Syntax Error!`로 일관 처리)
- **동일 사양을 Python과 C++ 두 언어로 구현**하여 언어별 설계 차이를 비교

## 실행
```bash
python interpreter.py        # stdin으로 한 줄 프로그램 입력
# 예) variable a; a = (1 + 2) * 3; print a;   →   9
```
