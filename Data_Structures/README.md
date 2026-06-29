# 📚 Data Structures (자료구조)

스택·동적 배열·재귀 등 기본 자료구조를 직접 구현하고 응용한 과목입니다. 스택 기반 수식 계산기, 정렬 알고리즘 4종, 커서 기반 동적 배열 클래스를 Java로 작성했습니다.

> **기술 스택:** Java · 스택(Stack) · 동적 배열(cursor 기반) · 분할정복 재귀

---

## 📂 MyCalculator — 스택 기반 수식 계산기 (`MyCalculator/`)
중위 표기 수식을 후위 표기로 변환하고 계산하는 계산기입니다. (`myCalculator.java`, 실행 화면 `screenshot1~4.png`)

- **과제 목표:** 사용자가 입력한 **중위(infix) 수식**을 **후위(postfix)** 로 변환하고 계산한다.
- **구현 내용:**
  - 배열 기반 **`Stack` 클래스 직접 구현** (`push`/`pop`/`peek`/`isEmpty`, `top` 인덱스 관리).
  - **중위 → 후위 변환(Shunting-yard):** 연산자 우선순위(`operRank`: `+ -` =1, `* /` =2)와 괄호를 스택으로 처리. 토큰화는 `(`/`)`에 공백을 넣고 `split`.
  - **후위 계산:** 피연산자 스택(`double[]`)으로 `+ - * /` 평가, `top != 0`이면 잘못된 수식으로 판정.
  - **예외 처리:** 괄호 불일치·미지원 토큰을 `IllegalArgumentException`으로 잡아 `잘못된 입력!` 출력. 정수/실수 결과 구분 출력, 반복 실행(Y/N) 메뉴.
- **핵심:** 스택의 대표 응용(수식 파싱·평가), 연산자 우선순위, 견고한 입력 검증.

## 📂 Sorting — 정렬 알고리즘 4종 시각화 (`Sorting/`)
삽입·선택·병합·퀵 정렬을 구현하고 **정렬 과정을 단계별로 출력**합니다. (`sort.java` — 보고서 `report.pdf`의 소스를 코드로 복원, 컴파일 검증 완료)

- **구현 내용:** 삽입(`System.arraycopy`로 시프트), 선택(최댓값을 뒤에서부터 채움), 병합(분할정복 + 보조 배열 `merge`), 퀵(첫 원소 pivot 양방향 분할 `arrayDivid`). 매 단계 배열 상태를 출력하고, 0~31 난수 32개로 메뉴 기반 테스트.
- **핵심:** 분할정복(재귀), in-place(삽입/선택/퀵) vs 보조 배열(병합), 정렬 과정의 시각적 추적.

> 📝 같은 정렬 주제를 **Algorithm 과목에서는 C++ 이동 의미론**으로 다룹니다 → [../Algorithm](../Algorithm) 와 비교.

## 📂 DoubleArraySeq — 커서 기반 동적 배열 시퀀스 (`DoubleArraySeq/`)
크기가 자동 확장되는 `double` 시퀀스 컨테이너 ADT입니다. (`DoubleArraySeqClass.java`, 데모 `DoubleArraySeqDemo.java`, 보고서 `report.pdf`)

- **과제 목표:** 내부 배열을 동적으로 확장하며 **커서(current)** 개념으로 원소를 순회·편집하는 시퀀스 ADT 구현.
- **구현한 연산 (메서드 단위):**

  | 분류 | 메서드 | 설명 |
  |------|--------|------|
  | 생성 | `DoubleArraySeq()`, `DoubleArraySeq(initialCapacity)` | 기본 용량 10 / 사용자 지정 용량 |
  | 삽입 | `addBefore`, `addAfter`, `addAll` | 커서 앞/뒤 삽입(원소 시프트), 다른 시퀀스 통째 추가 |
  | 커서 | `start`, `advance`, `getCurrent`, `isCurrent` | 커서 이동·조회 |
  | 삭제 | `removeCurrent` | 커서 원소 제거 후 뒤 원소 시프트 |
  | 용량 | `ensureCapacity`, `getCapacity`, `trimToSize` | 수동 배열 증설(새 배열 할당 후 복사) / 용량 축소 |
  | 기타 | `clone`, `concatenation`(static), `size` | 깊은 복제, 두 시퀀스 연결 |

- **핵심:** 동적 배열의 **용량 관리**(초과 시 더 큰 배열 할당 후 복사), 커서 기반 순회, 깊은 복사(`clone`), ADT 캡슐화.

---

## 핵심 기술
- 스택 자료구조와 응용(수식 변환·평가, Shunting-yard)
- 정렬 알고리즘(삽입/선택/병합/퀵)과 분할정복
- 커서 기반 동적 배열(자동 확장·축소) 컨테이너 설계
- ADT 캡슐화, 깊은 복사, 예외/경계 조건 처리

## 실행
```bash
javac myCalculator.java && java myCalculator
javac sort.java && java sort
javac DoubleArraySeqClass.java DoubleArraySeqDemo.java && java DoubleArraySeqDemo
```
