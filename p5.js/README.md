# 🎨 p5.js — 크리에이티브 코딩

JavaScript 그래픽스 라이브러리 **p5.js**로 제작한 인터랙티브 작품 모음입니다. 정적 드로잉부터 마우스·키보드로 반응하는 애니메이션까지 단계적으로 제작했습니다.

> **기술 스택:** JavaScript · p5.js · HTML/CSS · Canvas · 애니메이션 루프 · 사용자 인터랙션

루트 [`index.html`](./index.html)(`나의 p5.js 작품` 갤러리)을 브라우저로 열면 4개 작품을 한 페이지(iframe)에서 볼 수 있습니다. 각 작품은 `index.html` + `sketch.js` + `style.css`로 구성됩니다.

---

## 작품별 상세

### 과제 1. 추상화 그리기 (정적) — `sample1/`
- 600×400 캔버스에 격자선(`line`)을 그리고, `ellipse`·`rect`로 **나무/풍경 형태의 추상 구성**을 배치.
- 기법: `createCanvas`, `background`, `fill`/`stroke`/`strokeWeight`/`noStroke`, 기본 도형 좌표 합성.

### 과제 2. 캐릭터 그리기 (정적) — `sample2/`
- 도형을 겹쳐 **캐릭터 얼굴**(머리·눈·입·치아(grillz))을 합성. 반복문으로 치아를 그리는 등 도형 패턴 활용.
- 기법: `ellipse`/`arc`/`rect`, 좌표·크기 변수화, `for` 루프 기반 반복 드로잉.

### 과제 3. 움직이는 캐릭터 (동적) — `sample3/`
- `draw()` 루프에서 **상태 변수**(`handAnim`, `mouthAnim`)를 증감시켜 애니메이션. **사용자 인터랙션**으로 반응:
  - 마우스 누름 → 손 동작 애니메이션, 키 `O` → 입 벌림
  - 키 `F` → 화면에 랜덤 흰 원 효과, 키 `S` → **`saveGif`로 10초 GIF 녹화**
- 기법: `mouseIsPressed`/`keyIsDown`, 상태 기반 애니메이션, `random`, `saveGif`.

### 과제 4. 추상화 움직임 (동적) — `sample4/`
- 과제 1의 추상화에 **시간에 따라 색이 변하는 애니메이션** 추가. **HSB↔RGB `colorMode` 전환**과 `lerpColor`로 시작색(`startC`)→끝색(`endC`) 보간.
- 기법: `colorMode(HSB/RGB)`, `lerpColor`, `draw()` 루프 기반 색 전이.

| 과제 | 유형 | 핵심 기법 |
|------|------|-----------|
| 1 | 정적 추상화 | 기본 도형 합성, fill/stroke |
| 2 | 정적 캐릭터 | 도형 합성, 반복 드로잉(`for`) |
| 3 | 동적 캐릭터 | 인터랙션(`mouse`/`key`), 상태 애니메이션, `saveGif` |
| 4 | 동적 추상화 | `colorMode` 전환, `lerpColor` 색 보간 |

## 핵심 기술
- p5.js `setup()`/`draw()` 라이프사이클과 캔버스 드로잉
- 기본 도형(`ellipse`/`rect`/`arc`/`line`)·색상(`fill`/`stroke`/`colorMode`) 합성
- `draw()` 루프 기반 **애니메이션**과 상태 변수 제어
- **사용자 인터랙션**(키보드/마우스), GIF 저장, HSB/RGB 색 공간·색 보간(`lerpColor`)

## 실행
```bash
python -m http.server 8000   # http://localhost:8000 접속 (또는 index.html 직접 열기)
```
> 💡 이 과목은 원래 별도 저장소(`hoeongj/p5js`)에 있던 작업을 본 포트폴리오로 통합한 것입니다.
