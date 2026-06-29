# 🤖 Artificial Intelligence (인공지능 / 딥러닝)

PyTorch로 **CNN → Transformer(밑바닥 구현) → Vision Transformer(ViT) → BERT 파인튜닝(최종 프로젝트)** 까지 단계적으로 구현·학습한 딥러닝 과목입니다. 기초 텐서 연산부터 최신 어텐션 기반 아키텍처와 사전학습 모델 전이학습까지 직접 코드로 작성했고, **모든 결과를 정량 지표로 검증**했습니다.

> **기술 스택:** Python · PyTorch · torchvision · **HuggingFace transformers** · scikit-learn · NumPy · Matplotlib

| 과제 | 주제 | 핵심 성과 |
|------|------|-----------|
| Assignment 0 | Python·NumPy·PyTorch 기초 | 텐서/autograd/MNIST 파이프라인 |
| **Assignment 1** | **CNN 이미지 분류 (CIFAR-10)** | **테스트 정확도 50% → 83%** (Inception 모듈 설계) |
| **Assignment 2** | Transformer(from scratch) · ViT | Transformer 자체 구현, **ViT 85.06%** |
| **Final Project** | **BERT 파인튜닝 (IMDB 감성분석)** | **검증 정확도 94.20% → 94.56%** |

---

## 📂 Assignment 0 & 1 — 기초 + CNN
`Assignment0-1_Basics_and_CNN/` · 명세: `Assignment1_AI.pdf`

### Assignment 0 — Python · NumPy · PyTorch 튜토리얼
- `..._part1_Python_Basics.ipynb` — 파이썬 기초(자료형·컨테이너·함수·클래스)
- `..._part2_Numpy_Matplotlib.ipynb` — NumPy 배열·인덱싱·브로드캐스팅, Matplotlib 시각화
- `..._part3_PyTorch_Tutorial.ipynb` — 텐서 연산, autograd, **MNIST 로딩·정규화**, 신경망 학습 기초

### ⭐ Assignment 1 — CNN 이미지 분류기 & 성능 개선 (`Assignment1-2_CNN.ipynb`)
- **과제 목표:** CIFAR-10 이미지 분류 CNN을 PyTorch로 구현하고, **베이스라인 모델을 개선**하여 정확도를 끌어올린다.
- **구현 & 성능 개선 (구체적 수치):**

  | 모델 | 구조 | 테스트 정확도(10,000장) | 가중치 |
  |------|------|------------------------|--------|
  | 베이스라인 small CNN | Conv–Pool–FC 단순 구조 | **50%** | `cifar_net.pth` |
  | 개선 모델 | **Inception 모듈**(다중 커널 병렬 합성곱) 설계 적용 | **83%** | `better_net.pth` |

  → 단순 CNN의 한계를 **Inception 모듈(1×1·3×3·5×5 등 서로 다른 수용영역을 병렬로 결합)** 도입으로 극복하여 **정확도를 50% → 83%로 +33%p 향상**시켰습니다. 학습된 두 모델의 가중치(`.pth`)를 함께 포함해 재현 가능합니다.
- **핵심:** CNN 아키텍처 설계, Conv2d/MaxPool/CrossEntropyLoss, 학습/평가 루프, **Inception 스타일 멀티-브랜치 합성곱**을 통한 성능 개선.

---

## 📂 Assignment 2 — Transformer & Vision Transformer
`Assignment2_Transformer_ViT/` · 명세: `Assignment2_AI(2025).pdf`

### Assignment 2-1 — Transformer **from scratch** (`Assignment2-1_Transformer_from_scratch.ipynb`)
- **과제 목표:** 라이브러리 모듈에 의존하지 않고 **Transformer를 밑바닥부터** 구현.
- **구현:** **Positional Encoding**, **Multi-Head Self-Attention**(Scaled Dot-Product), Feed-Forward, Residual Connection + Layer Norm으로 Transformer 블록을 직접 작성·학습. 100 epoch 동안 loss를 단조 감소시켜 최종 loss ≈ 3.33로 수렴.

### Assignment 2-2 — Vision Transformer (ViT) (`Assignment2-2_ViT.ipynb`)
- **과제 목표:** 이미지를 패치 시퀀스로 다루는 **ViT** 구현·학습.
- **구현:** **Patch Embedding**(이미지 패치 분할·임베딩), class token, position embedding, Transformer Encoder 적층, 분류 헤드. 학습 가중치 `vit.pth` 포함.
- **결과:** 5 epoch 학습으로 학습 손실 0.39, **테스트 정확도 85.06%**(test loss 0.40).

---

## 📂 ⭐ Final Project — BERT 파인튜닝 (IMDB 감성분석)
`Final_Project_BERT_Finetuning/` · 노트북 `final_project_BERT_finetuning.ipynb` · 보고서 `report.pdf`

사전학습 **`bert-base-uncased`** 를 IMDB 영화 리뷰 5만 건에 맞춰 파인튜닝하여 **긍정/부정 이진 감성분류** 모델을 구축한 최종 프로젝트입니다. 일반화(generalization) 성능 극대화를 목표로 체계적인 실험을 진행했습니다.

- **모델:** `bert-base-uncased`(12L/768H/12-head) 위에 **[CLS] pooled output → Linear 분류 헤드**를 설계, Dropout(0.1). 사전학습 가중치는 규정상 고정.
- **데이터 처리:** IMDB는 리뷰가 길어 128/256 길이는 정보 손실 → **Max Sequence Length 512**로 전체 문맥 학습, Batch 16.
- **최적화·정규화:** AdamW + **Label Smoothing(0.1)** (과신 방지) + **Cosine LR Scheduler** (후반 미세 최적화) + **Seed 튜닝**(42/777/2025 비교).
- **성능 개선 과정 (구체적 수치, 보고서 Table 1):**

  | 단계 | 변경 내용 | 검증 정확도 | 비고 |
  |------|-----------|-------------|------|
  | Baseline | Linear Schedule, 4 epoch | 94.20% | overfitting |
  | Trial 1 | Dropout 0.3 | 94.30% | 수렴 느림 |
  | Trial 2 | **Cosine Scheduler** | 94.45% | 안정적 |
  | **Final** | **Seed 2025 + Label Smoothing** | **94.56%** | **최고 성능** |

  → 단순 베이스라인 대비 overfitting을 제어하면서 **검증 정확도 94.20% → 94.56%** 달성. scikit-learn으로 precision/recall/F1까지 분석.
- **핵심:** **전이학습(transfer learning)**, HuggingFace `transformers`, 커스텀 분류 헤드 설계, **정규화(Label Smoothing)·학습률 스케줄링(Cosine)·하이퍼파라미터/시드 ablation**을 통한 일반화 성능 최적화.

> 📝 파인튜닝된 가중치 `finetuned_bert.pth`(약 417MB)는 GitHub 100MB 제한을 초과하여 저장소에서 제외했습니다(로컬 보관, 요청 시 제공). 노트북에 학습 코드와 실행 로그가 모두 보존되어 있어 재현 가능합니다.

---

## 핵심 기술 (과목 전체)
- PyTorch 모델 정의(`nn.Module`), 학습/평가 루프, 옵티마이저·손실함수, GPU 학습
- **CNN** 아키텍처 설계 + **Inception 모듈**로 성능 개선 (50%→83%)
- **어텐션·Transformer**를 처음부터 구현, **ViT** 패치 임베딩
- **사전학습 모델 전이학습(BERT 파인튜닝)** 과 NLP 텍스트 분류
- **정규화·스케줄링·하이퍼파라미터 튜닝**을 통한 일반화 최적화, 정량 평가(Accuracy/F1)

## 실행
```bash
# 가상환경(env/environment.yml) 구성 후 Jupyter/Colab에서 실행 (GPU 권장)
pip install torch torchvision transformers scikit-learn pandas
jupyter notebook
```
> 모든 노트북에 실제 학습 로그·정확도 출력이 보존되어 있어 결과를 검증할 수 있습니다.
