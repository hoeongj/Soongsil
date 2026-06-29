# ⚡ Algorithm (알고리즘)

정렬 알고리즘 4종을 C++로 구현하되, **이동 의미론(move semantics)으로 불필요한 복사를 제거**하는 데 초점을 둔 과제입니다. 객체의 생성·복사·이동·소멸 횟수를 세는 계측형 타입 위에서 동작하도록 설계되어, "정렬 중 복사 0회"를 목표로 합니다.

> **기술 스택:** C++ (C++11 이상) · STL(`std::vector`, iterator) · `std::move`/rvalue reference · `std::function` · `std::swap`

소스: [`src/`](./src) — `testmain.cpp`(테스트 하니스), `hw1_myheader.h`(정렬 구현), `MyInteger.h`(계측 타입), `hw1_common.h`(공통 정의)

---

## 과제 목표
정렬 알고리즘을 구현하되, **정렬 도중 원소의 복사(copy)가 발생하지 않도록** 이동 의미론을 활용한다. 테스트 하니스(`testmain.cpp`)는 정렬 전후로 일반 생성자·복사 생성자·복사 대입·소멸 호출 횟수를 출력하고, **복사가 1회라도 발생하면 경고**를 띄워 "복사 없는 정렬"을 강제한다.

## 구현한 정렬 알고리즘 (`hw1_myheader.h`)

| 알고리즘 | 시간복잡도(평균/최악) | 공간 | 방식 | 핵심 구현 포인트 |
|----------|----------------------|------|------|------------------|
| **Insertion Sort** | O(n²) / O(n²) | O(1) | in-place | 삽입 위치 탐색 중 `std::move`로 원소를 한 칸씩 이동(복사 대입 회피) |
| **Selection Sort** | O(n²) / O(n²) | O(1) | in-place | 최솟값 위치를 찾아 `std::swap`으로 교환(직접 구현한 TODO 구간) |
| **Merge Sort** | O(n log n) / O(n log n) | O(n) | 분할정복 + 보조 배열 | 좌/우 구간을 임시 `vector`로 `emplace_back(std::move(...))` 이동, 병합 결과를 `std::move`로 원위치 |
| **Quick Sort** | O(n log n) / O(n²) | O(log n) | 분할정복 + in-place | **Lomuto 분할**(마지막 원소 pivot), 분할 후 pivot 좌/우를 재귀 정렬 |

- **비교자 일반화:** 모든 정렬이 `predicate = std::function<bool(const MyInteger&, const MyInteger&)>`(기본값 `std::less`)를 받아 **임의 정렬 기준 주입**이 가능.
- **정확성 추론:** 각 알고리즘 주석에 **루프 불변식**(Initialize / Maintenance / Termination)을 명시하여 정확성을 논증.
- **부가 구현:** 정렬 검증용 `verify()`(인접 원소 순서 확인), 학습용 `bogosort()`(랜덤 셔플 후 검증 반복)도 포함.

## 🎯 핵심 — 이동 의미론으로 복사 제거
- `MyInteger`(`MyInteger.h`)는 **일반 생성자 / 복사 생성자 / 이동 생성자 / 복사 대입 / 이동 대입 / 소멸** 호출을 각각 카운트하는 계측 타입이다.
- 따라서 정렬을 "올바르게" 짜는 것에 더해, **`std::move`·`emplace_back`·`std::swap`만으로 원소를 옮겨** 복사 생성/대입이 0이 되도록 작성해야 한다.
- 예) 삽입 정렬의 시프트를 `beg[idx+1] = std::move(beg[idx])`로, 병합 정렬의 구간 분리를 `left.emplace_back(std::move(*pos))`로 처리.
- `testmain.cpp`는 정렬 후 카운트를 출력하므로, 복사 0회 달성 여부를 **정량적으로 확인**할 수 있다.

## 핵심 기술
- **이동 의미론**과 rvalue reference로 복사 비용 제거 (`std::move`, `emplace_back`)
- 분할정복 재귀 설계, in-place vs 보조 배열 트레이드오프
- 루프 불변식 기반 정확성 추론
- `std::function`을 통한 비교자 일반화, 계측 타입을 통한 성능 특성 검증

## 실행
```bash
g++ -std=c++17 -O2 src/testmain.cpp -o sort_test
./sort_test     # 정렬 전/후 배열과 생성·복사·이동·소멸 호출 횟수 출력 (복사 0 목표)
```
