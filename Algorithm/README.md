# ⚡ Algorithm (알고리즘)

정렬 알고리즘 4종을 C++로 구현하되, **이동 의미론(move semantics)으로 불필요한 복사를 제거**하는 데 초점을 둔 과제입니다. 객체의 생성·복사·이동·소멸 횟수를 세는 계측형 타입 위에서 동작하도록 설계되어, "정렬 중 복사 0회"를 목표로 `std::move`/`std::swap`만으로 원소를 옮깁니다(테스트 하니스가 복사 발생 시 경고를 출력).

> **기술 스택:** C++ (C++11 이상) · STL(`std::vector`, iterator) · `std::move` / rvalue reference · `std::function`

소스: [`src/`](./src) — `testmain.cpp`, `hw1_myheader.h`(정렬 구현), `MyInteger.h`(계측 타입), `hw1_common.h`

---

## 과제 목표
정렬 알고리즘을 구현하되, **정렬 도중 원소의 복사(copy)가 발생하지 않도록** 이동 의미론을 활용한다. 정렬 중 일반 생성자·복사 생성자·복사 대입·소멸이 일어나면 경고가 출력되도록 설계된 테스트 하니스를 통과해야 한다.

## 구현 내용

### 계측 기반 검증 환경
- `MyInteger`는 생성자/복사 생성자/이동 생성자/복사 대입/이동 대입/소멸 호출을 **카운트하는 계측 타입**으로, `default_logger`가 정렬 전후의 연산 횟수를 출력한다.
- `testmain.cpp`는 난수 시퀀스를 채우고 정렬 후 카운트를 출력하며, **복사·일반 생성자가 1회라도 발생하면 경고**를 띄운다 → 즉, "복사 없는 정렬"을 강제한다.

### 정렬 알고리즘 4종 (`hw1_myheader.h`)
- **Insertion Sort** — 삽입 위치를 찾는 동안 `std::move`로 원소를 밀어내어 임시 복사를 제거.
- **Selection Sort** — 최솟값 위치를 찾아 `std::swap`으로 교환(직접 구현한 TODO 구간).
- **Merge Sort** — 좌/우 구간을 임시 벡터로 `emplace_back(std::move(...))` 이동시킨 뒤 병합, 결과를 다시 `std::move`로 원위치.
- **Quick Sort** — 마지막 원소를 pivot으로 하는 Lomuto 분할(`partition`)을 직접 구현하고, 분할 후 좌/우를 재귀 정렬.
- 비교자는 `std::function<bool(const MyInteger&, const MyInteger&)>`(기본 `std::less`)로 받아 **임의 정렬 기준**을 주입할 수 있도록 일반화.
- 정렬 결과 검증을 위한 `verify()`(인접 원소 순서 확인)와 학습용 `bogosort`도 포함.

### 핵심 기술
- **이동 의미론(move semantics)** 과 rvalue reference로 복사 비용 제거 (`std::move`, `emplace_back`)
- 분할 정복(merge/quick) 재귀 설계, in-place vs 보조 배열 트레이드오프 이해
- 루프 불변식(loop invariant) 기반의 정확성 추론(코드 주석으로 Initialize/Maintenance/Termination 명시)
- 계측 타입을 통한 **성능 특성의 정량적 검증** (복사 횟수 = 0)

## 실행
```bash
g++ -std=c++17 -O2 src/testmain.cpp -o sort_test
./sort_test     # 정렬 전/후 배열과 생성·복사·이동·소멸 호출 횟수 출력
```
