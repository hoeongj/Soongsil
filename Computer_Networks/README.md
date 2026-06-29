# 🔌 Computer Networks (컴퓨터 네트워크)

Java 소켓 API로 **TCP / UDP 클라이언트–서버 통신 프로그램**을 구현하여 두 전송 계층 프로토콜의 동작 차이를 직접 확인한 과목입니다.

> **기술 스택:** Java · `ServerSocket`/`Socket`(TCP) · `DatagramSocket`/`DatagramPacket`(UDP) · `BufferedReader`/`DataOutputStream` · `InetAddress`

소스: `TCPServer.java`, `TCPClient.java`, `UDPServer.java`, `UDPClient.java` · 보고서: `report_TCP_socket.pdf`, `report_UDP_socket.pdf`

> 📝 `.java` 파일은 제출 보고서(PDF)의 소스 화면을 그대로 옮긴 것이며(`javac` 컴파일 검증 완료), 보고서에는 실제 실행 결과 화면이 함께 담겨 있습니다.

---

## 📂 TCP 소켓 프로그램 — 연결 지향(connection-oriented)
- **`TCPServer.java`** — 6789 포트에서 `ServerSocket`을 만들고 `accept()`로 접속 대기 → 클라이언트 문장을 `BufferedReader.readLine()`으로 수신·출력 → `DataOutputStream.writeBytes()`로 학번(20221528) 응답 → 연결 종료.
- **`TCPClient.java`** — `new Socket("localhost", 6789)`로 연결 → 키보드 입력을 서버로 전송 → 서버 응답(학번) 수신·출력.
- **소켓 생명주기:** `socket → bind/listen → accept → (read/write 스트림) → close` — `accept()`가 블로킹되어 연결이 수립되면 통신용 소켓(`connectionSocket`)이 별도로 생성되는 구조를 직접 확인.

## 📂 UDP 소켓 프로그램 — 비연결(connectionless)
- **`UDPServer.java`** — 9876 포트 `DatagramSocket`에서 `receive()`로 패킷 수신 → 보낸 이름 출력 → **패킷에서 추출한 발신자 주소/포트**(`getAddress`/`getPort`)로 학번을 `send()`.
- **`UDPClient.java`** — 이름을 `DatagramPacket`에 담아 `send()`(포트는 OS가 자동 할당) → 서버 응답을 `receive()`로 수신·출력.
- **데이터그램 모델:** 연결 수립 없이 패킷 단위로 송수신하며, 각 패킷이 목적지 주소를 직접 담는 구조를 확인.

## 🔍 TCP vs UDP — 직접 구현하며 체감한 차이

| 구분 | TCP (이 과제) | UDP (이 과제) |
|------|---------------|---------------|
| 연결 | `accept()`로 연결 수립 후 통신 | 연결 없이 즉시 패킷 송수신 |
| 핵심 클래스 | `ServerSocket`, `Socket` | `DatagramSocket`, `DatagramPacket` |
| 데이터 단위 | 바이트 **스트림**(`BufferedReader`/`DataOutputStream`) | **데이터그램**(byte[] 패킷) |
| 주소 지정 | 연결 시 1회 | **매 패킷마다** 주소/포트 지정 |
| 신뢰성 | 순서·전달 보장 | 보장 없음(애플리케이션 책임) |

## 핵심 기술
- **TCP vs UDP**의 구조적 차이(연결 지향 스트림 vs 비연결 데이터그램) 이해
- 소켓 생성·바인딩·수신 대기·송수신·종료의 전체 생명주기
- 스트림 기반 I/O와 바이트 배열/패킷 처리, `InetAddress` 주소 변환
- 클라이언트–서버 모델 설계

> 💡 같은 과목군의 **Network Programming(Python)** 에서는 TLS·asyncio·ZeroMQ 등 한층 고급 주제를 다룹니다 → [../Network_Programming](../Network_Programming)

## 실행
```bash
javac TCPServer.java TCPClient.java && java TCPServer   # 터미널1 / java TCPClient 터미널2
javac UDPServer.java UDPClient.java && java UDPServer   # 터미널1 / java UDPClient 터미널2
```
