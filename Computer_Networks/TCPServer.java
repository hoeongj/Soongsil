import java.io.*;
import java.net.*;

// TCP 소켓 서버: 6789 포트에서 클라이언트 접속을 기다리다가, 클라이언트가 보낸
// 문자열을 받아 출력하고 학번(20221528)을 응답으로 돌려준 뒤 종료한다.
class TCPServer {
    public static void main(String argv[]) throws Exception {
        String clientSentence; // Client에서 data를 보내면 받고 저장할 변수 선언.

        ServerSocket welcomeSocket = new ServerSocket(6789);
        // ServerSocket을 통해 welcomeSocket 객체에 6789 포트로 소켓을 만들고 자동으로 내부에서 바인딩을 함
        // 서버는 6789라는 포트번호로 데이터를 받을 준비가 됨
        System.out.println("socket 생성 완료. port : 6789");

        while (true) { // 데이터 받을 때까지 계속 기다림 - Listen 돌입
            System.out.println("Listen 상태 돌입");

            Socket connectionSocket = welcomeSocket.accept();
            // 소스코드는 이 구간에서 멈춰 있다가 Client가 접속을 하면 welcomeSocket에 accept가 일어남
            // accept가 일어나면 일반적으로 통신을 할 connectionSocket이 만들어짐
            System.out.println("accept가 일어남");

            BufferedReader inFromClient = new BufferedReader(new InputStreamReader(connectionSocket.getInputStream()));
            // Client로부터 data를 받기 위한 InputStream 생성

            DataOutputStream outToClient = new DataOutputStream(connectionSocket.getOutputStream());
            // Client에게 data를 보내기 위한 OutputStream 생성

            clientSentence = inFromClient.readLine();
            // client가 접속하면 보낸 data를 바로 clientSentence에 저장

            System.out.println("클라이언트가 보낸 메시지 : " + clientSentence);
            // client에서 보낸 메시지를 바로 console에 출력

            outToClient.writeBytes("20221528\n");
            // 마지막으로 Client에 학번을 보냄.
            System.out.println("Client로 학번이 전송됨");

            System.out.println("Server를 종료합니다.");
            connectionSocket.close();
            break;
        }
    }
}
