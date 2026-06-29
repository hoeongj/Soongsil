# 📊 Data Analysis (데이터 분석)

UC Berkeley의 *Foundations of Data Science(**Data 8**)* 커리큘럼을 기반으로, **표 데이터 처리 → 시각화 → 시뮬레이션·확률 → 추론 통계(가설검정·신뢰구간) → 회귀**까지 데이터 과학의 전 과정을 Python으로 실습한 과목입니다. 총 22개의 랩 노트북(lab03–05, lab07–25)을 직접 작성·실행했으며, lab01·02는 입문용 문서(PDF)로 포함했습니다.

> **기술 스택:** Python · `datascience`(Berkeley Tables) · NumPy · Matplotlib · SciPy · 확률·통계

- `00_intro_hello_world.ipynb` — 환경 설정/소개용 인트로 노트북
- `labs/` — lab01~lab25 (각 노트북 `.ipynb` + 문제·해설 `.pdf`)

---

## 📋 랩 구성 (전 과정)

| Lab | 주제 | 핵심 내용 |
|-----|------|-----------|
| 01–02 | 시작하기 *(PDF)* | 파이썬·Jupyter 환경, 표현식 기초 |
| 03 | Table Operations | `datascience` Table 생성·열 선택·정렬 |
| 04 | Data Types & Arrays | 문자열/숫자, NumPy 배열 연산 |
| 05 | Table Manipulation & Visualization | 실업률·출생률 데이터 시각화 |
| 07 | Functions & Visualizations | 함수 정의, 시각화 |
| 08 | Functions & Tables | `apply`, 그룹 집계 기초 |
| 09 | Groups | `group`/`pivot` (자전거·공무원 급여 데이터) |
| 10 | Applying Functions & Iteration | 함수 적용·반복문(미식축구 데이터) |
| 11 | Simulations | `np.random.choice`, 몬테카를로 시뮬레이션 |
| 12 | Probability, Simulation & Estimation | 룰렛 베팅 확률·추정 |
| 13 | Therapeutic Touch | 가설검정 입문(Emily Rosa 실험), `sample_proportions` |
| 14 | Testing Hypotheses | **TVD**(총변동거리)를 검정통계량으로 |
| 15 | Testing Hypotheses (2) | 분포 비교 가설검정 |
| 16 | A/B Testing | Great British Bake Off 데이터로 A/B 검정 |
| 17 | Climate Change | 기온·강수량 시계열 분석(plotly) |
| 18 | Confidence Intervals | 부트스트랩 신뢰구간 |
| 19 | Confidence Intervals (2) | 신뢰구간 해석, 테니스 데이터 |
| 20 | Sample Sizes & Confidence | 분포 꼬리 경계, 표본 크기와 신뢰수준(SciPy) |
| 21 | Normal Distribution | 정규분포, 여론조사 적용 |
| 22 | Variability of Sample Means | 표본평균의 변동성, 중심극한정리 |
| 23 | Old Faithful | 산점도·상관, 회귀 입문 |
| 24 | World Population & Poverty | 세계 인구·빈곤 데이터 분석 |
| 25 | Linear Regression | **선형 회귀**(최소제곱), FIFA 데이터 예측 |

> (lab06은 원본에 포함되지 않았습니다.)

---

## 핵심 기술
- **표 데이터 처리:** `datascience` Table로 선택·필터·정렬·그룹·피벗·조인
- **데이터 시각화:** Matplotlib/plotly로 분포·시계열·산점도 표현
- **시뮬레이션·확률:** 난수 표본 추출, 몬테카를로 추정
- **추론 통계:** 가설검정(TVD, A/B 테스트), **부트스트랩 신뢰구간**, 정규분포, 중심극한정리
- **예측 모델링:** **선형 회귀**(최소제곱법)와 예측

## 실행
```bash
pip install datascience numpy matplotlib scipy
jupyter notebook labs/lab25.ipynb     # 또는 Google Colab 업로드
```
> 각 노트북에는 실행 결과(표·그래프·통계량)가 그대로 보존되어 있습니다.
