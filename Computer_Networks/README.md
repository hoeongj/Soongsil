# 🔌 Computer Networks (컴퓨터 네트워크)

Java 소켓 API로 **TCP / UDP 클라이언트–서버 통신 프로그램**을 구현하여 두 전송 계층 프로토콜의 동작 차이를 직접 확인한 과목입니다.

> **기술 스택:** Java · `ServerSocket`/`Socket`(TCP) · `DatagramSocket`/`DatagramPacket`(UDP) · `BufferedReader`/`DataOutputStream`

소스: `TCPServer.java`, `TCPClient.java`, `UDPServer.java`, `UDPClient.java` · 보고서: `report_TCP_socket.pdf`, `report_UDP_socket.pdf`

> 📝 `.java` 파일은 제출 보고서(PDF)의 소스 화면을 그대로 옮긴 것이며, 보고서에는 실제 실행 결과 화면이 함께 담겨 있습니다.

---

## 📂 TCP 소켓 프로그램
연결 지향(connection-oriented) 통신을 구현합니다.

- **`TCPServer.java`** — 6789 포트에서 `ServerSocket`으로 `accept()` 대기 → 클라이언트가 보낸 문장을 `BufferedReader`로 수신해 출력 → `DataOutputStream`으로 학번(20221528)을 응답 → 연결 종료.
- **`TCPClient.java`** — `localhost:6789`로 연결 → 키보드 입력 문장을 서버로 전송 → 서버 응답(학번)을 수신·출력.
- **흐름:** `socket → bind/listen → accept → read/write → close` 의 TCP 3-way 기반 연결 모델.

## 📂 UDP 소켓 프로그램
비연결(connectionless) 통신을 구현합니다.

- **`UDPServer.java`** — 9876 포트의 `DatagramSocket`에서 `receive()`로 패킷 수신 → 클라이언트가 보낸 이름 출력 → 패킷에서 얻은 **발신자 주소/포트로** 학번을 `send()`.
- **`UDPClient.java`** — 이름을 `DatagramPacket`에 담아 서버로 `send()` → 서버 응답(학번)을 `receive()`로 수신·출력.
- **흐름:** 연결 수립 없이 **데이터그램 단위**로 송수신하며, 송신 측 주소를 패킷에서 추출해 응답.

---

## 핵심 기술
- **TCP vs UDP**: 연결 지향 스트림(`Socket`) vs 비연결 데이터그램(`DatagramSocket`)의 구조적 차이 이해
- 소켓 생성·바인딩·수신 대기·송수신·종료의 전체 생명주기
- 스트림 기반 I/O(`BufferedReader`, `DataOutputStream`)와 바이트 배열/패킷 처리
- 클라이언트–서버 모델 설계

> 💡 같은 과목군의 **Network Programming(Python)** 에서는 TLS·asyncio·ZeroMQ 등 한층 고급 주제를 다룹니다 → [../Network_Programming](../Network_Programming)

## 실행
```bash
javac TCPServer.java TCPClient.java
java TCPServer        # 터미널 1
java TCPClient        # 터미널 2

javac UDPServer.java UDPClient.java
java UDPServer        # 터미널 1
java UDPClient        # 터미널 2
```
