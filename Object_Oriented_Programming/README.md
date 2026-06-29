# ☕ Object-Oriented Programming (객체지향 프로그래밍)

Java로 객체지향의 4대 특성(**캡슐화·상속·다형성·추상화**)과 클래스 설계, 컬렉션·문자열 처리를 단계적으로 익힌 과목입니다. 각 제출본 폴더에는 보고서(PDF)가 함께 들어 있습니다.

> **기술 스택:** Java · 클래스/객체 설계 · 추상 클래스 · 상속·다형성 · `equals`/`toString` 오버라이딩

---

## 📂 01. 기초 클래스 프로그램 (`01_basics_birth_calc_car_travel/`)
- `birth.java`, `calc.java`, `car.java`, `travel.java` — 클래스·객체·메서드·생성자를 사용한 기초 응용 프로그램(생일 계산, 계산기, 자동차, 여행 등 도메인 모델링).
- **핵심:** 클래스 정의, 필드/메서드/생성자, 객체 상태와 행위의 캡슐화.

## 📂 02. 객체지향 문제 모음 (`02_numbered_problems/`)
- `pro6.java`, `pro10.java`, `pro14.java`, `pro16.java`, `pro18.java` — 클래스 설계·상속·메서드 활용을 다루는 다양한 연습 문제.

## 📂 03. 클래스 & 동적 배열 (`03_classes_and_dynamic_array/`)
- `pro2.java` — `Cube` 클래스(치수 증가 `increase` 등 메서드)로 캡슐화 연습.
- `pro14.java` — **`VArray`(가변 배열) 클래스 직접 구현**: 용량 확장(`encapacity`), `insert`, `remove`, `printAll` — 동적 배열 자료구조를 OOP로 캡슐화.
- **핵심:** 상태를 캡슐화한 클래스 설계, 가변 길이 컨테이너 구현.

## 📂 04. 추상 클래스 & 다형성 (`04_abstract_classes_polymorphism/`)
- `pro8.java` — **추상 클래스 `Box`**(공통 인터페이스 정의, 하위 클래스에서 구체화).
- `pro12.java` — **추상 클래스 `PairMap`**(키-값 매핑 자료구조의 추상화, 배열 기반 구현).
- `pro14.java` — **추상 클래스 `Shape`** + `setNext`로 도형을 **연결 리스트로 연결**하고 공통 메서드를 다형적으로 호출 → **상속·다형성·추상화**의 종합.
- **핵심:** 추상 클래스/추상 메서드, 업캐스팅을 통한 **다형성**, 객체들을 연결한 자료구조 설계.

## 📂 05. 문자열 처리 & 응용 (`05_strings_and_BookApp/`)
- `pro2_BookApp.java` — **`Book` 클래스에서 `equals()`·`toString()` 오버라이딩**(객체 동등성·문자열 표현 재정의)과 도서 관리 응용.
- `pro6_strRotation.java` — 문자열 회전(rotation) 알고리즘.
- `pro10_strQuiz_1.java`, `pro10_strQuiz_2.java` — 문자열 처리 퀴즈(파싱·변환).
- **핵심:** `Object` 메서드 오버라이딩, 문자열 알고리즘, 객체 동등성 개념.

---

## 핵심 기술
- 객체지향 4대 특성: **캡슐화 · 상속 · 다형성 · 추상화**
- 추상 클래스/추상 메서드를 통한 인터페이스 설계
- `equals`/`toString` 등 `Object` 메서드 오버라이딩
- OOP로 캡슐화한 자료구조(가변 배열, 키-값 맵, 연결된 객체)
- 문자열 처리 알고리즘

## 실행
```bash
javac pro14.java && java pro14    # 각 파일의 public class 이름으로 실행
```
