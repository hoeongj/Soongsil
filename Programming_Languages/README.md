# 🧩 Programming Languages (프로그래밍 언어론)

직접 설계한 **미니 명령형 언어**를 해석·실행하는 인터프리터를 **렉서 → 재귀하향 파서 → AST → 트리워킹 평가기** 전 과정으로 구현한 과목입니다. 컴파일러/인터프리터 프런트엔드를 처음부터 끝까지 직접 작성했습니다.

> **기술 스택:** Python · C++ (동일 사양을 두 언어로 구현) · 정규식 토크나이저 · 재귀하향 파서(Recursive Descent) · AST 트리워킹 인터프리터(인터프리터 패턴)

소스: [`interpreter.py`](./interpreter.py), [`interpreter.cpp`](./interpreter.cpp) · 보고서: `report.pdf`

---

## 과제 목표
한 줄로 주어지는 미니 언어 프로그램을 파싱·실행하여 `print` 결과를 출력하고, 문법/의미 오류 시 `Syntax Error!`를 출력하는 인터프리터를 구현한다.

### 지원 문법 (요약)
```
program     → decl* statement*
decl        → ("variable" id ";")  |  ("constant" id "=" number ";")
statement   → assign | print | repeat | select
repeat      → "repeat" block "until" "(" bexpr ")" ";"
select      → "select" "(" bexpr ")" block "else" block ";"
block       → "begin" statement* "end"
bexpr       → aexpr (== | != | < | > | <= | >=) aexpr
aexpr       → term (("+"|"-"|"*") term)*          // 좌결합
term        → "-"? ( number | id | "(" aexpr ")" )   // 단항 부정, 괄호
```
- 식별자/숫자 최대 길이 제한, 예약어(`variable/constant/print/repeat/begin/end/until/select/else`) 구분, **상수 재할당 금지**.

## 구현 내용

### 1. 렉서 (Tokenizer)
- 정규식 `(==|!=|<=|>=|[;()+\-*=<>])`로 연산자·구분자 주위에 공백을 삽입한 뒤 분리하여 토큰열 생성. `is_number`/`is_var_name`으로 토큰 유효성(길이·문자 범위·예약어 충돌) 검사.

### 2. 재귀하향 파서 (Recursive Descent)
- `parse_program → parse_declaration → parse_statement → parse_block → parse_bexpr → parse_aexpr → parse_term` 의 문법 규칙별 함수.
- **연산자 우선순위**를 문법 계층(`aexpr`=가감승, `term`=단항/괄호/원자)으로 자연스럽게 표현.
- **의미 오류를 파싱 단계에서 검출:** 상수 재할당, 미선언 변수 사용, 중복 선언 → 예외.

### 3. AST & 트리워킹 평가기 (인터프리터 패턴)

| AST 노드 | 역할 | 평가 메서드 |
|----------|------|-------------|
| `NumberExpr`, `VarExpr`, `NegExpr` | 리터럴·변수·단항부정 | `eval(env)` |
| `BinExpr`(+,-,*), `BoolExpr`(비교) | 산술·비교식 | `eval(env)` |
| `AssignStmt`, `PrintStmt` | 대입·출력 | `execute(env, output)` |
| `RepeatStmt`, `SelectStmt` | 반복(do-while 의미)·조건분기 | `execute(env, output)` |

- 각 노드가 `eval`/`execute`를 **다형적으로 구현**하여 AST를 재귀 순회하며 환경(`env`, 변수 테이블)을 갱신하고 출력을 누적.
- 모든 문법/의미 오류를 `Syntax Error!`로 일관 처리(견고성).

### 핵심 기술
- 컴파일러 프런트엔드 전 과정(어휘분석 → 구문분석 → AST → 의미평가) 구현
- 재귀하향 파싱 + 문법 계층으로 **연산자 우선순위** 처리
- AST 설계와 **인터프리터 패턴**(다형적 `eval`/`execute`)
- **동일 사양을 Python·C++ 두 언어로 구현**하여 언어별 설계 차이 비교

## 실행
```bash
python interpreter.py        # stdin으로 한 줄 프로그램 입력
# 예) variable a; a = (1 + 2) * 3; print a;        → 9
# 예) constant n = 3; variable s; s = 0; variable i; i = 0;
#     repeat begin s = s + i; i = i + 1; end until(i > n); print s;   → 6
```
