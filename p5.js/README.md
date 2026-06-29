# 🎨 p5.js — 크리에이티브 코딩

JavaScript 그래픽스 라이브러리 **p5.js**로 제작한 인터랙티브 작품 모음입니다. 정적 드로잉부터 마우스·키보드로 반응하는 애니메이션까지 단계적으로 제작했습니다.

> **기술 스택:** JavaScript · p5.js · HTML/CSS · Canvas · 애니메이션 · 사용자 인터랙션

루트 [`index.html`](./index.html) 을 브라우저로 열면 4개 작품을 한 페이지(iframe 갤러리)에서 볼 수 있습니다.

---

## 작품 구성

| 과제 | 폴더 | 내용 | 기법 |
|------|------|------|------|
| **과제 1. 추상화 그리기 (정적)** | `sample1/` | 도형으로 구성한 정적 추상화/풍경 | `createCanvas`, `ellipse`/`rect`/`line`, `fill`/`stroke` |
| **과제 2. 캐릭터 그리기 (정적)** | `sample2/` | 도형 조합으로 만든 캐릭터(얼굴) | 좌표 기반 도형 합성, `arc`, 색상 |
| **과제 3. 움직이는 캐릭터 (동적)** | `sample3/` | 키보드·마우스로 반응하는 캐릭터 애니메이션 | `keyIsDown`/`mouseIsPressed` 인터랙션, 상태 변수 애니메이션, `saveGif` |
| **과제 4. 추상화 움직임 (동적)** | `sample4/` | 색이 변하는 애니메이션 추상화 | `draw()` 루프 애니메이션, **HSB↔RGB colorMode**, 색 보간(`lerpColor`) |

각 작품은 `index.html`(캔버스 호스트) + `sketch.js`(p5.js 코드) + `style.css`로 구성됩니다.

## 핵심 기술
- p5.js의 `setup()`/`draw()` 라이프사이클과 캔버스 드로잉
- 기본 도형(`ellipse`, `rect`, `arc`, `line`)과 색상(`fill`, `stroke`, `colorMode`)으로 이미지 구성
- `draw()` 루프 기반 **애니메이션**과 상태 변수 제어
- **사용자 인터랙션**(키보드 `keyIsDown`, 마우스 `mouseIsPressed`)
- GIF 저장(`saveGif`), HSB/RGB 색 공간과 색 보간

## 실행
```bash
# 정적 사이트 - 브라우저로 바로 열거나 로컬 서버로 서빙
python -m http.server 8000   # 후 http://localhost:8000 접속
```

> 💡 이 과목은 원래 별도 저장소(`hoeongj/p5js`)에 있던 작업을 본 포트폴리오로 통합한 것입니다.
