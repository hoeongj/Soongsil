# 🌐 Network Programming (네트워크 프로그래밍)

원시 소켓부터 **TLS·비동기 I/O·분산 메시징·HTTP/REST·웹 스크래핑**까지, 현대 네트워크 애플리케이션의 핵심 기법을 Python으로 폭넓게 구현한 과목입니다. 4개의 과제 세트(총 19개 솔루션, 각 폴더에 `report.pdf` 포함)로 구성됩니다.

> **기술 스택:** Python · `socket` · `ssl`(TLS) · `asyncio` · **ZeroMQ(pyzmq)** · **memcached(pymemcache)** · `pickle`/`struct` · Flask · `requests` · BeautifulSoup

---

## 📂 Assignment 1 — 소켓 기초 (`Assignment1_sockets_basics/`)
- **`solution1.py`** — `socket` + `ssl`로 **HTTPS 요청을 직접 구성**(443 포트 연결 → `GET` 헤더 수작업 작성 → 응답 파싱)하여 timeapi.io에서 시간 조회. 원시 소켓 위에서 HTTP/TLS를 다룬다.
- **`solution2_*`** — TCP 클라이언트/서버: 클라이언트가 보낸 문장의 **모음(vowel) 개수**를 서버가 세어 응답(`count_vowels`).
- **`solution3_*`** — TCP **숫자 맞히기 게임 서버**: 난수 정답, **시도 3회 제한**(`MAX_ATTEMPTS`), 업/다운 힌트(`play_game`).

## 📂 Assignment 2 — 직렬화 & TLS (`Assignment2_serialization_TLS/`)
- **`solution1_*`** — `pickle` + `struct`로 **길이 프리픽스(length-prefixed) 프레이밍**을 구현한 연락처 저장 서버/클라이언트: 객체를 직렬화하고 `struct.pack`으로 4바이트 길이 헤더를 붙여 TCP 스트림 경계 문제를 해결.
- **`solution2.py`** — 잘못된 코드(UDP `SOCK_DGRAM`로 `listen()` 시도)를 **TCP(`SOCK_STREAM`)로 바로잡는 디버깅 과제** (원인·수정 주석 포함).
- **`solution3_*`** — **TLS 에코 서버/클라이언트**: `ssl.SSLContext(PROTOCOL_TLS_SERVER)` + `load_cert_chain(server.crt, server.key)` + `wrap_socket(server_side=True)`로 암호화 채널 구성.

## 📂 Assignment 3 — 비동기 & 분산 처리 (`Assignment3_async_and_ZeroMQ/`)
- **`solution1_*` (asyncio 채팅 릴레이)** — `asyncio.start_server` 기반 비동기 채팅: `channels` 딕셔너리(채널명 → `StreamWriter` 집합)로 **named channel**을 관리하고, `broadcast()`로 같은 채널 클라이언트에게만 중계(발신자 제외), `await writer.drain()` 백프레셔 처리, 접속 종료 시 `remove_client`로 정리. 단일 스레드 이벤트 루프로 다중 접속 동시 처리.
- **`solution2_producer.py` / `solution2_worker.py` (ZeroMQ + memcached 분산 처리)** — producer가 **ZeroMQ `PUB`**(:5555)로 연산 작업을 발행하고 **`PULL`**(:5556)로 답을 수집, `zmq.Poller`로 이벤트 멀티플렉싱. 여러 worker가 작업을 받아 계산해 답을 돌려주고, **memcached**(:11211)에 worker별 점수를 누적(`read_score`/`write_score`). 정답 검증(`handle_answer`)까지 수행하는 분산 컴퓨팅 파이프라인.

## 📂 Assignment 4 — HTTP · Flask · 웹 스크래핑 (`Assignment4_HTTP_Flask_scraping/`)
- **`solution1_*`** — `keywords.csv`를 읽어 단어 뜻을 돌려주는 **HTTP 사전 조회 서버**: 원시 소켓으로 HTTP 요청 라인을 파싱하고 응답(상태줄/헤더/바디)을 직접 생성.
- **`solution2_app.py` / `solution2_client.py`** — **Flask REST API + BeautifulSoup 웹 스크래핑**(390줄): 외부 페이지에서 출판물 목록을 크롤링하여 CSV로 관리하고 JSON API(`/`, `request`/`jsonify`)로 제공.
- **`solution3.py`** — `threading` + `urllib.parse.quote`를 활용한 서버 동작 분석/공격 시연 스크립트(보안 관점 실습, 입력 검증의 중요성).

---

## 핵심 기술
- TCP/UDP 소켓, **HTTP 프로토콜을 원시 소켓으로 직접 파싱·생성**
- **TLS/SSL** 암호화 통신(인증서·컨텍스트 구성)
- 바이트 스트림 **직렬화·프레이밍**(`pickle`+`struct`, 길이 프리픽스)
- **비동기 I/O**(`asyncio`, StreamReader/Writer, 이벤트 루프)로 다중 클라이언트 동시 처리
- **분산 메시징**(ZeroMQ PUB/PULL + Poller) + **캐시(memcached)** 기반 작업 분산·집계
- **Flask** REST API, **웹 스크래핑**(BeautifulSoup), 동시성·네트워크 보안

## 실행 (예시)
```bash
python solution2_server.py &   # 서버 먼저
python solution2_client.py
# 추가 패키지: pip install pyzmq pymemcache flask requests beautifulsoup4
```
