# 🌐 Network Programming (네트워크 프로그래밍)

원시 소켓부터 **TLS·비동기 I/O·분산 메시징·HTTP/REST·웹 스크래핑**까지, 현대 네트워크 애플리케이션의 핵심 기법을 Python으로 폭넓게 구현한 과목입니다. 4개의 과제 세트로 구성됩니다.

> **기술 스택:** Python · `socket` · `ssl`(TLS) · `asyncio` · **ZeroMQ** · **memcached** · `pickle`/`struct` · Flask · `requests` · BeautifulSoup

각 과제 폴더에는 보고서(`report.pdf`)가 포함되어 있습니다.

---

## 📂 Assignment 1 — 소켓 기초 (`Assignment1_sockets_basics/`)
- **`solution1.py`** — `socket` + `ssl`로 **HTTPS 요청을 직접 구성**하여 외부 API(timeapi.io)에 접속, 원시 HTTP 응답을 파싱.
- **`solution2_*.py`** — TCP 클라이언트/서버: 클라이언트가 보낸 문장의 **모음(vowel) 개수**를 서버가 세어 응답.
- **`solution3_*.py`** — TCP 기반 **숫자 맞히기 게임 서버**(난수 생성, 시도 횟수 제한, 힌트 제공).

## 📂 Assignment 2 — 직렬화 & TLS (`Assignment2_serialization_TLS/`)
- **`solution1_*.py`** — `pickle` + `struct`로 **길이 프리픽스(length-prefixed) 직렬화**를 구현한 연락처 저장 서버/클라이언트(객체를 안전하게 송수신).
- **`solution2.py`** — 잘못된 코드(UDP `SOCK_DGRAM`로 `listen()` 시도)를 **TCP로 바로잡는 디버깅 과제**(원인·수정 주석 포함).
- **`solution3_*.py`** — 인증서(`server.crt`/`server.key`) 기반 **TLS 에코 서버/클라이언트**(`ssl` 래핑으로 암호화 통신).

## 📂 Assignment 3 — 비동기 & 분산 처리 (`Assignment3_async_and_ZeroMQ/`)
- **`solution1_*.py`** — **`asyncio` 기반 비동기 채팅 릴레이 서버**: 이름 있는 채널을 관리하고 같은 채널의 클라이언트들에게 메시지를 중계, 접속/종료/예외를 비동기로 처리.
- **`solution2_producer.py` / `solution2_worker.py`** — **ZeroMQ(PUB/PULL) + memcached**를 이용한 **분산 작업 처리 시스템**: producer가 연산 작업을 발행하고 다수의 worker가 병렬로 계산해 결과를 회수.

## 📂 Assignment 4 — HTTP · Flask · 웹 스크래핑 (`Assignment4_HTTP_Flask_scraping/`)
- **`solution1_*.py`** — `keywords.csv`를 읽어 단어 뜻을 돌려주는 **HTTP 사전 조회 서버**(원시 소켓으로 HTTP 요청 파싱·응답 생성).
- **`solution2_app.py` / `solution2_client.py`** — **Flask REST API** + **BeautifulSoup 웹 스크래핑**: 외부 페이지에서 출판물 정보를 크롤링하여 CSV로 관리하고 JSON API로 제공(390줄 규모).
- **`solution3.py`** — 동시성(threading)·URL 인코딩을 활용한 서버 동작 분석/공격 시연 스크립트(보안 관점 실습).

---

## 핵심 기술
- TCP/UDP 소켓 프로그래밍, **HTTP 프로토콜**을 원시 소켓으로 직접 파싱/생성
- **TLS/SSL** 암호화 통신, 인증서 기반 서버
- 바이트 스트림 **직렬화**(`pickle`+`struct`, 길이 프리픽스 프레이밍)
- **비동기 I/O**(`asyncio`)로 다중 클라이언트 동시 처리
- **분산 메시징**(ZeroMQ PUB/PULL) + **캐시(memcached)** 기반 작업 분산
- **Flask** REST API 설계, **웹 스크래핑**(BeautifulSoup)
- 네트워크 보안 관점(취약점 시연, 입력 검증의 중요성)

## 실행 (예시)
```bash
# 서버 먼저 실행 후 클라이언트 실행
python solution2_server.py   &
python solution2_client.py
# 일부 과제는 추가 패키지 필요: pip install pyzmq pymemcache flask requests beautifulsoup4
```
