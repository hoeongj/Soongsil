# 🤖 Artificial Intelligence (인공지능 / 딥러닝)

PyTorch로 **CNN → Transformer(밑바닥 구현) → Vision Transformer(ViT)** 까지 단계적으로 구현하고 학습시킨 딥러닝 과목입니다. 기초 텐서 연산부터 최신 어텐션 기반 아키텍처까지 직접 코드로 작성했습니다.

> **기술 스택:** Python · PyTorch · torchvision · NumPy · Matplotlib

---

## 📂 Assignment 0 & 1 — 기초 + CNN
`Assignment0-1_Basics_and_CNN/` · 명세: `Assignment1_AI.pdf`

> 📌 한 폴더에 **Assignment 0(기초)** 와 **Assignment 1(CNN)** 이 함께 들어 있습니다 (노트북이 공유 이미지/환경 파일을 참조하므로 원본 구성을 유지).

### Assignment 0 — Python · NumPy · PyTorch 튜토리얼
- `Assignment1-1_part1_Python_Basics.ipynb` — 파이썬 기초(자료형, 컨테이너, 함수, 클래스)
- `Assignment1-1_part2_Numpy_Matplotlib.ipynb` — NumPy 배열·인덱싱·브로드캐스팅·연산, Matplotlib 시각화
- `Assignment1-1_part3_PyTorch_Tutorial.ipynb` — 텐서 연산, autograd, **MNIST 로딩·정규화**, 신경망 학습 기초

### Assignment 1 — CNN 이미지 분류기 (`Assignment1-2_CNN.ipynb`)
- **과제 목표:** 합성곱 신경망(CNN)을 PyTorch로 구성하고 이미지 데이터셋으로 학습·평가한다.
- **구현 내용:** Conv–ReLU–Pooling–FC로 이어지는 CNN 아키텍처 정의, 데이터 로딩·정규화, 손실함수·옵티마이저 설정, 학습 루프와 평가 루프 작성.
- **결과:** 10 epoch 학습으로 학습 손실을 약 0.69 → 0.19 수준까지 낮추고, **테스트 이미지 10,000장에 대해 정확도 82%** 달성.

---

## 📂 Assignment 2 — Transformer & Vision Transformer
`Assignment2_Transformer_ViT/` · 명세: `Assignment2_AI(2025).pdf`

### Assignment 2-1 — Transformer **from scratch** (`Assignment2-1_Transformer_from_scratch.ipynb`)
- **과제 목표:** 라이브러리 모듈에 의존하지 않고 **Transformer를 밑바닥부터** 구현한다.
- **구현 내용:** **Positional Encoding**, **Multi-Head Self-Attention**(Scaled Dot-Product Attention), Feed-Forward Network, Residual Connection + Layer Normalization으로 구성된 Transformer 블록을 직접 작성하고 학습.
- **결과:** 100 epoch 학습 동안 손실을 단조 감소시켜 최종 loss ≈ **3.33** 까지 수렴.

### Assignment 2-2 — Vision Transformer (ViT) (`Assignment2-2_ViT.ipynb`)
- **과제 목표:** 이미지를 패치 시퀀스로 다루는 **ViT**를 구현·학습한다.
- **구현 내용:** **Patch Embedding**(이미지를 패치로 분할 후 임베딩), class token, position embedding, Transformer Encoder 블록 적층, 분류 헤드 구성. 학습된 가중치 `vit.pth` 포함.
- **결과:** 5 epoch 학습으로 학습 손실 0.39, **테스트 정확도 85.06%**(test loss 0.40) 달성.

---

## 핵심 기술
- PyTorch 모델 정의(`nn.Module`), 학습/평가 루프, 옵티마이저·손실함수, GPU 학습
- **CNN** 아키텍처 설계와 이미지 분류
- **어텐션 메커니즘**과 **Transformer**의 내부 구성요소를 처음부터 구현
- **ViT**의 패치 임베딩 기반 비전 처리
- 정량적 성능 평가(정확도·손실) 및 학습 곡선 분석

## 실행
```bash
# 가상환경 구성(제공된 env/environment.yml 참고) 후 Jupyter/Colab에서 노트북 실행
jupyter notebook   # 또는 Google Colab 업로드 (GPU 권장)
```
> 노트북에는 실제 학습 로그와 출력(정확도·손실)이 그대로 보존되어 있어 결과를 검증할 수 있습니다.
